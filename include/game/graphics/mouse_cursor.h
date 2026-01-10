/**
 * MouseCursor - Custom Software Mouse Cursor
 *
 * Renders the game's custom mouse cursor on top of all other graphics.
 * The cursor changes based on game context (move, attack, select, etc.).
 *
 * Usage:
 *   MouseCursor& cursor = MouseCursor::Instance();
 *   cursor.Load();
 *   cursor.SetType(CURSOR_MOVE);
 *   // Each frame:
 *   cursor.Update();
 *   cursor.Draw(screen_buffer);
 *
 * Original: CODE/MOUSE.CPP, WIN32LIB/MOUSE.CPP
 */

#ifndef GAME_GRAPHICS_MOUSE_CURSOR_H
#define GAME_GRAPHICS_MOUSE_CURSOR_H

#include <cstdint>
#include <memory>

// Forward declarations
class GraphicsBuffer;
class ShapeRenderer;

// =============================================================================
// Cursor Types
// =============================================================================

/**
 * CursorType - Different cursor appearances
 *
 * Maps to frame indices in MOUSE.SHP.
 */
enum CursorType : int {
    CURSOR_NORMAL = 0,       // Default arrow pointer

    // Scroll cursors (for edge scrolling)
    CURSOR_SCROLL_N = 1,
    CURSOR_SCROLL_NE = 2,
    CURSOR_SCROLL_E = 3,
    CURSOR_SCROLL_SE = 4,
    CURSOR_SCROLL_S = 5,
    CURSOR_SCROLL_SW = 6,
    CURSOR_SCROLL_W = 7,
    CURSOR_SCROLL_NW = 8,
    CURSOR_NO_SCROLL = 9,

    // Action cursors
    CURSOR_MOVE = 10,        // Move to location
    CURSOR_NO_MOVE = 11,     // Cannot move there
    CURSOR_ENTER = 12,       // Enter building/transport
    CURSOR_DEPLOY = 13,      // Deploy unit

    // Selection cursors (animated)
    CURSOR_SELECT = 14,      // Select/target (base frame)
    CURSOR_SELECT_ANIM1 = 15,
    CURSOR_SELECT_ANIM2 = 16,
    CURSOR_SELECT_ANIM3 = 17,

    // Attack cursors (animated)
    CURSOR_ATTACK = 18,      // Attack target (base frame)
    CURSOR_ATTACK_ANIM1 = 19,
    CURSOR_ATTACK_ANIM2 = 20,
    CURSOR_ATTACK_ANIM3 = 21,

    // Sell/Repair cursors
    CURSOR_SELL = 22,
    CURSOR_SELL_OK = 23,
    CURSOR_REPAIR = 24,
    CURSOR_REPAIR_OK = 25,
    CURSOR_NO_SELL = 26,
    CURSOR_NO_REPAIR = 27,

    // Special action cursors
    CURSOR_GUARD = 28,       // Guard area
    CURSOR_WAYPOINT = 29,    // Set waypoint

    // Nuke cursor (animated)
    CURSOR_NUKE = 30,
    CURSOR_NUKE_ANIM1 = 31,
    CURSOR_NUKE_ANIM2 = 32,
    CURSOR_NUKE_ANIM3 = 33,

    // Ion cannon
    CURSOR_ION = 34,

    // Air strike
    CURSOR_AIRSTRIKE = 35,

    // Chronosphere
    CURSOR_CHRONO = 36,

    // Engineer capture
    CURSOR_CAPTURE = 37,
    CURSOR_NO_CAPTURE = 38,

    // Harvest
    CURSOR_HARVEST = 39,
    CURSOR_NO_HARVEST = 40,

    CURSOR_COUNT
};

// =============================================================================
// Cursor State
// =============================================================================

/**
 * CursorState - Additional cursor state flags
 */
enum CursorState : uint32_t {
    CURSOR_STATE_NONE = 0,
    CURSOR_STATE_HIDDEN = 1 << 0,    // Cursor is hidden
    CURSOR_STATE_LOCKED = 1 << 1,    // Cursor type is locked (won't change)
    CURSOR_STATE_ANIMATING = 1 << 2, // Cursor is animating
};

// =============================================================================
// Hotspot Data
// =============================================================================

/**
 * CursorHotspot - Click point offset from cursor top-left
 */
struct CursorHotspot {
    int x;
    int y;
};

// =============================================================================
// MouseCursor Class
// =============================================================================

/**
 * MouseCursor - Singleton mouse cursor manager
 */
class MouseCursor {
public:
    // =========================================================================
    // Singleton
    // =========================================================================

    static MouseCursor& Instance();

    // Non-copyable
    MouseCursor(const MouseCursor&) = delete;
    MouseCursor& operator=(const MouseCursor&) = delete;

    // =========================================================================
    // Initialization
    // =========================================================================

    /**
     * Load cursor graphics
     *
     * @param filename Shape file to load (default: "MOUSE.SHP")
     * @return true if loaded successfully
     */
    bool Load(const char* filename = "MOUSE.SHP");

    /**
     * Check if cursor is loaded
     */
    bool IsLoaded() const { return shape_ != nullptr; }

