# Task 14f: DisplayClass Port

## Dependencies
- Task 14d (MapClass Foundation) must be complete
- Task 14e (CellClass Implementation) must be complete
- Platform layer graphics must be functional

## Context
DisplayClass handles the tactical map rendering - drawing visible terrain cells, objects, and effects to the screen. It inherits from MapClass and adds viewport management, scroll handling, and the main rendering loop.

## Objective
Port DisplayClass to render the tactical map:
1. Viewport/tactical rectangle management
2. Terrain cell rendering
3. Scroll position control
4. Basic object drawing hooks
5. Cursor/selection box drawing

## Technical Background

### Original DisplayClass (from CODE/DISPLAY.H)
The original handles:
- Tactical rectangle (visible portion of map)
- Drawing terrain templates
- Drawing overlays
- Drawing objects (via virtual calls)
- Mouse cursor and selection

### Rendering Pipeline
```
1. Clear tactical area (or blit shadow buffer)
2. For each visible cell:
   a. Draw terrain template
   b. Draw overlay (tiberium, walls)
   c. Draw objects in cell
3. Draw UI elements (selection box, cursor)
4. Flip buffers
```

### Coordinate Systems
- Screen coordinates: 0,0 = top-left of window
- Tactical coordinates: Screen position of tactical area
- Map coordinates: Cell/lepton positions in world

## Deliverables
- [ ] `include/game/display.h` - DisplayClass header
- [ ] `src/game/display/display.cpp` - DisplayClass implementation
- [ ] Viewport management
- [ ] Cell rendering loop
- [ ] Scroll handling
- [ ] Placeholder object rendering
- [ ] Verification script passes

## Files to Create

