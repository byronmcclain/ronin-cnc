# Task 16d: Mouse Handler & Coordinate System

| Field | Value |
|-------|-------|
| **Task ID** | 16d |
| **Task Name** | Mouse Handler & Coordinate System |
| **Phase** | 16 - Input Integration |
| **Depends On** | 16a (Input Foundation), 15g (Viewport) |
| **Estimated Complexity** | Medium-High (~600 lines) |

---

## Dependencies

- Task 16a (Input Foundation) must be complete
- Phase 15g (Viewport) must be complete for coordinate conversion
- MouseState structure must be functional

---

## Context

The mouse handler manages all mouse input including position tracking across multiple coordinate systems, click detection, drag operations, and cursor context (what the mouse is pointing at). This is one of the most complex input components because:

1. **Multiple Coordinate Spaces**: Window → Screen → World → Cell
2. **UI Regions**: Mouse behavior differs over tactical area, sidebar, radar
3. **Context Detection**: What's under the cursor affects cursor shape and actions
4. **Drag State Machine**: Track drag start, movement, end, and threshold

The original Red Alert used DirectInput for mouse with manual coordinate scaling. Our implementation uses SDL2 through the platform layer with comprehensive coordinate conversion.

---

## Objective

Create a mouse handler that:
1. Tracks mouse position in all coordinate systems
2. Detects which UI region the mouse is over
3. Identifies objects/terrain under the cursor
4. Manages drag selection box state
5. Provides cursor context for visual feedback

---

## Technical Background

### Coordinate Systems

```
Window Coordinates (raw from SDL):
  - Origin: top-left of window
  - Scale: actual window pixels (1280x800 if 2x scale)

Screen Coordinates (game logical):
  - Origin: top-left of screen
  - Scale: game pixels (640x400)
  - Formula: screen = window / window_scale

World Coordinates (map-relative):
  - Origin: top-left of map
  - Scale: game pixels
  - Formula: world = screen + viewport_offset

Cell Coordinates (tile grid):
  - Origin: top-left of map
  - Scale: tiles (24x24 pixels each)
  - Formula: cell = world / 24
```

### Screen Regions

```
+------------------+--------+
|    TAB BAR       |        |
|    (16px)        |        |
+------------------+ SIDEBAR|
|                  | (160px)|
|  TACTICAL AREA   |        |
|  (480x384)       |        |
|                  |        |
+------------------+--------+

Regions:
- TAB: y < 16
- SIDEBAR: x >= 480
- TACTICAL: x < 480 && y >= 16
- RADAR: Inside sidebar radar area
```

### Cursor Context

What the mouse is over determines:
- Cursor shape (pointer, target, move, etc.)
- Left-click action (select, attack, etc.)
- Right-click action (move, context menu)

---

## Deliverables

- [ ] `src/game/input/mouse_handler.h` - Handler class declaration
- [ ] `src/game/input/mouse_handler.cpp` - Handler implementation
- [ ] `src/game/input/cursor_context.h` - Cursor context types
- [ ] `tests/test_mouse_handler.cpp` - Test program
- [ ] CMakeLists.txt updated
- [ ] All tests pass

---

## Files to Create

### src/game/input/cursor_context.h

