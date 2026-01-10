// include/game/input/scroll_processor.h
// Edge and keyboard scroll processing
// Task 16g - Scrolling

#ifndef SCROLL_PROCESSOR_H
#define SCROLL_PROCESSOR_H

#include <cstdint>
#include <functional>

//=============================================================================
// Scroll Constants
//=============================================================================

static const int SCROLL_ZONE_SIZE = 8;          // Pixels from edge to trigger
static const int SCROLL_SPEED_NORMAL = 2;       // Pixels per frame at 60 FPS
static const int SCROLL_SPEED_FAST = 6;         // With Shift held

//=============================================================================
// Scroll Direction Flags (compatible with viewport.h)
//=============================================================================

enum ScrollDir : uint8_t {
    SCROLLDIR_NONE  = 0x00,
    SCROLLDIR_N     = 0x01,
    SCROLLDIR_S     = 0x02,
    SCROLLDIR_W     = 0x04,
    SCROLLDIR_E     = 0x08,
    SCROLLDIR_NE    = SCROLLDIR_N | SCROLLDIR_E,
    SCROLLDIR_SE    = SCROLLDIR_S | SCROLLDIR_E,
    SCROLLDIR_NW    = SCROLLDIR_N | SCROLLDIR_W,
    SCROLLDIR_SW    = SCROLLDIR_S | SCROLLDIR_W,
};

//=============================================================================
// Scroll Processor
//=============================================================================

class ScrollProcessor {
public:
    // Singleton access
    static ScrollProcessor& Instance();

    // Lifecycle
    bool Initialize();
    void Shutdown();

    // Per-frame processing
    void ProcessFrame();

    //=========================================================================
    // Edge Scrolling
    //=========================================================================

    void SetEdgeScrollEnabled(bool enabled) { edge_scroll_enabled_ = enabled; }
    bool IsEdgeScrollEnabled() const { return edge_scroll_enabled_; }

    // Get current edge scroll direction (for cursor display)
    ScrollDir GetEdgeScrollDirection() const { return edge_direction_; }

    //=========================================================================
    // Keyboard Scrolling
    //=========================================================================

    void SetKeyboardScrollEnabled(bool enabled) { keyboard_scroll_enabled_ = enabled; }
    bool IsKeyboardScrollEnabled() const { return keyboard_scroll_enabled_; }

    //=========================================================================
    // Scroll State
    //=========================================================================

    // Combined scroll direction (edge + keyboard)
    ScrollDir GetScrollDirection() const { return combined_direction_; }

    // Is scrolling active?
    bool IsScrolling() const { return combined_direction_ != SCROLLDIR_NONE; }

    // Get actual scroll delta for this frame
    int GetScrollDeltaX() const { return scroll_delta_x_; }
    int GetScrollDeltaY() const { return scroll_delta_y_; }

    //=========================================================================
    // Configuration
    //=========================================================================

    void SetScrollSpeed(int normal, int fast);
    void SetEdgeZoneSize(int pixels);

    //=========================================================================
    // Viewport Integration
    //=========================================================================

    // Callback to apply scroll to viewport
    using ApplyScrollFunc = std::function<void(int dx, int dy)>;
    void SetApplyScrollCallback(ApplyScrollFunc func) { apply_scroll_ = func; }

    // Callback to check if scroll is valid
    using CanScrollFunc = std::function<bool(int dx, int dy)>;
    void SetCanScrollCallback(CanScrollFunc func) { can_scroll_ = func; }

private:
    ScrollProcessor();
    ~ScrollProcessor();
    ScrollProcessor(const ScrollProcessor&) = delete;
    ScrollProcessor& operator=(const ScrollProcessor&) = delete;

    ScrollDir DetectEdgeScroll();
    ScrollDir DetectKeyboardScroll();
    void CalculateScrollDelta(ScrollDir dir, bool fast);
    void ApplyScroll();

    bool initialized_;

    // Enable flags
    bool edge_scroll_enabled_;
    bool keyboard_scroll_enabled_;

    // Speed settings
    int scroll_speed_normal_;
    int scroll_speed_fast_;
    int edge_zone_size_;

    // Current scroll state
    ScrollDir edge_direction_;
    ScrollDir keyboard_direction_;
    ScrollDir combined_direction_;
    int scroll_delta_x_;
    int scroll_delta_y_;

    // Callbacks
    ApplyScrollFunc apply_scroll_;
    CanScrollFunc can_scroll_;
};

//=============================================================================
// Global Functions
//=============================================================================

bool ScrollProcessor_Init();
void ScrollProcessor_Shutdown();
void ScrollProcessor_ProcessFrame();

bool ScrollProcessor_IsScrolling();
int ScrollProcessor_GetDeltaX();
int ScrollProcessor_GetDeltaY();

void ScrollProcessor_SetEdgeEnabled(bool enabled);
void ScrollProcessor_SetKeyboardEnabled(bool enabled);

#endif // SCROLL_PROCESSOR_H
