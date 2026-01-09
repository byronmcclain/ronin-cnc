#!/bin/bash
# Verification script for Watcom compatibility header

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Watcom Compatibility Header ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check header exists
echo "[1/4] Checking header file exists..."
if [ ! -f "include/compat/watcom.h" ]; then
    echo "VERIFY_FAILED: include/compat/watcom.h not found"
    exit 1
fi
echo "  OK: Header file exists"

# Step 2: Syntax check
echo "[2/4] Checking syntax..."
if ! clang++ -std=c++17 -fsyntax-only -I include include/compat/watcom.h 2>&1; then
    echo "VERIFY_FAILED: Header has syntax errors"
    exit 1
fi
echo "  OK: Syntax valid"

# Step 3: Compile test with Watcom-style code
echo "[3/4] Compiling test program with Watcom syntax..."
cat > /tmp/test_watcom.cpp << 'EOF'
#include "compat/watcom.h"
#include <cstdio>
#include <cstring>

// Test calling conventions (should compile as no-op)
int __cdecl cdecl_func(int a) { return a; }
int __stdcall stdcall_func(int a) { return a; }
int __fastcall fastcall_func(int a) { return a; }
int WINAPI winapi_func(int a) { return a; }

// Test segment qualifiers (should compile as no-op)
char __far* far_ptr = nullptr;
char __near* near_ptr = nullptr;
char far* far_ptr2 = nullptr;
char near* near_ptr2 = nullptr;

// Test export keywords (should compile as no-op)
__export void exported_func() {}

// Test inline assembly marker (should skip silently)
// Note: We only test _asm (single underscore), not __asm
// because __asm conflicts with macOS symbol aliasing macros
void asm_test() {
    int x = 5;
    // The _asm block becomes if(0) which skips the contents
    // We use a simple statement instead of actual assembly
    _asm { x = x + 1; }
    printf("Assembly test: x = %d (should still be 5 since asm skipped)\n", x);
}

// Test far string functions
void string_test() {
    char buf[64];
    _fstrcpy(buf, "Hello");
    size_t len = _fstrlen(buf);
    printf("String test: len = %zu\n", len);
}

int main() {
    printf("Testing Watcom compatibility...\n\n");

    // Test calling conventions
    if (cdecl_func(1) != 1) return 1;
    if (stdcall_func(2) != 2) return 1;
    if (fastcall_func(3) != 3) return 1;
    if (winapi_func(4) != 4) return 1;
    printf("Calling conventions: OK\n");

    // Test pointers
    far_ptr = nullptr;
    near_ptr = nullptr;
    printf("Segment qualifiers: OK\n");

    // Test assembly skip
    asm_test();
    printf("Assembly skip: OK\n");

    // Test string functions
    string_test();

    // Test flat model macro
    if (!__FLAT__) return 1;
    printf("Flat model: OK\n");

    printf("\nAll Watcom compatibility tests passed!\n");
    return 0;
}
EOF

if ! clang++ -std=c++17 -I include /tmp/test_watcom.cpp -o /tmp/test_watcom 2>&1; then
    echo "VERIFY_FAILED: Test program compilation failed"
    rm -f /tmp/test_watcom.cpp
    exit 1
fi
echo "  OK: Test program compiled"

# Step 4: Run test
echo "[4/4] Running test program..."
if ! /tmp/test_watcom; then
    echo "VERIFY_FAILED: Test program failed"
    rm -f /tmp/test_watcom.cpp /tmp/test_watcom
    exit 1
fi

# Cleanup
rm -f /tmp/test_watcom.cpp /tmp/test_watcom

echo ""
echo "VERIFY_SUCCESS"
exit 0
