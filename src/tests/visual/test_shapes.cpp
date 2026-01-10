// src/tests/visual/test_shapes.cpp
// Shape Rendering Visual Tests
// Task 18d - Visual Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "screenshot_utils.h"
#include "platform.h"
#include <cstring>

//=============================================================================
// Helpers
//=============================================================================

static bool HasGameData() {
    return Platform_File_Exists("gamedata/REDALERT.MIX") ||
           Platform_File_Exists("REDALERT.MIX") ||
           Platform_File_Exists("data/REDALERT.MIX");
}

static bool RegisterGameMix() {
    if (Platform_Mix_Register("REDALERT.MIX") == 0) return true;
    if (Platform_Mix_Register("gamedata/REDALERT.MIX") == 0) return true;
    if (Platform_Mix_Register("data/REDALERT.MIX") == 0) return true;
    return false;
}

//=============================================================================
// Shape Rendering Tests
//=============================================================================

TEST_CASE(Visual_Shape_Mouse, "Visual") {
    if (!HasGameData()) {
        TEST_SKIP("Game data not found");
    }

    Platform_Init();
    Platform_Graphics_Init();
    Platform_Assets_Init();

    if (!RegisterGameMix()) {
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        TEST_SKIP("Could not load game MIX file");
    }

    // Load palette first
    uint8_t pal_data[768];
    if (Platform_Palette_Load("TEMPERAT.PAL", pal_data) == 0) {
        PaletteEntry entries[256];
        for (int i = 0; i < 256; i++) {
            entries[i].r = pal_data[i * 3] * 4;
            entries[i].g = pal_data[i * 3 + 1] * 4;
            entries[i].b = pal_data[i * 3 + 2] * 4;
        }
        Platform_Graphics_SetPalette(entries, 0, 256);
    }

    // Load shape
    PlatformShape* shape = Platform_Shape_Load("MOUSE.SHP");
    if (!shape) {
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        TEST_SKIP("MOUSE.SHP not found");
    }

    uint8_t* buffer;
    int32_t w, h, p;
    Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);

    // Clear with dark color
    for (int y = 0; y < h; y++) {
        memset(buffer + y * p, 32, w);
    }

    // Get shape info
    int32_t shape_w, shape_h;
    Platform_Shape_GetSize(shape, &shape_w, &shape_h);
    int frame_count = Platform_Shape_GetFrameCount(shape);

    // Draw all mouse cursor frames in a grid
    int cols = 8;
    int spacing = 50;
    int32_t frame_size = shape_w * shape_h;
    uint8_t* frame_data = new uint8_t[frame_size];

    for (int i = 0; i < frame_count && i < 64; i++) {
        int col = i % cols;
        int row = i / cols;
        int x = 50 + col * spacing;
        int y_pos = 50 + row * spacing;

        Platform_Shape_GetFrame(shape, i, frame_data, frame_size);

        // Draw frame manually
        for (int sy = 0; sy < shape_h && y_pos + sy < h; sy++) {
            for (int sx = 0; sx < shape_w && x + sx < w; sx++) {
                uint8_t pixel = frame_data[sy * shape_w + sx];
                if (pixel != 0) {  // Skip transparent
                    buffer[(y_pos + sy) * p + (x + sx)] = pixel;
                }
            }
        }
    }

    delete[] frame_data;

    Platform_Graphics_Flip();
    Screenshot_Capture("visual_shape_mouse.bmp");

    Platform_Shape_Free(shape);
    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

TEST_CASE(Visual_Shape_Animation, "Visual") {
    if (!HasGameData()) {
        TEST_SKIP("Game data not found");
    }

    Platform_Init();
    Platform_Graphics_Init();
    Platform_Assets_Init();

    if (!RegisterGameMix()) {
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        TEST_SKIP("Could not load game MIX file");
    }

    // Load palette
    uint8_t pal_data[768];
    if (Platform_Palette_Load("TEMPERAT.PAL", pal_data) == 0) {
        PaletteEntry entries[256];
        for (int i = 0; i < 256; i++) {
            entries[i].r = pal_data[i * 3] * 4;
            entries[i].g = pal_data[i * 3 + 1] * 4;
            entries[i].b = pal_data[i * 3 + 2] * 4;
        }
        Platform_Graphics_SetPalette(entries, 0, 256);
    }

    PlatformShape* shape = Platform_Shape_Load("MOUSE.SHP");
    if (!shape) {
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        TEST_SKIP("Shape not found");
    }

    uint8_t* buffer;
    int32_t w, h, p;
    Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);

    int32_t shape_w, shape_h;
    Platform_Shape_GetSize(shape, &shape_w, &shape_h);
    int frame_count = Platform_Shape_GetFrameCount(shape);
    int32_t frame_size = shape_w * shape_h;
    uint8_t* frame_data = new uint8_t[frame_size];

    // Animate through frames (3 loops or 30 frames max)
    int max_frames = (frame_count < 10) ? frame_count * 3 : 30;
    for (int i = 0; i < max_frames; i++) {
        int frame = i % frame_count;

        // Clear
        for (int y = 0; y < h; y++) {
            memset(buffer + y * p, 32, w);
        }

        int x = w / 2 - shape_w / 2;
        int y_pos = h / 2 - shape_h / 2;

        Platform_Shape_GetFrame(shape, frame, frame_data, frame_size);

        // Draw
        for (int sy = 0; sy < shape_h && y_pos + sy < h; sy++) {
            for (int sx = 0; sx < shape_w && x + sx < w; sx++) {
                uint8_t pixel = frame_data[sy * shape_w + sx];
                if (pixel != 0) {
                    buffer[(y_pos + sy) * p + (x + sx)] = pixel;
                }
            }
        }

        Platform_Graphics_Flip();
        Platform_Timer_Delay(100);
    }

    delete[] frame_data;
    Platform_Shape_Free(shape);
    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Synthetic Shape Tests (no game data needed)