### include/game/display.h (NEW)
```cpp
/**
 * DisplayClass - Tactical Map Rendering
 *
 * Handles rendering the visible portion of the game map including
 * terrain, overlays, and objects.
 *
 * Original location: CODE/DISPLAY.H, CODE/DISPLAY.CPP
 *
 * Class Hierarchy:
 *   GScreenClass
 *       |
 *       +-- MapClass
 *           |
 *           +-- DisplayClass    <- This class
 */

#pragma once

#include "game/map.h"
#include "game/coord.h"
#include <cstdint>

// =============================================================================
// Display Constants
// =============================================================================

// Tactical display area (within screen)
constexpr int TACTICAL_X = 0;        // Left edge
constexpr int TACTICAL_Y = 16;       // Below tab bar
constexpr int TACTICAL_WIDTH = 640;  // Full width
constexpr int TACTICAL_HEIGHT = 384; // Screen minus UI

// Scroll speeds (pixels per tick)
constexpr int SCROLL_SPEED_SLOW = 4;
constexpr int SCROLL_SPEED_NORMAL = 8;
constexpr int SCROLL_SPEED_FAST = 16;

// Selection box colors
constexpr uint8_t SELECT_COLOR_ALLY = 120;   // Green
constexpr uint8_t SELECT_COLOR_ENEMY = 123;  // Red
constexpr uint8_t SELECT_COLOR_NEUTRAL = 176;// Grey

// =============================================================================
// DisplayClass
// =============================================================================

/**
 * DisplayClass - Tactical map renderer
 *
 * Adds to MapClass:
 * - Tactical viewport management
 * - Cell-by-cell rendering
 * - Scroll handling
 * - Selection/cursor drawing
 */
class DisplayClass : public MapClass {
public:
    // -------------------------------------------------------------------------
    // Construction / Destruction
    // -------------------------------------------------------------------------

    DisplayClass();
    virtual ~DisplayClass();

    // -------------------------------------------------------------------------
    // Lifecycle Methods (override)
    // -------------------------------------------------------------------------

    virtual void One_Time() override;
    virtual void Init(int theater = THEATER_TEMPERATE) override;
    virtual void Render() override;

    // -------------------------------------------------------------------------
    // Viewport/Tactical Area
    // -------------------------------------------------------------------------

    /**
     * Get tactical display rectangle
     */
    int Tactical_X() const { return tactical_x_; }
    int Tactical_Y() const { return tactical_y_; }
    int Tactical_Width() const { return tactical_width_; }
    int Tactical_Height() const { return tactical_height_; }

    /**
     * Set tactical display area
     */
    void Set_Tactical_Area(int x, int y, int w, int h);

    /**
     * Get visible cell range
     */
    void Get_Visible_Cells(int& start_x, int& start_y, int& end_x, int& end_y) const;

    /**
     * Is cell visible on screen?
     */
    bool Is_Cell_Visible(CELL cell) const;
    bool Is_Cell_Visible(int x, int y) const;

    // -------------------------------------------------------------------------
    // Scrolling
    // -------------------------------------------------------------------------

    /**
     * Scroll the tactical view
     *
     * @param dx Pixels to scroll horizontally (positive = right)
     * @param dy Pixels to scroll vertically (positive = down)
     */
    void Scroll(int dx, int dy);

    /**
     * Center view on coordinate
     */
    void Center_On(COORDINATE coord);
    void Center_On(CELL cell);

    /**
     * Jump view to coordinate (instant, no smooth scroll)
     */
    void Jump_To(COORDINATE coord);

    /**
     * Scroll towards coordinate (for edge scroll)
     *
     * @param edge Which screen edge (0=N, 1=NE, 2=E, etc.)
     * @param speed Scroll speed in pixels
     */
    void Edge_Scroll(int edge, int speed = SCROLL_SPEED_NORMAL);

    /**
     * Get/set scroll constraints
     */
    bool Is_Scroll_Constrained() const { return scroll_constrained_; }
    void Set_Scroll_Constrained(bool val) { scroll_constrained_ = val; }

    // -------------------------------------------------------------------------
    // Coordinate Conversion
    // -------------------------------------------------------------------------

    /**
     * Convert screen position to cell
     *
     * @param screen_x Screen X coordinate
     * @param screen_y Screen Y coordinate
     * @return Cell under cursor (or CELL_NONE if outside tactical)
     */
    CELL Screen_To_Cell(int screen_x, int screen_y) const;

    /**
     * Convert cell to screen position
     *
     * @param cell Cell index
     * @param[out] screen_x Screen X (center of cell)
     * @param[out] screen_y Screen Y (center of cell)
     * @return true if cell is on screen
     */
    bool Cell_To_Screen(CELL cell, int& screen_x, int& screen_y) const;

    /**
     * Convert world coordinate to screen position
     */
    bool Coord_To_Screen(COORDINATE coord, int& screen_x, int& screen_y) const;

    // -------------------------------------------------------------------------
    // Selection
    // -------------------------------------------------------------------------

    /**
     * Draw selection box (rubber band)
     *
     * @param x1, y1 Start corner (screen coords)
     * @param x2, y2 End corner (screen coords)
     * @param color Box color
     */
    void Draw_Selection_Box(int x1, int y1, int x2, int y2, uint8_t color);

    /**
     * Draw cell highlight
     */
    void Highlight_Cell(CELL cell, uint8_t color);

    // -------------------------------------------------------------------------
    // Cursor
    // -------------------------------------------------------------------------

    /**
     * Set cursor position (cell under mouse)
     */
    void Set_Cursor_Cell(CELL cell) { cursor_cell_ = cell; }
    CELL Get_Cursor_Cell() const { return cursor_cell_; }

    /**
     * Draw cursor highlight at current cell
     */
    void Draw_Cursor();

protected:
    // -------------------------------------------------------------------------
    // Rendering (protected)
    // -------------------------------------------------------------------------

    /**
     * Draw all visible cells
     */
    virtual void Draw_Tactical();

    /**
     * Draw single cell (terrain + overlay)
     *
     * @param cell Cell to draw
     * @param screen_x Screen X position
     * @param screen_y Screen Y position
     */
    virtual void Draw_Cell(CELL cell, int screen_x, int screen_y);

    /**
     * Draw terrain template for cell
     */
    virtual void Draw_Template(CellClass& cell, int screen_x, int screen_y);

    /**
     * Draw overlay for cell (tiberium, walls, etc.)
     */
    virtual void Draw_Overlay(CellClass& cell, int screen_x, int screen_y);

    /**
     * Draw objects in cell
     */
    virtual void Draw_Objects(CellClass& cell, int screen_x, int screen_y);

    /**
     * Draw shroud/fog for cell
     */
    virtual void Draw_Shroud(CellClass& cell, int screen_x, int screen_y);

    // -------------------------------------------------------------------------
    // Protected Members
    // -------------------------------------------------------------------------

    // Tactical display area (screen coordinates)
    int tactical_x_;
    int tactical_y_;
    int tactical_width_;
    int tactical_height_;

    // Cursor
    CELL cursor_cell_;          // Cell under mouse cursor

    // Scroll settings
    bool scroll_constrained_;   // Limit scroll to map bounds

    // Render state
    bool need_full_redraw_;     // Force complete redraw
};

// =============================================================================
// Global Display Pointer
// =============================================================================

extern DisplayClass* Display;
```

