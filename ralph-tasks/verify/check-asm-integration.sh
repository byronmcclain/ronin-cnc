#!/bin/bash
# Verification script for Task 09h: Assembly Replacement Integration Test

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Assembly Replacement Integration ==="

cd "$ROOT_DIR"

# Step 1: Check test file exists
echo "Checking test_asm.cpp exists..."
if [ ! -f "src/test_asm.cpp" ]; then
    echo "VERIFY_FAILED: src/test_asm.cpp not found"
    exit 1
fi

# Step 2: Build platform library
echo "Building platform library..."
if ! cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1; then
    echo "VERIFY_FAILED: Cargo build failed"
    exit 1
fi

# Step 3: Configure CMake (use default generator if Ninja not available)
echo "Configuring CMake..."
rm -rf build
if ! cmake -B build 2>&1; then
    echo "VERIFY_FAILED: CMake configuration failed"
    exit 1
fi

# Step 4: Build TestAsm executable
echo "Building TestAsm..."
if ! cmake --build build --target TestAsm 2>&1; then
    echo "VERIFY_FAILED: TestAsm build failed"
    exit 1
fi

# Step 5: Check TestAsm executable exists
echo "Checking TestAsm executable..."
if [ ! -f "build/TestAsm" ]; then
    echo "VERIFY_FAILED: build/TestAsm not found"
    exit 1
fi

# Step 6: Run TestAsm
echo "Running TestAsm..."
if ! ./build/TestAsm 2>&1; then
    echo "VERIFY_FAILED: TestAsm execution failed"
    exit 1
fi

# Step 7: Run all Rust unit tests for assembly replacements
echo "Running Rust unit tests..."
if ! cargo test --manifest-path platform/Cargo.toml 2>&1; then
    echo "VERIFY_FAILED: Rust unit tests failed"
    exit 1
fi

echo "VERIFY_SUCCESS"
exit 0
