// src/tests/visual/test_animation.cpp
// Animation Visual Tests
// Task 18d - Visual Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "screenshot_utils.h"
#include "platform.h"
#include <cstring>
#include <cmath>

//=============================================================================
// Animation Tests
//=============================================================================

TEST_WITH_FIXTURE(GraphicsFixture, Visual_Animation_Smoothness, "Visual") {
    if (!fixture.IsInitialized()) {
        TEST_SKIP("Graphics not initialized");
    }

    int width = fixture.GetWidth();
    int height = fixture.GetHeight();

    // Animate a moving box
    const int FRAMES = 60;  // 1 second at 60fps
    const int BOX_SIZE = 50;

    for (int frame = 0; frame < FRAMES; frame++) {
        uint8_t* buffer = fixture.GetBackBuffer();
        int pitch = fixture.GetPitch();

        // Clear
        for (int y = 0; y < height; y++) {
            memset(buffer + y * pitch, 32, width);
        }

        // Calculate box position (smooth sine motion)
        float t = static_cast<float>(frame) / 60.0f;  // Time in seconds
        int box_x = (width - BOX_SIZE) / 2 +
                    static_cast<int>(200 * std::sin(t * 3.14159f * 2));
        int box_y = (height - BOX_SIZE) / 2 +
                    static_cast<int>(100 * std::sin(t * 3.14159f * 4));

        // Clamp to screen bounds
        if (box_x < 0) box_x = 0;
        if (box_y < 0) box_y = 0;
        if (box_x + BOX_SIZE > width) box_x = width - BOX_SIZE;
        if (box_y + BOX_SIZE > height) box_y = height - BOX_SIZE;

        // Draw box
        for (int y = box_y; y < box_y + BOX_SIZE && y < height; y++) {
            for (int x = box_x; x < box_x + BOX_SIZE && x < width; x++) {
                buffer[y * pitch + x] = 200;
            }
        }

        fixture.RenderFrame();
        Platform_Timer_Delay(16);  // ~60fps
    }

    // Final frame screenshot
    Screenshot_Capture("visual_animation_smoothness.bmp");
}

TEST_WITH_FIXTURE(GraphicsFixture, Visual_Animation_ColorCycle, "Visual") {
    if (!fixture.IsInitialized()) {
        TEST_SKIP("Graphics not initialized");
    }

    // Animate palette cycling
    const int FRAMES = 64;  // About 1 second

    // Create a base palette
    PaletteEntry base_palette[256];
    for (int i = 0; i < 256; i++) {
        base_palette[i].r = static_cast<uint8_t>(i);
        base_palette[i].g = static_cast<uint8_t>(i);
        base_palette[i].b = static_cast<uint8_t>(i);
    }

    // Draw gradient once
    uint8_t* buffer = fixture.GetBackBuffer();
    int pitch = fixture.GetPitch();
    int width = fixture.GetWidth();
    int height = fixture.GetHeight();

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            buffer[y * pitch + x] = x & 0xFF;
        }
    }

    for (int frame = 0; frame < FRAMES; frame++) {
        // Rotate palette
        PaletteEntry rotated[256];
        for (int i = 0; i < 256; i++) {
            rotated[i] = base_palette[(i + frame * 4) % 256];
        }
        Platform_Graphics_SetPalette(rotated, 0, 256);

        fixture.RenderFrame();
        Platform_Timer_Delay(16);
    }

    Screenshot_Capture("visual_animation_colorcycle.bmp");
}

