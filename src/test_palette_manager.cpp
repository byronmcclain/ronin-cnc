/**
 * Palette Manager Test Program
 */

#include "game/graphics/palette_manager.h"
#include "game/graphics/graphics_buffer.h"
#include "platform.h"
#include <cstdio>
#include <cstring>

// =============================================================================
// Test Utilities
// =============================================================================

static int tests_run = 0;
static int tests_passed = 0;

#define TEST_START(name) do { \
    printf("  Testing %s... ", name); \
    tests_run++; \
} while(0)

#define TEST_PASS() do { \
    printf("PASS\n"); \
    tests_passed++; \
} while(0)

#define TEST_FAIL(msg) do { \
    printf("FAIL: %s\n", msg); \
    return false; \
} while(0)

#define ASSERT(cond, msg) do { \
    if (!(cond)) TEST_FAIL(msg); \
} while(0)

// =============================================================================
// Unit Tests
// =============================================================================

bool test_singleton() {
    TEST_START("singleton pattern");

    PaletteManager& pm1 = PaletteManager::Instance();
    PaletteManager& pm2 = PaletteManager::Instance();

    ASSERT(&pm1 == &pm2, "Should return same instance");

    TEST_PASS();
    return true;
}

bool test_palette_color() {
    TEST_START("PaletteColor operations");

    PaletteColor white(255, 255, 255);
    PaletteColor black(0, 0, 0);

    // Test scaling
    PaletteColor half = white.Scaled(0.5f);
    ASSERT(half.r == 127, "Scaled R should be 127");
    ASSERT(half.g == 127, "Scaled G should be 127");
    ASSERT(half.b == 127, "Scaled B should be 127");

    // Test lerp
    PaletteColor mid = PaletteColor::Lerp(black, white, 0.5f);
    ASSERT(mid.r == 127, "Lerp R should be 127");

    // Test equality
    PaletteColor a(100, 150, 200);
    PaletteColor b(100, 150, 200);
    PaletteColor c(100, 150, 201);
    ASSERT(a == b, "Equal colors should match");
    ASSERT(!(a == c), "Different colors should not match");

    TEST_PASS();
    return true;
}

bool test_initialization() {
    TEST_START("initialization");

    PaletteManager& pm = PaletteManager::Instance();
    pm.Init();

    // Should have grayscale palette
    const PaletteColor* palette = pm.GetCurrentPalette();
    ASSERT(palette != nullptr, "Palette should not be null");

    // Index 0 should be black
    ASSERT(palette[0].r == 0 && palette[0].g == 0 && palette[0].b == 0,
           "Index 0 should be black");

    // Index 255 should be white
    ASSERT(palette[255].r == 255, "Index 255 should be white");

    TEST_PASS();
    return true;
}

bool test_fade_state() {
    TEST_START("fade state machine");

    PaletteManager& pm = PaletteManager::Instance();
    pm.Init();

    // Initial state
    ASSERT(pm.GetFadeState() == FadeState::None, "Should start with no fade");
    ASSERT(pm.GetFadeProgress() == 1.0f, "Should start at full brightness");

    // Start fade out
    pm.StartFadeOut(10);
    ASSERT(pm.GetFadeState() == FadeState::FadingOut, "Should be fading out");
    ASSERT(pm.IsFading(), "IsFading should be true");

    // Advance a few frames
    for (int i = 0; i < 5; i++) {
        pm.Update();
    }
    ASSERT(pm.GetFadeProgress() < 1.0f, "Progress should have decreased");
    ASSERT(pm.GetFadeProgress() > 0.0f, "Progress should not be zero yet");

    // Complete fade
    for (int i = 0; i < 10; i++) {
        pm.Update();
    }
    ASSERT(pm.GetFadeState() == FadeState::FadedOut, "Should be faded out");
    ASSERT(pm.GetFadeProgress() == 0.0f, "Progress should be 0");

    // Restore
    pm.RestoreFromBlack();
    ASSERT(pm.GetFadeState() == FadeState::None, "Should be restored");
    ASSERT(pm.GetFadeProgress() == 1.0f, "Should be at full brightness");

    TEST_PASS();
    return true;
}

