# Task 15g: Viewport & Scrolling

| Field | Value |
|-------|-------|
| **Task ID** | 15g |
| **Task Name** | Viewport & Scrolling |
| **Phase** | 15 - Graphics Integration |
| **Depends On** | 15a (GraphicsBuffer), 15f (Render Pipeline) |
| **Estimated Complexity** | Medium (~800 lines) |

---

## Context

The viewport system manages which portion of the game world is visible on screen. In C&C Red Alert, the map can be much larger than the visible area (up to 128x128 cells = 3072x3072 pixels), so smooth scrolling is essential. The viewport must handle:

1. **Edge scrolling** - Moving mouse to screen edges scrolls the map
2. **Keyboard scrolling** - Arrow keys for precise movement
3. **Jump scrolling** - Clicking radar minimap jumps to location
4. **Unit following** - Viewport can track selected units
5. **Smooth vs instant scrolling** - Gradual movement for edge scroll, instant for radar clicks

The original game used the "lepton" coordinate system (256 leptons = 1 cell = 24 pixels) for sub-pixel precision.

---

## Objective

Create a complete viewport management system with smooth scrolling, edge detection, map bounds clamping, and coordinate conversion utilities.

---

## Technical Background

### Coordinate Systems

C&C uses multiple coordinate systems:

```
Cell Coordinates:    0-127 (map grid position)
Lepton Coordinates:  0-32767 (256 leptons per cell)
Pixel Coordinates:   0-3071 (24 pixels per cell)
Screen Coordinates:  0-479 x 0-383 (visible tactical area)
```

Conversion formulas:
- Cell → Pixel: `pixel = cell * 24`
- Pixel → Cell: `cell = pixel / 24`
- Lepton → Pixel: `pixel = (lepton * 24) / 256`
- Pixel → Lepton: `lepton = (pixel * 256) / 24`

### Screen Layout

```
+----------------------------------+----------------+
|          Tab Bar (16px)          |    Sidebar     |
+----------------------------------+   (160px)      |
|                                  |                |
|     Tactical Area (480x384)      |   Cameos       |
|                                  |   Buttons      |
|     (Viewport into world map)    |   Radar        |
|                                  |   Power Bar    |
|                                  |   Credits      |
+----------------------------------+----------------+
```

### Scroll Behavior

1. **Edge Scroll Zone** - 16-pixel band at screen edges
2. **Scroll Speed** - Varies by distance into edge zone
3. **Acceleration** - Scroll speeds up if edge is held
4. **Keyboard Scroll** - Fixed speed, instant response
5. **Map Bounds** - Cannot scroll beyond map edges

---

## Deliverables

### 1. Viewport Class

**File: `src/game/viewport.h`**

```cpp
// src/game/viewport.h
// Viewport management for map scrolling
// Task 15g - Viewport & Scrolling

#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <cstdint>

// Screen layout constants
static const int SCREEN_WIDTH = 640;
static const int SCREEN_HEIGHT = 400;
static const int SIDEBAR_WIDTH = 160;
static const int TAB_HEIGHT = 16;
static const int TACTICAL_WIDTH = 480;   // SCREEN_WIDTH - SIDEBAR_WIDTH
static const int TACTICAL_HEIGHT = 384;  // SCREEN_HEIGHT - TAB_HEIGHT

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

// Scroll direction flags
enum ScrollDirection : uint8_t {
    SCROLL_NONE  = 0x00,
    SCROLL_UP    = 0x01,
    SCROLL_DOWN  = 0x02,
    SCROLL_LEFT  = 0x04,
    SCROLL_RIGHT = 0x08
};

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

class Viewport {
public:
    // Current viewport position in world pixels
    int x;      // Left edge in world pixels
    int y;      // Top edge in world pixels
    int width;  // Viewport width in pixels (usually TACTICAL_WIDTH)
    int height; // Viewport height in pixels (usually TACTICAL_HEIGHT)

    // Singleton access
    static Viewport& Instance();

    // Initialization
    void Initialize();
    void Reset();

    // Map bounds
    void SetMapSize(int cells_wide, int cells_high);
    int GetMapPixelWidth() const { return map_width_ * TILE_PIXEL_WIDTH; }
    int GetMapPixelHeight() const { return map_height_ * TILE_PIXEL_HEIGHT; }
    int GetMapCellWidth() const { return map_width_; }
    int GetMapCellHeight() const { return map_height_; }

    // Scrolling
    void Scroll(int delta_x, int delta_y);
    void ScrollTo(int world_x, int world_y);
    void CenterOn(int world_x, int world_y);
    void CenterOnCell(int cell_x, int cell_y);
    void CenterOnCoord(COORDINATE coord);
    void JumpTo(int world_x, int world_y);  // Instant move (no smooth scroll)

    // Edge scrolling (call each frame)
    void UpdateEdgeScroll(int mouse_x, int mouse_y);
    void UpdateKeyboardScroll(bool up, bool down, bool left, bool right);

    // Scroll control
    void EnableScroll(bool enable) { scroll_enabled_ = enable; }
    bool IsScrollEnabled() const { return scroll_enabled_; }
    void SetScrollSpeed(int speed) { scroll_speed_multiplier_ = speed; }
    int GetScrollSpeed() const { return scroll_speed_multiplier_; }

    // Edge scroll state
    uint8_t GetCurrentScrollDirection() const { return current_scroll_direction_; }
    bool IsScrolling() const { return current_scroll_direction_ != SCROLL_NONE; }

    // Target tracking (follow unit)
    void SetTrackTarget(COORDINATE coord);
    void ClearTrackTarget();
    bool HasTrackTarget() const { return tracking_enabled_; }
    void UpdateTracking();

    // Coordinate conversion
    void WorldToScreen(int world_x, int world_y, int& screen_x, int& screen_y) const;
    void ScreenToWorld(int screen_x, int screen_y, int& world_x, int& world_y) const;
    void WorldToCell(int world_x, int world_y, int& cell_x, int& cell_y) const;
    void CellToWorld(int cell_x, int cell_y, int& world_x, int& world_y) const;
    void ScreenToCell(int screen_x, int screen_y, int& cell_x, int& cell_y) const;
    void CellToScreen(int cell_x, int cell_y, int& screen_x, int& screen_y) const;

    // Lepton coordinate conversion
    void LeptonToPixel(int lepton_x, int lepton_y, int& pixel_x, int& pixel_y) const;
    void PixelToLepton(int pixel_x, int pixel_y, int& lepton_x, int& lepton_y) const;
    void CoordToScreen(COORDINATE coord, int& screen_x, int& screen_y) const;

    // Visibility testing
    bool IsPointVisible(int world_x, int world_y) const;
    bool IsRectVisible(int world_x, int world_y, int width, int height) const;
    bool IsCellVisible(int cell_x, int cell_y) const;
    bool IsCoordVisible(COORDINATE coord) const;

    // Get visible cell range
    void GetVisibleCellRange(int& start_x, int& start_y, int& end_x, int& end_y) const;

    // Bounds clamping
    void ClampToBounds();

private:
    Viewport();

    // Map dimensions in cells
    int map_width_;
    int map_height_;

    // Scroll state
    bool scroll_enabled_;
    int scroll_speed_multiplier_;
    uint8_t current_scroll_direction_;
    int scroll_accel_counter_;  // Frames of continuous scrolling

    // Target tracking
    bool tracking_enabled_;
    COORDINATE track_target_;

    // Calculate scroll speed based on edge distance and acceleration
    int CalculateEdgeScrollSpeed(int distance_into_zone) const;
};

// Global viewport access (for convenience)
extern Viewport& TheViewport;

#endif // VIEWPORT_H
```

