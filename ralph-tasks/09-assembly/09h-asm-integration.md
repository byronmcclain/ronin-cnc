# Task 09h: Assembly Replacement Integration Test

## Dependencies
- Tasks 09a-09g must be complete
- All assembly replacement modules implemented and passing unit tests

## Context
This task creates a comprehensive C++ integration test that verifies all the assembly replacement functions work correctly when called through the FFI layer. This is similar to `test_audio.cpp` from Phase 08 - it exercises the complete pipeline from C++ through FFI to Rust.

## Objective
Create `src/test_asm.cpp` that tests:
1. Buffer operations (clear, fill, blit, transparent blit)
2. Palette remapping
3. Scaling (1x, 2x, 0.5x)
4. Shadow drawing
5. LCW compression/decompression
6. CRC32 calculation
7. Random number generation

## Deliverables
- [ ] `src/test_asm.cpp` - Comprehensive integration test
- [ ] `CMakeLists.txt` updated with TestAsm target
- [ ] All tests pass
- [ ] Verification passes

## Test Strategy

### Test Categories
1. **Basic Correctness**: Do functions produce expected output?
2. **Edge Cases**: Empty buffers, zero dimensions, negative coordinates
3. **Boundary Conditions**: Maximum sizes, clipping at all edges
4. **Round-trip**: Compress/decompress, remap with identity table
5. **Known Values**: CRC32 with standard test vectors

### Visual Inspection (Optional)
For buffer operations, the test can optionally write output to files for visual inspection of patterns.

## Files to Create/Modify

