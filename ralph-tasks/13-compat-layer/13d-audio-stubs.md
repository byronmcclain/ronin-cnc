# Task 13d: Audio Library Stubs

## Dependencies
- Task 13a (Windows Types) must be complete

## Context
Red Alert originally used two proprietary audio libraries:
1. **HMI SOS (Sound Operating System)** - For digital audio playback
2. **Greenleaf Communications Library (GCL)** - For modem/serial multiplayer

Neither library is available today, and we're replacing their functionality:
- Audio: Platform layer (rodio via Rust)
- Networking: enet via platform layer (TCP/UDP, not serial)

This task creates stub headers that allow the original code to compile while returning "not available" for all operations.

## Objective
Create two stub headers:
1. `include/compat/hmi_sos.h` - HMI Sound Operating System stubs
2. `include/compat/gcl.h` - Greenleaf Communications Library stubs

## Technical Background

### HMI SOS Overview
HMI SOS was a DOS/Windows audio library that handled:
- Digital audio (PCM) playback
- Sample mixing
- Volume control

The original code calls functions like:
```cpp
sosDIGIInitSystem(...)     // Initialize audio
sosDIGIStartSample(...)    // Play a sample
sosDIGIStopSample(...)     // Stop a sample
sosDIGISetSampleVolume(...)// Set volume
```

### GCL Overview
GCL was a communications library for serial/modem connections:
- Dial modem
- Answer incoming calls
- Send/receive data over serial port

This is completely obsolete - modern multiplayer uses TCP/IP.

## Deliverables
- [ ] `include/compat/hmi_sos.h` - HMI SOS audio stubs
- [ ] `include/compat/gcl.h` - GCL communications stubs
- [ ] All stubs return failure/no-op
- [ ] Verification script passes

## Files to Create

### include/compat/hmi_sos.h (NEW)
```cpp
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
```

