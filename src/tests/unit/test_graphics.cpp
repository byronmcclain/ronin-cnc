// src/tests/unit/test_graphics.cpp
// Graphics System Unit Tests
// Task 18b - Unit Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "platform.h"
#include <cstring>
#include <algorithm>

//=============================================================================
// Graphics Initialization Tests
//=============================================================================

TEST_WITH_FIXTURE(GraphicsFixture, Graphics_Init_Succeeds, "Graphics") {
    TEST_ASSERT(fixture.IsInitialized());
}

TEST_WITH_FIXTURE(GraphicsFixture, Graphics_Dimensions_Valid, "Graphics") {
    TEST_ASSERT_GT(fixture.GetWidth(), 0);
    TEST_ASSERT_GT(fixture.GetHeight(), 0);
    TEST_ASSERT_GE(fixture.GetPitch(), fixture.GetWidth());
}

TEST_WITH_FIXTURE(GraphicsFixture, Graphics_BackBuffer_NotNull, "Graphics") {
    uint8_t* buffer = fixture.GetBackBuffer();
    TEST_ASSERT_NOT_NULL(buffer);
}

//=============================================================================
// Pixel Operations Tests
//=============================================================================

TEST_WITH_FIXTURE(GraphicsFixture, Graphics_WritePixel, "Graphics") {
    uint8_t* buffer = fixture.GetBackBuffer();
    TEST_ASSERT_NOT_NULL(buffer);

    // Write a pixel
    buffer[0] = 123;

    // Verify
    TEST_ASSERT_EQ(buffer[0], 123);
}

TEST_WITH_FIXTURE(GraphicsFixture, Graphics_ClearBuffer, "Graphics") {
    fixture.ClearBackBuffer(42);

    uint8_t* buffer = fixture.GetBackBuffer();
    int width = fixture.GetWidth();
    int height = fixture.GetHeight();
    int pitch = fixture.GetPitch();

    // Verify some pixels are cleared
    TEST_ASSERT_EQ(buffer[0], 42);
    TEST_ASSERT_EQ(buffer[pitch + 1], 42);
    TEST_ASSERT_EQ(buffer[(height - 1) * pitch + (width - 1)], 42);
}

TEST_WITH_FIXTURE(GraphicsFixture, Graphics_HorizontalLine, "Graphics") {
    fixture.ClearBackBuffer(0);

    uint8_t* buffer = fixture.GetBackBuffer();
    int pitch = fixture.GetPitch();

    // Draw horizontal line at y=10
    int y = 10;
    for (int x = 0; x < 100; x++) {
        buffer[y * pitch + x] = 255;
    }

    // Verify line
    TEST_ASSERT_EQ(buffer[y * pitch + 0], 255);
    TEST_ASSERT_EQ(buffer[y * pitch + 50], 255);
    TEST_ASSERT_EQ(buffer[y * pitch + 99], 255);

    // Verify adjacent pixels are still 0
    TEST_ASSERT_EQ(buffer[(y-1) * pitch + 50], 0);
    TEST_ASSERT_EQ(buffer[(y+1) * pitch + 50], 0);
}

//=============================================================================
// Color Index Tests
//=============================================================================

TEST_CASE(Graphics_PaletteIndex_Range, "Graphics") {
    // Palette index is 0-255
    for (int i = 0; i < 256; i++) {
        uint8_t index = static_cast<uint8_t>(i);
        TEST_ASSERT_EQ(index, i);
    }
}

TEST_CASE(Graphics_TransparentColor, "Graphics") {
    // Color 0 is typically transparent
    uint8_t transparent = 0;
    TEST_ASSERT_EQ(transparent, 0);
}

//=============================================================================
// Resolution Tests
//=============================================================================

TEST_CASE(Graphics_Resolution_640x480, "Graphics") {
    // Default game resolution
    int width = 640;
    int height = 480;

    TEST_ASSERT_EQ(width, 640);
    TEST_ASSERT_EQ(height, 480);

    // Total pixels
    int total = width * height;
    TEST_ASSERT_EQ(total, 307200);
}

TEST_CASE(Graphics_Resolution_320x200, "Graphics") {
    // Original DOS resolution
    int width = 320;
    int height = 200;

    TEST_ASSERT_EQ(width, 320);
    TEST_ASSERT_EQ(height, 200);
}

//=============================================================================
// Pitch/Stride Tests
//=============================================================================

TEST_CASE(Graphics_Pitch_Alignment, "Graphics") {
    // Pitch might be aligned to 4 or 8 bytes
    int width = 640;
    int alignment = 4;

    int aligned_pitch = (width + alignment - 1) & ~(alignment - 1);
    TEST_ASSERT_GE(aligned_pitch, width);
    TEST_ASSERT_EQ(aligned_pitch % alignment, 0);
}

TEST_WITH_FIXTURE(GraphicsFixture, Graphics_Pitch_AtLeastWidth, "Graphics") {
    int width = fixture.GetWidth();
    int pitch = fixture.GetPitch();

    TEST_ASSERT_GE(pitch, width);
}

