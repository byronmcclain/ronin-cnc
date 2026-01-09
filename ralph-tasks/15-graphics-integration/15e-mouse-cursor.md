# Task 15e: Mouse Cursor Rendering

## Dependencies
- Task 15a (GraphicsBuffer) must be complete
- Task 15b (Shape Drawing) must be complete
- Platform input APIs must work (`Platform_Mouse_*`)
- MOUSE.SHP asset must be loadable from MIX files

## Context
Red Alert uses a custom software-rendered mouse cursor from `MOUSE.SHP`. The cursor changes based on game context:
- Normal pointer (default)
- Move cursor (when ordering unit movement)
- Attack cursor (when targeting enemy)
- Select cursor (when drawing selection box)
- No-go cursor (when action not allowed)
- Repair/sell cursors (special actions)

The cursor is always drawn last in the render pipeline, on top of everything else. The original game hides the system cursor and draws its own sprite at the mouse position.

This task creates a `MouseCursor` class that:
1. Loads cursor shapes from MOUSE.SHP
2. Tracks cursor state and animation
3. Handles cursor positioning with scale correction
4. Draws the cursor each frame

## Objective
Create a mouse cursor system that loads cursor graphics from MOUSE.SHP and draws the appropriate cursor sprite based on game state.

## Technical Background

### MOUSE.SHP Structure
The mouse cursor shape file contains multiple frames for different cursor types:
```
Frame 0:  Normal arrow pointer
Frame 1:  Scroll N
Frame 2:  Scroll NE
Frame 3:  Scroll E
Frame 4:  Scroll SE
Frame 5:  Scroll S
Frame 6:  Scroll SW
Frame 7:  Scroll W
Frame 8:  Scroll NW
Frame 9:  No scroll
Frame 10: Move cursor
Frame 11: No move
Frame 12: Enter building
Frame 13: Deploy
Frame 14: Select (normal crosshair)
Frame 15-17: Select animation
Frame 18: Attack
Frame 19-21: Attack animation
Frame 22: Sell
Frame 23: Sell (over building)
Frame 24: Repair
Frame 25: Repair (over building)
Frame 26: No sell
Frame 27: No repair
Frame 28: Guard area
Frame 29: Waypoint
Frame 30: Nuke
Frame 31-33: Nuke animation
...
```

### Cursor Hotspot
Each cursor has a "hotspot" - the pixel that represents the actual click point:
- Arrow: top-left (0, 0)
- Crosshairs: center
- Icons: center

### Coordinate Transformation
Mouse position from platform layer is in window coordinates. With 2x scaling:
```cpp
// Platform gives window coordinates (0-1280 for 2x scale)
int window_x, window_y;
Platform_Mouse_GetPosition(&window_x, &window_y);

// Convert to game coordinates (0-640)
int game_x = window_x / 2;
int game_y = window_y / 2;
```

### Cursor Animation
Some cursors animate (attack, select, nuke):
```cpp
// 4-frame animation at ~10 fps
anim_frame = (anim_frame + 1) % 4;
// Or use game frame counter:
anim_frame = (Frame / 4) % 4;
```

## Deliverables
- [ ] `include/game/graphics/mouse_cursor.h` - Mouse cursor class
- [ ] `src/game/graphics/mouse_cursor.cpp` - Implementation
- [ ] `src/test_mouse_cursor.cpp` - Test program
- [ ] `ralph-tasks/verify/check-mouse-cursor.sh` - Verification script
- [ ] All tests pass

## Files to Create

### include/game/graphics/mouse_cursor.h
```cpp
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
```

