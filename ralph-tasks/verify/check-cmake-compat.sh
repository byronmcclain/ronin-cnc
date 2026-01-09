#!/bin/bash
# Verification script for CMake compatibility integration

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking CMake Compatibility Integration ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check source files exist
echo "[1/5] Checking source files..."
if [ ! -f "src/compat/compat.cpp" ]; then
    echo "VERIFY_FAILED: src/compat/compat.cpp not found"
    exit 1
fi
if [ ! -f "src/test_compat.cpp" ]; then
    echo "VERIFY_FAILED: src/test_compat.cpp not found"
    exit 1
fi
echo "  OK: Source files exist"

# Step 2: Configure CMake (if needed)
echo "[2/5] Configuring CMake..."
CMAKE_POLICY_VERSION_MINIMUM=3.5 cmake -B build -S . 2>&1 || {
    echo "VERIFY_FAILED: CMake configure failed"
    exit 1
}
echo "  OK: CMake configured"

# Step 3: Build compat library
echo "[3/5] Building compat library..."
if ! cmake --build build --target compat 2>&1; then
    echo "VERIFY_FAILED: compat library build failed"
    exit 1
fi
echo "  OK: compat library built"

# Step 4: Build TestCompat
echo "[4/5] Building TestCompat..."
if ! cmake --build build --target TestCompat 2>&1; then
    echo "VERIFY_FAILED: TestCompat build failed"
    exit 1
fi
echo "  OK: TestCompat built"

# Step 5: Run TestCompat
echo "[5/5] Running TestCompat..."
if ! ./build/TestCompat; then
    echo "VERIFY_FAILED: TestCompat failed"
    exit 1
fi

echo ""
echo "VERIFY_SUCCESS"
exit 0
