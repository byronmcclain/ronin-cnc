#include <cstdio>
#include <cstring>
#include <cassert>
#include "platform.h"

// Test helper macros
#define TEST(name) printf("  Testing %s... ", name)
#define PASS() printf("PASS\n")
#define FAIL(msg) do { printf("FAIL: %s\n", msg); return 1; } while(0)

int test_version() {
    TEST("Platform_GetVersion");
    int version = Platform_GetVersion();
    if (version < 1) FAIL("Version should be >= 1");
    printf("(v%d) ", version);
    PASS();
    return 0;
}

int test_init_shutdown() {
    TEST("Platform_Init/Shutdown cycle");

    // Should not be initialized yet (we reset in main)
    if (Platform_IsInitialized()) FAIL("Should not be initialized");

    // Init should succeed
    PlatformResult result = Platform_Init();
    if (result != PLATFORM_RESULT_SUCCESS) FAIL("Init failed");

    // Should be initialized
    if (!Platform_IsInitialized()) FAIL("Should be initialized after init");

    // Double init should fail
    result = Platform_Init();
    if (result != PLATFORM_RESULT_ALREADY_INITIALIZED) FAIL("Double init should fail");

    // Shutdown should succeed
    result = Platform_Shutdown();
    if (result != PLATFORM_RESULT_SUCCESS) FAIL("Shutdown failed");

    // Should not be initialized
    if (Platform_IsInitialized()) FAIL("Should not be initialized after shutdown");

    // Double shutdown should fail
    result = Platform_Shutdown();
    if (result != PLATFORM_RESULT_NOT_INITIALIZED) FAIL("Double shutdown should fail");

    PASS();
    return 0;
}

int test_error_handling() {
    TEST("Error handling");

    // Clear any existing error
    Platform_ClearError();

    // Get error should return empty/null
    const char* err = Platform_GetErrorString();
    // err may be null or empty string - both are valid

    // Set an error
    Platform_SetError("Test error message");

    // Get error should return our message
    err = Platform_GetErrorString();
    if (err == nullptr) FAIL("GetErrorString returned null after SetError");
    if (strstr(err, "Test error") == nullptr) FAIL("Error message not found");

    // Test buffer-based error retrieval
    char buffer[256];
    int len = Platform_GetLastError(reinterpret_cast<int8_t*>(buffer), sizeof(buffer));
    if (len < 0) FAIL("GetLastError failed");
    if (strstr(buffer, "Test error") == nullptr) FAIL("Buffer error message not found");

    // Clear error
    Platform_ClearError();

    PASS();
    return 0;
}

int test_logging() {
    TEST("Logging functions");

    // These should not crash - we can't easily verify output
    Platform_Log(LOG_LEVEL_DEBUG, "Debug test message");
    Platform_Log(LOG_LEVEL_INFO, "Info test message");
    Platform_Log(LOG_LEVEL_WARN, "Warn test message");
    Platform_Log(LOG_LEVEL_ERROR, "Error test message");

    Platform_LogDebug("Debug shorthand test");
    Platform_LogInfo("Info shorthand test");
    Platform_LogWarn("Warn shorthand test");
    Platform_LogError("Error shorthand test");

    // Test with null (should not crash)
    Platform_Log(LOG_LEVEL_INFO, nullptr);

    PASS();
    return 0;
}

int main(int argc, char* argv[]) {
    bool test_mode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--test-init") == 0 ||
            strcmp(argv[i], "--test") == 0) {
            test_mode = true;
        }
    }

    printf("=== Platform Layer Compatibility Test ===\n\n");
    printf("Platform version: %d\n\n", Platform_GetVersion());

    int failures = 0;

    // Run tests
    failures += test_version();
    failures += test_init_shutdown();
    failures += test_error_handling();
    failures += test_logging();

    // Final init/shutdown for normal operation test
    printf("\n  Final init/shutdown cycle... ");
    if (Platform_Init() == PLATFORM_RESULT_SUCCESS) {
        Platform_Shutdown();
        printf("PASS\n");
    } else {
        printf("FAIL\n");
        failures++;
    }

    printf("\n=== Results ===\n");
    if (failures == 0) {
        printf("All tests passed!\n");
        printf("Test passed: init/shutdown cycle complete\n");
        return 0;
    } else {
        printf("%d test(s) failed\n", failures);
        return 1;
    }
}
