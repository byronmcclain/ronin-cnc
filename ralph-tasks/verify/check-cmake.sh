#!/bin/bash
# Verification script for CMake configuration

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking CMake Configuration ==="

cd "$ROOT_DIR"

# Clean previous build
rm -rf build

# Configure with Ninja
if ! cmake -B build -G Ninja 2>&1; then
    echo "VERIFY_FAILED: CMake configuration failed"
    exit 1
fi

# Check build directory exists
if [ ! -d build ]; then
    echo "VERIFY_FAILED: build directory not created"
    exit 1
fi

# Check Ninja files exist
if [ ! -f build/build.ninja ]; then
    echo "VERIFY_FAILED: Ninja build files not generated"
    exit 1
fi

echo "VERIFY_SUCCESS"
exit 0
