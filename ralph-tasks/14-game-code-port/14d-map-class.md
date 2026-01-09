# Task 14d: MapClass Foundation

## Dependencies
- Task 14c (GScreenClass Port) must be complete
- Task 14b (Coordinate Types) must be complete

## Context
MapClass is the first derived class from GScreenClass and manages the game's terrain map data. It stores the array of cells, handles coordinate validation, and provides basic map operations. This is the foundation that DisplayClass builds upon for rendering.

## Objective
Port the MapClass foundation:
1. Cell array storage and access
2. Map dimension management
3. Cell coordinate validation
4. Map bounds and wrapping
5. Basic map operations (clear, load placeholder)

## Technical Background

### Original MapClass (from CODE/MAP.H)
The original handles:
- 128x128 cell array (CellClass objects)
- Map bounds (visible play area within the full map)
- Cell access by CELL index or X,Y coordinates
- Coordinate validation and clamping

### Map vs. Visible Area
The full map is 128x128 cells, but scenarios define a smaller playable area within it. MapClass tracks both the full array and the current scenario bounds.

### Cell Storage
Cells are stored in a flat array indexed by CELL type:
```cpp
CellClass Cells[MAP_CELL_TOTAL];
CELL cell = XY_Cell(x, y);
CellClass& c = Cells[Cell_Index(cell)];
```

## Deliverables
- [ ] `include/game/map.h` - MapClass header
- [ ] `src/game/display/map.cpp` - MapClass implementation
- [ ] Map bounds tracking
- [ ] Cell array with accessor methods
- [ ] Coordinate validation functions
- [ ] Unit test for map operations
- [ ] Verification script passes

## Files to Create

