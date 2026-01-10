# Task 18d: Visual Tests

| Field | Value |
|-------|-------|
| **Task ID** | 18d |
| **Task Name** | Visual Tests |
| **Phase** | 18 - Testing & Polish |
| **Depends On** | Tasks 18a-18c (Test Framework, Unit/Integration Tests) |
| **Estimated Complexity** | Medium (~500 lines) |

---

## Dependencies

- Task 18a (Test Framework) - Test infrastructure
- Task 18c (Integration Tests) - Graphics integration
- Graphics system functional
- Asset system loading palettes and shapes
- Game data files available for visual verification

---

## Context

Visual tests verify that graphics rendering produces correct output. Unlike unit tests which check logic, visual tests verify:

1. **Palette Rendering**: All 256 colors display correctly
2. **Shape Rendering**: Sprites draw correctly with transparency
3. **Screen Modes**: All supported resolutions work
4. **Animation**: Frame sequences render smoothly
5. **UI Elements**: Menus, buttons, sidebar render correctly
6. **Reference Comparison**: Output matches expected images

Visual tests may generate screenshots for manual review or automated comparison.

---

## Technical Background

### Visual Test Categories

| Category | Description | Verification |
|----------|-------------|--------------|
| Palette | Color reproduction | All 256 colors visible |
| Shapes | Sprite rendering | Correct pixels, transparency |
| Compositing | Layered rendering | Z-order, blending |
| Animation | Frame sequences | Smooth playback |
| Resolution | Different screen sizes | No distortion |
| Reference | Known-good comparison | Pixel match or threshold |

### Screenshot Capture

```cpp
void CaptureScreenshot(const char* filename) {
    uint8_t* buffer;
    int32_t w, h, p;
    Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);

    // Convert to PNG/BMP and save
    SaveImage(filename, buffer, w, h, p);
}
```

### Reference Image Comparison

```cpp
bool CompareImages(const uint8_t* a, const uint8_t* b,
                   int width, int height, int threshold) {
    int differences = 0;
    for (int i = 0; i < width * height; i++) {
        if (std::abs(a[i] - b[i]) > threshold) {
            differences++;
        }
    }
    return differences < (width * height * 0.01);  // < 1% different
}
```

---

## Objective

Create visual tests that:
1. Render all palettes and verify color accuracy
2. Draw shapes at various positions and verify output
3. Test different screen resolutions
4. Verify animation sequences
5. Generate reference screenshots
6. Compare output against reference images

---

## Deliverables

- [ ] `src/tests/visual/test_palettes.cpp` - Palette rendering tests
- [ ] `src/tests/visual/test_shapes.cpp` - Shape rendering tests
- [ ] `src/tests/visual/test_resolution.cpp` - Resolution tests
- [ ] `src/tests/visual/test_animation.cpp` - Animation tests
- [ ] `src/tests/visual/screenshot_utils.cpp` - Screenshot utilities
- [ ] `src/tests/visual_tests_main.cpp` - Main entry point
- [ ] `reference/` directory for reference images
- [ ] CMakeLists.txt updated
- [ ] All tests pass

---

## Files to Create

### src/tests/visual/screenshot_utils.h

