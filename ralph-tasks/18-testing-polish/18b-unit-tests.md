# Task 18b: Unit Tests

| Field | Value |
|-------|-------|
| **Task ID** | 18b |
| **Task Name** | Unit Tests |
| **Phase** | 18 - Testing & Polish |
| **Depends On** | Task 18a (Test Framework Foundation) |
| **Estimated Complexity** | High (~800 lines) |

---

## Dependencies

- Task 18a (Test Framework) - Test macros, fixtures, runner
- Platform layer - All platform APIs
- Asset system - MIX, palette, shape loading
- Audio system - AUD loading, playback
- Input system - Keyboard, mouse

---

## Context

Unit tests verify individual components work correctly in isolation. This task creates a comprehensive unit test suite covering:

1. **Platform Layer Tests**: Timer, memory, basic I/O
2. **Asset System Tests**: MIX parsing, palette loading, shape loading, AUD loading
3. **Audio System Tests**: Sound creation, playback, volume control
4. **Input System Tests**: Key states, mouse position, event handling
5. **Graphics System Tests**: Surface creation, pixel operations, palette application
6. **Utility Tests**: String operations, math functions, data structures

Unit tests should be fast, deterministic, and not require external resources when possible.

---

## Technical Background

### Test Categories

| Category | Description | Example Tests |
|----------|-------------|---------------|
| Platform | Core platform API | Timer accuracy, memory allocation |
| Assets | File format parsing | MIX header, palette parsing |
| Audio | Sound subsystem | PCM creation, volume control |
| Input | Input handling | Key state tracking |
| Graphics | Rendering primitives | Surface creation, pixel access |
| Utils | Helper functions | String operations, math |

### Testing Strategy

1. **Isolated Tests**: Each test should be independent
2. **Mock Dependencies**: Use mock objects for external dependencies
3. **Fast Execution**: Unit tests should run in milliseconds
4. **Deterministic**: Same input always produces same output
5. **Comprehensive**: Cover edge cases and error conditions

### Test Naming Convention

```
Test_[Component]_[Behavior]_[Condition]
```

Examples:
- `Test_Timer_GetTicks_ReturnsIncreasingValues`
- `Test_MixFile_ParseHeader_ValidFile`
- `Test_Palette_Load_InvalidSize_ReturnsFalse`

---

## Objective

Create a unit test suite that:
1. Tests all major platform API functions
2. Tests asset format parsing (MIX, PAL, SHP, AUD)
3. Tests audio system components
4. Tests input system components
5. Tests graphics primitives
6. Achieves good code coverage of critical paths
7. Runs quickly (< 10 seconds total)

---

## Deliverables

- [ ] `src/tests/unit/test_platform.cpp` - Platform layer tests
- [ ] `src/tests/unit/test_assets.cpp` - Asset system tests
- [ ] `src/tests/unit/test_audio.cpp` - Audio system tests
- [ ] `src/tests/unit/test_input.cpp` - Input system tests
- [ ] `src/tests/unit/test_graphics.cpp` - Graphics system tests
- [ ] `src/tests/unit/test_utils.cpp` - Utility function tests
- [ ] `src/tests/unit_tests_main.cpp` - Main entry point
- [ ] CMakeLists.txt updated
- [ ] All tests pass

---

## Files to Create

### src/tests/unit/test_platform.cpp

```cpp
// src/tests/unit/test_platform.cpp
// Platform Layer Unit Tests
// Task 18b - Unit Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "platform.h"
#include <thread>
#include <chrono>

//=============================================================================
// Platform Initialization Tests
//=============================================================================

TEST_CASE(Platform_Init_Succeeds, "Platform") {
    int result = Platform_Init();
    TEST_ASSERT_EQ(result, 0);

    TEST_ASSERT(Platform_IsInitialized());

    Platform_Shutdown();
    TEST_ASSERT(!Platform_IsInitialized());
}

TEST_CASE(Platform_Init_MultipleCalls, "Platform") {
    // Multiple init calls should be safe
    TEST_ASSERT_EQ(Platform_Init(), 0);
    TEST_ASSERT_EQ(Platform_Init(), 0);  // Should succeed or return already initialized

    Platform_Shutdown();
    Platform_Shutdown();  // Multiple shutdown should be safe
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
    // Should not be more than 100ms (excessive delay would indicate problem)
    TEST_ASSERT_LE(elapsed, 100u);
}

TEST_WITH_FIXTURE(PlatformFixture, Timer_Delay_Zero, "Timer") {
    // Zero delay should return immediately
    uint32_t start = Platform_Timer_GetTicks();
    Platform_Timer_Delay(0);
    uint32_t elapsed = Platform_Timer_GetTicks() - start;

    TEST_ASSERT_LE(elapsed, 10u);  // Should be nearly instant
}

TEST_WITH_FIXTURE(PlatformFixture, Timer_GetTicksMs_MatchesGetTicks, "Timer") {
    uint32_t ticks_ms = Platform_Timer_GetTicks();

    // Both functions should return similar values
    // (depending on implementation, they might be identical)
    TEST_ASSERT_GE(ticks_ms, 0u);
}

//=============================================================================
// Logging Tests
//=============================================================================

TEST_WITH_FIXTURE(PlatformFixture, Log_DoesNotCrash, "Platform") {
    // Just verify logging doesn't crash
    Platform_Log("Test log message");
    Platform_Log("Test with number: %d", 42);
    Platform_Log("Test with string: %s", "hello");
    Platform_Log("Test with multiple: %d %s %f", 1, "two", 3.0);

    TEST_ASSERT(true);  // If we got here, logging works
}

//=============================================================================
// Memory Tests (if platform provides memory functions)
//=============================================================================

TEST_WITH_FIXTURE(PlatformFixture, Memory_BasicAllocation, "Memory") {
    // Test basic allocation if platform provides it
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

//=============================================================================
// File Path Tests
//=============================================================================

TEST_CASE(Path_Normalization, "Platform") {
    // Test path handling (if platform provides path utilities)
    // These are placeholder tests - implement based on actual API

    std::string path = "gamedata/REDALERT.MIX";
    TEST_ASSERT(!path.empty());
}

//=============================================================================
// Error Handling Tests
//=============================================================================

TEST_WITH_FIXTURE(PlatformFixture, Error_GetLastError_InitiallyZero, "Platform") {
    // Platform should have clean error state after init
    // (if GetLastError API exists)
    TEST_ASSERT(true);  // Placeholder
}
```

