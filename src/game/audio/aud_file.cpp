// src/game/audio/aud_file.cpp
// Westwood AUD file format loader implementation
// Task 17a - AUD File Format Parser

#include "game/audio/aud_file.h"
#include "platform.h"
#include <cstring>
#include <algorithm>
#include <cstdio>

//=============================================================================
// WW ADPCM Tables
//=============================================================================

// Westwood ADPCM step size table (89 values)
static const int16_t WW_STEP_TABLE[89] = {
    7,     8,     9,    10,    11,    12,    13,    14,
    16,    17,    19,    21,    23,    25,    28,    31,
    34,    37,    41,    45,    50,    55,    60,    66,
    73,    80,    88,    97,   107,   118,   130,   143,
    157,   173,   190,   209,   230,   253,   279,   307,
    337,   371,   408,   449,   494,   544,   598,   658,
    724,   796,   876,   963,  1060,  1166,  1282,  1411,
    1552,  1707,  1878,  2066,  2272,  2499,  2749,  3024,
    3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,
    7132,  7845,  8630,  9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

// Westwood ADPCM index adjustment table
static const int8_t WW_INDEX_TABLE[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

//=============================================================================
// AudFile Implementation
//=============================================================================

AudFile::AudFile()
    : loaded_(false) {
    memset(&header_, 0, sizeof(header_));
}

AudFile::~AudFile() {
    Clear();
}

AudFile::AudFile(AudFile&& other) noexcept
    : header_(other.header_)
    , pcm_data_(std::move(other.pcm_data_))
    , filename_(std::move(other.filename_))
    , loaded_(other.loaded_) {
    other.loaded_ = false;
    memset(&other.header_, 0, sizeof(other.header_));
}

AudFile& AudFile::operator=(AudFile&& other) noexcept {
    if (this != &other) {
        header_ = other.header_;
        pcm_data_ = std::move(other.pcm_data_);
        filename_ = std::move(other.filename_);
        loaded_ = other.loaded_;

        other.loaded_ = false;
        memset(&other.header_, 0, sizeof(other.header_));
    }
    return *this;
}

void AudFile::Clear() {
    pcm_data_.clear();
    pcm_data_.shrink_to_fit();
    filename_.clear();
    memset(&header_, 0, sizeof(header_));
    loaded_ = false;
}

//=============================================================================
// Loading Methods
//=============================================================================

bool AudFile::LoadFromData(const uint8_t* data, uint32_t size) {
    Clear();

    // Validate minimum size for header
    if (!data || size < sizeof(AudHeader)) {
        Platform_LogInfo("AudFile: Data too small for header");
        return false;
    }

    // Copy header
    memcpy(&header_, data, sizeof(AudHeader));

    // Validate header
    if (header_.sample_rate == 0 || header_.sample_rate > 48000) {
        char msg[128];
        snprintf(msg, sizeof(msg), "AudFile: Invalid sample rate %u", header_.sample_rate);
        Platform_LogInfo(msg);
        return false;
    }

    if (header_.compressed_size == 0) {
        Platform_LogInfo("AudFile: Zero compressed size");
        return false;
    }

    // Check we have enough data
    uint32_t expected_size = sizeof(AudHeader) + header_.compressed_size;
    if (size < expected_size) {
        char msg[128];
        snprintf(msg, sizeof(msg), "AudFile: Insufficient data (%u < %u)", size, expected_size);
        Platform_LogInfo(msg);
        return false;
    }

    // Get pointer to audio data (after header)
    const uint8_t* audio_data = data + sizeof(AudHeader);
    uint32_t audio_size = header_.compressed_size;

    // Decode based on compression type
    bool success = false;

    switch (header_.compression) {
        case AUD_COMPRESS_NONE:
            success = DecodeUncompressed(audio_data, audio_size);
            break;

        case AUD_COMPRESS_WW:
            success = DecodeWwAdpcm(audio_data, audio_size);
            break;

        case AUD_COMPRESS_IMA:
            success = DecodeImaAdpcm(audio_data, audio_size);
            break;

        default:
            {
                char msg[128];
                snprintf(msg, sizeof(msg), "AudFile: Unknown compression type %u", header_.compression);
                Platform_LogInfo(msg);
            }
            return false;
    }

    if (success) {
        loaded_ = true;
    }

    return success;
}

bool AudFile::LoadFromMix(const char* filename) {
    if (!filename) return false;

    // Check if file exists in any loaded MIX
    int32_t file_size = Platform_Mix_GetSize(filename);
    if (file_size <= 0) {
        char msg[256];
        snprintf(msg, sizeof(msg), "AudFile: File not found in MIX: %s", filename);
        Platform_LogInfo(msg);
        return false;
    }

    // Allocate buffer and read file
    std::vector<uint8_t> buffer(static_cast<size_t>(file_size));
    int32_t bytes_read = Platform_Mix_Read(filename, buffer.data(), file_size);

    if (bytes_read != file_size) {
        char msg[256];
        snprintf(msg, sizeof(msg), "AudFile: Failed to read %s (got %d, expected %d)",
                 filename, bytes_read, file_size);
        Platform_LogInfo(msg);
        return false;
    }

    filename_ = filename;
    return LoadFromData(buffer.data(), static_cast<uint32_t>(file_size));
}

//=============================================================================
// Decode Uncompressed PCM
//=============================================================================

bool AudFile::DecodeUncompressed(const uint8_t* src, uint32_t src_size) {
    if (Is16Bit()) {
        // 16-bit PCM - copy directly
        size_t sample_count = src_size / 2;
        pcm_data_.resize(sample_count);

        const int16_t* src16 = reinterpret_cast<const int16_t*>(src);
        for (size_t i = 0; i < sample_count; i++) {
            pcm_data_[i] = src16[i];
        }
    } else {
        // 8-bit unsigned PCM - convert to 16-bit signed
        size_t sample_count = src_size;
        pcm_data_.resize(sample_count);

        for (size_t i = 0; i < sample_count; i++) {
            // Convert unsigned 8-bit [0..255] to signed 16-bit [-32768..32767]
            int16_t sample = (static_cast<int16_t>(src[i]) - 128) * 256;
            pcm_data_[i] = sample;
        }
    }

    return true;
}

//=============================================================================
// WW ADPCM Decoder
//=============================================================================

int16_t AudFile::DecodeWwNibble(WwAdpcmState& state, uint8_t nibble) {
    // Get step size from table
    int32_t step = WW_STEP_TABLE[state.step_index];

    // Calculate difference
    int32_t diff = step >> 3;

    if (nibble & 1) diff += step >> 2;
    if (nibble & 2) diff += step >> 1;
    if (nibble & 4) diff += step;

    // Apply sign bit
    if (nibble & 8) {
        diff = -diff;
    }

    // Update predictor
    state.predictor += diff;

    // Clamp to 16-bit range
    if (state.predictor > 32767) state.predictor = 32767;
    if (state.predictor < -32768) state.predictor = -32768;

    // Update step index
    state.step_index += WW_INDEX_TABLE[nibble & 0x0F];

    // Clamp step index
    if (state.step_index < 0) state.step_index = 0;
    if (state.step_index > 88) state.step_index = 88;

    return static_cast<int16_t>(state.predictor);
}

bool AudFile::DecodeWwAdpcm(const uint8_t* src, uint32_t src_size) {
    // WW ADPCM works in 16-byte chunks:
    //   - 2 bytes: initial predictor (int16)
    //   - 1 byte: initial step index
    //   - 1 byte: reserved (0)
    //   - 12 bytes: ADPCM nibbles (24 samples)
    // Total: 25 samples per 16-byte chunk

    const uint32_t CHUNK_SIZE = 16;
    const uint32_t NIBBLE_BYTES = 12;
    const uint32_t SAMPLES_PER_CHUNK = 1 + (NIBBLE_BYTES * 2);

    if (src_size < CHUNK_SIZE) {
        Platform_LogInfo("AudFile: WW ADPCM data too small");
        return false;
    }

    // Calculate expected output size
    uint32_t num_chunks = src_size / CHUNK_SIZE;
    uint32_t expected_samples = num_chunks * SAMPLES_PER_CHUNK;

    if (IsStereo()) {
        num_chunks /= 2;
        expected_samples = num_chunks * SAMPLES_PER_CHUNK * 2;
    }

    pcm_data_.reserve(expected_samples);

    if (IsStereo()) {
        // Stereo decoding - interleave L/R samples
        WwAdpcmState state_l, state_r;

        const uint8_t* ptr = src;
        const uint8_t* end = src + src_size;

        while (ptr + CHUNK_SIZE * 2 <= end) {
            // Left channel chunk
            int16_t initial_l = *reinterpret_cast<const int16_t*>(ptr);
            uint8_t index_l = ptr[2];
            const uint8_t* nibbles_l = ptr + 4;
            ptr += CHUNK_SIZE;

            // Right channel chunk
            int16_t initial_r = *reinterpret_cast<const int16_t*>(ptr);
            uint8_t index_r = ptr[2];
            const uint8_t* nibbles_r = ptr + 4;
            ptr += CHUNK_SIZE;

            // Initialize states
            state_l.predictor = initial_l;
            state_l.step_index = std::min(static_cast<int32_t>(index_l), 88);
            state_r.predictor = initial_r;
            state_r.step_index = std::min(static_cast<int32_t>(index_r), 88);

            // Output initial samples (interleaved)
            pcm_data_.push_back(initial_l);
            pcm_data_.push_back(initial_r);

            // Decode nibbles
            for (uint32_t i = 0; i < NIBBLE_BYTES; i++) {
                int16_t sample_l1 = DecodeWwNibble(state_l, nibbles_l[i] & 0x0F);
                int16_t sample_r1 = DecodeWwNibble(state_r, nibbles_r[i] & 0x0F);
                pcm_data_.push_back(sample_l1);
                pcm_data_.push_back(sample_r1);

                int16_t sample_l2 = DecodeWwNibble(state_l, (nibbles_l[i] >> 4) & 0x0F);
                int16_t sample_r2 = DecodeWwNibble(state_r, (nibbles_r[i] >> 4) & 0x0F);
                pcm_data_.push_back(sample_l2);
                pcm_data_.push_back(sample_r2);
            }
        }
    } else {
        // Mono decoding
        WwAdpcmState state;

        const uint8_t* ptr = src;
        const uint8_t* end = src + src_size;

        while (ptr + CHUNK_SIZE <= end) {
            // Read chunk header
            int16_t initial = *reinterpret_cast<const int16_t*>(ptr);
            uint8_t index = ptr[2];
            const uint8_t* nibbles = ptr + 4;
            ptr += CHUNK_SIZE;

            // Initialize state from chunk header
            state.predictor = initial;
            state.step_index = std::min(static_cast<int32_t>(index), 88);

            // Output initial sample
            pcm_data_.push_back(initial);

            // Decode nibbles
            for (uint32_t i = 0; i < NIBBLE_BYTES; i++) {
                int16_t sample1 = DecodeWwNibble(state, nibbles[i] & 0x0F);
                pcm_data_.push_back(sample1);

                int16_t sample2 = DecodeWwNibble(state, (nibbles[i] >> 4) & 0x0F);
                pcm_data_.push_back(sample2);
            }
        }
    }

    return !pcm_data_.empty();
}

//=============================================================================
// IMA ADPCM Decoder
//=============================================================================

bool AudFile::DecodeImaAdpcm(const uint8_t* src, uint32_t src_size) {
    // Each byte produces 2 samples
    size_t expected_samples = src_size * 2;

    pcm_data_.resize(expected_samples);

    // IMA ADPCM tables (same as WW)
    static const int16_t ima_step_table[89] = {
        7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
        19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
        50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
        130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
        337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
        876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
        2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
        5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
        15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
    };

    static const int8_t ima_index_table[16] = {
        -1, -1, -1, -1, 2, 4, 6, 8,
        -1, -1, -1, -1, 2, 4, 6, 8
    };

    int32_t predictor = 0;
    int32_t step_index = 0;
    size_t out_idx = 0;

    for (uint32_t i = 0; i < src_size && out_idx < expected_samples; i++) {
        uint8_t byte = src[i];

        // Decode low nibble
        {
            uint8_t nibble = byte & 0x0F;
            int32_t step = ima_step_table[step_index];

            int32_t diff = step >> 3;
            if (nibble & 1) diff += step >> 2;
            if (nibble & 2) diff += step >> 1;
            if (nibble & 4) diff += step;
            if (nibble & 8) diff = -diff;

            predictor += diff;
            if (predictor > 32767) predictor = 32767;
            if (predictor < -32768) predictor = -32768;

            pcm_data_[out_idx++] = static_cast<int16_t>(predictor);

            step_index += ima_index_table[nibble];
            if (step_index < 0) step_index = 0;
            if (step_index > 88) step_index = 88;
        }

        // Decode high nibble
        {
            uint8_t nibble = (byte >> 4) & 0x0F;
            int32_t step = ima_step_table[step_index];

            int32_t diff = step >> 3;
            if (nibble & 1) diff += step >> 2;
            if (nibble & 2) diff += step >> 1;
            if (nibble & 4) diff += step;
            if (nibble & 8) diff = -diff;

            predictor += diff;
            if (predictor > 32767) predictor = 32767;
            if (predictor < -32768) predictor = -32768;

            pcm_data_[out_idx++] = static_cast<int16_t>(predictor);

            step_index += ima_index_table[nibble];
            if (step_index < 0) step_index = 0;
            if (step_index > 88) step_index = 88;
        }
    }

    pcm_data_.resize(out_idx);
    return !pcm_data_.empty();
}

//=============================================================================
// Property Methods
//=============================================================================

const char* AudFile::GetCompressionName() const {
    switch (header_.compression) {
        case AUD_COMPRESS_NONE: return "Uncompressed";
        case AUD_COMPRESS_WW:   return "WW ADPCM";
        case AUD_COMPRESS_IMA:  return "IMA ADPCM";
        default:                return "Unknown";
    }
}

uint32_t AudFile::GetDurationMs() const {
    if (!loaded_ || header_.sample_rate == 0) return 0;

    size_t mono_samples = pcm_data_.size() / GetChannels();
    return static_cast<uint32_t>((mono_samples * 1000) / header_.sample_rate);
}

float AudFile::GetDurationSeconds() const {
    return GetDurationMs() / 1000.0f;
}

void AudFile::PrintInfo() const {
    char msg[256];

    Platform_LogInfo("AUD File Info:");

    snprintf(msg, sizeof(msg), "  Filename: %s",
             filename_.empty() ? "(memory)" : filename_.c_str());
    Platform_LogInfo(msg);

    snprintf(msg, sizeof(msg), "  Sample Rate: %u Hz", header_.sample_rate);
    Platform_LogInfo(msg);

    snprintf(msg, sizeof(msg), "  Channels: %d (%s)", GetChannels(),
             IsStereo() ? "stereo" : "mono");
    Platform_LogInfo(msg);

    snprintf(msg, sizeof(msg), "  Bits/Sample: %d", GetBitsPerSample());
    Platform_LogInfo(msg);

    snprintf(msg, sizeof(msg), "  Compression: %s (%u)",
             GetCompressionName(), header_.compression);
    Platform_LogInfo(msg);

    snprintf(msg, sizeof(msg), "  Compressed Size: %u bytes", header_.compressed_size);
    Platform_LogInfo(msg);

    snprintf(msg, sizeof(msg), "  Uncompressed Size: %u bytes", header_.uncompressed_size);
    Platform_LogInfo(msg);

    snprintf(msg, sizeof(msg), "  Decoded PCM: %zu samples (%zu bytes)",
             pcm_data_.size(), GetPCMDataSize());
    Platform_LogInfo(msg);

    snprintf(msg, sizeof(msg), "  Duration: %.2f seconds", GetDurationSeconds());
    Platform_LogInfo(msg);
}

//=============================================================================
// Convenience Functions
//=============================================================================

bool LoadAudFromMix(const char* filename,
                    std::vector<int16_t>& out_data,
                    uint16_t* out_sample_rate,
                    int* out_channels) {
    AudFile aud;
    if (!aud.LoadFromMix(filename)) {
        return false;
    }

    out_data.assign(aud.GetPCMData(), aud.GetPCMData() + aud.GetPCMSampleCount());

    if (out_sample_rate) {
        *out_sample_rate = aud.GetSampleRate();
    }
    if (out_channels) {
        *out_channels = aud.GetChannels();
    }

    return true;
}

bool GetAudInfo(const uint8_t* data, uint32_t size, AudHeader* out_header) {
    if (!data || size < sizeof(AudHeader)) {
        return false;
    }

    AudHeader header;
    memcpy(&header, data, sizeof(AudHeader));

    // Basic validation
    if (header.sample_rate == 0 || header.sample_rate > 48000) {
        return false;
    }
    if (header.compressed_size == 0) {
        return false;
    }

    if (out_header) {
        *out_header = header;
    }

    return true;
}
