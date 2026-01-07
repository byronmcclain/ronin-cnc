#!/bin/bash
# Verification script for Task 09e: Shadow Drawing

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Shadow Drawing ==="

cd "$ROOT_DIR"

# Step 1: Check shadow module exists
echo "Checking shadow module..."
if [ ! -f "platform/src/blit/shadow.rs" ]; then
    echo "VERIFY_FAILED: platform/src/blit/shadow.rs not found"
    exit 1
fi

# Step 2: Check mod.rs has shadow module
echo "Checking blit/mod.rs includes shadow..."
if ! grep -q "pub mod shadow" platform/src/blit/mod.rs; then
    echo "VERIFY_FAILED: shadow module not exported in blit/mod.rs"
    exit 1
fi

# Step 3: Run shadow unit tests
echo "Running shadow unit tests..."
if ! cargo test --manifest-path platform/Cargo.toml -- blit::shadow 2>&1; then
    echo "VERIFY_FAILED: Shadow unit tests failed"
    exit 1
fi

# Step 4: Build with cbindgen
echo "Building platform library..."
if ! cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1; then
    echo "VERIFY_FAILED: Cargo build failed"
    exit 1
fi

# Step 5: Check header has FFI exports
echo "Checking header for shadow operations..."
if ! grep -q "Platform_Buffer_Shadow" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Buffer_Shadow not in header"
    exit 1
fi

echo "VERIFY_SUCCESS"
exit 0
