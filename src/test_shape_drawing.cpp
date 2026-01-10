/**
 * Shape Drawing Test Program
 *
 * Tests the ShapeRenderer class with various drawing modes.
 */

#include "game/graphics/shape_renderer.h"
#include "game/graphics/graphics_buffer.h"
#include "game/graphics/remap_tables.h"
#include "platform.h"
#include <cstdio>
#include <cstring>
#include <vector>

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

bool test_shape_loading() {
    TEST_START("shape loading");

    ShapeRenderer renderer;
    ASSERT(!renderer.IsLoaded(), "Should not be loaded initially");

    // Test unload is safe on not-loaded
    renderer.Unload();
    ASSERT(!renderer.IsLoaded(), "Should still not be loaded");

    TEST_PASS();
    return true;
}

bool test_shape_cache() {
    TEST_START("shape cache");

    ShapeCache& cache = ShapeCache::Instance();

    // Clear any existing cache
    cache.Clear();
    ASSERT(cache.GetCount() == 0, "Cache should be empty after clear");

    // Note: Can't test actual loading without real SHP files
    // Just verify cache operations work

    TEST_PASS();
    return true;
}

bool test_remap_tables() {
    TEST_START("remap tables");

    // Initialize with a simple grayscale palette
    uint8_t palette[768];
    for (int i = 0; i < 256; i++) {
        palette[i * 3 + 0] = static_cast<uint8_t>(i);
        palette[i * 3 + 1] = static_cast<uint8_t>(i);
        palette[i * 3 + 2] = static_cast<uint8_t>(i);
    }

    InitRemapTables(palette);
    ASSERT(AreRemapTablesInitialized(), "Tables should be initialized");

    // Test identity table
    const uint8_t* identity = GetIdentityTable();
    ASSERT(identity != nullptr, "Identity table should exist");
    for (int i = 0; i < 256; i++) {
        if (identity[i] != i) {
            TEST_FAIL("Identity should map to self");
        }
    }

    // Test house tables exist
    for (int h = 0; h < HOUSE_COLOR_COUNT; h++) {
        const uint8_t* house_table = GetHouseRemapTable(h);
        ASSERT(house_table != nullptr, "House table should exist");
    }

    // Test shadow table
    const uint8_t* shadow = GetShadowTable();
    ASSERT(shadow != nullptr, "Shadow table should exist");
    ASSERT(shadow[0] == 0, "Shadow should preserve transparent");

    // Test fade tables
    const uint8_t* fade = GetFadeTables();
    ASSERT(fade != nullptr, "Fade tables should exist");
    // Level 0 should be near-identity
    ASSERT(fade[128] > 0, "Fade level 0 should not be black");
    // Level 15 should be dark
    ASSERT(fade[15 * 256 + 128] < 128, "Fade level 15 should be dark");

    TEST_PASS();
    return true;
}

bool test_draw_flags() {
    TEST_START("draw flags");

    // Test flag combinations
    uint32_t flags = SHAPE_NORMAL;
    ASSERT(flags == 0, "SHAPE_NORMAL should be 0");

    flags = SHAPE_CENTER | SHAPE_FLIP_X;
    ASSERT((flags & SHAPE_CENTER) != 0, "CENTER flag should be set");
    ASSERT((flags & SHAPE_FLIP_X) != 0, "FLIP_X flag should be set");
    ASSERT((flags & SHAPE_GHOST) == 0, "GHOST flag should not be set");

    TEST_PASS();
    return true;
}

bool test_shape_renderer_api() {
    TEST_START("shape renderer API");

    ShapeRenderer renderer;

    // Test initial state
    ASSERT(renderer.GetFrameCount() == 0, "Frame count should be 0");
    ASSERT(renderer.GetWidth() == 0, "Width should be 0");
    ASSERT(renderer.GetHeight() == 0, "Height should be 0");
    ASSERT(renderer.GetName()[0] == '\0', "Name should be empty");

    // Test cache operations
    renderer.ClearCache();  // Should not crash
    ASSERT(renderer.GetCacheSize() == 0, "Cache should be empty");

    // Test move semantics
    ShapeRenderer renderer2 = std::move(renderer);
    ASSERT(!renderer2.IsLoaded(), "Moved renderer should not be loaded");

    TEST_PASS();
    return true;
}

// =============================================================================
// Visual Test
// =============================================================================

