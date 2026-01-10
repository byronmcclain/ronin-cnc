// src/test_aud_loader.cpp
// Unit tests for AUD file loader
// Task 17a - AUD File Format Parser

#include "game/audio/aud_file.h"
#include "platform.h"
#include <cstdio>
#include <cstring>

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
// Mock AUD Data for Testing
//=============================================================================

static std::vector<uint8_t> CreateMockUncompressedAud(
    uint16_t sample_rate,
    bool stereo,
    bool is_16bit,
    const std::vector<int16_t>& samples)
{
    std::vector<uint8_t> data;

    size_t sample_bytes = is_16bit ? samples.size() * 2 : samples.size();

    AudHeader header;
    header.sample_rate = sample_rate;
    header.uncompressed_size = static_cast<uint32_t>(sample_bytes);
    header.compressed_size = static_cast<uint32_t>(sample_bytes);
    header.flags = (stereo ? AUD_FLAG_STEREO : 0) | (is_16bit ? AUD_FLAG_16BIT : 0);
    header.compression = AUD_COMPRESS_NONE;

    data.resize(sizeof(AudHeader) + sample_bytes);
    memcpy(data.data(), &header, sizeof(AudHeader));

    if (is_16bit) {
        memcpy(data.data() + sizeof(AudHeader), samples.data(), sample_bytes);
    } else {
        for (size_t i = 0; i < samples.size(); i++) {
            int16_t s = samples[i];
            uint8_t u8 = static_cast<uint8_t>((s / 256) + 128);
            data[sizeof(AudHeader) + i] = u8;
        }
    }

    return data;
}

static std::vector<uint8_t> CreateMockImaAdpcmAud(uint16_t sample_rate, size_t num_bytes) {
    std::vector<uint8_t> data;

    AudHeader header;
    header.sample_rate = sample_rate;
    header.uncompressed_size = static_cast<uint32_t>(num_bytes * 4);
    header.compressed_size = static_cast<uint32_t>(num_bytes);
    header.flags = 0;
    header.compression = AUD_COMPRESS_IMA;

    data.resize(sizeof(AudHeader) + num_bytes);
    memcpy(data.data(), &header, sizeof(AudHeader));

    for (size_t i = 0; i < num_bytes; i++) {
        data[sizeof(AudHeader) + i] = static_cast<uint8_t>((i % 2) ? 0x77 : 0x11);
    }

    return data;
}

static std::vector<uint8_t> CreateMockWwAdpcmAud(uint16_t sample_rate) {
    // Create a single WW ADPCM chunk (16 bytes)
    const size_t CHUNK_SIZE = 16;

    AudHeader header;
    header.sample_rate = sample_rate;
    header.uncompressed_size = 50;  // Approximate
    header.compressed_size = CHUNK_SIZE;
    header.flags = 0;  // Mono
    header.compression = AUD_COMPRESS_WW;

    std::vector<uint8_t> data(sizeof(AudHeader) + CHUNK_SIZE);
    memcpy(data.data(), &header, sizeof(AudHeader));

    // Chunk: predictor (2), step_index (1), reserved (1), nibbles (12)
    uint8_t* chunk = data.data() + sizeof(AudHeader);
    int16_t initial_pred = 0;
    memcpy(chunk, &initial_pred, 2);  // Initial predictor = 0
    chunk[2] = 20;  // Initial step index
    chunk[3] = 0;   // Reserved

    // Fill nibbles with simple pattern
    for (int i = 0; i < 12; i++) {
        chunk[4 + i] = 0x12;  // Low nibble = 2, high nibble = 1
    }

    return data;
}

//=============================================================================
// Unit Tests
//=============================================================================

bool Test_HeaderSize() {
    TEST_ASSERT(sizeof(AudHeader) == 12, "AudHeader must be 12 bytes");
    return true;
}

bool Test_LoadUncompressed16Bit() {
    std::vector<int16_t> samples = {0, 16000, 32000, 16000, 0, -16000, -32000, -16000};
    auto aud_data = CreateMockUncompressedAud(22050, false, true, samples);

    AudFile aud;
    TEST_ASSERT(aud.LoadFromData(aud_data.data(), aud_data.size()),
                "Failed to load uncompressed 16-bit AUD");

    TEST_ASSERT(aud.IsLoaded(), "File should be marked as loaded");
    TEST_ASSERT(aud.GetSampleRate() == 22050, "Wrong sample rate");
    TEST_ASSERT(aud.GetChannels() == 1, "Should be mono");
    TEST_ASSERT(aud.Is16Bit(), "Should be 16-bit");
    TEST_ASSERT(aud.GetCompressionType() == AUD_COMPRESS_NONE, "Wrong compression type");
    TEST_ASSERT(aud.GetPCMSampleCount() == samples.size(), "Wrong sample count");

    const int16_t* pcm = aud.GetPCMData();
    for (size_t i = 0; i < samples.size(); i++) {
        TEST_ASSERT(pcm[i] == samples[i], "Sample mismatch");
    }

    return true;
}

