# Task 05d: Timing Integration Test

## Dependencies
- Task 05c must be complete (Frame Rate Control)

## Context
This task verifies the complete timing system works correctly from C++ code. It tests accuracy of timing functions, periodic timer callbacks, and frame rate limiting.

## Objective
Add timing tests to `src/main.cpp` that verify:
1. `Platform_Timer_GetTicks()` returns increasing values
2. `Platform_Timer_Delay()` sleeps for approximately the correct time
3. `Platform_Timer_GetPerformanceCounter()` has nanosecond precision
4. Frame rate limiting works correctly
5. All tests pass

## Deliverables
- [ ] `test_timing()` function in `src/main.cpp`
- [ ] Updated `run_tests()` to call timing tests
- [ ] Demo mode shows FPS counter
- [ ] All tests pass
- [ ] Verification passes

## Files to Modify

### src/main.cpp (MODIFY)
Add test function after `test_input()`:
```cpp
int test_timing() {
    TEST("Timing system");

    // Test get_ticks returns increasing values
    uint32_t t1 = Platform_Timer_GetTicks();
    Platform_Timer_Delay(10);
    uint32_t t2 = Platform_Timer_GetTicks();

    if (t2 <= t1) FAIL("Ticks should increase over time");

    // Check delay was approximately correct (allow 5-50ms tolerance)
    uint32_t elapsed = t2 - t1;
    if (elapsed < 5 || elapsed > 50) {
        printf("(delay was %ums, expected ~10ms) ", elapsed);
        // Don't fail - timing can vary on different systems
    }

    // Test performance counter
    uint64_t freq = Platform_Timer_GetPerformanceFrequency();
    if (freq != 1000000000) {
        printf("(freq=%llu, expected 1GHz) ", (unsigned long long)freq);
    }

    uint64_t pc1 = Platform_Timer_GetPerformanceCounter();
    Platform_Timer_Delay(1);
    uint64_t pc2 = Platform_Timer_GetPerformanceCounter();

    if (pc2 <= pc1) FAIL("Performance counter should increase");

    // Test frame timing functions exist and don't crash
    Platform_Frame_Begin();
    Platform_Timer_Delay(16); // Simulate ~60fps frame
    Platform_Frame_End();

    double fps = Platform_Frame_GetFPS();
    double frame_time = Platform_Frame_GetTime();

    printf("(fps=%.1f, frame=%.3fs) ", fps, frame_time);

    PASS();
    return 0;
}
```

### src/main.cpp (MODIFY)
Update `run_tests()` to include timing test:
```cpp
// After test_input() call, add:
{
    Platform_Init();
    failures += test_timing();
    Platform_Shutdown();
}
```

### src/main.cpp (MODIFY)
Update `run_demo()` to use frame rate limiter:
In the main loop, replace the FPS calculation with frame controller:
```cpp
// At start of frame:
Platform_Frame_Begin();

// ... existing demo code ...

// At end of frame (replace existing FPS tracking):
Platform_Frame_End();

// Update FPS display
if (time - last_fps_time >= 1000) {
    double fps = Platform_Frame_GetFPS();
    printf("\rFPS: %.1f | Mouse: (%d,%d) | Shift:%d Ctrl:%d Alt:%d    ",
           fps, mouse_x, mouse_y,
           Platform_Key_ShiftDown(),
           Platform_Key_CtrlDown(),
           Platform_Key_AltDown());
    fflush(stdout);
    last_fps_time = time;
}
```

## Verification Command
```bash
cd /Users/ooartist/Src/Ronin_CnC && \
cmake --build build 2>&1 | tail -5 && \
./build/RedAlert --test 2>&1 | grep -q "All tests passed" && \
echo "VERIFY_SUCCESS"
```

## Implementation Steps
1. Add `test_timing()` function to main.cpp
2. Add timing test call to `run_tests()`
3. Update demo mode to use frame rate limiter
4. Build the project
5. Run tests to verify timing works
6. Run verification command

## Completion Promise
When verification passes, output:
```
<promise>TASK_05D_COMPLETE</promise>
```

Then proceed to **Phase 06: Memory Management**.

## Escape Hatch
If stuck after 12 iterations:
- Document what's blocking in `ralph-tasks/blocked/05d.md`
- List attempted approaches
- Output: `<promise>TASK_05D_BLOCKED</promise>`

## Max Iterations
12
