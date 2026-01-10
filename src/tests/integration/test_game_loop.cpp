// src/tests/integration/test_game_loop.cpp
// Game Loop Integration Tests
// Task 18c - Integration Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "platform.h"
#include <cstring>

//=============================================================================
// Basic Game Loop Tests
//=============================================================================

TEST_CASE(GameLoop_BasicFrameLoop, "GameLoop") {
    Platform_Init();
    Platform_Graphics_Init();

    // Run 60 frames
    for (int frame = 0; frame < 60; frame++) {
        Platform_PollEvents();
        Platform_Input_Update();

        // Check for quit
        if (Platform_Key_IsPressed(KEY_CODE_ESCAPE)) {
            break;
        }

        // Simple render
        uint8_t* buffer;
        int32_t w, h, p;
        Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);

        uint8_t color = frame & 0xFF;
        for (int y = 0; y < h; y++) {
            memset(buffer + y * p, color, w);
        }

        Platform_Graphics_Flip();
        Platform_Timer_Delay(16);  // ~60fps
    }

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

TEST_CASE(GameLoop_TimingConsistency, "GameLoop") {
    Platform_Init();
    Platform_Graphics_Init();

    const int FRAMES = 30;
    uint32_t frame_times[FRAMES];

    uint32_t start = Platform_Timer_GetTicks();
    for (int frame = 0; frame < FRAMES; frame++) {
        uint32_t frame_start = Platform_Timer_GetTicks();

        Platform_PollEvents();

        uint8_t* buffer;
        int32_t w, h, p;
        Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);
        memset(buffer, frame * 8, h * p);
        Platform_Graphics_Flip();

        // Target 16ms per frame
        uint32_t elapsed = Platform_Timer_GetTicks() - frame_start;
        if (elapsed < 16) {
            Platform_Timer_Delay(16 - elapsed);
        }

        frame_times[frame] = Platform_Timer_GetTicks() - frame_start;
    }
    uint32_t total_time = Platform_Timer_GetTicks() - start;

    // Should take approximately 0.5 seconds (30 frames * 16ms)
    TEST_ASSERT_GE(total_time, 400u);   // At least 400ms
    TEST_ASSERT_LE(total_time, 700u);   // At most 700ms

    // Calculate average frame time
    uint32_t total_frame_time = 0;
    for (int i = 0; i < FRAMES; i++) {
        total_frame_time += frame_times[i];
    }
    uint32_t avg_frame_time = total_frame_time / FRAMES;

    // Average should be close to 16ms
    TEST_ASSERT_GE(avg_frame_time, 14u);
    TEST_ASSERT_LE(avg_frame_time, 22u);

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Input in Game Loop Tests
//=============================================================================

TEST_CASE(GameLoop_InputHandling, "GameLoop") {
    Platform_Init();
    Platform_Graphics_Init();
    Platform_Input_Init();

    int frames_run = 0;

    for (int frame = 0; frame < 30; frame++) {
        Platform_PollEvents();
        Platform_Input_Update();

        // Check mouse position
        int32_t mx, my;
        Platform_Mouse_GetPosition(&mx, &my);

        // Simple render
        uint8_t* buffer;
        int32_t w, h, p;
        Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);
        memset(buffer, 50, h * p);

        // Draw cursor indicator
        if (mx >= 0 && mx < w && my >= 0 && my < h) {
            buffer[my * p + mx] = 255;
        }

        Platform_Graphics_Flip();
        Platform_Timer_Delay(16);
        frames_run++;
    }

    TEST_ASSERT_EQ(frames_run, 30);

    Platform_Input_Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Full System Game Loop Tests
//=============================================================================

TEST_CASE(GameLoop_AllSystems, "GameLoop") {
    // Initialize all systems
    Platform_Init();
    Platform_Graphics_Init();

    AudioConfig audio = {22050, 2, 16, 1024};
    Platform_Audio_Init(&audio);

    Platform_Assets_Init();

    // Run game loop
    for (int frame = 0; frame < 30; frame++) {
        // Input
        Platform_PollEvents();
        Platform_Input_Update();

        // Render
        uint8_t* buffer;
        int32_t w, h, p;
        Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);
        memset(buffer, 30 + frame, h * p);
        Platform_Graphics_Flip();

        // Frame timing
        Platform_Timer_Delay(16);
    }

    // Clean shutdown
    Platform_Audio_Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();

    // Verify clean state
    TEST_ASSERT(!Platform_IsInitialized());
}

//=============================================================================
// State Transition Tests
//=============================================================================