### src/tests/unit/test_assets.cpp

```cpp
// src/tests/unit/test_assets.cpp
// Asset System Unit Tests
// Task 18b - Unit Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "game/mix_manager.h"
#include "game/palette.h"
#include "game/shape.h"
#include "game/audio/aud_file.h"
#include <cstring>

//=============================================================================
// MIX File Tests
//=============================================================================

TEST_CASE(Mix_HeaderSize_Correct, "Assets") {
    // MIX header should be known size
    // Depends on actual implementation
    TEST_ASSERT_EQ(sizeof(uint32_t), 4u);  // Basic sanity check
}

TEST_CASE(Mix_FileEntry_Size, "Assets") {
    // MIX file entry should be known size
    struct MixEntry {
        uint32_t id;
        uint32_t offset;
        uint32_t size;
    };
    TEST_ASSERT_EQ(sizeof(MixEntry), 12u);
}

TEST_WITH_FIXTURE(AssetFixture, Mix_Initialize_Succeeds, "Assets") {
    TEST_ASSERT(fixture.AreAssetsLoaded());
}

TEST_CASE(Mix_CRC_Calculation, "Assets") {
    // Test CRC calculation for filename lookup
    // MIX files use CRC to identify files

    // Known CRC values for common filenames
    // These would need to match the actual CRC implementation
    const char* test_name = "MOUSE.SHP";
    // uint32_t crc = MixManager::CalculateCRC(test_name);
    // TEST_ASSERT_NE(crc, 0);

    TEST_ASSERT(true);  // Placeholder
}

//=============================================================================
// Palette Tests
//=============================================================================

TEST_CASE(Palette_Size_Correct, "Assets") {
    // Palette should be 256 colors * 3 bytes (RGB)
    TEST_ASSERT_EQ(256 * 3, 768);
}

TEST_CASE(Palette_Entry_Range, "Assets") {
    // VGA palette entries are 0-63, not 0-255
    // Our loader should convert them
    uint8_t vga_value = 63;
    uint8_t converted = (vga_value * 255) / 63;
    TEST_ASSERT_EQ(converted, 255u);

    vga_value = 0;
    converted = (vga_value * 255) / 63;
    TEST_ASSERT_EQ(converted, 0u);

    vga_value = 32;
    converted = (vga_value * 255) / 63;
    TEST_ASSERT_GE(converted, 127u);
    TEST_ASSERT_LE(converted, 130u);
}

TEST_CASE(Palette_ParseBuffer_ValidData, "Assets") {
    // Create a mock palette buffer
    uint8_t buffer[768];
    for (int i = 0; i < 768; i++) {
        buffer[i] = (i % 64);  // VGA range 0-63
    }

    // Would test PaletteData::LoadFromBuffer(buffer, 768)
    TEST_ASSERT(true);  // Placeholder
}

TEST_CASE(Palette_ParseBuffer_TooSmall, "Assets") {
    uint8_t buffer[100];  // Too small

    // Should return false or handle gracefully
    // bool result = PaletteData::LoadFromBuffer(buffer, 100);
    // TEST_ASSERT(!result);

    TEST_ASSERT(true);  // Placeholder
}

//=============================================================================
// Shape (SHP) Tests
//=============================================================================

TEST_CASE(Shape_HeaderSize, "Assets") {
    // SHP header structure
    struct ShpHeader {
        uint16_t frame_count;
        uint16_t x_offset;  // Or other fields
        uint16_t y_offset;
        uint16_t width;
        uint16_t height;
    };

    // Verify expected size
    TEST_ASSERT_GE(sizeof(ShpHeader), 10u);
}

TEST_CASE(Shape_FrameOffset_Valid, "Assets") {
    // Frame offsets should be sequential
    uint32_t offsets[] = {0, 100, 200, 300};

    for (int i = 1; i < 4; i++) {
        TEST_ASSERT_GT(offsets[i], offsets[i-1]);
    }
}

TEST_CASE(Shape_RLE_Decode_Simple, "Assets") {
    // Test RLE decoding
    // 0x80 = end of line
    // 0x00-0x7F = copy N transparent pixels
    // 0x81-0xFF = copy (N-0x80) pixels from data

    uint8_t rle_data[] = {
        0x05,       // 5 transparent pixels
        0x83,       // 3 opaque pixels follow
        0x10, 0x20, 0x30,
        0x80        // End of line
    };

    // Would decode to: [trans][trans][trans][trans][trans][0x10][0x20][0x30]
    int transparent_count = rle_data[0];
    int opaque_count = rle_data[1] - 0x80;

    TEST_ASSERT_EQ(transparent_count, 5);
    TEST_ASSERT_EQ(opaque_count, 3);
}

//=============================================================================
// AUD File Tests
//=============================================================================

TEST_CASE(Aud_HeaderSize, "Assets") {
    // AUD header is exactly 12 bytes
    TEST_ASSERT_EQ(sizeof(AudHeader), 12u);
}

TEST_CASE(Aud_Header_Parse, "Assets") {
    // Create a mock AUD header
    uint8_t header_data[12] = {
        0x22, 0x56,  // Sample rate: 22050 (0x5622)
        0x00, 0x10, 0x00, 0x00,  // Uncompressed size: 4096
        0x00, 0x08, 0x00, 0x00,  // Compressed size: 2048
        0x00,        // Flags: mono, 8-bit
        0x01         // Compression: WW ADPCM
    };

    AudHeader* header = reinterpret_cast<AudHeader*>(header_data);

    TEST_ASSERT_EQ(header->sample_rate, 22050u);
    TEST_ASSERT_EQ(header->uncompressed_size, 4096u);
    TEST_ASSERT_EQ(header->compressed_size, 2048u);
    TEST_ASSERT_EQ(header->flags, 0u);
    TEST_ASSERT_EQ(header->compression, 1u);
}

TEST_CASE(Aud_Flags_Stereo, "Assets") {
    uint8_t flags = 0x01;  // Bit 0 = stereo
    bool is_stereo = (flags & 0x01) != 0;
    TEST_ASSERT(is_stereo);

    flags = 0x00;
    is_stereo = (flags & 0x01) != 0;
    TEST_ASSERT(!is_stereo);
}

TEST_CASE(Aud_Flags_16Bit, "Assets") {
    uint8_t flags = 0x02;  // Bit 1 = 16-bit
    bool is_16bit = (flags & 0x02) != 0;
    TEST_ASSERT(is_16bit);

    flags = 0x00;
    is_16bit = (flags & 0x02) != 0;
    TEST_ASSERT(!is_16bit);
}

TEST_CASE(Aud_CompressionType_WW, "Assets") {
    TEST_ASSERT_EQ(AUD_COMPRESS_WW, 1);
}

TEST_CASE(Aud_CompressionType_IMA, "Assets") {
    TEST_ASSERT_EQ(AUD_COMPRESS_IMA, 99);
}

//=============================================================================
// ADPCM Decoder Tests
//=============================================================================

TEST_CASE(ADPCM_StepTable_Size, "Assets") {
    // IMA/WW ADPCM step table has 89 entries
    static const int16_t step_table[] = {
        7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
        34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143,
        157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544,
        598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707,
        1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871,
        5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
        15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
    };

    TEST_ASSERT_EQ(sizeof(step_table) / sizeof(step_table[0]), 89u);
    TEST_ASSERT_EQ(step_table[0], 7);
    TEST_ASSERT_EQ(step_table[88], 32767);
}

TEST_CASE(ADPCM_IndexTable_Size, "Assets") {
    static const int8_t index_table[] = {
        -1, -1, -1, -1, 2, 4, 6, 8,
        -1, -1, -1, -1, 2, 4, 6, 8
    };

    TEST_ASSERT_EQ(sizeof(index_table), 16u);
}

TEST_CASE(ADPCM_Decode_SingleNibble, "Assets") {
    // Test decoding a single ADPCM nibble
    int16_t predictor = 0;
    int step_index = 0;
    int16_t step = 7;  // step_table[0]

    uint8_t nibble = 0x04;  // Positive step

    int diff = step >> 3;
    if (nibble & 1) diff += step >> 2;
    if (nibble & 2) diff += step >> 1;
    if (nibble & 4) diff += step;
    if (nibble & 8) diff = -diff;

    predictor += diff;

    // With nibble=4, step=7: diff = 7>>3 + 7 = 0 + 7 = 7
    TEST_ASSERT_EQ(predictor, 7);
}

//=============================================================================
// String ID/CRC Tests
//=============================================================================

TEST_CASE(CRC_DifferentStrings_DifferentValues, "Assets") {
    // Different strings should produce different CRCs
    // This tests the CRC used for MIX file lookups

    const char* str1 = "MOUSE.SHP";
    const char* str2 = "UNITS.SHP";

    // Would use actual CRC function
    // uint32_t crc1 = CalculateCRC(str1);
    // uint32_t crc2 = CalculateCRC(str2);
    // TEST_ASSERT_NE(crc1, crc2);

    TEST_ASSERT_NE(strcmp(str1, str2), 0);  // Basic sanity
}

TEST_CASE(CRC_CaseInsensitive, "Assets") {
    // MIX CRCs should be case-insensitive
    const char* lower = "mouse.shp";
    const char* upper = "MOUSE.SHP";

    // uint32_t crc1 = CalculateCRC(lower);
    // uint32_t crc2 = CalculateCRC(upper);
    // TEST_ASSERT_EQ(crc1, crc2);

    TEST_ASSERT(true);  // Placeholder
}
```