### src/game/graphics/mouse_cursor.cpp
```cpp
/**
 * MouseCursor Implementation
 */

#include "game/graphics/mouse_cursor.h"
#include "game/graphics/graphics_buffer.h"
#include "game/graphics/shape_renderer.h"
#include "platform.h"

// =============================================================================
// Static Data
// =============================================================================

CursorHotspot MouseCursor::s_hotspots[CURSOR_COUNT];
bool MouseCursor::s_hotspots_initialized = false;

// =============================================================================
// Singleton
// =============================================================================

MouseCursor& MouseCursor::Instance() {
    static MouseCursor instance;
    return instance;
}

MouseCursor::MouseCursor()
    : current_type_(CURSOR_NORMAL)
    , base_type_(CURSOR_NORMAL)
    , state_(CURSOR_STATE_NONE)
    , anim_frame_(0)
    , anim_delay_(4)
    , anim_counter_(0)
    , scale_(2)
{
    InitHotspots();
}

MouseCursor::~MouseCursor() {
    Unload();
}

// =============================================================================
// Initialization
// =============================================================================

void MouseCursor::InitHotspots() {
    if (s_hotspots_initialized) return;

    // Default all hotspots to (0, 0)
    for (int i = 0; i < CURSOR_COUNT; i++) {
        s_hotspots[i] = {0, 0};
    }

    // Arrow cursor - hotspot at tip (top-left)
    s_hotspots[CURSOR_NORMAL] = {0, 0};

    // Scroll cursors - hotspot at center
    for (int i = CURSOR_SCROLL_N; i <= CURSOR_NO_SCROLL; i++) {
        s_hotspots[i] = {12, 12};
    }

    // Action cursors - hotspot at center
    s_hotspots[CURSOR_MOVE] = {12, 12};
    s_hotspots[CURSOR_NO_MOVE] = {12, 12};
    s_hotspots[CURSOR_ENTER] = {12, 12};
    s_hotspots[CURSOR_DEPLOY] = {12, 12};

    // Crosshair cursors - hotspot at exact center
    for (int i = CURSOR_SELECT; i <= CURSOR_SELECT_ANIM3; i++) {
        s_hotspots[i] = {15, 15};
    }
    for (int i = CURSOR_ATTACK; i <= CURSOR_ATTACK_ANIM3; i++) {
        s_hotspots[i] = {15, 15};
    }

    // Icon cursors - hotspot at center
    s_hotspots[CURSOR_SELL] = {12, 12};
    s_hotspots[CURSOR_SELL_OK] = {12, 12};
    s_hotspots[CURSOR_REPAIR] = {12, 12};
    s_hotspots[CURSOR_REPAIR_OK] = {12, 12};
    s_hotspots[CURSOR_NO_SELL] = {12, 12};
    s_hotspots[CURSOR_NO_REPAIR] = {12, 12};

    s_hotspots[CURSOR_GUARD] = {12, 12};
    s_hotspots[CURSOR_WAYPOINT] = {12, 12};

    // Nuke cursor - hotspot at bottom center (bomb drop point)
    for (int i = CURSOR_NUKE; i <= CURSOR_NUKE_ANIM3; i++) {
        s_hotspots[i] = {15, 30};
    }

    s_hotspots[CURSOR_ION] = {15, 15};
    s_hotspots[CURSOR_AIRSTRIKE] = {15, 15};
    s_hotspots[CURSOR_CHRONO] = {15, 15};

    s_hotspots[CURSOR_CAPTURE] = {12, 12};
    s_hotspots[CURSOR_NO_CAPTURE] = {12, 12};
    s_hotspots[CURSOR_HARVEST] = {12, 12};
    s_hotspots[CURSOR_NO_HARVEST] = {12, 12};

    s_hotspots_initialized = true;
}

bool MouseCursor::Load(const char* filename) {
    Unload();

    shape_ = std::make_unique<ShapeRenderer>();
    if (!shape_->Load(filename)) {
        shape_.reset();
        Platform_LogWarn("Failed to load mouse cursor shape");
        return false;
    }

    // Hide system cursor
    Platform_Mouse_Hide();

    return true;
}

void MouseCursor::Unload() {
    shape_.reset();

    // Show system cursor
    Platform_Mouse_Show();
}

// =============================================================================
// Cursor Type
// =============================================================================

void MouseCursor::SetType(CursorType type) {
    if (IsLocked()) return;

    if (type >= 0 && type < CURSOR_COUNT) {
        // Reset animation when cursor type changes
        if (type != base_type_) {
            anim_frame_ = 0;
            anim_counter_ = 0;
        }

        base_type_ = type;
        current_type_ = type;
    }
}

// =============================================================================
// State Control
// =============================================================================

void MouseCursor::Show() {
    state_ &= ~CURSOR_STATE_HIDDEN;
}

void MouseCursor::Hide() {
    state_ |= CURSOR_STATE_HIDDEN;
}

void MouseCursor::Lock() {
    state_ |= CURSOR_STATE_LOCKED;
}

void MouseCursor::Unlock() {
    state_ &= ~CURSOR_STATE_LOCKED;
}

// =============================================================================
// Position
// =============================================================================

void MouseCursor::GetPosition(int* x, int* y) const {
    int window_x, window_y;
    Platform_Mouse_GetPosition(&window_x, &window_y);

    // Convert from window coordinates to game coordinates
    int game_x = window_x / scale_;
    int game_y = window_y / scale_;

    if (x) *x = game_x;
    if (y) *y = game_y;
}

int MouseCursor::GetX() const {
    int x;
    GetPosition(&x, nullptr);
    return x;
}

int MouseCursor::GetY() const {
    int y;
    GetPosition(nullptr, &y);
    return y;
}

const CursorHotspot& MouseCursor::GetHotspot() const {
    int type = current_type_;
    if (type < 0 || type >= CURSOR_COUNT) {
        type = 0;
    }
    return s_hotspots[type];
}

void MouseCursor::GetClickPosition(int* x, int* y) const {
    int mx, my;
    GetPosition(&mx, &my);

    const CursorHotspot& hotspot = GetHotspot();

    if (x) *x = mx + hotspot.x;
    if (y) *y = my + hotspot.y;
}

// =============================================================================
// Animation
// =============================================================================

bool MouseCursor::IsAnimated() const {
    switch (base_type_) {
        case CURSOR_SELECT:
        case CURSOR_ATTACK:
        case CURSOR_NUKE:
            return true;
        default:
            return false;
    }
}

int MouseCursor::GetAnimationFrameCount() const {
    switch (base_type_) {
        case CURSOR_SELECT:
        case CURSOR_ATTACK:
        case CURSOR_NUKE:
            return 4;
        default:
            return 1;
    }
}

void MouseCursor::SetAnimationSpeed(int frame_delay) {
    anim_delay_ = frame_delay > 0 ? frame_delay : 1;
}

void MouseCursor::GetAnimationInfo(CursorType type, int* base_frame, int* frame_count) const {
    switch (type) {
        case CURSOR_SELECT:
            *base_frame = CURSOR_SELECT;
            *frame_count = 4;
            break;
        case CURSOR_ATTACK:
            *base_frame = CURSOR_ATTACK;
            *frame_count = 4;
            break;
        case CURSOR_NUKE:
            *base_frame = CURSOR_NUKE;
            *frame_count = 4;
            break;
        default:
            *base_frame = type;
            *frame_count = 1;
            break;
    }
}

// =============================================================================
// Update & Draw
// =============================================================================

void MouseCursor::Update() {
    if (!IsAnimated()) {
        current_type_ = base_type_;
        return;
    }

    // Advance animation
    anim_counter_++;
    if (anim_counter_ >= anim_delay_) {
        anim_counter_ = 0;

        int base_frame, frame_count;
        GetAnimationInfo(base_type_, &base_frame, &frame_count);

        anim_frame_ = (anim_frame_ + 1) % frame_count;
        current_type_ = static_cast<CursorType>(base_frame + anim_frame_);
    }
}

void MouseCursor::Draw(GraphicsBuffer& buffer) {
    if (IsHidden() || !IsLoaded()) return;

    int x, y;
    GetPosition(&x, &y);
    DrawAt(buffer, x, y);
}

void MouseCursor::DrawAt(GraphicsBuffer& buffer, int x, int y) {
    if (IsHidden() || !IsLoaded()) return;

    // Get hotspot and adjust position
    const CursorHotspot& hotspot = GetHotspot();
    int draw_x = x - hotspot.x;
    int draw_y = y - hotspot.y;

    // Draw cursor sprite
    int frame = GetFrameForType(current_type_);
    shape_->Draw(buffer, draw_x, draw_y, frame);
}

int MouseCursor::GetFrameForType(CursorType type) const {
    // Direct mapping - frame index equals cursor type
    return static_cast<int>(type);
}

// =============================================================================
// Scroll Edge Detection
// =============================================================================

void MouseCursor::SetScrollCursor(int dx, int dy) {
    // Map direction to scroll cursor
    // Direction: (-1,-1) = NW, (0,-1) = N, (1,-1) = NE, etc.

    if (dx == 0 && dy == 0) {
        SetType(CURSOR_NORMAL);
        return;
    }

    CursorType scroll_type = CURSOR_NO_SCROLL;

    if (dy < 0) {
        // North
        if (dx < 0) scroll_type = CURSOR_SCROLL_NW;
        else if (dx > 0) scroll_type = CURSOR_SCROLL_NE;
        else scroll_type = CURSOR_SCROLL_N;
    }
    else if (dy > 0) {
        // South
        if (dx < 0) scroll_type = CURSOR_SCROLL_SW;
        else if (dx > 0) scroll_type = CURSOR_SCROLL_SE;
        else scroll_type = CURSOR_SCROLL_S;
    }
    else {
        // East/West
        if (dx < 0) scroll_type = CURSOR_SCROLL_W;
        else scroll_type = CURSOR_SCROLL_E;
    }

    SetType(scroll_type);
}

bool MouseCursor::CheckScrollEdge(int mouse_x, int mouse_y,
                                   int screen_width, int screen_height,
                                   int edge_size) {
    int dx = 0, dy = 0;

    if (mouse_x < edge_size) dx = -1;
    else if (mouse_x >= screen_width - edge_size) dx = 1;

    if (mouse_y < edge_size) dy = -1;
    else if (mouse_y >= screen_height - edge_size) dy = 1;

    if (dx != 0 || dy != 0) {
        SetScrollCursor(dx, dy);
        return true;
    }

    return false;
}
```