```cpp
// src/game/input/cursor_context.h
// Cursor context types for mouse interaction
// Task 16d - Mouse Handler

#ifndef CURSOR_CONTEXT_H
#define CURSOR_CONTEXT_H

#include <cstdint>

//=============================================================================
// Screen Region
//=============================================================================

enum class ScreenRegion {
    TACTICAL,       // Main game view
    SIDEBAR,        // Right sidebar
    RADAR,          // Radar minimap
    TAB_BAR,        // Top tab bar
    CAMEO_AREA,     // Build cameos in sidebar
    POWER_BAR,      // Power indicator
    CREDITS_AREA,   // Credits display
    OUTSIDE         // Outside game window
};

//=============================================================================
// Cursor Context Type
//=============================================================================

enum class CursorContextType {
    // Basic states
    NORMAL,             // Default arrow cursor

    // Over terrain
    TERRAIN_PASSABLE,   // Can move here
    TERRAIN_BLOCKED,    // Cannot move here
    TERRAIN_WATER,      // Water terrain
    TERRAIN_SHROUD,     // Shrouded area

    // Over own units/buildings
    OWN_UNIT,           // Friendly unit (selectable)
    OWN_BUILDING,       // Friendly building
    OWN_HARVESTER,      // Harvester (special actions)
    OWN_TRANSPORT,      // Transport unit

    // Over enemy units/buildings
    ENEMY_UNIT,         // Enemy unit (attackable)
    ENEMY_BUILDING,     // Enemy building

    // Over neutral/civilian
    NEUTRAL_BUILDING,   // Neutral structure
    CIVILIAN,           // Civilian unit

    // Special contexts
    REPAIR_TARGET,      // Valid repair target
    SELL_TARGET,        // Valid sell target
    ENTER_TARGET,       // Can enter this
    HARVEST_AREA,       // Ore/gems to harvest

    // UI contexts
    UI_BUTTON,          // Over UI button
    UI_CAMEO,           // Over build cameo
    UI_RADAR,           // Over radar

    // Action contexts
    PLACEMENT_VALID,    // Valid building placement
    PLACEMENT_INVALID,  // Invalid placement

    // Edge scroll
    SCROLL_N,
    SCROLL_NE,
    SCROLL_E,
    SCROLL_SE,
    SCROLL_S,
    SCROLL_SW,
    SCROLL_W,
    SCROLL_NW
};

//=============================================================================
// Cursor Context
//=============================================================================

struct CursorContext {
    CursorContextType type;
    ScreenRegion region;

    // World position
    int world_x;
    int world_y;
    int cell_x;
    int cell_y;

    // Object under cursor (if any)
    void* object;           // ObjectClass* when integrated
    uint32_t object_id;     // For reference
    bool is_own;            // Belongs to player
    bool is_enemy;          // Belongs to enemy
    bool is_selectable;     // Can be selected
    bool is_attackable;     // Can be attacked

    // Terrain info
    bool is_passable;
    bool is_buildable;
    bool is_visible;        // Not shrouded

    // UI element (if any)
    int ui_element_id;

    void Clear();
};

//=============================================================================
// Cursor Shape (for rendering)
//=============================================================================

enum class CursorShape {
    ARROW,          // Default pointer
    SELECT,         // Selection hand
    MOVE,           // Move order
    ATTACK,         // Attack target
    NO_MOVE,        // Can't move there
    NO_ATTACK,      // Can't attack
    ENTER,          // Enter transport/building
    DEPLOY,         // Deploy action
    SELL,           // Sell mode
    REPAIR,         // Repair mode
    NO_SELL,        // Can't sell here
    NO_REPAIR,      // Can't repair this
    HARVEST,        // Harvest ore
    SCROLL_N,       // Edge scroll cursors
    SCROLL_NE,
    SCROLL_E,
    SCROLL_SE,
    SCROLL_S,
    SCROLL_SW,
    SCROLL_W,
    SCROLL_NW,
    BEACON,         // Place beacon
    AIR_STRIKE,     // Air strike target
    NUKE,           // Nuke target
    CHRONO,         // Chronosphere target
    CUSTOM          // Custom cursor
};

// Map cursor context to appropriate cursor shape
CursorShape GetCursorShapeForContext(const CursorContext& context, bool has_selection);

#endif // CURSOR_CONTEXT_H
```

### src/game/input/mouse_handler.h

