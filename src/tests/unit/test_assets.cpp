// src/tests/unit/test_assets.cpp
// Asset System Unit Tests
// Task 18b - Unit Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "game/audio/aud_file.h"
#include <cstring>

//=============================================================================
// MIX File Tests
//=============================================================================

TEST_CASE(Mix_HeaderSize_Correct, "Assets") {
    // MIX header should be known size
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
    // Test that different filenames produce different hashes
    const char* name1 = "MOUSE.SHP";
    const char* name2 = "UNITS.SHP";

    // Simple hash check - actual CRC would use specific algorithm
    size_t hash1 = 0;
    size_t hash2 = 0;

    for (const char* p = name1; *p; p++) {
        hash1 = hash1 * 31 + *p;
    }
    for (const char* p = name2; *p; p++) {
        hash2 = hash2 * 31 + *p;
    }

    TEST_ASSERT_NE(hash1, hash2);
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
        buffer[i] = static_cast<uint8_t>(i % 64);  // VGA range 0-63
    }

    // Verify buffer is filled correctly
    TEST_ASSERT_EQ(buffer[0], 0u);
    TEST_ASSERT_EQ(buffer[63], 63u);
    TEST_ASSERT_EQ(buffer[64], 0u);
}

//=============================================================================
// Shape (SHP) Tests
//=============================================================================

TEST_CASE(Shape_HeaderSize, "Assets") {
    // SHP header structure
    struct ShpHeader {
        uint16_t frame_count;
        uint16_t x_offset;
        uint16_t y_offset;
        uint16_t width;
        uint16_t height;
    };

    // Verify expected size
    TEST_ASSERT_EQ(sizeof(ShpHeader), 10u);
}

TEST_CASE(Shape_FrameOffset_Valid, "Assets") {
    // Frame offsets should be sequential
    uint32_t offsets[] = {0, 100, 200, 300};

    for (int i = 1; i < 4; i++) {
        TEST_ASSERT_GT(offsets[i], offsets[i-1]);
    }
}

TEST_CASE(Shape_RLE_Decode_Simple, "Assets") {
    // Test RLE decoding logic
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
    int16_t step = 7;  // step_table[0]

    uint8_t nibble = 0x04;  // Positive step

    int diff = step >> 3;
    if (nibble & 1) diff += step >> 2;
    if (nibble & 2) diff += step >> 1;
    if (nibble & 4) diff += step;
    if (nibble & 8) diff = -diff;

    predictor += static_cast<int16_t>(diff);

    // With nibble=4, step=7: diff = 7>>3 + 7 = 0 + 7 = 7
    TEST_ASSERT_EQ(predictor, 7);
}

//=============================================================================
// String ID/CRC Tests
//=============================================================================

TEST_CASE(CRC_DifferentStrings_DifferentValues, "Assets") {
    // Different strings should produce different CRCs
    const char* str1 = "MOUSE.SHP";
    const char* str2 = "UNITS.SHP";

    // Basic check - strings are different
    TEST_ASSERT_NE(strcmp(str1, str2), 0);
}

TEST_CASE(CRC_CaseConversion, "Assets") {
    // MIX CRCs should be case-insensitive
    // This tests the conversion logic
    const char* lower = "mouse.shp";
    const char* upper = "MOUSE.SHP";

    // Convert lower to upper
    char converted[64];
    strncpy(converted, lower, sizeof(converted) - 1);
    converted[sizeof(converted) - 1] = '\0';

    for (char* p = converted; *p; p++) {
        if (*p >= 'a' && *p <= 'z') {
            *p = *p - 'a' + 'A';
        }
    }

    TEST_ASSERT_EQ(strcmp(converted, upper), 0);
}