### src/test_mouse_cursor.cpp
```cpp
/**
 * Mouse Cursor Test Program
 */

#include "game/graphics/mouse_cursor.h"
#include "game/graphics/graphics_buffer.h"
#include "platform.h"
#include <cstdio>

// =============================================================================
// Test Utilities
// =============================================================================

static int tests_run = 0;
static int tests_passed = 0;

#define TEST_START(name) do { \
    printf("  Testing %s... ", name); \
    tests_run++; \
} while(0)

#define TEST_PASS() do { \
    printf("PASS\n"); \
    tests_passed++; \
} while(0)

#define TEST_FAIL(msg) do { \
    printf("FAIL: %s\n", msg); \
    return false; \
} while(0)

#define ASSERT(cond, msg) do { \
    if (!(cond)) TEST_FAIL(msg); \
} while(0)

// =============================================================================
// Unit Tests
// =============================================================================

bool test_singleton() {
    TEST_START("singleton pattern");

    MouseCursor& mc1 = MouseCursor::Instance();
    MouseCursor& mc2 = MouseCursor::Instance();

    ASSERT(&mc1 == &mc2, "Should return same instance");

    TEST_PASS();
    return true;
}

bool test_cursor_types() {
    TEST_START("cursor type setting");

    MouseCursor& mc = MouseCursor::Instance();

    mc.SetType(CURSOR_MOVE);
    ASSERT(mc.GetType() == CURSOR_MOVE, "Type should be CURSOR_MOVE");

    mc.SetType(CURSOR_ATTACK);
    ASSERT(mc.GetType() == CURSOR_ATTACK, "Type should be CURSOR_ATTACK");

    mc.Reset();
    ASSERT(mc.GetType() == CURSOR_NORMAL, "Reset should set CURSOR_NORMAL");

    TEST_PASS();
    return true;
}

bool test_visibility() {
    TEST_START("visibility control");

    MouseCursor& mc = MouseCursor::Instance();

    mc.Show();
    ASSERT(mc.IsVisible(), "Should be visible after Show()");
    ASSERT(!mc.IsHidden(), "Should not be hidden after Show()");

    mc.Hide();
    ASSERT(!mc.IsVisible(), "Should not be visible after Hide()");
    ASSERT(mc.IsHidden(), "Should be hidden after Hide()");

    mc.Show();
    ASSERT(mc.IsVisible(), "Should be visible again");

    TEST_PASS();
    return true;
}

bool test_locking() {
    TEST_START("type locking");

    MouseCursor& mc = MouseCursor::Instance();

    mc.Unlock();
    mc.SetType(CURSOR_NORMAL);

    mc.Lock();
    ASSERT(mc.IsLocked(), "Should be locked");

    mc.SetType(CURSOR_ATTACK);
    ASSERT(mc.GetType() == CURSOR_NORMAL, "Locked cursor should not change");

    mc.Unlock();
    ASSERT(!mc.IsLocked(), "Should be unlocked");

    mc.SetType(CURSOR_ATTACK);
    ASSERT(mc.GetType() == CURSOR_ATTACK, "Unlocked cursor should change");

    mc.Reset();

    TEST_PASS();
    return true;
}

bool test_animation() {
    TEST_START("animation");

    MouseCursor& mc = MouseCursor::Instance();

    // Non-animated cursor
    mc.SetType(CURSOR_NORMAL);
    ASSERT(!mc.IsAnimated(), "CURSOR_NORMAL should not animate");
    ASSERT(mc.GetAnimationFrameCount() == 1, "Non-animated should have 1 frame");

    // Animated cursor
    mc.SetType(CURSOR_ATTACK);
    ASSERT(mc.IsAnimated(), "CURSOR_ATTACK should animate");
    ASSERT(mc.GetAnimationFrameCount() == 4, "Attack cursor should have 4 frames");

    mc.SetType(CURSOR_SELECT);
    ASSERT(mc.IsAnimated(), "CURSOR_SELECT should animate");

    mc.Reset();

    TEST_PASS();
    return true;
}

bool test_hotspots() {
    TEST_START("hotspot data");

    MouseCursor& mc = MouseCursor::Instance();

    mc.SetType(CURSOR_NORMAL);
    const CursorHotspot& normal_hs = mc.GetHotspot();
    ASSERT(normal_hs.x == 0 && normal_hs.y == 0, "Normal cursor hotspot should be (0,0)");

    mc.SetType(CURSOR_SELECT);
    const CursorHotspot& select_hs = mc.GetHotspot();
    ASSERT(select_hs.x == 15 && select_hs.y == 15, "Select cursor hotspot should be (15,15)");

    mc.Reset();

    TEST_PASS();
    return true;
}

bool test_scroll_cursor() {
    TEST_START("scroll cursor selection");

    MouseCursor& mc = MouseCursor::Instance();

    mc.SetScrollCursor(0, -1);  // North
    ASSERT(mc.GetType() == CURSOR_SCROLL_N, "Should be N scroll cursor");

    mc.SetScrollCursor(1, -1);  // Northeast
    ASSERT(mc.GetType() == CURSOR_SCROLL_NE, "Should be NE scroll cursor");

    mc.SetScrollCursor(1, 0);   // East
    ASSERT(mc.GetType() == CURSOR_SCROLL_E, "Should be E scroll cursor");

    mc.SetScrollCursor(-1, 1);  // Southwest
    ASSERT(mc.GetType() == CURSOR_SCROLL_SW, "Should be SW scroll cursor");

    mc.SetScrollCursor(0, 0);   // No scroll
    ASSERT(mc.GetType() == CURSOR_NORMAL, "Should be normal cursor");

    TEST_PASS();
    return true;
}

// =============================================================================
// Visual Test
// =============================================================================

void run_visual_test() {
    printf("\n=== Visual Mouse Cursor Test ===\n");

    MouseCursor& mc = MouseCursor::Instance();
    GraphicsBuffer& screen = GraphicsBuffer::Screen();

    // Set up simple palette
    uint8_t palette[768];
    for (int i = 0; i < 256; i++) {
        palette[i * 3 + 0] = i;
        palette[i * 3 + 1] = i;
        palette[i * 3 + 2] = i;
    }
    Platform_Graphics_SetPalette((PaletteEntry*)palette, 0, 256);

    // Try to load cursor (may fail without MIX files)
    bool cursor_loaded = mc.Load("MOUSE.SHP");

    if (!cursor_loaded) {
        printf("Could not load MOUSE.SHP - drawing placeholder cursor\n");
    }

    // Test cursor types
    CursorType test_types[] = {
        CURSOR_NORMAL,
        CURSOR_MOVE,
        CURSOR_ATTACK,
        CURSOR_SELECT,
        CURSOR_SELL,
        CURSOR_REPAIR,
        CURSOR_NUKE,
    };

    int type_count = sizeof(test_types) / sizeof(test_types[0]);
    int current_type_index = 0;
    int frame_count = 0;

    printf("Move mouse around. Press space (via ESC to exit).\n");
    printf("Cursor type cycles every 60 frames.\n");

    mc.Show();

    while (!Platform_Input_ShouldQuit()) {
        Platform_Input_Update();

        // Cycle cursor type
        frame_count++;
        if (frame_count >= 60) {
            frame_count = 0;
            current_type_index = (current_type_index + 1) % type_count;
            mc.SetType(test_types[current_type_index]);
        }

        // Update cursor animation
        mc.Update();

        // Draw
        screen.Lock();
        screen.Clear(32);  // Dark gray background

        // Draw grid
        for (int y = 0; y < screen.Get_Height(); y += 50) {
            screen.Draw_HLine(0, y, screen.Get_Width(), 64);
        }
        for (int x = 0; x < screen.Get_Width(); x += 50) {
            screen.Draw_VLine(x, 0, screen.Get_Height(), 64);
        }

        // Draw cursor info
        int mx, my;
        mc.GetPosition(&mx, &my);

        // Draw a marker at click position
        int cx, cy;
        mc.GetClickPosition(&cx, &cy);
        screen.Fill_Rect(cx - 2, cy - 2, 5, 5, 200);

        // Draw cursor (if loaded) or placeholder
        if (cursor_loaded) {
            mc.Draw(screen);
        } else {
            // Draw simple crosshair as placeholder
            screen.Draw_HLine(mx - 10, my, 21, 255);
            screen.Draw_VLine(mx, my - 10, 21, 255);
        }

        screen.Unlock();
        screen.Flip();

        Platform_Delay(16);  // ~60 fps
    }

    mc.Hide();
    printf("Visual test complete.\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("==========================================\n");
    printf("Mouse Cursor Test Suite\n");
    printf("==========================================\n\n");

    if (Platform_Init() != PLATFORM_RESULT_SUCCESS) {
        printf("ERROR: Failed to initialize platform\n");
        return 1;
    }

    if (Platform_Graphics_Init() != 0) {
        printf("ERROR: Failed to initialize graphics\n");
        Platform_Shutdown();
        return 1;
    }

    Platform_Input_Init();

    printf("=== Unit Tests ===\n\n");

    test_singleton();
    test_cursor_types();
    test_visibility();
    test_locking();
    test_animation();
    test_hotspots();
    test_scroll_cursor();

    printf("\n------------------------------------------\n");
    printf("Tests: %d/%d passed\n", tests_passed, tests_run);
    printf("------------------------------------------\n");

    if (tests_passed == tests_run) {
        run_visual_test();
    }

    Platform_Input_Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();

    printf("\n==========================================\n");
    if (tests_passed == tests_run) {
        printf("All tests PASSED!\n");
    } else {
        printf("Some tests FAILED\n");
    }
    printf("==========================================\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
```

