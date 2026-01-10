// src/tests/visual/test_palettes.cpp
// Palette Rendering Visual Tests
// Task 18d - Visual Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "screenshot_utils.h"
#include "platform.h"
#include <cstring>

//=============================================================================
// Palette Gradient Tests
//=============================================================================

TEST_WITH_FIXTURE(GraphicsFixture, Visual_Palette_Gradient, "Visual") {
    if (!fixture.IsInitialized()) {
        TEST_SKIP("Graphics not initialized");
    }

    uint8_t* buffer = fixture.GetBackBuffer();
    int width = fixture.GetWidth();
    int height = fixture.GetHeight();
    int pitch = fixture.GetPitch();

    // Draw 256 vertical stripes
    for (int x = 0; x < width; x++) {
        uint8_t color = static_cast<uint8_t>((x * 256) / width);

        for (int y = 0; y < height; y++) {
            buffer[y * pitch + x] = color;
        }
    }

    fixture.RenderFrame();
    Screenshot_Capture("visual_palette_gradient.bmp");
}

TEST_WITH_FIXTURE(GraphicsFixture, Visual_Palette_Grid, "Visual") {
    if (!fixture.IsInitialized()) {
        TEST_SKIP("Graphics not initialized");
    }

    uint8_t* buffer = fixture.GetBackBuffer();
    int width = fixture.GetWidth();
    int height = fixture.GetHeight();
    int pitch = fixture.GetPitch();

    // 16x16 grid of all 256 colors
    int cell_w = width / 16;
    int cell_h = height / 16;

    for (int cy = 0; cy < 16; cy++) {
        for (int cx = 0; cx < 16; cx++) {
            uint8_t color = static_cast<uint8_t>(cy * 16 + cx);

            for (int y = cy * cell_h; y < (cy + 1) * cell_h && y < height; y++) {
                for (int x = cx * cell_w; x < (cx + 1) * cell_w && x < width; x++) {
                    buffer[y * pitch + x] = color;
                }
            }
        }
    }

    fixture.RenderFrame();
    Screenshot_Capture("visual_palette_grid.bmp");
}

//=============================================================================
// Game Palette Tests
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

TEST_CASE(Visual_Palette_Temperate, "Visual") {
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
    if (Platform_Palette_Load("TEMPERAT.PAL", pal_data) != 0) {
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        TEST_SKIP("TEMPERAT.PAL not found");
    }

    // Convert and apply palette
    PaletteEntry entries[256];
    for (int i = 0; i < 256; i++) {
        // VGA palette is 0-63, need to scale to 0-255
        entries[i].r = pal_data[i * 3] * 4;
        entries[i].g = pal_data[i * 3 + 1] * 4;
        entries[i].b = pal_data[i * 3 + 2] * 4;
    }
    Platform_Graphics_SetPalette(entries, 0, 256);

    // Draw color grid
    uint8_t* buffer;
    int32_t w, h, p;
    Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);

    int cell_w = w / 16;
    int cell_h = h / 16;

    for (int cy = 0; cy < 16; cy++) {
        for (int cx = 0; cx < 16; cx++) {
            uint8_t color = static_cast<uint8_t>(cy * 16 + cx);
            for (int y = cy * cell_h; y < (cy + 1) * cell_h && y < h; y++) {
                for (int x = cx * cell_w; x < (cx + 1) * cell_w && x < w; x++) {
                    buffer[y * p + x] = color;
                }
            }
        }
    }

    Platform_Graphics_Flip();
    Screenshot_Capture("visual_palette_temperate.bmp");

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

