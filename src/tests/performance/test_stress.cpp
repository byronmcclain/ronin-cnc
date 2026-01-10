// src/tests/performance/test_stress.cpp
// Stress Performance Tests
// Task 18f - Performance Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "perf_utils.h"
#include <cstring>
#include <cmath>

//=============================================================================
// Stress Tests - Many Sprites
//=============================================================================

TEST_WITH_FIXTURE(GraphicsFixture, Perf_Stress_ManySprites, "Performance") {
    if (!fixture.IsInitialized()) {
        TEST_SKIP("Graphics not initialized");
    }

    const int SPRITE_COUNT = 100;
    const int SPRITE_SIZE = 16;
    const int FRAMES = 60;

    struct Sprite {
        float x, y;
        float vx, vy;
    };

    Sprite sprites[SPRITE_COUNT];
    int width = fixture.GetWidth();
    int height = fixture.GetHeight();

    // Initialize sprites with random positions/velocities
    for (int i = 0; i < SPRITE_COUNT; i++) {
        sprites[i].x = static_cast<float>((i * 37) % (width - SPRITE_SIZE));
        sprites[i].y = static_cast<float>((i * 53) % (height - SPRITE_SIZE));
        sprites[i].vx = ((i % 7) - 3) * 1.0f;
        sprites[i].vy = ((i % 5) - 2) * 1.0f;
    }

    FrameRateTracker tracker;

    for (int frame = 0; frame < FRAMES; frame++) {
        uint32_t frame_start = Platform_Timer_GetTicks();

        uint8_t* buffer = fixture.GetBackBuffer();
        int pitch = fixture.GetPitch();

        // Clear
        for (int y = 0; y < height; y++) {
            memset(buffer + y * pitch, 0, width);
        }

        // Update and draw all sprites
        for (int i = 0; i < SPRITE_COUNT; i++) {
            // Update position
            sprites[i].x += sprites[i].vx;
            sprites[i].y += sprites[i].vy;

            // Bounce
            if (sprites[i].x < 0 || sprites[i].x + SPRITE_SIZE > width) {
                sprites[i].vx = -sprites[i].vx;
                sprites[i].x += sprites[i].vx;
            }
            if (sprites[i].y < 0 || sprites[i].y + SPRITE_SIZE > height) {
                sprites[i].vy = -sprites[i].vy;
                sprites[i].y += sprites[i].vy;
            }

            // Draw sprite (filled square with border)
            int sx = static_cast<int>(sprites[i].x);
            int sy = static_cast<int>(sprites[i].y);
            uint8_t color = static_cast<uint8_t>(100 + (i % 100));

            for (int dy = 0; dy < SPRITE_SIZE; dy++) {
                int py = sy + dy;
                if (py < 0 || py >= height) continue;

                for (int dx = 0; dx < SPRITE_SIZE; dx++) {
                    int px = sx + dx;
                    if (px < 0 || px >= width) continue;

                    if (dx == 0 || dx == SPRITE_SIZE - 1 ||
                        dy == 0 || dy == SPRITE_SIZE - 1) {
                        buffer[py * pitch + px] = 255;  // Border
                    } else {
                        buffer[py * pitch + px] = color;  // Fill
                    }
                }
            }
        }

        fixture.RenderFrame();

        uint32_t frame_time = Platform_Timer_GetTicks() - frame_start;
        tracker.RecordFrame(frame_time);
    }

    float avg_fps = tracker.GetAverageFPS();

    char msg[128];
    snprintf(msg, sizeof(msg), "100 sprites: avg=%.1f fps, min=%.1f fps",
             avg_fps, tracker.GetMinFPS());
    Platform_Log(LOG_LEVEL_INFO, msg);

    // Should maintain playable frame rate
    TEST_ASSERT_GT(avg_fps, 30.0f);
}