### 2. Viewport Implementation

**File: `src/game/viewport.cpp`**

```cpp
// src/game/viewport.cpp
// Viewport management implementation
// Task 15g - Viewport & Scrolling

#include "viewport.h"
#include "platform.h"
#include <algorithm>

// Global viewport reference
Viewport& TheViewport = Viewport::Instance();

Viewport::Viewport()
    : x(0)
    , y(0)
    , width(TACTICAL_WIDTH)
    , height(TACTICAL_HEIGHT)
    , map_width_(64)
    , map_height_(64)
    , scroll_enabled_(true)
    , scroll_speed_multiplier_(100)  // 100% = normal speed
    , current_scroll_direction_(SCROLL_NONE)
    , scroll_accel_counter_(0)
    , tracking_enabled_(false)
    , track_target_(0) {
}

Viewport& Viewport::Instance() {
    static Viewport instance;
    return instance;
}

void Viewport::Initialize() {
    x = 0;
    y = 0;
    width = TACTICAL_WIDTH;
    height = TACTICAL_HEIGHT;
    scroll_enabled_ = true;
    current_scroll_direction_ = SCROLL_NONE;
    scroll_accel_counter_ = 0;
    tracking_enabled_ = false;
}

void Viewport::Reset() {
    Initialize();
}

void Viewport::SetMapSize(int cells_wide, int cells_high) {
    map_width_ = cells_wide;
    map_height_ = cells_high;

    // Re-clamp viewport to new bounds
    ClampToBounds();
}

void Viewport::Scroll(int delta_x, int delta_y) {
    if (!scroll_enabled_) return;

    // Apply scroll speed multiplier
    delta_x = (delta_x * scroll_speed_multiplier_) / 100;
    delta_y = (delta_y * scroll_speed_multiplier_) / 100;

    x += delta_x;
    y += delta_y;

    ClampToBounds();
}

void Viewport::ScrollTo(int world_x, int world_y) {
    x = world_x;
    y = world_y;
    ClampToBounds();
}

void Viewport::CenterOn(int world_x, int world_y) {
    x = world_x - width / 2;
    y = world_y - height / 2;
    ClampToBounds();
}

void Viewport::CenterOnCell(int cell_x, int cell_y) {
    int world_x = cell_x * TILE_PIXEL_WIDTH + TILE_PIXEL_WIDTH / 2;
    int world_y = cell_y * TILE_PIXEL_HEIGHT + TILE_PIXEL_HEIGHT / 2;
    CenterOn(world_x, world_y);
}

void Viewport::CenterOnCoord(COORDINATE coord) {
    int lepton_x = COORD_X(coord);
    int lepton_y = COORD_Y(coord);
    int pixel_x, pixel_y;
    LeptonToPixel(lepton_x, lepton_y, pixel_x, pixel_y);
    CenterOn(pixel_x, pixel_y);
}

void Viewport::JumpTo(int world_x, int world_y) {
    // Same as ScrollTo but could add animation later
    ScrollTo(world_x, world_y);
}

void Viewport::UpdateEdgeScroll(int mouse_x, int mouse_y) {
    if (!scroll_enabled_) {
        current_scroll_direction_ = SCROLL_NONE;
        scroll_accel_counter_ = 0;
        return;
    }

    // Check if mouse is in tactical area
    if (mouse_x >= TACTICAL_WIDTH || mouse_y < TAB_HEIGHT) {
        current_scroll_direction_ = SCROLL_NONE;
        scroll_accel_counter_ = 0;
        return;
    }

    uint8_t scroll_dir = SCROLL_NONE;
    int scroll_x = 0;
    int scroll_y = 0;

    // Adjust mouse_y for tactical area (subtract tab height)
    int tactical_y = mouse_y - TAB_HEIGHT;

    // Check left edge
    if (mouse_x < EDGE_SCROLL_ZONE) {
        int distance = EDGE_SCROLL_ZONE - mouse_x;
        scroll_x = -CalculateEdgeScrollSpeed(distance);
        scroll_dir |= SCROLL_LEFT;
    }
    // Check right edge
    else if (mouse_x >= TACTICAL_WIDTH - EDGE_SCROLL_ZONE) {
        int distance = mouse_x - (TACTICAL_WIDTH - EDGE_SCROLL_ZONE);
        scroll_x = CalculateEdgeScrollSpeed(distance);
        scroll_dir |= SCROLL_RIGHT;
    }

    // Check top edge
    if (tactical_y < EDGE_SCROLL_ZONE) {
        int distance = EDGE_SCROLL_ZONE - tactical_y;
        scroll_y = -CalculateEdgeScrollSpeed(distance);
        scroll_dir |= SCROLL_UP;
    }
    // Check bottom edge
    else if (tactical_y >= TACTICAL_HEIGHT - EDGE_SCROLL_ZONE) {
        int distance = tactical_y - (TACTICAL_HEIGHT - EDGE_SCROLL_ZONE);
        scroll_y = CalculateEdgeScrollSpeed(distance);
        scroll_dir |= SCROLL_DOWN;
    }

    // Update acceleration counter
    if (scroll_dir != SCROLL_NONE) {
        if (scroll_accel_counter_ < SCROLL_ACCEL_FRAMES) {
            scroll_accel_counter_++;
        }
    } else {
        scroll_accel_counter_ = 0;
    }

    current_scroll_direction_ = scroll_dir;

    // Apply scroll
    if (scroll_x != 0 || scroll_y != 0) {
        Scroll(scroll_x, scroll_y);
    }
}

void Viewport::UpdateKeyboardScroll(bool up, bool down, bool left, bool right) {
    if (!scroll_enabled_) return;

    int scroll_x = 0;
    int scroll_y = 0;
    uint8_t scroll_dir = SCROLL_NONE;

    if (up) {
        scroll_y = -KEYBOARD_SCROLL_SPEED;
        scroll_dir |= SCROLL_UP;
    }
    if (down) {
        scroll_y = KEYBOARD_SCROLL_SPEED;
        scroll_dir |= SCROLL_DOWN;
    }
    if (left) {
        scroll_x = -KEYBOARD_SCROLL_SPEED;
        scroll_dir |= SCROLL_LEFT;
    }
    if (right) {
        scroll_x = KEYBOARD_SCROLL_SPEED;
        scroll_dir |= SCROLL_RIGHT;
    }

    // Keyboard scroll doesn't use acceleration
    if (scroll_dir != SCROLL_NONE) {
        current_scroll_direction_ = scroll_dir;
        Scroll(scroll_x, scroll_y);
    }
}

int Viewport::CalculateEdgeScrollSpeed(int distance_into_zone) const {
    // Base speed varies linearly with distance into edge zone
    float zone_ratio = static_cast<float>(distance_into_zone) / EDGE_SCROLL_ZONE;
    int base_speed = MIN_SCROLL_SPEED + static_cast<int>((MAX_SCROLL_SPEED - MIN_SCROLL_SPEED) * zone_ratio);

    // Apply acceleration based on how long we've been scrolling
    float accel_ratio = static_cast<float>(scroll_accel_counter_) / SCROLL_ACCEL_FRAMES;
    int accel_bonus = static_cast<int>(base_speed * accel_ratio * 0.5f);  // Up to 50% bonus

    return base_speed + accel_bonus;
}

void Viewport::SetTrackTarget(COORDINATE coord) {
    track_target_ = coord;
    tracking_enabled_ = true;
}

void Viewport::ClearTrackTarget() {
    tracking_enabled_ = false;
}

void Viewport::UpdateTracking() {
    if (!tracking_enabled_) return;

    // Smoothly move viewport towards tracked target
    int target_pixel_x, target_pixel_y;
    LeptonToPixel(COORD_X(track_target_), COORD_Y(track_target_), target_pixel_x, target_pixel_y);

    // Calculate center of viewport
    int center_x = x + width / 2;
    int center_y = y + height / 2;

    // Calculate distance to target
    int dx = target_pixel_x - center_x;
    int dy = target_pixel_y - center_y;

    // If target is too far from center, scroll towards it
    int dead_zone = 100;  // Pixels from center before scrolling starts

    if (abs(dx) > dead_zone || abs(dy) > dead_zone) {
        // Smooth scroll - move fraction of distance each frame
        int move_x = dx / 8;
        int move_y = dy / 8;

        // Ensure minimum movement
        if (move_x > 0 && move_x < 1) move_x = 1;
        if (move_x < 0 && move_x > -1) move_x = -1;
        if (move_y > 0 && move_y < 1) move_y = 1;
        if (move_y < 0 && move_y > -1) move_y = -1;

        Scroll(move_x, move_y);
    }
}

void Viewport::WorldToScreen(int world_x, int world_y, int& screen_x, int& screen_y) const {
    screen_x = world_x - x;
    screen_y = world_y - y + TAB_HEIGHT;
}

void Viewport::ScreenToWorld(int screen_x, int screen_y, int& world_x, int& world_y) const {
    world_x = screen_x + x;
    world_y = screen_y - TAB_HEIGHT + y;
}

void Viewport::WorldToCell(int world_x, int world_y, int& cell_x, int& cell_y) const {
    cell_x = world_x / TILE_PIXEL_WIDTH;
    cell_y = world_y / TILE_PIXEL_HEIGHT;
}

void Viewport::CellToWorld(int cell_x, int cell_y, int& world_x, int& world_y) const {
    world_x = cell_x * TILE_PIXEL_WIDTH;
    world_y = cell_y * TILE_PIXEL_HEIGHT;
}

void Viewport::ScreenToCell(int screen_x, int screen_y, int& cell_x, int& cell_y) const {
    int world_x, world_y;
    ScreenToWorld(screen_x, screen_y, world_x, world_y);
    WorldToCell(world_x, world_y, cell_x, cell_y);
}

void Viewport::CellToScreen(int cell_x, int cell_y, int& screen_x, int& screen_y) const {
    int world_x, world_y;
    CellToWorld(cell_x, cell_y, world_x, world_y);
    WorldToScreen(world_x, world_y, screen_x, screen_y);
}

void Viewport::LeptonToPixel(int lepton_x, int lepton_y, int& pixel_x, int& pixel_y) const {
    // 256 leptons = 1 cell = 24 pixels
    // pixel = (lepton * 24) / 256 = (lepton * 3) / 32
    pixel_x = (lepton_x * TILE_PIXEL_WIDTH) / LEPTONS_PER_CELL;
    pixel_y = (lepton_y * TILE_PIXEL_HEIGHT) / LEPTONS_PER_CELL;
}

void Viewport::PixelToLepton(int pixel_x, int pixel_y, int& lepton_x, int& lepton_y) const {
    // lepton = (pixel * 256) / 24 = (pixel * 32) / 3
    lepton_x = (pixel_x * LEPTONS_PER_CELL) / TILE_PIXEL_WIDTH;
    lepton_y = (pixel_y * LEPTONS_PER_CELL) / TILE_PIXEL_HEIGHT;
}

void Viewport::CoordToScreen(COORDINATE coord, int& screen_x, int& screen_y) const {
    int pixel_x, pixel_y;
    LeptonToPixel(COORD_X(coord), COORD_Y(coord), pixel_x, pixel_y);
    WorldToScreen(pixel_x, pixel_y, screen_x, screen_y);
}

bool Viewport::IsPointVisible(int world_x, int world_y) const {
    return (world_x >= x && world_x < x + width &&
            world_y >= y && world_y < y + height);
}

bool Viewport::IsRectVisible(int world_x, int world_y, int rect_width, int rect_height) const {
    // Check if rectangles overlap
    return !(world_x + rect_width < x || world_x > x + width ||
             world_y + rect_height < y || world_y > y + height);
}

bool Viewport::IsCellVisible(int cell_x, int cell_y) const {
    int world_x = cell_x * TILE_PIXEL_WIDTH;
    int world_y = cell_y * TILE_PIXEL_HEIGHT;
    return IsRectVisible(world_x, world_y, TILE_PIXEL_WIDTH, TILE_PIXEL_HEIGHT);
}

bool Viewport::IsCoordVisible(COORDINATE coord) const {
    int pixel_x, pixel_y;
    LeptonToPixel(COORD_X(coord), COORD_Y(coord), pixel_x, pixel_y);
    return IsPointVisible(pixel_x, pixel_y);
}

void Viewport::GetVisibleCellRange(int& start_x, int& start_y, int& end_x, int& end_y) const {
    start_x = x / TILE_PIXEL_WIDTH;
    start_y = y / TILE_PIXEL_HEIGHT;
    end_x = (x + width) / TILE_PIXEL_WIDTH + 1;
    end_y = (y + height) / TILE_PIXEL_HEIGHT + 1;

    // Clamp to map bounds
    if (start_x < 0) start_x = 0;
    if (start_y < 0) start_y = 0;
    if (end_x > map_width_) end_x = map_width_;
    if (end_y > map_height_) end_y = map_height_;
}

void Viewport::ClampToBounds() {
    int max_x = GetMapPixelWidth() - width;
    int max_y = GetMapPixelHeight() - height;

    if (max_x < 0) max_x = 0;
    if (max_y < 0) max_y = 0;

    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x > max_x) x = max_x;
    if (y > max_y) y = max_y;
}
```