### src/test_asm.cpp (NEW)
```cpp
// Assembly replacement integration test
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <vector>
#include <cmath>

extern "C" {
#include "platform.h"
}

// Test helper macros
#define TEST(name) printf("Test: %s... ", name)
#define PASS() do { printf("PASSED\n"); return 0; } while(0)
#define FAIL(msg) do { printf("FAILED: %s\n", msg); return 1; } while(0)

// =============================================================================
// Buffer Operations Tests
// =============================================================================

int test_buffer_clear() {
    TEST("Buffer_Clear");

    uint8_t buffer[100];
    memset(buffer, 0xFF, sizeof(buffer));

    Platform_Buffer_Clear(buffer, 100, 0x42);

    for (int i = 0; i < 100; i++) {
        if (buffer[i] != 0x42) FAIL("Clear didn't fill correctly");
    }

    PASS();
}

int test_buffer_fill_rect() {
    TEST("Buffer_FillRect");

    uint8_t buffer[100];  // 10x10
    memset(buffer, 0, sizeof(buffer));

    Platform_Buffer_FillRect(buffer, 10, 10, 10, 2, 2, 4, 4, 0xFF);

    // Check filled region
    for (int y = 0; y < 10; y++) {
        for (int x = 0; x < 10; x++) {
            uint8_t expected = (x >= 2 && x < 6 && y >= 2 && y < 6) ? 0xFF : 0;
            if (buffer[y * 10 + x] != expected) {
                printf("at (%d,%d): got %d, expected %d\n", x, y, buffer[y*10+x], expected);
                FAIL("FillRect incorrect");
            }
        }
    }

    PASS();
}

int test_buffer_fill_rect_clipping() {
    TEST("Buffer_FillRect (clipping)");

    uint8_t buffer[100];
    memset(buffer, 0, sizeof(buffer));

    // Fill rect that extends off left and top edges
    Platform_Buffer_FillRect(buffer, 10, 10, 10, -2, -2, 5, 5, 0xFF);

    // Should only fill (0,0) to (2,2)
    for (int y = 0; y < 10; y++) {
        for (int x = 0; x < 10; x++) {
            uint8_t expected = (x < 3 && y < 3) ? 0xFF : 0;
            if (buffer[y * 10 + x] != expected) {
                FAIL("Clipped FillRect incorrect");
            }
        }
    }

    PASS();
}

int test_buffer_blit() {
    TEST("Buffer_Blit");

    uint8_t src[25];   // 5x5 source
    uint8_t dest[100]; // 10x10 dest
    memset(src, 0xAB, sizeof(src));
    memset(dest, 0, sizeof(dest));

    Platform_Buffer_Blit(
        dest, 10, 10, 10,
        src, 5, 5, 5,
        2, 2,    // dest position
        0, 0, 5, 5  // source rect
    );

    // Check blitted region
    for (int y = 0; y < 10; y++) {
        for (int x = 0; x < 10; x++) {
            uint8_t expected = (x >= 2 && x < 7 && y >= 2 && y < 7) ? 0xAB : 0;
            if (dest[y * 10 + x] != expected) {
                FAIL("Blit incorrect");
            }
        }
    }

    PASS();
}

int test_buffer_blit_trans() {
    TEST("Buffer_BlitTrans");

    // Source with checkerboard of 0 and 1
    uint8_t src[25];
    for (int i = 0; i < 25; i++) {
        src[i] = (i % 2) ? 1 : 0;  // 0, 1, 0, 1, ...
    }

    uint8_t dest[100];
    memset(dest, 0xFF, sizeof(dest));

    Platform_Buffer_BlitTrans(
        dest, 10, 10, 10,
        src, 5, 5, 5,
        0, 0,
        0, 0, 5, 5
    );

    // Where source is 0, dest should still be 0xFF
    // Where source is 1, dest should be 1
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            int i = y * 5 + x;
            uint8_t expected = src[i] ? 1 : 0xFF;
            if (dest[y * 10 + x] != expected) {
                FAIL("BlitTrans incorrect");
            }
        }
    }

    PASS();
}

// =============================================================================
// Palette Remap Tests
// =============================================================================

int test_buffer_remap() {
    TEST("Buffer_Remap");

    uint8_t buffer[100];
    for (int i = 0; i < 100; i++) buffer[i] = 1;  // All 1s

    // Create remap table: 1 -> 42
    uint8_t remap[256];
    for (int i = 0; i < 256; i++) remap[i] = i;  // Identity
    remap[1] = 42;

    Platform_Buffer_Remap(buffer, 10, 0, 0, 10, 10, remap);

    for (int i = 0; i < 100; i++) {
        if (buffer[i] != 42) FAIL("Remap incorrect");
    }

    PASS();
}

int test_buffer_remap_trans() {
    TEST("Buffer_RemapTrans");

    // Buffer with 0 and 1 values
    uint8_t buffer[100];
    for (int i = 0; i < 100; i++) {
        buffer[i] = (i % 2) ? 1 : 0;
    }

    uint8_t remap[256];
    for (int i = 0; i < 256; i++) remap[i] = i;
    remap[1] = 42;
    // remap[0] = 99, but shouldn't be applied

    Platform_Buffer_RemapTrans(buffer, 10, 0, 0, 10, 10, remap);

    for (int i = 0; i < 100; i++) {
        uint8_t expected = (i % 2) ? 42 : 0;  // 0 stays 0
        if (buffer[i] != expected) FAIL("RemapTrans incorrect");
    }

    PASS();
}

// =============================================================================
// Scale Tests
// =============================================================================

int test_buffer_scale_1x() {
    TEST("Buffer_Scale (1x)");

    uint8_t src[16] = {  // 4x4
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        13, 14, 15, 16
    };
    uint8_t dest[64];  // 8x8
    memset(dest, 0, sizeof(dest));

    Platform_Buffer_Scale(
        dest, 8, 8, 8,
        src, 4, 4, 4,
        2, 2,     // dest position
        4, 4      // scale to 4x4 (1:1)
    );

    // Check that pixels match at offset
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (dest[(y+2) * 8 + (x+2)] != src[y * 4 + x]) {
                FAIL("Scale 1x incorrect");
            }
        }
    }

    PASS();
}

int test_buffer_scale_2x() {
    TEST("Buffer_Scale (2x magnification)");

    uint8_t src[4] = {1, 2, 3, 4};  // 2x2
    uint8_t dest[16];  // 4x4
    memset(dest, 0, sizeof(dest));

    Platform_Buffer_Scale(
        dest, 4, 4, 4,
        src, 2, 2, 2,
        0, 0,
        4, 4  // Scale 2x2 to 4x4
    );

    // Each source pixel should become 2x2 block
    // Expected: [1,1,2,2], [1,1,2,2], [3,3,4,4], [3,3,4,4]
    uint8_t expected[16] = {
        1, 1, 2, 2,
        1, 1, 2, 2,
        3, 3, 4, 4,
        3, 3, 4, 4
    };

    if (memcmp(dest, expected, 16) != 0) {
        printf("Got: ");
        for (int i = 0; i < 16; i++) printf("%d ", dest[i]);
        printf("\n");
        FAIL("Scale 2x incorrect");
    }

    PASS();
}

int test_buffer_scale_trans() {
    TEST("Buffer_ScaleTrans");

    uint8_t src[4] = {0, 1, 1, 0};  // 2x2 with transparency
    uint8_t dest[16];
    memset(dest, 0xFF, sizeof(dest));

    Platform_Buffer_ScaleTrans(
        dest, 4, 4, 4,
        src, 2, 2, 2,
        0, 0,
        4, 4
    );

    // Where source was 0, dest should remain 0xFF
    if (dest[0] != 0xFF || dest[1] != 0xFF || dest[4] != 0xFF || dest[5] != 0xFF) {
        FAIL("ScaleTrans didn't preserve transparency");
    }

    // Where source was 1, dest should be 1
    if (dest[2] != 1 || dest[3] != 1 || dest[6] != 1 || dest[7] != 1) {
        FAIL("ScaleTrans didn't copy opaque pixels");
    }

    PASS();
}

// =============================================================================
// Shadow Tests
// =============================================================================

int test_buffer_shadow() {
    TEST("Buffer_Shadow");

    uint8_t buffer[100];
    for (int i = 0; i < 100; i++) buffer[i] = 100;

    // Shadow table: halve each value
    uint8_t shadow[256];
    for (int i = 0; i < 256; i++) shadow[i] = i / 2;

    Platform_Buffer_Shadow(buffer, 10, 2, 2, 4, 4, shadow);

    // Check shadowed region
    for (int y = 0; y < 10; y++) {
        for (int x = 0; x < 10; x++) {
            uint8_t expected = (x >= 2 && x < 6 && y >= 2 && y < 6) ? 50 : 100;
            if (buffer[y * 10 + x] != expected) {
                FAIL("Shadow incorrect");
            }
        }
    }

    PASS();
}

// =============================================================================
// LCW Compression Tests
// =============================================================================

int test_lcw_round_trip() {
    TEST("LCW round-trip");

    const char* test_data = "Hello, World! This is a test of LCW compression.";
    int data_len = strlen(test_data);

    int max_comp_size = Platform_LCW_MaxCompressedSize(data_len);
    std::vector<uint8_t> compressed(max_comp_size);
    std::vector<uint8_t> decompressed(data_len + 100);

    int comp_len = Platform_LCW_Compress(
        (const uint8_t*)test_data, data_len,
        compressed.data(), max_comp_size
    );

    if (comp_len <= 0) FAIL("Compression failed");

    int decomp_len = Platform_LCW_Uncompress(
        compressed.data(), comp_len,
        decompressed.data(), decompressed.size()
    );

    if (decomp_len != data_len) {
        printf("decomp_len=%d, expected=%d\n", decomp_len, data_len);
        FAIL("Decompression length mismatch");
    }

    if (memcmp(test_data, decompressed.data(), data_len) != 0) {
        FAIL("Decompression data mismatch");
    }

    printf("(orig=%d, comp=%d) ", data_len, comp_len);
    PASS();
}

int test_lcw_repetitive_data() {
    TEST("LCW with repetitive data");

    // Data with lots of repetition should compress well
    std::vector<uint8_t> test_data(1000);
    for (int i = 0; i < 1000; i++) {
        test_data[i] = 'A' + (i % 4);  // ABCDABCDABCD...
    }

    int max_comp_size = Platform_LCW_MaxCompressedSize(1000);
    std::vector<uint8_t> compressed(max_comp_size);
    std::vector<uint8_t> decompressed(1000);

    int comp_len = Platform_LCW_Compress(
        test_data.data(), 1000,
        compressed.data(), max_comp_size
    );

    if (comp_len <= 0) FAIL("Compression failed");

    int decomp_len = Platform_LCW_Uncompress(
        compressed.data(), comp_len,
        decompressed.data(), 1000
    );

    if (decomp_len != 1000) FAIL("Decompression length mismatch");
    if (memcmp(test_data.data(), decompressed.data(), 1000) != 0) {
        FAIL("Decompression data mismatch");
    }

    printf("(1000->%d) ", comp_len);
    PASS();
}

// =============================================================================
// CRC32 Tests
// =============================================================================

int test_crc32_known_value() {
    TEST("CRC32 known value");

    // Standard test vector: CRC32("123456789") = 0xCBF43926
    const char* test_str = "123456789";
    uint32_t crc = Platform_CRC32((const uint8_t*)test_str, 9);

    if (crc != 0xCBF43926) {
        printf("got 0x%08X, expected 0xCBF43926\n", crc);
        FAIL("CRC32 incorrect");
    }

    PASS();
}

int test_crc32_streaming() {
    TEST("CRC32 streaming");

    const char* data = "Hello, World!";
    int len = strlen(data);

    // One-shot
    uint32_t crc_oneshot = Platform_CRC32((const uint8_t*)data, len);

    // Streaming
    uint32_t crc_stream = Platform_CRC32_Init();
    crc_stream = Platform_CRC32_Update(crc_stream, (const uint8_t*)data, 7);  // "Hello, "
    crc_stream = Platform_CRC32_Update(crc_stream, (const uint8_t*)(data + 7), len - 7);  // "World!"
    crc_stream = Platform_CRC32_Finalize(crc_stream);

    if (crc_oneshot != crc_stream) {
        FAIL("Streaming CRC differs from one-shot");
    }

    PASS();
}

// =============================================================================
// Random Tests
// =============================================================================

int test_random_deterministic() {
    TEST("Random deterministic");

    Platform_Random_Seed(12345);
    uint32_t seq1[10];
    for (int i = 0; i < 10; i++) {
        seq1[i] = Platform_Random_Get();
    }

    Platform_Random_Seed(12345);
    for (int i = 0; i < 10; i++) {
        if (Platform_Random_Get() != seq1[i]) {
            FAIL("Random sequence not deterministic");
        }
    }

    PASS();
}

int test_random_range() {
    TEST("Random range");

    Platform_Random_Seed(42);

    for (int i = 0; i < 1000; i++) {
        int val = Platform_Random_Range(10, 20);
        if (val < 10 || val > 20) {
            printf("got %d, expected [10,20]\n", val);
            FAIL("Random_Range out of bounds");
        }
    }

    PASS();
}

int test_random_distribution() {
    TEST("Random distribution");

    Platform_Random_Seed(999);

    // Check that we get varied values
    int counts[8] = {0};
    for (int i = 0; i < 1000; i++) {
        int val = Platform_Random_Range(0, 7);
        counts[val]++;
    }

    // Each bucket should have roughly 125 hits (1000/8)
    // Allow wide variance since it's random
    for (int i = 0; i < 8; i++) {
        if (counts[i] < 50 || counts[i] > 200) {
            printf("bucket %d has %d (expected ~125)\n", i, counts[i]);
            FAIL("Distribution seems off");
        }
    }

    PASS();
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[]) {
    printf("=== Assembly Replacement Integration Test ===\n\n");

    int failures = 0;

    // Buffer tests
    printf("--- Buffer Operations ---\n");
    failures += test_buffer_clear();
    failures += test_buffer_fill_rect();
    failures += test_buffer_fill_rect_clipping();
    failures += test_buffer_blit();
    failures += test_buffer_blit_trans();

    // Remap tests
    printf("\n--- Palette Remapping ---\n");
    failures += test_buffer_remap();
    failures += test_buffer_remap_trans();

    // Scale tests
    printf("\n--- Scaling ---\n");
    failures += test_buffer_scale_1x();
    failures += test_buffer_scale_2x();
    failures += test_buffer_scale_trans();

    // Shadow tests
    printf("\n--- Shadow ---\n");
    failures += test_buffer_shadow();

    // LCW tests
    printf("\n--- LCW Compression ---\n");
    failures += test_lcw_round_trip();
    failures += test_lcw_repetitive_data();

    // CRC32 tests
    printf("\n--- CRC32 ---\n");
    failures += test_crc32_known_value();
    failures += test_crc32_streaming();

    // Random tests
    printf("\n--- Random ---\n");
    failures += test_random_deterministic();
    failures += test_random_range();
    failures += test_random_distribution();

    // Summary
    printf("\n=== Test Summary ===\n");
    if (failures == 0) {
        printf("All tests PASSED\n");
        return 0;
    } else {
        printf("%d test(s) FAILED\n", failures);
        return 1;
    }
}
```