    /**
     * Unload cursor graphics
     */
    void Unload();

    // =========================================================================
    // Cursor Type
    // =========================================================================

    /**
     * Set current cursor type
     *
     * The cursor appearance changes immediately (or at next Update).
     *
     * @param type Cursor type to display
     */
    void SetType(CursorType type);

    /**
     * Get current cursor type
     */
    CursorType GetType() const { return current_type_; }

    /**
     * Reset to default cursor
     */
    void Reset() { SetType(CURSOR_NORMAL); }

    // =========================================================================
    // State Control
    // =========================================================================

    /**
     * Show/hide cursor
     */
    void Show();
    void Hide();
    bool IsVisible() const { return !IsHidden(); }
    bool IsHidden() const { return (state_ & CURSOR_STATE_HIDDEN) != 0; }

    /**
     * Lock cursor type (prevents automatic changes)
     */
    void Lock();
    void Unlock();
    bool IsLocked() const { return (state_ & CURSOR_STATE_LOCKED) != 0; }

    // =========================================================================
    // Position
    // =========================================================================

    /**
     * Get cursor position in game coordinates
     *
     * Automatically applies scale correction from window coordinates.
     */
    void GetPosition(int* x, int* y) const;
    int GetX() const;
    int GetY() const;

    /**
     * Get hotspot offset for current cursor
     */
    const CursorHotspot& GetHotspot() const;

    /**
     * Get click position (cursor position adjusted by hotspot)
     */
    void GetClickPosition(int* x, int* y) const;

    // =========================================================================
    // Update & Draw
    // =========================================================================

    /**
     * Update cursor animation
     *
     * Call once per frame before Draw().
     */
    void Update();

    /**
     * Draw cursor to buffer
     *
     * Should be called last in render pipeline (cursor on top).
     *
     * @param buffer Target graphics buffer (must be locked)
     */
    void Draw(GraphicsBuffer& buffer);

    /**
     * Draw cursor at specific position
     *
     * Useful for drawing cursor in different location (e.g., replay).
     *
     * @param buffer Target buffer
     * @param x, y   Position to draw
     */
    void DrawAt(GraphicsBuffer& buffer, int x, int y);

    // =========================================================================
    // Animated Cursors
    // =========================================================================

    /**
     * Check if current cursor type is animated
     */
    bool IsAnimated() const;

    /**
     * Get animation frame count for current cursor
     */
    int GetAnimationFrameCount() const;

    /**
     * Set animation speed (frames between animation steps)
     */
    void SetAnimationSpeed(int frame_delay);

    // =========================================================================
    // Context-Based Cursor Selection
    // =========================================================================

    /**
     * Set cursor for scroll direction
     *
     * @param dx Horizontal direction (-1, 0, 1)
     * @param dy Vertical direction (-1, 0, 1)
     */
    void SetScrollCursor(int dx, int dy);

    /**
     * Check if over scroll edge and set appropriate cursor
     *
     * @param mouse_x, mouse_y Cursor position
     * @param screen_width, screen_height Screen dimensions
     * @param edge_size Pixel size of scroll edge
     * @return true if cursor is in scroll edge
     */
    bool CheckScrollEdge(int mouse_x, int mouse_y,
                         int screen_width, int screen_height,
                         int edge_size = 16);

private:
    MouseCursor();
    ~MouseCursor();

    // =========================================================================
    // Private Data
    // =========================================================================

    std::unique_ptr<ShapeRenderer> shape_;  // Cursor graphics

    CursorType current_type_;     // Current cursor type
    CursorType base_type_;        // Base type (for animation)
    uint32_t state_;              // State flags

    int anim_frame_;              // Current animation frame
    int anim_delay_;              // Frames between animation steps
    int anim_counter_;            // Frame counter for animation

    int scale_;                   // Window scale factor (default 2)

    // =========================================================================
    // Private Methods
    // =========================================================================

    /**
     * Get frame index for cursor type
     */
    int GetFrameForType(CursorType type) const;

    /**
     * Get animation info for cursor type
     *
     * @param type Cursor type
     * @param base_frame Output: base frame index
     * @param frame_count Output: number of animation frames
     */
    void GetAnimationInfo(CursorType type, int* base_frame, int* frame_count) const;

    /**
     * Initialize hotspot data
     */
    void InitHotspots();

    // Hotspot lookup table
    static CursorHotspot s_hotspots[CURSOR_COUNT];
    static bool s_hotspots_initialized;
};

// =============================================================================
// Convenience Functions
// =============================================================================

/**
 * Get current mouse position in game coordinates
 */
inline void Get_Mouse_Position(int* x, int* y) {
    MouseCursor::Instance().GetPosition(x, y);
}

/**
 * Set cursor type globally
 */
inline void Set_Cursor(CursorType type) {
    MouseCursor::Instance().SetType(type);
}

/**
 * Hide the cursor
 */
inline void Hide_Cursor() {
    MouseCursor::Instance().Hide();
}

/**
 * Show the cursor
 */
inline void Show_Cursor() {
    MouseCursor::Instance().Show();
}

#endif // GAME_GRAPHICS_MOUSE_CURSOR_H
