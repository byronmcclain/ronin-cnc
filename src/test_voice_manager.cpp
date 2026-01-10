// src/test_voice_manager.cpp
// Unit tests for Voice Manager
// Task 17d - Voice System

#include "game/audio/voice_manager.h"
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

bool Test_EvaVoiceEnum() {
    TEST_ASSERT(static_cast<int>(EvaVoice::NONE) == 0, "NONE should be 0");
    TEST_ASSERT(static_cast<int>(EvaVoice::COUNT) > 20, "Should have 20+ EVA voices");
    TEST_ASSERT(static_cast<int>(EvaVoice::CONSTRUCTION_COMPLETE) > 0, "CONSTRUCTION_COMPLETE exists");
    TEST_ASSERT(static_cast<int>(EvaVoice::BASE_UNDER_ATTACK) > 0, "BASE_UNDER_ATTACK exists");
    TEST_ASSERT(static_cast<int>(EvaVoice::MISSION_ACCOMPLISHED) > 0, "MISSION_ACCOMPLISHED exists");

    return true;
}

bool Test_UnitVoiceEnum() {
    TEST_ASSERT(static_cast<int>(UnitVoice::NONE) == 0, "NONE should be 0");
    TEST_ASSERT(static_cast<int>(UnitVoice::COUNT) > 15, "Should have 15+ unit voices");
    TEST_ASSERT(static_cast<int>(UnitVoice::REPORTING) > 0, "REPORTING exists");
    TEST_ASSERT(static_cast<int>(UnitVoice::ACKNOWLEDGED) > 0, "ACKNOWLEDGED exists");
    TEST_ASSERT(static_cast<int>(UnitVoice::MOVING_OUT) > 0, "MOVING_OUT exists");
    TEST_ASSERT(static_cast<int>(UnitVoice::ATTACKING) > 0, "ATTACKING exists");

    return true;
}

bool Test_EvaVoiceInfo() {
    const EvaVoiceInfo& info = GetEvaVoiceInfo(EvaVoice::CONSTRUCTION_COMPLETE);
    TEST_ASSERT(info.filename != nullptr, "Should have filename");
    TEST_ASSERT(strstr(info.filename, ".AUD") != nullptr, "Should be AUD file");
    TEST_ASSERT(info.priority > 0, "Should have priority");
    TEST_ASSERT(info.min_interval_ms > 0, "Should have interval");
    TEST_ASSERT(info.description != nullptr, "Should have description");

    const EvaVoiceInfo& attack_info = GetEvaVoiceInfo(EvaVoice::BASE_UNDER_ATTACK);
    TEST_ASSERT(attack_info.priority > info.priority, "BASE_UNDER_ATTACK should be higher priority");
    TEST_ASSERT(attack_info.min_interval_ms >= 30000, "BASE_UNDER_ATTACK should have long interval");

    // Critical voices should have highest priority
    const EvaVoiceInfo& nuke_info = GetEvaVoiceInfo(EvaVoice::NUKE_ATTACK);
    TEST_ASSERT(nuke_info.priority == 255, "NUKE_ATTACK should have max priority");

    return true;
}

bool Test_UnitVoiceInfo() {
    const UnitVoiceInfo& info = GetUnitVoiceInfo(UnitVoice::REPORTING);
    TEST_ASSERT(info.filename != nullptr, "Should have filename");
    TEST_ASSERT(strstr(info.filename, ".AUD") != nullptr, "Should be AUD file");
    TEST_ASSERT(info.min_interval_ms > 0, "Should have interval");

    // Check attack response has shorter interval (more urgent)
    const UnitVoiceInfo& attack_info = GetUnitVoiceInfo(UnitVoice::ATTACKING);
    const UnitVoiceInfo& select_info = GetUnitVoiceInfo(UnitVoice::REPORTING);
    TEST_ASSERT(attack_info.min_interval_ms <= select_info.min_interval_ms,
                "Attack should have shorter/equal interval");

    return true;
}

bool Test_EvaVoiceFilename() {
    const char* filename = GetEvaVoiceFilename(EvaVoice::UNIT_READY);
    TEST_ASSERT(filename != nullptr, "Should return filename");
    TEST_ASSERT(strstr(filename, "UNITREDY") != nullptr, "Should contain UNITREDY");

    const char* none_filename = GetEvaVoiceFilename(EvaVoice::NONE);
    TEST_ASSERT(none_filename == nullptr, "NONE should return nullptr");

    return true;
}

bool Test_UnitVoiceFilename() {
    const char* filename = GetUnitVoiceFilename(UnitVoice::ACKNOWLEDGED, VoiceFaction::NEUTRAL);
    TEST_ASSERT(filename != nullptr, "Should return filename");
    TEST_ASSERT(strstr(filename, "ACKNO") != nullptr, "Should contain ACKNO");

    // Test faction variants
    const char* soviet = GetUnitVoiceFilename(UnitVoice::FOR_MOTHER_RUSSIA, VoiceFaction::SOVIET);
    TEST_ASSERT(soviet != nullptr, "Soviet variant should exist");

    return true;
}

