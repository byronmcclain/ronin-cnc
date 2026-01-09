#!/bin/bash
# Verification script for Windows types header

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Windows Types Header ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check header exists
echo "[1/4] Checking header file exists..."
if [ ! -f "include/compat/windows.h" ]; then
    echo "VERIFY_FAILED: include/compat/windows.h not found"
    exit 1
fi
echo "  OK: Header file exists"

# Step 2: Syntax check
echo "[2/4] Checking syntax..."
if ! clang++ -std=c++17 -fsyntax-only -I include include/compat/windows.h 2>&1; then
    echo "VERIFY_FAILED: Header has syntax errors"
    exit 1
fi
echo "  OK: Syntax valid"

# Step 3: Compile test program
echo "[3/4] Compiling test program..."
cat > /tmp/test_windows_types.cpp << 'EOF'
#include "compat/windows.h"
#include <cstdio>

int main() {
    // Test type sizes
    printf("BOOL:   %zu bytes (expected 4)\n", sizeof(BOOL));
    printf("DWORD:  %zu bytes (expected 4)\n", sizeof(DWORD));
    printf("WORD:   %zu bytes (expected 2)\n", sizeof(WORD));
    printf("BYTE:   %zu bytes (expected 1)\n", sizeof(BYTE));
    printf("POINT:  %zu bytes (expected 8)\n", sizeof(POINT));
    printf("RECT:   %zu bytes (expected 16)\n", sizeof(RECT));

    // Test constants
    if (TRUE != 1) return 1;
    if (FALSE != 0) return 1;

    // Test structures
    POINT pt = {100, 200};
    if (pt.x != 100 || pt.y != 200) return 1;

    RECT rc = {0, 0, 640, 400};
    if (rc.right != 640 || rc.bottom != 400) return 1;

    // Test macros
    DWORD dw = MAKELONG(0x1234, 0x5678);
    if (LOWORD(dw) != 0x1234) return 1;
    if (HIWORD(dw) != 0x5678) return 1;

    DWORD color = RGB(255, 128, 64);
    if (GetRValue(color) != 255) return 1;
    if (GetGValue(color) != 128) return 1;
    if (GetBValue(color) != 64) return 1;

    printf("\nAll type tests passed!\n");
    return 0;
}
EOF

if ! clang++ -std=c++17 -I include /tmp/test_windows_types.cpp -o /tmp/test_windows_types 2>&1; then
    echo "VERIFY_FAILED: Test program compilation failed"
    rm -f /tmp/test_windows_types.cpp
    exit 1
fi
echo "  OK: Test program compiled"

# Step 4: Run test
echo "[4/4] Running test program..."
if ! /tmp/test_windows_types; then
    echo "VERIFY_FAILED: Test program failed"
    rm -f /tmp/test_windows_types.cpp /tmp/test_windows_types
    exit 1
fi

# Cleanup
rm -f /tmp/test_windows_types.cpp /tmp/test_windows_types

echo ""
echo "VERIFY_SUCCESS"
exit 0
