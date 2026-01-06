# Task 04d: Input Integration Test

## Dependencies
- Task 04c must be complete (Mouse input)
- Full keyboard and mouse handling in place

## Context
This final task for the input system verifies everything works together by updating the demo to display real-time input state. This serves as both a test and a visual demonstration that keyboard and mouse input are functioning correctly.

## Objective
Create an interactive input demo that displays keyboard state, mouse position, button state, and responds to input in real-time.

## Deliverables
- [ ] Update demo mode to show input state
- [ ] Display mouse position and button state visually
- [ ] Show key presses on screen
- [ ] Verify modifier keys work (Shift, Ctrl, Alt)
- [ ] Test double-click detection
- [ ] All input FFI functions callable from C++

## Files to Modify

### Update src/main.cpp
Replace or enhance run_demo() to show input:
```cpp
int run_demo() {
    printf("=== Red Alert Platform Demo ===\n\n");
    printf("Controls:\n");
    printf("  Mouse: Move to see position, click to see buttons\n");
    printf("  Keys:  Press any key to see it detected\n");
    printf("  Shift/Ctrl/Alt: Hold to see modifier state\n");
    printf("  ESC: Exit demo\n\n");

    // Initialize platform
    if (Platform_Init() != PLATFORM_RESULT_SUCCESS) {
        printf("Failed to initialize platform!\n");
        return 1;
    }

    // Initialize graphics
    if (Platform_Graphics_Init() != 0) {
        char err[256];
        Platform_GetLastError(reinterpret_cast<int8_t*>(err), sizeof(err));
        printf("Failed to initialize graphics: %s\n", err);
        Platform_Shutdown();
        return 1;
    }

    // Get back buffer
    uint8_t* pixels = nullptr;
    int32_t width, height, pitch;
    if (Platform_Graphics_GetBackBuffer(&pixels, &width, &height, &pitch) != 0) {
        printf("Failed to get back buffer!\n");
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return 1;
    }

    printf("Display: %dx%d\n", width, height);
    printf("Running input demo...\n\n");

    // Setup grayscale palette (default is fine)

    // Main loop
    uint32_t frame = 0;
    uint32_t last_fps_time = Platform_GetTicks();
    int fps_counter = 0;

    while (!Platform_PollEvents()) {
        uint32_t time = Platform_GetTicks();

        // Begin input frame
        Platform_Input_Update();

        // Clear back buffer
        Platform_Graphics_ClearBackBuffer(0);

        // Get mouse position (scale from window to buffer coords)
        int32_t mouse_x, mouse_y;
        Platform_Mouse_GetPosition(&mouse_x, &mouse_y);
        // Window is 2x scaled, so divide by 2
        mouse_x /= 2;
        mouse_y /= 2;

        // Clamp to buffer bounds
        if (mouse_x < 0) mouse_x = 0;
        if (mouse_y < 0) mouse_y = 0;
        if (mouse_x >= width) mouse_x = width - 1;
        if (mouse_y >= height) mouse_y = height - 1;

        // Draw crosshair at mouse position
        for (int i = -10; i <= 10; i++) {
            int x = mouse_x + i;
            int y = mouse_y;
            if (x >= 0 && x < width) {
                pixels[y * pitch + x] = 255;
            }
            x = mouse_x;
            y = mouse_y + i;
            if (y >= 0 && y < height) {
                pixels[y * pitch + x] = 255;
            }
        }

        // Draw mouse button indicators (top left)
        // Left button - red square if pressed
        uint8_t left_color = Platform_Mouse_IsPressed(MOUSE_BUTTON_LEFT) ? 200 : 50;
        for (int y = 10; y < 30; y++) {
            for (int x = 10; x < 30; x++) {
                pixels[y * pitch + x] = left_color;
            }
        }

        // Right button - green square if pressed
        uint8_t right_color = Platform_Mouse_IsPressed(MOUSE_BUTTON_RIGHT) ? 150 : 50;
        for (int y = 10; y < 30; y++) {
            for (int x = 35; x < 55; x++) {
                pixels[y * pitch + x] = right_color;
            }
        }

        // Middle button - blue square if pressed
        uint8_t middle_color = Platform_Mouse_IsPressed(MOUSE_BUTTON_MIDDLE) ? 100 : 50;
        for (int y = 10; y < 30; y++) {
            for (int x = 60; x < 80; x++) {
                pixels[y * pitch + x] = middle_color;
            }
        }

        // Draw modifier key indicators (top right area)
        // Shift indicator
        uint8_t shift_color = Platform_Key_ShiftDown() ? 255 : 30;
        for (int y = 10; y < 25; y++) {
            for (int x = width - 90; x < width - 70; x++) {
                pixels[y * pitch + x] = shift_color;
            }
        }

        // Ctrl indicator
        uint8_t ctrl_color = Platform_Key_CtrlDown() ? 255 : 30;
        for (int y = 10; y < 25; y++) {
            for (int x = width - 65; x < width - 45; x++) {
                pixels[y * pitch + x] = ctrl_color;
            }
        }

        // Alt indicator
        uint8_t alt_color = Platform_Key_AltDown() ? 255 : 30;
        for (int y = 10; y < 25; y++) {
            for (int x = width - 40; x < width - 20; x++) {
                pixels[y * pitch + x] = alt_color;
            }
        }

        // Draw border
        for (int x = 0; x < width; x++) {
            pixels[x] = 128;
            pixels[(height - 1) * pitch + x] = 128;
        }
        for (int y = 0; y < height; y++) {
            pixels[y * pitch] = 128;
            pixels[y * pitch + width - 1] = 128;
        }

        // Check for double-click (flash screen)
        if (Platform_Mouse_WasDoubleClicked(MOUSE_BUTTON_LEFT)) {
            // Flash the border bright
            for (int x = 0; x < width; x++) {
                pixels[x] = 255;
                pixels[(height - 1) * pitch + x] = 255;
            }
            for (int y = 0; y < height; y++) {
                pixels[y * pitch] = 255;
                pixels[y * pitch + width - 1] = 255;
            }
        }

        // Flip to screen
        Platform_Graphics_Flip();
        frame++;
        fps_counter++;

        // Print status every second
        if (time - last_fps_time >= 1000) {
            printf("\rFPS: %d | Mouse: (%d,%d) | Shift:%d Ctrl:%d Alt:%d    ",
                   fps_counter, mouse_x, mouse_y,
                   Platform_Key_ShiftDown(),
                   Platform_Key_CtrlDown(),
                   Platform_Key_AltDown());
            fflush(stdout);
            fps_counter = 0;
            last_fps_time = time;
        }
    }

    printf("\n\nDemo ended. Total frames: %u\n", frame);

    // Cleanup
    Platform_Graphics_Shutdown();
    Platform_Shutdown();

    return 0;
}
```