### include/game/map.h (NEW)
```cpp
/**
 * MapClass - Map Data Management
 *
 * Manages the terrain map data including all cells, map bounds,
 * and basic cell access operations.
 *
 * Original location: CODE/MAP.H, CODE/MAP.CPP
 *
 * Class Hierarchy:
 *   GScreenClass
 *       |
 *       +-- MapClass        <- This class
 *           |
 *           +-- DisplayClass
 */

#pragma once

#include "game/gscreen.h"
#include "game/coord.h"
#include <cstdint>

// Forward declaration
class CellClass;

// =============================================================================
// Map Constants
// =============================================================================

// Theater types (terrain tileset themes)
enum TheaterType : int8_t {
    THEATER_NONE = -1,
    THEATER_TEMPERATE = 0,   // Green grass, trees
    THEATER_SNOW = 1,        // Winter terrain
    THEATER_INTERIOR = 2,    // Indoor missions

    THEATER_COUNT = 3
};

// Theater name strings
extern const char* TheaterNames[THEATER_COUNT];
extern const char* TheaterFileSuffix[THEATER_COUNT];

// =============================================================================
// MapClass
// =============================================================================

/**
 * MapClass - Base map data management
 *
 * Inherits from GScreenClass and adds:
 * - Cell array storage
 * - Map bounds (scenario play area)
 * - Cell access methods
 * - Coordinate validation
 */
class MapClass : public GScreenClass {
public:
    // -------------------------------------------------------------------------
    // Construction / Destruction
    // -------------------------------------------------------------------------

    MapClass();
    virtual ~MapClass();

    // -------------------------------------------------------------------------
    // Lifecycle Methods (override from GScreenClass)
    // -------------------------------------------------------------------------

    /**
     * One_Time - Called once at startup
     *
     * Initializes cell array and map data.
     */
    virtual void One_Time() override;

    /**
     * Init - Called at scenario start
     *
     * Resets map to initial state and loads theater.
     *
     * @param theater Theater type (TEMPERATE, SNOW, etc.)
     */
    virtual void Init(int theater = THEATER_TEMPERATE) override;

    // -------------------------------------------------------------------------
    // Cell Access
    // -------------------------------------------------------------------------

    /**
     * operator[] - Access cell by CELL index
     *
     * @param cell Cell index (XY_Cell format)
     * @return Reference to cell (or dummy cell if out of bounds)
     */
    CellClass& operator[](CELL cell);
    const CellClass& operator[](CELL cell) const;

    /**
     * Cell_At - Access cell by X,Y coordinates
     *
     * @param x Cell X (0-127)
     * @param y Cell Y (0-127)
     * @return Pointer to cell (or nullptr if invalid)
     */
    CellClass* Cell_At(int x, int y);
    const CellClass* Cell_At(int x, int y) const;

    /**
     * Cell_Coord - Access cell by world coordinate
     *
     * @param coord World coordinate
     * @return Pointer to cell containing coordinate
     */
    CellClass* Cell_Coord(COORDINATE coord);

    /**
     * Is_Valid_Cell - Check if cell index is valid
     */
    bool Is_Valid_Cell(CELL cell) const;

    /**
     * Is_Valid_XY - Check if X,Y is valid cell coordinate
     */
    bool Is_Valid_XY(int x, int y) const;

    // -------------------------------------------------------------------------
    // Map Bounds (Scenario Play Area)
    // -------------------------------------------------------------------------

    /**
     * Set_Map_Bounds - Set the playable area
     *
     * Scenarios don't use the full 128x128 map. This sets
     * the bounds of the actual play area.
     *
     * @param x Left edge cell X
     * @param y Top edge cell Y
     * @param w Width in cells
     * @param h Height in cells
     */
    void Set_Map_Bounds(int x, int y, int w, int h);

    /**
     * Get map bounds
     */
    int Map_Bounds_X() const { return map_x_; }
    int Map_Bounds_Y() const { return map_y_; }
    int Map_Bounds_Width() const { return map_width_; }
    int Map_Bounds_Height() const { return map_height_; }

    /**
     * Get map bounds as cell range
     */
    CELL Map_First_Cell() const { return XY_Cell(map_x_, map_y_); }
    CELL Map_Last_Cell() const { return XY_Cell(map_x_ + map_width_ - 1, map_y_ + map_height_ - 1); }

    /**
     * Is_In_Bounds - Check if cell is within scenario bounds
     */
    bool Is_In_Bounds(CELL cell) const;
    bool Is_In_Bounds(int x, int y) const;

    /**
     * Clamp_To_Bounds - Clamp coordinates to map bounds
     */
    void Clamp_To_Bounds(int& x, int& y) const;
    CELL Clamp_To_Bounds(CELL cell) const;

    // -------------------------------------------------------------------------
    // Map Operations
    // -------------------------------------------------------------------------

    /**
     * Clear_Map - Reset all cells to default state
     */
    void Clear_Map();

    /**
     * Recalc_All - Recalculate all cell data
     *
     * Called after map changes to update passability,
     * visibility, etc.
     */
    void Recalc_All();

    /**
     * Get_Theater - Get current theater type
     */
    TheaterType Get_Theater_Type() const { return theater_type_; }

    // -------------------------------------------------------------------------
    // Coordinate Utilities
    // -------------------------------------------------------------------------

    /**
     * Pick_Cell - Convert screen pixel to cell
     *
     * @param screen_x Screen X coordinate
     * @param screen_y Screen Y coordinate
     * @return Cell under screen position (accounting for scroll)
     */
    CELL Pick_Cell(int screen_x, int screen_y) const;

    /**
     * Cell_To_Screen - Convert cell to screen pixel
     *
     * @param cell Cell index
     * @param[out] screen_x Screen X coordinate
     * @param[out] screen_y Screen Y coordinate
     * @return true if cell is visible on screen
     */
    bool Cell_To_Screen(CELL cell, int& screen_x, int& screen_y) const;

    // -------------------------------------------------------------------------
    // Scroll Position (used by DisplayClass)
    // -------------------------------------------------------------------------

    /**
     * Get/set tactical view scroll position (in leptons)
     */
    COORDINATE Get_Tactical_Position() const { return tactical_pos_; }
    void Set_Tactical_Position(COORDINATE pos);

    /**
     * Get scroll position as cell
     */
    CELL Get_Tactical_Cell() const { return ::Coord_Cell(tactical_pos_); }

protected:
    // -------------------------------------------------------------------------
    // Protected Members
    // -------------------------------------------------------------------------

    // Cell storage
    CellClass* cells_;          // Array of cells [MAP_CELL_TOTAL]
    bool cells_owned_;          // Did we allocate cells_?

    // Map bounds (scenario play area)
    int map_x_;                 // Left edge cell X
    int map_y_;                 // Top edge cell Y
    int map_width_;             // Width in cells
    int map_height_;            // Height in cells

    // Theater
    TheaterType theater_type_;  // Current terrain theme

    // Tactical view position
    COORDINATE tactical_pos_;   // Top-left of viewport in leptons

    // Dummy cell for out-of-bounds access
    static CellClass dummy_cell_;
};

// =============================================================================
// Global Map Pointer
// =============================================================================

/**
 * Map - Global map instance
 *
 * Most game code accesses the map through this global.
 */
extern MapClass* Map;
```

