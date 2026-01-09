/**
 * Coordinate System Implementation
 *
 * Provides all coordinate conversion and math functions for the
 * Red Alert coordinate system.
 */

#include "game/coord.h"
#include "game/facing.h"
#include <cmath>

// =============================================================================
// Direction Offset Tables
// =============================================================================

// X offsets for 8 directions (N, NE, E, SE, S, SW, W, NW)
const int8_t FacingOffset_X[8] = { 0,  1, 1, 1, 0, -1, -1, -1 };

// Y offsets for 8 directions (N, NE, E, SE, S, SW, W, NW)
const int8_t FacingOffset_Y[8] = { -1, -1, 0, 1, 1,  1,  0, -1 };

// Cell index offsets for 8 directions
// Format: (y_offset << 8) | x_offset
const int16_t FacingOffset_Cell[8] = {
    -MAP_CELL_WIDTH,      // N:  y-1
    -MAP_CELL_WIDTH + 1,  // NE: y-1, x+1
    1,                    // E:  x+1
    MAP_CELL_WIDTH + 1,   // SE: y+1, x+1
    MAP_CELL_WIDTH,       // S:  y+1
    MAP_CELL_WIDTH - 1,   // SW: y+1, x-1
    -1,                   // W:  x-1
    -MAP_CELL_WIDTH - 1   // NW: y-1, x-1
};

// =============================================================================
// Coordinate Conversion Functions
// =============================================================================

extern "C" {

/**
 * Convert CELL to COORDINATE (centers on cell)
 *
 * Coordinate format: X in high 16 bits, Y in low 16 bits
 * Each 16-bit value = (cell << 8) | subcell
 * Cell center has subcell = 128 (half of 256)
 */
COORDINATE Cell_Coord(CELL cell) {
    if (cell == CELL_NONE) {
        return COORD_NONE;
    }

    int x = Cell_X(cell);
    int y = Cell_Y(cell);

    // Center of cell: (cell << 8) | 128
    // = (cell * 256) + 128
    int x_lepton = (x << SUBCELL_SHIFT) + (SUBCELL_PER_CELL / 2);
    int y_lepton = (y << SUBCELL_SHIFT) + (SUBCELL_PER_CELL / 2);

    return XY_Coord(x_lepton, y_lepton);
}

/**
 * Convert COORDINATE to CELL
 *
 * Extract cell index from the high 8 bits of each 16-bit coordinate
 */
CELL Coord_Cell(COORDINATE coord) {
    if (coord == COORD_NONE) {
        return CELL_NONE;
    }

    int x_lepton = Coord_X(coord);
    int y_lepton = Coord_Y(coord);

    // Cell = lepton >> 8 (divide by 256)
    int x_cell = x_lepton >> SUBCELL_SHIFT;
    int y_cell = y_lepton >> SUBCELL_SHIFT;

    // Clamp to valid range
    if (x_cell < 0) x_cell = 0;
    if (x_cell >= MAP_CELL_WIDTH) x_cell = MAP_CELL_WIDTH - 1;
    if (y_cell < 0) y_cell = 0;
    if (y_cell >= MAP_CELL_HEIGHT) y_cell = MAP_CELL_HEIGHT - 1;

    return XY_Cell(x_cell, y_cell);
}

/**
 * Get distance between two coordinates (in leptons)
 * Uses fast integer approximation
 */
int Coord_Distance(COORDINATE coord1, COORDINATE coord2) {
    if (coord1 == COORD_NONE || coord2 == COORD_NONE) {
        return 0x7FFFFFFF; // Max distance for invalid coords
    }

    int dx = Coord_X(coord2) - Coord_X(coord1);
    int dy = Coord_Y(coord2) - Coord_Y(coord1);

    // Fast distance approximation (octagonal)
    // More accurate than max(dx,dy), faster than sqrt
    int adx = (dx < 0) ? -dx : dx;
    int ady = (dy < 0) ? -dy : dy;

    if (adx > ady) {
        return adx + (ady >> 1);
    } else {
        return ady + (adx >> 1);
    }
}

/**
 * Calculate direction from one coordinate to another
 * Returns 0-255 direction value
 */
uint8_t Coord_Direction(COORDINATE from, COORDINATE to) {
    if (from == COORD_NONE || to == COORD_NONE) {
        return 0;
    }

    int dx = Coord_X(to) - Coord_X(from);
    int dy = Coord_Y(to) - Coord_Y(from);

    // atan2 returns -PI to PI, we need 0-255
    // North (up) = 0, East = 64, South = 128, West = 192
    double angle = atan2((double)dx, (double)-dy);

    // Convert to 0-255 range
    int dir = (int)((angle / 3.14159265358979) * 128.0);
    if (dir < 0) dir += 256;

    return (uint8_t)dir;
}

/**
 * Move coordinate by distance in given direction
 */
COORDINATE Coord_Move(COORDINATE coord, uint8_t direction, int distance) {
    if (coord == COORD_NONE || distance == 0) {
        return coord;
    }

    // Direction to radians (0 = north/up = -Y)
    double angle = (direction / 128.0) * 3.14159265358979;

    int dx = (int)(sin(angle) * distance);
    int dy = (int)(-cos(angle) * distance);

    int x = Coord_X(coord) + dx;
    int y = Coord_Y(coord) + dy;

    // Clamp to valid range
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= MAX_COORD_VALUE) x = MAX_COORD_VALUE - 1;
    if (y >= MAX_COORD_VALUE) y = MAX_COORD_VALUE - 1;

    return XY_Coord(x, y);
}

