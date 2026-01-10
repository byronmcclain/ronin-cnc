# Task 17a: AUD File Format Parser

| Field | Value |
|-------|-------|
| **Task ID** | 17a |
| **Task Name** | AUD File Format Parser |
| **Phase** | 17 - Audio Integration |
| **Depends On** | Platform Audio API (08-audio), MixManager (12-asset-system) |
| **Estimated Complexity** | Medium-High (~500 lines) |

---

## Dependencies

- Platform audio API must be functional:
  - `Platform_Audio_Init()`, `Platform_Audio_Shutdown()`
  - `Platform_Sound_CreateFromMemory()`, `Platform_Sound_CreateFromADPCM()`
  - IMA ADPCM decoder available in platform layer
- MixManager must be implemented:
  - `MixManager::Instance().FindFile()` to locate AUD files in MIX archives
- C++ standard library for vectors and file handling

---

## Context

Red Alert uses Westwood's proprietary AUD format for all audio content including sound effects, music, and voice lines. AUD files are stored inside MIX archive files (SOUNDS.MIX, SCORES.MIX, SPEECH.MIX). This task implements the AUD file parser and decoders needed to extract PCM audio data that can be played through the platform audio system.

The AUD format supports three compression modes:
1. **Uncompressed** - Raw PCM data (rare, used for very short sounds)
2. **WW ADPCM** (type 1) - Westwood's proprietary ADPCM variant
3. **IMA ADPCM** (type 99) - Standard IMA ADPCM (already supported by platform)

This is a foundational task - all subsequent audio tasks depend on the AUD loader to access game audio content.

---

## Objective

Create a complete AUD file parser that can:
1. Parse the 12-byte AUD header to extract audio properties
2. Decode WW ADPCM compressed audio to 16-bit PCM
3. Handle IMA ADPCM via the platform decoder
4. Support uncompressed PCM passthrough
5. Load AUD files from MIX archives via MixManager
6. Provide clean C++ interface for sound/music/voice systems

---

## Technical Background

### AUD File Format

The AUD format is a simple container with a fixed header followed by audio data.

**Header Structure (12 bytes):**
```
Offset  Size  Type    Description
------  ----  ------  -----------
0x0000  2     uint16  Sample rate (Hz) - typically 22050
0x0002  4     uint32  Uncompressed size in bytes
0x0006  4     uint32  Compressed size in bytes (or same as uncompressed if raw)
0x000A  1     uint8   Flags:
                        bit 0: Stereo (1) or Mono (0)
                        bit 1: 16-bit (1) or 8-bit (0)
0x000B  1     uint8   Compression type:
                        0 = Uncompressed PCM
                        1 = WW ADPCM (Westwood proprietary)
                        99 = IMA ADPCM (standard)
```

**Audio Data:**
After the 12-byte header, the audio data follows. Format depends on compression type.

### WW ADPCM (Westwood ADPCM)

Westwood's ADPCM is a modified ADPCM that operates on 16-byte chunks:

**Chunk Structure (for mono):**
```
Byte 0-1:  uint16 - Initial predictor value (sample)
Byte 2:    uint8  - Initial step index (0-88)
Byte 3:    uint8  - Reserved (must be 0)
Byte 4-15: 12 bytes of nibble-packed ADPCM codes (24 samples)
```

Each chunk decodes to approximately 512 samples. The algorithm is similar to IMA ADPCM but with different step table coefficients.

**WW ADPCM Step Table (89 values):**
```c
static const int16_t ww_step_table[89] = {
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
```

**WW ADPCM Index Table:**
```c
static const int8_t ww_index_table[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};
```

### Decoding Algorithm

```
For each nibble (4-bit code):
1. Get step = step_table[step_index]
2. Calculate difference:
   diff = step >> 3
   if (nibble & 1) diff += step >> 2
   if (nibble & 2) diff += step >> 1
   if (nibble & 4) diff += step
   if (nibble & 8) diff = -diff  // Sign bit
3. predictor += diff
4. Clamp predictor to [-32768, 32767]
5. Output predictor as sample
6. step_index += index_table[nibble]
7. Clamp step_index to [0, 88]
```

