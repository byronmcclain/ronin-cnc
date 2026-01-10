/**
 * Scroll Manager Implementation
 */

#include "game/scroll_manager.h"

// =============================================================================
// Singleton
// =============================================================================

ScrollManager::ScrollManager()
    : start_x_(0)
    , start_y_(0)
{
}

ScrollManager& ScrollManager::Instance() {
    static ScrollManager instance;
    return instance;
}

// =============================================================================
// Animated Scrolling
// =============================================================================

void ScrollManager::ScrollTo(int world_x, int world_y, ScrollAnimationType type, int duration_frames) {
    if (type == ScrollAnimationType::INSTANT || duration_frames <= 0) {
        JumpTo(world_x, world_y);
        return;
    }

    // Set up animation
    start_x_ = GameViewport::Instance().x;
    start_y_ = GameViewport::Instance().y;

    current_request_.target_x = world_x;
    current_request_.target_y = world_y;
    current_request_.type = type;
    current_request_.duration_frames = duration_frames;
    current_request_.elapsed_frames = 0;
    current_request_.active = true;
}

void ScrollManager::CenterOn(int world_x, int world_y, ScrollAnimationType type, int duration_frames) {
    int target_x = world_x - GameViewport::Instance().width / 2;
    int target_y = world_y - GameViewport::Instance().height / 2;
    ScrollTo(target_x, target_y, type, duration_frames);
}

void ScrollManager::CenterOnCell(int cell_x, int cell_y, ScrollAnimationType type, int duration_frames) {
    int world_x = cell_x * TILE_PIXEL_WIDTH + TILE_PIXEL_WIDTH / 2;
    int world_y = cell_y * TILE_PIXEL_HEIGHT + TILE_PIXEL_HEIGHT / 2;
    CenterOn(world_x, world_y, type, duration_frames);
}

// =============================================================================
// Instant Scrolling
// =============================================================================

void ScrollManager::JumpTo(int world_x, int world_y) {
    CancelScroll();
    GameViewport::Instance().ScrollTo(world_x, world_y);
}

void ScrollManager::JumpToCell(int cell_x, int cell_y) {
    int world_x = cell_x * TILE_PIXEL_WIDTH;
    int world_y = cell_y * TILE_PIXEL_HEIGHT;
    JumpTo(world_x, world_y);
}

// =============================================================================
// Animation Control
// =============================================================================

void ScrollManager::CancelScroll() {
    current_request_.active = false;
}

void ScrollManager::Update() {
    if (!current_request_.active) return;

    current_request_.elapsed_frames++;

    // Calculate progress (0.0 to 1.0)
    float t = static_cast<float>(current_request_.elapsed_frames) /
              static_cast<float>(current_request_.duration_frames);

    if (t >= 1.0f) {
        t = 1.0f;
        current_request_.active = false;
    }

    // Apply easing function
    float eased_t = EaseFunction(t, current_request_.type);

    // Interpolate position
    int new_x = start_x_ + static_cast<int>(
        static_cast<float>(current_request_.target_x - start_x_) * eased_t);
    int new_y = start_y_ + static_cast<int>(
        static_cast<float>(current_request_.target_y - start_y_) * eased_t);

    // Update viewport
    GameViewport::Instance().ScrollTo(new_x, new_y);
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
