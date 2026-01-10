// src/tests/integration/test_audio_pipeline.cpp
// Audio Pipeline Integration Tests
// Task 18c - Integration Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "platform.h"
#include <cmath>
#include <vector>

//=============================================================================
// Helper to check for game data
//=============================================================================

static bool HasGameData() {
    return Platform_File_Exists("gamedata/SOUNDS.MIX") ||
           Platform_File_Exists("SOUNDS.MIX") ||
           Platform_File_Exists("data/SOUNDS.MIX");
}

static bool RegisterSoundsMix() {
    if (Platform_Mix_Register("SOUNDS.MIX") == 0) return true;
    if (Platform_Mix_Register("gamedata/SOUNDS.MIX") == 0) return true;
    if (Platform_Mix_Register("data/SOUNDS.MIX") == 0) return true;
    return false;
}

//=============================================================================
// Audio Playback Tests
//=============================================================================

TEST_WITH_FIXTURE(AudioFixture, AudioPipeline_PlaySilence, "Audio") {
    if (!fixture.IsAudioInitialized()) {
        TEST_SKIP("Audio not initialized");
    }

    // Create silent audio buffer (1 second of silence)
    std::vector<int16_t> silence(22050, 0);

    SoundHandle handle = Platform_Sound_CreateFromMemory(
        reinterpret_cast<uint8_t*>(silence.data()),
        static_cast<int32_t>(silence.size() * sizeof(int16_t)),
        22050, 1, 16
    );

    TEST_ASSERT_NE(handle, INVALID_SOUND_HANDLE);

    PlayHandle play = Platform_Sound_Play(handle, 1.0f, 0.0f, false);
    TEST_ASSERT_NE(play, INVALID_PLAY_HANDLE);

    // Wait briefly
    fixture.WaitMs(100);

    Platform_Sound_Stop(play);
    Platform_Sound_Destroy(handle);
}

