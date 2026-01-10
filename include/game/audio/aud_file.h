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

    // Load AUD from raw data buffer
    bool LoadFromData(const uint8_t* data, uint32_t size);

    // Load AUD from a MIX archive file via platform API
    bool LoadFromMix(const char* filename);

    // Clear loaded data and reset state
    void Clear();

    // Check if file is loaded and valid
    bool IsLoaded() const { return loaded_; }

    //=========================================================================
    // Audio Properties
    //=========================================================================

    uint16_t GetSampleRate() const { return header_.sample_rate; }
    uint32_t GetUncompressedSize() const { return header_.uncompressed_size; }
    uint32_t GetCompressedSize() const { return header_.compressed_size; }
    bool IsStereo() const { return (header_.flags & AUD_FLAG_STEREO) != 0; }
    bool Is16Bit() const { return (header_.flags & AUD_FLAG_16BIT) != 0; }
    int GetChannels() const { return IsStereo() ? 2 : 1; }
    int GetBitsPerSample() const { return Is16Bit() ? 16 : 8; }

    AudCompressionType GetCompressionType() const {
        return static_cast<AudCompressionType>(header_.compression);
    }

    const char* GetCompressionName() const;

    //=========================================================================
    // PCM Data Access
    //=========================================================================

    // Data is always 16-bit signed
    const int16_t* GetPCMData() const { return pcm_data_.data(); }
    size_t GetPCMSampleCount() const { return pcm_data_.size(); }
    size_t GetPCMDataSize() const { return pcm_data_.size() * sizeof(int16_t); }

    const uint8_t* GetPCMDataBytes() const {
        return reinterpret_cast<const uint8_t*>(pcm_data_.data());
    }

    uint32_t GetDurationMs() const;
    float GetDurationSeconds() const;

    //=========================================================================
    // Debug Information
    //=========================================================================

    const std::string& GetFilename() const { return filename_; }
    void PrintInfo() const;

private:
    AudHeader header_;
    std::vector<int16_t> pcm_data_;
    std::string filename_;
    bool loaded_;

    bool DecodeUncompressed(const uint8_t* src, uint32_t src_size);
    bool DecodeWwAdpcm(const uint8_t* src, uint32_t src_size);
    bool DecodeImaAdpcm(const uint8_t* src, uint32_t src_size);
    int16_t DecodeWwNibble(WwAdpcmState& state, uint8_t nibble);
};

//=============================================================================
// Convenience Functions
//=============================================================================

// Load an AUD file from MIX and return decoded PCM data
bool LoadAudFromMix(const char* filename,
                    std::vector<int16_t>& out_data,
                    uint16_t* out_sample_rate = nullptr,
                    int* out_channels = nullptr);

// Quick function to get AUD file info without full decode
bool GetAudInfo(const uint8_t* data, uint32_t size, AudHeader* out_header = nullptr);

#endif // AUD_FILE_H