### 3. Smooth Scroll Manager

**File: `src/game/scroll_manager.h`**

```cpp
// src/game/scroll_manager.h
// Smooth scroll animation management
// Task 15g - Viewport & Scrolling

#ifndef SCROLL_MANAGER_H
#define SCROLL_MANAGER_H

#include "viewport.h"

// Scroll animation types
enum class ScrollAnimationType {
    INSTANT,    // Jump immediately
    LINEAR,     // Constant speed
    EASE_OUT,   // Start fast, slow down
    EASE_IN_OUT // Slow-fast-slow
};

// Pending scroll request
struct ScrollRequest {
    int target_x;
    int target_y;
    ScrollAnimationType type;
    int duration_frames;
    int elapsed_frames;
    bool active;

    ScrollRequest()
        : target_x(0), target_y(0)
        , type(ScrollAnimationType::INSTANT)
        , duration_frames(0), elapsed_frames(0)
        , active(false) {}
};

class ScrollManager {
public:
    static ScrollManager& Instance();

    // Start animated scroll
    void ScrollTo(int world_x, int world_y, ScrollAnimationType type, int duration_frames);
    void CenterOn(int world_x, int world_y, ScrollAnimationType type, int duration_frames);
    void CenterOnCell(int cell_x, int cell_y, ScrollAnimationType type, int duration_frames);

    // Instant scroll (cancels any animation)
    void JumpTo(int world_x, int world_y);
    void JumpToCell(int cell_x, int cell_y);

    // Cancel current animation
    void CancelScroll();

    // Update animation (call each frame)
    void Update();

    // Check state
    bool IsAnimating() const { return current_request_.active; }
    int GetTargetX() const { return current_request_.target_x; }
    int GetTargetY() const { return current_request_.target_y; }

private:
    ScrollManager();

    float EaseFunction(float t, ScrollAnimationType type) const;

    ScrollRequest current_request_;
    int start_x_;
    int start_y_;
};

#endif // SCROLL_MANAGER_H
```

