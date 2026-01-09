# Task 16g: Edge & Keyboard Scrolling

| Field | Value |
|-------|-------|
| **Task ID** | 16g |
| **Task Name** | Edge & Keyboard Scrolling |
| **Phase** | 16 - Input Integration |
| **Depends On** | 16d (Mouse Handler), 16b (GameAction), 15g (Viewport) |
| **Estimated Complexity** | Low-Medium (~250 lines) |

---

## Dependencies

- Task 16d (Mouse Handler) must be complete for edge detection
- Task 16b (GameAction) must be complete for scroll actions
- Phase 15g (Viewport) must be complete for actual scrolling

---

## Context

Scrolling is fundamental to RTS gameplay - players need to navigate the map quickly. Red Alert supports two scrolling methods:

1. **Edge Scrolling**: Move mouse to screen edge to scroll
2. **Keyboard Scrolling**: Use arrow keys to scroll

Both methods should feel responsive and support fast scrolling with Shift held. The scrolling system integrates with the viewport (from Phase 15) to actually move the view.

---

## Objective

Create a scroll processor that:
1. Detects mouse at screen edges
2. Responds to arrow key input
3. Supports fast scrolling with Shift modifier
4. Smoothly integrates with viewport
5. Respects map boundaries

---

## Technical Background

### Edge Scroll Zones

```
+--------------------------------------------------+
|   N     N     N     N     N     N     N     N    |  <- 8px from top
| W                                            E   |
| W                                            E   |
| W          TACTICAL AREA (no scroll)         E   |
| W                                            E   |
| W                                            E   |
|   S     S     S     S     S     S     S     S    |  <- 8px from bottom
+--------------------------------------------------+
  ^                                            ^
  |                                            |
 8px from left                            8px from right
 (only in tactical area, x < 480)

Corner zones scroll diagonally (NE, SE, SW, NW)
```

### Scroll Speeds

```cpp
// Original Red Alert scroll speeds (per frame at 15 FPS)
SCROLL_SPEED_NORMAL = 8;    // pixels per frame
SCROLL_SPEED_FAST = 24;     // with Shift held

// At 60 FPS, we need to adjust:
SCROLL_SPEED_NORMAL = 2;    // equivalent smooth scrolling
SCROLL_SPEED_FAST = 6;
```

### Scroll Directions

```cpp
enum ScrollDirection {
    SCROLL_NONE = 0,
    SCROLL_N    = (1 << 0),
    SCROLL_S    = (1 << 1),
    SCROLL_E    = (1 << 2),
    SCROLL_W    = (1 << 3),
    SCROLL_NE   = SCROLL_N | SCROLL_E,
    SCROLL_SE   = SCROLL_S | SCROLL_E,
    SCROLL_NW   = SCROLL_N | SCROLL_W,
    SCROLL_SW   = SCROLL_S | SCROLL_W,
};
```

---

## Deliverables

- [ ] `src/game/input/scroll_processor.h` - Scroll processing declaration
- [ ] `src/game/input/scroll_processor.cpp` - Scroll processing implementation
- [ ] `tests/test_scrolling.cpp` - Test program
- [ ] CMakeLists.txt updated
- [ ] All tests pass

---

## Files to Create

### src/game/input/scroll_processor.h