```cpp
// src/tests/visual/screenshot_utils.h
// Screenshot capture and comparison utilities
// Task 18d - Visual Tests

#ifndef SCREENSHOT_UTILS_H
#define SCREENSHOT_UTILS_H

#include <cstdint>
#include <string>
#include <vector>

//=============================================================================
// Screenshot Capture
//=============================================================================

/// Capture current backbuffer to file
bool Screenshot_Capture(const char* filename);

/// Capture to memory buffer
bool Screenshot_CaptureToBuffer(std::vector<uint8_t>& buffer,
                                 int& width, int& height);

//=============================================================================
// Image Comparison
//=============================================================================

struct ImageCompareResult {
    bool match;              // Within threshold
    int total_pixels;        // Total pixels compared
    int different_pixels;    // Pixels exceeding threshold
    float difference_ratio;  // Ratio of different pixels
    int max_difference;      // Maximum single-pixel difference
};

/// Compare two image buffers
ImageCompareResult Screenshot_Compare(
    const uint8_t* a, const uint8_t* b,
    int width, int height,
    int threshold = 5);      // Per-pixel threshold

/// Compare captured buffer against reference file
ImageCompareResult Screenshot_CompareToReference(
    const char* reference_file,
    int threshold = 5);

//=============================================================================
// Reference Image Management
//=============================================================================

/// Generate reference image from current frame
bool Screenshot_SaveReference(const char* name);

/// Load reference image
bool Screenshot_LoadReference(const char* name,
                               std::vector<uint8_t>& buffer,
                               int& width, int& height);

/// Get reference image path
std::string Screenshot_GetReferencePath(const char* name);

//=============================================================================
// Visual Test Macros
//=============================================================================

#define VISUAL_ASSERT_MATCHES_REFERENCE(name) \
    do { \
        auto result = Screenshot_CompareToReference(name); \
        if (!result.match) { \
            std::ostringstream _ss; \
            _ss << "Visual mismatch: " << (result.difference_ratio * 100) \
                << "% different (max diff: " << result.max_difference << ")"; \
            throw TestAssertionFailed(_ss.str(), __FILE__, __LINE__); \
        } \
    } while (0)

#define VISUAL_GENERATE_REFERENCE(name) \
    Screenshot_SaveReference(name)

#endif // SCREENSHOT_UTILS_H
```

### src/tests/visual/screenshot_utils.cpp