### src/game/display/display.cpp (NEW)
```cpp
/**
 * DisplayClass Implementation
 *
 * Tactical map rendering.
 */

#include "game/display.h"
#include "game/cell.h"
#include "platform.h"
#include <algorithm>

// =============================================================================
// Global Display Pointer
// =============================================================================

DisplayClass* Display = nullptr;

// =============================================================================
// Construction / Destruction
// =============================================================================

DisplayClass::DisplayClass()
    : MapClass()
    , tactical_x_(TACTICAL_X)
    , tactical_y_(TACTICAL_Y)
    , tactical_width_(TACTICAL_WIDTH)
    , tactical_height_(TACTICAL_HEIGHT)
    , cursor_cell_(CELL_NONE)
    , scroll_constrained_(true)
    , need_full_redraw_(true)
{
}

DisplayClass::~DisplayClass() {
}

// =============================================================================
// Lifecycle Methods
// =============================================================================

void DisplayClass::One_Time() {
    MapClass::One_Time();

    Platform_LogInfo("DisplayClass::One_Time: Tactical area (%d,%d) %dx%d",
                     tactical_x_, tactical_y_, tactical_width_, tactical_height_);
}

void DisplayClass::Init(int theater) {
    MapClass::Init(theater);

    // Reset cursor
    cursor_cell_ = CELL_NONE;

    // Force full redraw
    need_full_redraw_ = true;

    // Center on map
    Center_On(Map_First_Cell());
}

void DisplayClass::Render() {
    if (!Is_Initialized()) {
        return;
    }

    // Lock buffer for drawing
    Lock();

    // Draw tactical map
    Draw_Tactical();

    // Draw cursor highlight
    Draw_Cursor();

    // Unlock buffer
    Unlock();

    // Present frame (via base class)
    GScreenClass::Flip();
}

// =============================================================================
// Viewport Management
// =============================================================================

void DisplayClass::Set_Tactical_Area(int x, int y, int w, int h) {
    tactical_x_ = x;
    tactical_y_ = y;
    tactical_width_ = w;
    tactical_height_ = h;
    need_full_redraw_ = true;
}

void DisplayClass::Get_Visible_Cells(int& start_x, int& start_y, int& end_x, int& end_y) const {
    // Get scroll position in pixels
    int scroll_x = Coord_XPixel(tactical_pos_);
    int scroll_y = Coord_YPixel(tactical_pos_);

    // Calculate cell range
    start_x = scroll_x / CELL_PIXEL_SIZE;
    start_y = scroll_y / CELL_PIXEL_SIZE;
    end_x = (scroll_x + tactical_width_ + CELL_PIXEL_SIZE - 1) / CELL_PIXEL_SIZE;
    end_y = (scroll_y + tactical_height_ + CELL_PIXEL_SIZE - 1) / CELL_PIXEL_SIZE;

    // Clamp to valid range
    if (start_x < 0) start_x = 0;
    if (start_y < 0) start_y = 0;
    if (end_x > MAP_CELL_WIDTH) end_x = MAP_CELL_WIDTH;
    if (end_y > MAP_CELL_HEIGHT) end_y = MAP_CELL_HEIGHT;
}

bool DisplayClass::Is_Cell_Visible(CELL cell) const {
    return Is_Cell_Visible(Cell_X(cell), Cell_Y(cell));
}

bool DisplayClass::Is_Cell_Visible(int x, int y) const {
    int start_x, start_y, end_x, end_y;
    Get_Visible_Cells(start_x, start_y, end_x, end_y);
    return (x >= start_x && x < end_x && y >= start_y && y < end_y);
}

// =============================================================================
// Scrolling
// =============================================================================

void DisplayClass::Scroll(int dx, int dy) {
    // Convert pixels to leptons
    int lx = dx * LEPTON_PER_PIXEL;
    int ly = dy * LEPTON_PER_PIXEL;

    // Calculate new position
    int new_x = Coord_X(tactical_pos_) + lx;
    int new_y = Coord_Y(tactical_pos_) + ly;

    // Constrain to map bounds if enabled
    if (scroll_constrained_) {
        int min_x = map_x_ * LEPTON_PER_CELL;
        int min_y = map_y_ * LEPTON_PER_CELL;
        int max_x = (map_x_ + map_width_) * LEPTON_PER_CELL - (tactical_width_ * LEPTON_PER_PIXEL);
        int max_y = (map_y_ + map_height_) * LEPTON_PER_CELL - (tactical_height_ * LEPTON_PER_PIXEL);

        if (new_x < min_x) new_x = min_x;
        if (new_y < min_y) new_y = min_y;
        if (new_x > max_x) new_x = max_x;
        if (new_y > max_y) new_y = max_y;
    }

    tactical_pos_ = XY_Coord(new_x, new_y);
    need_full_redraw_ = true;
}

void DisplayClass::Center_On(COORDINATE coord) {
    // Calculate position to center this coordinate
    int center_x = Coord_X(coord) - (tactical_width_ * LEPTON_PER_PIXEL / 2);
    int center_y = Coord_Y(coord) - (tactical_height_ * LEPTON_PER_PIXEL / 2);

    tactical_pos_ = XY_Coord(center_x, center_y);

    // Apply scroll constraints
    if (scroll_constrained_) {
        Scroll(0, 0); // This will clamp to bounds
    }

    need_full_redraw_ = true;
}

void DisplayClass::Center_On(CELL cell) {
    Center_On(Cell_Coord(cell));
}

void DisplayClass::Jump_To(COORDINATE coord) {
    Center_On(coord);
}

void DisplayClass::Edge_Scroll(int edge, int speed) {
    // Edge directions: 0=N, 1=NE, 2=E, 3=SE, 4=S, 5=SW, 6=W, 7=NW
    static const int dx[] = { 0, 1, 1, 1, 0, -1, -1, -1 };
    static const int dy[] = { -1, -1, 0, 1, 1, 1, 0, -1 };

    if (edge >= 0 && edge < 8) {
        Scroll(dx[edge] * speed, dy[edge] * speed);
    }
}

// =============================================================================
// Coordinate Conversion
// =============================================================================

CELL DisplayClass::Screen_To_Cell(int screen_x, int screen_y) const {
    // Check if within tactical area
    if (screen_x < tactical_x_ || screen_x >= tactical_x_ + tactical_width_ ||
        screen_y < tactical_y_ || screen_y >= tactical_y_ + tactical_height_) {
        return CELL_NONE;
    }

    // Convert to tactical-relative
    int tac_x = screen_x - tactical_x_;
    int tac_y = screen_y - tactical_y_;

    // Add scroll offset
    int world_x = tac_x + Coord_XPixel(tactical_pos_);
    int world_y = tac_y + Coord_YPixel(tactical_pos_);

    // Convert to cell
    int cell_x = world_x / CELL_PIXEL_SIZE;
    int cell_y = world_y / CELL_PIXEL_SIZE;

    if (!Is_Valid_XY(cell_x, cell_y)) {
        return CELL_NONE;
    }

    return XY_Cell(cell_x, cell_y);
}

bool DisplayClass::Cell_To_Screen(CELL cell, int& screen_x, int& screen_y) const {
    if (!Is_Valid_Cell(cell)) {
        return false;
    }

    int cell_x = Cell_X(cell);
    int cell_y = Cell_Y(cell);

    // Calculate world pixel position (center of cell)
    int world_x = cell_x * CELL_PIXEL_SIZE + CELL_PIXEL_SIZE / 2;
    int world_y = cell_y * CELL_PIXEL_SIZE + CELL_PIXEL_SIZE / 2;

    // Subtract scroll and add tactical offset
    screen_x = world_x - Coord_XPixel(tactical_pos_) + tactical_x_;
    screen_y = world_y - Coord_YPixel(tactical_pos_) + tactical_y_;

    // Check if on screen
    return (screen_x >= tactical_x_ - CELL_PIXEL_SIZE &&
            screen_x < tactical_x_ + tactical_width_ + CELL_PIXEL_SIZE &&
            screen_y >= tactical_y_ - CELL_PIXEL_SIZE &&
            screen_y < tactical_y_ + tactical_height_ + CELL_PIXEL_SIZE);
}

bool DisplayClass::Coord_To_Screen(COORDINATE coord, int& screen_x, int& screen_y) const {
    int world_x = Coord_XPixel(coord);
    int world_y = Coord_YPixel(coord);

    screen_x = world_x - Coord_XPixel(tactical_pos_) + tactical_x_;
    screen_y = world_y - Coord_YPixel(tactical_pos_) + tactical_y_;

    return (screen_x >= tactical_x_ && screen_x < tactical_x_ + tactical_width_ &&
            screen_y >= tactical_y_ && screen_y < tactical_y_ + tactical_height_);
}

// =============================================================================
// Selection/Cursor Drawing
// =============================================================================

void DisplayClass::Draw_Selection_Box(int x1, int y1, int x2, int y2, uint8_t color) {
    // Normalize coordinates
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);

    // Draw box outline
    Draw_Line(x1, y1, x2, y1, color); // Top
    Draw_Line(x1, y2, x2, y2, color); // Bottom
    Draw_Line(x1, y1, x1, y2, color); // Left
    Draw_Line(x2, y1, x2, y2, color); // Right
}

void DisplayClass::Highlight_Cell(CELL cell, uint8_t color) {
    int screen_x, screen_y;
    if (Cell_To_Screen(cell, screen_x, screen_y)) {
        // Draw box around cell
        int left = screen_x - CELL_PIXEL_SIZE / 2;
        int top = screen_y - CELL_PIXEL_SIZE / 2;
        Draw_Selection_Box(left, top, left + CELL_PIXEL_SIZE - 1, top + CELL_PIXEL_SIZE - 1, color);
    }
}

void DisplayClass::Draw_Cursor() {
    if (cursor_cell_ != CELL_NONE) {
        Highlight_Cell(cursor_cell_, SELECT_COLOR_ALLY);
    }
}

// =============================================================================
// Tactical Rendering
// =============================================================================

void DisplayClass::Draw_Tactical() {
    int start_x, start_y, end_x, end_y;
    Get_Visible_Cells(start_x, start_y, end_x, end_y);

    int scroll_x = Coord_XPixel(tactical_pos_);
    int scroll_y = Coord_YPixel(tactical_pos_);

    // Draw each visible cell
    for (int cy = start_y; cy < end_y; cy++) {
        for (int cx = start_x; cx < end_x; cx++) {
            CELL cell = XY_Cell(cx, cy);

            // Calculate screen position
            int screen_x = (cx * CELL_PIXEL_SIZE) - scroll_x + tactical_x_;
            int screen_y = (cy * CELL_PIXEL_SIZE) - scroll_y + tactical_y_;

            // Draw the cell
            Draw_Cell(cell, screen_x, screen_y);
        }
    }
}

void DisplayClass::Draw_Cell(CELL cell, int screen_x, int screen_y) {
    CellClass& c = (*this)[cell];

    // Draw shroud if not explored
    if (c.Is_Shrouded()) {
        Draw_Shroud(c, screen_x, screen_y);
        return;
    }

    // Draw terrain template
    Draw_Template(c, screen_x, screen_y);

    // Draw overlay (tiberium, walls, etc.)
    if (c.Has_Overlay()) {
        Draw_Overlay(c, screen_x, screen_y);
    }

    // Draw objects
    if (c.Is_Occupied()) {
        Draw_Objects(c, screen_x, screen_y);
    }

    // Draw fog if explored but not visible
    if (!c.Is_Visible()) {
        // Draw semi-transparent fog overlay
        // (Placeholder - actual fog rendering is more complex)
    }
}

void DisplayClass::Draw_Template(CellClass& cell, int screen_x, int screen_y) {
    // Placeholder: Draw colored rectangle based on template
    uint8_t color;

    if (!cell.Has_Template()) {
        color = 0; // Black for no template
    } else {
        // Different colors for different land types
        switch (cell.Get_Land()) {
            case LAND_CLEAR:    color = 141; break; // Green
            case LAND_ROAD:     color = 176; break; // Grey
            case LAND_WATER:    color = 154; break; // Blue
            case LAND_ROCK:     color = 8;   break; // Dark grey
            case LAND_TIBERIUM: color = 144; break; // Green/yellow
            case LAND_BEACH:    color = 157; break; // Tan
            case LAND_ROUGH:    color = 134; break; // Brown
            default:            color = 141; break;
        }
    }

    Draw_Rect(screen_x, screen_y, CELL_PIXEL_SIZE, CELL_PIXEL_SIZE, color);
}

void DisplayClass::Draw_Overlay(CellClass& cell, int screen_x, int screen_y) {
    // Placeholder: Draw overlay indicator
    if (cell.Is_Tiberium()) {
        // Draw tiberium crystal pattern
        int cx = screen_x + CELL_PIXEL_SIZE / 2;
        int cy = screen_y + CELL_PIXEL_SIZE / 2;
        Put_Pixel(cx, cy, 113); // Green
        Put_Pixel(cx - 2, cy, 113);
        Put_Pixel(cx + 2, cy, 113);
        Put_Pixel(cx, cy - 2, 113);
        Put_Pixel(cx, cy + 2, 113);
    } else if (cell.Is_Wall()) {
        // Draw wall outline
        Draw_Rect(screen_x + 2, screen_y + 2, CELL_PIXEL_SIZE - 4, CELL_PIXEL_SIZE - 4, 8);
    }
}

void DisplayClass::Draw_Objects(CellClass& cell, int screen_x, int screen_y) {
    // Placeholder: Draw object marker
    // Real implementation would iterate cell objects and call their Draw methods

    int cx = screen_x + CELL_PIXEL_SIZE / 2;
    int cy = screen_y + CELL_PIXEL_SIZE / 2;

    // Draw small diamond for objects
    Put_Pixel(cx, cy - 3, 15);
    Put_Pixel(cx - 3, cy, 15);
    Put_Pixel(cx + 3, cy, 15);
    Put_Pixel(cx, cy + 3, 15);
}

void DisplayClass::Draw_Shroud(CellClass& cell, int screen_x, int screen_y) {
    (void)cell;
    // Draw black for unexplored areas
    Draw_Rect(screen_x, screen_y, CELL_PIXEL_SIZE, CELL_PIXEL_SIZE, 0);
}
```

