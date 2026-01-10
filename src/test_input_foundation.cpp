// src/test_input_foundation.cpp
// Test program for input foundation
// Task 16a - Input Foundation

#include "game/input/input_state.h"
#include "game/viewport.h"
#include "platform.h"
#include <cstdio>
#include <cstring>

//=============================================================================
// Unit Tests
//=============================================================================

bool Test_KeyboardStateInit() {
    printf("Test: KeyboardState Initialization... ");

    KeyboardState ks;
    ks.Clear();

    // All keys should be up
    for (int i = 0; i < KEY_CODE_MAX; i++) {
        if (ks.keys_down[i] || ks.keys_down_prev[i]) {
            printf("FAILED - Key %d not cleared\n", i);
            return false;
        }
    }

    if (ks.modifiers != MOD_NONE) {
        printf("FAILED - Modifiers not cleared\n");
        return false;
    }

    if (ks.HasBufferedKeys()) {
        printf("FAILED - Key buffer not empty\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_KeyBuffer() {
    printf("Test: Key Buffer... ");

    KeyboardState ks;
    ks.Clear();

    // Buffer some keys
    ks.BufferKey(KEY_A);
    ks.BufferKey(KEY_B);
    ks.BufferKey(KEY_C);

    if (!ks.HasBufferedKeys()) {
        printf("FAILED - Buffer should have keys\n");
        return false;
    }

    // Get them back in order
    if (ks.GetBufferedKey() != KEY_A) {
        printf("FAILED - Expected KEY_A\n");
        return false;
    }
    if (ks.GetBufferedKey() != KEY_B) {
        printf("FAILED - Expected KEY_B\n");
        return false;
    }
    if (ks.GetBufferedKey() != KEY_C) {
        printf("FAILED - Expected KEY_C\n");
        return false;
    }

    // Buffer should be empty now
    if (ks.HasBufferedKeys()) {
        printf("FAILED - Buffer should be empty\n");
        return false;
    }

    if (ks.GetBufferedKey() != KEY_NONE) {
        printf("FAILED - Empty buffer should return KEY_NONE\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_MouseStateInit() {
    printf("Test: MouseState Initialization... ");

    MouseState ms;
    ms.Clear();

    if (ms.screen_x != 0 || ms.screen_y != 0) {
        printf("FAILED - Position not cleared\n");
        return false;
    }

    for (int i = 0; i < INPUT_MOUSE_MAX; i++) {
        if (ms.buttons_down[i]) {
            printf("FAILED - Button %d not cleared\n", i);
            return false;
        }
    }

    if (ms.is_dragging) {
        printf("FAILED - Drag state not cleared\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_DragDistance() {
    printf("Test: Drag Distance Calculation... ");

    MouseState ms;
    ms.Clear();

    ms.drag_start_x = 100;
    ms.drag_start_y = 100;
    ms.drag_current_x = 103;
    ms.drag_current_y = 104;

    if (ms.GetDragDistanceX() != 3) {
        printf("FAILED - Expected dx=3, got %d\n", ms.GetDragDistanceX());
        return false;
    }

    if (ms.GetDragDistanceY() != 4) {
        printf("FAILED - Expected dy=4, got %d\n", ms.GetDragDistanceY());
        return false;
    }

    if (ms.GetDragDistanceSquared() != 25) {
        printf("FAILED - Expected 25, got %d\n", ms.GetDragDistanceSquared());
        return false;
    }

    // With threshold of 4, distance of 5 should exceed
    if (!ms.DragThresholdExceeded()) {
        printf("FAILED - Drag threshold should be exceeded\n");
        return false;
    }

    // Smaller movement should not exceed
    ms.drag_current_x = 102;
    ms.drag_current_y = 102;
    if (ms.DragThresholdExceeded()) {
        printf("FAILED - Small movement should not exceed threshold\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_EdgeDetection() {
    printf("Test: Key Edge Detection... ");

    KeyboardState ks;
    ks.Clear();

    // Frame 1: Key A pressed
    ks.SavePreviousState();
    ks.keys_down[KEY_A] = true;

    // Just pressed check
    bool just_pressed = ks.keys_down[KEY_A] && !ks.keys_down_prev[KEY_A];
    if (!just_pressed) {
        printf("FAILED - Key A should be 'just pressed'\n");
        return false;
    }

    // Frame 2: Key A still held
    ks.SavePreviousState();
    ks.keys_down[KEY_A] = true;

    just_pressed = ks.keys_down[KEY_A] && !ks.keys_down_prev[KEY_A];
    if (just_pressed) {
        printf("FAILED - Key A should NOT be 'just pressed' (held)\n");
        return false;
    }

    // Frame 3: Key A released
    ks.SavePreviousState();
    ks.keys_down[KEY_A] = false;

    bool just_released = !ks.keys_down[KEY_A] && ks.keys_down_prev[KEY_A];
    if (!just_released) {
        printf("FAILED - Key A should be 'just released'\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_InputStateInit() {
    printf("Test: InputState Initialization... ");

    // Note: This test requires platform to be initialized
    Platform_Init();
    Platform_Graphics_Init();

    // Initialize viewport for coordinate conversion
    GameViewport::Instance().Initialize();
    GameViewport::Instance().SetMapSize(64, 64);

    InputState& input = InputState::Instance();

    if (!input.Initialize()) {
        printf("FAILED - Could not initialize InputState\n");
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return false;
    }

    if (!input.IsInitialized()) {
        printf("FAILED - Not marked as initialized\n");
        input.Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return false;
    }

    input.Shutdown();

    if (input.IsInitialized()) {
        printf("FAILED - Still marked as initialized after shutdown\n");
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return false;
    }

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
    printf("PASSED\n");
    return true;
}

bool Test_GlobalFunctions() {
    printf("Test: Global Input Functions... ");

    Platform_Init();
    Platform_Graphics_Init();

    // Initialize viewport for coordinate conversion
    GameViewport::Instance().Initialize();
    GameViewport::Instance().SetMapSize(64, 64);

    if (!Input_Init()) {
        printf("FAILED - Input_Init failed\n");
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return false;
    }

    // Call update
    Input_Update();

    // Test position retrieval
    int mx, my;
    Input_GetMousePosition(&mx, &my);
    // Just verify it doesn't crash

    // Test drag rect
    int x1, y1, x2, y2;
    Input_GetDragRect(&x1, &y1, &x2, &y2);
    // Verify it doesn't crash

    Input_Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_KeyCodes() {
    printf("Test: Key Code Constants... ");

    // Verify key codes match expected values
    if (KEY_ESCAPE != 27) {
        printf("FAILED - KEY_ESCAPE != 27\n");
        return false;
    }
    if (KEY_RETURN != 13) {
        printf("FAILED - KEY_RETURN != 13\n");
        return false;
    }
    if (KEY_SPACE != 32) {
        printf("FAILED - KEY_SPACE != 32\n");
        return false;
    }
    if (KEY_A != 'A') {
        printf("FAILED - KEY_A != 'A'\n");
        return false;
    }
    if (KEY_0 != '0') {
        printf("FAILED - KEY_0 != '0'\n");
        return false;
    }
    if (KEY_F1 != 112) {
        printf("FAILED - KEY_F1 != 112\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_ModifierFlags() {
    printf("Test: Modifier Flags... ");

    // Test flag combinations
    uint8_t mods = MOD_NONE;
    if (mods != 0) {
        printf("FAILED - MOD_NONE != 0\n");
        return false;
    }

    mods = MOD_SHIFT | MOD_CTRL;
    if ((mods & MOD_SHIFT) == 0) {
        printf("FAILED - MOD_SHIFT not set\n");
        return false;
    }
    if ((mods & MOD_CTRL) == 0) {
        printf("FAILED - MOD_CTRL not set\n");
        return false;
    }
    if ((mods & MOD_ALT) != 0) {
        printf("FAILED - MOD_ALT should not be set\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

//=============================================================================
// Interactive Test
//=============================================================================

void Interactive_Test() {
    printf("\n=== Interactive Input Test ===\n");
    printf("Press keys and move mouse to test\n");
    printf("Press ESC to exit\n\n");

    Platform_Init();
    Platform_Graphics_Init();

    // Initialize viewport for coordinate conversion
    GameViewport::Instance().Initialize();
    GameViewport::Instance().SetMapSize(64, 64);

    Input_Init();

    bool running = true;
    while (running) {
        Platform_PollEvents();
        Input_Update();

        // Check for exit
        if (Input_KeyPressed(KEY_ESCAPE)) {
            running = false;
            continue;
        }

        // Report any key presses
        for (int k = 0; k < KEY_CODE_MAX; k++) {
            if (Input_KeyPressed(k)) {
                printf("Key pressed: %d", k);
                if (k >= 'A' && k <= 'Z') {
                    printf(" ('%c')", k);
                } else if (k >= '0' && k <= '9') {
                    printf(" ('%c')", k);
                }
                if (Input_ShiftDown()) printf(" +SHIFT");
                if (Input_CtrlDown()) printf(" +CTRL");
                if (Input_AltDown()) printf(" +ALT");
                printf("\n");
            }
        }

        // Report mouse clicks
        if (Input_MouseButtonPressed(INPUT_MOUSE_LEFT)) {
            int mx, my;
            Input_GetMousePosition(&mx, &my);
            printf("Left click at screen (%d, %d)", mx, my);

            int wx, wy;
            Input_GetMouseWorldPosition(&wx, &wy);
            printf(" world (%d, %d)", wx, wy);

            int cx, cy;
            Input_GetMouseCellPosition(&cx, &cy);
            printf(" cell (%d, %d)\n", cx, cy);
        }

        if (Input_MouseButtonPressed(INPUT_MOUSE_RIGHT)) {
            printf("Right click\n");
        }

        if (Input_MouseDoubleClicked(INPUT_MOUSE_LEFT)) {
            printf("Double click!\n");
        }

        // Report drag
        static bool was_dragging = false;
        if (Input_IsDragging() && !was_dragging) {
            printf("Drag started\n");
        }
        if (!Input_IsDragging() && was_dragging) {
            int x1, y1, x2, y2;
            Input_GetDragRect(&x1, &y1, &x2, &y2);
            printf("Drag ended: (%d,%d) to (%d,%d)\n", x1, y1, x2, y2);
        }
        was_dragging = Input_IsDragging();

        Platform_Timer_Delay(16);
    }

    Input_Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Main
//=============================================================================

int main(int argc, char* argv[]) {
    printf("=== Input Foundation Tests (Task 16a) ===\n\n");

    // Check for quick mode (skip interactive)
    bool quick_mode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--quick") == 0 || strcmp(argv[i], "-q") == 0) {
            quick_mode = true;
        }
    }

    int passed = 0;
    int failed = 0;

    // Run unit tests
    if (Test_KeyboardStateInit()) passed++; else failed++;
    if (Test_KeyBuffer()) passed++; else failed++;
    if (Test_MouseStateInit()) passed++; else failed++;
    if (Test_DragDistance()) passed++; else failed++;
    if (Test_EdgeDetection()) passed++; else failed++;
    if (Test_KeyCodes()) passed++; else failed++;
    if (Test_ModifierFlags()) passed++; else failed++;
    if (Test_InputStateInit()) passed++; else failed++;
    if (Test_GlobalFunctions()) passed++; else failed++;

    printf("\n");
    if (failed == 0) {
        printf("All tests PASSED (%d/%d)\n", passed, passed + failed);
    } else {
        printf("Results: %d passed, %d failed\n", passed, failed);
    }

    // Interactive test
    if (!quick_mode && argc > 1 && strcmp(argv[1], "-i") == 0) {
        Interactive_Test();
    } else if (!quick_mode) {
        printf("\nRun with -i for interactive test\n");
    }

    return (failed == 0) ? 0 : 1;
}