### src/game/display/map.cpp (NEW)
```cpp
/**
 * MapClass Implementation
 *
 * Map data management and cell access.
 */

#include "game/map.h"
#include "game/cell.h"
#include "platform.h"
#include <cstring>
#include <cstdio>

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

        Platform_LogInfo("MapClass::One_Time: Allocated %d cells", MAP_CELL_TOTAL);
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
    tactical_pos_ = Cell_Coord(XY_Cell(map_x_, map_y_));

    // Clear all cells
    Clear_Map();

    Platform_LogInfo("MapClass::Init: Theater=%s, Bounds=(%d,%d,%d,%d)",
                     TheaterNames[theater_type_],
                     map_x_, map_y_, map_width_, map_height_);
}

// =============================================================================
// Cell Access
// =============================================================================

CellClass& MapClass::operator[](CELL cell) {
    if (!Is_Valid_Cell(cell) || cells_ == nullptr) {
        return dummy_cell_;
    }
    return cells_[Cell_Index(cell)];
}

const CellClass& MapClass::operator[](CELL cell) const {
    if (!Is_Valid_Cell(cell) || cells_ == nullptr) {
        return dummy_cell_;
    }
    return cells_[Cell_Index(cell)];
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

CellClass* MapClass::Cell_Coord(COORDINATE coord) {
    if (coord == COORD_NONE) {
        return nullptr;
    }
    return Cell_At(Coord_XCell(coord), Coord_YCell(coord));
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

    Platform_LogInfo("MapClass: Set bounds to (%d,%d) %dx%d",
                     map_x_, map_y_, map_width_, map_height_);
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
    int max_x = (map_x_ + map_width_) * LEPTON_PER_CELL - (width_ * LEPTON_PER_PIXEL);
    int max_y = (map_y_ + map_height_) * LEPTON_PER_CELL - (height_ * LEPTON_PER_PIXEL);
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
```

## Verification Command
```bash
ralph-tasks/verify/check-map-class.sh
```

## Verification Script

