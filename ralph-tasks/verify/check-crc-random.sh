#!/bin/bash
# Verification script for Task 09g: CRC32 and Random Number Generator

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking CRC32 and Random Number Generator ==="

cd "$ROOT_DIR"

# Step 1: Check util modules exist
echo "Checking utility modules..."
if [ ! -f "platform/src/util/crc.rs" ]; then
    echo "VERIFY_FAILED: platform/src/util/crc.rs not found"
    exit 1
fi

if [ ! -f "platform/src/util/random.rs" ]; then
    echo "VERIFY_FAILED: platform/src/util/random.rs not found"
    exit 1
fi

# Step 2: Check util module structure
echo "Checking util module structure..."
if [ ! -f "platform/src/util/mod.rs" ]; then
    echo "VERIFY_FAILED: platform/src/util/mod.rs not found"
    exit 1
fi

# Step 3: Check lib.rs has util module
echo "Checking lib.rs includes util..."
if ! grep -q "pub mod util" platform/src/lib.rs; then
    echo "VERIFY_FAILED: util module not exported in lib.rs"
    exit 1
fi

# Step 4: Run CRC unit tests
echo "Running CRC unit tests..."
if ! cargo test --manifest-path platform/Cargo.toml -- util::crc 2>&1; then
    echo "VERIFY_FAILED: CRC unit tests failed"
    exit 1
fi

# Step 5: Run random unit tests
echo "Running random unit tests..."
if ! cargo test --manifest-path platform/Cargo.toml -- util::random 2>&1; then
    echo "VERIFY_FAILED: Random unit tests failed"
    exit 1
fi

# Step 6: Build with cbindgen
echo "Building platform library..."
if ! cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1; then
    echo "VERIFY_FAILED: Cargo build failed"
    exit 1
fi

# Step 7: Check header has FFI exports
echo "Checking header for CRC operations..."
if ! grep -q "Platform_CRC32" include/platform.h; then
    echo "VERIFY_FAILED: Platform_CRC32 not in header"
    exit 1
fi

echo "Checking header for Random operations..."
if ! grep -q "Platform_Random" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Random not in header"
    exit 1
fi

echo "VERIFY_SUCCESS"
exit 0