```cpp
// src/game/input/mouse_handler.h
// Mouse input handler with coordinate conversion
// Task 16d - Mouse Handler

#ifndef MOUSE_HANDLER_H
#define MOUSE_HANDLER_H

#include "cursor_context.h"
#include "input_defs.h"
#include <cstdint>
#include <functional>

//=============================================================================
// Drag State
//=============================================================================

struct DragState {
    bool active;
    bool started;           // Threshold exceeded
    int start_screen_x;
    int start_screen_y;
    int start_world_x;
    int start_world_y;
    int current_screen_x;
    int current_screen_y;
    int current_world_x;
    int current_world_y;
    int button;             // Which button initiated drag

    void Clear();
    void Begin(int sx, int sy, int wx, int wy, int btn);
    void Update(int sx, int sy, int wx, int wy);
    void End();

    // Get normalized rect (min to max)
    void GetScreenRect(int& x1, int& y1, int& x2, int& y2) const;
    void GetWorldRect(int& x1, int& y1, int& x2, int& y2) const;

    int GetWidth() const;
    int GetHeight() const;
};

//=============================================================================
// Mouse Handler
//=============================================================================

class MouseHandler {
public:
    // Singleton access
    static MouseHandler& Instance();

    // Lifecycle
    bool Initialize();
    void Shutdown();

    // Per-frame update (call after Input_Update)
    void ProcessFrame();

    //=========================================================================
    // Position Queries
    //=========================================================================

    // Screen coordinates (640x400 space)
    int GetScreenX() const { return screen_x_; }
    int GetScreenY() const { return screen_y_; }

    // World coordinates (map space)
    int GetWorldX() const { return world_x_; }
    int GetWorldY() const { return world_y_; }

    // Cell coordinates (tile grid)
    int GetCellX() const { return cell_x_; }
    int GetCellY() const { return cell_y_; }

    // Check if position is in tactical area
    bool IsInTacticalArea() const;

    // Get current screen region
    ScreenRegion GetScreenRegion() const { return current_region_; }

    //=========================================================================
    // Click State
    //=========================================================================

    bool WasLeftClicked() const;
    bool WasRightClicked() const;
    bool WasMiddleClicked() const;
    bool WasDoubleClicked() const;

    bool IsLeftDown() const;
    bool IsRightDown() const;

    //=========================================================================
    // Drag State
    //=========================================================================

    const DragState& GetDrag() const { return drag_; }
    bool IsDragging() const { return drag_.started; }
    bool WasDragCompleted() const { return drag_completed_; }

    //=========================================================================
    // Context
    //=========================================================================

    const CursorContext& GetContext() const { return context_; }
    CursorShape GetCurrentCursorShape() const { return cursor_shape_; }

    //=========================================================================
    // Configuration
    //=========================================================================

    void SetWindowScale(int scale) { window_scale_ = scale; }
    void SetEdgeScrollEnabled(bool enabled) { edge_scroll_enabled_ = enabled; }
    bool IsEdgeScrollEnabled() const { return edge_scroll_enabled_; }

    //=========================================================================
    // Game State Hooks
    //=========================================================================

    // Callbacks for context detection
    using ObjectAtPosFunc = std::function<void*(int cell_x, int cell_y)>;
    using TerrainQueryFunc = std::function<bool(int cell_x, int cell_y)>;
    using SelectionQueryFunc = std::function<bool()>;

    void SetObjectAtPosQuery(ObjectAtPosFunc func) { get_object_at_ = func; }
    void SetTerrainPassableQuery(TerrainQueryFunc func) { is_passable_ = func; }
    void SetTerrainVisibleQuery(TerrainQueryFunc func) { is_visible_ = func; }
    void SetHasSelectionQuery(SelectionQueryFunc func) { has_selection_ = func; }

    //=========================================================================
    // Special Modes
    //=========================================================================

    void SetPlacementMode(bool enabled, int building_type = 0);
    bool IsInPlacementMode() const { return placement_mode_; }

    void SetSellMode(bool enabled);
    bool IsInSellMode() const { return sell_mode_; }

    void SetRepairMode(bool enabled);
    bool IsInRepairMode() const { return repair_mode_; }

private:
    MouseHandler();
    ~MouseHandler();
    MouseHandler(const MouseHandler&) = delete;
    MouseHandler& operator=(const MouseHandler&) = delete;

    void UpdatePosition();
    void UpdateRegion();
    void UpdateDrag();
    void UpdateContext();
    void UpdateCursorShape();

    CursorContextType DetermineContextType();
    bool IsAtScreenEdge(int& dx, int& dy) const;

    bool initialized_;
    int window_scale_;
    bool edge_scroll_enabled_;

    // Position in various coordinate spaces
    int raw_x_, raw_y_;         // Window coordinates
    int screen_x_, screen_y_;   // Screen coordinates
    int world_x_, world_y_;     // World coordinates
    int cell_x_, cell_y_;       // Cell coordinates

    // Region
    ScreenRegion current_region_;

    // Drag
    DragState drag_;
    bool drag_completed_;   // True for one frame after drag ends

    // Context
    CursorContext context_;
    CursorShape cursor_shape_;

    // Special modes
    bool placement_mode_;
    int placement_building_type_;
    bool sell_mode_;
    bool repair_mode_;

    // Callbacks
    ObjectAtPosFunc get_object_at_;
    TerrainQueryFunc is_passable_;
    TerrainQueryFunc is_visible_;
    SelectionQueryFunc has_selection_;
};

//=============================================================================
// Global Access
//=============================================================================

bool MouseHandler_Init();
void MouseHandler_Shutdown();
void MouseHandler_ProcessFrame();

int MouseHandler_GetScreenX();
int MouseHandler_GetScreenY();
int MouseHandler_GetWorldX();
int MouseHandler_GetWorldY();
int MouseHandler_GetCellX();
int MouseHandler_GetCellY();

bool MouseHandler_WasLeftClicked();
bool MouseHandler_WasRightClicked();
bool MouseHandler_IsDragging();

#endif // MOUSE_HANDLER_H
```

### src/game/input/mouse_handler.cpp