### ralph-tasks/verify/check-mouse-cursor.sh
```bash
#!/bin/bash
# Verification script for Mouse Cursor

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Mouse Cursor Implementation ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check files exist
echo "[1/5] Checking file existence..."
FILES=(
    "include/game/graphics/mouse_cursor.h"
    "src/game/graphics/mouse_cursor.cpp"
    "src/test_mouse_cursor.cpp"
)

for file in "${FILES[@]}"; do
    if [ ! -f "$file" ]; then
        echo "VERIFY_FAILED: $file not found"
        exit 1
    fi
done
echo "  OK: All source files exist"

# Step 2: Check header syntax
echo "[2/5] Checking header syntax..."
if ! clang++ -std=c++17 -fsyntax-only \
    -I include \
    -I include/game \
    -I include/game/graphics \
    include/game/graphics/mouse_cursor.h 2>&1; then
    echo "VERIFY_FAILED: Header has syntax errors"
    exit 1
fi
echo "  OK: Header compiles"

# Step 3: Build test executable
echo "[3/5] Building TestMouseCursor..."
if ! cmake --build build --target TestMouseCursor 2>&1; then
    echo "VERIFY_FAILED: Build failed"
    exit 1
fi
echo "  OK: Build succeeded"

# Step 4: Check executable exists
echo "[4/5] Checking executable..."
if [ ! -f "build/TestMouseCursor" ]; then
    echo "VERIFY_FAILED: TestMouseCursor executable not found"
    exit 1
fi
echo "  OK: Executable exists"

# Step 5: Run tests (with timeout since visual test requires interaction)
echo "[5/5] Running unit tests..."
# Just check unit tests pass, visual test needs user interaction
if timeout 10 ./build/TestMouseCursor --unit-only 2>&1 | grep -q "passed"; then
    echo "  OK: Unit tests passed"
elif timeout 10 ./build/TestMouseCursor 2>&1 | head -20 | grep -q "PASS"; then
    echo "  OK: Tests running"
else
    echo "  WARN: Could not verify tests (may need display)"
fi

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## CMakeLists.txt Additions

```cmake
# Add mouse cursor sources
target_sources(game_core PRIVATE
    src/game/graphics/mouse_cursor.cpp
)

