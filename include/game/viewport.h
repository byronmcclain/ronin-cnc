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

// =============================================================================
// Screen Layout Constants
// =============================================================================

// Screen layout constants with unique names to avoid redefinition conflicts
// (sidebar_render.h also defines some of these)
static const int VP_SCREEN_WIDTH = 640;
static const int VP_SCREEN_HEIGHT = 400;
static const int VP_SIDEBAR_WIDTH = 160;
static const int VP_TAB_HEIGHT = 16;

// Tactical area dimensions
static const int TACTICAL_WIDTH = VP_SCREEN_WIDTH - VP_SIDEBAR_WIDTH;   // 480
static const int TACTICAL_HEIGHT = VP_SCREEN_HEIGHT - VP_TAB_HEIGHT;    // 384


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
// Coordinate Types
// =============================================================================

// Coordinate type for world positions (lepton-based)
typedef uint32_t COORDINATE;

// Coordinate manipulation macros
#define COORD_X(coord)      ((coord) & 0xFFFF)
#define COORD_Y(coord)      (((coord) >> 16) & 0xFFFF)
#define MAKE_COORD(x, y)    (((uint32_t)(y) << 16) | ((uint32_t)(x) & 0xFFFF))

// Cell coordinate type
typedef uint16_t CELL;

#define CELL_X(cell)        ((cell) & 0xFF)
#define CELL_Y(cell)        (((cell) >> 8) & 0xFF)
#define MAKE_CELL(x, y)     (((uint16_t)(y) << 8) | ((uint16_t)(x) & 0xFF))

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
