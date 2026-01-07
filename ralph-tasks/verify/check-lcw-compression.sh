#!/bin/bash
# Verification script for Task 09f: LCW Compression

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking LCW Compression ==="

cd "$ROOT_DIR"

# Step 1: Check lcw module exists
echo "Checking lcw module..."
if [ ! -f "platform/src/compression/lcw.rs" ]; then
    echo "VERIFY_FAILED: platform/src/compression/lcw.rs not found"
    exit 1
fi

# Step 2: Check compression module structure
echo "Checking compression module structure..."
if [ ! -f "platform/src/compression/mod.rs" ]; then
    echo "VERIFY_FAILED: platform/src/compression/mod.rs not found"
    exit 1
fi

# Step 3: Check lib.rs has compression module
echo "Checking lib.rs includes compression..."
if ! grep -q "pub mod compression" platform/src/lib.rs; then
    echo "VERIFY_FAILED: compression module not exported in lib.rs"
    exit 1
fi

# Step 4: Run LCW unit tests
echo "Running LCW unit tests..."
if ! cargo test --manifest-path platform/Cargo.toml -- compression::lcw 2>&1; then
    echo "VERIFY_FAILED: LCW unit tests failed"
    exit 1
fi

# Step 5: Build with cbindgen
echo "Building platform library..."
if ! cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1; then
    echo "VERIFY_FAILED: Cargo build failed"
    exit 1
fi

# Step 6: Check header has FFI exports
echo "Checking header for LCW operations..."
if ! grep -q "Platform_LCW_Decompress" include/platform.h; then
    echo "VERIFY_FAILED: Platform_LCW_Decompress not in header"
    exit 1
fi

if ! grep -q "Platform_LCW_Compress" include/platform.h; then
    echo "VERIFY_FAILED: Platform_LCW_Compress not in header"
    exit 1
fi

# Step 7: Run round-trip test if available
echo "Checking round-trip compression..."
if ! cargo test --manifest-path platform/Cargo.toml -- lcw_round_trip 2>&1; then
    echo "VERIFY_FAILED: LCW round-trip test failed"
    exit 1
fi

echo "VERIFY_SUCCESS"
exit 0
