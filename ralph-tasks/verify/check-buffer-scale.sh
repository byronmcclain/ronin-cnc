#!/bin/bash
# Verification script for Task 09d: Buffer Scaling

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Buffer Scaling ==="

cd "$ROOT_DIR"

# Step 1: Check scale module exists
echo "Checking scale module..."
if [ ! -f "platform/src/blit/scale.rs" ]; then
    echo "VERIFY_FAILED: platform/src/blit/scale.rs not found"
    exit 1
fi

# Step 2: Check mod.rs has scale module
echo "Checking blit/mod.rs includes scale..."
if ! grep -q "pub mod scale" platform/src/blit/mod.rs; then
    echo "VERIFY_FAILED: scale module not exported in blit/mod.rs"
    exit 1
fi

# Step 3: Run scale unit tests
echo "Running scale unit tests..."
if ! cargo test --manifest-path platform/Cargo.toml -- blit::scale 2>&1; then
    echo "VERIFY_FAILED: Scale unit tests failed"
    exit 1
fi

# Step 4: Build with cbindgen
echo "Building platform library..."
if ! cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1; then
    echo "VERIFY_FAILED: Cargo build failed"
    exit 1
fi

# Step 5: Check header has FFI exports
echo "Checking header for scale operations..."
if ! grep -q "Platform_Buffer_Scale" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Buffer_Scale not in header"
    exit 1
fi

echo "VERIFY_SUCCESS"
exit 0
