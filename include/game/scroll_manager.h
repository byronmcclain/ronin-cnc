/**
 * Scroll Manager - Smooth scroll animation
 *
 * Manages animated viewport scrolling with easing functions.
 *
 * Original: CODE/SCROLL.CPP
 */

#ifndef GAME_SCROLL_MANAGER_H
#define GAME_SCROLL_MANAGER_H

#include "game/viewport.h"

// =============================================================================
// Scroll Animation Types
// =============================================================================

enum class ScrollAnimationType {
    INSTANT,      // Jump immediately
    LINEAR,       // Constant speed
    EASE_OUT,     // Start fast, slow down
    EASE_IN_OUT   // Slow-fast-slow
};

// =============================================================================
// Scroll Request
// =============================================================================

/**
 * ScrollRequest - Pending scroll animation
 */
struct ScrollRequest {
    int target_x;
    int target_y;
    ScrollAnimationType type;
    int duration_frames;
    int elapsed_frames;
    bool active;

    ScrollRequest()
        : target_x(0)
        , target_y(0)
        , type(ScrollAnimationType::INSTANT)
        , duration_frames(0)
        , elapsed_frames(0)
        , active(false)
    {}
};

// =============================================================================
// ScrollManager Class
// =============================================================================

/**
 * ScrollManager - Handles smooth scroll animations
 */
class ScrollManager {
public:
    // Singleton access
    static ScrollManager& Instance();

    // Prevent copying
    ScrollManager(const ScrollManager&) = delete;
    ScrollManager& operator=(const ScrollManager&) = delete;

    // =========================================================================
    // Animated Scrolling
    // =========================================================================

    /**
     * Start animated scroll to position
     */
    void ScrollTo(int world_x, int world_y, ScrollAnimationType type, int duration_frames);

    /**
     * Start animated scroll to center on position
     */
    void CenterOn(int world_x, int world_y, ScrollAnimationType type, int duration_frames);

    /**
     * Start animated scroll to center on cell
     */
    void CenterOnCell(int cell_x, int cell_y, ScrollAnimationType type, int duration_frames);

    // =========================================================================
    // Instant Scrolling
    // =========================================================================

    /**
     * Instant scroll (cancels any animation)
     */
    void JumpTo(int world_x, int world_y);

    /**
     * Instant scroll to cell
     */
    void JumpToCell(int cell_x, int cell_y);

    // =========================================================================
    // Animation Control
    // =========================================================================

    /**
     * Cancel current animation
     */
    void CancelScroll();

    /**
     * Update animation (call each frame)
     */
    void Update();

    /**
     * Check if animation is in progress
     */
    bool IsAnimating() const { return current_request_.active; }

    /**
     * Get animation target
     */
    int GetTargetX() const { return current_request_.target_x; }
    int GetTargetY() const { return current_request_.target_y; }

private:
    ScrollManager();
    ~ScrollManager() = default;

    /**
     * Apply easing function to progress
     */
    float EaseFunction(float t, ScrollAnimationType type) const;

    ScrollRequest current_request_;
    int start_x_;
    int start_y_;
};

#endif // GAME_SCROLL_MANAGER_H
