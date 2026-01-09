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