void run_visual_test() {
    printf("\n=== Visual Shape Test ===\n");

    // Create a test shape in memory (simple 32x32 sprite)
    const int SIZE = 32;
    std::vector<uint8_t> test_sprite(SIZE * SIZE);

    // Draw a simple pattern: circle with house-color-range fill
    for (int y = 0; y < SIZE; y++) {
        for (int x = 0; x < SIZE; x++) {
            int dx = x - SIZE / 2;
            int dy = y - SIZE / 2;
            int dist_sq = dx * dx + dy * dy;
            int radius_sq = (SIZE / 2 - 2) * (SIZE / 2 - 2);

            if (dist_sq < radius_sq) {
                // Inside circle - use house color range
                int shade = (dist_sq * 16) / radius_sq;
                test_sprite[y * SIZE + x] = static_cast<uint8_t>(80 + shade);  // House color range
            } else {
                test_sprite[y * SIZE + x] = 0;  // Transparent
            }
        }
    }

    // Set up palette
    uint8_t palette[768];
    for (int i = 0; i < 256; i++) {
        palette[i * 3 + 0] = static_cast<uint8_t>(i);
        palette[i * 3 + 1] = static_cast<uint8_t>(i);
        palette[i * 3 + 2] = static_cast<uint8_t>(i);
    }

    // Add some color to house range
    for (int i = 80; i <= 95; i++) {
        int shade = (i - 80) * 16;
        palette[i * 3 + 0] = static_cast<uint8_t>(shade);
        palette[i * 3 + 1] = static_cast<uint8_t>(shade / 2);
        palette[i * 3 + 2] = static_cast<uint8_t>(shade);
    }

    PaletteEntry entries[256];
    for (int i = 0; i < 256; i++) {
        entries[i].r = palette[i * 3 + 0];
        entries[i].g = palette[i * 3 + 1];
        entries[i].b = palette[i * 3 + 2];
    }
    Platform_Graphics_SetPalette(entries, 0, 256);
    InitRemapTables(palette);

    // Get screen buffer
    GraphicsBuffer& screen = GraphicsBuffer::Screen();
    screen.Lock();
    screen.Clear(32);  // Dark gray background

    // Draw test sprites with different house colors
    printf("Drawing test sprites with house colors...\n");

    for (int h = 0; h < HOUSE_COLOR_COUNT; h++) {
        int x = 50 + (h % 4) * 100;
        int y = 50 + (h / 4) * 100;

        const uint8_t* remap = GetHouseRemapTable(h);

        // Draw sprite with remapping
        for (int sy = 0; sy < SIZE; sy++) {
            for (int sx = 0; sx < SIZE; sx++) {
                uint8_t pixel = test_sprite[sy * SIZE + sx];
                if (pixel != 0) {
                    screen.Put_Pixel(x + sx, y + sy, remap[pixel]);
                }
            }
        }
    }

    // Draw with shadow effect
    printf("Drawing shadow sprites...\n");
    const uint8_t* shadow = GetShadowTable();
    for (int sy = 0; sy < SIZE; sy++) {
        for (int sx = 0; sx < SIZE; sx++) {
            uint8_t pixel = test_sprite[sy * SIZE + sx];
            if (pixel != 0) {
                // Shadow is offset and uses shadow table on background
                int x = 450 + sx + 4;
                int y = 50 + sy + 4;
                uint8_t bg = screen.Get_Pixel(x, y);
                screen.Put_Pixel(x, y, shadow[bg]);
            }
        }
    }

    // Draw the actual sprite over shadow
    for (int sy = 0; sy < SIZE; sy++) {
        for (int sx = 0; sx < SIZE; sx++) {
            uint8_t pixel = test_sprite[sy * SIZE + sx];
            if (pixel != 0) {
                screen.Put_Pixel(450 + sx, 50 + sy, pixel);
            }
        }
    }

    screen.Unlock();
    screen.Flip();

    printf("Visual test displayed. Waiting 2 seconds...\n");
    Platform_Delay(2000);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[]) {
    printf("==========================================\n");
    printf("Shape Drawing Test Suite\n");
    printf("==========================================\n\n");

    // Check for quick mode
    bool quick_mode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--quick") == 0) {
            quick_mode = true;
        }
    }

    // Initialize platform
    if (Platform_Init() != PLATFORM_RESULT_SUCCESS) {
        printf("ERROR: Failed to initialize platform\n");
        return 1;
    }

    // Initialize graphics
    if (Platform_Graphics_Init() != 0) {
        printf("ERROR: Failed to initialize graphics\n");
        Platform_Shutdown();
        return 1;
    }

    printf("=== Unit Tests ===\n\n");

    // Run unit tests
    test_shape_loading();
    test_shape_cache();
    test_remap_tables();
    test_draw_flags();
    test_shape_renderer_api();

    printf("\n------------------------------------------\n");
    printf("Tests: %d/%d passed\n", tests_passed, tests_run);
    printf("------------------------------------------\n");

    // Run visual test if all unit tests passed and not in quick mode
    if (tests_passed == tests_run && !quick_mode) {
        run_visual_test();
    }

    // Cleanup
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