### Common AUD Files

**Sound Effects (SOUNDS.MIX):**
- `CLICK.AUD` - UI click sound
- `XPLOS.AUD` - Small explosion
- `XPLOBIG.AUD` - Large explosion
- `GUNSHOT.AUD` - Gunfire
- `CANNON.AUD` - Tank cannon

**Music (SCORES.MIX):**
- `HELLMRCH.AUD` - Hell March (iconic track)
- `BIGFOOT.AUD`, `CRUSH.AUD`, etc.

**Voice (SPEECH.MIX):**
- `REPORTN1.AUD` - "Reporting"
- `ACKNO1.AUD` - "Acknowledged"
- `UNITREDY.AUD` - "Unit ready"

---

## Deliverables

- [ ] `include/game/audio/aud_file.h` - AUD file loader header
- [ ] `src/game/audio/aud_file.cpp` - Implementation
- [ ] `src/test_aud_loader.cpp` - Unit tests and interactive test
- [ ] CMakeLists.txt updated
- [ ] All tests pass

---

## Files to Create

### include/game/audio/aud_file.h

```cpp
// include/game/audio/aud_file.h
// Westwood AUD file format loader
// Task 17a - AUD File Format Parser

#ifndef AUD_FILE_H
#define AUD_FILE_H

#include <cstdint>
#include <vector>
#include <string>

//=============================================================================
// AUD Compression Types
//=============================================================================

enum AudCompressionType : uint8_t {
    AUD_COMPRESS_NONE   = 0,    // Uncompressed PCM
    AUD_COMPRESS_WW     = 1,    // Westwood ADPCM
    AUD_COMPRESS_IMA    = 99,   // IMA ADPCM
};

//=============================================================================
// AUD Header Structure
//=============================================================================

#pragma pack(push, 1)
struct AudHeader {
    uint16_t sample_rate;        // Sample rate in Hz (typically 22050)
    uint32_t uncompressed_size;  // Size of decoded PCM data
    uint32_t compressed_size;    // Size of compressed data
    uint8_t  flags;              // bit 0: stereo, bit 1: 16-bit
    uint8_t  compression;        // Compression type (see AudCompressionType)
};
#pragma pack(pop)

static_assert(sizeof(AudHeader) == 12, "AudHeader must be 12 bytes");

//=============================================================================
// AUD File Flags
//=============================================================================

constexpr uint8_t AUD_FLAG_STEREO = 0x01;  // Stereo audio (vs mono)
constexpr uint8_t AUD_FLAG_16BIT  = 0x02;  // 16-bit samples (vs 8-bit)

//=============================================================================
// WW ADPCM Decoder State
//=============================================================================

struct WwAdpcmState {
    int32_t predictor;   // Current predicted sample value
    int32_t step_index;  // Current index into step table

    WwAdpcmState() : predictor(0), step_index(0) {}
    void Reset() { predictor = 0; step_index = 0; }
};

//=============================================================================
// AUD File Loader Class
//=============================================================================

class AudFile {
public:
    AudFile();
    ~AudFile();

    // Prevent copying (owns PCM data)
    AudFile(const AudFile&) = delete;
    AudFile& operator=(const AudFile&) = delete;

    // Allow moving
    AudFile(AudFile&& other) noexcept;
    AudFile& operator=(AudFile&& other) noexcept;

    //=========================================================================
    // Loading Methods
    //=========================================================================

    /// Load AUD from raw data buffer
    /// @param data Pointer to AUD file data (header + compressed audio)
    /// @param size Size of data buffer in bytes
    /// @return true if loading succeeded
    bool LoadFromData(const uint8_t* data, uint32_t size);

    /// Load AUD from a MIX archive file
    /// @param filename AUD filename (e.g., "CLICK.AUD")
    /// @return true if file found and loaded successfully
    bool LoadFromMix(const char* filename);

    /// Load AUD from a specific MIX archive
    /// @param mix_name Name of MIX file (e.g., "SOUNDS.MIX")
    /// @param filename AUD filename within that MIX
    /// @return true if found and loaded
    bool LoadFromMix(const char* mix_name, const char* filename);

    /// Clear loaded data and reset state
    void Clear();

    /// Check if file is loaded and valid
    bool IsLoaded() const { return loaded_; }

    //=========================================================================
    // Audio Properties
    //=========================================================================

    /// Get sample rate in Hz
    uint16_t GetSampleRate() const { return header_.sample_rate; }

    /// Get original uncompressed size
    uint32_t GetUncompressedSize() const { return header_.uncompressed_size; }

    /// Get compressed data size
    uint32_t GetCompressedSize() const { return header_.compressed_size; }

    /// Check if audio is stereo
    bool IsStereo() const { return (header_.flags & AUD_FLAG_STEREO) != 0; }

    /// Check if audio is 16-bit (vs 8-bit)
    bool Is16Bit() const { return (header_.flags & AUD_FLAG_16BIT) != 0; }

    /// Get number of audio channels (1 = mono, 2 = stereo)
    int GetChannels() const { return IsStereo() ? 2 : 1; }

    /// Get bits per sample (8 or 16)
    int GetBitsPerSample() const { return Is16Bit() ? 16 : 8; }

    /// Get compression type
    AudCompressionType GetCompressionType() const {
        return static_cast<AudCompressionType>(header_.compression);
    }

    /// Get human-readable compression name
    const char* GetCompressionName() const;

    //=========================================================================
    // PCM Data Access
    //=========================================================================

    /// Get pointer to decoded PCM data
    /// Data is always 16-bit signed, mono or stereo depending on IsStereo()
    const int16_t* GetPCMData() const { return pcm_data_.data(); }

    /// Get number of PCM samples (total across all channels)
    size_t GetPCMSampleCount() const { return pcm_data_.size(); }

    /// Get size of PCM data in bytes
    size_t GetPCMDataSize() const { return pcm_data_.size() * sizeof(int16_t); }

    /// Get raw PCM data as byte pointer (for platform API)
    const uint8_t* GetPCMDataBytes() const {
        return reinterpret_cast<const uint8_t*>(pcm_data_.data());
    }

    /// Get duration in milliseconds
    uint32_t GetDurationMs() const;

    /// Get duration in seconds
    float GetDurationSeconds() const;

    //=========================================================================
    // Debug Information
    //=========================================================================

    /// Get filename if loaded from MIX
    const std::string& GetFilename() const { return filename_; }

    /// Print debug info about the loaded file
    void PrintInfo() const;

private:
    AudHeader header_;
    std::vector<int16_t> pcm_data_;
    std::string filename_;
    bool loaded_;

    //=========================================================================
    // Decoder Methods
    //=========================================================================

    /// Decode uncompressed PCM (convert 8-bit to 16-bit if needed)
    bool DecodeUncompressed(const uint8_t* src, uint32_t src_size);

    /// Decode Westwood ADPCM compressed data
    bool DecodeWwAdpcm(const uint8_t* src, uint32_t src_size);

    /// Decode IMA ADPCM using platform decoder
    bool DecodeImaAdpcm(const uint8_t* src, uint32_t src_size);

    /// Decode a single WW ADPCM nibble
    int16_t DecodeWwNibble(WwAdpcmState& state, uint8_t nibble);
};

//=============================================================================
// Convenience Functions
//=============================================================================

/// Load an AUD file from MIX and return decoded PCM data
/// @param filename AUD filename (e.g., "CLICK.AUD")
/// @param out_data Output vector for PCM samples
/// @param out_sample_rate Output sample rate
/// @param out_channels Output channel count
/// @return true if successful
bool LoadAudFromMix(const char* filename,
                    std::vector<int16_t>& out_data,
                    uint16_t* out_sample_rate = nullptr,
                    int* out_channels = nullptr);

/// Quick function to get AUD file info without full decode
/// @param data Raw AUD file data
/// @param size Size of data
/// @param out_header Output header (optional)
/// @return true if valid AUD header
bool GetAudInfo(const uint8_t* data, uint32_t size, AudHeader* out_header = nullptr);

#endif // AUD_FILE_H
```

