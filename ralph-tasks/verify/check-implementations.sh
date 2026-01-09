#!/bin/bash
# Verification script for compat implementations

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Compatibility Implementations ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check files exist
echo "[1/4] Checking source file exists..."
if [ ! -f "src/compat/compat.cpp" ]; then
    echo "VERIFY_FAILED: src/compat/compat.cpp not found"
    exit 1
fi
echo "  OK: Source file exists"

# Step 2: Syntax check (with headers only mode)
echo "[2/4] Checking syntax..."
if ! clang++ -std=c++17 -fsyntax-only -DCOMPAT_HEADERS_ONLY -I include src/compat/compat.cpp 2>&1; then
    echo "VERIFY_FAILED: Implementation has syntax errors"
    exit 1
fi
echo "  OK: Syntax valid"

# Step 3: Compile test program
echo "[3/4] Compiling test program..."
mkdir -p /tmp/compat_test

cat > /tmp/compat_test/test.cpp << 'EOF'
#define COMPAT_HEADERS_ONLY
#include "compat/compat.h"
#include <cstdio>
#include <cstring>

// Forward declare the functions we're testing
extern "C" {
    void Compat_NormalizePath(char* path);
    const char* Compat_GetExtension(const char* path);
    const char* Compat_GetFilename(const char* path);
    BOOL Compat_IntersectRect(LPRECT lprcDst, const RECT* lprcSrc1, const RECT* lprcSrc2);
    int Compat_Init(void);
    void Compat_Shutdown(void);
}

int main() {
    printf("Testing compatibility implementations...\n\n");

    // Test init
    if (Compat_Init() != 0) {
        printf("FAIL: Compat_Init failed\n");
        return 1;
    }
    printf("Compat_Init: OK\n");

    // Test path normalization
    char path1[] = "DATA\\CONQUER\\MAPS\\SCG01EA.INI";
    Compat_NormalizePath(path1);
    printf("NormalizePath: %s\n", path1);
    if (strchr(path1, '\\') != nullptr) {
        printf("FAIL: Backslashes not converted\n");
        return 1;
    }

    // Test extension
    const char* ext = Compat_GetExtension("FILE.MIX");
    printf("GetExtension(FILE.MIX): %s\n", ext);
    if (strcmp(ext, "MIX") != 0) {
        printf("FAIL: Expected 'MIX', got '%s'\n", ext);
        return 1;
    }

    // Test filename
    const char* fname = Compat_GetFilename("/path/to/file.txt");
    printf("GetFilename(/path/to/file.txt): %s\n", fname);
    if (strcmp(fname, "file.txt") != 0) {
        printf("FAIL: Expected 'file.txt', got '%s'\n", fname);
        return 1;
    }

    // Test rect intersection
    RECT r1 = {0, 0, 100, 100};
    RECT r2 = {50, 50, 150, 150};
    RECT result;
    BOOL intersects = Compat_IntersectRect(&result, &r1, &r2);
    printf("IntersectRect: intersects=%d, result=(%ld,%ld,%ld,%ld)\n",
           intersects, result.left, result.top, result.right, result.bottom);
    if (!intersects || result.left != 50 || result.top != 50 ||
        result.right != 100 || result.bottom != 100) {
        printf("FAIL: Intersection incorrect\n");
        return 1;
    }

    Compat_Shutdown();
    printf("\nAll implementation tests passed!\n");
    return 0;
}
EOF

# Compile implementation and test
if ! clang++ -std=c++17 -DCOMPAT_HEADERS_ONLY -I include \
    src/compat/compat.cpp /tmp/compat_test/test.cpp -o /tmp/compat_test/test 2>&1; then
    echo "VERIFY_FAILED: Test program compilation failed"
    rm -rf /tmp/compat_test
    exit 1
fi
echo "  OK: Test program compiled"

# Step 4: Run test
echo "[4/4] Running test program..."
if ! /tmp/compat_test/test; then
    echo "VERIFY_FAILED: Test program failed"
    rm -rf /tmp/compat_test
    exit 1
fi

# Cleanup
rm -rf /tmp/compat_test

echo ""
echo "VERIFY_SUCCESS"
exit 0
