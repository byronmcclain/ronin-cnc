#!/bin/bash
# Verification script for logging infrastructure

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Logging Infrastructure ==="

cd "$ROOT_DIR"

# Step 1: Check module file exists
echo "Step 1: Checking module file exists..."
if [ ! -f "platform/src/logging/mod.rs" ]; then
    if [ -f "platform/src/logging.rs" ]; then
        echo "  Found at platform/src/logging.rs"
    else
        echo "VERIFY_FAILED: logging module not found"
        exit 1
    fi
fi
echo "  Logging module exists"

# Step 2: Check lib.rs has logging module
echo "Step 2: Checking lib.rs includes logging module..."
if ! grep -q "pub mod logging" platform/src/lib.rs; then
    echo "VERIFY_FAILED: logging module not declared in lib.rs"
    exit 1
fi
echo "  Logging module declared in lib.rs"

# Step 3: Build the platform crate
echo "Step 3: Building platform crate..."
if ! cargo build --manifest-path platform/Cargo.toml --release 2>&1; then
    echo "VERIFY_FAILED: cargo build failed"
    exit 1
fi
echo "  Build successful"

# Step 4: Run logging tests
echo "Step 4: Running logging tests..."
if ! cargo test --manifest-path platform/Cargo.toml --release -- logging --test-threads=1 2>&1; then
    echo "VERIFY_FAILED: logging tests failed"
    exit 1
fi
echo "  Tests passed"

# Step 5: Generate and check header
echo "Step 5: Checking FFI exports in header..."
if ! cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1; then
    echo "VERIFY_FAILED: cbindgen header generation failed"
    exit 1
fi

# Check for logging FFI exports
for func in Platform_Log_Init Platform_Log_Shutdown Platform_LogMessage Platform_Log_SetLevel Platform_Log_Flush; do
    if ! grep -q "$func" include/platform.h; then
        echo "VERIFY_FAILED: $func not found in platform.h"
        exit 1
    fi
    echo "  Found $func in header"
done

echo ""
echo "VERIFY_SUCCESS"
exit 0
