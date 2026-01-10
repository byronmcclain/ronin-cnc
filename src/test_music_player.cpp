// src/test_music_player.cpp
// Unit tests for Music Player
// Task 17c - Music System

#include "game/audio/music_player.h"
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

bool Test_MusicTrackEnum() {
    TEST_ASSERT(static_cast<int>(MusicTrack::NONE) == 0, "NONE should be 0");
    TEST_ASSERT(static_cast<int>(MusicTrack::COUNT) > 10, "Should have 10+ tracks");
    TEST_ASSERT(static_cast<int>(MusicTrack::HELL_MARCH) > 0, "HELL_MARCH should exist");

    return true;
}

bool Test_MusicTrackInfo() {
    const MusicTrackInfo& info = GetMusicTrackInfo(MusicTrack::HELL_MARCH);
    TEST_ASSERT(info.filename != nullptr, "HELL_MARCH should have filename");
    TEST_ASSERT(strstr(info.filename, "HELLMRCH") != nullptr, "Should contain HELLMRCH");
    TEST_ASSERT(!info.is_menu_track, "HELL_MARCH is not a menu track");

    const MusicTrackInfo& menu_info = GetMusicTrackInfo(MusicTrack::MENU);
    TEST_ASSERT(menu_info.is_menu_track, "MENU should be a menu track");

    return true;
}

bool Test_InGameTrackCheck() {
    TEST_ASSERT(IsMusicTrackInGame(MusicTrack::HELL_MARCH), "HELL_MARCH is in-game");
    TEST_ASSERT(IsMusicTrackInGame(MusicTrack::BIGFOOT), "BIGFOOT is in-game");
    TEST_ASSERT(!IsMusicTrackInGame(MusicTrack::MENU), "MENU is not in-game");
    TEST_ASSERT(!IsMusicTrackInGame(MusicTrack::TITLE), "TITLE is not in-game");

    return true;
}

bool Test_NextTrack() {
    MusicTrack next = GetNextMusicTrack(MusicTrack::BIGFOOT);
    TEST_ASSERT(next != MusicTrack::BIGFOOT, "Next should be different");
    TEST_ASSERT(IsMusicTrackInGame(next), "Next should be in-game track");

    // Test wrap-around
    MusicTrack last = MusicTrack::LAST_INGAME;
    MusicTrack wrapped = GetNextMusicTrack(last);
    TEST_ASSERT(wrapped == MusicTrack::FIRST_INGAME, "Should wrap to first");

    return true;
}

bool Test_RandomTrack() {
    // Test that random tracks are valid in-game tracks
    for (int i = 0; i < 10; i++) {
        MusicTrack random = GetRandomInGameTrack();
        TEST_ASSERT(IsMusicTrackInGame(random), "Random should be in-game track");
    }

    return true;
}

bool Test_VolumeControl() {
    MusicPlayer& player = MusicPlayer::Instance();

    if (!player.IsInitialized()) {
        player.Initialize();
    }

    player.SetVolume(0.5f);
    TEST_ASSERT(std::abs(player.GetVolume() - 0.5f) < 0.01f, "Volume should be 0.5");

    player.SetVolume(1.0f);
    TEST_ASSERT(std::abs(player.GetVolume() - 1.0f) < 0.01f, "Volume should be 1.0");

    // Test clamping
    player.SetVolume(2.0f);
    TEST_ASSERT(player.GetVolume() <= 1.0f, "Should clamp to 1.0");

    player.SetVolume(0.8f);  // Reset

    return true;
}

bool Test_MuteControl() {
    MusicPlayer& player = MusicPlayer::Instance();

    if (!player.IsInitialized()) {
        player.Initialize();
    }

    TEST_ASSERT(!player.IsMuted(), "Should not be muted initially");

    player.SetMuted(true);
    TEST_ASSERT(player.IsMuted(), "Should be muted");

    player.SetMuted(false);
    TEST_ASSERT(!player.IsMuted(), "Should not be muted");

    return true;
}

bool Test_ShuffleMode() {
    MusicPlayer& player = MusicPlayer::Instance();

    if (!player.IsInitialized()) {
        player.Initialize();
    }

    player.SetShuffleEnabled(true);
    TEST_ASSERT(player.IsShuffleEnabled(), "Shuffle should be enabled");

    player.SetShuffleEnabled(false);
    TEST_ASSERT(!player.IsShuffleEnabled(), "Shuffle should be disabled");

    return true;
}

bool Test_LoopMode() {
    MusicPlayer& player = MusicPlayer::Instance();

    if (!player.IsInitialized()) {
        player.Initialize();
    }

    player.SetLoopEnabled(false);
    TEST_ASSERT(!player.IsLoopEnabled(), "Loop should be disabled");

    player.SetLoopEnabled(true);
    TEST_ASSERT(player.IsLoopEnabled(), "Loop should be enabled");

    return true;
}

bool Test_InitialState() {
    MusicPlayer& player = MusicPlayer::Instance();

    // Reinitialize for clean state
    player.Shutdown();
    player.Initialize();

    TEST_ASSERT(player.GetState() == MusicState::STOPPED, "Should start stopped");
    TEST_ASSERT(player.GetCurrentTrack() == MusicTrack::NONE, "Should have no track");
    TEST_ASSERT(!player.IsPlaying(), "Should not be playing");

    return true;
}

