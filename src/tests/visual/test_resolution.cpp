// src/tests/visual/test_resolution.cpp
// Resolution Visual Tests
// Task 18d - Visual Tests

#include "test/test_framework.h"
#include "screenshot_utils.h"
#include "platform.h"
#include <cstring>
#include <sstream>

//=============================================================================
// Resolution Tests
//=============================================================================

TEST_CASE(Visual_Resolution_Default, "Visual") {
    Platform_Init();

    TEST_ASSERT_EQ(Platform_Graphics_Init(), 0);

    uint8_t* buffer;
    int32_t w, h, p;
    Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);

    // Verify dimensions
    TEST_ASSERT_GT(w, 0);
    TEST_ASSERT_GT(h, 0);
    TEST_ASSERT_GE(p, w);

    // Draw test pattern
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            buffer[y * p + x] = (x ^ y) & 0xFF;
        }
    }

    Platform_Graphics_Flip();

    std::ostringstream filename;
    filename << "visual_res_" << w << "x" << h << ".bmp";
    Screenshot_Capture(filename.str().c_str());

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

TEST_CASE(Visual_Resolution_TestPattern, "Visual") {
    Platform_Init();
    Platform_Graphics_Init();

    uint8_t* buffer;
    int32_t w, h, p;
    Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);

    // Draw comprehensive test pattern

    // 1. Border (white)
    for (int x = 0; x < w; x++) {
        buffer[0 * p + x] = 255;
        buffer[(h-1) * p + x] = 255;
    }
    for (int y = 0; y < h; y++) {
        buffer[y * p + 0] = 255;
        buffer[y * p + (w-1)] = 255;
    }

    // 2. Corner markers
    int marker_size = 20;
    for (int y = 0; y < marker_size; y++) {
        for (int x = 0; x < marker_size; x++) {
            // Top-left (red = high color index)
            buffer[y * p + x] = 200;
            // Top-right
            buffer[y * p + (w - 1 - x)] = 200;
            // Bottom-left
            buffer[(h - 1 - y) * p + x] = 200;
            // Bottom-right
            buffer[(h - 1 - y) * p + (w - 1 - x)] = 200;
        }
    }

    // 3. Center crosshair
    int cx = w / 2;
    int cy = h / 2;
    for (int i = -50; i <= 50; i++) {
        if (cx + i >= 0 && cx + i < w) buffer[cy * p + (cx + i)] = 255;
        if (cy + i >= 0 && cy + i < h) buffer[(cy + i) * p + cx] = 255;
    }

    // 4. Color bars in center
    int bar_h = 40;
    int bar_y = cy - bar_h / 2;
    int bar_w = w / 8;

    for (int i = 0; i < 8; i++) {
        int bar_x = i * bar_w;
        uint8_t color = static_cast<uint8_t>(i * 32);

        for (int y = bar_y; y < bar_y + bar_h && y < h; y++) {
            for (int x = bar_x; x < bar_x + bar_w && x < w; x++) {
                buffer[y * p + x] = color;
            }
        }
    }

    Platform_Graphics_Flip();
    Screenshot_Capture("visual_res_testpattern.bmp");

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

TEST_CASE(Visual_Resolution_Aspect, "Visual") {
    Platform_Init();
    Platform_Graphics_Init();

    uint8_t* buffer;
    int32_t w, h, p;
    Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);

    // Clear
    memset(buffer, 0, h * p);

    // Draw circles to verify aspect ratio
    int cx = w / 2;
    int cy = h / 2;
    int radius = (w < h ? w : h) / 3;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int dx = x - cx;
            int dy = y - cy;
            int dist_sq = dx * dx + dy * dy;

            // Draw multiple circles
            if (dist_sq >= radius * radius - 200 && dist_sq <= radius * radius + 200) {
                buffer[y * p + x] = 255;  // Outer circle
            }
            int r2 = radius * 2 / 3;
            if (dist_sq >= r2 * r2 - 200 && dist_sq <= r2 * r2 + 200) {
                buffer[y * p + x] = 200;  // Middle circle
            }
            int r3 = radius / 3;
            if (dist_sq >= r3 * r3 - 200 && dist_sq <= r3 * r3 + 200) {
                buffer[y * p + x] = 150;  // Inner circle
            }
        }
    }

    // Draw square to compare with circle
    int sq_size = radius;
    int sq_x = cx - sq_size / 2;
    int sq_y = cy - sq_size / 2;

    for (int y = sq_y; y < sq_y + sq_size && y < h; y++) {
        if (y < 0) continue;
        buffer[y * p + sq_x] = 100;
        if (sq_x + sq_size - 1 < w) buffer[y * p + sq_x + sq_size - 1] = 100;
    }
    for (int x = sq_x; x < sq_x + sq_size && x < w; x++) {
        if (x < 0) continue;
        buffer[sq_y * p + x] = 100;
        if (sq_y + sq_size - 1 < h) buffer[(sq_y + sq_size - 1) * p + x] = 100;
    }

    Platform_Graphics_Flip();
    Screenshot_Capture("visual_res_aspect.bmp");

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Display Mode Tests
//=============================================================================

TEST_CASE(Visual_Resolution_DisplayMode, "Visual") {
    Platform_Init();
    Platform_Graphics_Init();

    DisplayMode mode;
    Platform_Graphics_GetMode(&mode);

    // Verify mode info
    TEST_ASSERT_GT(mode.width, 0);
    TEST_ASSERT_GT(mode.height, 0);

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Pitch/Stride Tests
//=============================================================================

TEST_CASE(Visual_Resolution_Pitch, "Visual") {
    Platform_Init();
    Platform_Graphics_Init();

    uint8_t* buffer;
    int32_t w, h, p;
    Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);

    // Pitch should be at least width
    TEST_ASSERT_GE(p, w);

    // Draw diagonal lines to verify pitch is correct
    for (int y = 0; y < h; y++) {
        // Clear row
        memset(buffer + y * p, 0, w);

        // Draw diagonal pixel
        int x = y % w;
        buffer[y * p + x] = 255;

        // Draw reverse diagonal
        x = (w - 1) - (y % w);
        buffer[y * p + x] = 200;
    }

    Platform_Graphics_Flip();
    Screenshot_Capture("visual_res_pitch.bmp");

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}