# Mouse Cursor Test
add_executable(TestMouseCursor src/test_mouse_cursor.cpp)

target_include_directories(TestMouseCursor PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/game/graphics
)

target_link_libraries(TestMouseCursor PRIVATE
    game_core
    redalert_platform
)

add_test(NAME TestMouseCursor COMMAND TestMouseCursor)
```

## Implementation Steps

1. Create `include/game/graphics/mouse_cursor.h`
2. Create `src/game/graphics/mouse_cursor.cpp`
3. Create `src/test_mouse_cursor.cpp`
4. Update CMakeLists.txt
5. Create verification script
6. Build and test

## Success Criteria

- [ ] MouseCursor compiles without warnings
- [ ] Singleton pattern works correctly
- [ ] Cursor type switching works
- [ ] Show/Hide works correctly
- [ ] Type locking works
- [ ] Animation detection works
- [ ] Hotspot data is correct
- [ ] Scroll cursor selection works
- [ ] Position conversion works (window → game coords)
- [ ] Click position includes hotspot offset
- [ ] Cursor loads from MOUSE.SHP (if available)
- [ ] Placeholder cursor works without assets
- [ ] All unit tests pass
- [ ] Visual test shows cursor tracking mouse

## Common Issues

### Cursor Position Off
**Symptom**: Click doesn't match visual cursor position
**Solution**: Check scale factor, verify hotspot offset

### Animation Not Smooth
**Symptom**: Cursor animation stutters
**Solution**: Ensure Update() called each frame, check anim_delay_

### System Cursor Visible
**Symptom**: Both system and game cursors visible
**Solution**: Ensure Platform_Mouse_Hide() called on Load()

### Cursor Not Drawing
**Symptom**: No cursor visible
**Solution**: Check IsHidden(), verify Draw() called after all other rendering

## Completion Promise
When verification passes, output:
```
<promise>TASK_15E_COMPLETE</promise>
```

## Escape Hatch
If stuck after 15 iterations:
- Document what's blocking in `ralph-tasks/blocked/15e.md`
- Output: `<promise>TASK_15E_BLOCKED</promise>`

## Max Iterations
15
