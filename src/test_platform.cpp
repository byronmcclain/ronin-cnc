/**
 * Platform Layer Integration Tests
 *
 * Tests all platform FFI functions from C++ to verify
 * the interface works correctly.
 */

#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// =============================================================================
// Test Framework (Simple)
// =============================================================================

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static void test_##name(void)
#define RUN_TEST(name) do { \
    printf("  Running: %s...", #name); \
    tests_run++; \
    test_##name(); \
    printf(" PASSED\n"); \
    tests_passed++; \
} while(0)

#define ASSERT(expr) do { \
    if (!(expr)) { \
        printf(" FAILED\n"); \
        printf("    Assertion failed: %s\n", #expr); \
        printf("    At line %d\n", __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b) ASSERT((a) == (b))
#define ASSERT_NE(a, b) ASSERT((a) != (b))
#define ASSERT_GT(a, b) ASSERT((a) > (b))
#define ASSERT_GE(a, b) ASSERT((a) >= (b))

// =============================================================================
// Platform Initialization Tests
// =============================================================================

TEST(platform_version) {
    int version = Platform_GetVersion();
    ASSERT_GT(version, 0);
}

TEST(platform_init_shutdown) {
    // First ensure we're in clean state
    Platform_Shutdown();

    // Init should succeed
    int result = (int)Platform_Init();
    ASSERT_EQ(result, 0);  // PlatformResult::Success

    // Should be initialized
    ASSERT(Platform_IsInitialized());

    // Double init should fail
    result = (int)Platform_Init();
    ASSERT_EQ(result, 1);  // PlatformResult::AlreadyInitialized

    // Shutdown should succeed
    result = (int)Platform_Shutdown();
    ASSERT_EQ(result, 0);

    // Should not be initialized
    ASSERT(!Platform_IsInitialized());

    // Double shutdown should fail
    result = (int)Platform_Shutdown();
    ASSERT_EQ(result, 2);  // PlatformResult::NotInitialized
}

// =============================================================================
// Configuration Path Tests
// =============================================================================

TEST(config_path) {
    char buffer[512];
    int len = Platform_GetConfigPath(buffer, sizeof(buffer));

    ASSERT_GT(len, 0);
    ASSERT(strstr(buffer, "RedAlert") != NULL);
}

TEST(saves_path) {
    char buffer[512];
    int len = Platform_GetSavesPath(buffer, sizeof(buffer));

    ASSERT_GT(len, 0);
    ASSERT(strstr(buffer, "saves") != NULL);
}

TEST(log_path) {
    char buffer[512];
    int len = Platform_GetLogPath(buffer, sizeof(buffer));

    ASSERT_GT(len, 0);
    ASSERT(strstr(buffer, "log") != NULL);
}

TEST(ensure_directories) {
    int result = Platform_EnsureDirectories();
    ASSERT_EQ(result, 0);
}

TEST(path_null_safety) {
    // Null buffer should return -1
    ASSERT_EQ(Platform_GetConfigPath(NULL, 100), -1);
    ASSERT_EQ(Platform_GetSavesPath(NULL, 100), -1);

    // Zero size should return -1
    char buffer[10];
    ASSERT_EQ(Platform_GetConfigPath(buffer, 0), -1);
}

// =============================================================================
// Logging Tests
// =============================================================================

TEST(log_init_shutdown) {
    // Init logging (may already be initialized)
    int result = Platform_Log_Init();
    ASSERT(result == 0 || result == -1);  // Success or already init

    // Log a message (should not crash)
    Platform_Log(LOG_LEVEL_INFO, "Test log message from C++");

    // Flush
    Platform_Log_Flush();

    Platform_Log_Shutdown();
}

TEST(log_levels) {
    Platform_Log_SetLevel(3);  // Debug
    ASSERT_EQ(Platform_Log_GetLevel(), 3);

    Platform_Log_SetLevel(1);  // Warn
    ASSERT_EQ(Platform_Log_GetLevel(), 1);

    Platform_Log_SetLevel(2);  // Reset to Info
}

// =============================================================================
// Application Lifecycle Tests
// =============================================================================

TEST(app_state) {
    // Default should be active
    int state = Platform_GetAppState();
    ASSERT_GE(state, 0);
    ASSERT(state <= 2);  // Valid states: 0, 1, 2
}

TEST(app_active) {
    int active = Platform_IsAppActive();
    ASSERT(active == 0 || active == 1);
}

TEST(quit_request) {
    // Should not be quit initially (unless something set it)
    Platform_ClearQuitRequest();
    ASSERT_EQ(Platform_ShouldQuit(), 0);

    Platform_RequestQuit();
    ASSERT_EQ(Platform_ShouldQuit(), 1);

    Platform_ClearQuitRequest();
    ASSERT_EQ(Platform_ShouldQuit(), 0);
}

// =============================================================================
// Performance Tests
// =============================================================================

TEST(frame_timing) {
    Platform_FrameStart();
    Platform_FrameEnd();

    // FPS should be non-negative
    int fps = Platform_GetFPS();
    ASSERT_GE(fps, 0);

    // Frame time should be non-negative
    int frame_time = Platform_GetFrameTime();
    ASSERT_GE(frame_time, 0);

    // Frame count should increment
    uint32_t count1 = Platform_GetFrameCount();
    Platform_FrameStart();
    Platform_FrameEnd();
    uint32_t count2 = Platform_GetFrameCount();
    ASSERT_GT(count2, count1);
}

// =============================================================================
// Network Tests
// =============================================================================

TEST(network_init_shutdown) {
    // Init
    int result = Platform_Network_Init();
    ASSERT(result == 0 || result == -1);  // Success or already init

    if (result == 0) {
        ASSERT_EQ(Platform_Network_IsInitialized(), 1);

        Platform_Network_Shutdown();
        ASSERT_EQ(Platform_Network_IsInitialized(), 0);
    }
}

// =============================================================================
// Test Runner
// =============================================================================

void run_smoke_tests(void) {
    printf("\n=== Smoke Tests ===\n");
    RUN_TEST(platform_version);
}

void run_init_tests(void) {
    printf("\n=== Initialization Tests ===\n");
    RUN_TEST(platform_init_shutdown);
}

void run_config_tests(void) {
    printf("\n=== Configuration Tests ===\n");
    RUN_TEST(config_path);
    RUN_TEST(saves_path);
    RUN_TEST(log_path);
    RUN_TEST(ensure_directories);
    RUN_TEST(path_null_safety);
}

void run_logging_tests(void) {
    printf("\n=== Logging Tests ===\n");
    RUN_TEST(log_init_shutdown);
    RUN_TEST(log_levels);
}

void run_lifecycle_tests(void) {
    printf("\n=== Lifecycle Tests ===\n");
    RUN_TEST(app_state);
    RUN_TEST(app_active);
    RUN_TEST(quit_request);
}

void run_performance_tests(void) {
    printf("\n=== Performance Tests ===\n");
    RUN_TEST(frame_timing);
}

void run_network_tests(void) {
    printf("\n=== Network Tests ===\n");
    RUN_TEST(network_init_shutdown);
}

int main(int argc, char* argv[]) {
    printf("============================================\n");
    printf("Platform Integration Tests\n");
    printf("============================================\n");

    bool quick = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--quick") == 0 || strcmp(argv[i], "-q") == 0) {
            quick = true;
        }
    }

    // Always run smoke tests
    run_smoke_tests();

    if (!quick) {
        run_init_tests();
        run_config_tests();
        run_logging_tests();
        run_lifecycle_tests();
        run_performance_tests();
        run_network_tests();
    }

    printf("\n============================================\n");
    printf("Results: %d/%d tests passed", tests_passed, tests_run);
    if (tests_failed > 0) {
        printf(" (%d FAILED)", tests_failed);
    }
    printf("\n============================================\n");

    return tests_failed > 0 ? 1 : 0;
}