TEST_WITH_FIXTURE(GraphicsFixture, Perf_Stress_500Sprites, "Performance") {
    if (!fixture.IsInitialized()) {
        TEST_SKIP("Graphics not initialized");
    }

    const int SPRITE_COUNT = 500;
    const int SPRITE_SIZE = 8;
    const int FRAMES = 30;

    struct Sprite {
        int16_t x, y;
        int8_t vx, vy;
        uint8_t color;
    };

    Sprite sprites[SPRITE_COUNT];
    int width = fixture.GetWidth();
    int height = fixture.GetHeight();

    // Initialize sprites
    for (int i = 0; i < SPRITE_COUNT; i++) {
        sprites[i].x = (i * 37) % (width - SPRITE_SIZE);
        sprites[i].y = (i * 53) % (height - SPRITE_SIZE);
        sprites[i].vx = ((i % 7) - 3);
        sprites[i].vy = ((i % 5) - 2);
        sprites[i].color = static_cast<uint8_t>(50 + (i % 200));
    }

    FrameRateTracker tracker;

    for (int frame = 0; frame < FRAMES; frame++) {
        uint32_t frame_start = Platform_Timer_GetTicks();

        uint8_t* buffer = fixture.GetBackBuffer();
        int pitch = fixture.GetPitch();

        // Clear
        for (int y = 0; y < height; y++) {
            memset(buffer + y * pitch, 0, width);
        }

        // Update and draw all sprites
        for (int i = 0; i < SPRITE_COUNT; i++) {
            // Update position
            sprites[i].x += sprites[i].vx;
            sprites[i].y += sprites[i].vy;

            // Bounce
            if (sprites[i].x < 0 || sprites[i].x + SPRITE_SIZE > width) {
                sprites[i].vx = -sprites[i].vx;
            }
            if (sprites[i].y < 0 || sprites[i].y + SPRITE_SIZE > height) {
                sprites[i].vy = -sprites[i].vy;
            }

            // Draw sprite (simple filled square)
            int sx = sprites[i].x;
            int sy = sprites[i].y;
            uint8_t color = sprites[i].color;

            for (int dy = 0; dy < SPRITE_SIZE && sy + dy < height; dy++) {
                int py = sy + dy;
                if (py < 0) continue;

                for (int dx = 0; dx < SPRITE_SIZE && sx + dx < width; dx++) {
                    int px = sx + dx;
                    if (px < 0) continue;
                    buffer[py * pitch + px] = color;
                }
            }
        }

        fixture.RenderFrame();

        uint32_t frame_time = Platform_Timer_GetTicks() - frame_start;
        tracker.RecordFrame(frame_time);
    }

    float avg_fps = tracker.GetAverageFPS();

    char msg[128];
    snprintf(msg, sizeof(msg), "500 sprites: avg=%.1f fps, min=%.1f fps",
             avg_fps, tracker.GetMinFPS());
    Platform_Log(LOG_LEVEL_INFO, msg);

    // Should still be reasonable (may drop below 30 but should not be terrible)
    TEST_ASSERT_GT(avg_fps, 15.0f);
}

TEST_WITH_FIXTURE(GraphicsFixture, Perf_Stress_FullScreenBlit, "Performance") {
    if (!fixture.IsInitialized()) {
        TEST_SKIP("Graphics not initialized");
    }

    const int FRAMES = 100;
    int width = fixture.GetWidth();
    int height = fixture.GetHeight();

    // Create a source buffer
    uint8_t* source = new uint8_t[width * height];
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            source[y * width + x] = static_cast<uint8_t>((x ^ y) & 0xFF);
        }
    }

    FrameRateTracker tracker;

    for (int frame = 0; frame < FRAMES; frame++) {
        uint32_t frame_start = Platform_Timer_GetTicks();

        uint8_t* buffer = fixture.GetBackBuffer();
        int pitch = fixture.GetPitch();

        // Full screen copy with slight modification
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                buffer[y * pitch + x] = source[y * width + x] + static_cast<uint8_t>(frame);
            }
        }

        fixture.RenderFrame();

        uint32_t frame_time = Platform_Timer_GetTicks() - frame_start;
        tracker.RecordFrame(frame_time);
    }

    delete[] source;

    float avg_fps = tracker.GetAverageFPS();

    char msg[128];
    snprintf(msg, sizeof(msg), "Full screen blit: avg=%.1f fps", avg_fps);
    Platform_Log(LOG_LEVEL_INFO, msg);

    TEST_ASSERT_GT(avg_fps, 30.0f);
}

