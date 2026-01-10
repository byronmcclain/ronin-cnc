// src/tests/performance/test_loading.cpp
// Asset Loading Performance Tests
// Task 18f - Performance Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "perf_utils.h"
#include "platform.h"
#include <cstring>

//=============================================================================
// Helper Functions
//=============================================================================

static bool RegisterGameMix() {
    // Try common MIX file locations
    if (Platform_Mix_Register("REDALERT.MIX") == 0) return true;
    if (Platform_Mix_Register("gamedata/REDALERT.MIX") == 0) return true;
    if (Platform_Mix_Register("data/REDALERT.MIX") == 0) return true;
    return false;
}

//=============================================================================
// MIX File Access Tests
//=============================================================================

TEST_WITH_FIXTURE(AssetFixture, Perf_Loading_MixOpen, "Performance") {
    // Time how long it takes to open MIX file
    PerfTimer timer;
    timer.Start();

    bool opened = RegisterGameMix();

    timer.Stop();

    if (!opened) {
        TEST_SKIP("Game MIX file not available");
    }

    char msg[128];
    snprintf(msg, sizeof(msg), "MIX open: %u ms", timer.ElapsedMs());
    Platform_Log(LOG_LEVEL_INFO, msg);

    // Should be quick
    TEST_ASSERT_LT(timer.ElapsedMs(), 1000u);
}

TEST_WITH_FIXTURE(AssetFixture, Perf_Loading_PaletteLoad, "Performance") {
    if (!RegisterGameMix()) {
        TEST_SKIP("Game MIX file not available");
    }

    // Check if palette exists
    if (!Platform_Mix_Exists("TEMPERAT.PAL")) {
        TEST_SKIP("Palette not found in MIX");
    }

    const int ITERATIONS = 100;
    int32_t pal_size = Platform_Mix_GetSize("TEMPERAT.PAL");

    PerfTimer timer;
    timer.Start();

    for (int i = 0; i < ITERATIONS; i++) {
        // Load palette data
        uint8_t pal[768];
        Platform_Mix_Read("TEMPERAT.PAL", pal, pal_size < 768 ? pal_size : 768);
    }

    timer.Stop();

    char msg[128];
    snprintf(msg, sizeof(msg),
             "Palette load: %d iterations in %u ms (%.2f ms each)",
             ITERATIONS, timer.ElapsedMs(),
             static_cast<float>(timer.ElapsedMs()) / ITERATIONS);
    Platform_Log(LOG_LEVEL_INFO, msg);

    // Should be very fast
    TEST_ASSERT_LT(timer.ElapsedMs(), 500u);
}

TEST_WITH_FIXTURE(AssetFixture, Perf_Loading_ShapeLoad, "Performance") {
    if (!RegisterGameMix()) {
        TEST_SKIP("Game MIX file not available");
    }

    // Check if shape exists
    if (!Platform_Mix_Exists("E1.SHP")) {
        TEST_SKIP("Shape not found in MIX");
    }

    const int ITERATIONS = 50;
    int32_t shape_size = Platform_Mix_GetSize("E1.SHP");

    PerfTimer timer;
    timer.Start();

    for (int i = 0; i < ITERATIONS; i++) {
        // Load shape data
        uint8_t* shape_data = new uint8_t[shape_size];
        Platform_Mix_Read("E1.SHP", shape_data, shape_size);
        delete[] shape_data;
    }

    timer.Stop();

    char msg[128];
    snprintf(msg, sizeof(msg),
             "Shape load: %d iterations in %u ms",
             ITERATIONS, timer.ElapsedMs());
    Platform_Log(LOG_LEVEL_INFO, msg);

    // May be slower if loading from disk each time
    TEST_ASSERT_LT(timer.ElapsedMs(), 2000u);
}

//=============================================================================
// Simulated Loading Tests (no game data required)
//=============================================================================

TEST_CASE(Perf_Loading_FileRead, "Performance") {
    // Test raw file reading performance
    const int BUFFER_SIZE = 64 * 1024;  // 64 KB
    const int ITERATIONS = 100;

    uint8_t* buffer = static_cast<uint8_t*>(malloc(BUFFER_SIZE));
    TEST_ASSERT(buffer != nullptr);

    // Create test data
    for (int i = 0; i < BUFFER_SIZE; i++) {
        buffer[i] = static_cast<uint8_t>(i & 0xFF);
    }

    PerfTimer timer;
    timer.Start();

    // Simulate file parsing
    for (int iter = 0; iter < ITERATIONS; iter++) {
        // Read header
        int offset = 0;
        int count = buffer[offset] | (buffer[offset + 1] << 8);
        offset += 2;

        // Process entries
        for (int i = 0; i < count && offset < BUFFER_SIZE - 8; i++) {
            int entry_id = buffer[offset] | (buffer[offset + 1] << 8);
            int entry_size = buffer[offset + 4] | (buffer[offset + 5] << 8);
            offset += 8;
            (void)entry_id;  // Suppress unused warning
            (void)entry_size;
        }
    }

    timer.Stop();
    free(buffer);

    char msg[128];
    snprintf(msg, sizeof(msg),
             "File parsing: %d iterations in %u ms",
             ITERATIONS, timer.ElapsedMs());
    Platform_Log(LOG_LEVEL_INFO, msg);

    TEST_ASSERT_LT(timer.ElapsedMs(), 100u);
}