bool Test_LoadUncompressed8Bit() {
    std::vector<int16_t> samples = {0, 8192, 16384, 8192, 0, -8192, -16384, -8192};
    auto aud_data = CreateMockUncompressedAud(11025, false, false, samples);

    AudFile aud;
    TEST_ASSERT(aud.LoadFromData(aud_data.data(), aud_data.size()),
                "Failed to load uncompressed 8-bit AUD");

    TEST_ASSERT(aud.GetSampleRate() == 11025, "Wrong sample rate");
    TEST_ASSERT(!aud.Is16Bit(), "Should be marked as 8-bit source");
    TEST_ASSERT(aud.GetPCMSampleCount() == samples.size(), "Wrong sample count");

    return true;
}

bool Test_LoadStereo() {
    std::vector<int16_t> samples = {1000, -1000, 2000, -2000, 3000, -3000, 4000, -4000};
    auto aud_data = CreateMockUncompressedAud(44100, true, true, samples);

    AudFile aud;
    TEST_ASSERT(aud.LoadFromData(aud_data.data(), aud_data.size()),
                "Failed to load stereo AUD");

    TEST_ASSERT(aud.IsStereo(), "Should be stereo");
    TEST_ASSERT(aud.GetChannels() == 2, "Should have 2 channels");

    return true;
}

bool Test_LoadImaAdpcm() {
    auto aud_data = CreateMockImaAdpcmAud(22050, 100);

    AudFile aud;
    TEST_ASSERT(aud.LoadFromData(aud_data.data(), aud_data.size()),
                "Failed to load IMA ADPCM AUD");

    TEST_ASSERT(aud.GetCompressionType() == AUD_COMPRESS_IMA, "Wrong compression type");
    TEST_ASSERT(aud.GetPCMSampleCount() > 0, "Should have decoded samples");
    TEST_ASSERT(aud.GetPCMSampleCount() == 200, "IMA ADPCM: 100 bytes = 200 samples");

    return true;
}

bool Test_LoadWwAdpcm() {
    auto aud_data = CreateMockWwAdpcmAud(22050);

    AudFile aud;
    TEST_ASSERT(aud.LoadFromData(aud_data.data(), aud_data.size()),
                "Failed to load WW ADPCM AUD");

    TEST_ASSERT(aud.GetCompressionType() == AUD_COMPRESS_WW, "Wrong compression type");
    TEST_ASSERT(aud.GetPCMSampleCount() > 0, "Should have decoded samples");
    // 16-byte chunk = 1 initial + 24 decoded = 25 samples
    TEST_ASSERT(aud.GetPCMSampleCount() == 25, "WW ADPCM: 16-byte chunk = 25 samples");

    return true;
}

bool Test_Duration() {
    // 22050 samples at 22050 Hz = 1 second
    std::vector<int16_t> samples(22050, 0);
    auto aud_data = CreateMockUncompressedAud(22050, false, true, samples);

    AudFile aud;
    TEST_ASSERT(aud.LoadFromData(aud_data.data(), aud_data.size()), "Load failed");

    uint32_t duration_ms = aud.GetDurationMs();
    TEST_ASSERT(duration_ms >= 990 && duration_ms <= 1010, "Duration should be ~1000ms");

    float duration_s = aud.GetDurationSeconds();
    TEST_ASSERT(duration_s >= 0.99f && duration_s <= 1.01f, "Duration should be ~1.0s");

    return true;
}

bool Test_Clear() {
    std::vector<int16_t> samples = {100, 200, 300};
    auto aud_data = CreateMockUncompressedAud(22050, false, true, samples);

    AudFile aud;
    aud.LoadFromData(aud_data.data(), aud_data.size());
    TEST_ASSERT(aud.IsLoaded(), "Should be loaded");

    aud.Clear();
    TEST_ASSERT(!aud.IsLoaded(), "Should not be loaded after Clear");
    TEST_ASSERT(aud.GetPCMSampleCount() == 0, "Samples should be cleared");

    return true;
}

bool Test_MoveSemantics() {
    std::vector<int16_t> samples = {1, 2, 3, 4, 5};
    auto aud_data = CreateMockUncompressedAud(22050, false, true, samples);

    AudFile aud1;
    aud1.LoadFromData(aud_data.data(), aud_data.size());

    // Move constructor
    AudFile aud2(std::move(aud1));
    TEST_ASSERT(!aud1.IsLoaded(), "Source should be empty after move");
    TEST_ASSERT(aud2.IsLoaded(), "Destination should be loaded");
    TEST_ASSERT(aud2.GetPCMSampleCount() == 5, "Samples should be moved");

    // Move assignment
    AudFile aud3;
    aud3 = std::move(aud2);
    TEST_ASSERT(!aud2.IsLoaded(), "Source should be empty after move assignment");
    TEST_ASSERT(aud3.IsLoaded(), "Destination should be loaded");

    return true;
}