```cpp
// src/game/input/mouse_handler.cpp
// Mouse input handler implementation
// Task 16d - Mouse Handler

#include "mouse_handler.h"
#include "input_state.h"
#include "viewport.h"
#include "platform.h"
#include <algorithm>
#include <cstring>

// Screen layout constants
static const int TAB_HEIGHT = 16;
static const int SIDEBAR_WIDTH = 160;
static const int SCREEN_WIDTH = 640;
static const int SCREEN_HEIGHT = 400;
static const int TACTICAL_WIDTH = 480;
static const int TACTICAL_HEIGHT = 384;

// Radar position in sidebar
static const int RADAR_X = 496;
static const int RADAR_Y = 16;
static const int RADAR_SIZE = 128;

// Edge scroll zone
static const int EDGE_ZONE = 8;

//=============================================================================
// DragState Implementation
//=============================================================================

void DragState::Clear() {
    active = false;
    started = false;
    start_screen_x = start_screen_y = 0;
    start_world_x = start_world_y = 0;
    current_screen_x = current_screen_y = 0;
    current_world_x = current_world_y = 0;
    button = -1;
}

void DragState::Begin(int sx, int sy, int wx, int wy, int btn) {
    active = true;
    started = false;
    start_screen_x = current_screen_x = sx;
    start_screen_y = current_screen_y = sy;
    start_world_x = current_world_x = wx;
    start_world_y = current_world_y = wy;
    button = btn;
}

void DragState::Update(int sx, int sy, int wx, int wy) {
    current_screen_x = sx;
    current_screen_y = sy;
    current_world_x = wx;
    current_world_y = wy;

    // Check threshold
    if (!started) {
        int dx = current_screen_x - start_screen_x;
        int dy = current_screen_y - start_screen_y;
        if (dx * dx + dy * dy >= DRAG_THRESHOLD_PIXELS * DRAG_THRESHOLD_PIXELS) {
            started = true;
        }
    }
}

void DragState::End() {
    active = false;
}

void DragState::GetScreenRect(int& x1, int& y1, int& x2, int& y2) const {
    x1 = std::min(start_screen_x, current_screen_x);
    y1 = std::min(start_screen_y, current_screen_y);
    x2 = std::max(start_screen_x, current_screen_x);
    y2 = std::max(start_screen_y, current_screen_y);
}

void DragState::GetWorldRect(int& x1, int& y1, int& x2, int& y2) const {
    x1 = std::min(start_world_x, current_world_x);
    y1 = std::min(start_world_y, current_world_y);
    x2 = std::max(start_world_x, current_world_x);
    y2 = std::max(start_world_y, current_world_y);
}

int DragState::GetWidth() const {
    return std::abs(current_screen_x - start_screen_x);
}

int DragState::GetHeight() const {
    return std::abs(current_screen_y - start_screen_y);
}

//=============================================================================
// CursorContext Implementation
//=============================================================================

void CursorContext::Clear() {
    type = CursorContextType::NORMAL;
    region = ScreenRegion::OUTSIDE;
    world_x = world_y = 0;
    cell_x = cell_y = 0;
    object = nullptr;
    object_id = 0;
    is_own = is_enemy = false;
    is_selectable = is_attackable = false;
    is_passable = is_buildable = is_visible = false;
    ui_element_id = -1;
}

//=============================================================================
// GetCursorShapeForContext
//=============================================================================

CursorShape GetCursorShapeForContext(const CursorContext& context, bool has_selection) {
    // Edge scroll cursors take priority
    if (context.type >= CursorContextType::SCROLL_N &&
        context.type <= CursorContextType::SCROLL_NW) {
        return static_cast<CursorShape>(
            static_cast<int>(CursorShape::SCROLL_N) +
            (static_cast<int>(context.type) - static_cast<int>(CursorContextType::SCROLL_N)));
    }

    // UI regions
    if (context.region == ScreenRegion::SIDEBAR ||
        context.region == ScreenRegion::TAB_BAR) {
        return CursorShape::ARROW;
    }

    // Special modes
    if (context.type == CursorContextType::PLACEMENT_VALID) {
        return CursorShape::SELECT;
    }
    if (context.type == CursorContextType::PLACEMENT_INVALID) {
        return CursorShape::NO_MOVE;
    }
    if (context.type == CursorContextType::SELL_TARGET) {
        return CursorShape::SELL;
    }
    if (context.type == CursorContextType::REPAIR_TARGET) {
        return CursorShape::REPAIR;
    }

    // With selection
    if (has_selection) {
        if (context.is_enemy && context.is_attackable) {
            return CursorShape::ATTACK;
        }
        if (context.is_own && context.type == CursorContextType::OWN_TRANSPORT) {
            return CursorShape::ENTER;
        }
        if (context.is_passable) {
            return CursorShape::MOVE;
        }
        return CursorShape::NO_MOVE;
    }

    // Without selection
    if (context.is_own && context.is_selectable) {
        return CursorShape::SELECT;
    }

    return CursorShape::ARROW;
}

//=============================================================================
// MouseHandler Singleton
//=============================================================================

MouseHandler& MouseHandler::Instance() {
    static MouseHandler instance;
    return instance;
}

MouseHandler::MouseHandler()
    : initialized_(false)
    , window_scale_(2)
    , edge_scroll_enabled_(true)
    , raw_x_(0), raw_y_(0)
    , screen_x_(0), screen_y_(0)
    , world_x_(0), world_y_(0)
    , cell_x_(0), cell_y_(0)
    , current_region_(ScreenRegion::OUTSIDE)
    , drag_completed_(false)
    , cursor_shape_(CursorShape::ARROW)
    , placement_mode_(false)
    , placement_building_type_(0)
    , sell_mode_(false)
    , repair_mode_(false)
    , get_object_at_(nullptr)
    , is_passable_(nullptr)
    , is_visible_(nullptr)
    , has_selection_(nullptr) {
    drag_.Clear();
    context_.Clear();
}

MouseHandler::~MouseHandler() {
    Shutdown();
}

bool MouseHandler::Initialize() {
    if (initialized_) return true;

    Platform_Log("MouseHandler: Initializing...");

    drag_.Clear();
    context_.Clear();
    cursor_shape_ = CursorShape::ARROW;

    initialized_ = true;
    Platform_Log("MouseHandler: Initialized");
    return true;
}

void MouseHandler::Shutdown() {
    if (!initialized_) return;

    Platform_Log("MouseHandler: Shutting down");
    initialized_ = false;
}

void MouseHandler::ProcessFrame() {
    if (!initialized_) return;

    // Clear per-frame state
    drag_completed_ = false;

    // Update all state
    UpdatePosition();
    UpdateRegion();
    UpdateDrag();
    UpdateContext();
    UpdateCursorShape();
}

void MouseHandler::UpdatePosition() {
    InputState& input = InputState::Instance();

    // Get raw position
    raw_x_ = input.Mouse().raw_x;
    raw_y_ = input.Mouse().raw_y;

    // Convert to screen coordinates
    screen_x_ = raw_x_ / window_scale_;
    screen_y_ = raw_y_ / window_scale_;

    // Clamp to screen bounds
    if (screen_x_ < 0) screen_x_ = 0;
    if (screen_y_ < 0) screen_y_ = 0;
    if (screen_x_ >= SCREEN_WIDTH) screen_x_ = SCREEN_WIDTH - 1;
    if (screen_y_ >= SCREEN_HEIGHT) screen_y_ = SCREEN_HEIGHT - 1;

    // Convert to world coordinates (only in tactical area)
    if (IsInTacticalArea()) {
        Viewport& vp = Viewport::Instance();
        world_x_ = screen_x_ + vp.x;
        world_y_ = (screen_y_ - TAB_HEIGHT) + vp.y;

        cell_x_ = world_x_ / TILE_PIXEL_WIDTH;
        cell_y_ = world_y_ / TILE_PIXEL_HEIGHT;

        // Clamp to map bounds
        if (cell_x_ < 0) cell_x_ = 0;
        if (cell_y_ < 0) cell_y_ = 0;
        if (cell_x_ >= vp.GetMapCellWidth()) {
            cell_x_ = vp.GetMapCellWidth() - 1;
        }
        if (cell_y_ >= vp.GetMapCellHeight()) {
            cell_y_ = vp.GetMapCellHeight() - 1;
        }
    }
}

void MouseHandler::UpdateRegion() {
    // Determine which screen region the mouse is in
    if (screen_y_ < TAB_HEIGHT) {
        current_region_ = ScreenRegion::TAB_BAR;
    } else if (screen_x_ >= TACTICAL_WIDTH) {
        // In sidebar area - determine specific region
        if (screen_x_ >= RADAR_X && screen_x_ < RADAR_X + RADAR_SIZE &&
            screen_y_ >= RADAR_Y && screen_y_ < RADAR_Y + RADAR_SIZE) {
            current_region_ = ScreenRegion::RADAR;
        } else {
            current_region_ = ScreenRegion::SIDEBAR;
        }
    } else {
        current_region_ = ScreenRegion::TACTICAL;
    }
}

void MouseHandler::UpdateDrag() {
    InputState& input = InputState::Instance();

    // Check for drag start
    if (input.WasMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (current_region_ == ScreenRegion::TACTICAL) {
            drag_.Begin(screen_x_, screen_y_, world_x_, world_y_, MOUSE_BUTTON_LEFT);
        }
    }

    // Update drag if active
    if (drag_.active) {
        if (input.IsMouseButtonDown(drag_.button)) {
            drag_.Update(screen_x_, screen_y_, world_x_, world_y_);
        } else {
            // Button released - end drag
            if (drag_.started) {
                drag_completed_ = true;
            }
            drag_.End();
        }
    }
}

void MouseHandler::UpdateContext() {
    context_.Clear();
    context_.region = current_region_;
    context_.world_x = world_x_;
    context_.world_y = world_y_;
    context_.cell_x = cell_x_;
    context_.cell_y = cell_y_;

    // Check for edge scroll cursor
    if (edge_scroll_enabled_ && current_region_ == ScreenRegion::TACTICAL) {
        int dx = 0, dy = 0;
        if (IsAtScreenEdge(dx, dy)) {
            // Map to scroll context type
            if (dy < 0 && dx == 0) context_.type = CursorContextType::SCROLL_N;
            else if (dy < 0 && dx > 0) context_.type = CursorContextType::SCROLL_NE;
            else if (dy == 0 && dx > 0) context_.type = CursorContextType::SCROLL_E;
            else if (dy > 0 && dx > 0) context_.type = CursorContextType::SCROLL_SE;
            else if (dy > 0 && dx == 0) context_.type = CursorContextType::SCROLL_S;
            else if (dy > 0 && dx < 0) context_.type = CursorContextType::SCROLL_SW;
            else if (dy == 0 && dx < 0) context_.type = CursorContextType::SCROLL_W;
            else if (dy < 0 && dx < 0) context_.type = CursorContextType::SCROLL_NW;
            return;
        }
    }

    // Special modes
    if (placement_mode_) {
        // TODO: Check if placement is valid using terrain query
        context_.is_buildable = true;  // Placeholder
        context_.type = context_.is_buildable ?
            CursorContextType::PLACEMENT_VALID : CursorContextType::PLACEMENT_INVALID;
        return;
    }

    if (sell_mode_) {
        // TODO: Check if can sell
        context_.type = CursorContextType::SELL_TARGET;
        return;
    }

    if (repair_mode_) {
        // TODO: Check if can repair
        context_.type = CursorContextType::REPAIR_TARGET;
        return;
    }

    // Query terrain
    if (is_passable_) {
        context_.is_passable = is_passable_(cell_x_, cell_y_);
    }
    if (is_visible_) {
        context_.is_visible = is_visible_(cell_x_, cell_y_);
    }

    // Query object at position
    if (get_object_at_) {
        context_.object = get_object_at_(cell_x_, cell_y_);
        if (context_.object) {
            // TODO: Fill in object details when integrated with game code
            context_.is_selectable = true;
        }
    }

    // Determine final context type
    context_.type = DetermineContextType();
}

CursorContextType MouseHandler::DetermineContextType() {
    // Priority: object > terrain > default

    if (context_.object) {
        if (context_.is_own) {
            return CursorContextType::OWN_UNIT;
        } else if (context_.is_enemy) {
            return CursorContextType::ENEMY_UNIT;
        }
    }

    if (!context_.is_visible) {
        return CursorContextType::TERRAIN_SHROUD;
    }

    if (context_.is_passable) {
        return CursorContextType::TERRAIN_PASSABLE;
    }

    return CursorContextType::TERRAIN_BLOCKED;
}

void MouseHandler::UpdateCursorShape() {
    bool has_sel = has_selection_ ? has_selection_() : false;
    cursor_shape_ = GetCursorShapeForContext(context_, has_sel);
}

bool MouseHandler::IsAtScreenEdge(int& dx, int& dy) const {
    dx = dy = 0;

    // Only in tactical area
    if (current_region_ != ScreenRegion::TACTICAL) {
        return false;
    }

    // Check edges
    if (screen_x_ < EDGE_ZONE) dx = -1;
    else if (screen_x_ >= TACTICAL_WIDTH - EDGE_ZONE) dx = 1;

    if (screen_y_ < TAB_HEIGHT + EDGE_ZONE) dy = -1;
    else if (screen_y_ >= SCREEN_HEIGHT - EDGE_ZONE) dy = 1;

    return dx != 0 || dy != 0;
}

bool MouseHandler::IsInTacticalArea() const {
    return current_region_ == ScreenRegion::TACTICAL;
}

bool MouseHandler::WasLeftClicked() const {
    return InputState::Instance().WasMouseButtonPressed(MOUSE_BUTTON_LEFT) && !drag_.started;
}

bool MouseHandler::WasRightClicked() const {
    return InputState::Instance().WasMouseButtonPressed(MOUSE_BUTTON_RIGHT);
}

bool MouseHandler::WasMiddleClicked() const {
    return InputState::Instance().WasMouseButtonPressed(MOUSE_BUTTON_MIDDLE);
}

bool MouseHandler::WasDoubleClicked() const {
    return InputState::Instance().WasMouseDoubleClicked(MOUSE_BUTTON_LEFT);
}

bool MouseHandler::IsLeftDown() const {
    return InputState::Instance().IsMouseButtonDown(MOUSE_BUTTON_LEFT);
}

bool MouseHandler::IsRightDown() const {
    return InputState::Instance().IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
}

void MouseHandler::SetPlacementMode(bool enabled, int building_type) {
    placement_mode_ = enabled;
    placement_building_type_ = building_type;
    if (enabled) {
        sell_mode_ = false;
        repair_mode_ = false;
    }
}

void MouseHandler::SetSellMode(bool enabled) {
    sell_mode_ = enabled;
    if (enabled) {
        placement_mode_ = false;
        repair_mode_ = false;
    }
}

void MouseHandler::SetRepairMode(bool enabled) {
    repair_mode_ = enabled;
    if (enabled) {
        placement_mode_ = false;
        sell_mode_ = false;
    }
}

//=============================================================================
// Global Functions
//=============================================================================

bool MouseHandler_Init() {
    return MouseHandler::Instance().Initialize();
}

void MouseHandler_Shutdown() {
    MouseHandler::Instance().Shutdown();
}

void MouseHandler_ProcessFrame() {
    MouseHandler::Instance().ProcessFrame();
}

int MouseHandler_GetScreenX() {
    return MouseHandler::Instance().GetScreenX();
}

int MouseHandler_GetScreenY() {
    return MouseHandler::Instance().GetScreenY();
}

int MouseHandler_GetWorldX() {
    return MouseHandler::Instance().GetWorldX();
}

int MouseHandler_GetWorldY() {
    return MouseHandler::Instance().GetWorldY();
}

int MouseHandler_GetCellX() {
    return MouseHandler::Instance().GetCellX();
}

int MouseHandler_GetCellY() {
    return MouseHandler::Instance().GetCellY();
}

bool MouseHandler_WasLeftClicked() {
    return MouseHandler::Instance().WasLeftClicked();
}

bool MouseHandler_WasRightClicked() {
    return MouseHandler::Instance().WasRightClicked();
}

bool MouseHandler_IsDragging() {
    return MouseHandler::Instance().IsDragging();
}
```