### CMakeLists.txt (MODIFY)
Add after the TestAudio target:
```cmake
# =============================================================================
# Assembly Replacement Integration Test
# =============================================================================

add_executable(TestAsm src/test_asm.cpp)

target_include_directories(TestAsm PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

add_dependencies(TestAsm generate_headers)

target_link_libraries(TestAsm PRIVATE redalert_platform)

if(APPLE)
    target_link_libraries(TestAsm PRIVATE
        "-framework CoreFoundation"
        "-framework Security"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework Carbon"
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework ForceFeedback"
        "-framework CoreVideo"
        "-framework Metal"
        "-framework QuartzCore"
        "-framework GameController"
        "-framework CoreHaptics"
        iconv
    )
endif()
```

## Verification Command
```bash
ralph-tasks/verify/check-asm-integration.sh
```

## Implementation Steps
1. Create `src/test_asm.cpp` with all integration tests
2. Update `CMakeLists.txt` with TestAsm target
3. Build: `cmake --build build --target TestAsm`
4. Run: `./build/TestAsm`
5. All tests should pass
6. Run verification script

## Success Criteria
- All buffer operation tests pass
- All remap tests pass
- All scale tests pass (1x, 2x, transparent)
- Shadow test passes
- LCW round-trip tests pass
- CRC32 matches known values
- Random is deterministic and in-range
- Exit code 0

## Common Issues

### Link errors
- Ensure all FFI exports are declared in header
- Check that platform library is built
- Verify cbindgen ran (header has functions)

### Test failures after code changes
- Run unit tests first to isolate issue
- Check specific failing function
- Verify FFI signatures match

### CRC32 test fails
- Verify polynomial and initial/final XOR values
- Check test vector is exactly "123456789"

## Completion Promise
When verification passes, output:
```
<promise>TASK_09H_COMPLETE</promise>
```

## Escape Hatch
If stuck after 15 iterations:
- Document what's blocking in `ralph-tasks/blocked/09h.md`
- List which specific tests are failing
- Output: `<promise>TASK_09H_BLOCKED</promise>`

## Max Iterations
15