### 4. Scroll Manager Implementation

**File: `src/game/scroll_manager.cpp`**

```cpp
// src/game/scroll_manager.cpp
// Smooth scroll animation implementation
// Task 15g - Viewport & Scrolling

#include "scroll_manager.h"
#include <cmath>

ScrollManager::ScrollManager()
    : start_x_(0)
    , start_y_(0) {
}

ScrollManager& ScrollManager::Instance() {
    static ScrollManager instance;
    return instance;
}

void ScrollManager::ScrollTo(int world_x, int world_y, ScrollAnimationType type, int duration_frames) {
    if (type == ScrollAnimationType::INSTANT || duration_frames <= 0) {
        JumpTo(world_x, world_y);
        return;
    }

    // Set up animation
    start_x_ = Viewport::Instance().x;
    start_y_ = Viewport::Instance().y;

    current_request_.target_x = world_x;
    current_request_.target_y = world_y;
    current_request_.type = type;
    current_request_.duration_frames = duration_frames;
    current_request_.elapsed_frames = 0;
    current_request_.active = true;
}

void ScrollManager::CenterOn(int world_x, int world_y, ScrollAnimationType type, int duration_frames) {
    int target_x = world_x - Viewport::Instance().width / 2;
    int target_y = world_y - Viewport::Instance().height / 2;
    ScrollTo(target_x, target_y, type, duration_frames);
}

void ScrollManager::CenterOnCell(int cell_x, int cell_y, ScrollAnimationType type, int duration_frames) {
    int world_x = cell_x * TILE_PIXEL_WIDTH + TILE_PIXEL_WIDTH / 2;
    int world_y = cell_y * TILE_PIXEL_HEIGHT + TILE_PIXEL_HEIGHT / 2;
    CenterOn(world_x, world_y, type, duration_frames);
}

void ScrollManager::JumpTo(int world_x, int world_y) {
    CancelScroll();
    Viewport::Instance().ScrollTo(world_x, world_y);
}

void ScrollManager::JumpToCell(int cell_x, int cell_y) {
    int world_x = cell_x * TILE_PIXEL_WIDTH;
    int world_y = cell_y * TILE_PIXEL_HEIGHT;
    JumpTo(world_x, world_y);
}

void ScrollManager::CancelScroll() {
    current_request_.active = false;
}

void ScrollManager::Update() {
    if (!current_request_.active) return;

    current_request_.elapsed_frames++;

    // Calculate progress (0.0 to 1.0)
    float t = static_cast<float>(current_request_.elapsed_frames) / current_request_.duration_frames;
    if (t >= 1.0f) {
        t = 1.0f;
        current_request_.active = false;
    }

    // Apply easing function
    float eased_t = EaseFunction(t, current_request_.type);

    // Interpolate position
    int new_x = start_x_ + static_cast<int>((current_request_.target_x - start_x_) * eased_t);
    int new_y = start_y_ + static_cast<int>((current_request_.target_y - start_y_) * eased_t);

    // Update viewport
    Viewport::Instance().ScrollTo(new_x, new_y);
}

float ScrollManager::EaseFunction(float t, ScrollAnimationType type) const {
    switch (type) {
        case ScrollAnimationType::LINEAR:
            return t;

        case ScrollAnimationType::EASE_OUT:
            // Quadratic ease-out: 1 - (1-t)^2
            return 1.0f - (1.0f - t) * (1.0f - t);

        case ScrollAnimationType::EASE_IN_OUT:
            // Smoothstep: 3t^2 - 2t^3
            return t * t * (3.0f - 2.0f * t);

        case ScrollAnimationType::INSTANT:
        default:
            return 1.0f;
    }
}
```