### include/compat/gcl.h (NEW)
```cpp
/**
 * Greenleaf Communications Library (GCL) Compatibility Stubs
 *
 * GCL was a serial/modem communications library used for multiplayer
 * in the DOS era. This functionality is completely obsolete.
 *
 * Modern multiplayer uses TCP/IP via enet (platform layer).
 * All GCL functions return failure to indicate "modem not available".
 */

#ifndef COMPAT_GCL_H
#define COMPAT_GCL_H

#include <cstdint>

// =============================================================================
// GCL Return Codes
// =============================================================================

#define GCL_SUCCESS          0
#define GCL_ERROR           -1
#define GCL_NO_CARRIER      -2
#define GCL_NO_MODEM        -3
#define GCL_BUSY            -4
#define GCL_NO_DIALTONE     -5
#define GCL_TIMEOUT         -6
#define GCL_INVALID_PORT    -7
#define GCL_PORT_IN_USE     -8
#define GCL_NO_MEMORY       -9
#define GCL_NOT_INITIALIZED -10

// =============================================================================
// GCL Types
// =============================================================================

typedef int32_t GCL_HANDLE;
typedef int32_t GCL_PORT;

// Port identifiers
#define GCL_COM1    0
#define GCL_COM2    1
#define GCL_COM3    2
#define GCL_COM4    3

// Baud rates
#define GCL_BAUD_300    300
#define GCL_BAUD_1200   1200
#define GCL_BAUD_2400   2400
#define GCL_BAUD_9600   9600
#define GCL_BAUD_14400  14400
#define GCL_BAUD_19200  19200
#define GCL_BAUD_28800  28800
#define GCL_BAUD_38400  38400
#define GCL_BAUD_57600  57600

// =============================================================================
// GCL Structures
// =============================================================================

typedef struct _GCL_CONFIG {
    GCL_PORT port;           // COM port
    int32_t  baudRate;       // Baud rate
    int32_t  dataBits;       // Data bits (7 or 8)
    int32_t  stopBits;       // Stop bits (1 or 2)
    int32_t  parity;         // Parity (0=none, 1=odd, 2=even)
    int32_t  flowControl;    // Flow control (0=none, 1=hardware, 2=software)
} GCL_CONFIG;

typedef struct _GCL_MODEM_INIT {
    char* initString;        // Modem init string (e.g., "ATZ")
    char* dialPrefix;        // Dial prefix (e.g., "ATDT")
    char* hangupString;      // Hangup string (e.g., "ATH0")
} GCL_MODEM_INIT;

// =============================================================================
// GCL Function Stubs
// =============================================================================
// All functions return failure - modem/serial is not supported

#ifdef __cplusplus
extern "C" {
#endif

// Initialization - Always fails
static inline int32_t gcl_init(void) {
    return GCL_NOT_INITIALIZED;
}

static inline int32_t gcl_shutdown(void) {
    return GCL_SUCCESS; // OK to "shut down" nothing
}

// Port management - Always fails
static inline GCL_HANDLE gcl_open_port(GCL_PORT port, GCL_CONFIG* config) {
    (void)port; (void)config;
    return GCL_NO_MODEM;
}

static inline int32_t gcl_close_port(GCL_HANDLE handle) {
    (void)handle;
    return GCL_SUCCESS;
}

static inline int32_t gcl_set_config(GCL_HANDLE handle, GCL_CONFIG* config) {
    (void)handle; (void)config;
    return GCL_ERROR;
}

// Modem operations - Always fail
static inline int32_t gcl_modem_init(GCL_HANDLE handle, GCL_MODEM_INIT* init) {
    (void)handle; (void)init;
    return GCL_NO_MODEM;
}

static inline int32_t gcl_dial(GCL_HANDLE handle, const char* phoneNumber) {
    (void)handle; (void)phoneNumber;
    return GCL_NO_MODEM;
}

static inline int32_t gcl_answer(GCL_HANDLE handle) {
    (void)handle;
    return GCL_NO_MODEM;
}

static inline int32_t gcl_hangup(GCL_HANDLE handle) {
    (void)handle;
    return GCL_SUCCESS; // OK to "hang up" nothing
}

static inline int32_t gcl_get_carrier(GCL_HANDLE handle) {
    (void)handle;
    return 0; // No carrier
}

// Data transfer - Always fail
static inline int32_t gcl_send(GCL_HANDLE handle, const void* data, int32_t length) {
    (void)handle; (void)data; (void)length;
    return GCL_ERROR;
}

static inline int32_t gcl_receive(GCL_HANDLE handle, void* buffer, int32_t maxLength) {
    (void)handle; (void)buffer; (void)maxLength;
    return 0; // No data received
}

static inline int32_t gcl_bytes_available(GCL_HANDLE handle) {
    (void)handle;
    return 0; // No data available
}

static inline int32_t gcl_flush(GCL_HANDLE handle) {
    (void)handle;
    return GCL_SUCCESS;
}

// Status
static inline int32_t gcl_get_error(GCL_HANDLE handle) {
    (void)handle;
    return GCL_NO_MODEM;
}

static inline const char* gcl_error_string(int32_t error) {
    switch (error) {
        case GCL_SUCCESS:          return "Success";
        case GCL_ERROR:            return "Error";
        case GCL_NO_CARRIER:       return "No carrier";
        case GCL_NO_MODEM:         return "Modem not available";
        case GCL_BUSY:             return "Line busy";
        case GCL_NO_DIALTONE:      return "No dial tone";
        case GCL_TIMEOUT:          return "Timeout";
        case GCL_INVALID_PORT:     return "Invalid port";
        case GCL_PORT_IN_USE:      return "Port in use";
        case GCL_NO_MEMORY:        return "Out of memory";
        case GCL_NOT_INITIALIZED:  return "Not initialized";
        default:                   return "Unknown error";
    }
}

#ifdef __cplusplus
}
#endif

// =============================================================================
// Macro stubs for common patterns
// =============================================================================

#define GCL_INIT()           gcl_init()
#define GCL_DIAL(num)        GCL_NO_MODEM
#define GCL_ANSWER()         GCL_NO_MODEM
#define GCL_HANGUP()         GCL_SUCCESS
#define GCL_SEND(d, l)       GCL_ERROR
#define GCL_RECEIVE(b, l)    0

// =============================================================================
// End of GCL Compatibility Header
// =============================================================================

#endif // COMPAT_GCL_H
```

## Verification Command
```bash
ralph-tasks/verify/check-audio-stubs.sh
```

## Verification Script

