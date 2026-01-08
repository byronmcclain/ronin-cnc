#!/bin/bash
# Verification script for performance module

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Performance Module ==="

cd "$ROOT_DIR"

# Step 1: Check module files exist
echo "Step 1: Checking module files exist..."
for file in mod.rs simd.rs pool.rs timing.rs; do
    if [ ! -f "platform/src/perf/$file" ]; then
        echo "VERIFY_FAILED: platform/src/perf/$file not found"
        exit 1
    fi
done
echo "  Perf module files exist"

# Step 2: Check lib.rs has perf module
echo "Step 2: Checking lib.rs includes perf module..."
if ! grep -q "pub mod perf" platform/src/lib.rs; then
    echo "VERIFY_FAILED: perf module not declared in lib.rs"
    exit 1
fi
echo "  Perf module declared in lib.rs"

# Step 3: Build the platform crate
echo "Step 3: Building platform crate..."
if ! cargo build --manifest-path platform/Cargo.toml --release 2>&1; then
    echo "VERIFY_FAILED: cargo build failed"
    exit 1
fi
echo "  Build successful"

# Step 4: Run perf tests
echo "Step 4: Running perf tests..."
if ! cargo test --manifest-path platform/Cargo.toml --release -- perf --test-threads=1 2>&1; then
    echo "VERIFY_FAILED: perf tests failed"
    exit 1
fi
echo "  Tests passed"

# Step 5: Generate and check header
echo "Step 5: Checking FFI exports in header..."
if ! cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1; then
    echo "VERIFY_FAILED: cbindgen header generation failed"
    exit 1
fi

# Check for perf FFI exports
for func in Platform_GetFPS Platform_GetFrameTime Platform_FrameStart Platform_FrameEnd Platform_GetFrameCount; do
    if ! grep -q "$func" include/platform.h; then
        echo "VERIFY_FAILED: $func not found in platform.h"
        exit 1
    fi
    echo "  Found $func in header"
done

echo ""
echo "VERIFY_SUCCESS"
exit 0