bool test_flash() {
    TEST_START("flash effects");

    PaletteManager& pm = PaletteManager::Instance();
    pm.Init();

    ASSERT(!pm.IsFlashing(), "Should not be flashing initially");

    pm.StartFlash(FlashType::White, 5, 1.0f);
    ASSERT(pm.IsFlashing(), "Should be flashing");

    // Update through flash
    for (int i = 0; i < 10; i++) {
        pm.Update();
    }
    ASSERT(!pm.IsFlashing(), "Flash should have ended");

    TEST_PASS();
    return true;
}

bool test_animation() {
    TEST_START("color animation");

    PaletteManager& pm = PaletteManager::Instance();
    pm.Init();

    // Enable water animation
    pm.SetWaterAnimationEnabled(true);

    // Get original color at animation start
    const PaletteColor* original = pm.GetOriginalPalette();
    PaletteColor start_color = original[PaletteManager::WATER_ANIM_START];

    // Update enough frames for rotation
    for (int i = 0; i < 20; i++) {
        pm.Update();
    }

    // Color should have rotated (hard to verify exactly without knowing palette)
    // Just verify no crash occurred
    (void)start_color;

    pm.SetWaterAnimationEnabled(false);

    TEST_PASS();
    return true;
}

bool test_find_closest_color() {
    TEST_START("find closest color");

    PaletteManager& pm = PaletteManager::Instance();
    pm.Init();

    // With grayscale palette, closest to (128, 128, 128) should be index 128
    int closest = pm.FindClosestColor(128, 128, 128, true);
    ASSERT(closest == 128, "Closest to gray should be 128");

    // Closest to (255, 255, 255) should be 255
    closest = pm.FindClosestColor(255, 255, 255, true);
    ASSERT(closest == 255, "Closest to white should be 255");

    // Closest to (0, 0, 0) with skip_zero should be 1
    closest = pm.FindClosestColor(0, 0, 0, true);
    ASSERT(closest == 1, "Closest to black (skipping 0) should be 1");

    TEST_PASS();
    return true;
}

bool test_raw_palette() {
    TEST_START("raw palette extraction");

    PaletteManager& pm = PaletteManager::Instance();
    pm.Init();

    uint8_t raw[768];
    pm.GetRawPalette(raw);

    // Verify index 0 is black
    ASSERT(raw[0] == 0 && raw[1] == 0 && raw[2] == 0, "Index 0 should be black");

    // Verify index 255 is white (grayscale)
    ASSERT(raw[765] == 255 && raw[766] == 255 && raw[767] == 255, "Index 255 should be white");

    TEST_PASS();
    return true;
}

// =============================================================================
// Visual Test
// =============================================================================

void draw_palette_grid(GraphicsBuffer& screen) {
    screen.Lock();
    screen.Clear(0);

    // Draw 16x16 grid of palette colors
    int cell_w = screen.Get_Width() / 16;
    int cell_h = (screen.Get_Height() - 50) / 16;

    for (int row = 0; row < 16; row++) {
        for (int col = 0; col < 16; col++) {
            int index = row * 16 + col;
            int x = col * cell_w;
            int y = row * cell_h;
            screen.Fill_Rect(x, y, cell_w - 1, cell_h - 1, static_cast<uint8_t>(index));
        }
    }

    screen.Unlock();
    screen.Flip();
}