### 5. Input Integration

**File: `src/game/scroll_input.h`**

```cpp
// src/game/scroll_input.h
// Input handling for viewport scrolling
// Task 15g - Viewport & Scrolling

#ifndef SCROLL_INPUT_H
#define SCROLL_INPUT_H

#include "viewport.h"

// Key bindings for scrolling
struct ScrollKeyBindings {
    int key_up;
    int key_down;
    int key_left;
    int key_right;
    int key_home;       // Return to base
    int key_select_next; // Cycle to next unit

    ScrollKeyBindings();
};

class ScrollInputHandler {
public:
    static ScrollInputHandler& Instance();

    // Initialize with default key bindings
    void Initialize();

    // Set custom key bindings
    void SetKeyBindings(const ScrollKeyBindings& bindings);
    const ScrollKeyBindings& GetKeyBindings() const { return bindings_; }

    // Process input each frame
    void Update();

    // Enable/disable input processing
    void Enable(bool enabled) { enabled_ = enabled; }
    bool IsEnabled() const { return enabled_; }

    // Edge scroll enable (separate from keyboard)
    void EnableEdgeScroll(bool enabled) { edge_scroll_enabled_ = enabled; }
    bool IsEdgeScrollEnabled() const { return edge_scroll_enabled_; }

    // Get current mouse position for cursor changes
    int GetMouseX() const { return mouse_x_; }
    int GetMouseY() const { return mouse_y_; }

    // Check if mouse is at an edge (for cursor display)
    bool IsMouseAtEdge() const;
    uint8_t GetEdgeDirection() const;

private:
    ScrollInputHandler();

    void ProcessKeyboardInput();
    void ProcessEdgeScroll();
    void ProcessSpecialKeys();

    ScrollKeyBindings bindings_;
    bool enabled_;
    bool edge_scroll_enabled_;
    int mouse_x_;
    int mouse_y_;
};

#endif // SCROLL_INPUT_H
```

### 6. Input Handler Implementation

**File: `src/game/scroll_input.cpp`**

```cpp
// src/game/scroll_input.cpp
// Input handling for viewport scrolling
// Task 15g - Viewport & Scrolling

#include "scroll_input.h"
#include "scroll_manager.h"
#include "platform.h"

// Default key bindings
ScrollKeyBindings::ScrollKeyBindings()
    : key_up(KEY_UP)
    , key_down(KEY_DOWN)
    , key_left(KEY_LEFT)
    , key_right(KEY_RIGHT)
    , key_home(KEY_H)
    , key_select_next(KEY_N) {
}

ScrollInputHandler::ScrollInputHandler()
    : enabled_(true)
    , edge_scroll_enabled_(true)
    , mouse_x_(0)
    , mouse_y_(0) {
}

ScrollInputHandler& ScrollInputHandler::Instance() {
    static ScrollInputHandler instance;
    return instance;
}

void ScrollInputHandler::Initialize() {
    enabled_ = true;
    edge_scroll_enabled_ = true;
    bindings_ = ScrollKeyBindings();
}

void ScrollInputHandler::SetKeyBindings(const ScrollKeyBindings& bindings) {
    bindings_ = bindings;
}

void ScrollInputHandler::Update() {
    if (!enabled_) return;

    // Get current mouse position
    Platform_Mouse_GetPosition(&mouse_x_, &mouse_y_);

    // Account for window scaling (2x)
    mouse_x_ /= 2;
    mouse_y_ /= 2;

    // Process inputs
    ProcessKeyboardInput();
    ProcessEdgeScroll();
    ProcessSpecialKeys();

    // Update scroll animations
    ScrollManager::Instance().Update();

    // Update target tracking
    Viewport::Instance().UpdateTracking();
}

void ScrollInputHandler::ProcessKeyboardInput() {
    bool up = Platform_Key_Down(bindings_.key_up);
    bool down = Platform_Key_Down(bindings_.key_down);
    bool left = Platform_Key_Down(bindings_.key_left);
    bool right = Platform_Key_Down(bindings_.key_right);

    if (up || down || left || right) {
        // Cancel any animated scroll when using keyboard
        ScrollManager::Instance().CancelScroll();
        Viewport::Instance().UpdateKeyboardScroll(up, down, left, right);
    }
}

void ScrollInputHandler::ProcessEdgeScroll() {
    if (!edge_scroll_enabled_) return;

    // Skip edge scroll if an animation is in progress
    if (ScrollManager::Instance().IsAnimating()) return;

    // Update edge scrolling
    Viewport::Instance().UpdateEdgeScroll(mouse_x_, mouse_y_);
}

void ScrollInputHandler::ProcessSpecialKeys() {
    // Home key - jump to player base
    static bool home_pressed = false;
    if (Platform_Key_Down(bindings_.key_home)) {
        if (!home_pressed) {
            // Would jump to player's construction yard/MCV
            // For now, jump to map center
            int center_x = (Viewport::Instance().GetMapCellWidth() / 2) * TILE_PIXEL_WIDTH;
            int center_y = (Viewport::Instance().GetMapCellHeight() / 2) * TILE_PIXEL_HEIGHT;
            ScrollManager::Instance().CenterOn(center_x, center_y, ScrollAnimationType::EASE_OUT, 15);
            home_pressed = true;
        }
    } else {
        home_pressed = false;
    }
}

bool ScrollInputHandler::IsMouseAtEdge() const {
    return GetEdgeDirection() != SCROLL_NONE;
}

uint8_t ScrollInputHandler::GetEdgeDirection() const {
    uint8_t dir = SCROLL_NONE;

    // Only check if mouse is in tactical area
    if (mouse_x_ >= TACTICAL_WIDTH || mouse_y_ < TAB_HEIGHT) {
        return SCROLL_NONE;
    }

    int tactical_y = mouse_y_ - TAB_HEIGHT;

    if (mouse_x_ < EDGE_SCROLL_ZONE) dir |= SCROLL_LEFT;
    if (mouse_x_ >= TACTICAL_WIDTH - EDGE_SCROLL_ZONE) dir |= SCROLL_RIGHT;
    if (tactical_y < EDGE_SCROLL_ZONE) dir |= SCROLL_UP;
    if (tactical_y >= TACTICAL_HEIGHT - EDGE_SCROLL_ZONE) dir |= SCROLL_DOWN;

    return dir;
}
```

