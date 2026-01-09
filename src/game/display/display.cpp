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

    Platform_LogInfo("DisplayClass::One_Time complete");
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
    (void)cell;

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
