/**
 * MapClass Implementation
 *
 * Map data management and cell access.
 */

#include "game/map.h"
#include "game/cell.h"
#include "platform.h"
#include <cstring>

// =============================================================================
// Theater Data
// =============================================================================

const char* TheaterNames[THEATER_COUNT] = {
    "TEMPERATE",
    "SNOW",
    "INTERIOR"
};

const char* TheaterFileSuffix[THEATER_COUNT] = {
    "TEM",  // TEMPERAT.MIX
    "SNO",  // SNOW.MIX
    "INT"   // INTERIOR.MIX
};

// =============================================================================
// Global Map Pointer
// =============================================================================

MapClass* Map = nullptr;

// =============================================================================
// Static Members
// =============================================================================

CellClass MapClass::dummy_cell_;

// =============================================================================
// Construction / Destruction
// =============================================================================

MapClass::MapClass()
    : GScreenClass()
    , cells_(nullptr)
    , cells_owned_(false)
    , map_x_(1)
    , map_y_(1)
    , map_width_(MAP_CELL_WIDTH - 2)
    , map_height_(MAP_CELL_HEIGHT - 2)
    , theater_type_(THEATER_NONE)
    , tactical_pos_(0)
{
}

MapClass::~MapClass() {
    if (cells_owned_ && cells_ != nullptr) {
        delete[] cells_;
        cells_ = nullptr;
    }
}

// =============================================================================
// Lifecycle Methods
// =============================================================================

void MapClass::One_Time() {
    // Call base class first
    GScreenClass::One_Time();

    // Allocate cell array
    if (cells_ == nullptr) {
        cells_ = new CellClass[MAP_CELL_TOTAL];
        cells_owned_ = true;

        if (cells_ == nullptr) {
            Platform_LogError("MapClass::One_Time: Failed to allocate cells");
            return;
        }

        // Initialize all cells
        for (int i = 0; i < MAP_CELL_TOTAL; i++) {
            cells_[i].Clear();
            cells_[i].Set_Cell_Index((CELL)i);
        }

        Platform_LogInfo("MapClass::One_Time: Cells allocated");
    }
}

void MapClass::Init(int theater) {
    // Call base class first
    GScreenClass::Init(theater);

    // Set theater type
    if (theater >= 0 && theater < THEATER_COUNT) {
        theater_type_ = (TheaterType)theater;
    } else {
        theater_type_ = THEATER_TEMPERATE;
    }

    // Reset map bounds to full map minus edge
    map_x_ = 1;
    map_y_ = 1;
    map_width_ = MAP_CELL_WIDTH - 2;
    map_height_ = MAP_CELL_HEIGHT - 2;

    // Reset scroll position
    tactical_pos_ = ::Cell_Coord(XY_Cell(map_x_, map_y_));

    // Clear all cells
    Clear_Map();

    Platform_LogInfo("MapClass::Init complete");
}

// =============================================================================
// Cell Access
// =============================================================================

CellClass& MapClass::operator[](CELL cell) {
    if (!Is_Valid_Cell(cell) || cells_ == nullptr) {
        return dummy_cell_;
    }
    // Convert CELL format (Y<<8|X) to array index (Y*128+X)
    int x = Cell_X(cell);
    int y = Cell_Y(cell);
    return cells_[y * MAP_CELL_WIDTH + x];
}

const CellClass& MapClass::operator[](CELL cell) const {
    if (!Is_Valid_Cell(cell) || cells_ == nullptr) {
        return dummy_cell_;
    }
    int x = Cell_X(cell);
    int y = Cell_Y(cell);
    return cells_[y * MAP_CELL_WIDTH + x];
}

CellClass* MapClass::Cell_At(int x, int y) {
    if (!Is_Valid_XY(x, y) || cells_ == nullptr) {
        return nullptr;
    }
    return &cells_[y * MAP_CELL_WIDTH + x];
}

const CellClass* MapClass::Cell_At(int x, int y) const {
    if (!Is_Valid_XY(x, y) || cells_ == nullptr) {
        return nullptr;
    }
    return &cells_[y * MAP_CELL_WIDTH + x];
}

CellClass* MapClass::Get_Cell_At_Coord(COORDINATE coord) {
    if (coord == COORD_NONE) {
        return nullptr;
    }
    // Extract cell X/Y from coordinate's cell components
    int x = Coord_XCell(coord);
    int y = Coord_YCell(coord);
    return Cell_At(x, y);
}

bool MapClass::Is_Valid_Cell(CELL cell) const {
    if (cell == CELL_NONE) {
        return false;
    }
    int x = Cell_X(cell);
    int y = Cell_Y(cell);
    return (x >= 0 && x < MAP_CELL_WIDTH && y >= 0 && y < MAP_CELL_HEIGHT);
}