### 7. Test Program

**File: `tests/test_viewport.cpp`**

```cpp
// tests/test_viewport.cpp
// Test program for viewport and scrolling
// Task 15g - Viewport & Scrolling

#include "platform.h"
#include "viewport.h"
#include "scroll_manager.h"
#include "scroll_input.h"
#include "graphics_buffer.h"
#include "palette_manager.h"
#include "mouse_cursor.h"
#include <cstdio>
#include <cstring>

// Test coordinate conversion
bool Test_CoordinateConversion() {
    printf("Test: Coordinate Conversion... ");

    Viewport& vp = Viewport::Instance();
    vp.Initialize();
    vp.SetMapSize(64, 64);
    vp.ScrollTo(100, 50);

    // Test WorldToScreen
    int screen_x, screen_y;
    vp.WorldToScreen(200, 100, screen_x, screen_y);
    // Expected: screen_x = 200 - 100 = 100
    //           screen_y = 100 - 50 + 16 = 66
    if (screen_x != 100 || screen_y != 66) {
        printf("FAILED - WorldToScreen: got (%d, %d), expected (100, 66)\n", screen_x, screen_y);
        return false;
    }

    // Test ScreenToWorld
    int world_x, world_y;
    vp.ScreenToWorld(100, 66, world_x, world_y);
    if (world_x != 200 || world_y != 100) {
        printf("FAILED - ScreenToWorld: got (%d, %d), expected (200, 100)\n", world_x, world_y);
        return false;
    }

    // Test cell conversion
    int cell_x, cell_y;
    vp.WorldToCell(60, 48, cell_x, cell_y);
    if (cell_x != 2 || cell_y != 2) {
        printf("FAILED - WorldToCell: got (%d, %d), expected (2, 2)\n", cell_x, cell_y);
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_LeptonConversion() {
    printf("Test: Lepton Conversion... ");

    Viewport& vp = Viewport::Instance();

    // 256 leptons = 24 pixels
    int pixel_x, pixel_y;
    vp.LeptonToPixel(256, 512, pixel_x, pixel_y);
    if (pixel_x != 24 || pixel_y != 48) {
        printf("FAILED - LeptonToPixel: got (%d, %d), expected (24, 48)\n", pixel_x, pixel_y);
        return false;
    }

    int lepton_x, lepton_y;
    vp.PixelToLepton(24, 48, lepton_x, lepton_y);
    if (lepton_x != 256 || lepton_y != 512) {
        printf("FAILED - PixelToLepton: got (%d, %d), expected (256, 512)\n", lepton_x, lepton_y);
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_BoundsClamping() {
    printf("Test: Bounds Clamping... ");

    Viewport& vp = Viewport::Instance();
    vp.Initialize();
    vp.SetMapSize(32, 32);  // 768x768 pixel map

    // Try to scroll past left edge
    vp.ScrollTo(-100, 0);
    if (vp.x != 0) {
        printf("FAILED - Left clamp: x=%d, expected 0\n", vp.x);
        return false;
    }

    // Try to scroll past top edge
    vp.ScrollTo(0, -100);
    if (vp.y != 0) {
        printf("FAILED - Top clamp: y=%d, expected 0\n", vp.y);
        return false;
    }

    // Try to scroll past right edge (map is 768, viewport is 480)
    vp.ScrollTo(500, 0);
    int max_x = vp.GetMapPixelWidth() - vp.width;  // 768 - 480 = 288
    if (vp.x != max_x) {
        printf("FAILED - Right clamp: x=%d, expected %d\n", vp.x, max_x);
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_VisibilityCheck() {
    printf("Test: Visibility Check... ");

    Viewport& vp = Viewport::Instance();
    vp.Initialize();
    vp.SetMapSize(64, 64);
    vp.ScrollTo(100, 100);

    // Point inside viewport
    if (!vp.IsPointVisible(200, 200)) {
        printf("FAILED - Point should be visible\n");
        return false;
    }

    // Point outside viewport
    if (vp.IsPointVisible(50, 50)) {
        printf("FAILED - Point should not be visible\n");
        return false;
    }

    // Rectangle partially visible
    if (!vp.IsRectVisible(50, 50, 100, 100)) {
        printf("FAILED - Partial rect should be visible\n");
        return false;
    }

    // Cell visibility
    vp.ScrollTo(0, 0);
    if (!vp.IsCellVisible(5, 5)) {
        printf("FAILED - Cell (5,5) should be visible at origin\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_VisibleCellRange() {
    printf("Test: Visible Cell Range... ");

    Viewport& vp = Viewport::Instance();
    vp.Initialize();
    vp.SetMapSize(64, 64);
    vp.ScrollTo(48, 24);  // 2 cells right, 1 cell down

    int start_x, start_y, end_x, end_y;
    vp.GetVisibleCellRange(start_x, start_y, end_x, end_y);

    // Start should be (2, 1), end should be (22, 17) approximately
    if (start_x != 2 || start_y != 1) {
        printf("FAILED - Start cell: got (%d, %d), expected (2, 1)\n", start_x, start_y);
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_ScrollAnimation() {
    printf("Test: Scroll Animation... ");

    Viewport& vp = Viewport::Instance();
    vp.Initialize();
    vp.SetMapSize(64, 64);
    vp.ScrollTo(0, 0);

    ScrollManager& sm = ScrollManager::Instance();

    // Start linear scroll animation
    sm.ScrollTo(200, 100, ScrollAnimationType::LINEAR, 10);

    if (!sm.IsAnimating()) {
        printf("FAILED - Animation should be active\n");
        return false;
    }

    // Simulate 10 frames
    for (int i = 0; i < 10; i++) {
        sm.Update();
    }

    if (sm.IsAnimating()) {
        printf("FAILED - Animation should be complete\n");
        return false;
    }

    if (vp.x != 200 || vp.y != 100) {
        printf("FAILED - Final position: got (%d, %d), expected (200, 100)\n", vp.x, vp.y);
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_CenterOn() {
    printf("Test: Center On... ");

    Viewport& vp = Viewport::Instance();
    vp.Initialize();
    vp.SetMapSize(64, 64);

    // Center on a point
    vp.CenterOn(500, 400);

    // Viewport center should be at (500, 400)
    int center_x = vp.x + vp.width / 2;
    int center_y = vp.y + vp.height / 2;

    if (center_x != 500 || center_y != 400) {
        printf("FAILED - Center: got (%d, %d), expected (500, 400)\n", center_x, center_y);
        return false;
    }

    // Center on cell
    vp.CenterOnCell(10, 10);
    // Cell (10,10) is at pixel (240, 240) center at (252, 252)
    center_x = vp.x + vp.width / 2;
    center_y = vp.y + vp.height / 2;

    if (abs(center_x - 252) > 1 || abs(center_y - 252) > 1) {
        printf("FAILED - Cell center: got (%d, %d), expected ~(252, 252)\n", center_x, center_y);
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_CoordMacros() {
    printf("Test: COORDINATE Macros... ");

    COORDINATE coord = MAKE_COORD(0x1234, 0x5678);

    if (COORD_X(coord) != 0x1234) {
        printf("FAILED - COORD_X: got 0x%04X, expected 0x1234\n", COORD_X(coord));
        return false;
    }

    if (COORD_Y(coord) != 0x5678) {
        printf("FAILED - COORD_Y: got 0x%04X, expected 0x5678\n", COORD_Y(coord));
        return false;
    }

    CELL cell = MAKE_CELL(50, 30);
    if (CELL_X(cell) != 50 || CELL_Y(cell) != 30) {
        printf("FAILED - CELL macros: got (%d, %d), expected (50, 30)\n", CELL_X(cell), CELL_Y(cell));
        return false;
    }

    printf("PASSED\n");
    return true;
}

void Interactive_Test() {
    printf("\nInteractive Viewport Test - Press ESC to exit\n");
    printf("Arrow keys: Scroll\n");
    printf("H: Jump to center\n");
    printf("Click radar area to jump to location\n\n");

    // Initialize platform
    if (Platform_Graphics_Init() != 0) {
        printf("Failed to initialize graphics\n");
        return;
    }

    // Initialize subsystems
    PaletteManager::Instance().Initialize();
    MouseCursor::Instance().Initialize();
    ScrollInputHandler::Instance().Initialize();

    // Load default palette
    PaletteManager::Instance().LoadTheaterPalette(TheaterType::TEMPERATE);

    // Set up viewport
    Viewport::Instance().Initialize();
    Viewport::Instance().SetMapSize(128, 128);  // Large map

    // Create graphics buffer
    GraphicsBuffer buffer;
    buffer.Initialize();

    // Main loop
    bool running = true;

    while (running) {
        Platform_PumpEvents();

        if (Platform_Key_Down(KEY_ESCAPE)) {
            running = false;
        }

        // Process scroll input
        ScrollInputHandler::Instance().Update();

        // Render
        buffer.Lock();
        buffer.Clear(0);

        Viewport& vp = Viewport::Instance();

        // Draw checkerboard pattern representing map
        int start_x, start_y, end_x, end_y;
        vp.GetVisibleCellRange(start_x, start_y, end_x, end_y);

        for (int cy = start_y; cy < end_y; cy++) {
            for (int cx = start_x; cx < end_x; cx++) {
                int screen_x, screen_y;
                vp.CellToScreen(cx, cy, screen_x, screen_y);

                // Alternating colors
                uint8_t color = ((cx + cy) % 2) ? 21 : 22;

                // Highlight edge cells
                if (cx == 0 || cy == 0 || cx == vp.GetMapCellWidth()-1 || cy == vp.GetMapCellHeight()-1) {
                    color = 12;  // Red for map edges
                }

                buffer.FillRect(screen_x, screen_y, TILE_PIXEL_WIDTH, TILE_PIXEL_HEIGHT, color);
            }
        }

        // Draw sidebar placeholder
        buffer.FillRect(TACTICAL_WIDTH, 0, SIDEBAR_WIDTH, SCREEN_HEIGHT, 0);
        buffer.DrawVLine(TACTICAL_WIDTH, 0, SCREEN_HEIGHT, 15);

        // Draw tab bar placeholder
        buffer.FillRect(0, 0, TACTICAL_WIDTH, TAB_HEIGHT, 15);

        // Draw simple radar representation
        int radar_x = 496;
        int radar_y = 232;
        int radar_size = 128;
        buffer.DrawRect(radar_x, radar_y, radar_size, radar_size, 15);

        // Draw viewport position on radar
        float scale_x = static_cast<float>(radar_size) / vp.GetMapPixelWidth();
        float scale_y = static_cast<float>(radar_size) / vp.GetMapPixelHeight();
        int box_x = radar_x + static_cast<int>(vp.x * scale_x);
        int box_y = radar_y + static_cast<int>(vp.y * scale_y);
        int box_w = static_cast<int>(vp.width * scale_x);
        int box_h = static_cast<int>(vp.height * scale_y);
        buffer.DrawRect(box_x, box_y, box_w, box_h, 179);

        // Draw viewport position text area
        buffer.FillRect(radar_x, radar_y - 20, radar_size, 16, 0);

        // Draw mouse cursor
        int mouse_x, mouse_y;
        Platform_Mouse_GetPosition(&mouse_x, &mouse_y);
        mouse_x /= 2;
        mouse_y /= 2;
        MouseCursor::Instance().Draw(buffer, mouse_x, mouse_y);

        // Check for radar click
        if (Platform_Mouse_Button(0)) {  // Left click
            if (mouse_x >= radar_x && mouse_x < radar_x + radar_size &&
                mouse_y >= radar_y && mouse_y < radar_y + radar_size) {
                // Calculate target position
                int click_x = mouse_x - radar_x;
                int click_y = mouse_y - radar_y;
                int world_x = static_cast<int>(click_x / scale_x);
                int world_y = static_cast<int>(click_y / scale_y);
                ScrollManager::Instance().CenterOn(world_x, world_y, ScrollAnimationType::EASE_OUT, 20);
            }
        }

        buffer.Unlock();
        Platform_Graphics_Flip();

        Platform_Timer_Delay(16);
    }

    // Cleanup
    buffer.Shutdown();
    MouseCursor::Instance().Shutdown();
    PaletteManager::Instance().Shutdown();
    Platform_Graphics_Shutdown();
}

int main(int argc, char* argv[]) {
    printf("=== Viewport & Scrolling Tests ===\n\n");

    int passed = 0;
    int failed = 0;

    // Initialize platform
    Platform_Init();

    // Run unit tests
    if (Test_CoordinateConversion()) passed++; else failed++;
    if (Test_LeptonConversion()) passed++; else failed++;
    if (Test_BoundsClamping()) passed++; else failed++;
    if (Test_VisibilityCheck()) passed++; else failed++;
    if (Test_VisibleCellRange()) passed++; else failed++;
    if (Test_ScrollAnimation()) passed++; else failed++;
    if (Test_CenterOn()) passed++; else failed++;
    if (Test_CoordMacros()) passed++; else failed++;

    printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);

    // Interactive test
    if (argc > 1 && strcmp(argv[1], "-i") == 0) {
        Interactive_Test();
    } else {
        printf("\nRun with -i for interactive visual test\n");
    }

    Platform_Shutdown();
    return (failed == 0) ? 0 : 1;
}
```