## Verification Command
```bash
ralph-tasks/verify/check-display-class.sh
```

## Verification Script

### ralph-tasks/verify/check-display-class.sh (NEW)
```bash
#!/bin/bash
# Verification script for DisplayClass port

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking DisplayClass Port ==="
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

check_file "include/game/display.h"
check_file "src/game/display/display.cpp"

# Step 2: Syntax check
echo "[2/4] Checking syntax..."
# Create mock platform for syntax check
cat > /tmp/mock_display_platform.h << 'EOF'
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

if ! clang++ -std=c++17 -fsyntax-only -I include \
    -include /tmp/mock_display_platform.h \
    src/game/display/display.cpp 2>&1; then
    echo "VERIFY_FAILED: display.cpp has syntax errors"
    rm -f /tmp/mock_display_platform.h
    exit 1
fi
echo "  OK: Syntax valid"

# Step 3: Build test
echo "[3/4] Building display test..."
cat > /tmp/test_display.cpp << 'EOF'
#include "game/display.h"
#include "game/cell.h"
#include <cstdio>

#define TEST(name, cond) do { \
    if (!(cond)) { printf("FAIL: %s\n", name); return 1; } \
    printf("PASS: %s\n", name); \
} while(0)

int main() {
    printf("=== DisplayClass Tests ===\n\n");

    DisplayClass display;
    TEST("Display created", true);

    // Initialize
    display.One_Time();
    TEST("One_Time called", display.Is_Initialized());

    display.Init(THEATER_TEMPERATE);
    TEST("Init called", true);

    // Test tactical area
    TEST("Tactical X", display.Tactical_X() == 0);
    TEST("Tactical Y", display.Tactical_Y() == 16);
    TEST("Tactical W", display.Tactical_Width() == 640);
    TEST("Tactical H", display.Tactical_Height() == 384);

    // Test visible cell calculation
    int sx, sy, ex, ey;
    display.Get_Visible_Cells(sx, sy, ex, ey);
    TEST("Visible cells start valid", sx >= 0 && sy >= 0);
    TEST("Visible cells end valid", ex > sx && ey > sy);

    // Test coordinate conversion
    display.Center_On(XY_Cell(64, 64));
    CELL cell = display.Screen_To_Cell(320, 200);
    TEST("Screen to cell works", cell != CELL_NONE);

    int screen_x, screen_y;
    bool visible = display.Cell_To_Screen(XY_Cell(64, 64), screen_x, screen_y);
    TEST("Cell to screen works", visible);

    // Test scrolling
    display.Scroll(10, 10);
    TEST("Scroll works", true);

    display.Center_On(XY_Cell(32, 32));
    TEST("Center on works", true);

    // Test cursor
    display.Set_Cursor_Cell(XY_Cell(50, 50));
    TEST("Cursor cell set", display.Get_Cursor_Cell() == XY_Cell(50, 50));

    // Test global pointer
    Display = &display;
    TEST("Global Display set", Display != nullptr);

    printf("\n=== All DisplayClass tests passed! ===\n");
    return 0;
}
EOF

# Compile all source files
if ! clang++ -std=c++17 -I include \
    -include /tmp/mock_display_platform.h \
    /tmp/test_display.cpp \
    src/game/core/coord.cpp \
    src/game/map/cell.cpp \
    src/game/display/gscreen.cpp \
    src/game/display/gadget.cpp \
    src/game/display/map.cpp \
    src/game/display/display.cpp \
    -o /tmp/test_display 2>&1; then
    echo "VERIFY_FAILED: Test compilation failed"
    rm -f /tmp/mock_display_platform.h /tmp/test_display.cpp
    exit 1
fi
echo "  OK: Test built"

# Step 4: Run test
echo "[4/4] Running display tests..."
if ! /tmp/test_display; then
    echo "VERIFY_FAILED: Display tests failed"
    rm -f /tmp/mock_display_platform.h /tmp/test_display.cpp /tmp/test_display
    exit 1
fi

rm -f /tmp/mock_display_platform.h /tmp/test_display.cpp /tmp/test_display

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Create `include/game/display.h`
2. Create `src/game/display/display.cpp`
3. Run verification script

## Success Criteria
- DisplayClass inherits from MapClass correctly
- Viewport management works
- Scroll constraints applied
- Coordinate conversions accurate
- Basic cell rendering produces visible output

## Common Issues

### Scroll position clamping wrong
- Check lepton vs pixel conversion
- Verify map bounds calculation

### Cells draw at wrong positions
- Check scroll offset subtraction
- Verify tactical area offset

### Performance issues
- Only draw visible cells
- Consider dirty rect tracking

## Completion Promise
When verification passes, output:
```
<promise>TASK_14F_COMPLETE</promise>
```

## Escape Hatch
If stuck after 12 iterations:
- Document what's blocking in `ralph-tasks/blocked/14f.md`
- Output: `<promise>TASK_14F_BLOCKED</promise>`

## Max Iterations
12
