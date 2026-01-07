#!/bin/bash
# Verification script for Task 08d: Audio Integration Test

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Audio Integration ==="

cd "$ROOT_DIR"

# Step 1: Build platform library
echo "Building platform library..."
if ! cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1; then
    echo "VERIFY_FAILED: Cargo build failed"
    exit 1
fi

# Step 2: Check test file exists
echo "Checking test file..."
if [ ! -f "src/test_audio.cpp" ]; then
    echo "VERIFY_FAILED: src/test_audio.cpp not found"
    exit 1
fi

# Step 3: Configure CMake
echo "Configuring CMake..."
if ! cmake -B build -G Ninja 2>&1; then
    echo "VERIFY_FAILED: CMake configuration failed"
    exit 1
fi

# Step 4: Build test executable
echo "Building TestAudio..."
if ! cmake --build build --target TestAudio 2>&1; then
    echo "VERIFY_FAILED: TestAudio build failed"
    exit 1
fi

# Step 5: Check executable exists
if [ ! -f "build/TestAudio" ]; then
    echo "VERIFY_FAILED: build/TestAudio not found"
    exit 1
fi

# Step 6: Run the test
echo "Running audio integration test..."
if ! ./build/TestAudio; then
    echo "VERIFY_FAILED: Audio integration test failed"
    exit 1
fi

echo "VERIFY_SUCCESS"
exit 0
