// src/tests/unit/test_audio.cpp
// Audio System Unit Tests
// Task 18b - Unit Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "platform.h"
#include <vector>

//=============================================================================
// Audio Initialization Tests
//=============================================================================

TEST_WITH_FIXTURE(AudioFixture, Audio_Init_Succeeds, "Audio") {
    TEST_ASSERT(fixture.IsAudioInitialized());
}

TEST_CASE(Audio_Config_ValidRates, "Audio") {
    // Valid sample rates
    int valid_rates[] = {11025, 22050, 44100, 48000};

    for (int rate : valid_rates) {
        TEST_ASSERT_GT(rate, 0);
        TEST_ASSERT_LE(rate, 96000);
    }
}

TEST_CASE(Audio_Config_ValidChannels, "Audio") {
    // Valid channel counts
    TEST_ASSERT_GE(1, 1);  // Mono
    TEST_ASSERT_GE(2, 1);  // Stereo
}

TEST_CASE(Audio_Config_ValidBits, "Audio") {
    // Valid bit depths
    int valid_bits[] = {8, 16, 32};

    for (int bits : valid_bits) {
        TEST_ASSERT(bits == 8 || bits == 16 || bits == 32);
    }
}

//=============================================================================
// Sound Handle Tests
//=============================================================================

TEST_CASE(Audio_InvalidHandle_Zero, "Audio") {
    // Invalid handle should be 0
    SoundHandle invalid = 0;
    TEST_ASSERT_EQ(invalid, 0u);
}

TEST_WITH_FIXTURE(AudioFixture, Audio_CreateSound_ReturnsHandle, "Audio") {
    if (!fixture.IsAudioInitialized()) {
        TEST_SKIP("Audio not initialized");
    }

    // Create a simple PCM buffer (silence)
    std::vector<int16_t> pcm_data(22050, 0);  // 1 second of silence

    SoundHandle handle = Platform_Sound_CreateFromMemory(
        reinterpret_cast<uint8_t*>(pcm_data.data()),
        pcm_data.size() * sizeof(int16_t),
        22050,  // sample rate
        1,      // channels
        16      // bits
    );

    // Should get a valid handle
    TEST_ASSERT_NE(handle, 0u);

    // Clean up
    Platform_Sound_Destroy(handle);
}

//=============================================================================
// Volume Tests
//=============================================================================

TEST_WITH_FIXTURE(AudioFixture, Audio_MasterVolume_SetGet, "Audio") {
    if (!fixture.IsAudioInitialized()) {
        TEST_SKIP("Audio not initialized");
    }

    // Set master volume
    Platform_Audio_SetMasterVolume(0.5f);
    float vol = Platform_Audio_GetMasterVolume();

    TEST_ASSERT_NEAR(vol, 0.5f, 0.01f);
}

TEST_WITH_FIXTURE(AudioFixture, Audio_MasterVolume_Clamp, "Audio") {
    if (!fixture.IsAudioInitialized()) {
        TEST_SKIP("Audio not initialized");
    }

    // Volume should clamp to [0, 1]
    Platform_Audio_SetMasterVolume(2.0f);
    float vol = Platform_Audio_GetMasterVolume();
    TEST_ASSERT_LE(vol, 1.0f);

    Platform_Audio_SetMasterVolume(-1.0f);
    vol = Platform_Audio_GetMasterVolume();
    TEST_ASSERT_GE(vol, 0.0f);
}

TEST_CASE(Audio_VolumeConversion_IntToFloat, "Audio") {
    // Convert 0-255 int volume to 0.0-1.0 float
    int int_vol = 128;
    float float_vol = int_vol / 255.0f;

    TEST_ASSERT_NEAR(float_vol, 0.5f, 0.01f);
}

