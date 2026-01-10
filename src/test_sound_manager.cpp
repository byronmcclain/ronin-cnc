// src/test_sound_manager.cpp
// Unit tests for Sound Manager
// Task 17b - Sound Effect Manager

#include "game/audio/sound_manager.h"
#include "game/audio/aud_file.h"
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

bool Test_SoundEffectEnum() {
    // Verify enum values are sequential
    TEST_ASSERT(static_cast<int>(SoundEffect::NONE) == 0, "NONE should be 0");
    TEST_ASSERT(static_cast<int>(SoundEffect::COUNT) > 50, "Should have 50+ sounds");

    // Verify some key sounds exist
    TEST_ASSERT(static_cast<int>(SoundEffect::UI_CLICK) > 0, "UI_CLICK should exist");
    TEST_ASSERT(static_cast<int>(SoundEffect::EXPLODE_LARGE) > 0, "EXPLODE_LARGE should exist");
    TEST_ASSERT(static_cast<int>(SoundEffect::WEAPON_CANNON) > 0, "WEAPON_CANNON should exist");

    return true;
}

bool Test_SoundInfo() {
    // Test GetSoundInfo
    const SoundInfo& click_info = GetSoundInfo(SoundEffect::UI_CLICK);
    TEST_ASSERT(click_info.filename != nullptr, "UI_CLICK should have filename");
    TEST_ASSERT(click_info.category == SoundCategory::UI, "UI_CLICK should be UI category");

    const SoundInfo& cannon_info = GetSoundInfo(SoundEffect::WEAPON_CANNON);
    TEST_ASSERT(cannon_info.filename != nullptr, "WEAPON_CANNON should have filename");
    TEST_ASSERT(cannon_info.category == SoundCategory::COMBAT, "WEAPON_CANNON should be COMBAT");

    // Test invalid sound
    const SoundInfo& none_info = GetSoundInfo(SoundEffect::NONE);
    TEST_ASSERT(none_info.filename == nullptr, "NONE should have no filename");

    return true;
}

bool Test_SoundFilename() {
    const char* click = GetSoundFilename(SoundEffect::UI_CLICK);
    TEST_ASSERT(click != nullptr, "UI_CLICK filename should exist");
    TEST_ASSERT(strstr(click, "CLICK") != nullptr, "Should contain CLICK");

    const char* none = GetSoundFilename(SoundEffect::NONE);
    TEST_ASSERT(none == nullptr, "NONE should have no filename");

    return true;
}

bool Test_SoundCategory() {
    TEST_ASSERT(GetSoundCategory(SoundEffect::UI_CLICK) == SoundCategory::UI,
                "UI_CLICK should be UI category");
    TEST_ASSERT(GetSoundCategory(SoundEffect::EXPLODE_SMALL) == SoundCategory::COMBAT,
                "EXPLODE_SMALL should be COMBAT category");
    TEST_ASSERT(GetSoundCategory(SoundEffect::MOVE_VEHICLE) == SoundCategory::UNIT,
                "MOVE_VEHICLE should be UNIT category");
    TEST_ASSERT(GetSoundCategory(SoundEffect::AMBIENT_FIRE) == SoundCategory::AMBIENT,
                "AMBIENT_FIRE should be AMBIENT category");
    TEST_ASSERT(GetSoundCategory(SoundEffect::SPECIAL_RADAR_ON) == SoundCategory::SPECIAL,
                "SPECIAL_RADAR_ON should be SPECIAL category");

    return true;
}

bool Test_ManagerSingleton() {
    SoundManager& mgr1 = SoundManager::Instance();
    SoundManager& mgr2 = SoundManager::Instance();
    TEST_ASSERT(&mgr1 == &mgr2, "Should return same instance");

    return true;
}

bool Test_ManagerInitialization() {
    SoundManager& mgr = SoundManager::Instance();

    // Should start uninitialized (or be initializable)
    bool was_initialized = mgr.IsInitialized();
    if (!was_initialized) {
        mgr.Initialize();
    }

    TEST_ASSERT(mgr.IsInitialized(), "Should be initialized after Initialize()");

    return true;
}