TEST_CASE(GameLoop_StateTransition, "GameLoop") {
    Platform_Init();
    Platform_Graphics_Init();

    enum GameState { STATE_MENU, STATE_GAME, STATE_PAUSED };
    GameState state = STATE_MENU;

    for (int frame = 0; frame < 60; frame++) {
        Platform_PollEvents();
        Platform_Input_Update();

        // State transitions
        if (frame == 20) {
            state = STATE_GAME;
        } else if (frame == 40) {
            state = STATE_PAUSED;
        }

        // Render based on state
        uint8_t* buffer;
        int32_t w, h, p;
        Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);

        uint8_t color;
        switch (state) {
            case STATE_MENU:   color = 50;  break;
            case STATE_GAME:   color = 100; break;
            case STATE_PAUSED: color = 150; break;
            default:           color = 0;   break;
        }

        memset(buffer, color, h * p);
        Platform_Graphics_Flip();
        Platform_Timer_Delay(16);
    }

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Frame Rate Control Tests
//=============================================================================

TEST_CASE(GameLoop_FrameRateControl, "GameLoop") {
    Platform_Init();
    Platform_Graphics_Init();

    Platform_Frame_SetTargetFPS(60);

    uint32_t start = Platform_Timer_GetTicks();

    for (int frame = 0; frame < 30; frame++) {
        Platform_Frame_Begin();

        Platform_PollEvents();

        uint8_t* buffer;
        int32_t w, h, p;
        Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);
        memset(buffer, frame * 8, h * p);
        Platform_Graphics_Flip();

        Platform_Frame_End();
    }

    uint32_t elapsed = Platform_Timer_GetTicks() - start;

    // At 60 FPS, 30 frames should take ~500ms
    TEST_ASSERT_GE(elapsed, 400u);
    TEST_ASSERT_LE(elapsed, 700u);

    // Check FPS reporting
    double fps = Platform_Frame_GetFPS();
    TEST_ASSERT_GE(fps, 40.0);
    TEST_ASSERT_LE(fps, 80.0);

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Quit Request Tests
//=============================================================================

TEST_CASE(GameLoop_QuitRequest, "GameLoop") {
    Platform_Init();

    // Clear any existing quit request
    Platform_ClearQuitRequest();
    TEST_ASSERT_EQ(Platform_ShouldQuit(), 0);

    // Request quit
    Platform_RequestQuit();
    TEST_ASSERT_EQ(Platform_ShouldQuit(), 1);

    // Clear quit
    Platform_ClearQuitRequest();
    TEST_ASSERT_EQ(Platform_ShouldQuit(), 0);

    Platform_Shutdown();
}

//=============================================================================
// App State Tests
//=============================================================================

TEST_CASE(GameLoop_AppState, "GameLoop") {
    Platform_Init();
    Platform_Graphics_Init();

    // App should be active when initialized
    int state = Platform_GetAppState();
    TEST_ASSERT_EQ(state, 0);  // 0 = Active

    TEST_ASSERT_EQ(Platform_IsAppActive(), 1);

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Performance Counter Tests
//=============================================================================

TEST_CASE(GameLoop_PerformanceTimer, "GameLoop") {
    Platform_Init();

    uint64_t freq = Platform_Timer_GetPerformanceFrequency();
    TEST_ASSERT_GT(freq, 0u);

    uint64_t t1 = Platform_Timer_GetPerformanceCounter();
    Platform_Timer_Delay(50);
    uint64_t t2 = Platform_Timer_GetPerformanceCounter();

    TEST_ASSERT_GT(t2, t1);

    // Calculate elapsed time
    double elapsed_sec = static_cast<double>(t2 - t1) / static_cast<double>(freq);
    TEST_ASSERT_GE(elapsed_sec, 0.040);  // At least 40ms
    TEST_ASSERT_LE(elapsed_sec, 0.100);  // At most 100ms

    Platform_Shutdown();
}

//=============================================================================
// Frame Counting Tests
//=============================================================================

TEST_CASE(GameLoop_FrameCount, "GameLoop") {
    Platform_Init();
    Platform_Graphics_Init();

    uint64_t initial_count = Platform_GetFrameCount();

    for (int i = 0; i < 10; i++) {
        Platform_FrameStart();
        Platform_Graphics_Flip();
        Platform_FrameEnd();
    }

    uint64_t final_count = Platform_GetFrameCount();
    TEST_ASSERT_EQ(final_count, initial_count + 10);

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}
