// src/game/input/mouse_handler.cpp
// Mouse input handler implementation
// Task 16d - Mouse Handler

#include "game/input/mouse_handler.h"
#include "game/input/input_state.h"
#include "game/viewport.h"
#include "platform.h"
#include <algorithm>
#include <cstring>

// Use viewport constants from viewport.h:
// VP_VP_TAB_HEIGHT, VP_VP_SCREEN_WIDTH, VP_VP_SCREEN_HEIGHT, TACTICAL_WIDTH, TACTICAL_HEIGHT

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

    Platform_LogInfo("MouseHandler: Initializing...");

    drag_.Clear();
    context_.Clear();
    cursor_shape_ = CursorShape::ARROW;

    initialized_ = true;
    Platform_LogInfo("MouseHandler: Initialized");
    return true;
}

void MouseHandler::Shutdown() {
    if (!initialized_) return;

    Platform_LogInfo("MouseHandler: Shutting down");
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
    if (screen_x_ >= VP_SCREEN_WIDTH) screen_x_ = VP_SCREEN_WIDTH - 1;
    if (screen_y_ >= VP_SCREEN_HEIGHT) screen_y_ = VP_SCREEN_HEIGHT - 1;

    // Convert to world coordinates (only in tactical area)
    if (IsInTacticalArea()) {
        GameViewport& vp = GameViewport::Instance();
        world_x_ = screen_x_ + vp.x;
        world_y_ = (screen_y_ - VP_TAB_HEIGHT) + vp.y;

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
    if (screen_y_ < VP_TAB_HEIGHT) {
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
    if (input.WasMouseButtonPressed(INPUT_MOUSE_LEFT)) {
        if (current_region_ == ScreenRegion::TACTICAL) {
            drag_.Begin(screen_x_, screen_y_, world_x_, world_y_, INPUT_MOUSE_LEFT);
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

    if (screen_y_ < VP_TAB_HEIGHT + EDGE_ZONE) dy = -1;
    else if (screen_y_ >= VP_SCREEN_HEIGHT - EDGE_ZONE) dy = 1;

    return dx != 0 || dy != 0;
}

bool MouseHandler::IsInTacticalArea() const {
    return current_region_ == ScreenRegion::TACTICAL;
}

bool MouseHandler::WasLeftClicked() const {
    return InputState::Instance().WasMouseButtonPressed(INPUT_MOUSE_LEFT) && !drag_.started;
}

bool MouseHandler::WasRightClicked() const {
    return InputState::Instance().WasMouseButtonPressed(INPUT_MOUSE_RIGHT);
}

bool MouseHandler::WasMiddleClicked() const {
    return InputState::Instance().WasMouseButtonPressed(INPUT_MOUSE_MIDDLE);
}

bool MouseHandler::WasDoubleClicked() const {
    return InputState::Instance().WasMouseDoubleClicked(INPUT_MOUSE_LEFT);
}

bool MouseHandler::IsLeftDown() const {
    return InputState::Instance().IsMouseButtonDown(INPUT_MOUSE_LEFT);
}

bool MouseHandler::IsRightDown() const {
    return InputState::Instance().IsMouseButtonDown(INPUT_MOUSE_RIGHT);
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
