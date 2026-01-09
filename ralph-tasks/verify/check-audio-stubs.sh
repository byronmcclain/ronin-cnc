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
