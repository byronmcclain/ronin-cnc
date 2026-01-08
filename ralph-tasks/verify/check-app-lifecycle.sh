#!/bin/bash
# Verification script for application lifecycle

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Application Lifecycle ==="

cd "$ROOT_DIR"

# Step 1: Check module files exist
echo "Step 1: Checking module files exist..."
if [ ! -f "platform/src/app/mod.rs" ]; then
    echo "VERIFY_FAILED: platform/src/app/mod.rs not found"
    exit 1
fi
if [ ! -f "platform/src/app/lifecycle.rs" ]; then
    echo "VERIFY_FAILED: platform/src/app/lifecycle.rs not found"
    exit 1
fi
echo "  App module files exist"

# Step 2: Check lib.rs has app module
echo "Step 2: Checking lib.rs includes app module..."
if ! grep -q "pub mod app" platform/src/lib.rs; then
    echo "VERIFY_FAILED: app module not declared in lib.rs"
    exit 1
fi
echo "  App module declared in lib.rs"

# Step 3: Build the platform crate
echo "Step 3: Building platform crate..."
if ! cargo build --manifest-path platform/Cargo.toml --release 2>&1; then
    echo "VERIFY_FAILED: cargo build failed"
    exit 1
fi
echo "  Build successful"

# Step 4: Run lifecycle tests
echo "Step 4: Running lifecycle tests..."
if ! cargo test --manifest-path platform/Cargo.toml --release -- lifecycle --test-threads=1 2>&1; then
    echo "VERIFY_FAILED: lifecycle tests failed"
    exit 1
fi
echo "  Tests passed"

# Step 5: Generate and check header
echo "Step 5: Checking FFI exports in header..."
if ! cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1; then
    echo "VERIFY_FAILED: cbindgen header generation failed"
    exit 1
fi

# Check for lifecycle FFI exports
for func in Platform_GetAppState Platform_IsAppActive Platform_IsAppBackground Platform_RequestQuit Platform_ShouldQuit Platform_ClearQuitRequest; do
    if ! grep -q "$func" include/platform.h; then
        echo "VERIFY_FAILED: $func not found in platform.h"
        exit 1
    fi
    echo "  Found $func in header"
done

echo ""
echo "VERIFY_SUCCESS"
exit 0