### tests/test_mouse_handler.cpp

```cpp
// tests/test_mouse_handler.cpp
// Test program for mouse handler
// Task 16d - Mouse Handler

#include "mouse_handler.h"
#include "input_state.h"
#include "viewport.h"
#include "platform.h"
#include <cstdio>
#include <cstring>

bool Test_DragState() {
    printf("Test: DragState... ");

    DragState drag;
    drag.Clear();

    if (drag.active || drag.started) {
        printf("FAILED - Not cleared\n");
        return false;
    }

    drag.Begin(100, 100, 200, 200, 0);
    if (!drag.active || drag.started) {
        printf("FAILED - Begin state wrong\n");
        return false;
    }

    // Move within threshold
    drag.Update(102, 102, 202, 202);
    if (drag.started) {
        printf("FAILED - Started too early\n");
        return false;
    }

    // Move past threshold
    drag.Update(110, 110, 210, 210);
    if (!drag.started) {
        printf("FAILED - Should have started\n");
        return false;
    }

    // Get rect
    int x1, y1, x2, y2;
    drag.GetScreenRect(x1, y1, x2, y2);
    if (x1 != 100 || y1 != 100 || x2 != 110 || y2 != 110) {
        printf("FAILED - Rect wrong\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_CursorContext() {
    printf("Test: CursorContext... ");

    CursorContext ctx;
    ctx.Clear();

    if (ctx.type != CursorContextType::NORMAL) {
        printf("FAILED - Default type wrong\n");
        return false;
    }

    if (ctx.object != nullptr) {
        printf("FAILED - Object not null\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_MouseHandlerInit() {
    printf("Test: MouseHandler Init... ");

    Platform_Init();
    Input_Init();
    Viewport::Instance().Initialize();
    Viewport::Instance().SetMapSize(64, 64);

    if (!MouseHandler_Init()) {
        printf("FAILED - Init failed\n");
        Input_Shutdown();
        Platform_Shutdown();
        return false;
    }

    MouseHandler_ProcessFrame();

    // Just verify no crash
    int sx = MouseHandler_GetScreenX();
    int sy = MouseHandler_GetScreenY();

    MouseHandler_Shutdown();
    Input_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_CursorShapeMapping() {
    printf("Test: Cursor Shape Mapping... ");

    CursorContext ctx;
    ctx.Clear();

    // Normal context, no selection
    ctx.type = CursorContextType::NORMAL;
    CursorShape shape = GetCursorShapeForContext(ctx, false);
    if (shape != CursorShape::ARROW) {
        printf("FAILED - Normal should be ARROW\n");
        return false;
    }

    // Enemy with selection
    ctx.type = CursorContextType::ENEMY_UNIT;
    ctx.is_enemy = true;
    ctx.is_attackable = true;
    shape = GetCursorShapeForContext(ctx, true);
    if (shape != CursorShape::ATTACK) {
        printf("FAILED - Enemy should be ATTACK\n");
        return false;
    }

    // Passable terrain with selection
    ctx.Clear();
    ctx.type = CursorContextType::TERRAIN_PASSABLE;
    ctx.is_passable = true;
    shape = GetCursorShapeForContext(ctx, true);
    if (shape != CursorShape::MOVE) {
        printf("FAILED - Passable should be MOVE\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

void Interactive_Test() {
    printf("\n=== Interactive Mouse Handler Test ===\n");
    printf("Move mouse around to see coordinates and context\n");
    printf("Left-drag to test drag selection\n");
    printf("Press P for placement mode, S for sell mode, R for repair mode\n");
    printf("ESC to exit\n\n");

    Platform_Init();
    Platform_Graphics_Init();
    Input_Init();
    Viewport::Instance().Initialize();
    Viewport::Instance().SetMapSize(128, 128);
    MouseHandler_Init();

    MouseHandler& mouse = MouseHandler::Instance();

    bool running = true;
    while (running) {
        Platform_PumpEvents();
        Input_Update();
        MouseHandler_ProcessFrame();

        if (Input_KeyPressed(KEY_ESCAPE)) {
            running = false;
            continue;
        }

        // Mode toggles
        if (Input_KeyPressed(KEY_P)) {
            mouse.SetPlacementMode(!mouse.IsInPlacementMode());
            printf("Placement mode: %s\n", mouse.IsInPlacementMode() ? "ON" : "OFF");
        }
        if (Input_KeyPressed(KEY_S)) {
            mouse.SetSellMode(!mouse.IsInSellMode());
            printf("Sell mode: %s\n", mouse.IsInSellMode() ? "ON" : "OFF");
        }
        if (Input_KeyPressed(KEY_R)) {
            mouse.SetRepairMode(!mouse.IsInRepairMode());
            printf("Repair mode: %s\n", mouse.IsInRepairMode() ? "ON" : "OFF");
        }

        // Report position changes
        static int last_cx = -1, last_cy = -1;
        int cx = mouse.GetCellX();
        int cy = mouse.GetCellY();
        if (cx != last_cx || cy != last_cy) {
            printf("Screen(%d,%d) World(%d,%d) Cell(%d,%d) Region:%d Cursor:%d\n",
                   mouse.GetScreenX(), mouse.GetScreenY(),
                   mouse.GetWorldX(), mouse.GetWorldY(),
                   cx, cy,
                   static_cast<int>(mouse.GetScreenRegion()),
                   static_cast<int>(mouse.GetCurrentCursorShape()));
            last_cx = cx;
            last_cy = cy;
        }

        // Report clicks
        if (MouseHandler_WasLeftClicked()) {
            printf("LEFT CLICK at cell (%d, %d)\n", cx, cy);
        }
        if (MouseHandler_WasRightClicked()) {
            printf("RIGHT CLICK at cell (%d, %d)\n", cx, cy);
        }

        // Report drag
        if (mouse.WasDragCompleted()) {
            int x1, y1, x2, y2;
            mouse.GetDrag().GetScreenRect(x1, y1, x2, y2);
            printf("DRAG completed: (%d,%d) to (%d,%d)\n", x1, y1, x2, y2);
        }

        Platform_Timer_Delay(16);
    }

    MouseHandler_Shutdown();
    Input_Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

int main(int argc, char* argv[]) {
    printf("=== Mouse Handler Tests (Task 16d) ===\n\n");

    int passed = 0;
    int failed = 0;

    if (Test_DragState()) passed++; else failed++;
    if (Test_CursorContext()) passed++; else failed++;
    if (Test_MouseHandlerInit()) passed++; else failed++;
    if (Test_CursorShapeMapping()) passed++; else failed++;

    printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);

    if (argc > 1 && strcmp(argv[1], "-i") == 0) {
        Interactive_Test();
    } else {
        printf("\nRun with -i for interactive test\n");
    }

    return (failed == 0) ? 0 : 1;
}
```