TEST_WITH_FIXTURE(GraphicsFixture, Visual_Animation_FrameRate, "Visual") {
    if (!fixture.IsInitialized()) {
        TEST_SKIP("Graphics not initialized");
    }

    int width = fixture.GetWidth();
    int height = fixture.GetHeight();

    const int TEST_FRAMES = 120;  // 2 seconds at 60fps
    uint32_t start = Platform_Timer_GetTicks();

    for (int frame = 0; frame < TEST_FRAMES; frame++) {
        uint8_t* buffer = fixture.GetBackBuffer();
        int pitch = fixture.GetPitch();

        // Simple render - color changes with frame
        uint8_t color = static_cast<uint8_t>((frame * 2) & 0xFF);
        for (int y = 0; y < height; y++) {
            memset(buffer + y * pitch, color, width);
        }

        fixture.RenderFrame();
        Platform_Timer_Delay(16);
    }

    uint32_t elapsed = Platform_Timer_GetTicks() - start;
    float actual_fps = (TEST_FRAMES * 1000.0f) / static_cast<float>(elapsed);

    // Log result (using char buffer since Platform_Log takes const char*)
    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg),
             "Animation test: %d frames in %u ms = %.1f fps",
             TEST_FRAMES, elapsed, actual_fps);
    Platform_Log(LOG_LEVEL_INFO, log_msg);

    // Should be reasonable fps (allow variance for system load)
    TEST_ASSERT_GT(actual_fps, 30.0f);
}

//=============================================================================
// Bouncing Ball Animation
//=============================================================================

TEST_WITH_FIXTURE(GraphicsFixture, Visual_Animation_BouncingBall, "Visual") {
    if (!fixture.IsInitialized()) {
        TEST_SKIP("Graphics not initialized");
    }

    int width = fixture.GetWidth();
    int height = fixture.GetHeight();

    const int FRAMES = 120;
    const int BALL_SIZE = 30;

    float ball_x = static_cast<float>(width / 4);
    float ball_y = static_cast<float>(height / 4);
    float vel_x = 5.0f;
    float vel_y = 3.0f;

    for (int frame = 0; frame < FRAMES; frame++) {
        uint8_t* buffer = fixture.GetBackBuffer();
        int pitch = fixture.GetPitch();

        // Clear
        for (int y = 0; y < height; y++) {
            memset(buffer + y * pitch, 0, width);
        }

        // Update ball position
        ball_x += vel_x;
        ball_y += vel_y;

        // Bounce off walls
        if (ball_x < 0 || ball_x + BALL_SIZE > width) {
            vel_x = -vel_x;
            ball_x += vel_x;
        }
        if (ball_y < 0 || ball_y + BALL_SIZE > height) {
            vel_y = -vel_y;
            ball_y += vel_y;
        }

        // Draw ball (filled circle)
        int cx = static_cast<int>(ball_x) + BALL_SIZE / 2;
        int cy = static_cast<int>(ball_y) + BALL_SIZE / 2;
        int radius = BALL_SIZE / 2;

        for (int y = cy - radius; y <= cy + radius; y++) {
            if (y < 0 || y >= height) continue;
            for (int x = cx - radius; x <= cx + radius; x++) {
                if (x < 0 || x >= width) continue;

                int dx = x - cx;
                int dy = y - cy;
                if (dx * dx + dy * dy <= radius * radius) {
                    buffer[y * pitch + x] = 200;
                }
            }
        }

        fixture.RenderFrame();
        Platform_Timer_Delay(16);
    }

    Screenshot_Capture("visual_animation_bouncing.bmp");
}

//=============================================================================
// Rotating Line Animation
//=============================================================================

