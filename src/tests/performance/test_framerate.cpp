// src/tests/performance/test_framerate.cpp
// Frame Rate Performance Tests
// Task 18f - Performance Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "perf_utils.h"
#include <cstring>

//=============================================================================
// Frame Rate Tests
//=============================================================================

TEST_WITH_FIXTURE(GraphicsFixture, Perf_FrameRate_Baseline, "Performance") {
    if (!fixture.IsInitialized()) {
        TEST_SKIP("Graphics not initialized");
    }

    FrameRateTracker tracker;
    const int FRAMES = 100;

    for (int frame = 0; frame < FRAMES; frame++) {
        uint32_t frame_start = Platform_Timer_GetTicks();

        // Minimal render - just clear
        uint8_t* buffer = fixture.GetBackBuffer();
        int pitch = fixture.GetPitch();
        int width = fixture.GetWidth();
        int height = fixture.GetHeight();

        for (int y = 0; y < height; y++) {
            memset(buffer + y * pitch, 0, width);
        }

        fixture.RenderFrame();

        uint32_t frame_time = Platform_Timer_GetTicks() - frame_start;
        tracker.RecordFrame(frame_time);
    }

    float avg_fps = tracker.GetAverageFPS();
    float min_fps = tracker.GetMinFPS();

    char msg[128];
    snprintf(msg, sizeof(msg), "Baseline FPS: avg=%.1f, min=%.1f, max=%.1f",
             avg_fps, min_fps, tracker.GetMaxFPS());
    Platform_Log(LOG_LEVEL_INFO, msg);

    // Baseline should be very fast
    TEST_ASSERT_GT(avg_fps, 60.0f);
}

TEST_WITH_FIXTURE(GraphicsFixture, Perf_FrameRate_FilledScreen, "Performance") {
    if (!fixture.IsInitialized()) {
        TEST_SKIP("Graphics not initialized");
    }

    FrameRateTracker tracker;
    const int FRAMES = 100;

    int width = fixture.GetWidth();
    int height = fixture.GetHeight();

    for (int frame = 0; frame < FRAMES; frame++) {
        uint32_t frame_start = Platform_Timer_GetTicks();

        uint8_t* buffer = fixture.GetBackBuffer();
        int pitch = fixture.GetPitch();

        // Fill entire screen with pattern
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                buffer[y * pitch + x] = static_cast<uint8_t>((x + y + frame) & 0xFF);
            }
        }

        fixture.RenderFrame();

        uint32_t frame_time = Platform_Timer_GetTicks() - frame_start;
        tracker.RecordFrame(frame_time);
    }

    float avg_fps = tracker.GetAverageFPS();

    char msg[128];
    snprintf(msg, sizeof(msg), "Filled screen FPS: avg=%.1f, min=%.1f",
             avg_fps, tracker.GetMinFPS());
    Platform_Log(LOG_LEVEL_INFO, msg);

    // Should maintain reasonable fps even with full screen updates
    TEST_ASSERT_GT(avg_fps, 30.0f);
}

TEST_WITH_FIXTURE(GraphicsFixture, Perf_FrameRate_PaletteAnimation, "Performance") {
    if (!fixture.IsInitialized()) {
        TEST_SKIP("Graphics not initialized");
    }

    FrameRateTracker tracker;
    const int FRAMES = 100;

    // Set up static screen content
    uint8_t* buffer = fixture.GetBackBuffer();
    int pitch = fixture.GetPitch();
    int width = fixture.GetWidth();
    int height = fixture.GetHeight();

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            buffer[y * pitch + x] = x & 0xFF;
        }
    }

    // Base palette
    PaletteEntry base_palette[256];
    for (int i = 0; i < 256; i++) {
        base_palette[i].r = static_cast<uint8_t>(i);
        base_palette[i].g = static_cast<uint8_t>((i + 85) & 0xFF);
        base_palette[i].b = static_cast<uint8_t>((i + 170) & 0xFF);
    }

    for (int frame = 0; frame < FRAMES; frame++) {
        uint32_t frame_start = Platform_Timer_GetTicks();

        // Rotate palette each frame
        PaletteEntry rotated[256];
        for (int i = 0; i < 256; i++) {
            rotated[i] = base_palette[(i + frame * 4) % 256];
        }
        Platform_Graphics_SetPalette(rotated, 0, 256);

        fixture.RenderFrame();

        uint32_t frame_time = Platform_Timer_GetTicks() - frame_start;
        tracker.RecordFrame(frame_time);
    }

    float avg_fps = tracker.GetAverageFPS();

    char msg[128];
    snprintf(msg, sizeof(msg), "Palette animation FPS: avg=%.1f", avg_fps);
    Platform_Log(LOG_LEVEL_INFO, msg);

    // Palette operations should be fast
    TEST_ASSERT_GT(avg_fps, 50.0f);
}