---

## CMakeLists.txt Additions

Add to `src/game/CMakeLists.txt`:

```cmake
# Viewport & Scrolling (Task 15g)
target_sources(game PRIVATE
    viewport.h
    viewport.cpp
    scroll_manager.h
    scroll_manager.cpp
    scroll_input.h
    scroll_input.cpp
)
```

Add to `tests/CMakeLists.txt`:

```cmake
# Viewport test (Task 15g)
add_executable(test_viewport test_viewport.cpp)
target_link_libraries(test_viewport PRIVATE game platform)
add_test(NAME ViewportTest COMMAND test_viewport)
```

---

## Implementation Steps

1. **Create viewport.h** - Define Viewport class with all coordinate conversion and scrolling methods
2. **Create viewport.cpp** - Implement viewport logic, bounds clamping, edge scrolling
3. **Create scroll_manager.h/cpp** - Implement smooth scroll animations with easing
4. **Create scroll_input.h/cpp** - Handle keyboard and mouse edge scroll input
5. **Create test_viewport.cpp** - Unit tests and interactive demo
6. **Update CMakeLists.txt** - Add new source files
7. **Build and run tests** - Verify all functionality
8. **Test interactive mode** - Verify scrolling feels responsive

---

## Success Criteria

- [ ] All coordinate conversions correct (world↔screen↔cell↔lepton)
- [ ] Viewport clamps to map bounds
- [ ] Edge scrolling works with acceleration
- [ ] Keyboard scrolling works with arrow keys
- [ ] Smooth scroll animations work (linear, ease-out, ease-in-out)
- [ ] CenterOn/CenterOnCell work correctly
- [ ] Visibility testing accurate
- [ ] GetVisibleCellRange returns correct range
- [ ] COORDINATE and CELL macros work correctly
- [ ] All unit tests pass
- [ ] Interactive test shows smooth, responsive scrolling

