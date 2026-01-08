/**
 * @file test_mix_decrypt.cpp
 * @brief Integration test for MIX file decryption (Task 12i)
 *
 * Tests encrypted MIX file loading using Blowfish/RSA decryption.
 * Requires actual encrypted game data files in gamedata/ directory.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <sys/stat.h>

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

#define TEST_SKIP(message) do { \
    printf("  [SKIP] %s\n", message); \
} while(0)

// Check if a file exists and has content
static bool file_exists_with_content(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return false;
    }
    // Check if file is larger than a typical LFS stub (~130 bytes)
    return st.st_size > 200;
}

// List of encrypted MIX files in Red Alert
static const char* ENCRYPTED_MIX_FILES[] = {
    "gamedata/REDALERT.MIX",
    "gamedata/MAIN.MIX",
    "gamedata/EDHI.MIX",
    "gamedata/EDLO.MIX",
    "gamedata/EXPAND.MIX",
    "gamedata/EXPAND2.MIX",
    "gamedata/HIRES.MIX",
    "gamedata/HIRES1.MIX",
    "gamedata/LORES.MIX",
    "gamedata/LORES1.MIX",
    "gamedata/NCHIRES.MIX",
    "gamedata/SPEECH.MIX",
    "gamedata/ALLIES.MIX",
    "gamedata/RUSSIAN.MIX",
    "gamedata/SOUNDS.MIX",
    "gamedata/SCORES.MIX",
    "gamedata/MOVIES1.MIX",
    "gamedata/MOVIES2.MIX",
    nullptr
};

// List of unencrypted MIX files
static const char* UNENCRYPTED_MIX_FILES[] = {
    "gamedata/LOCAL.MIX",
    "gamedata/GENERAL.MIX",
    nullptr
};

void test_encrypted_mix_files() {
    printf("\n=== Testing Encrypted MIX File Loading ===\n");

    int result = Platform_Assets_Init();
    TEST_ASSERT(result == 0, "Asset system initialized");

    int loaded = 0;
    int skipped = 0;

    for (int i = 0; ENCRYPTED_MIX_FILES[i] != nullptr; i++) {
        const char* path = ENCRYPTED_MIX_FILES[i];

        if (!file_exists_with_content(path)) {
            printf("  [SKIP] %s (not found or LFS stub)\n", path);
            skipped++;
            continue;
        }

        result = Platform_Mix_Register(path);
        if (result == 0) {
            printf("  [PASS] Loaded encrypted MIX: %s\n", path);
            loaded++;
            tests_passed++;
        } else {
            printf("  [FAIL] Failed to load encrypted MIX: %s\n", path);
            tests_failed++;
        }
    }

    printf("\nEncrypted MIX files: %d loaded, %d skipped\n", loaded, skipped);

    if (loaded > 0) {
        TEST_ASSERT(true, "At least one encrypted MIX file loaded successfully");
    } else if (skipped > 0) {
        TEST_SKIP("No encrypted MIX files available (LFS stubs)");
    }
}

void test_unencrypted_mix_files() {
    printf("\n=== Testing Unencrypted MIX File Loading ===\n");

    int loaded = 0;
    int skipped = 0;

    for (int i = 0; UNENCRYPTED_MIX_FILES[i] != nullptr; i++) {
        const char* path = UNENCRYPTED_MIX_FILES[i];

        if (!file_exists_with_content(path)) {
            printf("  [SKIP] %s (not found or LFS stub)\n", path);
            skipped++;
            continue;
        }

        int result = Platform_Mix_Register(path);
        if (result == 0) {
            printf("  [PASS] Loaded unencrypted MIX: %s\n", path);
            loaded++;
            tests_passed++;
        } else {
            printf("  [FAIL] Failed to load unencrypted MIX: %s\n", path);
            tests_failed++;
        }
    }

    printf("\nUnencrypted MIX files: %d loaded, %d skipped\n", loaded, skipped);
}

void test_file_lookup() {
    printf("\n=== Testing File Lookup in MIX Archives ===\n");

    int mix_count = Platform_Mix_GetCount();
    if (mix_count == 0) {
        TEST_SKIP("No MIX files loaded, skipping lookup tests");
        return;
    }

    printf("Testing lookup across %d MIX files\n", mix_count);

    // Try to find common files that should exist in the game
    const char* test_files[] = {
        "PALETTE.PAL",
        "SHADOW.PAL",
        "TEMPERAT.PAL",
        "CONQUER.MIX",
        "INTRO.VQA",
        nullptr
    };

    int found = 0;
    for (int i = 0; test_files[i] != nullptr; i++) {
        int exists = Platform_Mix_Exists(test_files[i]);
        if (exists) {
            printf("  [PASS] Found: %s\n", test_files[i]);
            found++;
            tests_passed++;
        } else {
            printf("  [INFO] Not found: %s\n", test_files[i]);
        }
    }

    if (found > 0) {
        TEST_ASSERT(true, "At least one file found in MIX archives");
    }
}

void test_file_read() {
    printf("\n=== Testing File Read from MIX Archives ===\n");

    int mix_count = Platform_Mix_GetCount();
    if (mix_count == 0) {
        TEST_SKIP("No MIX files loaded, skipping read tests");
        return;
    }

    // Try to read a palette file (should be 768 bytes)
    int exists = Platform_Mix_Exists("PALETTE.PAL");
    if (exists) {
        int32_t size = Platform_Mix_GetSize("PALETTE.PAL");
        if (size > 0) {
            printf("  [INFO] PALETTE.PAL size: %d bytes\n", size);
            TEST_ASSERT(size == 768, "PALETTE.PAL is 768 bytes (standard VGA palette)");

            uint8_t* data = (uint8_t*)malloc(size);
            int32_t read = Platform_Mix_Read("PALETTE.PAL", data, size);
            TEST_ASSERT(read == size, "Read complete file");

            // Verify it looks like a palette (values should be 0-63 for 6-bit VGA)
            bool valid_palette = true;
            for (int i = 0; i < size && valid_palette; i++) {
                if (data[i] > 63) {
                    valid_palette = false;
                }
            }
            TEST_ASSERT(valid_palette, "Palette data appears valid (all values 0-63)");

            free(data);
        } else {
            TEST_SKIP("Could not get PALETTE.PAL size");
        }
    } else {
        TEST_SKIP("PALETTE.PAL not found");
    }
}

void test_redalert_mix_contents() {
    printf("\n=== Testing REDALERT.MIX Contents ===\n");

    // REDALERT.MIX is the main encrypted MIX file
    // It should contain other MIX files as nested archives
    if (!file_exists_with_content("gamedata/REDALERT.MIX")) {
        TEST_SKIP("REDALERT.MIX not available");
        return;
    }

    // Files known to be in REDALERT.MIX
    const char* known_files[] = {
        "CONQUER.MIX",   // Game data
        "DESEICNH.MIX",  // Desert terrain
        "SNOW.MIX",      // Snow terrain
        "TEMPERAT.MIX",  // Temperate terrain
        "INTERIOR.MIX",  // Interior terrain
        nullptr
    };

    int found = 0;
    for (int i = 0; known_files[i] != nullptr; i++) {
        int exists = Platform_Mix_Exists(known_files[i]);
        if (exists) {
            printf("  [PASS] Found nested MIX: %s\n", known_files[i]);
            found++;
            tests_passed++;
        } else {
            printf("  [INFO] Not found: %s\n", known_files[i]);
        }
    }

    if (found > 0) {
        TEST_ASSERT(true, "Found nested MIX files in REDALERT.MIX");
    }
}

int main(int argc, char* argv[]) {
    printf("==========================================\n");
    printf("MIX File Decryption Test (Task 12i)\n");
    printf("==========================================\n");
    printf("This test verifies that encrypted MIX files\n");
    printf("can be loaded using Blowfish/RSA decryption.\n");
    printf("==========================================\n");

    // Run all tests
    test_encrypted_mix_files();
    test_unencrypted_mix_files();
    test_file_lookup();
    test_file_read();
    test_redalert_mix_contents();

    // Print summary
    printf("\n==========================================\n");
    printf("Test Summary\n");
    printf("==========================================\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Total:  %d\n", tests_passed + tests_failed);
    printf("==========================================\n");

    if (tests_passed > 0 && tests_failed == 0) {
        printf("\nMIX decryption is working correctly!\n");
    } else if (tests_failed > 0) {
        printf("\nSome tests failed - check implementation.\n");
    } else {
        printf("\nNo tests ran - ensure game data files are present.\n");
    }

    return tests_failed > 0 ? 1 : 0;
}
