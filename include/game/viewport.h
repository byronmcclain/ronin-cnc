/**
 * Viewport - Map scrolling and coordinate conversion
 *
 * Manages which portion of the game world is visible on screen.
 * Handles edge scrolling, keyboard scrolling, and coordinate conversion.
 *
 * Original: CODE/SCROLL.CPP, CODE/DISPLAY.CPP
 */

#ifndef GAME_VIEWPORT_H
#define GAME_VIEWPORT_H

#include <cstdint>

// Include coord.h for COORDINATE, CELL, and related macros (avoid redefinition)
#include "game/coord.h"

// =============================================================================
// Screen Layout Constants
// =============================================================================

// Screen layout constants with unique names to avoid redefinition conflicts
// (sidebar_render.h also defines some of these)
static const int VP_SCREEN_WIDTH = 640;
static const int VP_SCREEN_HEIGHT = 400;
static const int VP_SIDEBAR_WIDTH = 160;
static const int VP_TAB_HEIGHT = 16;

// Tactical area dimensions (with VP_ prefix to avoid conflicts with display.h)
static const int VP_TACTICAL_WIDTH = VP_SCREEN_WIDTH - VP_SIDEBAR_WIDTH;   // 480
static const int VP_TACTICAL_HEIGHT = VP_SCREEN_HEIGHT - VP_TAB_HEIGHT;    // 384


// Tile dimensions
static const int TILE_PIXEL_WIDTH = 24;
static const int TILE_PIXEL_HEIGHT = 24;
static const int LEPTONS_PER_CELL = 256;

// Scroll constants
static const int EDGE_SCROLL_ZONE = 16;      // Pixels from edge to trigger scroll
static const int MIN_SCROLL_SPEED = 4;       // Minimum pixels per frame
static const int MAX_SCROLL_SPEED = 32;      // Maximum pixels per frame
static const int KEYBOARD_SCROLL_SPEED = 16; // Pixels per frame for keyboard
static const int SCROLL_ACCEL_FRAMES = 30;   // Frames to reach max speed

// =============================================================================
// Scroll Direction Flags
// =============================================================================

enum ScrollDirection : uint8_t {
    SCROLL_NONE  = 0x00,
    SCROLL_UP    = 0x01,
    SCROLL_DOWN  = 0x02,
    SCROLL_LEFT  = 0x04,
    SCROLL_RIGHT = 0x08
};

// =============================================================================
// Coordinate Types - Defined in game/coord.h
// =============================================================================
// COORDINATE, CELL, and related macros (XY_Cell, Cell_X, Cell_Y, etc.)
// are now included from game/coord.h to avoid redefinition conflicts.

// Compatibility macros for viewport.cpp (old style SCREAMING_CASE -> CamelCase)
// Note: coord.h already defines Cell_X, Cell_Y, Coord_X, Coord_Y as CamelCase
#ifndef COORD_X
#define COORD_X(coord)      Coord_X(coord)
#endif
#ifndef COORD_Y
#define COORD_Y(coord)      Coord_Y(coord)
#endif
#ifndef MAKE_COORD
#define MAKE_COORD(x, y)    XY_Coord(x, y)
#endif
#ifndef MAKE_CELL
#define MAKE_CELL(x, y)     XY_Cell(x, y)
#endif
// SCREAMING_CASE cell macros (used by tests)
#ifndef CELL_X
#define CELL_X(cell)        Cell_X(cell)
#endif
#ifndef CELL_Y
#define CELL_Y(cell)        Cell_Y(cell)
#endif

// Compatibility for TACTICAL_WIDTH/HEIGHT
#define TACTICAL_WIDTH      VP_TACTICAL_WIDTH
#define TACTICAL_HEIGHT     VP_TACTICAL_HEIGHT

// =============================================================================
// GameViewport Class
// =============================================================================

/**
 * GameViewport - Manages map viewport and scrolling
 */
class GameViewport {
public:
    // Current viewport position in world pixels
    int x;      // Left edge in world pixels
    int y;      // Top edge in world pixels
    int width;  // Viewport width in pixels (usually TACTICAL_WIDTH)
    int height; // Viewport height in pixels (usually TACTICAL_HEIGHT)

    // Singleton access
    static GameViewport& Instance();

    // Prevent copying
    GameViewport(const GameViewport&) = delete;
    GameViewport& operator=(const GameViewport&) = delete;

    // =========================================================================
    // Initialization
    // =========================================================================

    /**
     * Initialize viewport to default state
     */
    void Initialize();

    /**
     * Reset to origin
     */
    void Reset();

    // =========================================================================
    // Map Bounds
    // =========================================================================

    /**
     * Set map size in cells
     */
    void SetMapSize(int cells_wide, int cells_high);

    /**
     * Get map dimensions
     */
    int GetMapPixelWidth() const { return map_width_ * TILE_PIXEL_WIDTH; }
    int GetMapPixelHeight() const { return map_height_ * TILE_PIXEL_HEIGHT; }
    int GetMapCellWidth() const { return map_width_; }
    int GetMapCellHeight() const { return map_height_; }

    // =========================================================================
    // Scrolling
    // =========================================================================