```cpp
// src/tests/visual/screenshot_utils.cpp
// Screenshot utilities implementation
// Task 18d - Visual Tests

#include "screenshot_utils.h"
#include "platform.h"
#include <fstream>
#include <cstring>
#include <algorithm>

// Simple BMP file writing (no external dependencies)
static bool WriteBMP(const char* filename, const uint8_t* data,
                     int width, int height, const uint32_t* palette) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) return false;

    // BMP header
    uint8_t header[54] = {
        'B', 'M',           // Signature
        0, 0, 0, 0,         // File size (filled later)
        0, 0, 0, 0,         // Reserved
        54 + 1024, 0, 0, 0, // Offset to pixel data (header + palette)
        40, 0, 0, 0,        // DIB header size
        0, 0, 0, 0,         // Width (filled later)
        0, 0, 0, 0,         // Height (filled later)
        1, 0,               // Planes
        8, 0,               // Bits per pixel
        0, 0, 0, 0,         // Compression
        0, 0, 0, 0,         // Image size
        0, 0, 0, 0,         // X pixels per meter
        0, 0, 0, 0,         // Y pixels per meter
        0, 1, 0, 0,         // Colors used (256)
        0, 0, 0, 0          // Important colors
    };

    // Fill in dimensions
    *(uint32_t*)&header[18] = width;
    *(uint32_t*)&header[22] = height;

    // Row padding (BMP rows must be multiple of 4 bytes)
    int row_padding = (4 - (width % 4)) % 4;
    int row_size = width + row_padding;
    int image_size = row_size * height;

    // File size
    *(uint32_t*)&header[2] = 54 + 1024 + image_size;

    file.write(reinterpret_cast<char*>(header), 54);

    // Write palette (BGRA format)
    for (int i = 0; i < 256; i++) {
        uint32_t color = palette ? palette[i] : ((i << 16) | (i << 8) | i);
        uint8_t bgra[4] = {
            static_cast<uint8_t>(color & 0xFF),         // Blue
            static_cast<uint8_t>((color >> 8) & 0xFF),  // Green
            static_cast<uint8_t>((color >> 16) & 0xFF), // Red
            0                                            // Alpha
        };
        file.write(reinterpret_cast<char*>(bgra), 4);
    }

    // Write pixel data (bottom-up)
    std::vector<uint8_t> row(row_size, 0);
    for (int y = height - 1; y >= 0; y--) {
        memcpy(row.data(), data + y * width, width);
        file.write(reinterpret_cast<char*>(row.data()), row_size);
    }

    return true;
}

bool Screenshot_Capture(const char* filename) {
    uint8_t* buffer;
    int32_t w, h, p;

    if (Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p) != 0) {
        return false;
    }

    // Get current palette
    uint32_t palette[256];
    Platform_Graphics_GetPalette(palette, 256);

    // Copy to contiguous buffer if pitch != width
    std::vector<uint8_t> contiguous;
    const uint8_t* src = buffer;

    if (p != w) {
        contiguous.resize(w * h);
        for (int y = 0; y < h; y++) {
            memcpy(contiguous.data() + y * w, buffer + y * p, w);
        }
        src = contiguous.data();
    }

    return WriteBMP(filename, src, w, h, palette);
}

bool Screenshot_CaptureToBuffer(std::vector<uint8_t>& out_buffer,
                                 int& width, int& height) {
    uint8_t* buffer;
    int32_t w, h, p;

    if (Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p) != 0) {
        return false;
    }

    width = w;
    height = h;
    out_buffer.resize(w * h);

    for (int y = 0; y < h; y++) {
        memcpy(out_buffer.data() + y * w, buffer + y * p, w);
    }

    return true;
}

ImageCompareResult Screenshot_Compare(const uint8_t* a, const uint8_t* b,
                                       int width, int height, int threshold) {
    ImageCompareResult result = {};
    result.total_pixels = width * height;

    for (int i = 0; i < result.total_pixels; i++) {
        int diff = std::abs(static_cast<int>(a[i]) - static_cast<int>(b[i]));
        result.max_difference = std::max(result.max_difference, diff);

        if (diff > threshold) {
            result.different_pixels++;
        }
    }

    result.difference_ratio =
        static_cast<float>(result.different_pixels) / result.total_pixels;
    result.match = result.difference_ratio < 0.01f;  // < 1% different

    return result;
}

std::string Screenshot_GetReferencePath(const char* name) {
    return std::string("reference/") + name + ".bmp";
}

bool Screenshot_SaveReference(const char* name) {
    std::string path = Screenshot_GetReferencePath(name);
    return Screenshot_Capture(path.c_str());
}

ImageCompareResult Screenshot_CompareToReference(const char* reference_file,
                                                  int threshold) {
    ImageCompareResult result = {};
    result.match = false;

    // Capture current
    std::vector<uint8_t> current;
    int cur_w, cur_h;
    if (!Screenshot_CaptureToBuffer(current, cur_w, cur_h)) {
        return result;
    }

    // Load reference
    std::vector<uint8_t> reference;
    int ref_w, ref_h;
    if (!Screenshot_LoadReference(reference_file, reference, ref_w, ref_h)) {
        return result;
    }

    // Size mismatch
    if (cur_w != ref_w || cur_h != ref_h) {
        result.different_pixels = cur_w * cur_h;
        result.total_pixels = cur_w * cur_h;
        result.difference_ratio = 1.0f;
        return result;
    }

    return Screenshot_Compare(current.data(), reference.data(),
                              cur_w, cur_h, threshold);
}

bool Screenshot_LoadReference(const char* name,
                               std::vector<uint8_t>& buffer,
                               int& width, int& height) {
    std::string path = Screenshot_GetReferencePath(name);

    // Simple BMP loading
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;

    // Read header
    uint8_t header[54];
    file.read(reinterpret_cast<char*>(header), 54);

    if (header[0] != 'B' || header[1] != 'M') {
        return false;
    }

    width = *reinterpret_cast<int32_t*>(&header[18]);
    height = *reinterpret_cast<int32_t*>(&header[22]);

    if (width <= 0 || height <= 0 || width > 4096 || height > 4096) {
        return false;
    }

    // Skip palette
    file.seekg(54 + 1024);

    // Read pixel data
    int row_padding = (4 - (width % 4)) % 4;
    buffer.resize(width * height);

    for (int y = height - 1; y >= 0; y--) {
        file.read(reinterpret_cast<char*>(buffer.data() + y * width), width);
        file.seekg(row_padding, std::ios::cur);
    }

    return true;
}
```

### src/tests/visual/test_palettes.cpp

