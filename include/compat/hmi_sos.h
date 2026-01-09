/**
 * HMI Sound Operating System (SOS) Compatibility Stubs
 *
 * HMI SOS was the audio library used by Westwood games in the DOS/Win95 era.
 * This header provides stub definitions that allow compilation while
 * returning "not available" for all audio operations.
 *
 * Actual audio is handled by the platform layer (rodio via Rust).
 * Game code should be migrated to use Platform_Sound_* functions.
 */

#ifndef COMPAT_HMI_SOS_H
#define COMPAT_HMI_SOS_H

#include <cstdint>

// =============================================================================
// SOS Return Codes
// =============================================================================

#define _SOS_NO_ERROR           0
#define _SOS_ERROR             -1
#define _SOS_INVALID_HANDLE    -1
#define _SOS_NO_DRIVER         -2
#define _SOS_NO_MEMORY         -3
#define _SOS_INVALID_POINTER   -4
#define _SOS_SAMPLE_PLAYING    -5
#define _SOS_SAMPLE_NOT_FOUND  -6
#define _SOS_NO_SAMPLES        -7
#define _SOS_DRIVER_LOADED     -8
#define _SOS_NOT_INITIALIZED   -9

// =============================================================================
// SOS Types
// =============================================================================

// Sample handle (like a file handle but for audio samples)
typedef int32_t HSAMPLE;
typedef int32_t HDRIVER;

// SOS callback type
typedef void (*SOSCALLBACK)(HSAMPLE);

// =============================================================================
// SOS Structures
// =============================================================================

// Sample descriptor (from original headers)
typedef struct _SOS_SAMPLE {
    void*    pSample;        // Pointer to sample data
    uint32_t wLength;        // Length in bytes
    uint16_t wRate;          // Sample rate (Hz)
    uint16_t wBitsPerSample; // 8 or 16
    uint16_t wChannels;      // 1=mono, 2=stereo
    uint16_t wVolume;        // 0-127
    int16_t  wPan;           // -128 to 127 (0=center)
    uint32_t dwFlags;        // Flags
    SOSCALLBACK pCallback;   // Completion callback
} _SOS_SAMPLE;

// Driver capabilities
typedef struct _SOS_CAPABILITIES {
    uint16_t wDeviceId;
    uint16_t wMaxSampleRate;
    uint16_t wMinSampleRate;
    uint16_t wMaxChannels;
    uint16_t wMaxBitsPerSample;
    uint32_t dwFlags;
} _SOS_CAPABILITIES;

// =============================================================================
// SOS Digital Audio Stubs
// =============================================================================

#ifdef __cplusplus
extern "C" {
#endif

// Initialization - Always returns success but does nothing
// Actual audio init is done via Platform_Audio_Init()
static inline int32_t sosDIGIInitSystem(void* pConfig, int32_t wSize) {
    (void)pConfig; (void)wSize;
    return _SOS_NO_ERROR; // Pretend success, platform layer handles audio
}

static inline int32_t sosDIGIUnInitSystem(HDRIVER hDriver) {
    (void)hDriver;
    return _SOS_NO_ERROR;
}

// Driver management - Return failure (no SOS drivers)
static inline HDRIVER sosDIGIInitDriver(int32_t wDriverId, void* pConfig) {
    (void)wDriverId; (void)pConfig;
    return _SOS_INVALID_HANDLE;
}

static inline int32_t sosDIGIUnInitDriver(HDRIVER hDriver) {
    (void)hDriver;
    return _SOS_NO_ERROR;
}

static inline int32_t sosDIGILoadDriver(void* pDriver) {
    (void)pDriver;
    return _SOS_NO_DRIVER;
}

// Sample playback - All stubbed
// In migrated code, use Platform_Sound_Play() instead
static inline HSAMPLE sosDIGIStartSample(HDRIVER hDriver, _SOS_SAMPLE* pSample) {
    (void)hDriver; (void)pSample;
    return _SOS_INVALID_HANDLE; // Sample won't play
}

static inline int32_t sosDIGIStopSample(HDRIVER hDriver, HSAMPLE hSample) {
    (void)hDriver; (void)hSample;
    return _SOS_NO_ERROR;
}

static inline int32_t sosDIGIPauseSample(HDRIVER hDriver, HSAMPLE hSample) {
    (void)hDriver; (void)hSample;
    return _SOS_NO_ERROR;
}

static inline int32_t sosDIGIResumeSample(HDRIVER hDriver, HSAMPLE hSample) {
    (void)hDriver; (void)hSample;
    return _SOS_NO_ERROR;
}

// Volume/Pan control
static inline int32_t sosDIGISetSampleVolume(HDRIVER hDriver, HSAMPLE hSample, uint16_t wVolume) {
    (void)hDriver; (void)hSample; (void)wVolume;
    return _SOS_NO_ERROR;
}

static inline uint16_t sosDIGIGetSampleVolume(HDRIVER hDriver, HSAMPLE hSample) {
    (void)hDriver; (void)hSample;
    return 127; // Max volume
}

static inline int32_t sosDIGISetSamplePan(HDRIVER hDriver, HSAMPLE hSample, int16_t wPan) {
    (void)hDriver; (void)hSample; (void)wPan;
    return _SOS_NO_ERROR;
}

static inline int16_t sosDIGIGetSamplePan(HDRIVER hDriver, HSAMPLE hSample) {
    (void)hDriver; (void)hSample;
    return 0; // Center
}

// Query functions
static inline int32_t sosDIGISamplesPlaying(HDRIVER hDriver) {
    (void)hDriver;
    return 0; // No samples playing
}

static inline int32_t sosDIGISampleDone(HDRIVER hDriver, HSAMPLE hSample) {
    (void)hDriver; (void)hSample;
    return 1; // Sample is done (never started)
}

static inline void* sosDIGIGetSamplePosition(HDRIVER hDriver, HSAMPLE hSample) {
    (void)hDriver; (void)hSample;
    return nullptr;
}

// Capabilities
static inline int32_t sosDIGIGetCaps(HDRIVER hDriver, _SOS_CAPABILITIES* pCaps) {
    (void)hDriver;
    if (pCaps) {
        pCaps->wDeviceId = 0;
        pCaps->wMaxSampleRate = 44100;
        pCaps->wMinSampleRate = 11025;
        pCaps->wMaxChannels = 2;
        pCaps->wMaxBitsPerSample = 16;
        pCaps->dwFlags = 0;
    }
    return _SOS_NO_ERROR;
}

// Master volume
static inline int32_t sosDIGISetMasterVolume(HDRIVER hDriver, uint16_t wVolume) {
    (void)hDriver; (void)wVolume;
    return _SOS_NO_ERROR;
}

static inline uint16_t sosDIGIGetMasterVolume(HDRIVER hDriver) {
    (void)hDriver;
    return 127;
}

#ifdef __cplusplus
}
#endif

// =============================================================================
// Macro-based stubs for common patterns
// =============================================================================
// Some code uses macro patterns - provide them for compatibility

#define SOS_DIGI_INIT(cfg) sosDIGIInitSystem(cfg, sizeof(cfg))
#define SOS_DIGI_UNINIT() sosDIGIUnInitSystem(0)
#define SOS_DIGI_PLAY(sample) sosDIGIStartSample(0, sample)
#define SOS_DIGI_STOP(handle) sosDIGIStopSample(0, handle)

// =============================================================================
// End of HMI SOS Compatibility Header
// =============================================================================

#endif // COMPAT_HMI_SOS_H