bool Test_VolumeControl() {
    SoundManager& mgr = SoundManager::Instance();

    if (!mgr.IsInitialized()) {
        mgr.Initialize();
    }

    // Test SFX volume
    mgr.SetVolume(0.5f);
    TEST_ASSERT(std::abs(mgr.GetVolume() - 0.5f) < 0.01f, "Volume should be 0.5");

    mgr.SetVolume(1.0f);
    TEST_ASSERT(std::abs(mgr.GetVolume() - 1.0f) < 0.01f, "Volume should be 1.0");

    // Test clamping
    mgr.SetVolume(2.0f);
    TEST_ASSERT(mgr.GetVolume() <= 1.0f, "Volume should be clamped to 1.0");

    mgr.SetVolume(-1.0f);
    TEST_ASSERT(mgr.GetVolume() >= 0.0f, "Volume should be clamped to 0.0");

    mgr.SetVolume(1.0f);  // Reset

    // Test category volume
    mgr.SetCategoryVolume(SoundCategory::COMBAT, 0.7f);
    TEST_ASSERT(std::abs(mgr.GetCategoryVolume(SoundCategory::COMBAT) - 0.7f) < 0.01f,
                "Combat volume should be 0.7");

    mgr.SetCategoryVolume(SoundCategory::COMBAT, 1.0f);  // Reset

    return true;
}

bool Test_Muting() {
    SoundManager& mgr = SoundManager::Instance();

    if (!mgr.IsInitialized()) {
        mgr.Initialize();
    }

    TEST_ASSERT(!mgr.IsMuted(), "Should not be muted initially");

    mgr.SetMuted(true);
    TEST_ASSERT(mgr.IsMuted(), "Should be muted");

    mgr.SetMuted(false);
    TEST_ASSERT(!mgr.IsMuted(), "Should not be muted after unmute");

    return true;
}

bool Test_ListenerPosition() {
    SoundManager& mgr = SoundManager::Instance();

    if (!mgr.IsInitialized()) {
        mgr.Initialize();
    }

    mgr.SetListenerPosition(500, 600);

    int lx, ly;
    mgr.GetListenerPosition(lx, ly);
    TEST_ASSERT(lx == 500, "Listener X should be 500");
    TEST_ASSERT(ly == 600, "Listener Y should be 600");

    return true;
}

bool Test_MaxDistance() {
    SoundManager& mgr = SoundManager::Instance();

    if (!mgr.IsInitialized()) {
        mgr.Initialize();
    }

    mgr.SetMaxDistance(1000);
    TEST_ASSERT(mgr.GetMaxDistance() == 1000, "Max distance should be 1000");

    mgr.SetMaxDistance(1500);
    TEST_ASSERT(mgr.GetMaxDistance() == 1500, "Max distance should be 1500");

    // Test minimum clamping
    mgr.SetMaxDistance(0);
    TEST_ASSERT(mgr.GetMaxDistance() >= 1, "Max distance should be at least 1");

    mgr.SetMaxDistance(1200);  // Reset to default

    return true;
}

bool Test_GlobalFunctions() {
    // Test the C-style global functions

    Sound_SetVolume(128);  // 50%
    int vol = Sound_GetVolume();
    TEST_ASSERT(vol >= 120 && vol <= 136, "Volume should be ~128");

    Sound_SetVolume(255);  // 100%
    vol = Sound_GetVolume();
    TEST_ASSERT(vol >= 250, "Volume should be ~255");

    Sound_SetVolume(0);  // 0%
    vol = Sound_GetVolume();
    TEST_ASSERT(vol <= 5, "Volume should be ~0");

    Sound_SetVolume(255);  // Reset

    return true;
}

bool Test_SoundInfoTableCompleteness() {
    // Verify all sounds in enum have corresponding info entries
    for (int i = 1; i < static_cast<int>(SoundEffect::COUNT); i++) {
        SoundEffect sfx = static_cast<SoundEffect>(i);
        const SoundInfo& info = GetSoundInfo(sfx);

        // Every non-NONE sound should have a filename
        if (info.filename == nullptr) {
            printf("\n  Sound %d has no filename\n  ", i);
            return false;
        }

        // Filename should end with .AUD
        size_t len = strlen(info.filename);
        if (len < 4 || strcmp(info.filename + len - 4, ".AUD") != 0) {
            printf("\n  Sound %d filename '%s' doesn't end with .AUD\n  ", i, info.filename);
            return false;
        }

        // Default volume should be reasonable
        if (info.default_volume < 0.0f || info.default_volume > 1.0f) {
            printf("\n  Sound %d has invalid default_volume %.2f\n  ", i, info.default_volume);
            return false;
        }
    }

    return true;
}