TEST_WITH_FIXTURE(GraphicsFixture, Perf_Stress_Particles, "Performance") {
    if (!fixture.IsInitialized()) {
        TEST_SKIP("Graphics not initialized");
    }

    const int PARTICLE_COUNT = 1000;
    const int FRAMES = 60;

    struct Particle {
        float x, y;
        float vx, vy;
        uint8_t color;
        uint8_t life;
    };

    Particle particles[PARTICLE_COUNT];
    int width = fixture.GetWidth();
    int height = fixture.GetHeight();

    // Initialize particles
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        particles[i].x = static_cast<float>(width / 2);
        particles[i].y = static_cast<float>(height / 2);
        float angle = (i * 6.28318f) / PARTICLE_COUNT;
        float speed = 2.0f + (i % 5);
        particles[i].vx = std::cos(angle) * speed;
        particles[i].vy = std::sin(angle) * speed;
        particles[i].color = static_cast<uint8_t>(200 + (i % 50));
        particles[i].life = 60 + (i % 60);
    }

    FrameRateTracker tracker;

    for (int frame = 0; frame < FRAMES; frame++) {
        uint32_t frame_start = Platform_Timer_GetTicks();

        uint8_t* buffer = fixture.GetBackBuffer();
        int pitch = fixture.GetPitch();

        // Clear with fade effect (darken existing pixels)
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                buffer[y * pitch + x] = buffer[y * pitch + x] >> 1;
            }
        }

        // Update and draw particles
        for (int i = 0; i < PARTICLE_COUNT; i++) {
            if (particles[i].life == 0) continue;

            // Update
            particles[i].x += particles[i].vx;
            particles[i].y += particles[i].vy;
            particles[i].vy += 0.1f;  // Gravity
            particles[i].life--;

            // Respawn if dead
            if (particles[i].life == 0) {
                particles[i].x = static_cast<float>(width / 2);
                particles[i].y = static_cast<float>(height / 2);
                float angle = (i * 6.28318f + frame * 0.1f) / PARTICLE_COUNT;
                float speed = 2.0f + (i % 5);
                particles[i].vx = std::cos(angle) * speed;
                particles[i].vy = std::sin(angle) * speed;
                particles[i].life = 60 + (i % 60);
            }

            // Draw (single pixel)
            int px = static_cast<int>(particles[i].x);
            int py = static_cast<int>(particles[i].y);
            if (px >= 0 && px < width && py >= 0 && py < height) {
                buffer[py * pitch + px] = particles[i].color;
            }
        }

        fixture.RenderFrame();

        uint32_t frame_time = Platform_Timer_GetTicks() - frame_start;
        tracker.RecordFrame(frame_time);
    }

    float avg_fps = tracker.GetAverageFPS();

    char msg[128];
    snprintf(msg, sizeof(msg), "1000 particles: avg=%.1f fps", avg_fps);
    Platform_Log(LOG_LEVEL_INFO, msg);

    TEST_ASSERT_GT(avg_fps, 30.0f);
}

//=============================================================================
// CPU Stress Tests
//=============================================================================

TEST_CASE(Perf_Stress_MathOperations, "Performance") {
    const int ITERATIONS = 100000;

    PerfTimer timer;
    timer.Start();

    float result = 0.0f;
    for (int i = 0; i < ITERATIONS; i++) {
        result += std::sin(static_cast<float>(i) * 0.001f);
        result += std::cos(static_cast<float>(i) * 0.001f);
        result += std::sqrt(static_cast<float>(i + 1));
    }

    timer.Stop();

    char msg[128];
    snprintf(msg, sizeof(msg),
             "Math ops: %d iterations in %u ms (result=%.2f)",
             ITERATIONS, timer.ElapsedMs(), result);
    Platform_Log(LOG_LEVEL_INFO, msg);

    // Should complete quickly (< 1 second)
    TEST_ASSERT_LT(timer.ElapsedMs(), 1000u);
}

TEST_CASE(Perf_Stress_ArrayAccess, "Performance") {
    const int ARRAY_SIZE = 10000;
    const int ITERATIONS = 1000;

    int* array = new int[ARRAY_SIZE];
    for (int i = 0; i < ARRAY_SIZE; i++) {
        array[i] = i;
    }

    PerfTimer timer;
    timer.Start();

    int sum = 0;
    for (int iter = 0; iter < ITERATIONS; iter++) {
        for (int i = 0; i < ARRAY_SIZE; i++) {
            sum += array[i];
        }
    }

    timer.Stop();

    delete[] array;

    char msg[128];
    snprintf(msg, sizeof(msg),
             "Array access: %d iterations in %u ms (sum=%d)",
             ITERATIONS * ARRAY_SIZE, timer.ElapsedMs(), sum);
    Platform_Log(LOG_LEVEL_INFO, msg);

    // Should be very fast
    TEST_ASSERT_LT(timer.ElapsedMs(), 500u);
}