### src/game/audio/aud_file.cpp

```cpp
// src/game/audio/aud_file.cpp
// Westwood AUD file format loader implementation
// Task 17a - AUD File Format Parser

#include "game/audio/aud_file.h"
#include "game/mix_manager.h"
#include "platform.h"
#include <cstring>
#include <algorithm>

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
        Platform_Log("AudFile: Data too small for header (%u bytes)", size);
        return false;
    }

    // Copy header
    memcpy(&header_, data, sizeof(AudHeader));

    // Validate header
    if (header_.sample_rate == 0 || header_.sample_rate > 48000) {
        Platform_Log("AudFile: Invalid sample rate %u", header_.sample_rate);
        return false;
    }

    if (header_.compressed_size == 0) {
        Platform_Log("AudFile: Zero compressed size");
        return false;
    }

    // Check we have enough data
    uint32_t expected_size = sizeof(AudHeader) + header_.compressed_size;
    if (size < expected_size) {
        Platform_Log("AudFile: Insufficient data (%u < %u)", size, expected_size);
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
            Platform_Log("AudFile: Unknown compression type %u", header_.compression);
            return false;
    }

    if (success) {
        loaded_ = true;
    }

    return success;
}

bool AudFile::LoadFromMix(const char* filename) {
    if (!filename) return false;

    // Try to find file in any loaded MIX
    uint32_t size = 0;
    const uint8_t* data = MixManager::Instance().FindFile(filename, &size);

    if (!data) {
        Platform_Log("AudFile: File not found in MIX: %s", filename);
        return false;
    }

    filename_ = filename;
    return LoadFromData(data, size);
}

bool AudFile::LoadFromMix(const char* mix_name, const char* filename) {
    if (!mix_name || !filename) return false;

    // Try to find file in specific MIX
    uint32_t size = 0;
    const uint8_t* data = MixManager::Instance().FindFileInMix(mix_name, filename, &size);

    if (!data) {
        Platform_Log("AudFile: File not found: %s in %s", filename, mix_name);
        return false;
    }

    filename_ = filename;
    return LoadFromData(data, size);
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
    // diff = step/8 + step/4*b0 + step/2*b1 + step*b2
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
    // WW ADPCM works in chunks. For mono:
    // Each chunk is 16 bytes:
    //   - 2 bytes: initial predictor (int16)
    //   - 1 byte: initial step index
    //   - 1 byte: reserved (0)
    //   - 12 bytes: ADPCM nibbles (24 samples)
    // Total: 25 samples per 16-byte chunk (1 initial + 24 decoded)
    //
    // For stereo, chunks are interleaved.

    const uint32_t CHUNK_SIZE = 16;
    const uint32_t NIBBLE_BYTES = 12;  // Bytes of ADPCM data per chunk
    const uint32_t SAMPLES_PER_CHUNK = 1 + (NIBBLE_BYTES * 2);  // 25 samples

    if (src_size < CHUNK_SIZE) {
        Platform_Log("AudFile: WW ADPCM data too small");
        return false;
    }

    // Calculate expected output size
    uint32_t num_chunks = src_size / CHUNK_SIZE;
    uint32_t expected_samples = num_chunks * SAMPLES_PER_CHUNK;

    if (IsStereo()) {
        // For stereo, chunks alternate L/R
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
                // Each byte has 2 nibbles (low first, then high)
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
                // Low nibble first
                int16_t sample1 = DecodeWwNibble(state, nibbles[i] & 0x0F);
                pcm_data_.push_back(sample1);

                // High nibble second
                int16_t sample2 = DecodeWwNibble(state, (nibbles[i] >> 4) & 0x0F);
                pcm_data_.push_back(sample2);
            }
        }
    }

    return !pcm_data_.empty();
}

//=============================================================================
// IMA ADPCM Decoder (via platform)
//=============================================================================

bool AudFile::DecodeImaAdpcm(const uint8_t* src, uint32_t src_size) {
    // IMA ADPCM is decoded by the platform layer
    // We need to convert to 16-bit PCM samples

    // Calculate expected output size
    // IMA ADPCM: each byte produces 2 samples
    size_t expected_samples = src_size * 2;

    // For stereo, samples are interleaved
    if (IsStereo()) {
        // Actually, stereo IMA ADPCM in AUD files is typically block-based
        // For simplicity, treat as mono and let the platform handle it
    }

    // Use platform's ADPCM decoder
    // The platform creates a sound handle, but we want raw samples
    // We'll use the ADPCM decode function directly if available,
    // or decode manually using the same algorithm

    pcm_data_.resize(expected_samples);

    // IMA ADPCM decoding (same algorithm as platform, implemented locally)
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

    // Trim to actual decoded size
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

    // Total samples / channels = mono samples
    size_t mono_samples = pcm_data_.size() / GetChannels();

    // Duration = samples / sample_rate * 1000
    return static_cast<uint32_t>((mono_samples * 1000) / header_.sample_rate);
}

float AudFile::GetDurationSeconds() const {
    return GetDurationMs() / 1000.0f;
}

void AudFile::PrintInfo() const {
    Platform_Log("AUD File Info:");
    Platform_Log("  Filename: %s", filename_.empty() ? "(memory)" : filename_.c_str());
    Platform_Log("  Sample Rate: %u Hz", header_.sample_rate);
    Platform_Log("  Channels: %d (%s)", GetChannels(), IsStereo() ? "stereo" : "mono");
    Platform_Log("  Bits/Sample: %d", GetBitsPerSample());
    Platform_Log("  Compression: %s (%u)", GetCompressionName(), header_.compression);
    Platform_Log("  Compressed Size: %u bytes", header_.compressed_size);
    Platform_Log("  Uncompressed Size: %u bytes", header_.uncompressed_size);
    Platform_Log("  Decoded PCM: %zu samples (%zu bytes)",
                 pcm_data_.size(), GetPCMDataSize());
    Platform_Log("  Duration: %.2f seconds", GetDurationSeconds());
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

    // Copy PCM data
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
```