TEST_CASE(Perf_Loading_DecompressRLE, "Performance") {
    // Test RLE decompression speed (common in game data)
    const int COMPRESSED_SIZE = 10000;
    const int ITERATIONS = 100;

    // Create compressed data
    uint8_t* compressed = static_cast<uint8_t*>(malloc(COMPRESSED_SIZE));
    uint8_t* decompressed = static_cast<uint8_t*>(malloc(COMPRESSED_SIZE * 4));
    TEST_ASSERT(compressed != nullptr);
    TEST_ASSERT(decompressed != nullptr);

    // Fill with RLE-like data (alternating runs and literals)
    for (int i = 0; i < COMPRESSED_SIZE; i += 2) {
        compressed[i] = static_cast<uint8_t>(10 + (i % 20));  // Run length
        compressed[i + 1] = static_cast<uint8_t>(i & 0xFF);   // Value
    }

    PerfTimer timer;
    timer.Start();

    for (int iter = 0; iter < ITERATIONS; iter++) {
        int src_idx = 0;
        int dst_idx = 0;

        while (src_idx < COMPRESSED_SIZE - 1 && dst_idx < COMPRESSED_SIZE * 4) {
            int count = compressed[src_idx++];
            uint8_t value = compressed[src_idx++];

            for (int i = 0; i < count && dst_idx < COMPRESSED_SIZE * 4; i++) {
                decompressed[dst_idx++] = value;
            }
        }
    }

    timer.Stop();

    free(compressed);
    free(decompressed);

    char msg[128];
    snprintf(msg, sizeof(msg),
             "RLE decompress: %d iterations in %u ms",
             ITERATIONS, timer.ElapsedMs());
    Platform_Log(LOG_LEVEL_INFO, msg);

    TEST_ASSERT_LT(timer.ElapsedMs(), 200u);
}

TEST_CASE(Perf_Loading_DecompressLCW, "Performance") {
    // Test LCW-style decompression (used in C&C)
    // Simplified simulation of format 80 decompression

    const int DATA_SIZE = 5000;
    const int ITERATIONS = 100;

    uint8_t* src = static_cast<uint8_t*>(malloc(DATA_SIZE));
    uint8_t* dst = static_cast<uint8_t*>(malloc(DATA_SIZE * 4));
    TEST_ASSERT(src != nullptr);
    TEST_ASSERT(dst != nullptr);

    // Create mock compressed data
    for (int i = 0; i < DATA_SIZE; i++) {
        src[i] = static_cast<uint8_t>(i & 0xFF);
    }

    PerfTimer timer;
    timer.Start();

    for (int iter = 0; iter < ITERATIONS; iter++) {
        int s = 0, d = 0;

        while (s < DATA_SIZE - 2 && d < DATA_SIZE * 4 - 10) {
            uint8_t cmd = src[s++];

            if (cmd == 0) {
                // End marker
                break;
            } else if (cmd < 0x80) {
                // Copy literal bytes
                int count = cmd;
                for (int i = 0; i < count && s < DATA_SIZE && d < DATA_SIZE * 4; i++) {
                    dst[d++] = src[s++];
                }
            } else {
                // Copy from earlier in output
                int count = (cmd & 0x3F) + 3;
                int offset = (s < DATA_SIZE) ? (src[s++] + 1) : 1;

                int copy_from = d - offset;
                if (copy_from < 0) copy_from = 0;

                for (int i = 0; i < count && d < DATA_SIZE * 4; i++) {
                    dst[d++] = dst[copy_from + (i % offset)];
                }
            }
        }
    }

    timer.Stop();

    free(src);
    free(dst);

    char msg[128];
    snprintf(msg, sizeof(msg),
             "LCW decompress: %d iterations in %u ms",
             ITERATIONS, timer.ElapsedMs());
    Platform_Log(LOG_LEVEL_INFO, msg);

    TEST_ASSERT_LT(timer.ElapsedMs(), 200u);
}

//=============================================================================
// Initialization Tests
//=============================================================================

TEST_WITH_FIXTURE(PlatformFixture, Perf_Loading_PlatformInit, "Performance") {
    // Platform is already initialized by fixture
    // Just verify it's working
    TEST_ASSERT(fixture.IsInitialized());

    // Log success
    Platform_Log(LOG_LEVEL_INFO, "Platform init: verified working");
}

TEST_WITH_FIXTURE(GraphicsFixture, Perf_Loading_GraphicsInit, "Performance") {
    if (!fixture.IsInitialized()) {
        TEST_SKIP("Graphics not initialized");
    }

    int width = fixture.GetWidth();
    int height = fixture.GetHeight();

    char msg[128];
    snprintf(msg, sizeof(msg),
             "Graphics init: %dx%d verified", width, height);
    Platform_Log(LOG_LEVEL_INFO, msg);

    TEST_ASSERT_GT(width, 0);
    TEST_ASSERT_GT(height, 0);
}