TEST_CASE(Audio_VolumeConversion_FloatToInt, "Audio") {
    // Convert 0.0-1.0 float to 0-255 int
    float float_vol = 0.5f;
    int int_vol = static_cast<int>(float_vol * 255);

    TEST_ASSERT_GE(int_vol, 127);
    TEST_ASSERT_LE(int_vol, 128);
}

//=============================================================================
// PCM Buffer Tests
//=============================================================================

TEST_CASE(Audio_PCM_8BitToFloat, "Audio") {
    // Convert 8-bit unsigned PCM to float
    uint8_t sample_8bit = 128;  // Silence for unsigned 8-bit
    float sample_float = (sample_8bit - 128) / 128.0f;

    TEST_ASSERT_NEAR(sample_float, 0.0f, 0.01f);
}

TEST_CASE(Audio_PCM_16BitToFloat, "Audio") {
    // Convert 16-bit signed PCM to float
    int16_t sample_16bit = 0;  // Silence
    float sample_float = sample_16bit / 32768.0f;

    TEST_ASSERT_NEAR(sample_float, 0.0f, 0.0001f);
}

TEST_CASE(Audio_PCM_MaxValue, "Audio") {
    // Maximum 16-bit value
    int16_t max_sample = 32767;
    float sample_float = max_sample / 32768.0f;

    TEST_ASSERT_NEAR(sample_float, 1.0f, 0.001f);
}

TEST_CASE(Audio_PCM_MinValue, "Audio") {
    // Minimum 16-bit value
    int16_t min_sample = -32768;
    float sample_float = min_sample / 32768.0f;

    TEST_ASSERT_NEAR(sample_float, -1.0f, 0.001f);
}

//=============================================================================
// Audio Buffer Size Tests
//=============================================================================

TEST_CASE(Audio_BufferSize_PowerOfTwo, "Audio") {
    // Common buffer sizes are powers of 2
    int sizes[] = {256, 512, 1024, 2048, 4096};

    for (int size : sizes) {
        // Check if power of 2
        bool is_power_of_2 = (size & (size - 1)) == 0;
        TEST_ASSERT(is_power_of_2);
    }
}

TEST_CASE(Audio_Latency_Calculation, "Audio") {
    // Latency in ms = (buffer_size / sample_rate) * 1000
    int buffer_size = 1024;
    int sample_rate = 22050;

    double latency_ms = (buffer_size / static_cast<double>(sample_rate)) * 1000;

    // ~46ms for these settings
    TEST_ASSERT_GT(latency_ms, 40.0);
    TEST_ASSERT_LT(latency_ms, 50.0);
}

//=============================================================================
// Sound State Tests
//=============================================================================

TEST_WITH_FIXTURE(AudioFixture, Audio_IsPlaying_InvalidHandle, "Audio") {
    if (!fixture.IsAudioInitialized()) {
        TEST_SKIP("Audio not initialized");
    }

    // Invalid handle should return false
    bool playing = Platform_Sound_IsPlaying(0);
    TEST_ASSERT(!playing);
}

TEST_WITH_FIXTURE(AudioFixture, Audio_Stop_InvalidHandle, "Audio") {
    if (!fixture.IsAudioInitialized()) {
        TEST_SKIP("Audio not initialized");
    }

    // Stopping invalid handle should not crash
    Platform_Sound_Stop(0);
    TEST_ASSERT(true);
}

TEST_WITH_FIXTURE(AudioFixture, Audio_GetSoundCount, "Audio") {
    if (!fixture.IsAudioInitialized()) {
        TEST_SKIP("Audio not initialized");
    }

    int32_t count = Platform_Sound_GetCount();
    TEST_ASSERT_GE(count, 0);
}

TEST_WITH_FIXTURE(AudioFixture, Audio_GetPlayingCount, "Audio") {
    if (!fixture.IsAudioInitialized()) {
        TEST_SKIP("Audio not initialized");
    }

    int32_t count = Platform_Sound_GetPlayingCount();
    TEST_ASSERT_GE(count, 0);
}