```cpp
// src/game/input/scroll_processor.h
// Edge and keyboard scroll processing
// Task 16g - Scrolling

#ifndef SCROLL_PROCESSOR_H
#define SCROLL_PROCESSOR_H

#include <cstdint>
#include <functional>

//=============================================================================
// Scroll Constants
//=============================================================================

static const int EDGE_SCROLL_ZONE = 8;       // Pixels from edge to trigger
static const int SCROLL_SPEED_NORMAL = 2;    // Pixels per frame at 60 FPS
static const int SCROLL_SPEED_FAST = 6;      // With Shift held

// Screen boundaries (tactical area only)
static const int SCROLL_AREA_LEFT = 0;
static const int SCROLL_AREA_TOP = 16;       // Below tab bar
static const int SCROLL_AREA_RIGHT = 480;    // Sidebar starts here
static const int SCROLL_AREA_BOTTOM = 400;

//=============================================================================
// Scroll Direction Flags
//=============================================================================

enum ScrollDirection : uint8_t {
    SCROLL_NONE = 0,
    SCROLL_N    = (1 << 0),
    SCROLL_S    = (1 << 1),
    SCROLL_E    = (1 << 2),
    SCROLL_W    = (1 << 3),
    SCROLL_NE   = SCROLL_N | SCROLL_E,
    SCROLL_SE   = SCROLL_S | SCROLL_E,
    SCROLL_NW   = SCROLL_N | SCROLL_W,
    SCROLL_SW   = SCROLL_S | SCROLL_W,
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
    ScrollDirection GetEdgeScrollDirection() const { return edge_direction_; }

    //=========================================================================
    // Keyboard Scrolling
    //=========================================================================

    void SetKeyboardScrollEnabled(bool enabled) { keyboard_scroll_enabled_ = enabled; }
    bool IsKeyboardScrollEnabled() const { return keyboard_scroll_enabled_; }

    //=========================================================================
    // Scroll State
    //=========================================================================

    // Combined scroll direction (edge + keyboard)
    ScrollDirection GetScrollDirection() const { return combined_direction_; }

    // Is scrolling active?
    bool IsScrolling() const { return combined_direction_ != SCROLL_NONE; }

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

    ScrollDirection DetectEdgeScroll();
    ScrollDirection DetectKeyboardScroll();
    void CalculateScrollDelta(ScrollDirection dir, bool fast);
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
    ScrollDirection edge_direction_;
    ScrollDirection keyboard_direction_;
    ScrollDirection combined_direction_;
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
```

### src/game/input/scroll_processor.cpp

```cpp
// src/game/input/scroll_processor.cpp
// Edge and keyboard scroll processing implementation
// Task 16g - Scrolling

#include "scroll_processor.h"
#include "mouse_handler.h"
#include "input_mapper.h"
#include "game_action.h"
#include "viewport.h"
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
    , edge_zone_size_(EDGE_SCROLL_ZONE)
    , edge_direction_(SCROLL_NONE)
    , keyboard_direction_(SCROLL_NONE)
    , combined_direction_(SCROLL_NONE)
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

    Platform_Log("ScrollProcessor: Initializing...");

    edge_direction_ = SCROLL_NONE;
    keyboard_direction_ = SCROLL_NONE;
    combined_direction_ = SCROLL_NONE;
    scroll_delta_x_ = scroll_delta_y_ = 0;

    initialized_ = true;
    Platform_Log("ScrollProcessor: Initialized");
    return true;
}

void ScrollProcessor::Shutdown() {
    if (!initialized_) return;

    Platform_Log("ScrollProcessor: Shutting down");
    initialized_ = false;
}

void ScrollProcessor::ProcessFrame() {
    if (!initialized_) return;

    // Reset per-frame state
    scroll_delta_x_ = 0;
    scroll_delta_y_ = 0;

    // Detect scroll from both sources
    edge_direction_ = edge_scroll_enabled_ ? DetectEdgeScroll() : SCROLL_NONE;
    keyboard_direction_ = keyboard_scroll_enabled_ ? DetectKeyboardScroll() : SCROLL_NONE;

    // Combine directions (keyboard takes priority for that axis if both active)
    combined_direction_ = static_cast<ScrollDirection>(
        edge_direction_ | keyboard_direction_);

    // Calculate and apply scroll if any direction active
    if (combined_direction_ != SCROLL_NONE) {
        // Check if fast scroll (Shift held)
        bool fast = InputMapper::Instance().IsActionActive(GameAction::SCROLL_FAST);

        CalculateScrollDelta(combined_direction_, fast);
        ApplyScroll();
    }
}

//=============================================================================
// Edge Scroll Detection
//=============================================================================

ScrollDirection ScrollProcessor::DetectEdgeScroll() {
    MouseHandler& mouse = MouseHandler::Instance();

    // Only scroll when mouse is in tactical area
    if (!mouse.IsInTacticalArea()) {
        return SCROLL_NONE;
    }

    int sx = mouse.GetScreenX();
    int sy = mouse.GetScreenY();

    uint8_t direction = SCROLL_NONE;

    // Check horizontal edges
    if (sx < SCROLL_AREA_LEFT + edge_zone_size_) {
        direction |= SCROLL_W;
    } else if (sx >= SCROLL_AREA_RIGHT - edge_zone_size_) {
        direction |= SCROLL_E;
    }

    // Check vertical edges
    if (sy < SCROLL_AREA_TOP + edge_zone_size_) {
        direction |= SCROLL_N;
    } else if (sy >= SCROLL_AREA_BOTTOM - edge_zone_size_) {
        direction |= SCROLL_S;
    }

    return static_cast<ScrollDirection>(direction);
}

//=============================================================================
// Keyboard Scroll Detection
//=============================================================================

ScrollDirection ScrollProcessor::DetectKeyboardScroll() {
    InputMapper& mapper = InputMapper::Instance();

    uint8_t direction = SCROLL_NONE;

    // Check arrow key actions
    if (mapper.IsActionActive(GameAction::SCROLL_UP)) {
        direction |= SCROLL_N;
    }
    if (mapper.IsActionActive(GameAction::SCROLL_DOWN)) {
        direction |= SCROLL_S;
    }
    if (mapper.IsActionActive(GameAction::SCROLL_LEFT)) {
        direction |= SCROLL_W;
    }
    if (mapper.IsActionActive(GameAction::SCROLL_RIGHT)) {
        direction |= SCROLL_E;
    }

    return static_cast<ScrollDirection>(direction);
}

//=============================================================================
// Scroll Calculation
//=============================================================================

void ScrollProcessor::CalculateScrollDelta(ScrollDirection dir, bool fast) {
    int speed = fast ? scroll_speed_fast_ : scroll_speed_normal_;

    scroll_delta_x_ = 0;
    scroll_delta_y_ = 0;

    // Horizontal
    if (dir & SCROLL_W) {
        scroll_delta_x_ = -speed;
    } else if (dir & SCROLL_E) {
        scroll_delta_x_ = speed;
    }

    // Vertical
    if (dir & SCROLL_N) {
        scroll_delta_y_ = -speed;
    } else if (dir & SCROLL_S) {
        scroll_delta_y_ = speed;
    }

    // Diagonal movement: normalize to prevent faster diagonal scroll
    // Optional: For true diagonal normalization, divide by sqrt(2)
    // Most RTS games don't do this, so keeping it simple
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
        // Default: apply directly to viewport
        Viewport& vp = Viewport::Instance();
        vp.Scroll(scroll_delta_x_, scroll_delta_y_);
    }
}

//=============================================================================
// Configuration
//=============================================================================

void ScrollProcessor::SetScrollSpeed(int normal, int fast) {
    scroll_speed_normal_ = normal;
    scroll_speed_fast_ = fast;
    Platform_Log("ScrollProcessor: Speed set to %d/%d", normal, fast);
}

void ScrollProcessor::SetEdgeZoneSize(int pixels) {
    edge_zone_size_ = pixels;
    Platform_Log("ScrollProcessor: Edge zone set to %d pixels", pixels);
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
```