```cpp
// src/tests/visual/test_palettes.cpp
// Palette Rendering Visual Tests
// Task 18d - Visual Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "screenshot_utils.h"
#include "platform.h"
#include "game/mix_manager.h"
#include "game/palette.h"

//=============================================================================
// Palette Gradient Tests
//=============================================================================

TEST_WITH_FIXTURE(GraphicsFixture, Visual_Palette_Gradient, "Visual") {
    if (!fixture.IsInitialized()) TEST_SKIP("Graphics not initialized");

    uint8_t* buffer = fixture.GetBackBuffer();
    int width = fixture.GetWidth();
    int height = fixture.GetHeight();
    int pitch = fixture.GetPitch();

    // Draw 256 vertical stripes
    int stripe_width = width / 256;
    for (int x = 0; x < width; x++) {
        uint8_t color = x / stripe_width;
        if (color > 255) color = 255;

        for (int y = 0; y < height; y++) {
            buffer[y * pitch + x] = color;
        }
    }

    fixture.RenderFrame();
    Screenshot_Capture("visual_palette_gradient.bmp");
}

TEST_WITH_FIXTURE(GraphicsFixture, Visual_Palette_Grid, "Visual") {
    if (!fixture.IsInitialized()) TEST_SKIP("Graphics not initialized");

    uint8_t* buffer = fixture.GetBackBuffer();
    int width = fixture.GetWidth();
    int height = fixture.GetHeight();
    int pitch = fixture.GetPitch();

    // 16x16 grid of all 256 colors
    int cell_w = width / 16;
    int cell_h = height / 16;

    for (int cy = 0; cy < 16; cy++) {
        for (int cx = 0; cx < 16; cx++) {
            uint8_t color = cy * 16 + cx;

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

TEST_CASE(Visual_Palette_Temperate, "Visual") {
    Platform_Init();
    GraphicsConfig gfx = {640, 480, false, false};
    Platform_Graphics_Init(&gfx);

    MixManager& mix = MixManager::Instance();
    mix.Initialize();

    if (!mix.LoadMix("REDALERT.MIX")) {
        mix.Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        TEST_SKIP("Game data not found");
    }

    PaletteData palette;
    if (palette.LoadFromMix("TEMPERAT.PAL")) {
        Platform_Graphics_SetPalette(palette.GetRGBData(), 256);

        // Draw color grid
        uint8_t* buffer;
        int32_t w, h, p;
        Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);

        int cell_w = w / 16;
        int cell_h = h / 16;

        for (int cy = 0; cy < 16; cy++) {
            for (int cx = 0; cx < 16; cx++) {
                uint8_t color = cy * 16 + cx;
                for (int y = cy * cell_h; y < (cy + 1) * cell_h; y++) {
                    for (int x = cx * cell_w; x < (cx + 1) * cell_w; x++) {
                        buffer[y * p + x] = color;
                    }
                }
            }
        }

        Platform_Graphics_Flip();
        Screenshot_Capture("visual_palette_temperate.bmp");
    }

    mix.Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

TEST_CASE(Visual_Palette_AllTheaters, "Visual") {
    Platform_Init();
    GraphicsConfig gfx = {640, 480, false, false};
    Platform_Graphics_Init(&gfx);

    MixManager& mix = MixManager::Instance();
    mix.Initialize();

    if (!mix.LoadMix("REDALERT.MIX")) {
        mix.Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        TEST_SKIP("Game data not found");
    }

    const char* palette_names[] = {
        "TEMPERAT.PAL", "SNOW.PAL", "INTERIOR.PAL"
    };

    for (const char* pal_name : palette_names) {
        PaletteData palette;
        if (!palette.LoadFromMix(pal_name)) continue;

        Platform_Graphics_SetPalette(palette.GetRGBData(), 256);

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

        std::string filename = "visual_palette_";
        filename += pal_name;
        filename = filename.substr(0, filename.find('.')) + ".bmp";
        Screenshot_Capture(filename.c_str());

        Platform_Timer_Delay(500);
    }

    mix.Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}
```

### src/tests/visual/test_shapes.cpp

