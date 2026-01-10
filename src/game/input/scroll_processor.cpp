// src/game/input/scroll_processor.cpp
// Edge and keyboard scroll processing implementation
// Task 16g - Scrolling

#include "game/input/scroll_processor.h"
#include "game/input/mouse_handler.h"
#include "game/input/input_mapper.h"
#include "game/input/game_action.h"
#include "game/viewport.h"
#include "platform.h"

//=============================================================================
// ScrollProcessor Singleton
//=============================================================================

ScrollProcessor& ScrollProcessor::Instance() {
    static ScrollProcessor instance;
    return instance;
}

ScrollProcessor::ScrollProcessor()
    : initialized_(false)
    , edge_scroll_enabled_(true)
    , keyboard_scroll_enabled_(true)
    , scroll_speed_normal_(SCROLL_SPEED_NORMAL)
    , scroll_speed_fast_(SCROLL_SPEED_FAST)
    , edge_zone_size_(SCROLL_ZONE_SIZE)
    , edge_direction_(SCROLLDIR_NONE)
    , keyboard_direction_(SCROLLDIR_NONE)
    , combined_direction_(SCROLLDIR_NONE)
    , scroll_delta_x_(0)
    , scroll_delta_y_(0)
    , apply_scroll_(nullptr)
    , can_scroll_(nullptr) {
}

ScrollProcessor::~ScrollProcessor() {
    Shutdown();
}

bool ScrollProcessor::Initialize() {
    if (initialized_) return true;

    Platform_LogInfo("ScrollProcessor: Initializing...");

    edge_direction_ = SCROLLDIR_NONE;
    keyboard_direction_ = SCROLLDIR_NONE;
    combined_direction_ = SCROLLDIR_NONE;
    scroll_delta_x_ = scroll_delta_y_ = 0;

    initialized_ = true;
    Platform_LogInfo("ScrollProcessor: Initialized");
    return true;
}

void ScrollProcessor::Shutdown() {
    if (!initialized_) return;

    Platform_LogInfo("ScrollProcessor: Shutting down");
    initialized_ = false;
}

void ScrollProcessor::ProcessFrame() {
    if (!initialized_) return;

    // Reset per-frame state
    scroll_delta_x_ = 0;
    scroll_delta_y_ = 0;

    // Detect scroll from both sources
    edge_direction_ = edge_scroll_enabled_ ? DetectEdgeScroll() : SCROLLDIR_NONE;
    keyboard_direction_ = keyboard_scroll_enabled_ ? DetectKeyboardScroll() : SCROLLDIR_NONE;

    // Combine directions (OR them together)
    combined_direction_ = static_cast<ScrollDir>(
        static_cast<uint8_t>(edge_direction_) | static_cast<uint8_t>(keyboard_direction_));

    // Calculate and apply scroll if any direction active
    if (combined_direction_ != SCROLLDIR_NONE) {
        // Check if fast scroll (Shift held)
        bool fast = InputMapper::Instance().IsActionActive(GameAction::SCROLL_FAST);

        CalculateScrollDelta(combined_direction_, fast);
        ApplyScroll();
    }
}

//=============================================================================
// Edge Scroll Detection
//=============================================================================

ScrollDir ScrollProcessor::DetectEdgeScroll() {
    MouseHandler& mouse = MouseHandler::Instance();

    // Only scroll when mouse is in tactical area
    if (!mouse.IsInTacticalArea()) {
        return SCROLLDIR_NONE;
    }

    int sx = mouse.GetScreenX();
    int sy = mouse.GetScreenY();

    uint8_t direction = SCROLLDIR_NONE;

    // Check horizontal edges (within tactical area only, x < TACTICAL_WIDTH)
    if (sx < edge_zone_size_) {
        direction |= SCROLLDIR_W;
    } else if (sx >= TACTICAL_WIDTH - edge_zone_size_) {
        direction |= SCROLLDIR_E;
    }

    // Check vertical edges (accounting for tab bar)
    if (sy < VP_TAB_HEIGHT + edge_zone_size_) {
        direction |= SCROLLDIR_N;
    } else if (sy >= VP_SCREEN_HEIGHT - edge_zone_size_) {
        direction |= SCROLLDIR_S;
    }

    return static_cast<ScrollDir>(direction);
}