### tests/test_scrolling.cpp

```cpp
// tests/test_scrolling.cpp
// Test program for scroll processor
// Task 16g - Scrolling

#include "scroll_processor.h"
#include "mouse_handler.h"
#include "input_state.h"
#include "input_mapper.h"
#include "viewport.h"
#include "platform.h"
#include <cstdio>
#include <cstring>

// Track scroll applications
static int g_total_scroll_x = 0;
static int g_total_scroll_y = 0;
static int g_scroll_calls = 0;

static void TestApplyScroll(int dx, int dy) {
    g_total_scroll_x += dx;
    g_total_scroll_y += dy;
    g_scroll_calls++;
}

static void ResetScrollTracking() {
    g_total_scroll_x = 0;
    g_total_scroll_y = 0;
    g_scroll_calls = 0;
}

bool Test_ScrollProcessorInit() {
    printf("Test: ScrollProcessor Init... ");

    if (!ScrollProcessor_Init()) {
        printf("FAILED - Init failed\n");
        return false;
    }

    if (ScrollProcessor_IsScrolling()) {
        printf("FAILED - Should not be scrolling initially\n");
        ScrollProcessor_Shutdown();
        return false;
    }

    ScrollProcessor_Shutdown();
    printf("PASSED\n");
    return true;
}

bool Test_ScrollDirection() {
    printf("Test: Scroll Direction Flags... ");

    // Test direction combinations
    if ((SCROLL_N | SCROLL_E) != SCROLL_NE) {
        printf("FAILED - NE combination wrong\n");
        return false;
    }

    if ((SCROLL_S | SCROLL_W) != SCROLL_SW) {
        printf("FAILED - SW combination wrong\n");
        return false;
    }

    // Test direction detection
    uint8_t dir = SCROLL_NONE;
    dir |= SCROLL_N;
    dir |= SCROLL_W;

    if (dir != SCROLL_NW) {
        printf("FAILED - Combined flags wrong\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_ScrollSpeedConfig() {
    printf("Test: Scroll Speed Configuration... ");

    ScrollProcessor_Init();

    auto& scroll = ScrollProcessor::Instance();

    scroll.SetScrollSpeed(5, 15);
    // Would need internal access to verify, just ensure no crash

    scroll.SetEdgeZoneSize(12);

    ScrollProcessor_Shutdown();
    printf("PASSED\n");
    return true;
}

bool Test_EnableDisable() {
    printf("Test: Enable/Disable Scrolling... ");

    ScrollProcessor_Init();

    auto& scroll = ScrollProcessor::Instance();

    // Disable edge scrolling
    scroll.SetEdgeScrollEnabled(false);
    if (scroll.IsEdgeScrollEnabled()) {
        printf("FAILED - Edge scroll should be disabled\n");
        ScrollProcessor_Shutdown();
        return false;
    }

    // Re-enable
    scroll.SetEdgeScrollEnabled(true);
    if (!scroll.IsEdgeScrollEnabled()) {
        printf("FAILED - Edge scroll should be enabled\n");
        ScrollProcessor_Shutdown();
        return false;
    }

    // Same for keyboard
    scroll.SetKeyboardScrollEnabled(false);
    if (scroll.IsKeyboardScrollEnabled()) {
        printf("FAILED - Keyboard scroll should be disabled\n");
        ScrollProcessor_Shutdown();
        return false;
    }

    scroll.SetKeyboardScrollEnabled(true);

    ScrollProcessor_Shutdown();
    printf("PASSED\n");
    return true;
}

bool Test_ScrollCallback() {
    printf("Test: Scroll Callback... ");

    ResetScrollTracking();

    Platform_Init();
    Input_Init();
    Viewport::Instance().Initialize();
    Viewport::Instance().SetMapSize(128, 128);
    MouseHandler_Init();
    InputMapper_Init();
    ScrollProcessor_Init();

    auto& scroll = ScrollProcessor::Instance();
    scroll.SetApplyScrollCallback(TestApplyScroll);

    // Process multiple frames
    for (int i = 0; i < 10; i++) {
        ScrollProcessor_ProcessFrame();
    }

    // Without any input, should not have scrolled
    // (actual scroll would require simulated input)

    ScrollProcessor_Shutdown();
    InputMapper_Shutdown();
    MouseHandler_Shutdown();
    Input_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_DirectionValues() {
    printf("Test: Direction Bit Values... ");

    // Verify directions don't overlap incorrectly
    if ((SCROLL_N & SCROLL_S) != 0) {
        printf("FAILED - N and S overlap\n");
        return false;
    }

    if ((SCROLL_E & SCROLL_W) != 0) {
        printf("FAILED - E and W overlap\n");
        return false;
    }

    // Verify compound directions
    if ((SCROLL_NE & SCROLL_N) == 0) {
        printf("FAILED - NE should contain N\n");
        return false;
    }

    if ((SCROLL_NE & SCROLL_E) == 0) {
        printf("FAILED - NE should contain E\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

void Interactive_Test() {
    printf("\n=== Interactive Scroll Test ===\n");
    printf("Move mouse to edges to scroll\n");
    printf("Use arrow keys to scroll\n");
    printf("Hold SHIFT for fast scroll\n");
    printf("Press E to toggle edge scroll, K for keyboard scroll\n");
    printf("ESC to exit\n\n");

    Platform_Init();
    Platform_Graphics_Init();
    Input_Init();
    Viewport::Instance().Initialize();
    Viewport::Instance().SetMapSize(128, 128);
    MouseHandler_Init();
    InputMapper_Init();
    ScrollProcessor_Init();

    auto& scroll = ScrollProcessor::Instance();
    auto& vp = Viewport::Instance();

    bool running = true;
    ScrollDirection last_dir = SCROLL_NONE;

    while (running) {
        Platform_PumpEvents();
        Input_Update();
        InputMapper_ProcessFrame();
        MouseHandler_ProcessFrame();
        ScrollProcessor_ProcessFrame();

        if (Input_KeyPressed(KEY_ESCAPE)) {
            running = false;
            continue;
        }

        // Toggle edge scroll
        if (Input_KeyPressed(KEY_E)) {
            scroll.SetEdgeScrollEnabled(!scroll.IsEdgeScrollEnabled());
            printf("Edge scroll: %s\n",
                   scroll.IsEdgeScrollEnabled() ? "ON" : "OFF");
        }

        // Toggle keyboard scroll
        if (Input_KeyPressed(KEY_K)) {
            scroll.SetKeyboardScrollEnabled(!scroll.IsKeyboardScrollEnabled());
            printf("Keyboard scroll: %s\n",
                   scroll.IsKeyboardScrollEnabled() ? "ON" : "OFF");
        }

        // Report direction changes
        ScrollDirection dir = scroll.GetScrollDirection();
        if (dir != last_dir) {
            if (dir != SCROLL_NONE) {
                printf("Scrolling: ");
                if (dir & SCROLL_N) printf("N ");
                if (dir & SCROLL_S) printf("S ");
                if (dir & SCROLL_E) printf("E ");
                if (dir & SCROLL_W) printf("W ");
                printf("(delta: %d, %d)\n",
                       scroll.GetScrollDeltaX(), scroll.GetScrollDeltaY());
            } else {
                printf("Scroll stopped\n");
            }
            last_dir = dir;
        }

        // Report viewport position periodically when scrolling
        static int frame = 0;
        if (scroll.IsScrolling() && (++frame % 30 == 0)) {
            printf("Viewport: (%d, %d)\n", vp.x, vp.y);
        }

        Platform_Timer_Delay(16);
    }

    ScrollProcessor_Shutdown();
    InputMapper_Shutdown();
    MouseHandler_Shutdown();
    Input_Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

int main(int argc, char* argv[]) {
    printf("=== Scroll Processor Tests (Task 16g) ===\n\n");

    int passed = 0;
    int failed = 0;

    if (Test_ScrollProcessorInit()) passed++; else failed++;
    if (Test_ScrollDirection()) passed++; else failed++;
    if (Test_ScrollSpeedConfig()) passed++; else failed++;
    if (Test_EnableDisable()) passed++; else failed++;
    if (Test_DirectionValues()) passed++; else failed++;
    if (Test_ScrollCallback()) passed++; else failed++;

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
# Scroll Processor (Task 16g)
target_sources(game PRIVATE
    input/scroll_processor.h
    input/scroll_processor.cpp
)
```

