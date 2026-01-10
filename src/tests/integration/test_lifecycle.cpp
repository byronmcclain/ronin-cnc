// src/tests/integration/test_lifecycle.cpp
// System Lifecycle Integration Tests
// Task 18c - Integration Tests

#include "test/test_framework.h"
#include "platform.h"

//=============================================================================
// Platform Lifecycle Tests
//=============================================================================

TEST_CASE(Lifecycle_Platform_InitShutdown, "Lifecycle") {
    // Save initial state
    bool was_initialized = Platform_IsInitialized();

    if (was_initialized) {
        Platform_Shutdown();
    }

    // Clean init
    PlatformResult result = Platform_Init();
    TEST_ASSERT(result == PLATFORM_RESULT_SUCCESS || result == PLATFORM_RESULT_ALREADY_INITIALIZED);
    TEST_ASSERT(Platform_IsInitialized());

    // Clean shutdown
    Platform_Shutdown();
    TEST_ASSERT(!Platform_IsInitialized());

    // Restore if was initialized
    if (was_initialized) {
        Platform_Init();
    }
}

TEST_CASE(Lifecycle_Platform_MultipleInitShutdown, "Lifecycle") {
    // Multiple cycles should work
    for (int i = 0; i < 3; i++) {
        PlatformResult result = Platform_Init();
        TEST_ASSERT(result == PLATFORM_RESULT_SUCCESS || result == PLATFORM_RESULT_ALREADY_INITIALIZED);
        TEST_ASSERT(Platform_IsInitialized());
        Platform_Shutdown();
        TEST_ASSERT(!Platform_IsInitialized());
    }

    // Leave platform initialized for other tests
    Platform_Init();
}

//=============================================================================
// Graphics Lifecycle Tests
//=============================================================================

TEST_CASE(Lifecycle_Graphics_InitShutdown, "Lifecycle") {
    Platform_Init();

    int32_t result = Platform_Graphics_Init();
    TEST_ASSERT_EQ(result, 0);
    TEST_ASSERT(Platform_Graphics_IsInitialized());

    Platform_Graphics_Shutdown();
    TEST_ASSERT(!Platform_Graphics_IsInitialized());

    Platform_Shutdown();
}

TEST_CASE(Lifecycle_Graphics_BackBufferAvailable, "Lifecycle") {
    Platform_Init();
    Platform_Graphics_Init();

    uint8_t* buffer;
    int32_t w, h, p;
    TEST_ASSERT_EQ(Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p), 0);
    TEST_ASSERT_NOT_NULL(buffer);
    TEST_ASSERT_GT(w, 0);
    TEST_ASSERT_GT(h, 0);
    TEST_ASSERT_GE(p, w);

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Audio Lifecycle Tests
//=============================================================================

TEST_CASE(Lifecycle_Audio_InitShutdown, "Lifecycle") {
    Platform_Init();

    AudioConfig config;
    config.sample_rate = 22050;
    config.channels = 2;
    config.bits_per_sample = 16;
    config.buffer_size = 1024;

    int32_t result = Platform_Audio_Init(&config);
    TEST_ASSERT_EQ(result, 0);
    TEST_ASSERT(Platform_Audio_IsInitialized());

    Platform_Audio_Shutdown();
    TEST_ASSERT(!Platform_Audio_IsInitialized());

    Platform_Shutdown();
}

//=============================================================================
// Full System Lifecycle Tests
//=============================================================================

TEST_CASE(Lifecycle_FullSystem_InitShutdown, "Lifecycle") {
    // Initialize in correct order
    Platform_Init();
    TEST_ASSERT(Platform_IsInitialized());

    Platform_Graphics_Init();
    TEST_ASSERT(Platform_Graphics_IsInitialized());

    AudioConfig audio_config = {22050, 2, 16, 1024};
    Platform_Audio_Init(&audio_config);
    TEST_ASSERT(Platform_Audio_IsInitialized());

    // Shutdown in reverse order
    Platform_Audio_Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();

    // Verify all shut down
    TEST_ASSERT(!Platform_Audio_IsInitialized());
    TEST_ASSERT(!Platform_Graphics_IsInitialized());
    TEST_ASSERT(!Platform_IsInitialized());
}

TEST_CASE(Lifecycle_FullSystem_MultipleCycles, "Lifecycle") {
    for (int cycle = 0; cycle < 3; cycle++) {
        Platform_Init();
        Platform_Graphics_Init();

        AudioConfig audio_config = {22050, 2, 16, 1024};
        Platform_Audio_Init(&audio_config);

        // Run a few frames
        for (int frame = 0; frame < 10; frame++) {
            Platform_PollEvents();
            Platform_Graphics_Flip();
            Platform_Timer_Delay(16);
        }

        Platform_Audio_Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
    }

    TEST_ASSERT(true);  // If we got here, no crashes
}

//=============================================================================
// Error Recovery Tests
//=============================================================================

TEST_CASE(Lifecycle_ShutdownWithoutInit, "Lifecycle") {
    // Make sure we're not initialized
    if (Platform_IsInitialized()) {
        Platform_Shutdown();
    }

    // Shutdown without init should not crash
    Platform_Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Audio_Shutdown();

    TEST_ASSERT(true);
}

TEST_CASE(Lifecycle_DoubleInit, "Lifecycle") {
    Platform_Init();
    PlatformResult result = Platform_Init();  // Double init

    // Should either succeed or return "already initialized"
    TEST_ASSERT(result == PLATFORM_RESULT_SUCCESS || result == PLATFORM_RESULT_ALREADY_INITIALIZED);

    Platform_Shutdown();
}

TEST_CASE(Lifecycle_DoubleShutdown, "Lifecycle") {
    Platform_Init();
    Platform_Shutdown();
    Platform_Shutdown();  // Double shutdown - should not crash

    TEST_ASSERT(true);
}

//=============================================================================
// MIX Asset System Lifecycle Tests
//=============================================================================

TEST_CASE(Lifecycle_Assets_InitShutdown, "Lifecycle") {
    Platform_Init();

    int32_t result = Platform_Assets_Init();
    TEST_ASSERT_EQ(result, 0);

    // Assets don't have explicit shutdown, but should work
    Platform_Shutdown();
}

TEST_CASE(Lifecycle_Mix_RegisterUnregister, "Lifecycle") {
    Platform_Init();
    Platform_Assets_Init();

    // Count before
    int32_t before = Platform_Mix_GetCount();

    // Try to register a non-existent file (should fail gracefully)
    int32_t result = Platform_Mix_Register("NONEXISTENT.MIX");
    TEST_ASSERT_EQ(result, -1);  // Should fail

    // Count should be same
    int32_t after = Platform_Mix_GetCount();
    TEST_ASSERT_EQ(before, after);

    Platform_Shutdown();
}
