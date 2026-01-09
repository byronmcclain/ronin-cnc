#!/bin/bash
# Verification script for DirectX stubs

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking DirectX Stubs ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check header exists
echo "[1/4] Checking header file exists..."
if [ ! -f "include/compat/directx.h" ]; then
    echo "VERIFY_FAILED: include/compat/directx.h not found"
    exit 1
fi
echo "  OK: Header file exists"

# Step 2: Check dependency exists
echo "[2/4] Checking dependencies..."
if [ ! -f "include/compat/windows.h" ]; then
    echo "VERIFY_FAILED: Dependency include/compat/windows.h not found"
    exit 1
fi
echo "  OK: Dependencies exist"

# Step 3: Compile test program
echo "[3/4] Compiling test program..."
cat > /tmp/test_directx.cpp << 'EOF'
#include "compat/directx.h"
#include <cstdio>

int main() {
    printf("Testing DirectX stubs...\n\n");

    // Test DirectDraw creation (should fail gracefully)
    LPDIRECTDRAW lpDD = nullptr;
    HRESULT hr = DirectDrawCreate(nullptr, &lpDD, nullptr);
    if (hr == DD_OK) {
        printf("FAIL: DirectDrawCreate should not succeed\n");
        return 1;
    }
    printf("DirectDrawCreate: Failed as expected (0x%08X)\n", (unsigned)hr);

    // Test DirectSound creation (should fail gracefully)
    LPDIRECTSOUND lpDS = nullptr;
    hr = DirectSoundCreate(nullptr, &lpDS, nullptr);
    if (hr == DS_OK) {
        printf("FAIL: DirectSoundCreate should not succeed\n");
        return 1;
    }
    printf("DirectSoundCreate: Failed as expected (0x%08X)\n", (unsigned)hr);

    // Test structure sizes
    printf("\nStructure sizes:\n");
    printf("  DDSCAPS:       %zu bytes\n", sizeof(DDSCAPS));
    printf("  DDCOLORKEY:    %zu bytes\n", sizeof(DDCOLORKEY));
    printf("  DDPIXELFORMAT: %zu bytes\n", sizeof(DDPIXELFORMAT));
    printf("  DDSURFACEDESC: %zu bytes\n", sizeof(DDSURFACEDESC));

    // Test using structures
    DDSURFACEDESC ddsd;
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
    ddsd.dwWidth = 640;
    ddsd.dwHeight = 400;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    printf("\nTest DDSURFACEDESC: %ux%u\n", ddsd.dwWidth, ddsd.dwHeight);

    // Test macros
    if (!FAILED(DDERR_GENERIC)) {
        printf("FAIL: FAILED macro not working\n");
        return 1;
    }
    if (!SUCCEEDED(DD_OK)) {
        printf("FAIL: SUCCEEDED macro not working\n");
        return 1;
    }
    printf("FAILED/SUCCEEDED macros: OK\n");

    printf("\nAll DirectX stub tests passed!\n");
    return 0;
}
EOF

if ! clang++ -std=c++17 -I include /tmp/test_directx.cpp -o /tmp/test_directx 2>&1; then
    echo "VERIFY_FAILED: Test program compilation failed"
    rm -f /tmp/test_directx.cpp
    exit 1
fi
echo "  OK: Test program compiled"

# Step 4: Run test
echo "[4/4] Running test program..."
if ! /tmp/test_directx; then
    echo "VERIFY_FAILED: Test program failed"
    rm -f /tmp/test_directx.cpp /tmp/test_directx
    exit 1
fi

# Cleanup
rm -f /tmp/test_directx.cpp /tmp/test_directx

echo ""
echo "VERIFY_SUCCESS"
exit 0