//=============================================================================
// Keyboard Scroll Detection
//=============================================================================

ScrollDir ScrollProcessor::DetectKeyboardScroll() {
    InputMapper& mapper = InputMapper::Instance();

    uint8_t direction = SCROLLDIR_NONE;

    // Check arrow key actions
    if (mapper.IsActionActive(GameAction::SCROLL_UP)) {
        direction |= SCROLLDIR_N;
    }
    if (mapper.IsActionActive(GameAction::SCROLL_DOWN)) {
        direction |= SCROLLDIR_S;
    }
    if (mapper.IsActionActive(GameAction::SCROLL_LEFT)) {
        direction |= SCROLLDIR_W;
    }
    if (mapper.IsActionActive(GameAction::SCROLL_RIGHT)) {
        direction |= SCROLLDIR_E;
    }

    return static_cast<ScrollDir>(direction);
}

//=============================================================================
// Scroll Calculation
//=============================================================================

void ScrollProcessor::CalculateScrollDelta(ScrollDir dir, bool fast) {
    int speed = fast ? scroll_speed_fast_ : scroll_speed_normal_;

    scroll_delta_x_ = 0;
    scroll_delta_y_ = 0;

    // Horizontal
    if (dir & SCROLLDIR_W) {
        scroll_delta_x_ = -speed;
    } else if (dir & SCROLLDIR_E) {
        scroll_delta_x_ = speed;
    }

    // Vertical
    if (dir & SCROLLDIR_N) {
        scroll_delta_y_ = -speed;
    } else if (dir & SCROLLDIR_S) {
        scroll_delta_y_ = speed;
    }

    // Note: Diagonal movement is faster by sqrt(2) but most RTS games don't normalize
    // Could optionally divide by 1.414 for true diagonal normalization
}

void ScrollProcessor::ApplyScroll() {
    if (scroll_delta_x_ == 0 && scroll_delta_y_ == 0) {
        return;
    }

    // Check if scroll is allowed
    if (can_scroll_ && !can_scroll_(scroll_delta_x_, scroll_delta_y_)) {
        return;
    }

    // Apply via callback or directly to viewport
    if (apply_scroll_) {
        apply_scroll_(scroll_delta_x_, scroll_delta_y_);
    } else {
        // Default: apply directly to GameViewport
        GameViewport& vp = GameViewport::Instance();
        vp.Scroll(scroll_delta_x_, scroll_delta_y_);
    }
}

//=============================================================================
// Configuration
//=============================================================================

void ScrollProcessor::SetScrollSpeed(int normal, int fast) {
    scroll_speed_normal_ = normal;
    scroll_speed_fast_ = fast;
}

void ScrollProcessor::SetEdgeZoneSize(int pixels) {
    edge_zone_size_ = pixels;
}

//=============================================================================
// Global Functions
//=============================================================================

bool ScrollProcessor_Init() {
    return ScrollProcessor::Instance().Initialize();
}

void ScrollProcessor_Shutdown() {
    ScrollProcessor::Instance().Shutdown();
}

void ScrollProcessor_ProcessFrame() {
    ScrollProcessor::Instance().ProcessFrame();
}

bool ScrollProcessor_IsScrolling() {
    return ScrollProcessor::Instance().IsScrolling();
}

int ScrollProcessor_GetDeltaX() {
    return ScrollProcessor::Instance().GetScrollDeltaX();
}

int ScrollProcessor_GetDeltaY() {
    return ScrollProcessor::Instance().GetScrollDeltaY();
}

void ScrollProcessor_SetEdgeEnabled(bool enabled) {
    ScrollProcessor::Instance().SetEdgeScrollEnabled(enabled);
}

void ScrollProcessor_SetKeyboardEnabled(bool enabled) {
    ScrollProcessor::Instance().SetKeyboardScrollEnabled(enabled);
}