TEST_WITH_FIXTURE(GraphicsFixture, Perf_FrameRate_FrameTiming, "Performance") {
    if (!fixture.IsInitialized()) {
        TEST_SKIP("Graphics not initialized");
    }

    // Test frame timing consistency at 60fps
    const int FRAMES = 120;  // 2 seconds
    const uint32_t TARGET_FRAME_TIME = 16;  // ~60fps

    uint32_t frame_times[FRAMES];
    uint32_t start = Platform_Timer_GetTicks();

    for (int frame = 0; frame < FRAMES; frame++) {
        uint32_t frame_start = Platform_Timer_GetTicks();

        // Simple render
        uint8_t* buffer = fixture.GetBackBuffer();
        int pitch = fixture.GetPitch();
        int width = fixture.GetWidth();
        int height = fixture.GetHeight();

        for (int y = 0; y < height; y++) {
            memset(buffer + y * pitch, static_cast<uint8_t>(frame & 0xFF), width);
        }

        fixture.RenderFrame();

        // Frame rate limiting
        uint32_t render_time = Platform_Timer_GetTicks() - frame_start;
        if (render_time < TARGET_FRAME_TIME) {
            Platform_Timer_Delay(TARGET_FRAME_TIME - render_time);
        }

        frame_times[frame] = Platform_Timer_GetTicks() - frame_start;
    }

    uint32_t total_time = Platform_Timer_GetTicks() - start;
    float actual_fps = (FRAMES * 1000.0f) / total_time;

    // Calculate frame time variance
    uint32_t min_time = UINT32_MAX, max_time = 0;
    uint32_t total_frame_time = 0;
    for (int i = 0; i < FRAMES; i++) {
        if (frame_times[i] < min_time) min_time = frame_times[i];
        if (frame_times[i] > max_time) max_time = frame_times[i];
        total_frame_time += frame_times[i];
    }

    char msg[128];
    snprintf(msg, sizeof(msg),
             "Frame timing: %.1f fps, frame time %u-%u ms (target %u)",
             actual_fps, min_time, max_time, TARGET_FRAME_TIME);
    Platform_Log(LOG_LEVEL_INFO, msg);

    // Should be close to 60fps (allow variance for system load)
    TEST_ASSERT_GT(actual_fps, 50.0f);
    TEST_ASSERT_LT(actual_fps, 70.0f);
}

TEST_WITH_FIXTURE(GraphicsFixture, Perf_FrameRate_30fps, "Performance") {
    if (!fixture.IsInitialized()) {
        TEST_SKIP("Graphics not initialized");
    }

    // Test frame timing at 30fps (original game speed)
    const int FRAMES = 60;  // 2 seconds
    const uint32_t TARGET_FRAME_TIME = 33;  // ~30fps

    FrameRateTracker tracker;
    uint32_t start = Platform_Timer_GetTicks();

    for (int frame = 0; frame < FRAMES; frame++) {
        uint32_t frame_start = Platform_Timer_GetTicks();

        // Simple render
        uint8_t* buffer = fixture.GetBackBuffer();
        int pitch = fixture.GetPitch();
        int width = fixture.GetWidth();
        int height = fixture.GetHeight();

        for (int y = 0; y < height; y++) {
            memset(buffer + y * pitch, static_cast<uint8_t>(frame & 0xFF), width);
        }

        fixture.RenderFrame();

        // Frame rate limiting
        uint32_t render_time = Platform_Timer_GetTicks() - frame_start;
        if (render_time < TARGET_FRAME_TIME) {
            Platform_Timer_Delay(TARGET_FRAME_TIME - render_time);
        }

        tracker.RecordFrame(Platform_Timer_GetTicks() - frame_start);
    }

    uint32_t total_time = Platform_Timer_GetTicks() - start;
    float actual_fps = (FRAMES * 1000.0f) / total_time;

    char msg[128];
    snprintf(msg, sizeof(msg), "30fps test: actual=%.1f fps", actual_fps);
    Platform_Log(LOG_LEVEL_INFO, msg);

    // Should be close to 30fps
    TEST_ASSERT_GT(actual_fps, 28.0f);
    TEST_ASSERT_LT(actual_fps, 32.0f);
}
