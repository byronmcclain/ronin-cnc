#!/bin/bash
# Verification script for display integration

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Display Integration ==="

cd "$ROOT_DIR"

# Step 1: Check display module exists
echo "Step 1: Checking display module exists..."
if [ ! -f "platform/src/graphics/display.rs" ]; then
    echo "VERIFY_FAILED: platform/src/graphics/display.rs not found"
    exit 1
fi
echo "  Display module exists"

# Step 2: Check mod.rs includes display
echo "Step 2: Checking mod.rs includes display..."
if ! grep -q "pub mod display" platform/src/graphics/mod.rs; then
    echo "VERIFY_FAILED: display module not declared in graphics/mod.rs"
    exit 1
fi
echo "  Display module declared"

# Step 3: Build the platform crate
echo "Step 3: Building platform crate..."
if ! cargo build --manifest-path platform/Cargo.toml --release 2>&1; then
    echo "VERIFY_FAILED: cargo build failed"
    exit 1
fi
echo "  Build successful"

# Step 4: Run display tests
echo "Step 4: Running display tests..."
if ! cargo test --manifest-path platform/Cargo.toml --release -- display 2>&1; then
    echo "VERIFY_FAILED: display tests failed"
    exit 1
fi
echo "  Tests passed"

# Step 5: Generate and check header
echo "Step 5: Checking FFI exports in header..."
if ! cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1; then
    echo "VERIFY_FAILED: cbindgen header generation failed"
    exit 1
fi

# Check for display FFI exports
for func in Platform_IsRetinaDisplay Platform_GetDisplayScale Platform_ToggleFullscreen Platform_SetFullscreen Platform_IsFullscreen Platform_GetWindowSize Platform_GetDrawableSize; do
    if ! grep -q "$func" include/platform.h; then
        echo "VERIFY_FAILED: $func not found in platform.h"
        exit 1
    fi
    echo "  Found $func in header"
done

echo ""
echo "VERIFY_SUCCESS"
exit 0