### src/test_aud_loader.cpp

```cpp
// src/test_aud_loader.cpp
// Unit tests for AUD file loader
// Task 17a - AUD File Format Parser

#include "game/audio/aud_file.h"
#include "game/mix_manager.h"
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

// Create a simple uncompressed AUD file in memory
static std::vector<uint8_t> CreateMockUncompressedAud(
    uint16_t sample_rate,
    bool stereo,
    bool is_16bit,
    const std::vector<int16_t>& samples)
{
    std::vector<uint8_t> data;

    // Calculate sizes
    size_t sample_bytes = is_16bit ? samples.size() * 2 : samples.size();

    // Header
    AudHeader header;
    header.sample_rate = sample_rate;
    header.uncompressed_size = static_cast<uint32_t>(sample_bytes);
    header.compressed_size = static_cast<uint32_t>(sample_bytes);
    header.flags = (stereo ? AUD_FLAG_STEREO : 0) | (is_16bit ? AUD_FLAG_16BIT : 0);
    header.compression = AUD_COMPRESS_NONE;

    data.resize(sizeof(AudHeader) + sample_bytes);
    memcpy(data.data(), &header, sizeof(AudHeader));

    // Audio data
    if (is_16bit) {
        memcpy(data.data() + sizeof(AudHeader), samples.data(), sample_bytes);
    } else {
        // Convert to 8-bit unsigned
        for (size_t i = 0; i < samples.size(); i++) {
            int16_t s = samples[i];
            uint8_t u8 = static_cast<uint8_t>((s / 256) + 128);
            data[sizeof(AudHeader) + i] = u8;
        }
    }

    return data;
}

// Create mock IMA ADPCM data
static std::vector<uint8_t> CreateMockImaAdpcmAud(uint16_t sample_rate, size_t num_bytes) {
    std::vector<uint8_t> data;

    AudHeader header;
    header.sample_rate = sample_rate;
    header.uncompressed_size = static_cast<uint32_t>(num_bytes * 4);  // Approximate
    header.compressed_size = static_cast<uint32_t>(num_bytes);
    header.flags = 0;  // Mono, 16-bit output
    header.compression = AUD_COMPRESS_IMA;

    data.resize(sizeof(AudHeader) + num_bytes);
    memcpy(data.data(), &header, sizeof(AudHeader));

    // Fill with semi-realistic ADPCM nibbles
    for (size_t i = 0; i < num_bytes; i++) {
        // Alternating nibbles that produce a simple waveform
        data[sizeof(AudHeader) + i] = static_cast<uint8_t>((i % 2) ? 0x77 : 0x11);
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
    // Create test data: simple sine-like samples
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

    // Verify samples match
    const int16_t* pcm = aud.GetPCMData();
    for (size_t i = 0; i < samples.size(); i++) {
        TEST_ASSERT(pcm[i] == samples[i], "Sample mismatch");
    }

    return true;
}

bool Test_LoadUncompressed8Bit() {
    // 8-bit samples get converted to 16-bit
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
    // Stereo samples: L, R, L, R, ...
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
    AudHeader bad_header = {0};
    bad_header.sample_rate = 0;
    bad_header.compressed_size = 100;
    TEST_ASSERT(!aud.LoadFromData(reinterpret_cast<uint8_t*>(&bad_header),
                                  sizeof(bad_header) + 100),
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

//=============================================================================
// Integration Test (requires MIX files)
//=============================================================================

bool Test_LoadFromMix() {
    printf("\n--- Integration Test: Load from MIX ---\n");

    // This test requires actual game assets
    // Skip if MixManager not available or no MIX files loaded

    if (!MixManager::Instance().HasAnyMixLoaded()) {
        printf("  SKIPPED: No MIX files loaded\n");
        return true;  // Not a failure, just skip
    }

    // Try to load a common sound effect
    const char* test_files[] = {
        "CLICK.AUD",
        "XPLOS.AUD",
        "GUNSHOT.AUD"
    };

    AudFile aud;
    bool found_any = false;

    for (const char* filename : test_files) {
        if (aud.LoadFromMix(filename)) {
            printf("  Loaded %s:\n", filename);
            printf("    Sample rate: %u Hz\n", aud.GetSampleRate());
            printf("    Channels: %d\n", aud.GetChannels());
            printf("    Compression: %s\n", aud.GetCompressionName());
            printf("    Duration: %.2f seconds\n", aud.GetDurationSeconds());
            printf("    PCM samples: %zu\n", aud.GetPCMSampleCount());
            found_any = true;
            break;
        }
    }

    if (!found_any) {
        printf("  SKIPPED: No test AUD files found in MIX archives\n");
    }

    return true;
}

//=============================================================================
// Interactive Test
//=============================================================================

void Interactive_Test() {
    printf("\n=== Interactive AUD Playback Test ===\n");

    // Initialize platform
    Platform_Init();

    AudioConfig audio_config = {};
    audio_config.sample_rate = 22050;
    audio_config.channels = 2;
    audio_config.bits_per_sample = 16;
    audio_config.buffer_size = 1024;

    if (Platform_Audio_Init(&audio_config) != 0) {
        printf("Failed to initialize audio\n");
        Platform_Shutdown();
        return;
    }

    // Try to load and play a sound from MIX
    if (!MixManager::Instance().HasAnyMixLoaded()) {
        // Try to load SOUNDS.MIX
        MixManager::Instance().LoadMix("SOUNDS.MIX");
    }

    AudFile aud;
    const char* filename = "CLICK.AUD";

    if (aud.LoadFromMix(filename)) {
        printf("Loaded %s successfully\n", filename);
        aud.PrintInfo();

        // Create sound and play it
        SoundHandle sound = Platform_Sound_CreateFromMemory(
            aud.GetPCMDataBytes(),
            static_cast<int32_t>(aud.GetPCMDataSize()),
            aud.GetSampleRate(),
            aud.GetChannels(),
            16  // Always 16-bit after decode
        );

        if (sound >= 0) {
            printf("\nPlaying sound... Press Enter to stop.\n");
            Platform_Sound_Play(sound, 1.0f, 0.0f, false);

            getchar();

            Platform_Sound_Destroy(sound);
        } else {
            printf("Failed to create sound\n");
        }
    } else {
        printf("Could not load %s - try with game data present\n", filename);
    }

    Platform_Audio_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Main
//=============================================================================

int main(int argc, char* argv[]) {
    printf("=== AUD File Loader Tests (Task 17a) ===\n\n");

    // Run unit tests
    RUN_TEST(HeaderSize);
    RUN_TEST(LoadUncompressed16Bit);
    RUN_TEST(LoadUncompressed8Bit);
    RUN_TEST(LoadStereo);
    RUN_TEST(LoadImaAdpcm);
    RUN_TEST(Duration);
    RUN_TEST(Clear);
    RUN_TEST(MoveSemantics);
    RUN_TEST(InvalidData);
    RUN_TEST(CompressionName);
    RUN_TEST(GetAudInfo);

    // Integration test
    Platform_Init();
    MixManager::Instance().Initialize();
    RUN_TEST(LoadFromMix);
    MixManager::Instance().Shutdown();
    Platform_Shutdown();

    printf("\n=== Results: %d passed, %d failed ===\n", tests_passed, tests_failed);

    // Interactive test
    if (argc > 1 && strcmp(argv[1], "-i") == 0) {
        Interactive_Test();
    } else {
        printf("\nRun with -i for interactive playback test\n");
    }

    return (tests_failed == 0) ? 0 : 1;
}
```

