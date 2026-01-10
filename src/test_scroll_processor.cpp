// src/test_scroll_processor.cpp
// Test program for scroll processor
// Task 16g - Scrolling

#include "game/input/scroll_processor.h"
#include "game/input/mouse_handler.h"
#include "game/input/input_state.h"
#include "game/input/input_mapper.h"
#include "game/viewport.h"
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
    if ((SCROLLDIR_N | SCROLLDIR_E) != SCROLLDIR_NE) {
        printf("FAILED - NE combination wrong\n");
        return false;
    }

    if ((SCROLLDIR_S | SCROLLDIR_W) != SCROLLDIR_SW) {
        printf("FAILED - SW combination wrong\n");
        return false;
    }

    // Test direction detection
    uint8_t dir = SCROLLDIR_NONE;
    dir |= SCROLLDIR_N;
    dir |= SCROLLDIR_W;

    if (dir != SCROLLDIR_NW) {
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
    GameViewport::Instance().Initialize();
    GameViewport::Instance().SetMapSize(128, 128);
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
    if ((SCROLLDIR_N & SCROLLDIR_S) != 0) {
        printf("FAILED - N and S overlap\n");
        return false;
    }

    if ((SCROLLDIR_E & SCROLLDIR_W) != 0) {
        printf("FAILED - E and W overlap\n");
        return false;
    }

    // Verify compound directions
    if ((SCROLLDIR_NE & SCROLLDIR_N) == 0) {
        printf("FAILED - NE should contain N\n");
        return false;
    }

    if ((SCROLLDIR_NE & SCROLLDIR_E) == 0) {
        printf("FAILED - NE should contain E\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_GlobalFunctions() {
    printf("Test: Global Functions... ");

    Platform_Init();
    Input_Init();
    GameViewport::Instance().Initialize();
    GameViewport::Instance().SetMapSize(128, 128);
    MouseHandler_Init();
    InputMapper_Init();

    if (!ScrollProcessor_Init()) {
        printf("FAILED - Init failed\n");
        InputMapper_Shutdown();
        MouseHandler_Shutdown();
        Input_Shutdown();
        Platform_Shutdown();
        return false;
    }

    // Test global enable/disable
    ScrollProcessor_SetEdgeEnabled(false);
    if (ScrollProcessor::Instance().IsEdgeScrollEnabled()) {
        printf("FAILED - Edge disable failed\n");
        ScrollProcessor_Shutdown();
        InputMapper_Shutdown();
        MouseHandler_Shutdown();
        Input_Shutdown();
        Platform_Shutdown();
        return false;
    }

    ScrollProcessor_SetEdgeEnabled(true);
    ScrollProcessor_SetKeyboardEnabled(false);
    if (ScrollProcessor::Instance().IsKeyboardScrollEnabled()) {
        printf("FAILED - Keyboard disable failed\n");
        ScrollProcessor_Shutdown();
        InputMapper_Shutdown();
        MouseHandler_Shutdown();
        Input_Shutdown();
        Platform_Shutdown();
        return false;
    }

    ScrollProcessor_SetKeyboardEnabled(true);

    // Test delta getters (should be 0 without input)
    ScrollProcessor_ProcessFrame();
    if (ScrollProcessor_GetDeltaX() != 0 || ScrollProcessor_GetDeltaY() != 0) {
        printf("FAILED - Delta should be 0 without input\n");
        ScrollProcessor_Shutdown();
        InputMapper_Shutdown();
        MouseHandler_Shutdown();
        Input_Shutdown();
        Platform_Shutdown();
        return false;
    }

    ScrollProcessor_Shutdown();
    InputMapper_Shutdown();
    MouseHandler_Shutdown();
    Input_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

int main(int argc, char* argv[]) {
    printf("=== Scroll Processor Tests (Task 16g) ===\n\n");

    // Check for quick mode
    bool quick_mode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--quick") == 0 || strcmp(argv[i], "-q") == 0) {
            quick_mode = true;
        }
    }
    (void)quick_mode;

    int passed = 0;
    int failed = 0;

    if (Test_ScrollProcessorInit()) passed++; else failed++;
    if (Test_ScrollDirection()) passed++; else failed++;
    if (Test_ScrollSpeedConfig()) passed++; else failed++;
    if (Test_EnableDisable()) passed++; else failed++;
    if (Test_DirectionValues()) passed++; else failed++;
    if (Test_ScrollCallback()) passed++; else failed++;
    if (Test_GlobalFunctions()) passed++; else failed++;

    printf("\n");
    if (failed == 0) {
        printf("All tests PASSED (%d/%d)\n", passed, passed + failed);
    } else {
        printf("Results: %d passed, %d failed\n", passed, failed);
    }

    return (failed == 0) ? 0 : 1;
}