TEST_WITH_FIXTURE(GraphicsFixture, Visual_Animation_RotatingLine, "Visual") {
    if (!fixture.IsInitialized()) {
        TEST_SKIP("Graphics not initialized");
    }

    int width = fixture.GetWidth();
    int height = fixture.GetHeight();

    const int FRAMES = 90;  // 1.5 seconds, full rotation
    int cx = width / 2;
    int cy = height / 2;
    int line_length = (width < height ? width : height) / 3;

    for (int frame = 0; frame < FRAMES; frame++) {
        uint8_t* buffer = fixture.GetBackBuffer();
        int pitch = fixture.GetPitch();

        // Clear
        for (int y = 0; y < height; y++) {
            memset(buffer + y * pitch, 32, width);
        }

        // Calculate line endpoints
        float angle = (frame * 2.0f * 3.14159f) / FRAMES;
        int x1 = cx + static_cast<int>(line_length * std::cos(angle));
        int y1 = cy + static_cast<int>(line_length * std::sin(angle));
        int x2 = cx - static_cast<int>(line_length * std::cos(angle));
        int y2 = cy - static_cast<int>(line_length * std::sin(angle));

        // Draw line using Bresenham
        int dx = std::abs(x2 - x1);
        int dy = std::abs(y2 - y1);
        int sx = x1 < x2 ? 1 : -1;
        int sy = y1 < y2 ? 1 : -1;
        int err = dx - dy;

        int x = x1, y = y1;
        while (true) {
            if (x >= 0 && x < width && y >= 0 && y < height) {
                buffer[y * pitch + x] = 255;
            }

            if (x == x2 && y == y2) break;

            int e2 = 2 * err;
            if (e2 > -dy) { err -= dy; x += sx; }
            if (e2 < dx) { err += dx; y += sy; }
        }

        // Draw center dot
        for (int y = cy - 3; y <= cy + 3; y++) {
            if (y < 0 || y >= height) continue;
            for (int x = cx - 3; x <= cx + 3; x++) {
                if (x < 0 || x >= width) continue;
                buffer[y * pitch + x] = 200;
            }
        }

        fixture.RenderFrame();
        Platform_Timer_Delay(16);
    }

    Screenshot_Capture("visual_animation_rotating.bmp");
}

//=============================================================================
// Fade In/Out Animation
//=============================================================================

TEST_WITH_FIXTURE(GraphicsFixture, Visual_Animation_Fade, "Visual") {
    if (!fixture.IsInitialized()) {
        TEST_SKIP("Graphics not initialized");
    }

    int width = fixture.GetWidth();
    int height = fixture.GetHeight();

    // Draw a static image
    uint8_t* buffer = fixture.GetBackBuffer();
    int pitch = fixture.GetPitch();

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            buffer[y * pitch + x] = (x + y) & 0xFF;
        }
    }

    // Store base palette
    PaletteEntry base_palette[256];
    for (int i = 0; i < 256; i++) {
        base_palette[i].r = static_cast<uint8_t>(i);
        base_palette[i].g = static_cast<uint8_t>(i);
        base_palette[i].b = static_cast<uint8_t>(i);
    }

    // Fade out
    for (int i = 255; i >= 0; i -= 8) {
        PaletteEntry faded[256];
        float factor = static_cast<float>(i) / 255.0f;

        for (int j = 0; j < 256; j++) {
            faded[j].r = static_cast<uint8_t>(base_palette[j].r * factor);
            faded[j].g = static_cast<uint8_t>(base_palette[j].g * factor);
            faded[j].b = static_cast<uint8_t>(base_palette[j].b * factor);
        }

        Platform_Graphics_SetPalette(faded, 0, 256);
        fixture.RenderFrame();
        Platform_Timer_Delay(30);
    }

    // Hold black
    Platform_Timer_Delay(200);

    // Fade in
    for (int i = 0; i <= 255; i += 8) {
        PaletteEntry faded[256];
        float factor = static_cast<float>(i) / 255.0f;

        for (int j = 0; j < 256; j++) {
            faded[j].r = static_cast<uint8_t>(base_palette[j].r * factor);
            faded[j].g = static_cast<uint8_t>(base_palette[j].g * factor);
            faded[j].b = static_cast<uint8_t>(base_palette[j].b * factor);
        }

        Platform_Graphics_SetPalette(faded, 0, 256);
        fixture.RenderFrame();
        Platform_Timer_Delay(30);
    }

    // Restore full palette
    Platform_Graphics_SetPalette(base_palette, 0, 256);
    fixture.RenderFrame();

    Screenshot_Capture("visual_animation_fade.bmp");
}