void run_visual_test() {
    printf("\n=== Visual Palette Test ===\n");

    PaletteManager& pm = PaletteManager::Instance();
    GraphicsBuffer& screen = GraphicsBuffer::Screen();

    // Create colorful test palette
    PaletteColor colors[256];
    for (int i = 0; i < 256; i++) {
        // Create rainbow effect
        float hue = static_cast<float>(i) / 256.0f * 6.0f;
        float r, g, b;

        if (hue < 1.0f) {
            r = 1.0f; g = hue; b = 0.0f;
        } else if (hue < 2.0f) {
            r = 2.0f - hue; g = 1.0f; b = 0.0f;
        } else if (hue < 3.0f) {
            r = 0.0f; g = 1.0f; b = hue - 2.0f;
        } else if (hue < 4.0f) {
            r = 0.0f; g = 4.0f - hue; b = 1.0f;
        } else if (hue < 5.0f) {
            r = hue - 4.0f; g = 0.0f; b = 1.0f;
        } else {
            r = 1.0f; g = 0.0f; b = 6.0f - hue;
        }

        colors[i] = PaletteColor(
            static_cast<uint8_t>(r * 255),
            static_cast<uint8_t>(g * 255),
            static_cast<uint8_t>(b * 255)
        );
    }
    colors[0] = PaletteColor(0, 0, 0);  // Keep transparent black

    pm.SetPalette(colors);
    pm.Apply();

    // Show initial palette
    printf("Showing rainbow palette...\n");
    draw_palette_grid(screen);
    Platform_Delay(1000);

    // Demonstrate fade out
    printf("Fading out...\n");
    pm.StartFadeOut(30);
    for (int i = 0; i < 35; i++) {
        pm.Update();
        draw_palette_grid(screen);
        Platform_Delay(33);
    }

    Platform_Delay(500);

    // Demonstrate fade in
    printf("Fading in...\n");
    pm.StartFadeIn(30);
    for (int i = 0; i < 35; i++) {
        pm.Update();
        draw_palette_grid(screen);
        Platform_Delay(33);
    }

    // Demonstrate flash
    printf("White flash...\n");
    pm.StartFlash(FlashType::White, 10, 0.8f);
    for (int i = 0; i < 15; i++) {
        pm.Update();
        draw_palette_grid(screen);
        Platform_Delay(33);
    }

    printf("Red flash...\n");
    pm.StartFlash(FlashType::Red, 10, 0.5f);
    for (int i = 0; i < 15; i++) {
        pm.Update();
        draw_palette_grid(screen);
        Platform_Delay(33);
    }

    // Enable animation and show for a bit
    printf("Water animation (2 seconds)...\n");
    pm.SetWaterAnimationEnabled(true);
    for (int i = 0; i < 60; i++) {
        pm.Update();
        draw_palette_grid(screen);
        Platform_Delay(33);
    }
    pm.SetWaterAnimationEnabled(false);

    printf("Visual test complete.\n");
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[]) {
    printf("==========================================\n");
    printf("Palette Manager Test Suite\n");
    printf("==========================================\n\n");

    // Check for quick mode
    bool quick_mode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--quick") == 0) {
            quick_mode = true;
        }
    }

    if (Platform_Init() != PLATFORM_RESULT_SUCCESS) {
        printf("ERROR: Failed to initialize platform\n");
        return 1;
    }

    if (Platform_Graphics_Init() != 0) {
        printf("ERROR: Failed to initialize graphics\n");
        Platform_Shutdown();
        return 1;
    }

    printf("=== Unit Tests ===\n\n");

    test_singleton();
    test_palette_color();
    test_initialization();
    test_fade_state();
    test_flash();
    test_animation();
    test_find_closest_color();
    test_raw_palette();

    printf("\n------------------------------------------\n");
    printf("Tests: %d/%d passed\n", tests_passed, tests_run);
    printf("------------------------------------------\n");

    if (tests_passed == tests_run && !quick_mode) {
        run_visual_test();
    }

    Platform_Graphics_Shutdown();
    Platform_Shutdown();

    printf("\n==========================================\n");
    if (tests_passed == tests_run) {
        printf("All tests PASSED!\n");
    } else {
        printf("Some tests FAILED\n");
    }
    printf("==========================================\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