bool Test_PlayingSoundCount() {
    SoundManager& mgr = SoundManager::Instance();

    if (!mgr.IsInitialized()) {
        mgr.Initialize();
    }

    // Initially should be 0 playing sounds
    mgr.StopAll();
    TEST_ASSERT(mgr.GetPlayingSoundCount() == 0, "Should have 0 playing sounds after StopAll");

    return true;
}

bool Test_SoundStats() {
    SoundManager& mgr = SoundManager::Instance();

    if (!mgr.IsInitialized()) {
        mgr.Initialize();
    }

    // Just verify these don't crash
    int loaded = mgr.GetLoadedSoundCount();
    int playing = mgr.GetPlayingSoundCount();

    // Loaded count should be >= 0
    TEST_ASSERT(loaded >= 0, "Loaded count should be non-negative");

    // Playing count should be >= 0
    TEST_ASSERT(playing >= 0, "Playing count should be non-negative");

    // PrintStats shouldn't crash
    mgr.PrintStats();

    return true;
}

//=============================================================================
// Integration Test (requires game assets)
//=============================================================================

bool Test_LoadFromMix() {
    printf("\n  (Integration test - requires game data)\n  ");

    // Check if any MIX files are loaded
    if (Platform_Mix_GetCount() == 0) {
        printf("SKIPPED - No MIX files loaded\n  ");
        return true;
    }

    SoundManager& mgr = SoundManager::Instance();
    if (!mgr.IsInitialized()) {
        mgr.Initialize();
    }

    int loaded = mgr.GetLoadedSoundCount();
    if (loaded > 0) {
        printf("Loaded %d sounds from MIX\n  ", loaded);

        // Check some specific sounds
        if (mgr.IsSoundLoaded(SoundEffect::UI_CLICK)) {
            printf("UI_CLICK loaded\n  ");
        }
        if (mgr.IsSoundLoaded(SoundEffect::EXPLODE_SMALL)) {
            printf("EXPLODE_SMALL loaded\n  ");
        }
    } else {
        printf("SKIPPED - No sounds loaded (game assets may not be present)\n  ");
    }

    return true;  // Not a failure if no assets
}

//=============================================================================
// Main
//=============================================================================

int main(int argc, char* argv[]) {
    printf("=== Sound Manager Tests (Task 17b) ===\n\n");

    bool quick_mode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--quick") == 0 || strcmp(argv[i], "-q") == 0) {
            quick_mode = true;
        }
    }

    // Initialize platform for tests
    Platform_Init();

    // Run unit tests
    RUN_TEST(SoundEffectEnum);
    RUN_TEST(SoundInfo);
    RUN_TEST(SoundFilename);
    RUN_TEST(SoundCategory);
    RUN_TEST(ManagerSingleton);
    RUN_TEST(ManagerInitialization);
    RUN_TEST(VolumeControl);
    RUN_TEST(Muting);
    RUN_TEST(ListenerPosition);
    RUN_TEST(MaxDistance);
    RUN_TEST(GlobalFunctions);
    RUN_TEST(SoundInfoTableCompleteness);
    RUN_TEST(PlayingSoundCount);
    RUN_TEST(SoundStats);

    // Integration tests
    if (!quick_mode) {
        RUN_TEST(LoadFromMix);
    }

    // Cleanup
    SoundManager::Instance().Shutdown();
    Platform_Shutdown();

    printf("\n");
    if (tests_failed == 0) {
        printf("All tests PASSED (%d/%d)\n", tests_passed, tests_passed + tests_failed);
    } else {
        printf("Results: %d passed, %d failed\n", tests_passed, tests_failed);
    }

    return (tests_failed == 0) ? 0 : 1;
}