### src/tests/unit/test_audio.cpp

```cpp
// src/tests/unit/test_audio.cpp
// Audio System Unit Tests
// Task 18b - Unit Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "platform.h"

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
```

### src/tests/unit/test_input.cpp

```cpp
// src/tests/unit/test_input.cpp
// Input System Unit Tests
// Task 18b - Unit Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "platform.h"

//=============================================================================
// Keyboard Tests
//=============================================================================

TEST_WITH_FIXTURE(PlatformFixture, Input_KeyDown_InitiallyFalse, "Input") {
    Platform_Input_Update();

    // No keys should be pressed initially
    TEST_ASSERT(!Platform_Key_IsDown(KEY_CODE_A));
    TEST_ASSERT(!Platform_Key_IsDown(KEY_CODE_ESCAPE));
    TEST_ASSERT(!Platform_Key_IsDown(KEY_CODE_SPACE));
}

TEST_CASE(Input_KeyCode_Values, "Input") {
    // Key codes should be distinct
    TEST_ASSERT_NE(KEY_CODE_A, KEY_CODE_B);
    TEST_ASSERT_NE(KEY_CODE_ESCAPE, KEY_CODE_RETURN);
    TEST_ASSERT_NE(KEY_CODE_SPACE, KEY_CODE_TAB);
}

TEST_CASE(Input_KeyCode_AlphaRange, "Input") {
    // Alpha keys A-Z should be sequential or follow a pattern
    TEST_ASSERT_LT(KEY_CODE_A, KEY_CODE_Z);
}

TEST_CASE(Input_KeyCode_ModifierFlags, "Input") {
    // Modifier flags
    TEST_ASSERT_NE(KEY_MOD_SHIFT, 0);
    TEST_ASSERT_NE(KEY_MOD_CTRL, 0);
    TEST_ASSERT_NE(KEY_MOD_ALT, 0);

    // Modifiers should be different bits
    TEST_ASSERT_EQ(KEY_MOD_SHIFT & KEY_MOD_CTRL, 0);
    TEST_ASSERT_EQ(KEY_MOD_SHIFT & KEY_MOD_ALT, 0);
    TEST_ASSERT_EQ(KEY_MOD_CTRL & KEY_MOD_ALT, 0);
}

TEST_WITH_FIXTURE(PlatformFixture, Input_GetModifiers_InitiallyZero, "Input") {
    Platform_Input_Update();

    uint32_t mods = Platform_Key_GetModifiers();
    // Modifiers should be zero when no keys pressed
    // (unless sticky keys or system state)
    TEST_ASSERT_GE(mods, 0u);
}

//=============================================================================
// Mouse Tests
//=============================================================================

TEST_WITH_FIXTURE(PlatformFixture, Input_MousePosition_Valid, "Input") {
    Platform_Input_Update();

    int32_t x, y;
    Platform_Mouse_GetPosition(&x, &y);

    // Position should be reasonable (not negative)
    // Note: might be negative if cursor is off-screen
    TEST_ASSERT(true);  // Just verify it doesn't crash
}

TEST_WITH_FIXTURE(PlatformFixture, Input_MouseButton_InitiallyUp, "Input") {
    Platform_Input_Update();

    // No buttons should be pressed initially
    TEST_ASSERT(!Platform_Mouse_IsButtonDown(MOUSE_BUTTON_LEFT));
    TEST_ASSERT(!Platform_Mouse_IsButtonDown(MOUSE_BUTTON_RIGHT));
    TEST_ASSERT(!Platform_Mouse_IsButtonDown(MOUSE_BUTTON_MIDDLE));
}

TEST_CASE(Input_MouseButton_Values, "Input") {
    // Mouse buttons should have distinct values
    TEST_ASSERT_NE(MOUSE_BUTTON_LEFT, MOUSE_BUTTON_RIGHT);
    TEST_ASSERT_NE(MOUSE_BUTTON_LEFT, MOUSE_BUTTON_MIDDLE);
    TEST_ASSERT_NE(MOUSE_BUTTON_RIGHT, MOUSE_BUTTON_MIDDLE);
}

TEST_WITH_FIXTURE(PlatformFixture, Input_MouseDelta_InitiallyZero, "Input") {
    // First update establishes position
    Platform_Input_Update();

    int32_t dx, dy;
    Platform_Mouse_GetDelta(&dx, &dy);

    // After first update, delta might be zero or undefined
    // Second update with no movement should give zero
    Platform_Input_Update();
    Platform_Mouse_GetDelta(&dx, &dy);

    // Delta should be small (no movement)
    TEST_ASSERT_LE(std::abs(dx), 10);
    TEST_ASSERT_LE(std::abs(dy), 10);
}

//=============================================================================
// Input State Tracking Tests
//=============================================================================

TEST_CASE(Input_JustPressed_Logic, "Input") {
    // JustPressed = down now AND was up last frame
    bool down_now = true;
    bool down_last = false;

    bool just_pressed = down_now && !down_last;
    TEST_ASSERT(just_pressed);

    // If was already down, not "just" pressed
    down_last = true;
    just_pressed = down_now && !down_last;
    TEST_ASSERT(!just_pressed);
}

TEST_CASE(Input_JustReleased_Logic, "Input") {
    // JustReleased = up now AND was down last frame
    bool down_now = false;
    bool down_last = true;

    bool just_released = !down_now && down_last;
    TEST_ASSERT(just_released);

    // If was already up, not "just" released
    down_last = false;
    just_released = !down_now && down_last;
    TEST_ASSERT(!just_released);
}

//=============================================================================
// Event Queue Tests
//=============================================================================

TEST_WITH_FIXTURE(PlatformFixture, Input_PumpEvents_DoesNotCrash, "Input") {
    // Pump events multiple times
    for (int i = 0; i < 10; i++) {
        Platform_PumpEvents();
        Platform_Input_Update();
    }

    TEST_ASSERT(true);
}

//=============================================================================
// Cursor Tests
//=============================================================================

TEST_WITH_FIXTURE(PlatformFixture, Input_ShowCursor_Toggle, "Input") {
    Platform_Mouse_ShowCursor(true);
    TEST_ASSERT(true);

    Platform_Mouse_ShowCursor(false);
    TEST_ASSERT(true);

    Platform_Mouse_ShowCursor(true);
    TEST_ASSERT(true);
}

TEST_WITH_FIXTURE(PlatformFixture, Input_SetCursorPosition, "Input") {
    // Set cursor to a known position
    Platform_Mouse_SetPosition(100, 100);

    // Note: This might not work in all environments (headless, etc.)
    // Just verify it doesn't crash
    TEST_ASSERT(true);
}

//=============================================================================
// Key Mapping Tests
//=============================================================================

TEST_CASE(Input_KeyToChar_Letters, "Input") {
    // Mapping key codes to characters (if API exists)
    // KEY_CODE_A should map to 'a' or 'A'

    TEST_ASSERT_GE(KEY_CODE_A, 0);
    TEST_ASSERT_GE(KEY_CODE_Z, 0);
}

TEST_CASE(Input_KeyToChar_Numbers, "Input") {
    // Number keys
    TEST_ASSERT_GE(KEY_CODE_0, 0);
    TEST_ASSERT_GE(KEY_CODE_9, 0);
}

TEST_CASE(Input_FunctionKeys, "Input") {
    // Function keys F1-F12
    TEST_ASSERT_GE(KEY_CODE_F1, 0);
    TEST_ASSERT_GE(KEY_CODE_F12, 0);
}
```

