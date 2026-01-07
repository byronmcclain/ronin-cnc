#!/bin/bash
# Verification script for Task 09a: Basic Buffer Operations

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Basic Buffer Operations ==="

cd "$ROOT_DIR"

# Step 1: Check blit module exists
echo "Checking blit module structure..."
if [ ! -f "platform/src/blit/mod.rs" ]; then
    echo "VERIFY_FAILED: platform/src/blit/mod.rs not found"
    exit 1
fi

if [ ! -f "platform/src/blit/basic.rs" ]; then
    echo "VERIFY_FAILED: platform/src/blit/basic.rs not found"
    exit 1
fi

# Step 2: Check lib.rs has blit module
echo "Checking lib.rs includes blit..."
if ! grep -q "pub mod blit" platform/src/lib.rs; then
    echo "VERIFY_FAILED: blit module not exported in lib.rs"
    exit 1
fi

# Step 3: Run ClipRect and blit unit tests
echo "Running blit unit tests..."
if ! cargo test --manifest-path platform/Cargo.toml -- blit 2>&1; then
    echo "VERIFY_FAILED: Blit unit tests failed"
    exit 1
fi

# Step 4: Build with cbindgen
echo "Building platform library..."
if ! cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1; then
    echo "VERIFY_FAILED: Cargo build failed"
    exit 1
fi

# Step 5: Check header has FFI exports
echo "Checking header for buffer operations..."
if ! grep -q "Platform_Buffer_Clear" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Buffer_Clear not in header"
    exit 1
fi

if ! grep -q "Platform_Buffer_FillRect" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Buffer_FillRect not in header"
    exit 1
fi

if ! grep -q "Platform_Buffer_HLine" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Buffer_HLine not in header"
    exit 1
fi

if ! grep -q "Platform_Buffer_VLine" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Buffer_VLine not in header"
    exit 1
fi

# Step 6: Check ClipRect structure in header
echo "Checking ClipRect in header..."
if ! grep -q "ClipRect" include/platform.h; then
    echo "VERIFY_FAILED: ClipRect struct not in header"
    exit 1
fi

echo "VERIFY_SUCCESS"
exit 0