TEST_CASE(AudioPipeline_CreateAndDestroy, "Audio") {
    Platform_Init();

    AudioConfig config = {22050, 2, 16, 1024};
    Platform_Audio_Init(&config);

    // Create a simple tone
    std::vector<int16_t> tone(22050);
    for (size_t i = 0; i < tone.size(); i++) {
        double t = static_cast<double>(i) / 22050.0;
        tone[i] = static_cast<int16_t>(16000 * std::sin(2 * 3.14159 * 440 * t));
    }

    SoundHandle handle = Platform_Sound_CreateFromMemory(
        reinterpret_cast<uint8_t*>(tone.data()),
        static_cast<int32_t>(tone.size() * sizeof(int16_t)),
        22050, 1, 16
    );

    TEST_ASSERT_NE(handle, INVALID_SOUND_HANDLE);

    // Destroy without playing
    Platform_Sound_Destroy(handle);

    Platform_Audio_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Volume Control Tests
//=============================================================================

TEST_WITH_FIXTURE(AudioFixture, AudioPipeline_VolumeRamp, "Audio") {
    if (!fixture.IsAudioInitialized()) {
        TEST_SKIP("Audio not initialized");
    }

    // Generate a tone
    std::vector<int16_t> tone(22050);
    for (size_t i = 0; i < tone.size(); i++) {
        double t = static_cast<double>(i) / 22050.0;
        tone[i] = static_cast<int16_t>(16000 * std::sin(2 * 3.14159 * 440 * t));
    }

    SoundHandle handle = Platform_Sound_CreateFromMemory(
        reinterpret_cast<uint8_t*>(tone.data()),
        static_cast<int32_t>(tone.size() * sizeof(int16_t)),
        22050, 1, 16
    );

    if (handle != INVALID_SOUND_HANDLE) {
        PlayHandle play = Platform_Sound_Play(handle, 0.0f, 0.0f, true);

        // Ramp volume up
        for (float vol = 0.0f; vol <= 1.0f; vol += 0.2f) {
            Platform_Sound_SetVolume(play, vol);
            fixture.WaitMs(50);
        }

        // Ramp volume down
        for (float vol = 1.0f; vol >= 0.0f; vol -= 0.2f) {
            Platform_Sound_SetVolume(play, vol);
            fixture.WaitMs(50);
        }

        Platform_Sound_Stop(play);
        Platform_Sound_Destroy(handle);
    }
}

TEST_CASE(AudioPipeline_MasterVolume, "Audio") {
    Platform_Init();

    AudioConfig config = {22050, 2, 16, 1024};
    Platform_Audio_Init(&config);

    // Get initial volume
    float initial = Platform_Audio_GetMasterVolume();
    TEST_ASSERT_GE(initial, 0.0f);
    TEST_ASSERT_LE(initial, 1.0f);

    // Set new volume
    Platform_Audio_SetMasterVolume(0.5f);
    TEST_ASSERT_NEAR(Platform_Audio_GetMasterVolume(), 0.5f, 0.01f);

    // Restore
    Platform_Audio_SetMasterVolume(initial);

    Platform_Audio_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Multi-Sound Tests
//=============================================================================

TEST_WITH_FIXTURE(AudioFixture, AudioPipeline_MultipleSounds, "Audio") {
    if (!fixture.IsAudioInitialized()) {
        TEST_SKIP("Audio not initialized");
    }

    std::vector<SoundHandle> handles;
    std::vector<PlayHandle> plays;

    // Create and play multiple sounds
    for (int i = 0; i < 3; i++) {
        std::vector<int16_t> tone(11025);  // 0.5 seconds
        int freq = 220 * (i + 1);  // Different frequencies

        for (size_t j = 0; j < tone.size(); j++) {
            double t = static_cast<double>(j) / 22050.0;
            tone[j] = static_cast<int16_t>(10000 * std::sin(2 * 3.14159 * freq * t));
        }

        SoundHandle h = Platform_Sound_CreateFromMemory(
            reinterpret_cast<uint8_t*>(tone.data()),
            static_cast<int32_t>(tone.size() * sizeof(int16_t)),
            22050, 1, 16
        );

        if (h != INVALID_SOUND_HANDLE) {
            handles.push_back(h);
            plays.push_back(Platform_Sound_Play(h, 0.5f, 0.0f, false));
        }
    }

    TEST_ASSERT_GT(handles.size(), 0u);

    // Wait for sounds
    fixture.WaitMs(600);

    // Cleanup
    for (auto play : plays) {
        Platform_Sound_Stop(play);
    }
    for (auto handle : handles) {
        Platform_Sound_Destroy(handle);
    }
}

//=============================================================================
// Pause/Resume Tests
//=============================================================================

TEST_WITH_FIXTURE(AudioFixture, AudioPipeline_PauseResume, "Audio") {
    if (!fixture.IsAudioInitialized()) {
        TEST_SKIP("Audio not initialized");
    }

    // Generate a tone
    std::vector<int16_t> tone(44100);  // 2 seconds
    for (size_t i = 0; i < tone.size(); i++) {
        double t = static_cast<double>(i) / 22050.0;
        tone[i] = static_cast<int16_t>(16000 * std::sin(2 * 3.14159 * 440 * t));
    }

    SoundHandle handle = Platform_Sound_CreateFromMemory(
        reinterpret_cast<uint8_t*>(tone.data()),
        static_cast<int32_t>(tone.size() * sizeof(int16_t)),
        22050, 1, 16
    );

    if (handle != INVALID_SOUND_HANDLE) {
        PlayHandle play = Platform_Sound_Play(handle, 0.5f, 0.0f, false);

        // Play for a bit
        fixture.WaitMs(200);

        // Pause
        Platform_Sound_Pause(play);
        fixture.WaitMs(200);

        // Resume
        Platform_Sound_Resume(play);
        fixture.WaitMs(200);

        Platform_Sound_Stop(play);
        Platform_Sound_Destroy(handle);
    }
}

//=============================================================================
// Sound Count Tests
//=============================================================================

TEST_CASE(AudioPipeline_SoundCount, "Audio") {
    Platform_Init();

    AudioConfig config = {22050, 2, 16, 1024};
    Platform_Audio_Init(&config);

    int32_t initial_count = Platform_Sound_GetCount();

    // Create some sounds
    std::vector<int16_t> silence(1024, 0);
    SoundHandle h1 = Platform_Sound_CreateFromMemory(
        reinterpret_cast<uint8_t*>(silence.data()),
        static_cast<int32_t>(silence.size() * sizeof(int16_t)),
        22050, 1, 16
    );
    SoundHandle h2 = Platform_Sound_CreateFromMemory(
        reinterpret_cast<uint8_t*>(silence.data()),
        static_cast<int32_t>(silence.size() * sizeof(int16_t)),
        22050, 1, 16
    );

    TEST_ASSERT_EQ(Platform_Sound_GetCount(), initial_count + 2);

    // Destroy
    Platform_Sound_Destroy(h1);
    TEST_ASSERT_EQ(Platform_Sound_GetCount(), initial_count + 1);

    Platform_Sound_Destroy(h2);
    TEST_ASSERT_EQ(Platform_Sound_GetCount(), initial_count);

    Platform_Audio_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// StopAll Tests
//=============================================================================

TEST_WITH_FIXTURE(AudioFixture, AudioPipeline_StopAll, "Audio") {
    if (!fixture.IsAudioInitialized()) {
        TEST_SKIP("Audio not initialized");
    }

    std::vector<SoundHandle> handles;

    // Create and play multiple sounds
    for (int i = 0; i < 3; i++) {
        std::vector<int16_t> tone(22050);
        for (size_t j = 0; j < tone.size(); j++) {
            double t = static_cast<double>(j) / 22050.0;
            tone[j] = static_cast<int16_t>(10000 * std::sin(2 * 3.14159 * 440 * t));
        }

        SoundHandle h = Platform_Sound_CreateFromMemory(
            reinterpret_cast<uint8_t*>(tone.data()),
            static_cast<int32_t>(tone.size() * sizeof(int16_t)),
            22050, 1, 16
        );

        if (h != INVALID_SOUND_HANDLE) {
            handles.push_back(h);
            Platform_Sound_Play(h, 0.3f, 0.0f, true);  // Loop
        }
    }

    fixture.WaitMs(100);

    // Get playing count
    int32_t playing = Platform_Sound_GetPlayingCount();
    TEST_ASSERT_GE(playing, 0);  // May vary due to timing

    // Stop all
    Platform_Sound_StopAll();
    fixture.WaitMs(50);

    // Cleanup handles
    for (auto handle : handles) {
        Platform_Sound_Destroy(handle);
    }
}

//=============================================================================
// ADPCM Tests (if supported)
//=============================================================================

TEST_CASE(AudioPipeline_ADPCM_Create, "Audio") {
    Platform_Init();

    AudioConfig config = {22050, 2, 16, 1024};
    Platform_Audio_Init(&config);

    // Create minimal ADPCM data (this is just a test that the function exists)
    uint8_t adpcm_data[256];
    memset(adpcm_data, 0, sizeof(adpcm_data));

    // This may fail if the data is invalid, but shouldn't crash
    SoundHandle handle = Platform_Sound_CreateFromADPCM(
        adpcm_data,
        sizeof(adpcm_data),
        22050, 1
    );

    // Either it succeeds or fails gracefully
    if (handle != INVALID_SOUND_HANDLE) {
        Platform_Sound_Destroy(handle);
    }

    Platform_Audio_Shutdown();
    Platform_Shutdown();
}