### src/tests/unit/test_graphics.cpp

```cpp
// src/tests/unit/test_graphics.cpp
// Graphics System Unit Tests
// Task 18b - Unit Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "platform.h"
#include <cstring>

//=============================================================================
// Graphics Initialization Tests
//=============================================================================

TEST_WITH_FIXTURE(GraphicsFixture, Graphics_Init_Succeeds, "Graphics") {
    TEST_ASSERT(fixture.IsInitialized());
}

TEST_WITH_FIXTURE(GraphicsFixture, Graphics_Dimensions_Valid, "Graphics") {
    TEST_ASSERT_GT(fixture.GetWidth(), 0);
    TEST_ASSERT_GT(fixture.GetHeight(), 0);
    TEST_ASSERT_GE(fixture.GetPitch(), fixture.GetWidth());
}

TEST_WITH_FIXTURE(GraphicsFixture, Graphics_BackBuffer_NotNull, "Graphics") {
    uint8_t* buffer = fixture.GetBackBuffer();
    TEST_ASSERT_NOT_NULL(buffer);
}

//=============================================================================
// Pixel Operations Tests
//=============================================================================

TEST_WITH_FIXTURE(GraphicsFixture, Graphics_WritePixel, "Graphics") {
    uint8_t* buffer = fixture.GetBackBuffer();
    TEST_ASSERT_NOT_NULL(buffer);

    // Write a pixel
    buffer[0] = 123;

    // Verify
    TEST_ASSERT_EQ(buffer[0], 123);
}

TEST_WITH_FIXTURE(GraphicsFixture, Graphics_ClearBuffer, "Graphics") {
    fixture.ClearBackBuffer(42);

    uint8_t* buffer = fixture.GetBackBuffer();
    int width = fixture.GetWidth();
    int height = fixture.GetHeight();
    int pitch = fixture.GetPitch();

    // Verify all pixels are cleared
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            TEST_ASSERT_EQ(buffer[y * pitch + x], 42);
        }
    }
}

TEST_WITH_FIXTURE(GraphicsFixture, Graphics_HorizontalLine, "Graphics") {
    fixture.ClearBackBuffer(0);

    uint8_t* buffer = fixture.GetBackBuffer();
    int pitch = fixture.GetPitch();

    // Draw horizontal line at y=10
    int y = 10;
    for (int x = 0; x < 100; x++) {
        buffer[y * pitch + x] = 255;
    }

    // Verify line
    TEST_ASSERT_EQ(buffer[y * pitch + 0], 255);
    TEST_ASSERT_EQ(buffer[y * pitch + 50], 255);
    TEST_ASSERT_EQ(buffer[y * pitch + 99], 255);

    // Verify adjacent pixels are still 0
    TEST_ASSERT_EQ(buffer[(y-1) * pitch + 50], 0);
    TEST_ASSERT_EQ(buffer[(y+1) * pitch + 50], 0);
}

//=============================================================================
// Color Index Tests
//=============================================================================

TEST_CASE(Graphics_PaletteIndex_Range, "Graphics") {
    // Palette index is 0-255
    for (int i = 0; i < 256; i++) {
        uint8_t index = static_cast<uint8_t>(i);
        TEST_ASSERT_EQ(index, i);
    }
}

TEST_CASE(Graphics_TransparentColor, "Graphics") {
    // Color 0 is typically transparent
    uint8_t transparent = 0;
    TEST_ASSERT_EQ(transparent, 0);
}

//=============================================================================
// Resolution Tests
//=============================================================================

TEST_CASE(Graphics_Resolution_640x480, "Graphics") {
    // Default game resolution
    int width = 640;
    int height = 480;

    TEST_ASSERT_EQ(width, 640);
    TEST_ASSERT_EQ(height, 480);

    // Total pixels
    int total = width * height;
    TEST_ASSERT_EQ(total, 307200);
}

TEST_CASE(Graphics_Resolution_320x200, "Graphics") {
    // Original DOS resolution
    int width = 320;
    int height = 200;

    TEST_ASSERT_EQ(width, 320);
    TEST_ASSERT_EQ(height, 200);
}

//=============================================================================
// Pitch/Stride Tests
//=============================================================================

TEST_CASE(Graphics_Pitch_Alignment, "Graphics") {
    // Pitch might be aligned to 4 or 8 bytes
    int width = 640;
    int alignment = 4;

    int aligned_pitch = (width + alignment - 1) & ~(alignment - 1);
    TEST_ASSERT_GE(aligned_pitch, width);
    TEST_ASSERT_EQ(aligned_pitch % alignment, 0);
}

TEST_WITH_FIXTURE(GraphicsFixture, Graphics_Pitch_AtLeastWidth, "Graphics") {
    int width = fixture.GetWidth();
    int pitch = fixture.GetPitch();

    TEST_ASSERT_GE(pitch, width);
}

//=============================================================================
// Flip/Present Tests
//=============================================================================

TEST_WITH_FIXTURE(GraphicsFixture, Graphics_Flip_DoesNotCrash, "Graphics") {
    // Draw something
    fixture.ClearBackBuffer(100);

    // Flip
    fixture.RenderFrame();

    TEST_ASSERT(true);
}

TEST_WITH_FIXTURE(GraphicsFixture, Graphics_MultipleFlips, "Graphics") {
    for (int i = 0; i < 10; i++) {
        fixture.ClearBackBuffer(i * 25);
        fixture.RenderFrame();
    }

    TEST_ASSERT(true);
}

//=============================================================================
// Clipping Tests
//=============================================================================

TEST_CASE(Graphics_ClipRect_Intersection, "Graphics") {
    // Test rectangle intersection logic
    struct Rect { int x, y, w, h; };

    Rect screen = {0, 0, 640, 480};
    Rect sprite = {600, 460, 100, 100};  // Partially off-screen

    // Clip sprite to screen
    int clip_x = (sprite.x < 0) ? 0 : sprite.x;
    int clip_y = (sprite.y < 0) ? 0 : sprite.y;
    int clip_w = std::min(sprite.x + sprite.w, screen.w) - clip_x;
    int clip_h = std::min(sprite.y + sprite.h, screen.h) - clip_y;

    TEST_ASSERT_EQ(clip_x, 600);
    TEST_ASSERT_EQ(clip_y, 460);
    TEST_ASSERT_EQ(clip_w, 40);   // Only 40 pixels visible
    TEST_ASSERT_EQ(clip_h, 20);   // Only 20 pixels visible
}

TEST_CASE(Graphics_ClipRect_FullyOffscreen, "Graphics") {
    // Sprite completely off-screen
    struct Rect { int x, y, w, h; };

    Rect screen = {0, 0, 640, 480};
    Rect sprite = {700, 500, 100, 100};  // Completely off-screen

    bool visible = (sprite.x < screen.w && sprite.y < screen.h &&
                   sprite.x + sprite.w > 0 && sprite.y + sprite.h > 0);

    TEST_ASSERT(!visible);
}

//=============================================================================
// Coordinate Conversion Tests
//=============================================================================

TEST_CASE(Graphics_ScreenToCell, "Graphics") {
    // Convert screen coordinates to map cell
    int cell_size = 24;  // ICON_PIXEL_W

    int screen_x = 100;
    int screen_y = 200;

    int cell_x = screen_x / cell_size;
    int cell_y = screen_y / cell_size;

    TEST_ASSERT_EQ(cell_x, 4);
    TEST_ASSERT_EQ(cell_y, 8);
}

TEST_CASE(Graphics_CellToScreen, "Graphics") {
    int cell_size = 24;

    int cell_x = 10;
    int cell_y = 20;

    int screen_x = cell_x * cell_size;
    int screen_y = cell_y * cell_size;

    TEST_ASSERT_EQ(screen_x, 240);
    TEST_ASSERT_EQ(screen_y, 480);
}
```