//=============================================================================

TEST_WITH_FIXTURE(GraphicsFixture, Visual_Shape_Synthetic, "Visual") {
    if (!fixture.IsInitialized()) {
        TEST_SKIP("Graphics not initialized");
    }

    uint8_t* buffer = fixture.GetBackBuffer();
    int width = fixture.GetWidth();
    int height = fixture.GetHeight();
    int pitch = fixture.GetPitch();

    // Clear
    fixture.ClearBackBuffer(32);

    // Draw synthetic "sprites" - colored squares with transparency pattern
    const int SPRITE_SIZE = 32;
    const int SPRITES_X = 10;
    const int SPRITES_Y = 8;

    for (int sy = 0; sy < SPRITES_Y; sy++) {
        for (int sx = 0; sx < SPRITES_X; sx++) {
            int x_base = 50 + sx * (SPRITE_SIZE + 10);
            int y_base = 50 + sy * (SPRITE_SIZE + 10);
            uint8_t color = static_cast<uint8_t>(sy * SPRITES_X + sx + 64);

            // Draw sprite with transparent corners (checkerboard pattern)
            for (int y = 0; y < SPRITE_SIZE && y_base + y < height; y++) {
                for (int x = 0; x < SPRITE_SIZE && x_base + x < width; x++) {
                    // Skip corners to simulate transparency
                    int dx = (x < SPRITE_SIZE/2) ? x : SPRITE_SIZE - 1 - x;
                    int dy = (y < SPRITE_SIZE/2) ? y : SPRITE_SIZE - 1 - y;
                    if (dx + dy < 4) continue;  // Corner cut

                    buffer[(y_base + y) * pitch + (x_base + x)] = color;
                }
            }
        }
    }

    fixture.RenderFrame();
    Screenshot_Capture("visual_shape_synthetic.bmp");
}

TEST_WITH_FIXTURE(GraphicsFixture, Visual_Shape_Transparency, "Visual") {
    if (!fixture.IsInitialized()) {
        TEST_SKIP("Graphics not initialized");
    }

    uint8_t* buffer = fixture.GetBackBuffer();
    int width = fixture.GetWidth();
    int height = fixture.GetHeight();
    int pitch = fixture.GetPitch();

    // Draw background pattern
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Checkerboard pattern
            buffer[y * pitch + x] = ((x / 8 + y / 8) % 2) ? 100 : 80;
        }
    }

    // Draw "sprite" with color 0 as transparent
    const int SPRITE_SIZE = 100;
    int x_base = width / 2 - SPRITE_SIZE / 2;
    int y_base = height / 2 - SPRITE_SIZE / 2;

    for (int y = 0; y < SPRITE_SIZE && y_base + y < height; y++) {
        for (int x = 0; x < SPRITE_SIZE && x_base + x < width; x++) {
            // Create a shape with transparent holes
            int dx = x - SPRITE_SIZE / 2;
            int dy = y - SPRITE_SIZE / 2;
            int dist_sq = dx * dx + dy * dy;

            // Outer circle
            if (dist_sq > (SPRITE_SIZE/2) * (SPRITE_SIZE/2)) {
                continue;  // Outside - keep background (transparent)
            }

            // Inner hole
            if (dist_sq < (SPRITE_SIZE/4) * (SPRITE_SIZE/4)) {
                continue;  // Inside hole - keep background (transparent)
            }

            // Draw the ring
            buffer[(y_base + y) * pitch + (x_base + x)] = 200;
        }
    }

    fixture.RenderFrame();
    Screenshot_Capture("visual_shape_transparency.bmp");
}
