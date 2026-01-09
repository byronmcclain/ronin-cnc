#!/bin/bash
# Verification script for original code compilation test

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Original Code Compilation ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check test file exists
echo "[1/4] Checking test file..."
if [ ! -f "src/test_original_compile.cpp" ]; then
    echo "VERIFY_FAILED: src/test_original_compile.cpp not found"
    exit 1
fi
echo "  OK: Test file exists"

# Step 2: Configure CMake
echo "[2/4] Configuring CMake..."
CMAKE_POLICY_VERSION_MINIMUM=3.5 cmake -B build -S . 2>&1 || {
    echo "VERIFY_FAILED: CMake configure failed"
    exit 1
}
echo "  OK: CMake configured"

# Step 3: Build TestOriginalCompile
echo "[3/4] Building TestOriginalCompile..."
if ! cmake --build build --target TestOriginalCompile 2>&1; then
    echo "VERIFY_FAILED: TestOriginalCompile build failed"
    exit 1
fi
echo "  OK: TestOriginalCompile built"

# Step 4: Run test
echo "[4/4] Running TestOriginalCompile..."
if ! ./build/TestOriginalCompile; then
    echo "VERIFY_FAILED: TestOriginalCompile execution failed"
    exit 1
fi

echo ""
echo "VERIFY_SUCCESS"
exit 0