---

## CMakeLists.txt Additions

Add to `src/game/CMakeLists.txt`:

```cmake
# Mouse Handler (Task 16d)
target_sources(game PRIVATE
    input/cursor_context.h
    input/mouse_handler.h
    input/mouse_handler.cpp
)
```

---

## Verification Script

```bash
#!/bin/bash
set -e

echo "=== Task 16d Verification ==="

cd build
cmake --build . --target test_mouse_handler

echo "Running tests..."
./tests/test_mouse_handler

echo "Checking files..."
for file in \
    "../src/game/input/cursor_context.h" \
    "../src/game/input/mouse_handler.h" \
    "../src/game/input/mouse_handler.cpp"
do
    if [ ! -f "$file" ]; then
        echo "ERROR: Missing $file"
        exit 1
    fi
done

echo ""
echo "=== Task 16d Verification PASSED ==="
```

---

## Implementation Steps

1. **Create cursor_context.h** - Define context types and cursor shapes
2. **Create mouse_handler.h** - Handler class declaration
3. **Create mouse_handler.cpp** - Full implementation
4. **Create test_mouse_handler.cpp** - Tests and interactive demo
5. **Update CMakeLists.txt** - Add files
6. **Build and test** - Verify all pass

---

## Success Criteria

- [ ] Coordinate conversion works correctly
- [ ] Screen region detection accurate
- [ ] Drag state tracks properly with threshold
- [ ] Cursor context updates based on position
- [ ] Cursor shape maps correctly to context
- [ ] Special modes (placement/sell/repair) work
- [ ] All unit tests pass
- [ ] Interactive test shows correct feedback

---

## Completion Promise

When verification passes, output:
<promise>TASK_16D_COMPLETE</promise>

## Escape Hatch

If stuck after 15 iterations, output:
<promise>TASK_16D_BLOCKED</promise>

## Max Iterations
15
