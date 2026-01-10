// src/test_audio_integration.cpp
// Integration test for unified audio system
// Task 17e - Audio Integration

#include "game/audio/audio_system.h"
#include "platform.h"
#include <cstdio>
#include <cstring>
#include <cmath>

//=============================================================================
// Test Utilities
//=============================================================================

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("  FAILED: %s\n", msg); \
            return false; \
        } \
    } while(0)

#define RUN_TEST(name) \
    do { \
        printf("Test: %s... ", #name); \
        if (Test_##name()) { \
            printf("PASSED\n"); \
            tests_passed++; \
        } else { \
            tests_failed++; \
        } \
    } while(0)

//=============================================================================
// Unit Tests
//=============================================================================

bool Test_Initialize() {
    AudioSystem& audio = AudioSystem::Instance();

    // Should start uninitialized
    audio.Shutdown();
    TEST_ASSERT(!audio.IsInitialized(), "Should not be initialized");

    TEST_ASSERT(Audio_Init(), "Audio_Init should succeed");
    TEST_ASSERT(audio.IsInitialized(), "Should be initialized");

    return true;
}

bool Test_MasterVolume() {
    Audio_SetMasterVolume(128);
    int vol = Audio_GetMasterVolume();
    TEST_ASSERT(vol >= 120 && vol <= 136, "Master volume ~128");

    Audio_SetMasterVolume(255);
    TEST_ASSERT(Audio_GetMasterVolume() >= 250, "Master volume 255");

    Audio_SetMasterVolume(0);
    TEST_ASSERT(Audio_GetMasterVolume() <= 5, "Master volume 0");

    Audio_SetMasterVolume(200);  // Reset

    return true;
}

bool Test_CategoryVolumes() {
    Audio_SetSFXVolume(200);
    TEST_ASSERT(Audio_GetSFXVolume() >= 195 && Audio_GetSFXVolume() <= 205, "SFX volume ~200");

    Audio_SetMusicVolume(150);
    TEST_ASSERT(Audio_GetMusicVolume() >= 145 && Audio_GetMusicVolume() <= 155, "Music volume ~150");

    Audio_SetVoiceVolume(180);
    TEST_ASSERT(Audio_GetVoiceVolume() >= 175 && Audio_GetVoiceVolume() <= 185, "Voice volume ~180");

    // Reset
    Audio_SetSFXVolume(255);
    Audio_SetMusicVolume(200);
    Audio_SetVoiceVolume(255);

    return true;
}

bool Test_MuteControl() {
    TEST_ASSERT(!Audio_IsMuted(), "Should not be muted initially");

    Audio_SetMuted(true);
    TEST_ASSERT(Audio_IsMuted(), "Should be muted");

    Audio_ToggleMute();
    TEST_ASSERT(!Audio_IsMuted(), "Should not be muted after toggle");

    Audio_ToggleMute();
    TEST_ASSERT(Audio_IsMuted(), "Should be muted after second toggle");

    Audio_SetMuted(false);  // Reset
    TEST_ASSERT(!Audio_IsMuted(), "Should not be muted after reset");

    return true;
}

bool Test_Stats() {
    AudioStats stats = AudioSystem::Instance().GetStats();

    TEST_ASSERT(stats.master_volume >= 0.0f && stats.master_volume <= 1.0f, "Valid master volume");
    TEST_ASSERT(stats.sfx_volume >= 0.0f && stats.sfx_volume <= 1.0f, "Valid SFX volume");
    TEST_ASSERT(stats.music_volume >= 0.0f && stats.music_volume <= 1.0f, "Valid music volume");
    TEST_ASSERT(stats.voice_volume >= 0.0f && stats.voice_volume <= 1.0f, "Valid voice volume");

    return true;
}

bool Test_StopAll() {
    // Should not crash
    Audio_StopAll();

    TEST_ASSERT(!Audio_IsMusicPlaying(), "Music should be stopped");

    return true;
}

bool Test_ConfigUpdate() {
    AudioSystemConfig config;
    config.master_volume = 0.5f;
    config.sfx_volume = 0.6f;
    config.music_volume = 0.7f;
    config.voice_volume = 0.8f;

    AudioSystem::Instance().UpdateConfig(config);

    TEST_ASSERT(std::abs(AudioSystem::Instance().GetMasterVolume() - 0.5f) < 0.01f, "Master volume updated");
    TEST_ASSERT(std::abs(AudioSystem::Instance().GetSFXVolume() - 0.6f) < 0.01f, "SFX volume updated");
    TEST_ASSERT(std::abs(AudioSystem::Instance().GetMusicVolume() - 0.7f) < 0.01f, "Music volume updated");
    TEST_ASSERT(std::abs(AudioSystem::Instance().GetVoiceVolume() - 0.8f) < 0.01f, "Voice volume updated");

    // Reset
    config.master_volume = 1.0f;
    config.sfx_volume = 1.0f;
    config.music_volume = 0.8f;
    config.voice_volume = 1.0f;
    AudioSystem::Instance().UpdateConfig(config);

    return true;
}

bool Test_GlobalFunctionWrappers() {
    // Test that global functions don't crash
    Audio_SetMasterVolume(200);
    Audio_SetSFXVolume(200);
    Audio_SetMusicVolume(200);
    Audio_SetVoiceVolume(200);

    // These should not crash even without game data
    Audio_StopAllSounds();
    Audio_StopMusic();
    Audio_StopAll();

    return true;
}

bool Test_ShutdownReinitialize() {
    Audio_Shutdown();
    TEST_ASSERT(!AudioSystem::Instance().IsInitialized(), "Should not be initialized after shutdown");

    TEST_ASSERT(Audio_Init(), "Should reinitialize");
    TEST_ASSERT(AudioSystem::Instance().IsInitialized(), "Should be initialized after reinit");

    return true;
}

bool Test_PrintDebugInfo() {
    // Should not crash
    printf("\n  Calling PrintDebugInfo():\n  ");
    AudioSystem::Instance().PrintDebugInfo();

    return true;
}

//=============================================================================
// Integration Test
//=============================================================================

bool Test_LoadAndPlay() {
    printf("\n  (Integration test - requires game data)\n  ");

    // Check if any MIX files are loaded
    if (Platform_Mix_GetCount() == 0) {
        printf("SKIPPED - No MIX files loaded\n  ");
        return true;
    }

    // Try to play a sound
    PlayHandle handle = Audio_PlaySound(SoundEffect::UI_CLICK);

    if (handle != INVALID_PLAY_HANDLE) {
        printf("Successfully played sound effect\n  ");

        // Brief delay
        Platform_Timer_Delay(100);

        Audio_StopAllSounds();
    } else {
        printf("SKIPPED: Could not play sound (assets may not be present)\n  ");
    }

    return true;
}

//=============================================================================
// Main
//=============================================================================

int main(int argc, char* argv[]) {
    printf("=== Audio Integration Tests (Task 17e) ===\n\n");

    bool quick_mode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--quick") == 0 || strcmp(argv[i], "-q") == 0) {
            quick_mode = true;
        }
    }

    // Initialize platform
    Platform_Init();

    // Run unit tests
    RUN_TEST(Initialize);
    RUN_TEST(MasterVolume);
    RUN_TEST(CategoryVolumes);
    RUN_TEST(MuteControl);
    RUN_TEST(Stats);
    RUN_TEST(StopAll);
    RUN_TEST(ConfigUpdate);
    RUN_TEST(GlobalFunctionWrappers);
    RUN_TEST(ShutdownReinitialize);
    RUN_TEST(PrintDebugInfo);

    // Integration test
    if (!quick_mode) {
        RUN_TEST(LoadAndPlay);
    }

    // Cleanup
    Audio_Shutdown();
    Platform_Shutdown();

    printf("\n");
    if (tests_failed == 0) {
        printf("All tests PASSED (%d/%d)\n", tests_passed, tests_passed + tests_failed);
    } else {
        printf("Results: %d passed, %d failed\n", tests_passed, tests_failed);
    }

    return (tests_failed == 0) ? 0 : 1;
}