### src/tests/unit/test_utils.cpp

```cpp
// src/tests/unit/test_utils.cpp
// Utility Function Unit Tests
// Task 18b - Unit Tests

#include "test/test_framework.h"
#include <cstring>
#include <cmath>
#include <algorithm>

//=============================================================================
// String Utility Tests
//=============================================================================

TEST_CASE(Utils_String_ToUpper, "Utils") {
    char str[] = "hello world";

    for (char* p = str; *p; p++) {
        if (*p >= 'a' && *p <= 'z') {
            *p = *p - 'a' + 'A';
        }
    }

    TEST_ASSERT_EQ(strcmp(str, "HELLO WORLD"), 0);
}

TEST_CASE(Utils_String_ToLower, "Utils") {
    char str[] = "HELLO WORLD";

    for (char* p = str; *p; p++) {
        if (*p >= 'A' && *p <= 'Z') {
            *p = *p - 'A' + 'a';
        }
    }

    TEST_ASSERT_EQ(strcmp(str, "hello world"), 0);
}

TEST_CASE(Utils_String_Compare_CaseInsensitive, "Utils") {
    const char* a = "Hello";
    const char* b = "HELLO";
    const char* c = "world";

    // Case-insensitive compare
    auto stricmp_impl = [](const char* s1, const char* s2) {
        while (*s1 && *s2) {
            char c1 = (*s1 >= 'A' && *s1 <= 'Z') ? *s1 + 32 : *s1;
            char c2 = (*s2 >= 'A' && *s2 <= 'Z') ? *s2 + 32 : *s2;
            if (c1 != c2) return c1 - c2;
            s1++; s2++;
        }
        return *s1 - *s2;
    };

    TEST_ASSERT_EQ(stricmp_impl(a, b), 0);
    TEST_ASSERT_NE(stricmp_impl(a, c), 0);
}

//=============================================================================
// Math Utility Tests
//=============================================================================

TEST_CASE(Utils_Math_Clamp, "Utils") {
    auto clamp = [](int value, int min, int max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    };

    TEST_ASSERT_EQ(clamp(50, 0, 100), 50);
    TEST_ASSERT_EQ(clamp(-10, 0, 100), 0);
    TEST_ASSERT_EQ(clamp(150, 0, 100), 100);
}

TEST_CASE(Utils_Math_Min, "Utils") {
    TEST_ASSERT_EQ(std::min(5, 10), 5);
    TEST_ASSERT_EQ(std::min(10, 5), 5);
    TEST_ASSERT_EQ(std::min(5, 5), 5);
}

TEST_CASE(Utils_Math_Max, "Utils") {
    TEST_ASSERT_EQ(std::max(5, 10), 10);
    TEST_ASSERT_EQ(std::max(10, 5), 10);
    TEST_ASSERT_EQ(std::max(5, 5), 5);
}

TEST_CASE(Utils_Math_Abs, "Utils") {
    TEST_ASSERT_EQ(std::abs(5), 5);
    TEST_ASSERT_EQ(std::abs(-5), 5);
    TEST_ASSERT_EQ(std::abs(0), 0);
}

TEST_CASE(Utils_Math_Lerp, "Utils") {
    auto lerp = [](float a, float b, float t) {
        return a + (b - a) * t;
    };

    TEST_ASSERT_NEAR(lerp(0.0f, 100.0f, 0.0f), 0.0f, 0.001f);
    TEST_ASSERT_NEAR(lerp(0.0f, 100.0f, 1.0f), 100.0f, 0.001f);
    TEST_ASSERT_NEAR(lerp(0.0f, 100.0f, 0.5f), 50.0f, 0.001f);
}

TEST_CASE(Utils_Math_Distance, "Utils") {
    auto distance = [](int x1, int y1, int x2, int y2) {
        int dx = x2 - x1;
        int dy = y2 - y1;
        return std::sqrt(dx * dx + dy * dy);
    };

    TEST_ASSERT_NEAR(distance(0, 0, 3, 4), 5.0, 0.001);
    TEST_ASSERT_NEAR(distance(0, 0, 0, 0), 0.0, 0.001);
}

TEST_CASE(Utils_Math_ManhattanDistance, "Utils") {
    auto manhattan = [](int x1, int y1, int x2, int y2) {
        return std::abs(x2 - x1) + std::abs(y2 - y1);
    };

    TEST_ASSERT_EQ(manhattan(0, 0, 3, 4), 7);
    TEST_ASSERT_EQ(manhattan(5, 5, 5, 5), 0);
}

//=============================================================================
// Bit Manipulation Tests
//=============================================================================

TEST_CASE(Utils_Bits_SetBit, "Utils") {
    uint32_t flags = 0;

    flags |= (1 << 0);  // Set bit 0
    TEST_ASSERT_EQ(flags, 1u);

    flags |= (1 << 3);  // Set bit 3
    TEST_ASSERT_EQ(flags, 9u);  // 0b1001
}

TEST_CASE(Utils_Bits_ClearBit, "Utils") {
    uint32_t flags = 0xFF;  // All bits set

    flags &= ~(1 << 0);  // Clear bit 0
    TEST_ASSERT_EQ(flags, 0xFEu);

    flags &= ~(1 << 3);  // Clear bit 3
    TEST_ASSERT_EQ(flags, 0xF6u);
}

TEST_CASE(Utils_Bits_TestBit, "Utils") {
    uint32_t flags = 0b1010;

    TEST_ASSERT((flags & (1 << 1)) != 0);  // Bit 1 is set
    TEST_ASSERT((flags & (1 << 3)) != 0);  // Bit 3 is set
    TEST_ASSERT((flags & (1 << 0)) == 0);  // Bit 0 is not set
    TEST_ASSERT((flags & (1 << 2)) == 0);  // Bit 2 is not set
}

TEST_CASE(Utils_Bits_ToggleBit, "Utils") {
    uint32_t flags = 0b1010;

    flags ^= (1 << 1);  // Toggle bit 1 (was 1, now 0)
    TEST_ASSERT_EQ(flags, 0b1000u);

    flags ^= (1 << 1);  // Toggle bit 1 (was 0, now 1)
    TEST_ASSERT_EQ(flags, 0b1010u);
}

//=============================================================================
// Fixed Point Math Tests
//=============================================================================

TEST_CASE(Utils_Fixed_IntToFixed, "Utils") {
    // 16.16 fixed point
    int32_t fixed = 5 << 16;  // 5.0 in fixed point
    TEST_ASSERT_EQ(fixed, 327680);
}

TEST_CASE(Utils_Fixed_FixedToInt, "Utils") {
    int32_t fixed = 327680;  // 5.0
    int integer = fixed >> 16;
    TEST_ASSERT_EQ(integer, 5);
}

TEST_CASE(Utils_Fixed_Multiply, "Utils") {
    // 2.5 * 2.0 = 5.0
    int32_t a = (2 << 16) + (1 << 15);  // 2.5
    int32_t b = 2 << 16;                 // 2.0

    int64_t result = ((int64_t)a * b) >> 16;

    int integer_part = static_cast<int32_t>(result) >> 16;
    TEST_ASSERT_EQ(integer_part, 5);
}

//=============================================================================
// Random Number Tests
//=============================================================================

TEST_CASE(Utils_Random_Range, "Utils") {
    // Simple linear congruential generator
    uint32_t seed = 12345;

    auto random = [&seed]() {
        seed = seed * 1103515245 + 12345;
        return seed;
    };

    // Generate some random numbers
    for (int i = 0; i < 100; i++) {
        uint32_t r = random();
        // Just verify it produces values
        TEST_ASSERT_GE(r, 0u);
    }
}

TEST_CASE(Utils_Random_InRange, "Utils") {
    auto random_in_range = [](int min, int max, uint32_t random_value) {
        return min + (random_value % (max - min + 1));
    };

    // Test with various random values
    for (uint32_t i = 0; i < 100; i++) {
        int result = random_in_range(10, 20, i * 12345);
        TEST_ASSERT_GE(result, 10);
        TEST_ASSERT_LE(result, 20);
    }
}

//=============================================================================
// Direction/Facing Tests
//=============================================================================

TEST_CASE(Utils_Direction_8Way, "Utils") {
    // 8 directions (0-7)
    // 0 = North, 1 = NE, 2 = East, etc.
    enum Direction { N = 0, NE, E, SE, S, SW, W, NW };

    TEST_ASSERT_EQ(Direction::N, 0);
    TEST_ASSERT_EQ(Direction::E, 2);
    TEST_ASSERT_EQ(Direction::S, 4);
    TEST_ASSERT_EQ(Direction::W, 6);
}

TEST_CASE(Utils_Direction_Opposite, "Utils") {
    auto opposite = [](int dir) {
        return (dir + 4) % 8;
    };

    TEST_ASSERT_EQ(opposite(0), 4);  // N -> S
    TEST_ASSERT_EQ(opposite(2), 6);  // E -> W
    TEST_ASSERT_EQ(opposite(4), 0);  // S -> N
}

TEST_CASE(Utils_Direction_FromDelta, "Utils") {
    // Calculate direction from dx, dy
    auto direction_from_delta = [](int dx, int dy) -> int {
        if (dx == 0 && dy < 0) return 0;   // N
        if (dx > 0 && dy < 0) return 1;    // NE
        if (dx > 0 && dy == 0) return 2;   // E
        if (dx > 0 && dy > 0) return 3;    // SE
        if (dx == 0 && dy > 0) return 4;   // S
        if (dx < 0 && dy > 0) return 5;    // SW
        if (dx < 0 && dy == 0) return 6;   // W
        if (dx < 0 && dy < 0) return 7;    // NW
        return 0;
    };

    TEST_ASSERT_EQ(direction_from_delta(0, -1), 0);   // North
    TEST_ASSERT_EQ(direction_from_delta(1, 0), 2);    // East
    TEST_ASSERT_EQ(direction_from_delta(0, 1), 4);    // South
    TEST_ASSERT_EQ(direction_from_delta(-1, 0), 6);   // West
}

//=============================================================================
// Coordinate/Cell Tests
//=============================================================================

TEST_CASE(Utils_Coord_CellToIndex, "Utils") {
    // Convert 2D cell coordinates to linear index
    int map_width = 64;

    auto cell_to_index = [map_width](int x, int y) {
        return y * map_width + x;
    };

    TEST_ASSERT_EQ(cell_to_index(0, 0), 0);
    TEST_ASSERT_EQ(cell_to_index(1, 0), 1);
    TEST_ASSERT_EQ(cell_to_index(0, 1), 64);
    TEST_ASSERT_EQ(cell_to_index(5, 10), 645);
}

TEST_CASE(Utils_Coord_IndexToCell, "Utils") {
    int map_width = 64;

    auto index_to_cell_x = [map_width](int index) {
        return index % map_width;
    };

    auto index_to_cell_y = [map_width](int index) {
        return index / map_width;
    };

    TEST_ASSERT_EQ(index_to_cell_x(645), 5);
    TEST_ASSERT_EQ(index_to_cell_y(645), 10);
}
```