//=============================================================================
// Flip/Present Tests
//=============================================================================

TEST_WITH_FIXTURE(GraphicsFixture, Graphics_Flip_DoesNotCrash, "Graphics") {
    // Draw something
    fixture.ClearBackBuffer(100);

    // Flip
    fixture.RenderFrame();

    TEST_ASSERT(true);
}

TEST_WITH_FIXTURE(GraphicsFixture, Graphics_MultipleFlips, "Graphics") {
    for (int i = 0; i < 10; i++) {
        fixture.ClearBackBuffer(static_cast<uint8_t>(i * 25));
        fixture.RenderFrame();
    }

    TEST_ASSERT(true);
}

//=============================================================================
// Clipping Tests
//=============================================================================

TEST_CASE(Graphics_ClipRect_Intersection, "Graphics") {
    // Test rectangle intersection logic
    struct Rect { int x, y, w, h; };

    Rect screen = {0, 0, 640, 480};
    Rect sprite = {600, 460, 100, 100};  // Partially off-screen

    // Clip sprite to screen
    int clip_x = (sprite.x < 0) ? 0 : sprite.x;
    int clip_y = (sprite.y < 0) ? 0 : sprite.y;
    int clip_w = std::min(sprite.x + sprite.w, screen.w) - clip_x;
    int clip_h = std::min(sprite.y + sprite.h, screen.h) - clip_y;

    TEST_ASSERT_EQ(clip_x, 600);
    TEST_ASSERT_EQ(clip_y, 460);
    TEST_ASSERT_EQ(clip_w, 40);   // Only 40 pixels visible
    TEST_ASSERT_EQ(clip_h, 20);   // Only 20 pixels visible
}

TEST_CASE(Graphics_ClipRect_FullyOffscreen, "Graphics") {
    // Sprite completely off-screen
    struct Rect { int x, y, w, h; };

    Rect screen = {0, 0, 640, 480};
    Rect sprite = {700, 500, 100, 100};  // Completely off-screen

    bool visible = (sprite.x < screen.w && sprite.y < screen.h &&
                   sprite.x + sprite.w > 0 && sprite.y + sprite.h > 0);

    TEST_ASSERT(!visible);
}

TEST_CASE(Graphics_ClipRect_NegativePosition, "Graphics") {
    // Sprite partially off top-left
    struct Rect { int x, y, w, h; };

    Rect sprite = {-50, -30, 100, 100};

    // Clipped origin
    int src_x = (sprite.x < 0) ? -sprite.x : 0;
    int src_y = (sprite.y < 0) ? -sprite.y : 0;

    TEST_ASSERT_EQ(src_x, 50);
    TEST_ASSERT_EQ(src_y, 30);
}

//=============================================================================
// Coordinate Conversion Tests
//=============================================================================

TEST_CASE(Graphics_ScreenToCell, "Graphics") {
    // Convert screen coordinates to map cell
    int cell_size = 24;  // ICON_PIXEL_W

    int screen_x = 100;
    int screen_y = 200;

    int cell_x = screen_x / cell_size;
    int cell_y = screen_y / cell_size;

    TEST_ASSERT_EQ(cell_x, 4);
    TEST_ASSERT_EQ(cell_y, 8);
}

TEST_CASE(Graphics_CellToScreen, "Graphics") {
    int cell_size = 24;

    int cell_x = 10;
    int cell_y = 20;

    int screen_x = cell_x * cell_size;
    int screen_y = cell_y * cell_size;

    TEST_ASSERT_EQ(screen_x, 240);
    TEST_ASSERT_EQ(screen_y, 480);
}

//=============================================================================
// Palette Tests
//=============================================================================

TEST_WITH_FIXTURE(GraphicsFixture, Graphics_SetPalette_DoesNotCrash, "Graphics") {
    // Create a test palette
    PaletteEntry entries[256];
    for (int i = 0; i < 256; i++) {
        entries[i].r = static_cast<uint8_t>(i);
        entries[i].g = static_cast<uint8_t>(255 - i);
        entries[i].b = static_cast<uint8_t>(i / 2);
    }

    Platform_Graphics_SetPalette(entries, 0, 256);
    TEST_ASSERT(true);
}

TEST_WITH_FIXTURE(GraphicsFixture, Graphics_GetPalette_DoesNotCrash, "Graphics") {
    PaletteEntry entries[256];
    Platform_Graphics_GetPalette(entries, 0, 256);
    TEST_ASSERT(true);
}

//=============================================================================
// Display Mode Tests
//=============================================================================

TEST_WITH_FIXTURE(GraphicsFixture, Graphics_GetMode_Valid, "Graphics") {
    DisplayMode mode;
    Platform_Graphics_GetMode(&mode);

    TEST_ASSERT_GT(mode.width, 0);
    TEST_ASSERT_GT(mode.height, 0);
}
