#!/bin/bash
# Verification script for Task 05d: Timing Integration Test

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Timing Integration ==="

cd "$ROOT_DIR"

# Step 1: Build the project
echo "Building project..."
if ! cmake --build build 2>&1; then
    echo "VERIFY_FAILED: CMake build failed"
    exit 1
fi

# Step 2: Check executable exists
if [ ! -f "build/RedAlert" ]; then
    echo "VERIFY_FAILED: build/RedAlert not found"
    exit 1
fi

# Step 3: Run tests
echo "Running tests..."
TEST_OUTPUT=$(./build/RedAlert --test 2>&1)

if echo "$TEST_OUTPUT" | grep -q "All tests passed"; then
    echo "All tests passed!"
else
    echo "Test output:"
    echo "$TEST_OUTPUT"
    echo "VERIFY_FAILED: Tests did not pass"
    exit 1
fi

# Step 4: Check timing test was included
if ! echo "$TEST_OUTPUT" | grep -q "Timing system"; then
    echo "VERIFY_FAILED: Timing test not found in output"
    exit 1
fi

echo "VERIFY_SUCCESS"
exit 0
