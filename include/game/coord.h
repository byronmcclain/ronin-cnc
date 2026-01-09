/**
 * Red Alert Coordinate System
 *
 * The original game uses a fixed-point coordinate system where world positions
 * are stored as 32-bit "leptons" (1/256th of a pixel). This allows sub-pixel
 * precision for smooth movement while using integer math.
 *
 * Terminology:
 * - LEPTON: Smallest unit of measurement (1/256 pixel)
 * - CELL: 24x24 pixel terrain tile
 * - COORDINATE: Full world position (cell + sub-cell leptons)
 */

#pragma once

#include <cstdint>

// =============================================================================
// Core Coordinate Types
// =============================================================================

/**
 * COORDINATE - 32-bit world position in leptons
 *
 * Format:
 *   Bits 31-24: X cell (0-127)
 *   Bits 23-16: X sub-cell (0-255 leptons within cell)
 *   Bits 15-8:  Y cell (0-127)
 *   Bits 7-0:   Y sub-cell (0-255 leptons within cell)
 *
 * Alternative view:
 *   High word (bits 31-16): X position in leptons
 *   Low word (bits 15-0):   Y position in leptons
 */
typedef uint32_t COORDINATE;

/**
 * CELL - 16-bit cell index
 *
 * Format:
 *   Bits 15-8: Y cell (0-127)
 *   Bits 7-0:  X cell (0-127)
 *
 * This provides direct array indexing: Cells[cell]
 */
typedef int16_t CELL;

/**
 * LEPTON - Sub-pixel measurement unit
 * 256 leptons = 1 pixel
 * 256 * 24 = 6144 leptons = 1 cell
 */
typedef int16_t LEPTON;

// =============================================================================
// Constants
// =============================================================================

// Sub-cell resolution (256 units per cell)
constexpr int SUBCELL_SHIFT = 8;                   // Bits per subcell
constexpr int SUBCELL_PER_CELL = 256;              // Subcell units per cell

// Cell dimensions in pixels
constexpr int CELL_PIXEL_SIZE = 24;                // Pixels per cell

// Lepton constants (for convenience - matches subcell)
constexpr int LEPTON_SHIFT = SUBCELL_SHIFT;
constexpr int LEPTON_PER_PIXEL = SUBCELL_PER_CELL / CELL_PIXEL_SIZE; // ~10.67, but we use 256/24
constexpr int LEPTON_PER_CELL = SUBCELL_PER_CELL;  // 256 (for this coordinate system)

// Map dimensions (standard scenario map)
constexpr int MAP_CELL_WIDTH = 128;
constexpr int MAP_CELL_HEIGHT = 128;
constexpr int MAP_CELL_TOTAL = MAP_CELL_WIDTH * MAP_CELL_HEIGHT;

// Map pixel dimensions
constexpr int MAP_PIXEL_WIDTH = MAP_CELL_WIDTH * CELL_PIXEL_SIZE;   // 3072
constexpr int MAP_PIXEL_HEIGHT = MAP_CELL_HEIGHT * CELL_PIXEL_SIZE; // 3072

// Max coordinate values (128 cells * 256 subcells = 32768)
constexpr int MAX_COORD_VALUE = MAP_CELL_WIDTH * SUBCELL_PER_CELL;  // 32768

// Invalid/null values
constexpr COORDINATE COORD_NONE = 0xFFFFFFFF;
constexpr CELL CELL_NONE = -1;

// =============================================================================
// Coordinate Macros (from original DEFINES.H)
// =============================================================================

/**
 * Extract X lepton from coordinate
 * Original: ((coord) >> 16)
 */
#define Coord_X(coord) ((int16_t)(((COORDINATE)(coord)) >> 16))

/**
 * Extract Y lepton from coordinate
 * Original: ((coord) & 0xFFFF)
 */
#define Coord_Y(coord) ((int16_t)(((COORDINATE)(coord)) & 0xFFFF))

/**
 * Extract X cell from coordinate
 */
#define Coord_XCell(coord) ((uint8_t)(((COORDINATE)(coord)) >> 24))

/**
 * Extract Y cell from coordinate
 */
#define Coord_YCell(coord) ((uint8_t)((((COORDINATE)(coord)) >> 8) & 0xFF))