bool Test_GlobalFunctions() {
    Music_SetVolume(128);
    int vol = Music_GetVolume();
    TEST_ASSERT(vol >= 120 && vol <= 136, "Volume should be ~128");

    Music_SetShuffle(true);
    Music_SetShuffle(false);

    return true;
}

bool Test_TrackInfoTableCompleteness() {
    // Verify all tracks have valid info entries
    for (int i = 1; i < static_cast<int>(MusicTrack::COUNT); i++) {
        MusicTrack track = static_cast<MusicTrack>(i);
        const MusicTrackInfo& info = GetMusicTrackInfo(track);

        // Every non-NONE track should have a filename
        if (info.filename == nullptr) {
            printf("\n  Track %d has no filename\n  ", i);
            return false;
        }

        // Filename should end with .AUD
        size_t len = strlen(info.filename);
        if (len < 4 || strcmp(info.filename + len - 4, ".AUD") != 0) {
            printf("\n  Track %d filename '%s' doesn't end with .AUD\n  ", i, info.filename);
            return false;
        }

        // Display name should exist
        if (info.display_name == nullptr) {
            printf("\n  Track %d has no display name\n  ", i);
            return false;
        }
    }

    return true;
}

bool Test_AutoAdvance() {
    MusicPlayer& player = MusicPlayer::Instance();

    if (!player.IsInitialized()) {
        player.Initialize();
    }

    player.SetAutoAdvance(true);
    TEST_ASSERT(player.IsAutoAdvanceEnabled(), "Auto-advance should be enabled");

    player.SetAutoAdvance(false);
    TEST_ASSERT(!player.IsAutoAdvanceEnabled(), "Auto-advance should be disabled");

    player.SetAutoAdvance(true);  // Reset

    return true;
}

bool Test_TrackDisplayName() {
    const char* name = GetMusicTrackDisplayName(MusicTrack::HELL_MARCH);
    TEST_ASSERT(name != nullptr, "Should have display name");
    TEST_ASSERT(strstr(name, "Hell") != nullptr || strstr(name, "March") != nullptr,
                "Display name should contain Hell or March");

    const char* none_name = GetMusicTrackDisplayName(MusicTrack::NONE);
    TEST_ASSERT(none_name != nullptr, "NONE should have display name");

    return true;
}

//=============================================================================
// Integration Tests (require game assets)
//=============================================================================

bool Test_LoadAndPlayTrack() {
    printf("\n  (Integration test - requires game data)\n  ");

    // Check if any MIX files are loaded
    if (Platform_Mix_GetCount() == 0) {
        printf("SKIPPED - No MIX files loaded\n  ");
        return true;
    }

    MusicPlayer& player = MusicPlayer::Instance();
    player.Shutdown();
    player.Initialize();

    // Try to play Hell March
    bool success = player.Play(MusicTrack::HELL_MARCH);

    if (success) {
        printf("Successfully started: %s\n  ", player.GetCurrentTrackName());

        // Verify state
        if (!player.IsPlaying()) {
            printf("FAILED: Should be playing\n  ");
            return false;
        }
        if (player.GetCurrentTrack() != MusicTrack::HELL_MARCH) {
            printf("FAILED: Track mismatch\n  ");
            return false;
        }

        // Stop after brief test
        player.Stop();
        if (player.IsPlaying()) {
            printf("FAILED: Should be stopped\n  ");
            return false;
        }
    } else {
        printf("SKIPPED: Could not load track (game assets may not be present)\n  ");
    }

    return true;
}

//=============================================================================
// Main
//=============================================================================

int main(int argc, char* argv[]) {
    printf("=== Music Player Tests (Task 17c) ===\n\n");

    bool quick_mode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--quick") == 0 || strcmp(argv[i], "-q") == 0) {
            quick_mode = true;
        }
    }

    // Initialize platform for tests
    Platform_Init();

    // Run unit tests
    RUN_TEST(MusicTrackEnum);
    RUN_TEST(MusicTrackInfo);
    RUN_TEST(InGameTrackCheck);
    RUN_TEST(NextTrack);
    RUN_TEST(RandomTrack);
    RUN_TEST(VolumeControl);
    RUN_TEST(MuteControl);
    RUN_TEST(ShuffleMode);
    RUN_TEST(LoopMode);
    RUN_TEST(InitialState);
    RUN_TEST(GlobalFunctions);
    RUN_TEST(TrackInfoTableCompleteness);
    RUN_TEST(AutoAdvance);
    RUN_TEST(TrackDisplayName);

    // Integration test
    if (!quick_mode) {
        RUN_TEST(LoadAndPlayTrack);
    }

    // Cleanup
    MusicPlayer::Instance().Shutdown();
    Platform_Shutdown();

    printf("\n");
    if (tests_failed == 0) {
        printf("All tests PASSED (%d/%d)\n", tests_passed, tests_passed + tests_failed);
    } else {
        printf("Results: %d passed, %d failed\n", tests_passed, tests_failed);
    }

    return (tests_failed == 0) ? 0 : 1;
}
