// src/tests/integration/test_render_pipeline.cpp
// Render Pipeline Integration Tests
// Task 18c - Integration Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "platform.h"
#include <cstring>

//=============================================================================
// Helper to check for game data
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
// Basic Render Tests
//=============================================================================

TEST_WITH_FIXTURE(GraphicsFixture, RenderPipeline_ClearScreen, "Render") {
    if (!fixture.IsInitialized()) {
        TEST_SKIP("Graphics not initialized");
    }

    // Clear to a specific color
    fixture.ClearBackBuffer(100);

    // Verify
    uint8_t* buffer = fixture.GetBackBuffer();
    TEST_ASSERT_EQ(buffer[0], 100);

    fixture.RenderFrame();
}

TEST_WITH_FIXTURE(GraphicsFixture, RenderPipeline_DrawPattern, "Render") {
    if (!fixture.IsInitialized()) {
        TEST_SKIP("Graphics not initialized");
    }

    uint8_t* buffer = fixture.GetBackBuffer();
    int width = fixture.GetWidth();
    int height = fixture.GetHeight();
    int pitch = fixture.GetPitch();

    // Draw a gradient pattern
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            buffer[y * pitch + x] = (x + y) & 0xFF;
        }
    }

    fixture.RenderFrame();

    // Verify pattern
    TEST_ASSERT_EQ(buffer[0], 0);
    TEST_ASSERT_EQ(buffer[100], 100);
}

//=============================================================================
// Palette Integration Tests
//=============================================================================