Add test target:

```cmake
# Scroll test
add_executable(test_scrolling
    tests/test_scrolling.cpp
    src/game/input/scroll_processor.cpp
    src/game/input/mouse_handler.cpp
    src/game/input/input_mapper.cpp
    src/game/input/input_state.cpp
)
target_include_directories(test_scrolling PRIVATE
    src/game/input
    src/game/graphics
    src/platform
)
target_link_libraries(test_scrolling platform)
```

---

## Verification Script

```bash
#!/bin/bash
set -e

echo "=== Task 16g Verification ==="

cd build
cmake --build . --target test_scrolling

echo "Running tests..."
./tests/test_scrolling

echo "Checking files..."
for file in \
    "../src/game/input/scroll_processor.h" \
    "../src/game/input/scroll_processor.cpp"
do
    if [ ! -f "$file" ]; then
        echo "ERROR: Missing $file"
        exit 1
    fi
done

echo ""
echo "=== Task 16g Verification PASSED ==="
```

---

## Implementation Steps

1. **Create scroll_processor.h** - Define constants and class
2. **Create scroll_processor.cpp** - Implement scroll logic
3. **Create test_scrolling.cpp** - Tests and interactive demo
4. **Update CMakeLists.txt** - Add files
5. **Build and test** - Verify all pass

---

## Success Criteria

- [ ] Edge scroll detects all 8 directions
- [ ] Keyboard scroll responds to arrow keys
- [ ] Fast scroll (Shift) increases speed
- [ ] Edge and keyboard scroll combine correctly
- [ ] Enable/disable toggles work
- [ ] Scroll callback receives correct deltas
- [ ] Interactive test shows smooth scrolling
- [ ] All unit tests pass

---

## Common Issues

### Diagonal scroll too fast
Consider normalizing diagonal movement speed (divide by sqrt(2)).

### Edge scroll triggers in sidebar
Ensure edge detection only works in tactical area (x < 480).

### Scroll doesn't stop
Make sure direction is reset to SCROLL_NONE when no input detected.

---

## Completion Promise

When verification passes, output:
<promise>TASK_16G_COMPLETE</promise>

## Escape Hatch

If stuck after 15 iterations, output:
<promise>TASK_16G_BLOCKED</promise>

## Max Iterations
15