### Ensure all tests still pass
The test_graphics() function should still work. Add a simple input test:
```cpp
int test_input() {
    TEST("Input system");

    // Input init should succeed (auto-initialized)
    int result = Platform_Input_Init();
    if (result != 0) {
        FAIL("Input init failed");
    }

    // Update should not crash
    Platform_Input_Update();

    // Key queries should return false (no keys pressed)
    if (Platform_Key_IsPressed(KEY_CODE_SPACE)) {
        // This might be true if user is holding space, so just warn
        printf("(space pressed?) ");
    }

    // Mouse position should be valid
    int32_t mx, my;
    Platform_Mouse_GetPosition(&mx, &my);
    // Position can be anything, just verify it doesn't crash

    // Modifier queries should work
    bool shift = Platform_Key_ShiftDown();
    bool ctrl = Platform_Key_CtrlDown();
    bool alt = Platform_Key_AltDown();
    (void)shift; (void)ctrl; (void)alt;  // Suppress unused warnings

    Platform_Input_Shutdown();

    PASS();
    return 0;
}
```

Add to test sequence in run_tests():
```cpp
failures += test_input();
```

## Verification Command
```bash
cd /Users/ooartist/Src/Ronin_CnC && \
cmake --build build 2>&1 && \
./build/RedAlert --test 2>&1 | grep -q "All tests passed" && \
echo "VERIFY_SUCCESS"
```

## Implementation Steps
1. Update run_demo() with input visualization
2. Add test_input() function
3. Add test_input() call to run_tests()
4. Build the project
5. Run tests to verify they pass
6. Optionally run --demo to visually verify input

## Manual Testing
After verification passes, manually test:
1. `./build/RedAlert --demo`
2. Move mouse - crosshair should follow
3. Click buttons - indicators should light up
4. Hold Shift/Ctrl/Alt - indicators should light up
5. Double-click - border should flash
6. Press Escape or close window to exit

## Common Issues

### Mouse position off by factor of 2
- Window is 2x scaled, divide mouse coords by 2
- Or adjust rendering to use window coords

### Indicators not updating
- Ensure Platform_Input_Update() called each frame
- Check that poll_events feeds input system

### Double-click not detected
- Clicks must be within 500ms and 4 pixels
- May need to click quickly in same spot

## Completion Promise
When verification passes (tests pass), output:
<promise>TASK_04D_COMPLETE</promise>

## Escape Hatch
If stuck after 10 iterations:
- Document blocking issue in `ralph-tasks/blocked/04d.md`
- Output: <promise>TASK_04D_BLOCKED</promise>

## Max Iterations
10

## Phase 04 Complete
When this task completes, the input system is fully operational:
- Keyboard state tracking and event queue
- Mouse position, buttons, double-click
- Modifier key detection
- Visual demo proving functionality

Proceed to **Phase 05: Timing System**.
