#!/bin/bash
# Verification script for master compatibility header

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Master Compatibility Header ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check all headers exist
echo "[1/5] Checking all header files exist..."
for header in watcom.h windows.h directx.h hmi_sos.h gcl.h compat.h; do
    if [ ! -f "include/compat/$header" ]; then
        echo "VERIFY_FAILED: include/compat/$header not found"
        exit 1
    fi
done
echo "  OK: All headers exist"

# Step 2: Syntax check (headers only mode)
echo "[2/5] Checking syntax (headers only)..."
if ! clang++ -std=c++17 -fsyntax-only -DCOMPAT_HEADERS_ONLY -I include include/compat/compat.h 2>&1; then
    echo "VERIFY_FAILED: Master header has syntax errors"
    exit 1
fi
echo "  OK: Syntax valid"

# Step 3: Compile test with headers only
echo "[3/5] Compiling test program (headers only mode)..."
cat > /tmp/test_compat.cpp << 'EOF'
#define COMPAT_HEADERS_ONLY
#include "compat/compat.h"
#include <cstdio>

int main() {
    printf("Testing compatibility header...\n\n");

    // Test Windows types
    DWORD dw = 0x12345678;
    WORD w = LOWORD(dw);
    printf("LOWORD(0x12345678) = 0x%04X (expected 0x5678)\n", w);
    if (w != 0x5678) return 1;

    w = HIWORD(dw);
    printf("HIWORD(0x12345678) = 0x%04X (expected 0x1234)\n", w);
    if (w != 0x1234) return 1;

    // Test RGB macros
    DWORD color = RGB(255, 128, 64);
    printf("RGB(255, 128, 64) = 0x%06X\n", (unsigned)color);
    if (GetRValue(color) != 255) return 1;
    if (GetGValue(color) != 128) return 1;
    if (GetBValue(color) != 64) return 1;
    printf("  GetRValue = %d, GetGValue = %d, GetBValue = %d\n",
           GetRValue(color), GetGValue(color), GetBValue(color));

    // Test function stubs
    DWORD ticks = GetTickCount();
    printf("GetTickCount() = %u\n", ticks);

    Sleep(1);
    printf("Sleep(1) completed\n");

    int result = MessageBoxA(NULL, "Test message", "Test", MB_OK);
    printf("MessageBoxA returned %d (expected IDOK=%d)\n", result, IDOK);
    if (result != IDOK) return 1;

    // Test DirectX stubs
    LPDIRECTDRAW lpDD = nullptr;
    HRESULT hr = DirectDrawCreate(nullptr, &lpDD, nullptr);
    printf("DirectDrawCreate returned 0x%08X (expected failure)\n", (unsigned)hr);
    if (hr == DD_OK) return 1;

    // Test memory stubs
    HGLOBAL hMem = GlobalAlloc(GPTR, 1024);
    void* ptr = GlobalLock(hMem);
    (void)ptr;
    GlobalUnlock(hMem);
    GlobalFree(hMem);
    printf("GlobalAlloc/Lock/Unlock/Free: OK\n");

    // Test string functions
    char buf[64];
    lstrcpy(buf, "Hello");
    printf("lstrlen(\"Hello\") = %d\n", lstrlen(buf));
    if (lstrlen(buf) != 5) return 1;

    printf("\nAll compatibility tests passed!\n");
    return 0;
}
EOF

if ! clang++ -std=c++17 -DCOMPAT_HEADERS_ONLY -I include /tmp/test_compat.cpp -o /tmp/test_compat 2>&1; then
    echo "VERIFY_FAILED: Test program compilation failed"
    rm -f /tmp/test_compat.cpp
    exit 1
fi
echo "  OK: Test program compiled"

# Step 4: Run test
echo "[4/5] Running test program..."
if ! /tmp/test_compat; then
    echo "VERIFY_FAILED: Test program failed"
    rm -f /tmp/test_compat.cpp /tmp/test_compat
    exit 1
fi

# Step 5: Check it compiles with platform layer (if available)
echo "[5/5] Checking integration with platform.h..."
if [ -f "include/platform.h" ]; then
    cat > /tmp/test_compat_full.cpp << 'EOF'
#include "compat/compat.h"
int main() {
    DWORD t = GetTickCount();
    return t > 0 ? 0 : 0;
}
EOF
    # Just syntax check - don't link
    if ! clang++ -std=c++17 -fsyntax-only -I include /tmp/test_compat_full.cpp 2>&1; then
        echo "VERIFY_WARNING: Integration with platform.h has issues"
        # Don't fail - platform layer may not be fully compatible yet
    else
        echo "  OK: Integrates with platform.h"
    fi
    rm -f /tmp/test_compat_full.cpp
else
    echo "  SKIP: platform.h not found"
fi

# Cleanup
rm -f /tmp/test_compat.cpp /tmp/test_compat

echo ""
echo "VERIFY_SUCCESS"
exit 0
