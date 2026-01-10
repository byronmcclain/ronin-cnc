// include/game/input/mouse_handler.h
// Mouse input handler with coordinate conversion
// Task 16d - Mouse Handler

#ifndef MOUSE_HANDLER_H
#define MOUSE_HANDLER_H

#include "cursor_context.h"
#include "input_defs.h"
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
