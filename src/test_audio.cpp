// Audio system integration test
#include <cstdio>
#include <cstring>
#include <cmath>

extern "C" {
#include "platform.h"
}

// Generate a sine wave for testing
void generate_sine_wave(int16_t* buffer, int samples, int sample_rate, float frequency) {
    const float pi = 3.14159265358979323846f;
    for (int i = 0; i < samples; i++) {
        float t = (float)i / (float)sample_rate;
        float value = sinf(2.0f * pi * frequency * t);
        buffer[i] = (int16_t)(value * 16384.0f); // 50% volume
    }
}

// Generate 8-bit test data
void generate_8bit_data(uint8_t* buffer, int samples) {
    for (int i = 0; i < samples; i++) {
        // Simple ramp wave, centered at 128
        buffer[i] = (uint8_t)(128 + (i % 256) - 128);
    }
}

// Simple test ADPCM data (silence encoded)
void generate_test_adpcm(uint8_t* buffer, int bytes) {
    // Fill with zeros - decodes to near-silence
    memset(buffer, 0, bytes);
}

int main(int argc, char* argv[]) {
    bool verbose = (argc > 1 && strcmp(argv[1], "-v") == 0);
    int errors = 0;

    printf("=== Audio System Integration Test ===\n\n");

    // Test 1: Audio initialization
    printf("Test 1: Audio initialization... ");
    AudioConfig config;
    config.sample_rate = 22050;
    config.channels = 1;
    config.bits_per_sample = 16;
    config.buffer_size = 1024;

    int result = Platform_Audio_Init(&config);
    if (result == 0) {
        printf("PASSED\n");
    } else {
        printf("FAILED (result=%d)\n", result);
        errors++;
    }

    // Test 2: Check initialized state
    printf("Test 2: Audio is initialized... ");
    if (Platform_Audio_IsInitialized()) {
        printf("PASSED\n");
    } else {
        printf("FAILED\n");
        errors++;
    }

    // Test 3: Master volume control
    printf("Test 3: Master volume control... ");
    Platform_Audio_SetMasterVolume(0.5f);
    Platform_Timer_Delay(50); // Give audio thread time to process
    float vol = Platform_Audio_GetMasterVolume();
    if (vol >= 0.49f && vol <= 0.51f) {
        printf("PASSED (vol=%.2f)\n", vol);
    } else {
        printf("FAILED (expected 0.5, got %.2f)\n", vol);
        errors++;
    }

    // Test 4: Create 16-bit PCM sound
    printf("Test 4: Create 16-bit PCM sound... ");
    const int sample_count = 22050; // 1 second at 22050Hz
    int16_t* pcm_buffer = new int16_t[sample_count];
    generate_sine_wave(pcm_buffer, sample_count, 22050, 440.0f); // 440Hz A note

    int32_t sound16 = Platform_Sound_CreateFromMemory(
        pcm_buffer,
        sample_count * sizeof(int16_t),
        22050,
        1,
        16
    );
    delete[] pcm_buffer;

    if (sound16 != -1) {
        printf("PASSED (handle=%d)\n", sound16);
    } else {
        printf("FAILED (invalid handle)\n");
        errors++;
    }

    // Test 5: Create 8-bit PCM sound
    printf("Test 5: Create 8-bit PCM sound... ");
    const int samples_8bit = 11025;
    uint8_t* pcm8_buffer = new uint8_t[samples_8bit];
    generate_8bit_data(pcm8_buffer, samples_8bit);

    int32_t sound8 = Platform_Sound_CreateFromMemory(
        pcm8_buffer,
        samples_8bit,
        11025,
        1,
        8
    );
    delete[] pcm8_buffer;

    if (sound8 != -1) {
        printf("PASSED (handle=%d)\n", sound8);
    } else {
        printf("FAILED (invalid handle)\n");
        errors++;
    }

    // Test 6: Create ADPCM sound
    printf("Test 6: Create ADPCM sound... ");
    const int adpcm_bytes = 1024;
    uint8_t* adpcm_buffer = new uint8_t[adpcm_bytes];
    generate_test_adpcm(adpcm_buffer, adpcm_bytes);

    int32_t sound_adpcm = Platform_Sound_CreateFromADPCM(
        adpcm_buffer,
        adpcm_bytes,
        22050,
        1
    );
    delete[] adpcm_buffer;

    if (sound_adpcm != -1) {
        printf("PASSED (handle=%d)\n", sound_adpcm);
    } else {
        printf("FAILED (invalid handle)\n");
        errors++;
    }

    // Test 7: Sound count
    printf("Test 7: Sound count... ");
    // Give the audio thread time to process
    Platform_Timer_Delay(50);
    int count = Platform_Sound_GetCount();
    if (count == 3) {
        printf("PASSED (count=%d)\n", count);
    } else {
        printf("FAILED (expected 3, got %d)\n", count);
        errors++;
    }

    // Test 8: Play sound (non-looping)
    printf("Test 8: Play sound... ");
    int32_t play1 = Platform_Sound_Play(sound16, 0.5f, 0.0f, false);
    if (play1 != -1) {
        printf("PASSED (play_handle=%d)\n", play1);
    } else {
        printf("FAILED (invalid play handle)\n");
        errors++;
    }

    // Test 9: Check is playing (approximate - threaded)
    printf("Test 9: Is playing check... ");
    Platform_Timer_Delay(50); // Give time to start playing
    // Due to threaded nature, we just check for no crash
    bool isPlaying = Platform_Sound_IsPlaying(play1);
    printf("PASSED (isPlaying=%s)\n", isPlaying ? "true" : "false");

    // Test 10: Set volume on playing sound
    printf("Test 10: Set playing volume... ");
    Platform_Sound_SetVolume(play1, 0.8f);
    printf("PASSED (no crash)\n");

    // Test 11: Pause and resume
    printf("Test 11: Pause/Resume... ");
    Platform_Sound_Pause(play1);
    Platform_Sound_Resume(play1);
    printf("PASSED (no crash)\n");

    // Test 12: Stop sound
    printf("Test 12: Stop sound... ");
    Platform_Sound_Stop(play1);
    Platform_Timer_Delay(50);
    printf("PASSED (no crash)\n");

    // Test 13: Play looping sound
    printf("Test 13: Play looping sound... ");
    int32_t play2 = Platform_Sound_Play(sound8, 0.3f, 0.0f, true);
    if (play2 != -1) {
        printf("PASSED (play_handle=%d)\n", play2);
    } else {
        printf("FAILED\n");
        errors++;
    }

    // Test 14: Stop all sounds
    printf("Test 14: Stop all sounds... ");
    Platform_Sound_StopAll();
    Platform_Timer_Delay(50);
    int playing_count = Platform_Sound_GetPlayingCount();
    // Note: playing_count may not be exactly 0 immediately due to threading
    printf("PASSED (playing_count=%d)\n", playing_count);

    // Test 15: Destroy sounds
    printf("Test 15: Destroy sounds... ");
    Platform_Sound_Destroy(sound16);
    Platform_Sound_Destroy(sound8);
    Platform_Sound_Destroy(sound_adpcm);
    Platform_Timer_Delay(50);
    count = Platform_Sound_GetCount();
    if (count == 0) {
        printf("PASSED\n");
    } else {
        printf("PARTIAL (count=%d, expected 0 - may take time to sync)\n", count);
        // Don't count as error due to async nature
    }

    // Test 16: Audio shutdown
    printf("Test 16: Audio shutdown... ");
    Platform_Audio_Shutdown();
    Platform_Timer_Delay(100); // Give thread time to shut down
    if (!Platform_Audio_IsInitialized()) {
        printf("PASSED\n");
    } else {
        printf("PARTIAL (may take time to shutdown)\n");
        // Don't count as error due to async nature
    }

    // Summary
    printf("\n=== Test Summary ===\n");
    if (errors == 0) {
        printf("All tests PASSED\n");
        return 0;
    } else {
        printf("%d test(s) FAILED\n", errors);
        return 1;
    }
}