bool Test_InvalidData() {
    AudFile aud;

    // Null data
    TEST_ASSERT(!aud.LoadFromData(nullptr, 100), "Should fail on null data");

    // Too small
    uint8_t small[8] = {0};
    TEST_ASSERT(!aud.LoadFromData(small, sizeof(small)), "Should fail on small data");

    // Invalid sample rate
    std::vector<uint8_t> bad_data(sizeof(AudHeader) + 100, 0);
    AudHeader* bad_header = reinterpret_cast<AudHeader*>(bad_data.data());
    bad_header->sample_rate = 0;
    bad_header->compressed_size = 100;
    TEST_ASSERT(!aud.LoadFromData(bad_data.data(), bad_data.size()),
                "Should fail on zero sample rate");

    return true;
}

bool Test_CompressionName() {
    AudFile aud;

    // Load uncompressed
    std::vector<int16_t> samples = {0};
    auto aud_data = CreateMockUncompressedAud(22050, false, true, samples);
    aud.LoadFromData(aud_data.data(), aud_data.size());

    TEST_ASSERT(strcmp(aud.GetCompressionName(), "Uncompressed") == 0,
                "Wrong compression name");

    // Load IMA
    auto ima_data = CreateMockImaAdpcmAud(22050, 10);
    aud.LoadFromData(ima_data.data(), ima_data.size());

    TEST_ASSERT(strcmp(aud.GetCompressionName(), "IMA ADPCM") == 0,
                "Wrong compression name for IMA");

    // Load WW
    auto ww_data = CreateMockWwAdpcmAud(22050);
    aud.LoadFromData(ww_data.data(), ww_data.size());

    TEST_ASSERT(strcmp(aud.GetCompressionName(), "WW ADPCM") == 0,
                "Wrong compression name for WW");

    return true;
}

bool Test_GetAudInfo() {
    std::vector<int16_t> samples = {100, 200};
    auto aud_data = CreateMockUncompressedAud(44100, true, true, samples);

    AudHeader header;
    TEST_ASSERT(GetAudInfo(aud_data.data(), aud_data.size(), &header),
                "GetAudInfo should succeed");

    TEST_ASSERT(header.sample_rate == 44100, "Wrong sample rate from GetAudInfo");
    TEST_ASSERT((header.flags & AUD_FLAG_STEREO) != 0, "Should be stereo");

    // Without output header
    TEST_ASSERT(GetAudInfo(aud_data.data(), aud_data.size(), nullptr),
                "GetAudInfo should work without output");

    // Invalid data
    TEST_ASSERT(!GetAudInfo(nullptr, 100, nullptr), "Should fail on null");
    TEST_ASSERT(!GetAudInfo(aud_data.data(), 4, nullptr), "Should fail on small data");

    return true;
}

bool Test_LoadFromMix() {
    // This test requires actual game data
    printf("\n  (Integration test - requires game data)\n  ");

    // Check if any MIX files are loaded
    if (Platform_Mix_GetCount() == 0) {
        printf("SKIPPED - No MIX files loaded\n");
        return true;
    }

    // Try to load a common sound
    const char* test_files[] = {"CLICK.AUD", "XPLOS.AUD", "GUNSHOT.AUD"};

    AudFile aud;
    for (const char* filename : test_files) {
        if (aud.LoadFromMix(filename)) {
            printf("Loaded %s: %u Hz, %s, %.2fs... ",
                   filename,
                   aud.GetSampleRate(),
                   aud.GetCompressionName(),
                   aud.GetDurationSeconds());
            return true;
        }
    }

    printf("SKIPPED - No test files found\n");
    return true;
}

//=============================================================================
// Main
//=============================================================================

int main(int argc, char* argv[]) {
    printf("=== AUD File Loader Tests (Task 17a) ===\n\n");

    bool quick_mode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--quick") == 0 || strcmp(argv[i], "-q") == 0) {
            quick_mode = true;
        }
    }

    // Initialize platform for integration tests
    Platform_Init();

    // Run unit tests
    RUN_TEST(HeaderSize);
    RUN_TEST(LoadUncompressed16Bit);
    RUN_TEST(LoadUncompressed8Bit);
    RUN_TEST(LoadStereo);
    RUN_TEST(LoadImaAdpcm);
    RUN_TEST(LoadWwAdpcm);
    RUN_TEST(Duration);
    RUN_TEST(Clear);
    RUN_TEST(MoveSemantics);
    RUN_TEST(InvalidData);
    RUN_TEST(CompressionName);
    RUN_TEST(GetAudInfo);

    // Integration test (may skip if no game data)
    if (!quick_mode) {
        RUN_TEST(LoadFromMix);
    }

    Platform_Shutdown();

    printf("\n");
    if (tests_failed == 0) {
        printf("All tests PASSED (%d/%d)\n", tests_passed, tests_passed + tests_failed);
    } else {
        printf("Results: %d passed, %d failed\n", tests_passed, tests_failed);
    }

    return (tests_failed == 0) ? 0 : 1;
}
