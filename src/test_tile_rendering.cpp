/**
 * Tile Rendering Test Program
 */

#include "game/graphics/tile_renderer.h"
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

bool test_theater_functions() {
    TEST_START("theater helper functions");

    // Test theater names
    ASSERT(strcmp(Theater_Name(THEATER_TEMPERATE), "TEMPERAT") == 0,
           "Temperate name wrong");
    ASSERT(strcmp(Theater_Name(THEATER_SNOW), "SNOW") == 0,
           "Snow name wrong");
    ASSERT(strcmp(Theater_Name(THEATER_INTERIOR), "INTERIOR") == 0,
           "Interior name wrong");

    // Test extensions
    ASSERT(strcmp(Theater_Extension(THEATER_TEMPERATE), ".TMP") == 0,
           "Temperate extension wrong");
    ASSERT(strcmp(Theater_Extension(THEATER_SNOW), ".SNO") == 0,
           "Snow extension wrong");
    ASSERT(strcmp(Theater_Extension(THEATER_INTERIOR), ".INT") == 0,
           "Interior extension wrong");

    // Test invalid theater
    ASSERT(Theater_Name((TheaterType)-1) != nullptr, "Invalid theater should return default");

    TEST_PASS();
    return true;
}

bool test_tile_renderer_singleton() {
    TEST_START("tile renderer singleton");

    TileRenderer& renderer1 = TileRenderer::Instance();
    TileRenderer& renderer2 = TileRenderer::Instance();

    ASSERT(&renderer1 == &renderer2, "Singleton should return same instance");
    ASSERT(!renderer1.IsTheaterLoaded(), "Should not have theater loaded initially");

    TEST_PASS();
    return true;
}

bool test_land_types() {
    TEST_START("land types");

    // Verify land type enum values
    ASSERT(LAND_CLEAR == 0, "LAND_CLEAR should be 0");
    ASSERT(LAND_ROAD == 1, "LAND_ROAD should be 1");
    ASSERT(LAND_WATER == 2, "LAND_WATER should be 2");
    ASSERT(LAND_COUNT == 9, "Should have 9 land types");

    TEST_PASS();
    return true;
}

bool test_template_types() {
    TEST_START("template types");

    ASSERT(TEMPLATE_NONE == -1, "TEMPLATE_NONE should be -1");
    ASSERT(TEMPLATE_CLEAR1 == 0, "TEMPLATE_CLEAR1 should be 0");
    ASSERT(TEMPLATE_COUNT > 0, "Should have positive template count");

    TEST_PASS();
    return true;
}

bool test_overlay_types() {
    TEST_START("overlay types");

    // OverlayType is now defined in cell.h with different values
    ASSERT(OVERLAY_NONE_TYPE == -1, "OVERLAY_NONE_TYPE should be -1");
    ASSERT(OVERLAY_GOLD1 == 0, "OVERLAY_GOLD1 should be 0");  // Resources start at 0
    ASSERT(OVERLAY_SANDBAG == 8, "OVERLAY_SANDBAG should be 8");  // Walls start at 8
    ASSERT(OVERLAY_COUNT > 0, "Should have positive overlay count");

    TEST_PASS();
    return true;
}

bool test_clear_terrain() {
    TEST_START("clear terrain drawing");

    GraphicsBuffer& screen = GraphicsBuffer::Screen();
    screen.Lock();
    screen.Clear(0);

    // Draw some clear terrain cells
    TileRenderer& renderer = TileRenderer::Instance();

    // Even without theater loaded, DrawClear should work (fallback)
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 10; x++) {
            renderer.DrawClear(screen, x * TILE_WIDTH, y * TILE_HEIGHT, x ^ y);
        }
    }

    screen.Unlock();

    // Verify something was drawn
    screen.Lock();
    bool found_nonzero = false;
    for (int y = 0; y < 5 * TILE_HEIGHT && !found_nonzero; y++) {
        for (int x = 0; x < 10 * TILE_WIDTH && !found_nonzero; x++) {
            if (screen.Get_Pixel(x, y) != 0) {
                found_nonzero = true;
            }
        }
    }
    screen.Unlock();

    ASSERT(found_nonzero, "DrawClear should have drawn something");

    TEST_PASS();
    return true;
}

bool test_tile_clipping() {
    TEST_START("tile clipping");

    GraphicsBuffer buf(64, 64);
    buf.Lock();
    buf.Clear(0);

    TileRenderer& renderer = TileRenderer::Instance();

    // Draw partially off-screen tiles (should not crash)
    renderer.DrawClear(buf, -12, 10, 0);   // Left edge
    renderer.DrawClear(buf, 10, -12, 1);   // Top edge
    renderer.DrawClear(buf, 50, 10, 2);    // Right edge
    renderer.DrawClear(buf, 10, 50, 3);    // Bottom edge
    renderer.DrawClear(buf, -100, 10, 4);  // Way off left
    renderer.DrawClear(buf, 10, 200, 5);   // Way off bottom

    buf.Unlock();

    TEST_PASS();
    return true;
}