bool MapClass::Is_Valid_XY(int x, int y) const {
    return (x >= 0 && x < MAP_CELL_WIDTH && y >= 0 && y < MAP_CELL_HEIGHT);
}

// =============================================================================
// Map Bounds
// =============================================================================

void MapClass::Set_Map_Bounds(int x, int y, int w, int h) {
    // Clamp to valid range
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (w < 1) w = 1;
    if (h < 1) h = 1;

    if (x + w > MAP_CELL_WIDTH) w = MAP_CELL_WIDTH - x;
    if (y + h > MAP_CELL_HEIGHT) h = MAP_CELL_HEIGHT - y;

    map_x_ = x;
    map_y_ = y;
    map_width_ = w;
    map_height_ = h;

    Platform_LogInfo("MapClass: Bounds set");
}

bool MapClass::Is_In_Bounds(CELL cell) const {
    if (cell == CELL_NONE) {
        return false;
    }
    return Is_In_Bounds(Cell_X(cell), Cell_Y(cell));
}

bool MapClass::Is_In_Bounds(int x, int y) const {
    return (x >= map_x_ && x < map_x_ + map_width_ &&
            y >= map_y_ && y < map_y_ + map_height_);
}

void MapClass::Clamp_To_Bounds(int& x, int& y) const {
    if (x < map_x_) x = map_x_;
    if (y < map_y_) y = map_y_;
    if (x >= map_x_ + map_width_) x = map_x_ + map_width_ - 1;
    if (y >= map_y_ + map_height_) y = map_y_ + map_height_ - 1;
}

CELL MapClass::Clamp_To_Bounds(CELL cell) const {
    int x = Cell_X(cell);
    int y = Cell_Y(cell);
    Clamp_To_Bounds(x, y);
    return XY_Cell(x, y);
}

// =============================================================================
// Map Operations
// =============================================================================

void MapClass::Clear_Map() {
    if (cells_ == nullptr) {
        return;
    }

    for (int i = 0; i < MAP_CELL_TOTAL; i++) {
        cells_[i].Clear();
        cells_[i].Set_Cell_Index((CELL)i);
    }
}

void MapClass::Recalc_All() {
    if (cells_ == nullptr) {
        return;
    }

    // Iterate through all cells in bounds and recalculate
    for (int y = map_y_; y < map_y_ + map_height_; y++) {
        for (int x = map_x_; x < map_x_ + map_width_; x++) {
            cells_[y * MAP_CELL_WIDTH + x].Recalc();
        }
    }
}

// =============================================================================
// Coordinate Utilities
// =============================================================================

CELL MapClass::Pick_Cell(int screen_x, int screen_y) const {
    // Convert screen position to map position
    int tac_x = Coord_XPixel(tactical_pos_);
    int tac_y = Coord_YPixel(tactical_pos_);

    int map_pixel_x = screen_x + tac_x;
    int map_pixel_y = screen_y + tac_y;

    int cell_x = map_pixel_x / CELL_PIXEL_SIZE;
    int cell_y = map_pixel_y / CELL_PIXEL_SIZE;

    if (!Is_Valid_XY(cell_x, cell_y)) {
        return CELL_NONE;
    }

    return XY_Cell(cell_x, cell_y);
}

bool MapClass::Cell_To_Screen(CELL cell, int& screen_x, int& screen_y) const {
    if (!Is_Valid_Cell(cell)) {
        return false;
    }

    int tac_x = Coord_XPixel(tactical_pos_);
    int tac_y = Coord_YPixel(tactical_pos_);

    int cell_x = Cell_X(cell);
    int cell_y = Cell_Y(cell);

    screen_x = (cell_x * CELL_PIXEL_SIZE) - tac_x;
    screen_y = (cell_y * CELL_PIXEL_SIZE) - tac_y;

    // Check if on screen
    return (screen_x >= -CELL_PIXEL_SIZE && screen_x < width_ &&
            screen_y >= -CELL_PIXEL_SIZE && screen_y < height_);
}

void MapClass::Set_Tactical_Position(COORDINATE pos) {
    // Clamp to valid range
    int max_x = (map_x_ + map_width_) * LEPTON_PER_CELL - (width_ * SUBCELL_PER_CELL / CELL_PIXEL_SIZE);
    int max_y = (map_y_ + map_height_) * LEPTON_PER_CELL - (height_ * SUBCELL_PER_CELL / CELL_PIXEL_SIZE);
    int min_x = map_x_ * LEPTON_PER_CELL;
    int min_y = map_y_ * LEPTON_PER_CELL;

    int x = Coord_X(pos);
    int y = Coord_Y(pos);

    if (x < min_x) x = min_x;
    if (y < min_y) y = min_y;
    if (x > max_x) x = max_x;
    if (y > max_y) y = max_y;

    tactical_pos_ = XY_Coord(x, y);
}