/**
 * Check if coordinate is within map bounds
 */
int Coord_InMap(COORDINATE coord) {
    if (coord == COORD_NONE) return 0;

    int x = Coord_X(coord);
    int y = Coord_Y(coord);

    return (x >= 0 && x < MAX_COORD_VALUE &&
            y >= 0 && y < MAX_COORD_VALUE);
}

/**
 * Check if cell is within map bounds
 */
int Cell_InMap(CELL cell) {
    if (cell == CELL_NONE) return 0;

    int x = Cell_X(cell);
    int y = Cell_Y(cell);

    return (x >= 0 && x < MAP_CELL_WIDTH &&
            y >= 0 && y < MAP_CELL_HEIGHT);
}

/**
 * Get adjacent cell in given direction
 */
CELL Adjacent_Cell(CELL cell, int direction) {
    if (cell == CELL_NONE || direction < 0 || direction >= FACING_COUNT) {
        return CELL_NONE;
    }

    int x = Cell_X(cell) + FacingOffset_X[direction];
    int y = Cell_Y(cell) + FacingOffset_Y[direction];

    if (x < 0 || x >= MAP_CELL_WIDTH || y < 0 || y >= MAP_CELL_HEIGHT) {
        return CELL_NONE;
    }

    return XY_Cell(x, y);
}

/**
 * Snap coordinate to cell center
 */
COORDINATE Coord_Snap(COORDINATE coord) {
    return Cell_Coord(Coord_Cell(coord));
}

/**
 * Convert pixel coordinates to world coordinate
 *
 * Inverse of Coord_XPixel: pixel = lepton * 24 / 256
 * So: lepton = pixel * 256 / 24 = pixel * 32 / 3 (approximately)
 */
COORDINATE Pixel_To_Coord(int pixel_x, int pixel_y) {
    // lepton = pixel * 256 / 24 = pixel * 32 / 3
    // Use integer math with rounding
    int x_lepton = (pixel_x * SUBCELL_PER_CELL) / CELL_PIXEL_SIZE;
    int y_lepton = (pixel_y * SUBCELL_PER_CELL) / CELL_PIXEL_SIZE;

    return XY_Coord(x_lepton, y_lepton);
}

/**
 * Convert world coordinate to pixel coordinates
 */
void Coord_To_Pixel(COORDINATE coord, int* pixel_x, int* pixel_y) {
    if (pixel_x) *pixel_x = Coord_XPixel(coord);
    if (pixel_y) *pixel_y = Coord_YPixel(coord);
}

/**
 * Calculate cell distance (Chebyshev - max of x,y distance)
 */
int Cell_Distance(CELL cell1, CELL cell2) {
    if (cell1 == CELL_NONE || cell2 == CELL_NONE) {
        return 0x7FFF;
    }

    int dx = Cell_X(cell2) - Cell_X(cell1);
    int dy = Cell_Y(cell2) - Cell_Y(cell1);

    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;

    return (dx > dy) ? dx : dy;
}

// =============================================================================
// Direction Functions
// =============================================================================

DirType Dir_Opposite(DirType dir) {
    return (DirType)(dir + 128);
}

int Dir_To_8Way(DirType dir) {
    // Divide 256 directions into 8 sectors
    // Each sector is 32 directions wide, centered on 0, 32, 64, etc.
    return ((dir + 16) >> 5) & 7;
}

int Dir_To_16Way(DirType dir) {
    return ((dir + 8) >> 4) & 15;
}

int Dir_To_32Way(DirType dir) {
    return ((dir + 4) >> 3) & 31;
}

DirType Way8_To_Dir(int way) {
    return (DirType)((way & 7) << 5);
}

int8_t Dir_Delta(DirType from, DirType to) {
    int diff = (int)to - (int)from;

    // Wrap to -128..+127 range
    if (diff > 127) diff -= 256;
    if (diff < -128) diff += 256;

    return (int8_t)diff;
}

DirType Dir_From_XY(int dx, int dy) {
    if (dx == 0 && dy == 0) return DIR_N;

    double angle = atan2((double)dx, (double)-dy);
    int dir = (int)((angle / 3.14159265358979) * 128.0);
    if (dir < 0) dir += 256;

    return (DirType)dir;
}

int Dir_X_Factor(DirType dir) {
    // sin(dir * 2 * PI / 256) * 256
    double angle = (dir / 128.0) * 3.14159265358979;
    return (int)(sin(angle) * 256);
}

int Dir_Y_Factor(DirType dir) {
    // -cos(dir * 2 * PI / 256) * 256
    double angle = (dir / 128.0) * 3.14159265358979;
    return (int)(-cos(angle) * 256);
}

} // extern "C"

// =============================================================================
// FacingClass Methods (C++)
// =============================================================================

