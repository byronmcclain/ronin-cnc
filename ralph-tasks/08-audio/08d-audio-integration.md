# Task 08d: Audio Integration Test

## Dependencies
- Task 08c must be complete (ADPCM decoder)
- All previous audio tasks complete

## Context
This final audio task verifies the complete audio system integration by testing audio initialization, PCM sound creation and playback, and ADPCM decoding through the C++ integration layer. It ensures all FFI functions work correctly from C++.

## Objective
Create a C++ test that exercises all audio functionality: initialization, PCM sound creation, ADPCM decoding, playback control, and shutdown. Verify no memory leaks or crashes occur.

## Deliverables
- [ ] Complete audio module with all FFI exports
- [ ] Dedicated audio header section in `include/platform.h`
- [ ] C++ integration test in `src/test_audio.cpp`
- [ ] CMake integration for audio test
- [ ] Verification passes

## Files to Modify

### platform/src/ffi/audio.rs (NEW - Optional Refactor)
If the ffi/mod.rs becomes too large, create a dedicated audio FFI module:
```rust
//! Audio FFI exports
//!
//! This module provides C-compatible functions for audio operations.

use crate::audio::{self, AudioConfig, SoundHandle, PlayHandle, INVALID_SOUND_HANDLE, INVALID_PLAY_HANDLE};
use crate::audio::adpcm;
use std::ffi::c_void;

// Re-export from main ffi module
pub use crate::audio::{SoundHandle, PlayHandle, INVALID_SOUND_HANDLE, INVALID_PLAY_HANDLE};

// All FFI functions should be in platform/src/ffi/mod.rs
// This file can be used for audio-specific helpers if needed
```

### src/test_audio.cpp (NEW)
```cpp
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

    SoundHandle sound16 = Platform_Sound_CreateFromMemory(
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

    SoundHandle sound8 = Platform_Sound_CreateFromMemory(
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

    SoundHandle sound_adpcm = Platform_Sound_CreateFromADPCM(
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
    int count = Platform_Sound_GetCount();
    if (count == 3) {
        printf("PASSED (count=%d)\n", count);
    } else {
        printf("FAILED (expected 3, got %d)\n", count);
        errors++;
    }

    // Test 8: Play sound (non-looping)
    printf("Test 8: Play sound... ");
    PlayHandle play1 = Platform_Sound_Play(sound16, 0.5f, 0.0f, false);
    if (play1 != -1) {
        printf("PASSED (play_handle=%d)\n", play1);
    } else {
        printf("FAILED (invalid play handle)\n");
        errors++;
    }

    // Test 9: Check is playing
    printf("Test 9: Is playing check... ");
    if (Platform_Sound_IsPlaying(play1)) {
        printf("PASSED\n");
    } else {
        // Sound might have finished immediately due to test conditions
        printf("SKIPPED (sound may have finished)\n");
    }

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
    if (!Platform_Sound_IsPlaying(play1)) {
        printf("PASSED\n");
    } else {
        printf("FAILED (still playing after stop)\n");
        errors++;
    }

    // Test 13: Play looping sound
    printf("Test 13: Play looping sound... ");
    PlayHandle play2 = Platform_Sound_Play(sound8, 0.3f, 0.0f, true);
    if (play2 != -1) {
        printf("PASSED (play_handle=%d)\n", play2);
    } else {
        printf("FAILED\n");
        errors++;
    }

    // Test 14: Stop all sounds
    printf("Test 14: Stop all sounds... ");
    Platform_Sound_StopAll();
    int playing_count = Platform_Sound_GetPlayingCount();
    if (playing_count == 0) {
        printf("PASSED\n");
    } else {
        printf("FAILED (still %d sounds playing)\n", playing_count);
        errors++;
    }

    // Test 15: Destroy sounds
    printf("Test 15: Destroy sounds... ");
    Platform_Sound_Destroy(sound16);
    Platform_Sound_Destroy(sound8);
    Platform_Sound_Destroy(sound_adpcm);
    count = Platform_Sound_GetCount();
    if (count == 0) {
        printf("PASSED\n");
    } else {
        printf("FAILED (count=%d after destroy)\n", count);
        errors++;
    }

    // Test 16: Audio shutdown
    printf("Test 16: Audio shutdown... ");
    Platform_Audio_Shutdown();
    if (!Platform_Audio_IsInitialized()) {
        printf("PASSED\n");
    } else {
        printf("FAILED (still initialized)\n");
        errors++;
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
```

### CMakeLists.txt (MODIFY)
Add audio test executable:
```cmake
# Audio integration test
add_executable(TestAudio src/test_audio.cpp)
target_link_libraries(TestAudio PRIVATE redalert_platform)
target_include_directories(TestAudio PRIVATE ${CMAKE_SOURCE_DIR}/include)
```

## Verification Command
```bash
cd /Users/ooartist/Src/Ronin_CnC && \
cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1 && \
cmake -B build -G Ninja 2>&1 && \
cmake --build build --target TestAudio 2>&1 && \
./build/TestAudio && \
echo "VERIFY_SUCCESS"
```

## Implementation Steps
1. Ensure all audio FFI exports are in `platform/src/ffi/mod.rs`
2. Verify header has all required audio function declarations
3. Create `src/test_audio.cpp` with comprehensive tests
4. Update CMakeLists.txt to build TestAudio
5. Build the platform library
6. Build the test executable
7. Run the test and verify all tests pass
8. Run verification command

## Completion Promise
When verification passes, output:
```
<promise>TASK_08D_COMPLETE</promise>
```

## Escape Hatch
If stuck after 15 iterations:
- Document what's blocking in `ralph-tasks/blocked/08d.md`
- List attempted approaches
- Output: `<promise>TASK_08D_BLOCKED</promise>`

## Max Iterations
15