### ralph-tasks/verify/check-audio-stubs.sh (NEW)
```bash
#!/bin/bash
# Verification script for audio library stubs

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Audio Library Stubs ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check headers exist
echo "[1/4] Checking header files exist..."
if [ ! -f "include/compat/hmi_sos.h" ]; then
    echo "VERIFY_FAILED: include/compat/hmi_sos.h not found"
    exit 1
fi
if [ ! -f "include/compat/gcl.h" ]; then
    echo "VERIFY_FAILED: include/compat/gcl.h not found"
    exit 1
fi
echo "  OK: Header files exist"

# Step 2: Syntax check both headers
echo "[2/4] Checking syntax..."
if ! clang++ -std=c++17 -fsyntax-only -I include include/compat/hmi_sos.h 2>&1; then
    echo "VERIFY_FAILED: hmi_sos.h has syntax errors"
    exit 1
fi
if ! clang++ -std=c++17 -fsyntax-only -I include include/compat/gcl.h 2>&1; then
    echo "VERIFY_FAILED: gcl.h has syntax errors"
    exit 1
fi
echo "  OK: Syntax valid"

# Step 3: Compile test program
echo "[3/4] Compiling test program..."
cat > /tmp/test_audio_stubs.cpp << 'EOF'
#include "compat/hmi_sos.h"
#include "compat/gcl.h"
#include <cstdio>

int main() {
    printf("Testing audio library stubs...\n\n");

    // Test HMI SOS
    printf("=== HMI SOS Tests ===\n");

    int32_t result = sosDIGIInitSystem(nullptr, 0);
    printf("sosDIGIInitSystem: %d (expected 0)\n", result);

    HSAMPLE sample = sosDIGIStartSample(0, nullptr);
    printf("sosDIGIStartSample: %d (expected -1)\n", sample);

    int32_t playing = sosDIGISamplesPlaying(0);
    printf("sosDIGISamplesPlaying: %d (expected 0)\n", playing);

    uint16_t vol = sosDIGIGetMasterVolume(0);
    printf("sosDIGIGetMasterVolume: %u (expected 127)\n", vol);

    // Test GCL
    printf("\n=== GCL Tests ===\n");

    int32_t gcl_result = gcl_init();
    printf("gcl_init: %d (expected %d)\n", gcl_result, GCL_NOT_INITIALIZED);

    GCL_HANDLE h = gcl_open_port(GCL_COM1, nullptr);
    printf("gcl_open_port: %d (expected %d)\n", h, GCL_NO_MODEM);

    int32_t dial = gcl_dial(0, "555-1234");
    printf("gcl_dial: %d (expected %d)\n", dial, GCL_NO_MODEM);

    const char* err = gcl_error_string(GCL_NO_MODEM);
    printf("gcl_error_string: \"%s\"\n", err);

    // Verify all stubs return expected values
    if (result != _SOS_NO_ERROR) return 1;
    if (sample != _SOS_INVALID_HANDLE) return 1;
    if (playing != 0) return 1;
    if (gcl_result != GCL_NOT_INITIALIZED) return 1;
    if (h != GCL_NO_MODEM) return 1;

    printf("\nAll audio library stub tests passed!\n");
    return 0;
}
EOF

if ! clang++ -std=c++17 -I include /tmp/test_audio_stubs.cpp -o /tmp/test_audio_stubs 2>&1; then
    echo "VERIFY_FAILED: Test program compilation failed"
    rm -f /tmp/test_audio_stubs.cpp
    exit 1
fi
echo "  OK: Test program compiled"

# Step 4: Run test
echo "[4/4] Running test program..."
if ! /tmp/test_audio_stubs; then
    echo "VERIFY_FAILED: Test program failed"
    rm -f /tmp/test_audio_stubs.cpp /tmp/test_audio_stubs
    exit 1
fi

# Cleanup
rm -f /tmp/test_audio_stubs.cpp /tmp/test_audio_stubs

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Create `include/compat/hmi_sos.h`
2. Create `include/compat/gcl.h`
3. Make verification script executable
4. Run verification script
5. Fix any issues and repeat

## Success Criteria
- Both headers compile without errors
- SOS init returns success (pretend audio is available)
- SOS playback returns invalid handle (samples won't play)
- GCL functions return appropriate failure codes
- Test program runs successfully

## Common Issues

### HMI SOS init should succeed
- Return _SOS_NO_ERROR so game code thinks audio is initialized
- Individual sample playback fails, but init appears to work
- This prevents "audio init failed" error messages

### GCL should always fail
- There's no modem to emulate
- All operations return failure
- This is expected - multiplayer will use enet instead

## Completion Promise
When verification passes, output:
```
<promise>TASK_13D_COMPLETE</promise>
```

## Escape Hatch
If stuck after 10 iterations:
- Document what's blocking in `ralph-tasks/blocked/13d.md`
- Output: `<promise>TASK_13D_BLOCKED</promise>`

## Max Iterations
10