int8_t FacingClass::Difference() const {
    return Dir_Delta(current_, desired_);
}

int FacingClass::Rotation_Direction() const {
    int8_t diff = Difference();
    if (diff > 0) return 1;   // Clockwise
    if (diff < 0) return -1;  // Counter-clockwise
    return 0;                  // No rotation needed
}

bool FacingClass::Rotate() {
    if (current_ == desired_) {
        return false;
    }

    // Instant rotation if rate is 0
    if (rate_ == 0) {
        current_ = desired_;
        return true;
    }

    // Calculate rotation amount
    int8_t diff = Difference();
    int step = rate_;

    // Clamp step to not overshoot
    if (diff > 0 && step > diff) step = diff;
    if (diff < 0 && step > -diff) step = -diff;

    // Apply rotation
    if (diff > 0) {
        current_ = (DirType)(current_ + step);
    } else {
        current_ = (DirType)(current_ - step);
    }

    return true;
}

// =============================================================================
// Direction Tables
// =============================================================================

const int8_t DirSineTable[256] = {
    0, 3, 6, 9, 12, 15, 18, 21, 24, 28, 31, 34, 37, 40, 43, 46,
    49, 51, 54, 57, 60, 63, 65, 68, 71, 73, 76, 78, 81, 83, 85, 88,
    90, 92, 94, 96, 98, 100, 102, 104, 106, 107, 109, 111, 112, 113, 115, 116,
    117, 118, 120, 121, 122, 122, 123, 124, 125, 125, 126, 126, 126, 127, 127, 127,
    127, 127, 127, 127, 126, 126, 126, 125, 125, 124, 123, 122, 122, 121, 120, 118,
    117, 116, 115, 113, 112, 111, 109, 107, 106, 104, 102, 100, 98, 96, 94, 92,
    90, 88, 85, 83, 81, 78, 76, 73, 71, 68, 65, 63, 60, 57, 54, 51,
    49, 46, 43, 40, 37, 34, 31, 28, 24, 21, 18, 15, 12, 9, 6, 3,
    0, -3, -6, -9, -12, -15, -18, -21, -24, -28, -31, -34, -37, -40, -43, -46,
    -49, -51, -54, -57, -60, -63, -65, -68, -71, -73, -76, -78, -81, -83, -85, -88,
    -90, -92, -94, -96, -98, -100, -102, -104, -106, -107, -109, -111, -112, -113, -115, -116,
    -117, -118, -120, -121, -122, -122, -123, -124, -125, -125, -126, -126, -126, -127, -127, -127,
    -127, -127, -127, -127, -126, -126, -126, -125, -125, -124, -123, -122, -122, -121, -120, -118,
    -117, -116, -115, -113, -112, -111, -109, -107, -106, -104, -102, -100, -98, -96, -94, -92,
    -90, -88, -85, -83, -81, -78, -76, -73, -71, -68, -65, -63, -60, -57, -54, -51,
    -49, -46, -43, -40, -37, -34, -31, -28, -24, -21, -18, -15, -12, -9, -6, -3
};

const int8_t DirCosineTable[256] = {
    127, 127, 127, 127, 126, 126, 126, 125, 125, 124, 123, 122, 122, 121, 120, 118,
    117, 116, 115, 113, 112, 111, 109, 107, 106, 104, 102, 100, 98, 96, 94, 92,
    90, 88, 85, 83, 81, 78, 76, 73, 71, 68, 65, 63, 60, 57, 54, 51,
    49, 46, 43, 40, 37, 34, 31, 28, 24, 21, 18, 15, 12, 9, 6, 3,
    0, -3, -6, -9, -12, -15, -18, -21, -24, -28, -31, -34, -37, -40, -43, -46,
    -49, -51, -54, -57, -60, -63, -65, -68, -71, -73, -76, -78, -81, -83, -85, -88,
    -90, -92, -94, -96, -98, -100, -102, -104, -106, -107, -109, -111, -112, -113, -115, -116,
    -117, -118, -120, -121, -122, -122, -123, -124, -125, -125, -126, -126, -126, -127, -127, -127,
    -127, -127, -127, -127, -126, -126, -126, -125, -125, -124, -123, -122, -122, -121, -120, -118,
    -117, -116, -115, -113, -112, -111, -109, -107, -106, -104, -102, -100, -98, -96, -94, -92,
    -90, -88, -85, -83, -81, -78, -76, -73, -71, -68, -65, -63, -60, -57, -54, -51,
    -49, -46, -43, -40, -37, -34, -31, -28, -24, -21, -18, -15, -12, -9, -6, -3,
    0, 3, 6, 9, 12, 15, 18, 21, 24, 28, 31, 34, 37, 40, 43, 46,
    49, 51, 54, 57, 60, 63, 65, 68, 71, 73, 76, 78, 81, 83, 85, 88,
    90, 92, 94, 96, 98, 100, 102, 104, 106, 107, 109, 111, 112, 113, 115, 116,
    117, 118, 120, 121, 122, 122, 123, 124, 125, 125, 126, 126, 126, 127, 127, 127
};

const uint8_t Dir8Way[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