TEST_CASE(Visual_Palette_AllTheaters, "Visual") {
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

    const char* palette_names[] = {
        "TEMPERAT.PAL", "SNOW.PAL", "INTERIOR.PAL"
    };
    const char* output_names[] = {
        "visual_palette_TEMPERAT", "visual_palette_SNOW", "visual_palette_INTERIOR"
    };

    int loaded = 0;
    for (int i = 0; i < 3; i++) {
        uint8_t pal_data[768];
        if (Platform_Palette_Load(palette_names[i], pal_data) != 0) {
            continue;
        }

        // Convert and apply palette
        PaletteEntry entries[256];
        for (int j = 0; j < 256; j++) {
            entries[j].r = pal_data[j * 3] * 4;
            entries[j].g = pal_data[j * 3 + 1] * 4;
            entries[j].b = pal_data[j * 3 + 2] * 4;
        }
        Platform_Graphics_SetPalette(entries, 0, 256);

        uint8_t* buffer;
        int32_t w, h, p;
        Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);

        // Draw gradient
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                buffer[y * p + x] = x & 0xFF;
            }
        }

        Platform_Graphics_Flip();

        std::string filename = std::string(output_names[i]) + ".bmp";
        Screenshot_Capture(filename.c_str());
        loaded++;

        Platform_Timer_Delay(100);
    }

    Platform_Graphics_Shutdown();
    Platform_Shutdown();

    if (loaded == 0) {
        TEST_SKIP("No palettes could be loaded");
    }
}

//=============================================================================
// Grayscale Palette Test
//=============================================================================

TEST_WITH_FIXTURE(GraphicsFixture, Visual_Palette_Grayscale, "Visual") {
    if (!fixture.IsInitialized()) {
        TEST_SKIP("Graphics not initialized");
    }

    // Set up grayscale palette
    PaletteEntry entries[256];
    for (int i = 0; i < 256; i++) {
        entries[i].r = static_cast<uint8_t>(i);
        entries[i].g = static_cast<uint8_t>(i);
        entries[i].b = static_cast<uint8_t>(i);
    }
    Platform_Graphics_SetPalette(entries, 0, 256);

    uint8_t* buffer = fixture.GetBackBuffer();
    int width = fixture.GetWidth();
    int height = fixture.GetHeight();
    int pitch = fixture.GetPitch();

    // Draw horizontal gradient
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            buffer[y * pitch + x] = static_cast<uint8_t>((x * 256) / width);
        }
    }

    fixture.RenderFrame();
    Screenshot_Capture("visual_palette_grayscale.bmp");
}

//=============================================================================
// Color Ramp Test
//=============================================================================

TEST_WITH_FIXTURE(GraphicsFixture, Visual_Palette_ColorRamps, "Visual") {
    if (!fixture.IsInitialized()) {
        TEST_SKIP("Graphics not initialized");
    }

    // Set up color ramp palette - Red, Green, Blue in groups of 64
    PaletteEntry entries[256];

    // 0-63: Red ramp
    for (int i = 0; i < 64; i++) {
        entries[i].r = static_cast<uint8_t>(i * 4);
        entries[i].g = 0;
        entries[i].b = 0;
    }
    // 64-127: Green ramp
    for (int i = 0; i < 64; i++) {
        entries[64 + i].r = 0;
        entries[64 + i].g = static_cast<uint8_t>(i * 4);
        entries[64 + i].b = 0;
    }
    // 128-191: Blue ramp
    for (int i = 0; i < 64; i++) {
        entries[128 + i].r = 0;
        entries[128 + i].g = 0;
        entries[128 + i].b = static_cast<uint8_t>(i * 4);
    }
    // 192-255: White ramp
    for (int i = 0; i < 64; i++) {
        uint8_t v = static_cast<uint8_t>(i * 4);
        entries[192 + i].r = v;
        entries[192 + i].g = v;
        entries[192 + i].b = v;
    }

    Platform_Graphics_SetPalette(entries, 0, 256);

    uint8_t* buffer = fixture.GetBackBuffer();
    int width = fixture.GetWidth();
    int height = fixture.GetHeight();
    int pitch = fixture.GetPitch();

    // Draw 4 horizontal bands
    int band_height = height / 4;

    for (int band = 0; band < 4; band++) {
        int base_color = band * 64;
        int y_start = band * band_height;

        for (int y = y_start; y < y_start + band_height && y < height; y++) {
            for (int x = 0; x < width; x++) {
                int color_offset = (x * 64) / width;
                buffer[y * pitch + x] = static_cast<uint8_t>(base_color + color_offset);
            }
        }
    }

    fixture.RenderFrame();
    Screenshot_Capture("visual_palette_colorramps.bmp");
}
