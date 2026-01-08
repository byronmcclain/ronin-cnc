#!/bin/bash
# Verification script for configuration paths module

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Configuration Paths Module ==="

cd "$ROOT_DIR"

# Step 1: Check module files exist
echo "Step 1: Checking module files exist..."
if [ ! -f "platform/src/config/mod.rs" ]; then
    echo "VERIFY_FAILED: platform/src/config/mod.rs not found"
    exit 1
fi
if [ ! -f "platform/src/config/paths.rs" ]; then
    echo "VERIFY_FAILED: platform/src/config/paths.rs not found"
    exit 1
fi
echo "  Config module files exist"

# Step 2: Check lib.rs has config module
echo "Step 2: Checking lib.rs includes config module..."
if ! grep -q "pub mod config" platform/src/lib.rs; then
    echo "VERIFY_FAILED: config module not declared in lib.rs"
    exit 1
fi
echo "  Config module declared in lib.rs"

# Step 3: Build the platform crate
echo "Step 3: Building platform crate..."
if ! cargo build --manifest-path platform/Cargo.toml --release 2>&1; then
    echo "VERIFY_FAILED: cargo build failed"
    exit 1
fi
echo "  Build successful"

# Step 4: Run config tests
echo "Step 4: Running config tests..."
if ! cargo test --manifest-path platform/Cargo.toml --release -- config --test-threads=1 2>&1; then
    echo "VERIFY_FAILED: config tests failed"
    exit 1
fi
echo "  Tests passed"

# Step 5: Generate and check header
echo "Step 5: Checking FFI exports in header..."
if ! cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1; then
    echo "VERIFY_FAILED: cbindgen header generation failed"
    exit 1
fi

# Check for config FFI exports
for func in Platform_GetConfigPath Platform_GetSavesPath Platform_GetLogPath Platform_GetCachePath Platform_GetDataPath Platform_EnsureDirectories; do
    if ! grep -q "$func" include/platform.h; then
        echo "VERIFY_FAILED: $func not found in platform.h"
        exit 1
    fi
    echo "  Found $func in header"
done

echo ""
echo "VERIFY_SUCCESS"
exit 0