---

## CMakeLists.txt Additions

Add to the main `CMakeLists.txt` or the appropriate subdirectory CMakeLists:

```cmake
# =============================================================================
# Audio Integration - Task 17a: AUD File Parser
# =============================================================================

# AUD file loader source
set(AUDIO_AUD_SOURCES
    ${CMAKE_SOURCE_DIR}/src/game/audio/aud_file.cpp
)

set(AUDIO_AUD_HEADERS
    ${CMAKE_SOURCE_DIR}/include/game/audio/aud_file.h
)

# Add to game library (or create audio library if needed)
# Assuming there's a game library target:
# target_sources(game PRIVATE ${AUDIO_AUD_SOURCES})

# Test executable for AUD loader
add_executable(TestAudLoader
    ${CMAKE_SOURCE_DIR}/src/test_aud_loader.cpp
    ${AUDIO_AUD_SOURCES}
)

target_include_directories(TestAudLoader PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(TestAudLoader PRIVATE
    redalert_platform
    # mix_manager or asset library
)

# Register test
add_test(NAME AudLoaderTests COMMAND TestAudLoader)
```

---

## Verification Script

```bash
#!/bin/bash
# Verification script for Task 17a: AUD File Parser
set -e

echo "=== Task 17a Verification: AUD File Parser ==="
echo ""

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"

# Step 1: Check source files exist
echo "Step 1: Checking source files..."

FILES=(
    "include/game/audio/aud_file.h"
    "src/game/audio/aud_file.cpp"
    "src/test_aud_loader.cpp"
)

for file in "${FILES[@]}"; do
    if [ ! -f "$ROOT_DIR/$file" ]; then
        echo "VERIFY_FAILED: Missing file $file"
        exit 1
    fi
    echo "  ✓ $file"
done

# Step 2: Build test executable
echo ""
echo "Step 2: Building test executable..."

cd "$BUILD_DIR"
cmake --build . --target TestAudLoader

if [ ! -f "$BUILD_DIR/TestAudLoader" ]; then
    echo "VERIFY_FAILED: TestAudLoader executable not built"
    exit 1
fi

echo "  ✓ TestAudLoader built successfully"

# Step 3: Run unit tests
echo ""
echo "Step 3: Running unit tests..."

if ! "$BUILD_DIR/TestAudLoader"; then
    echo "VERIFY_FAILED: Unit tests failed"
    exit 1
fi

echo "  ✓ All unit tests passed"

# Step 4: Check header structure
echo ""
echo "Step 4: Verifying header structure..."

# Check AudHeader is properly defined
if ! grep -q "struct AudHeader" "$ROOT_DIR/include/game/audio/aud_file.h"; then
    echo "VERIFY_FAILED: AudHeader struct not found"
    exit 1
fi

# Check compression types are defined
if ! grep -q "AUD_COMPRESS_WW.*=.*1" "$ROOT_DIR/include/game/audio/aud_file.h"; then
    echo "VERIFY_FAILED: WW ADPCM compression type not defined"
    exit 1
fi

if ! grep -q "AUD_COMPRESS_IMA.*=.*99" "$ROOT_DIR/include/game/audio/aud_file.h"; then
    echo "VERIFY_FAILED: IMA ADPCM compression type not defined"
    exit 1
fi

echo "  ✓ Header structure correct"

# Step 5: Check decoder tables
echo ""
echo "Step 5: Verifying decoder tables..."

# Check WW step table exists
if ! grep -q "WW_STEP_TABLE\[89\]" "$ROOT_DIR/src/game/audio/aud_file.cpp"; then
    echo "VERIFY_FAILED: WW_STEP_TABLE not found or wrong size"
    exit 1
fi

# Check WW index table exists
if ! grep -q "WW_INDEX_TABLE\[16\]" "$ROOT_DIR/src/game/audio/aud_file.cpp"; then
    echo "VERIFY_FAILED: WW_INDEX_TABLE not found or wrong size"
    exit 1
fi

echo "  ✓ Decoder tables present"

echo ""
echo "=== Task 17a Verification PASSED ==="
echo ""
echo "AUD file parser implementation complete."
echo "Run '$BUILD_DIR/TestAudLoader -i' for interactive playback test."
```