```cpp
// src/tests/visual/test_shapes.cpp
// Shape Rendering Visual Tests
// Task 18d - Visual Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "screenshot_utils.h"
#include "platform.h"
#include "game/mix_manager.h"
#include "game/palette.h"
#include "game/shape.h"

//=============================================================================
// Shape Rendering Tests
//=============================================================================

TEST_CASE(Visual_Shape_Mouse, "Visual") {
    Platform_Init();
    GraphicsConfig gfx = {640, 480, false, false};
    Platform_Graphics_Init(&gfx);

    MixManager& mix = MixManager::Instance();
    mix.Initialize();

    if (!mix.LoadMix("REDALERT.MIX")) {
        mix.Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        TEST_SKIP("Game data not found");
    }

    // Load palette
    PaletteData palette;
    if (!palette.LoadFromMix("TEMPERAT.PAL")) {
        mix.Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        TEST_SKIP("Palette not found");
    }
    Platform_Graphics_SetPalette(palette.GetRGBData(), 256);

    // Load shape
    ShapeData shape;
    if (!shape.LoadFromMix("MOUSE.SHP")) {
        mix.Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        TEST_SKIP("MOUSE.SHP not found");
    }

    uint8_t* buffer;
    int32_t w, h, p;
    Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);

    // Clear with dark color
    memset(buffer, 32, h * p);

    // Draw all mouse cursor frames in a grid
    int cols = 8;
    int spacing = 50;
    int frame_count = shape.GetFrameCount();

    for (int i = 0; i < frame_count; i++) {
        int col = i % cols;
        int row = i / cols;
        int x = 50 + col * spacing;
        int y = 50 + row * spacing;

        shape.DrawFrame(buffer, p, w, h, i, x, y);
    }

    Platform_Graphics_Flip();
    Screenshot_Capture("visual_shape_mouse.bmp");

    mix.Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

TEST_CASE(Visual_Shape_Animation, "Visual") {
    Platform_Init();
    GraphicsConfig gfx = {640, 480, false, false};
    Platform_Graphics_Init(&gfx);

    MixManager& mix = MixManager::Instance();
    mix.Initialize();

    if (!mix.LoadMix("REDALERT.MIX")) {
        mix.Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        TEST_SKIP("Game data not found");
    }

    PaletteData palette;
    palette.LoadFromMix("TEMPERAT.PAL");
    Platform_Graphics_SetPalette(palette.GetRGBData(), 256);

    ShapeData shape;
    if (!shape.LoadFromMix("MOUSE.SHP")) {
        mix.Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        TEST_SKIP("Shape not found");
    }

    uint8_t* buffer;
    int32_t w, h, p;
    Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);

    // Animate through frames
    int frame_count = shape.GetFrameCount();
    for (int i = 0; i < frame_count * 3; i++) {  // 3 loops
        int frame = i % frame_count;

        memset(buffer, 32, h * p);

        int x = w / 2 - shape.GetWidth() / 2;
        int y = h / 2 - shape.GetHeight() / 2;

        shape.DrawFrame(buffer, p, w, h, frame, x, y);

        Platform_Graphics_Flip();
        Platform_Timer_Delay(100);
    }

    mix.Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}
```

### src/tests/visual/test_resolution.cpp

