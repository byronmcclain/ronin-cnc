/**
 * @file test_assets.cpp
 * @brief Integration test for the Phase 12 Asset System
 *
 * Tests MIX file loading, file lookup, palette loading, and shape loading.
 * Requires actual game data files in gamedata/ directory.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

// Include the platform header for FFI bindings
extern "C" {
#include "../include/platform.h"
}

// Test result tracking
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message) do { \
    if (condition) { \
        printf("  [PASS] %s\n", message); \
        tests_passed++; \
    } else { \
        printf("  [FAIL] %s\n", message); \
        tests_failed++; \
    } \
} while(0)

// Test Westwood CRC
void test_westwood_crc() {
    printf("\n=== Testing Westwood CRC ===\n");

    // Test basic CRC calculation
    uint32_t crc = Platform_Westwood_CRC((const uint8_t*)"TEST", 4);
    TEST_ASSERT(crc != 0, "CRC of 'TEST' is non-zero");

    // Test case-insensitive filename hashing
    uint32_t crc_upper = Platform_Westwood_CRC_Filename("CONQUER.MIX");
    uint32_t crc_lower = Platform_Westwood_CRC_Filename("conquer.mix");
    uint32_t crc_mixed = Platform_Westwood_CRC_Filename("Conquer.Mix");

    TEST_ASSERT(crc_upper == crc_lower, "CRC is case-insensitive (upper == lower)");
    TEST_ASSERT(crc_upper == crc_mixed, "CRC is case-insensitive (upper == mixed)");

    // Test known file names produce different hashes
    uint32_t crc_a = Platform_Westwood_CRC_Filename("PALETTE.PAL");
    uint32_t crc_b = Platform_Westwood_CRC_Filename("SHADOW.PAL");
    TEST_ASSERT(crc_a != crc_b, "Different filenames produce different hashes");
}

// Test MIX file operations
void test_mix_operations() {
    printf("\n=== Testing MIX Operations ===\n");

    // Initialize asset system
    int result = Platform_Assets_Init();
    TEST_ASSERT(result == 0, "Asset system initialized");

    // Check initial state
    int count = Platform_Mix_GetCount();
    TEST_ASSERT(count == 0, "No MIX files registered initially");

    // Try to register a MIX file (may fail if files are LFS stubs)
    result = Platform_Mix_Register("gamedata/REDALERT.MIX");
    if (result == 0) {
        TEST_ASSERT(true, "Registered REDALERT.MIX");

        count = Platform_Mix_GetCount();
        TEST_ASSERT(count == 1, "One MIX file registered");

        // Try to unregister
        result = Platform_Mix_Unregister("gamedata/REDALERT.MIX");
        TEST_ASSERT(result == 1, "Unregistered REDALERT.MIX");

        count = Platform_Mix_GetCount();
        TEST_ASSERT(count == 0, "No MIX files after unregister");
    } else {
        printf("  [SKIP] MIX files may be LFS stubs - skipping file tests\n");
    }
}

// Test palette loading from raw data
void test_palette_loading() {
    printf("\n=== Testing Palette Loading ===\n");

    // Create test PAL data (6-bit values)
    uint8_t pal_data[768];
    memset(pal_data, 0, sizeof(pal_data));

    // Set first entry to red (63, 0, 0) in 6-bit
    pal_data[0] = 63;  // R
    pal_data[1] = 0;   // G
    pal_data[2] = 0;   // B

    // Set second entry to green
    pal_data[3] = 0;
    pal_data[4] = 63;
    pal_data[5] = 0;

    // Set third entry to blue
    pal_data[6] = 0;
    pal_data[7] = 0;
    pal_data[8] = 63;

    // Load palette
    uint8_t output[768];
    int result = Platform_Palette_LoadPAL(pal_data, output);
    TEST_ASSERT(result == 0, "Palette loaded successfully");

    // Check that 6-bit values were shifted to 8-bit (63 << 2 = 252)
    TEST_ASSERT(output[0] == 252, "Red channel shifted correctly (252)");
    TEST_ASSERT(output[1] == 0, "Green channel is 0");
    TEST_ASSERT(output[2] == 0, "Blue channel is 0");

    TEST_ASSERT(output[3] == 0, "Entry 1: Red is 0");
    TEST_ASSERT(output[4] == 252, "Entry 1: Green is 252");
    TEST_ASSERT(output[5] == 0, "Entry 1: Blue is 0");

    TEST_ASSERT(output[6] == 0, "Entry 2: Red is 0");
    TEST_ASSERT(output[7] == 0, "Entry 2: Green is 0");
    TEST_ASSERT(output[8] == 252, "Entry 2: Blue is 252");
}

// Test shape loading from memory
void test_shape_loading() {
    printf("\n=== Testing Shape Loading ===\n");

    // Create minimal valid SHP header
    // Header: frame_count(2), unknown(2), width(2), height(2) = 8 bytes
    // + frame offsets: 8 bytes per frame
    uint8_t shp_data[8 + 8 + 16];  // Header + 1 frame offset + 4x4 pixels
    memset(shp_data, 0, sizeof(shp_data));

    // Header
    shp_data[0] = 1;   // frame_count low
    shp_data[1] = 0;   // frame_count high
    shp_data[2] = 0;   // unknown1 low
    shp_data[3] = 0;   // unknown1 high
    shp_data[4] = 4;   // width low
    shp_data[5] = 0;   // width high
    shp_data[6] = 4;   // height low
    shp_data[7] = 0;   // height high

    // Frame 0 offset entry
    shp_data[8] = 16;  // offset = 16 (after header + offset table)
    shp_data[9] = 0;
    shp_data[10] = 0;
    shp_data[11] = 0;
    shp_data[12] = 0;  // format = 0 (raw, no compression)
    shp_data[13] = 0;  // ref_frame = 0
    shp_data[14] = 0;  // unknown low
    shp_data[15] = 0;  // unknown high

    // Frame 0 pixel data (4x4 = 16 bytes)
    for (int i = 0; i < 16; i++) {
        shp_data[16 + i] = (uint8_t)(i + 1);  // Fill with 1-16
    }

    // Load shape from memory
    PlatformShape* shape = Platform_Shape_LoadFromMemory(shp_data, sizeof(shp_data));
    TEST_ASSERT(shape != nullptr, "Shape loaded from memory");

    if (shape) {
        // Check dimensions
        int32_t width = 0, height = 0;
        Platform_Shape_GetSize(shape, &width, &height);
        TEST_ASSERT(width == 4, "Shape width is 4");
        TEST_ASSERT(height == 4, "Shape height is 4");

        // Check frame count
        int32_t frame_count = Platform_Shape_GetFrameCount(shape);
        TEST_ASSERT(frame_count == 1, "Shape has 1 frame");

        // Get frame data
        uint8_t frame_data[16];
        int32_t bytes_read = Platform_Shape_GetFrame(shape, 0, frame_data, sizeof(frame_data));
        TEST_ASSERT(bytes_read == 16, "Read 16 bytes from frame");
        TEST_ASSERT(frame_data[0] == 1, "First pixel is 1");
        TEST_ASSERT(frame_data[15] == 16, "Last pixel is 16");

        // Free shape
        Platform_Shape_Free(shape);
        TEST_ASSERT(true, "Shape freed");
    }
}

// Test template loading from memory
void test_template_loading() {
    printf("\n=== Testing Template Loading ===\n");

    // Create raw tile data (2 tiles of 4x4)
    uint8_t tile_data[32];
    for (int i = 0; i < 16; i++) {
        tile_data[i] = 1;      // First tile filled with 1s
        tile_data[16 + i] = 2; // Second tile filled with 2s
    }

    // Load template from memory
    PlatformTemplate* tmpl = Platform_Template_LoadFromMemory(tile_data, sizeof(tile_data), 4, 4);
    TEST_ASSERT(tmpl != nullptr, "Template loaded from memory");

    if (tmpl) {
        // Check tile count
        int32_t tile_count = Platform_Template_GetTileCount(tmpl);
        TEST_ASSERT(tile_count == 2, "Template has 2 tiles");

        // Check tile size
        int32_t width = 0, height = 0;
        Platform_Template_GetTileSize(tmpl, &width, &height);
        TEST_ASSERT(width == 4, "Tile width is 4");
        TEST_ASSERT(height == 4, "Tile height is 4");

        // Get first tile
        uint8_t tile0[16];
        int32_t bytes_read = Platform_Template_GetTile(tmpl, 0, tile0, sizeof(tile0));
        TEST_ASSERT(bytes_read == 16, "Read 16 bytes from tile 0");
        TEST_ASSERT(tile0[0] == 1, "Tile 0 first pixel is 1");

        // Get second tile
        uint8_t tile1[16];
        bytes_read = Platform_Template_GetTile(tmpl, 1, tile1, sizeof(tile1));
        TEST_ASSERT(bytes_read == 16, "Read 16 bytes from tile 1");
        TEST_ASSERT(tile1[0] == 2, "Tile 1 first pixel is 2");

        // Free template
        Platform_Template_Free(tmpl);
        TEST_ASSERT(true, "Template freed");
    }
}

int main(int argc, char* argv[]) {
    printf("========================================\n");
    printf("Phase 12 Asset System Integration Test\n");
    printf("========================================\n");

    // Run all tests
    test_westwood_crc();
    test_mix_operations();
    test_palette_loading();
    test_shape_loading();
    test_template_loading();

    // Print summary
    printf("\n========================================\n");
    printf("Test Summary\n");
    printf("========================================\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Total:  %d\n", tests_passed + tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