---

## Implementation Steps

1. **Create directory structure** - `include/game/audio/` and `src/game/audio/`
2. **Create aud_file.h** - Header with AudHeader struct, AudFile class declaration
3. **Create aud_file.cpp** - Full implementation with WW ADPCM and IMA ADPCM decoders
4. **Create test_aud_loader.cpp** - Comprehensive unit tests
5. **Update CMakeLists.txt** - Add new files and test target
6. **Build and run unit tests** - Verify all pass
7. **Run integration test** - Test with actual game AUD files if available
8. **Run interactive test** - Verify decoded audio plays correctly

---

## Success Criteria

- [ ] AudHeader struct is exactly 12 bytes (packed)
- [ ] Uncompressed 8-bit PCM loads and converts to 16-bit correctly
- [ ] Uncompressed 16-bit PCM loads correctly
- [ ] Stereo audio loads with correct channel count
- [ ] IMA ADPCM decodes correctly (2 samples per byte)
- [ ] WW ADPCM decodes correctly (25 samples per 16-byte chunk)
- [ ] Duration calculation is accurate
- [ ] Move semantics work correctly
- [ ] Invalid data is rejected gracefully
- [ ] LoadFromMix integration works (when MIX files available)
- [ ] All unit tests pass
- [ ] Decoded audio plays without distortion

---

## Common Issues

1. **Header not 12 bytes**
   - Check `#pragma pack(push, 1)` is present
   - Verify no padding between struct members

2. **WW ADPCM sounds distorted**
   - Check step table values match original game
   - Verify nibble order (low nibble first, then high)
   - Ensure predictor/step_index reset per chunk

3. **IMA ADPCM sounds wrong**
   - Verify nibble order matches platform decoder
   - Check step_index clamping to [0, 88]

4. **File not found in MIX**
   - Ensure MixManager is initialized
   - Check filename case sensitivity
   - Verify MIX file is loaded before searching

5. **Stereo sounds mono**
   - Check AUD_FLAG_STEREO bit handling
   - Verify interleaved sample output

6. **Clicks/pops in audio**
   - Check for integer overflow in predictor
   - Verify clamping to [-32768, 32767]

---

## Completion Promise

When verification passes, output:
<promise>TASK_17A_COMPLETE</promise>

## Escape Hatch

If stuck after 15 iterations:
- Document what's blocking in `ralph-tasks/blocked/17a.md`
- List attempted approaches
- Output:
<promise>TASK_17A_BLOCKED</promise>

## Max Iterations
15
