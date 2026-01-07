#!/bin/bash
# Verification script for Task 09c: Palette Remapping

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Palette Remapping ==="

cd "$ROOT_DIR"

# Step 1: Check remap module exists
echo "Checking remap module..."
if [ ! -f "platform/src/blit/remap.rs" ]; then
    echo "VERIFY_FAILED: platform/src/blit/remap.rs not found"
    exit 1
fi

# Step 2: Check mod.rs has remap module
echo "Checking blit/mod.rs includes remap..."
if ! grep -q "pub mod remap" platform/src/blit/mod.rs; then
    echo "VERIFY_FAILED: remap module not exported in blit/mod.rs"
    exit 1
fi

# Step 3: Run remap unit tests
echo "Running remap unit tests..."
if ! cargo test --manifest-path platform/Cargo.toml -- blit::remap 2>&1; then
    echo "VERIFY_FAILED: Remap unit tests failed"
    exit 1
fi

# Step 4: Build with cbindgen
echo "Building platform library..."
if ! cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1; then
    echo "VERIFY_FAILED: Cargo build failed"
    exit 1
fi

# Step 5: Check header has FFI exports
echo "Checking header for remap operations..."
if ! grep -q "Platform_Buffer_Remap" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Buffer_Remap not in header"
    exit 1
fi

echo "VERIFY_SUCCESS"
exit 0