### ralph-tasks/verify/check-map-class.sh (NEW)
```bash
#!/bin/bash
# Verification script for MapClass foundation

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking MapClass Foundation ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check files exist
echo "[1/4] Checking files..."
check_file() {
    if [ ! -f "$1" ]; then
        echo "VERIFY_FAILED: $1 not found"
        exit 1
    fi
    echo "  OK: $1"
}

check_file "include/game/map.h"
check_file "src/game/display/map.cpp"

# Step 2: Syntax check
echo "[2/4] Checking syntax..."
if ! clang++ -std=c++17 -fsyntax-only -I include include/game/map.h 2>&1; then
    echo "VERIFY_FAILED: map.h has syntax errors"
    exit 1
fi
echo "  OK: Header syntax valid"

# Step 3: Build test with mocks
echo "[3/4] Building map test..."

# Create mock headers
mkdir -p /tmp/map_test/include/game
cat > /tmp/map_test/mock_platform.h << 'EOF'
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdarg>
enum PlatformResult { PLATFORM_RESULT_SUCCESS = 0 };
inline PlatformResult Platform_Graphics_Init() { return PLATFORM_RESULT_SUCCESS; }
inline PlatformResult Platform_Graphics_GetBackBuffer(uint8_t** buf, int32_t* w, int32_t* h, int32_t* p) {
    static uint8_t buffer[640*400];
    *buf = buffer; *w = 640; *h = 400; *p = 640;
    return PLATFORM_RESULT_SUCCESS;
}
inline void Platform_Graphics_Flip() {}
inline void Platform_Graphics_LockBackBuffer() {}
inline void Platform_Graphics_UnlockBackBuffer() {}
inline void Platform_Input_Update() {}
inline void Platform_Mouse_GetPosition(int32_t* x, int32_t* y) { *x = 0; *y = 0; }
inline bool Platform_Mouse_ButtonJustPressed(int) { return false; }
inline bool Platform_Mouse_ButtonJustReleased(int) { return false; }
inline bool Platform_Mouse_ButtonHeld(int) { return false; }
inline void Platform_LogInfo(const char* fmt, ...) { va_list a; va_start(a,fmt); vprintf(fmt,a); va_end(a); printf("\n"); }
inline void Platform_LogError(const char* fmt, ...) { va_list a; va_start(a,fmt); vprintf(fmt,a); va_end(a); printf("\n"); }
EOF

# Create minimal cell stub
cat > /tmp/map_test/include/game/cell.h << 'EOF'
#pragma once
#include "game/coord.h"
class CellClass {
public:
    CellClass() : cell_index_(0), template_type_(0) {}
    void Clear() { template_type_ = 0; }
    void Set_Cell_Index(CELL c) { cell_index_ = c; }
    CELL Get_Cell_Index() const { return cell_index_; }
    void Recalc() {}
private:
    CELL cell_index_;
    uint8_t template_type_;
};
EOF

# Create test program
cat > /tmp/map_test/test_map.cpp << 'EOF'
#include "game/map.h"
#include "game/cell.h"
#include <cstdio>

#define TEST(name, cond) do { \
    if (!(cond)) { printf("FAIL: %s\n", name); return 1; } \
    printf("PASS: %s\n", name); \
} while(0)

int main() {
    printf("=== MapClass Tests ===\n\n");

    MapClass map;
    TEST("Map created", true);

    // Initialize
    map.One_Time();
    TEST("One_Time called", map.Is_Initialized());

    map.Init(THEATER_TEMPERATE);
    TEST("Init called", map.Get_Theater_Type() == THEATER_TEMPERATE);

    // Test cell access
    CELL center = XY_Cell(64, 64);
    CellClass& cell = map[center];
    TEST("Cell access works", cell.Get_Cell_Index() == center);

    // Test by coordinates
    CellClass* ptr = map.Cell_At(64, 64);
    TEST("Cell_At works", ptr != nullptr);

    // Test validation
    TEST("Valid cell is valid", map.Is_Valid_Cell(XY_Cell(64, 64)));
    TEST("Invalid cell is invalid", !map.Is_Valid_Cell(CELL_NONE));

    // Test bounds
    TEST("In bounds", map.Is_In_Bounds(64, 64));
    TEST("Out of bounds - edge", !map.Is_In_Bounds(0, 0));

    // Test bounds setting
    map.Set_Map_Bounds(10, 10, 50, 50);
    TEST("Bounds X", map.Map_Bounds_X() == 10);
    TEST("Bounds Y", map.Map_Bounds_Y() == 10);
    TEST("Bounds W", map.Map_Bounds_Width() == 50);
    TEST("Bounds H", map.Map_Bounds_Height() == 50);

    // Test clamping
    int x = 5, y = 5;
    map.Clamp_To_Bounds(x, y);
    TEST("Clamp X", x == 10);
    TEST("Clamp Y", y == 10);

    // Test coordinate conversion
    COORDINATE coord = Cell_Coord(XY_Cell(32, 32));
    CellClass* coord_cell = map.Cell_Coord(coord);
    TEST("Cell_Coord works", coord_cell != nullptr);

    // Test global pointer
    Map = &map;
    TEST("Global Map set", Map != nullptr);

    printf("\n=== All MapClass tests passed! ===\n");
    return 0;
}
EOF

# Copy source files
cp include/game/coord.h /tmp/map_test/include/game/
cp include/game/gscreen.h /tmp/map_test/include/game/
cp include/game/gadget.h /tmp/map_test/include/game/
cp include/game/map.h /tmp/map_test/include/game/

# Compile
if ! clang++ -std=c++17 -I/tmp/map_test/include -include /tmp/map_test/mock_platform.h \
    /tmp/map_test/test_map.cpp \
    src/game/core/coord.cpp \
    src/game/display/gscreen.cpp \
    src/game/display/gadget.cpp \
    src/game/display/map.cpp \
    -o /tmp/map_test/test_map 2>&1; then
    echo "VERIFY_FAILED: Test compilation failed"
    rm -rf /tmp/map_test
    exit 1
fi
echo "  OK: Test built"

# Step 4: Run test
echo "[4/4] Running map tests..."
if ! /tmp/map_test/test_map; then
    echo "VERIFY_FAILED: Map tests failed"
    rm -rf /tmp/map_test
    exit 1
fi

rm -rf /tmp/map_test

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Create `include/game/map.h` with MapClass definition
2. Create `src/game/display/map.cpp` with implementation
3. Ensure cell.h stub exists (will be expanded in 14e)
4. Run verification script

## Success Criteria
- MapClass inherits from GScreenClass correctly
- Cell array allocation works
- Cell access by CELL, X/Y, and COORDINATE works
- Map bounds tracking functions correctly
- Coordinate validation works
- Clamping works at bounds

## Common Issues

### "CellClass incomplete type"
- Forward declare CellClass in map.h
- Include full cell.h in map.cpp

### Cell index calculation wrong
- Remember: cell = y * MAP_CELL_WIDTH + x
- Use Cell_Index() macro for consistency

### Out of bounds access
- Always check Is_Valid_Cell() before access
- Return dummy_cell_ for invalid access

## Completion Promise
When verification passes, output:
```
<promise>TASK_14D_COMPLETE</promise>
```

## Escape Hatch
If stuck after 12 iterations:
- Document what's blocking in `ralph-tasks/blocked/14d.md`
- Output: `<promise>TASK_14D_BLOCKED</promise>`

## Max Iterations
12