    /**
     * Scroll by delta amount
     */
    void Scroll(int delta_x, int delta_y);

    /**
     * Scroll to absolute position
     */
    void ScrollTo(int world_x, int world_y);

    /**
     * Center viewport on a world point
     */
    void CenterOn(int world_x, int world_y);

    /**
     * Center viewport on a cell
     */
    void CenterOnCell(int cell_x, int cell_y);

    /**
     * Center viewport on a lepton coordinate
     */
    void CenterOnCoord(COORDINATE coord);

    /**
     * Instant jump (same as ScrollTo, for clarity)
     */
    void JumpTo(int world_x, int world_y);

    // =========================================================================
    // Edge Scrolling
    // =========================================================================

    /**
     * Update edge scroll based on mouse position
     * Call each frame
     */
    void UpdateEdgeScroll(int mouse_x, int mouse_y);

    /**
     * Update keyboard scroll
     */
    void UpdateKeyboardScroll(bool up, bool down, bool left, bool right);

    // =========================================================================
    // Scroll Control
    // =========================================================================

    /**
     * Enable/disable scrolling
     */
    void EnableScroll(bool enable) { scroll_enabled_ = enable; }
    bool IsScrollEnabled() const { return scroll_enabled_; }

    /**
     * Set scroll speed multiplier (100 = normal)
     */
    void SetScrollSpeed(int speed) { scroll_speed_multiplier_ = speed; }
    int GetScrollSpeed() const { return scroll_speed_multiplier_; }

    /**
     * Get current scroll direction
     */
    uint8_t GetCurrentScrollDirection() const { return current_scroll_direction_; }
    bool IsScrolling() const { return current_scroll_direction_ != SCROLL_NONE; }

    // =========================================================================
    // Target Tracking
    // =========================================================================

    /**
     * Set target to track (for following units)
     */
    void SetTrackTarget(COORDINATE coord);

    /**
     * Clear tracking target
     */
    void ClearTrackTarget();

    /**
     * Check if tracking a target
     */
    bool HasTrackTarget() const { return tracking_enabled_; }

    /**
     * Update tracking (call each frame)
     */
    void UpdateTracking();

    // =========================================================================
    // Coordinate Conversion
    // =========================================================================

    /**
     * Convert world coordinates to screen coordinates
     */
    void WorldToScreen(int world_x, int world_y, int& screen_x, int& screen_y) const;

    /**
     * Convert screen coordinates to world coordinates
     */
    void ScreenToWorld(int screen_x, int screen_y, int& world_x, int& world_y) const;

    /**
     * Convert world coordinates to cell coordinates
     */
    void WorldToCell(int world_x, int world_y, int& cell_x, int& cell_y) const;

    /**
     * Convert cell coordinates to world coordinates
     */
    void CellToWorld(int cell_x, int cell_y, int& world_x, int& world_y) const;

    /**
     * Convert screen coordinates to cell coordinates
     */
    void ScreenToCell(int screen_x, int screen_y, int& cell_x, int& cell_y) const;

    /**
     * Convert cell coordinates to screen coordinates
     */
    void CellToScreen(int cell_x, int cell_y, int& screen_x, int& screen_y) const;

    /**
     * Convert lepton coordinates to pixel coordinates
     */
    void LeptonToPixel(int lepton_x, int lepton_y, int& pixel_x, int& pixel_y) const;

    /**
     * Convert pixel coordinates to lepton coordinates
     */
    void PixelToLepton(int pixel_x, int pixel_y, int& lepton_x, int& lepton_y) const;

    /**
     * Convert COORDINATE to screen position
     */
    void CoordToScreen(COORDINATE coord, int& screen_x, int& screen_y) const;

    // =========================================================================
    // Visibility Testing
    // =========================================================================

    /**
     * Check if a world point is visible
     */
    bool IsPointVisible(int world_x, int world_y) const;

    /**
     * Check if a world rectangle is visible
     */
    bool IsRectVisible(int world_x, int world_y, int rect_width, int rect_height) const;

    /**
     * Check if a cell is visible
     */
    bool IsCellVisible(int cell_x, int cell_y) const;

    /**
     * Check if a COORDINATE is visible
     */
    bool IsCoordVisible(COORDINATE coord) const;

    /**
     * Get visible cell range (for rendering)
     */
    void GetVisibleCellRange(int& start_x, int& start_y, int& end_x, int& end_y) const;

    // =========================================================================
    // Bounds Clamping
    // =========================================================================

    /**
     * Clamp viewport to map bounds
     */
    void ClampToBounds();

private:
    GameViewport();
    ~GameViewport() = default;

    /**
     * Calculate scroll speed based on edge distance
     */
    int CalculateEdgeScrollSpeed(int distance_into_zone) const;

    // Map dimensions in cells
    int map_width_;
    int map_height_;

    // Scroll state
    bool scroll_enabled_;
    int scroll_speed_multiplier_;
    uint8_t current_scroll_direction_;
    int scroll_accel_counter_;

    // Target tracking
    bool tracking_enabled_;
    COORDINATE track_target_;
};

// Global viewport access
extern GameViewport& TheViewport;

#endif // GAME_VIEWPORT_H