bool Test_VolumeControl() {
    VoiceManager& mgr = VoiceManager::Instance();

    if (!mgr.IsInitialized()) {
        mgr.Initialize();
    }

    mgr.SetVolume(0.5f);
    TEST_ASSERT(std::abs(mgr.GetVolume() - 0.5f) < 0.01f, "Volume should be 0.5");

    mgr.SetVolume(1.0f);
    TEST_ASSERT(std::abs(mgr.GetVolume() - 1.0f) < 0.01f, "Volume should be 1.0");

    // Test clamping
    mgr.SetVolume(2.0f);
    TEST_ASSERT(mgr.GetVolume() <= 1.0f, "Should clamp to 1.0");

    mgr.SetVolume(-1.0f);
    TEST_ASSERT(mgr.GetVolume() >= 0.0f, "Should clamp to 0.0");

    mgr.SetVolume(0.8f);  // Reset

    return true;
}

bool Test_MuteControl() {
    VoiceManager& mgr = VoiceManager::Instance();

    if (!mgr.IsInitialized()) {
        mgr.Initialize();
    }

    TEST_ASSERT(!mgr.IsMuted(), "Should not be muted initially");

    mgr.SetMuted(true);
    TEST_ASSERT(mgr.IsMuted(), "Should be muted");

    mgr.SetMuted(false);
    TEST_ASSERT(!mgr.IsMuted(), "Should not be muted");

    return true;
}

bool Test_GlobalFunctions() {
    Voice_SetVolume(128);
    int vol = Voice_GetVolume();
    TEST_ASSERT(vol >= 120 && vol <= 136, "Volume should be ~128");

    Voice_SetVolume(255);
    vol = Voice_GetVolume();
    TEST_ASSERT(vol >= 250, "Volume should be ~255");

    Voice_SetVolume(0);
    vol = Voice_GetVolume();
    TEST_ASSERT(vol <= 5, "Volume should be ~0");

    Voice_SetVolume(200);  // Reset

    return true;
}

bool Test_InitialState() {
    VoiceManager& mgr = VoiceManager::Instance();

    // Reinitialize for clean state
    mgr.Shutdown();
    mgr.Initialize();

    TEST_ASSERT(!mgr.IsEvaSpeaking(), "Should not be speaking");
    TEST_ASSERT(!mgr.IsUnitSpeaking(), "Should not be speaking");
    TEST_ASSERT(mgr.GetCurrentEvaVoice() == EvaVoice::NONE, "Should have no EVA voice");
    TEST_ASSERT(mgr.GetQueueSize() == 0, "Queue should be empty");

    return true;
}

bool Test_QueueOperations() {
    VoiceManager& mgr = VoiceManager::Instance();

    if (!mgr.IsInitialized()) {
        mgr.Initialize();
    }

    mgr.ClearEvaQueue();
    TEST_ASSERT(mgr.GetQueueSize() == 0, "Queue should be empty");

    // Queue some voices
    mgr.QueueEva(EvaVoice::BUILDING);
    mgr.QueueEva(EvaVoice::CONSTRUCTION_COMPLETE);
    mgr.QueueEva(EvaVoice::UNIT_READY);

    TEST_ASSERT(mgr.GetQueueSize() == 3, "Queue should have 3 items");

    mgr.ClearEvaQueue();
    TEST_ASSERT(mgr.GetQueueSize() == 0, "Queue should be cleared");

    return true;
}

bool Test_VoiceFactionEnum() {
    TEST_ASSERT(static_cast<int>(VoiceFaction::NEUTRAL) == 0, "NEUTRAL should be 0");
    TEST_ASSERT(static_cast<int>(VoiceFaction::ALLIED) == 1, "ALLIED should be 1");
    TEST_ASSERT(static_cast<int>(VoiceFaction::SOVIET) == 2, "SOVIET should be 2");

    return true;
}

bool Test_EvaVoiceInfoTableCompleteness() {
    // Verify all EVA voices have valid info
    for (int i = 1; i < static_cast<int>(EvaVoice::COUNT); i++) {
        EvaVoice voice = static_cast<EvaVoice>(i);
        const EvaVoiceInfo& info = GetEvaVoiceInfo(voice);

        if (info.filename == nullptr) {
            printf("\n  EVA voice %d has no filename\n  ", i);
            return false;
        }

        size_t len = strlen(info.filename);
        if (len < 4 || strcmp(info.filename + len - 4, ".AUD") != 0) {
            printf("\n  EVA voice %d filename '%s' doesn't end with .AUD\n  ", i, info.filename);
            return false;
        }

        if (info.description == nullptr) {
            printf("\n  EVA voice %d has no description\n  ", i);
            return false;
        }
    }

    return true;
}