/**
 * Extract X pixel from coordinate
 * Formula: (lepton / 256) * 24 = lepton * 24 / 256
 * Simplified: lepton * 24 >> 8 = lepton * 3 >> 5 (approximate)
 * Or: (cell * 24) + (subcell * 24 / 256)
 */
#define Coord_XPixel(coord) ((Coord_X(coord) * CELL_PIXEL_SIZE) >> SUBCELL_SHIFT)

/**
 * Extract Y pixel from coordinate
 */
#define Coord_YPixel(coord) ((Coord_Y(coord) * CELL_PIXEL_SIZE) >> SUBCELL_SHIFT)

/**
 * Build coordinate from X and Y leptons
 */
#define XY_Coord(x, y) ((COORDINATE)((((uint32_t)(x)) << 16) | ((uint32_t)(y) & 0xFFFF)))

/**
 * Build cell from X and Y cell indices
 */
#define XY_Cell(x, y) ((CELL)(((y) << 8) | ((x) & 0xFF)))

/**
 * Extract X from cell
 */
#define Cell_X(cell) ((int)((cell) & 0xFF))

/**
 * Extract Y from cell
 */
#define Cell_Y(cell) ((int)(((cell) >> 8) & 0xFF))

/**
 * Convert cell to array index
 */
#define Cell_Index(cell) ((int)(cell))

// =============================================================================
// Coordinate Functions (implementations in coord.cpp)
// =============================================================================

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Convert CELL to COORDINATE (centers on cell)
 * Returns coordinate at center of the given cell
 */
COORDINATE Cell_Coord(CELL cell);

/**
 * Convert COORDINATE to CELL
 * Returns cell containing the coordinate
 */
CELL Coord_Cell(COORDINATE coord);

/**
 * Get distance between two coordinates (in leptons)
 * Uses integer approximation of sqrt(dx^2 + dy^2)
 */
int Coord_Distance(COORDINATE coord1, COORDINATE coord2);

/**
 * Calculate direction from one coordinate to another
 * Returns 0-255 direction value (0=North, 64=East, 128=South, 192=West)
 */
uint8_t Coord_Direction(COORDINATE from, COORDINATE to);

/**
 * Move coordinate by distance in given direction
 */
COORDINATE Coord_Move(COORDINATE coord, uint8_t direction, int distance);

/**
 * Check if coordinate is within map bounds
 */
int Coord_InMap(COORDINATE coord);

/**
 * Check if cell is within map bounds
 */
int Cell_InMap(CELL cell);

/**
 * Get adjacent cell in given direction
 * Returns CELL_NONE if out of bounds
 */
CELL Adjacent_Cell(CELL cell, int direction);

/**
 * Snap coordinate to cell center
 */
COORDINATE Coord_Snap(COORDINATE coord);

/**
 * Convert pixel coordinates to world coordinate
 */
COORDINATE Pixel_To_Coord(int pixel_x, int pixel_y);

/**
 * Convert world coordinate to pixel coordinates
 */
void Coord_To_Pixel(COORDINATE coord, int* pixel_x, int* pixel_y);

/**
 * Calculate cell distance (Manhattan or Chebyshev)
 */
int Cell_Distance(CELL cell1, CELL cell2);

#ifdef __cplusplus
}
#endif

// =============================================================================
// Direction Offsets for Adjacent Cells
// =============================================================================

/**
 * Facing direction to cell offset mapping
 * Use: Adjacent_Cell(cell, FACING_N) to get cell to the north
 */
enum FacingType {
    FACING_N = 0,      // North (up)
    FACING_NE = 1,     // Northeast
    FACING_E = 2,      // East (right)
    FACING_SE = 3,     // Southeast
    FACING_S = 4,      // South (down)
    FACING_SW = 5,     // Southwest
    FACING_W = 6,      // West (left)
    FACING_NW = 7,     // Northwest
    FACING_COUNT = 8,
    FACING_NONE = -1
};

// Cell offsets for each facing direction
// Index with FacingType to get delta-cell value
extern const int8_t FacingOffset_X[8];
extern const int8_t FacingOffset_Y[8];
extern const int16_t FacingOffset_Cell[8];
