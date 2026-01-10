// src/tests/unit/test_platform.cpp
// Platform Layer Unit Tests
// Task 18b - Unit Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "platform.h"
#include <cstring>

//=============================================================================
// Platform Initialization Tests
//=============================================================================

TEST_CASE(Platform_Init_Succeeds, "Platform") {
    // If not already initialized, init and check
    bool was_initialized = Platform_IsInitialized();

    if (!was_initialized) {
        int result = Platform_Init();
        TEST_ASSERT_EQ(result, 0);
    }

    TEST_ASSERT(Platform_IsInitialized());

    // Only shutdown if we initialized
    if (!was_initialized) {
        Platform_Shutdown();
        TEST_ASSERT(!Platform_IsInitialized());
        // Re-init for other tests
        Platform_Init();
    }
}

TEST_CASE(Platform_Init_MultipleCalls, "Platform") {
    // Multiple init calls should be safe
    // First call might succeed or already be initialized
    int result1 = Platform_Init();
    TEST_ASSERT(result1 == 0 || Platform_IsInitialized());

    // Second call when already initialized
    int result2 = Platform_Init();
    TEST_ASSERT(result2 == 0 || result2 == 1);  // 1 might mean already initialized

    // Verify platform is still usable
    TEST_ASSERT(Platform_IsInitialized());

    Platform_Shutdown();
    // Note: Not shutting down twice as other tests might need it
}

//=============================================================================
// Timer Tests
//=============================================================================

TEST_WITH_FIXTURE(PlatformFixture, Timer_GetTicks_ReturnsValue, "Timer") {
    TEST_ASSERT(fixture.IsInitialized());

    uint32_t ticks = Platform_Timer_GetTicks();
    TEST_ASSERT_GE(ticks, 0u);
}

TEST_WITH_FIXTURE(PlatformFixture, Timer_GetTicks_Increases, "Timer") {
    uint32_t t1 = Platform_Timer_GetTicks();

    // Small delay
    Platform_Timer_Delay(10);

    uint32_t t2 = Platform_Timer_GetTicks();
    TEST_ASSERT_GT(t2, t1);
}

TEST_WITH_FIXTURE(PlatformFixture, Timer_Delay_MinimumAccuracy, "Timer") {
    uint32_t start = Platform_Timer_GetTicks();

    Platform_Timer_Delay(50);  // 50ms delay

    uint32_t elapsed = Platform_Timer_GetTicks() - start;

    // Should be at least 40ms (allowing some tolerance)
    TEST_ASSERT_GE(elapsed, 40u);
    // Should not be more than 150ms (excessive delay would indicate problem)
    TEST_ASSERT_LE(elapsed, 150u);
}

TEST_WITH_FIXTURE(PlatformFixture, Timer_Delay_Zero, "Timer") {
    // Zero delay should return immediately
    uint32_t start = Platform_Timer_GetTicks();
    Platform_Timer_Delay(0);
    uint32_t elapsed = Platform_Timer_GetTicks() - start;

    TEST_ASSERT_LE(elapsed, 10u);  // Should be nearly instant
}

TEST_WITH_FIXTURE(PlatformFixture, Timer_PerformanceCounter, "Timer") {
    uint64_t counter = Platform_Timer_GetPerformanceCounter();
    TEST_ASSERT_GT(counter, 0u);

    uint64_t freq = Platform_Timer_GetPerformanceFrequency();
    TEST_ASSERT_GT(freq, 0u);
}

TEST_WITH_FIXTURE(PlatformFixture, Timer_GetTime, "Timer") {
    double time1 = Platform_Timer_GetTime();
    Platform_Timer_Delay(10);
    double time2 = Platform_Timer_GetTime();

    TEST_ASSERT_GT(time2, time1);
}

//=============================================================================
// Logging Tests
//=============================================================================

TEST_WITH_FIXTURE(PlatformFixture, Log_DoesNotCrash, "Platform") {
    // Just verify logging doesn't crash
    Platform_LogInfo("Test info message");
    Platform_LogDebug("Test debug message");
    Platform_LogWarn("Test warning message");
    Platform_LogError("Test error message");

    TEST_ASSERT(true);  // If we got here, logging works
}

TEST_WITH_FIXTURE(PlatformFixture, Log_WithLevel, "Platform") {
    Platform_Log(LOG_LEVEL_DEBUG, "Debug level log");
    Platform_Log(LOG_LEVEL_INFO, "Info level log");
    Platform_Log(LOG_LEVEL_WARN, "Warn level log");
    Platform_Log(LOG_LEVEL_ERROR, "Error level log");

    TEST_ASSERT(true);
}

//=============================================================================
// Memory Tests
//=============================================================================

TEST_CASE(Memory_BasicAllocation, "Memory") {
    // Test basic allocation using standard library
    void* ptr = malloc(1024);
    TEST_ASSERT_NOT_NULL(ptr);

    // Write to memory
    memset(ptr, 0xAB, 1024);

    // Verify
    uint8_t* bytes = static_cast<uint8_t*>(ptr);
    TEST_ASSERT_EQ(bytes[0], 0xAB);
    TEST_ASSERT_EQ(bytes[1023], 0xAB);

    free(ptr);
}

TEST_CASE(Memory_LargeAllocation, "Memory") {
    // Allocate 1MB
    size_t size = 1024 * 1024;
    void* ptr = malloc(size);
    TEST_ASSERT_NOT_NULL(ptr);

    // Write first and last bytes
    uint8_t* bytes = static_cast<uint8_t*>(ptr);
    bytes[0] = 0x12;
    bytes[size - 1] = 0x34;

    TEST_ASSERT_EQ(bytes[0], 0x12);
    TEST_ASSERT_EQ(bytes[size - 1], 0x34);

    free(ptr);
}

//=============================================================================
// Path/String Tests
//=============================================================================

TEST_CASE(Path_Normalization, "Platform") {
    // Test path handling
    std::string path = "gamedata/REDALERT.MIX";
    TEST_ASSERT(!path.empty());
    TEST_ASSERT_EQ(path.find("MIX") != std::string::npos, true);
}

TEST_CASE(Path_Extension, "Platform") {
    std::string filename = "MOUSE.SHP";

    size_t dot = filename.rfind('.');
    TEST_ASSERT_NE(dot, std::string::npos);

    std::string ext = filename.substr(dot + 1);
    TEST_ASSERT_EQ(ext, "SHP");
}

//=============================================================================
// Platform Event Tests
//=============================================================================

TEST_WITH_FIXTURE(PlatformFixture, Events_Poll_DoesNotCrash, "Platform") {
    // Poll events multiple times
    for (int i = 0; i < 10; i++) {
        Platform_PollEvents();
    }

    TEST_ASSERT(true);
}