```cpp
// src/tests/visual/test_resolution.cpp
// Resolution Visual Tests
// Task 18d - Visual Tests

#include "test/test_framework.h"
#include "screenshot_utils.h"
#include "platform.h"

//=============================================================================
// Resolution Tests
//=============================================================================

TEST_CASE(Visual_Resolution_640x480, "Visual") {
    Platform_Init();

    GraphicsConfig config = {640, 480, false, false};
    TEST_ASSERT_EQ(Platform_Graphics_Init(&config), 0);

    uint8_t* buffer;
    int32_t w, h, p;
    Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);

    TEST_ASSERT_EQ(w, 640);
    TEST_ASSERT_EQ(h, 480);

    // Draw test pattern
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            buffer[y * p + x] = (x ^ y) & 0xFF;
        }
    }

    Platform_Graphics_Flip();
    Screenshot_Capture("visual_res_640x480.bmp");

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

TEST_CASE(Visual_Resolution_800x600, "Visual") {
    Platform_Init();

    GraphicsConfig config = {800, 600, false, false};
    int result = Platform_Graphics_Init(&config);

    if (result != 0) {
        Platform_Shutdown();
        TEST_SKIP("800x600 not supported");
    }

    uint8_t* buffer;
    int32_t w, h, p;
    Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);

    // Draw test pattern
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            buffer[y * p + x] = (x + y) & 0xFF;
        }
    }

    Platform_Graphics_Flip();
    Screenshot_Capture("visual_res_800x600.bmp");

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

TEST_CASE(Visual_Resolution_Fullscreen, "Visual") {
    Platform_Init();

    GraphicsConfig config = {640, 480, true, false};  // Fullscreen
    int result = Platform_Graphics_Init(&config);

    if (result != 0) {
        Platform_Shutdown();
        TEST_SKIP("Fullscreen not available");
    }

    uint8_t* buffer;
    int32_t w, h, p;
    Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);

    // Draw test pattern
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            buffer[y * p + x] = 100;
        }
    }

    Platform_Graphics_Flip();
    Platform_Timer_Delay(1000);

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}
```

### src/tests/visual/test_animation.cpp

```cpp
// src/tests/visual/test_animation.cpp
// Animation Visual Tests
// Task 18d - Visual Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "screenshot_utils.h"
#include "platform.h"
#include <cmath>

//=============================================================================
// Animation Tests
//=============================================================================

TEST_WITH_FIXTURE(GraphicsFixture, Visual_Animation_Smoothness, "Visual") {
    if (!fixture.IsInitialized()) TEST_SKIP("Graphics not initialized");

    int width = fixture.GetWidth();
    int height = fixture.GetHeight();

    // Animate a moving box
    const int FRAMES = 120;
    const int BOX_SIZE = 50;

    for (int frame = 0; frame < FRAMES; frame++) {
        uint8_t* buffer = fixture.GetBackBuffer();
        int pitch = fixture.GetPitch();

        // Clear
        for (int y = 0; y < height; y++) {
            memset(buffer + y * pitch, 32, width);
        }

        // Calculate box position (smooth sine motion)
        float t = frame / 60.0f;  // Time in seconds
        int box_x = (width - BOX_SIZE) / 2 +
                    static_cast<int>(200 * std::sin(t * 3.14159f));
        int box_y = (height - BOX_SIZE) / 2 +
                    static_cast<int>(100 * std::sin(t * 3.14159f * 2));

        // Draw box
        for (int y = box_y; y < box_y + BOX_SIZE && y < height; y++) {
            if (y < 0) continue;
            for (int x = box_x; x < box_x + BOX_SIZE && x < width; x++) {
                if (x < 0) continue;
                buffer[y * pitch + x] = 200;
            }
        }

        fixture.RenderFrame();
        Platform_Timer_Delay(16);  // 60fps
    }
}

TEST_WITH_FIXTURE(GraphicsFixture, Visual_Animation_ColorCycle, "Visual") {
    if (!fixture.IsInitialized()) TEST_SKIP("Graphics not initialized");

    // Animate palette cycling
    const int FRAMES = 256;

    uint32_t base_palette[256];
    for (int i = 0; i < 256; i++) {
        base_palette[i] = (i << 16) | (i << 8) | i;  // Grayscale
    }

    for (int frame = 0; frame < FRAMES; frame++) {
        // Rotate palette
        uint32_t rotated[256];
        for (int i = 0; i < 256; i++) {
            rotated[i] = base_palette[(i + frame) % 256];
        }
        Platform_Graphics_SetPalette(rotated, 256);

        // Draw gradient
        uint8_t* buffer = fixture.GetBackBuffer();
        int pitch = fixture.GetPitch();
        int width = fixture.GetWidth();
        int height = fixture.GetHeight();

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                buffer[y * pitch + x] = x & 0xFF;
            }
        }

        fixture.RenderFrame();
        Platform_Timer_Delay(16);
    }
}

TEST_WITH_FIXTURE(GraphicsFixture, Visual_Animation_FrameRate, "Visual") {
    if (!fixture.IsInitialized()) TEST_SKIP("Graphics not initialized");

    int width = fixture.GetWidth();
    int height = fixture.GetHeight();

    const int TEST_FRAMES = 300;  // 5 seconds at 60fps
    uint32_t start = Platform_Timer_GetTicks();

    for (int frame = 0; frame < TEST_FRAMES; frame++) {
        uint8_t* buffer = fixture.GetBackBuffer();
        int pitch = fixture.GetPitch();

        // Simple render
        uint8_t color = (frame * 2) & 0xFF;
        for (int y = 0; y < height; y++) {
            memset(buffer + y * pitch, color, width);
        }

        fixture.RenderFrame();
        Platform_Timer_Delay(16);
    }

    uint32_t elapsed = Platform_Timer_GetTicks() - start;
    float actual_fps = (TEST_FRAMES * 1000.0f) / elapsed;

    Platform_Log("Animation test: %d frames in %u ms = %.1f fps",
                 TEST_FRAMES, elapsed, actual_fps);

    // Should be close to 60fps
    TEST_ASSERT_GT(actual_fps, 50.0f);
}
```

