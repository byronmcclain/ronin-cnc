#!/bin/bash
# Verification script for Task 09b: Buffer-to-Buffer Blit

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Buffer-to-Buffer Blit ==="

cd "$ROOT_DIR"

# Step 1: Check copy module exists
echo "Checking copy module..."
if [ ! -f "platform/src/blit/copy.rs" ]; then
    echo "VERIFY_FAILED: platform/src/blit/copy.rs not found"
    exit 1
fi

# Step 2: Check mod.rs has copy module
echo "Checking blit/mod.rs includes copy..."
if ! grep -q "pub mod copy" platform/src/blit/mod.rs; then
    echo "VERIFY_FAILED: copy module not exported in blit/mod.rs"
    exit 1
fi

# Step 3: Run copy unit tests
echo "Running copy unit tests..."
if ! cargo test --manifest-path platform/Cargo.toml -- blit::copy 2>&1; then
    echo "VERIFY_FAILED: Copy unit tests failed"
    exit 1
fi

# Step 4: Build with cbindgen
echo "Building platform library..."
if ! cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1; then
    echo "VERIFY_FAILED: Cargo build failed"
    exit 1
fi

# Step 5: Check header has FFI exports
echo "Checking header for blit operations..."
if ! grep -q "Platform_Buffer_ToBuffer" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Buffer_ToBuffer not in header"
    exit 1
fi

if ! grep -q "Platform_Buffer_ToBufferTrans" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Buffer_ToBufferTrans not in header"
    exit 1
fi

echo "VERIFY_SUCCESS"
exit 0