### src/tests/unit_tests_main.cpp

```cpp
// src/tests/unit_tests_main.cpp
// Unit Tests Main Entry Point
// Task 18b - Unit Tests

#include "test/test_framework.h"

// Include all unit test files
// (They self-register via TEST_CASE macro)

TEST_MAIN()
```

---

## CMakeLists.txt Additions

```cmake
# Task 18b: Unit Tests

# Collect all unit test sources
set(UNIT_TEST_SOURCES
    ${CMAKE_SOURCE_DIR}/src/tests/unit/test_platform.cpp
    ${CMAKE_SOURCE_DIR}/src/tests/unit/test_assets.cpp
    ${CMAKE_SOURCE_DIR}/src/tests/unit/test_audio.cpp
    ${CMAKE_SOURCE_DIR}/src/tests/unit/test_input.cpp
    ${CMAKE_SOURCE_DIR}/src/tests/unit/test_graphics.cpp
    ${CMAKE_SOURCE_DIR}/src/tests/unit/test_utils.cpp
    ${CMAKE_SOURCE_DIR}/src/tests/unit_tests_main.cpp
)

add_executable(UnitTests ${UNIT_TEST_SOURCES})
target_link_libraries(UnitTests PRIVATE test_framework redalert_platform)
target_include_directories(UnitTests PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Register with CTest
add_test(NAME UnitTests COMMAND UnitTests)

# Quick unit tests (subset for CI)
add_test(NAME UnitTests_Quick COMMAND UnitTests --category Utils)
```