### src/tests/visual_tests_main.cpp

```cpp
// src/tests/visual_tests_main.cpp
// Visual Tests Main Entry Point
// Task 18d - Visual Tests

#include "test/test_framework.h"

TEST_MAIN()
```

---

## CMakeLists.txt Additions

```cmake
# Task 18d: Visual Tests

set(VISUAL_TEST_SOURCES
    ${CMAKE_SOURCE_DIR}/src/tests/visual/screenshot_utils.cpp
    ${CMAKE_SOURCE_DIR}/src/tests/visual/test_palettes.cpp
    ${CMAKE_SOURCE_DIR}/src/tests/visual/test_shapes.cpp
    ${CMAKE_SOURCE_DIR}/src/tests/visual/test_resolution.cpp
    ${CMAKE_SOURCE_DIR}/src/tests/visual/test_animation.cpp
    ${CMAKE_SOURCE_DIR}/src/tests/visual_tests_main.cpp
)

add_executable(VisualTests ${VISUAL_TEST_SOURCES})
target_link_libraries(VisualTests PRIVATE
    test_framework redalert_platform redalert_game
)
target_include_directories(VisualTests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/tests/visual
)

add_test(NAME VisualTests COMMAND VisualTests)

# Create reference directory
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/reference)
```

---

## Verification Script

```bash
#!/bin/bash
set -e
echo "=== Task 18d Verification: Visual Tests ==="

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"

echo "Checking files..."
for f in src/tests/visual/screenshot_utils.cpp \
         src/tests/visual/test_palettes.cpp \
         src/tests/visual/test_shapes.cpp; do
    [ -f "$ROOT_DIR/$f" ] || { echo "Missing: $f"; exit 1; }
    echo "  $f"
done

echo "Building..."
cd "$BUILD_DIR" && cmake --build . --target VisualTests
echo "  Built"

echo "Running visual tests..."
"$BUILD_DIR/VisualTests" --verbose || { echo "Visual tests failed"; exit 1; }

echo "Checking screenshots..."
ls -la "$BUILD_DIR"/*.bmp 2>/dev/null || echo "No screenshots generated"

echo "=== Task 18d Verification PASSED ==="
```

---

## Success Criteria

- [ ] Palette tests render all 256 colors
- [ ] Shape tests draw sprites correctly
- [ ] Resolution tests work at multiple resolutions
- [ ] Animation tests run smoothly
- [ ] Screenshots are captured successfully
- [ ] Reference comparison works (when references exist)

---

## Completion Promise

When verification passes, output:
<promise>TASK_18D_COMPLETE</promise>

## Escape Hatch

If stuck after 15 iterations, document in `ralph-tasks/blocked/18d.md` and output:
<promise>TASK_18D_BLOCKED</promise>

## Max Iterations
15