bool test_cache_operations() {
    TEST_START("cache operations");

    TileRenderer& renderer = TileRenderer::Instance();

    // Get initial cache size
    size_t initial = renderer.GetCacheSize();

    // Clear cache
    renderer.ClearCache();
    ASSERT(renderer.GetCacheSize() == 0, "Cache should be empty after clear");

    // GetTileCount should return 0 for non-existent template
    int count = renderer.GetTileCount(TEMPLATE_CLEAR1);
    // Without MIX files, this will be 0
    (void)count;  // Just verify it doesn't crash

    // GetLandType should return LAND_CLEAR for non-existent
    LandType land = renderer.GetLandType(TEMPLATE_CLEAR1, 0);
    ASSERT(land == LAND_CLEAR, "Should return LAND_CLEAR for missing template");

    TEST_PASS();
    return true;
}

// =============================================================================
// Visual Test
// =============================================================================

void run_visual_test() {
    printf("\n=== Visual Tile Test ===\n");

    // Set up a simple palette
    uint8_t palette[768];
    for (int i = 0; i < 256; i++) {
        palette[i * 3 + 0] = static_cast<uint8_t>(i);
        palette[i * 3 + 1] = static_cast<uint8_t>(i);
        palette[i * 3 + 2] = static_cast<uint8_t>(i);
    }

    // Add some colors for variety
    // Greens for terrain
    for (int i = 16; i < 32; i++) {
        palette[i * 3 + 0] = 0;
        palette[i * 3 + 1] = static_cast<uint8_t>((i - 16) * 8 + 64);
        palette[i * 3 + 2] = 0;
    }
    // Blues for water
    for (int i = 32; i < 48; i++) {
        palette[i * 3 + 0] = 0;
        palette[i * 3 + 1] = 0;
        palette[i * 3 + 2] = static_cast<uint8_t>((i - 32) * 8 + 64);
    }
    // Browns for roads
    for (int i = 48; i < 64; i++) {
        palette[i * 3 + 0] = static_cast<uint8_t>((i - 48) * 4 + 64);
        palette[i * 3 + 1] = static_cast<uint8_t>((i - 48) * 3 + 32);
        palette[i * 3 + 2] = static_cast<uint8_t>((i - 48) * 2);
    }

    PaletteEntry entries[256];
    for (int i = 0; i < 256; i++) {
        entries[i].r = palette[i * 3 + 0];
        entries[i].g = palette[i * 3 + 1];
        entries[i].b = palette[i * 3 + 2];
    }
    Platform_Graphics_SetPalette(entries, 0, 256);

    GraphicsBuffer& screen = GraphicsBuffer::Screen();
    screen.Lock();
    screen.Clear(0);

    printf("Drawing grid of clear terrain tiles...\n");

    TileRenderer& renderer = TileRenderer::Instance();

    // Draw a grid of tiles
    int cols = screen.Get_Width() / TILE_WIDTH;
    int rows = screen.Get_Height() / TILE_HEIGHT;

    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            // Vary the seed for different patterns
            uint32_t seed = x * 31 + y * 17;
            renderer.DrawClear(screen, x * TILE_WIDTH, y * TILE_HEIGHT, seed);
        }
    }

    // Draw some "water" tiles (blue rectangles for visualization)
    for (int y = 5; y < 8; y++) {
        for (int x = 10; x < 15; x++) {
            screen.Fill_Rect(x * TILE_WIDTH, y * TILE_HEIGHT,
                            TILE_WIDTH, TILE_HEIGHT, 40);
        }
    }

    // Draw a "road" (brown rectangles)
    for (int x = 0; x < cols; x++) {
        screen.Fill_Rect(x * TILE_WIDTH, 10 * TILE_HEIGHT,
                        TILE_WIDTH, TILE_HEIGHT, 56);
    }

    // Draw grid lines to show tile boundaries
    for (int y = 0; y <= rows; y++) {
        screen.Draw_HLine(0, y * TILE_HEIGHT, screen.Get_Width(), 100);
    }
    for (int x = 0; x <= cols; x++) {
        screen.Draw_VLine(x * TILE_WIDTH, 0, screen.Get_Height(), 100);
    }

    screen.Unlock();
    screen.Flip();

    printf("Tile grid displayed. Waiting 2 seconds...\n");
    Platform_Delay(2000);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[]) {
    printf("==========================================\n");
    printf("Tile Rendering Test Suite\n");
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

    if (Platform_Graphics_Init() != 0) {
        printf("ERROR: Failed to initialize graphics\n");
        Platform_Shutdown();
        return 1;
    }

    printf("=== Unit Tests ===\n\n");

    test_theater_functions();
    test_tile_renderer_singleton();
    test_land_types();
    test_template_types();
    test_overlay_types();
    test_clear_terrain();
    test_tile_clipping();
    test_cache_operations();

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