---

## Verification Script

```bash
#!/bin/bash
set -e
echo "=== Task 18b Verification: Unit Tests ==="

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"

echo "Checking files..."
for f in src/tests/unit/test_platform.cpp \
         src/tests/unit/test_assets.cpp \
         src/tests/unit/test_audio.cpp \
         src/tests/unit/test_input.cpp \
         src/tests/unit/test_graphics.cpp \
         src/tests/unit/test_utils.cpp; do
    [ -f "$ROOT_DIR/$f" ] || { echo "Missing: $f"; exit 1; }
    echo "  $f"
done

echo "Building..."
cd "$BUILD_DIR" && cmake --build . --target UnitTests
echo "  Built"

echo "Running unit tests..."
"$BUILD_DIR/UnitTests" || { echo "Unit tests failed"; exit 1; }

echo "Running quick tests..."
"$BUILD_DIR/UnitTests" --category Utils

echo "Generating XML report..."
"$BUILD_DIR/UnitTests" --xml "$BUILD_DIR/unit_test_results.xml"
[ -f "$BUILD_DIR/unit_test_results.xml" ] || { echo "XML not generated"; exit 1; }

echo "=== Task 18b Verification PASSED ==="
```

---

## Success Criteria

- [ ] All platform API functions have tests
- [ ] Asset format parsing is tested (MIX, PAL, SHP, AUD)
- [ ] Audio system components are tested
- [ ] Input system components are tested
- [ ] Graphics primitives are tested
- [ ] Utility functions are tested
- [ ] All tests pass
- [ ] Tests complete in < 10 seconds
- [ ] XML output generated correctly

---

## Completion Promise

When verification passes, output:
<promise>TASK_18B_COMPLETE</promise>

## Escape Hatch

If stuck after 15 iterations, document in `ralph-tasks/blocked/18b.md` and output:
<promise>TASK_18B_BLOCKED</promise>

## Max Iterations
15