bool Test_UnitVoiceInfoTableCompleteness() {
    // Verify all unit voices have valid info
    for (int i = 1; i < static_cast<int>(UnitVoice::COUNT); i++) {
        UnitVoice voice = static_cast<UnitVoice>(i);
        const UnitVoiceInfo& info = GetUnitVoiceInfo(voice);

        if (info.filename == nullptr) {
            printf("\n  Unit voice %d has no filename\n  ", i);
            return false;
        }

        size_t len = strlen(info.filename);
        if (len < 4 || strcmp(info.filename + len - 4, ".AUD") != 0) {
            printf("\n  Unit voice %d filename '%s' doesn't end with .AUD\n  ", i, info.filename);
            return false;
        }
    }

    return true;
}

bool Test_PriorityOrdering() {
    // Test priority values are reasonable
    const EvaVoiceInfo& building = GetEvaVoiceInfo(EvaVoice::BUILDING);
    const EvaVoiceInfo& complete = GetEvaVoiceInfo(EvaVoice::CONSTRUCTION_COMPLETE);
    const EvaVoiceInfo& attack = GetEvaVoiceInfo(EvaVoice::BASE_UNDER_ATTACK);
    const EvaVoiceInfo& nuke = GetEvaVoiceInfo(EvaVoice::NUKE_ATTACK);

    TEST_ASSERT(building.priority < complete.priority, "Building < Complete");
    TEST_ASSERT(complete.priority < attack.priority, "Complete < Attack");
    TEST_ASSERT(attack.priority < nuke.priority, "Attack < Nuke");

    return true;
}

//=============================================================================
// Integration Test
//=============================================================================

bool Test_LoadAndPlayVoice() {
    printf("\n  (Integration test - requires game data)\n  ");

    // Check if any MIX files are loaded
    if (Platform_Mix_GetCount() == 0) {
        printf("SKIPPED - No MIX files loaded\n  ");
        return true;
    }

    VoiceManager& mgr = VoiceManager::Instance();
    mgr.Shutdown();
    mgr.Initialize();

    // Try to play an EVA voice
    bool success = mgr.PlayEva(EvaVoice::CONSTRUCTION_COMPLETE);

    if (success) {
        printf("Successfully started EVA voice\n  ");

        // Verify state
        if (!mgr.IsEvaSpeaking()) {
            printf("FAILED: Should be speaking\n  ");
            return false;
        }
        if (mgr.GetCurrentEvaVoice() != EvaVoice::CONSTRUCTION_COMPLETE) {
            printf("FAILED: Voice mismatch\n  ");
            return false;
        }

        // Stop after brief test
        mgr.StopEva();
        if (mgr.IsEvaSpeaking()) {
            printf("FAILED: Should be stopped\n  ");
            return false;
        }
    } else {
        printf("SKIPPED: Could not load voice (game assets may not be present)\n  ");
    }

    return true;
}

//=============================================================================
// Main
//=============================================================================

int main(int argc, char* argv[]) {
    printf("=== Voice Manager Tests (Task 17d) ===\n\n");

    bool quick_mode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--quick") == 0 || strcmp(argv[i], "-q") == 0) {
            quick_mode = true;
        }
    }

    // Initialize platform
    Platform_Init();

    // Run unit tests
    RUN_TEST(EvaVoiceEnum);
    RUN_TEST(UnitVoiceEnum);
    RUN_TEST(EvaVoiceInfo);
    RUN_TEST(UnitVoiceInfo);
    RUN_TEST(EvaVoiceFilename);
    RUN_TEST(UnitVoiceFilename);
    RUN_TEST(VolumeControl);
    RUN_TEST(MuteControl);
    RUN_TEST(GlobalFunctions);
    RUN_TEST(InitialState);
    RUN_TEST(QueueOperations);
    RUN_TEST(VoiceFactionEnum);
    RUN_TEST(EvaVoiceInfoTableCompleteness);
    RUN_TEST(UnitVoiceInfoTableCompleteness);
    RUN_TEST(PriorityOrdering);

    // Integration test
    if (!quick_mode) {
        RUN_TEST(LoadAndPlayVoice);
    }

    // Cleanup
    VoiceManager::Instance().Shutdown();
    Platform_Shutdown();

    printf("\n");
    if (tests_failed == 0) {
        printf("All tests PASSED (%d/%d)\n", tests_passed, tests_passed + tests_failed);
    } else {
        printf("Results: %d passed, %d failed\n", tests_passed, tests_failed);
    }

    return (tests_failed == 0) ? 0 : 1;
}