---

## Common Issues

1. **Screen position off by TAB_HEIGHT**
   - Remember to add TAB_HEIGHT when converting to screen coordinates
   - WorldToScreen adds it, ScreenToWorld subtracts it

2. **Edge scroll not triggering**
   - Check mouse is in tactical area (not sidebar/tab)
   - Verify EDGE_SCROLL_ZONE value

3. **Scroll feels jerky**
   - Ensure consistent frame timing
   - Check scroll speed calculation

4. **Map scrolls past edges**
   - Verify ClampToBounds is called after every scroll
   - Check max_x/max_y calculation

5. **Lepton conversion precision**
   - Use integer math: `(lepton * 24) / 256`
   - Avoid floating point for game logic

---

## Verification Script

```bash
#!/bin/bash
set -e

echo "=== Task 15g Verification ==="

# Build
echo "Building..."
cd build
cmake --build . --target test_viewport

# Run tests
echo "Running unit tests..."
./tests/test_viewport

# Check files
echo "Checking source files..."
for file in \
    "../src/game/viewport.h" \
    "../src/game/viewport.cpp" \
    "../src/game/scroll_manager.h" \
    "../src/game/scroll_manager.cpp" \
    "../src/game/scroll_input.h" \
    "../src/game/scroll_input.cpp"
do
    if [ ! -f "$file" ]; then
        echo "ERROR: Missing $file"
        exit 1
    fi
done

echo ""
echo "=== Task 15g Verification PASSED ==="
echo "Run './tests/test_viewport -i' for interactive test"
```

---

## Completion Promise

When this task is complete:
1. Viewport class managing map view position
2. Complete coordinate conversion (world, screen, cell, lepton)
3. Bounds clamping to map edges
4. Edge scrolling with acceleration
5. Keyboard scrolling with arrow keys
6. Smooth scroll animations with multiple easing types
7. Target tracking for following units
8. All unit tests passing
9. Interactive demo showing responsive scrolling

When verification passes, output:
<promise>TASK_15G_COMPLETE</promise>

## Escape Hatch

If you get stuck:
1. Start with basic ScrollTo/ClampToBounds
2. Add coordinate conversion one type at a time
3. Skip smooth animations initially (instant scroll works)
4. Skip edge scroll acceleration (constant speed is fine)
5. Test each conversion function in isolation

If blocked after max iterations, document blockers and output:
<promise>TASK_15G_BLOCKED</promise>

## Max Iterations
5
