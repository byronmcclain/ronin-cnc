#!/bin/bash
# Verification script for Task 08c: ADPCM Decoder

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking ADPCM Decoder ==="

cd "$ROOT_DIR"

# Step 1: Check adpcm module exists
echo "Checking ADPCM module..."
if [ ! -f "platform/src/audio/adpcm.rs" ]; then
    echo "VERIFY_FAILED: platform/src/audio/adpcm.rs not found"
    exit 1
fi

# Step 2: Run ADPCM unit tests
echo "Running ADPCM unit tests..."
if ! cargo test --manifest-path platform/Cargo.toml -- adpcm 2>&1; then
    echo "VERIFY_FAILED: ADPCM unit tests failed"
    exit 1
fi

# Step 3: Build with cbindgen
echo "Building platform library..."
if ! cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1; then
    echo "VERIFY_FAILED: Cargo build failed"
    exit 1
fi

# Step 4: Check header has ADPCM functions
echo "Checking header for ADPCM functions..."
if ! grep -q "Platform_Sound_CreateFromADPCM" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Sound_CreateFromADPCM not in header"
    exit 1
fi

echo "VERIFY_SUCCESS"
exit 0