TEST_CASE(RenderPipeline_Palette_Apply, "Render") {
    Platform_Init();
    Platform_Graphics_Init();

    // Create a grayscale palette
    PaletteEntry entries[256];
    for (int i = 0; i < 256; i++) {
        entries[i].r = static_cast<uint8_t>(i);
        entries[i].g = static_cast<uint8_t>(i);
        entries[i].b = static_cast<uint8_t>(i);
    }

    Platform_Graphics_SetPalette(entries, 0, 256);

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

TEST_CASE(RenderPipeline_Palette_LoadAndApply, "Render") {
    if (!HasGameData()) {
        TEST_SKIP("Game data not found");
    }

    Platform_Init();
    Platform_Graphics_Init();
    Platform_Assets_Init();

    bool has_data = RegisterGameMix();
    if (!has_data) {
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        TEST_SKIP("Game data not found");
    }

    uint8_t palette_data[768];
    if (Platform_Palette_Load("TEMPERAT.PAL", palette_data) == 0) {
        // Convert to PaletteEntry array
        PaletteEntry entries[256];
        for (int i = 0; i < 256; i++) {
            entries[i].r = palette_data[i * 3];
            entries[i].g = palette_data[i * 3 + 1];
            entries[i].b = palette_data[i * 3 + 2];
        }

        // Apply palette to graphics
        Platform_Graphics_SetPalette(entries, 0, 256);

        // Render a frame with the palette
        Platform_Graphics_Flip();
    }

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Shape Rendering Tests
//=============================================================================

TEST_CASE(RenderPipeline_Shape_DrawToBuffer, "Render") {
    if (!HasGameData()) {
        TEST_SKIP("Game data not found");
    }

    Platform_Init();
    Platform_Graphics_Init();
    Platform_Assets_Init();

    bool has_data = RegisterGameMix();
    if (!has_data) {
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        TEST_SKIP("Game data not found");
    }

    PlatformShape* shape = Platform_Shape_Load("MOUSE.SHP");
    if (shape) {
        uint8_t* buffer;
        int32_t w, h, p;
        Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);

        // Clear buffer
        memset(buffer, 0, h * p);

        // Get shape dimensions
        int32_t shape_w, shape_h;
        Platform_Shape_GetSize(shape, &shape_w, &shape_h);

        // Get frame data
        int32_t frame_size = shape_w * shape_h;
        uint8_t* frame_data = new uint8_t[frame_size];
        Platform_Shape_GetFrame(shape, 0, frame_data, frame_size);

        // Draw shape frame 0 at center of screen (manual blit)
        int draw_x = (w - shape_w) / 2;
        int draw_y = (h - shape_h) / 2;

        for (int sy = 0; sy < shape_h && draw_y + sy < h; sy++) {
            for (int sx = 0; sx < shape_w && draw_x + sx < w; sx++) {
                uint8_t pixel = frame_data[sy * shape_w + sx];
                if (pixel != 0) {  // Skip transparent
                    buffer[(draw_y + sy) * p + (draw_x + sx)] = pixel;
                }
            }
        }

        Platform_Graphics_Flip();

        // Verify something was drawn (not all zeros in draw area)
        bool has_content = false;
        for (int y = draw_y; y < draw_y + shape_h && y < h; y++) {
            for (int x = draw_x; x < draw_x + shape_w && x < w; x++) {
                if (buffer[y * p + x] != 0) {
                    has_content = true;
                    break;
                }
            }
            if (has_content) break;
        }
        TEST_ASSERT(has_content);

        delete[] frame_data;
        Platform_Shape_Free(shape);
    }

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Multi-Frame Render Tests
//=============================================================================

TEST_CASE(RenderPipeline_Animation_MultipleFrames, "Render") {
    Platform_Init();
    Platform_Graphics_Init();

    // Render 30 frames (0.5 second at 60fps)
    for (int frame = 0; frame < 30; frame++) {
        uint8_t* buffer;
        int32_t w, h, p;
        Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);

        // Animate - change clear color each frame
        uint8_t color = (frame * 8) & 0xFF;
        for (int y = 0; y < h; y++) {
            memset(buffer + y * p, color, w);
        }

        Platform_Graphics_Flip();
        Platform_Timer_Delay(16);
    }

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

TEST_CASE(RenderPipeline_Shape_AllFrames, "Render") {
    if (!HasGameData()) {
        TEST_SKIP("Game data not found");
    }

    Platform_Init();
    Platform_Graphics_Init();
    Platform_Assets_Init();

    bool has_data = RegisterGameMix();
    if (!has_data) {
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        TEST_SKIP("Game data not found");
    }

    PlatformShape* shape = Platform_Shape_Load("MOUSE.SHP");
    if (shape) {
        uint8_t* buffer;
        int32_t w, h, p;
        Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);

        int32_t shape_w, shape_h;
        Platform_Shape_GetSize(shape, &shape_w, &shape_h);

        int32_t frame_size = shape_w * shape_h;
        uint8_t* frame_data = new uint8_t[frame_size];

        // Draw each frame
        int frame_count = Platform_Shape_GetFrameCount(shape);
        int max_frames = (frame_count > 10) ? 10 : frame_count;  // Limit for test speed

        for (int i = 0; i < max_frames; i++) {
            memset(buffer, 0, h * p);

            Platform_Shape_GetFrame(shape, i, frame_data, frame_size);

            int draw_x = (w - shape_w) / 2;
            int draw_y = (h - shape_h) / 2;

            for (int sy = 0; sy < shape_h && draw_y + sy < h; sy++) {
                for (int sx = 0; sx < shape_w && draw_x + sx < w; sx++) {
                    uint8_t pixel = frame_data[sy * shape_w + sx];
                    if (pixel != 0) {
                        buffer[(draw_y + sy) * p + (draw_x + sx)] = pixel;
                    }
                }
            }

            Platform_Graphics_Flip();
            Platform_Timer_Delay(50);  // 50ms per frame for visibility
        }

        delete[] frame_data;
        Platform_Shape_Free(shape);
    }

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Buffer Operations Tests
//=============================================================================

TEST_CASE(RenderPipeline_BufferFill, "Render") {
    Platform_Init();
    Platform_Graphics_Init();

    uint8_t* buffer;
    int32_t w, h, p;
    Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);

    // Use platform buffer fill
    Platform_Buffer_FillRect(buffer, p, w, h, 100, 100, 200, 150, 42);

    // Verify fill
    TEST_ASSERT_EQ(buffer[100 * p + 100], 42);
    TEST_ASSERT_EQ(buffer[150 * p + 200], 42);

    Platform_Graphics_Flip();

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

TEST_CASE(RenderPipeline_BufferLines, "Render") {
    Platform_Init();
    Platform_Graphics_Init();

    uint8_t* buffer;
    int32_t w, h, p;
    Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);

    // Clear
    Platform_Buffer_Clear(buffer, h * p, 0);

    // Draw horizontal line
    Platform_Buffer_HLine(buffer, p, w, h, 50, 100, 200, 255);

    // Draw vertical line
    Platform_Buffer_VLine(buffer, p, w, h, 150, 50, 100, 128);

    // Verify
    TEST_ASSERT_EQ(buffer[100 * p + 50], 255);  // H-line start
    TEST_ASSERT_EQ(buffer[50 * p + 150], 128);  // V-line start

    Platform_Graphics_Flip();

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Palette Fade Tests
//=============================================================================

TEST_CASE(RenderPipeline_PaletteFade, "Render") {
    Platform_Init();
    Platform_Graphics_Init();

    // Set up a palette
    PaletteEntry entries[256];
    for (int i = 0; i < 256; i++) {
        entries[i].r = static_cast<uint8_t>(i);
        entries[i].g = static_cast<uint8_t>(i);
        entries[i].b = static_cast<uint8_t>(i);
    }
    Platform_Graphics_SetPalette(entries, 0, 256);

    // Draw gradient
    uint8_t* buffer;
    int32_t w, h, p;
    Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            buffer[y * p + x] = x & 0xFF;
        }
    }

    // Fade to black
    for (float f = 1.0f; f >= 0.0f; f -= 0.1f) {
        Platform_Graphics_FadePalette(f);
        Platform_Graphics_Flip();
        Platform_Timer_Delay(50);
    }

    // Restore
    Platform_Graphics_RestorePalette();
    Platform_Graphics_Flip();

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}
