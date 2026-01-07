#!/bin/bash
# Verification script for Task 09i: SIMD Optimization (Optional)

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking SIMD Optimization ==="

cd "$ROOT_DIR"

# Step 1: Check simd module exists
echo "Checking simd module..."
if [ ! -f "platform/src/blit/simd.rs" ]; then
    echo "VERIFY_FAILED: platform/src/blit/simd.rs not found"
    exit 1
fi

# Step 2: Check Cargo.toml has simd feature
echo "Checking Cargo.toml has simd feature..."
if ! grep -q 'simd = \[\]' platform/Cargo.toml; then
    echo "VERIFY_FAILED: simd feature not defined in Cargo.toml"
    exit 1
fi

# Step 3: Build WITHOUT simd feature (ensure scalar fallback compiles)
echo "Building without simd feature (scalar fallback)..."
if ! cargo build --manifest-path platform/Cargo.toml 2>&1; then
    echo "VERIFY_FAILED: Build without simd feature failed"
    exit 1
fi

# Step 4: Run tests WITHOUT simd feature
echo "Running tests without simd feature..."
if ! cargo test --manifest-path platform/Cargo.toml 2>&1; then
    echo "VERIFY_FAILED: Tests without simd feature failed"
    exit 1
fi

# Step 5: Build WITH simd feature
echo "Building with simd feature..."
if ! cargo build --manifest-path platform/Cargo.toml --features simd 2>&1; then
    echo "VERIFY_FAILED: Build with simd feature failed"
    exit 1
fi

# Step 6: Run tests WITH simd feature
echo "Running tests with simd feature..."
if ! cargo test --manifest-path platform/Cargo.toml --features simd 2>&1; then
    echo "VERIFY_FAILED: Tests with simd feature failed"
    exit 1
fi

# Step 7: Run SIMD-specific tests
echo "Running SIMD-specific unit tests..."
if ! cargo test --manifest-path platform/Cargo.toml --features simd -- simd 2>&1; then
    echo "VERIFY_FAILED: SIMD unit tests failed"
    exit 1
fi

# Step 8: Verify results match between scalar and SIMD
echo "Verifying scalar and SIMD results match..."
if ! cargo test --manifest-path platform/Cargo.toml --features simd -- buffer_clear_fast 2>&1; then
    echo "VERIFY_FAILED: buffer_clear_fast test failed"
    exit 1
fi

if ! cargo test --manifest-path platform/Cargo.toml --features simd -- buffer_copy_fast 2>&1; then
    echo "VERIFY_FAILED: buffer_copy_fast test failed"
    exit 1
fi

if ! cargo test --manifest-path platform/Cargo.toml --features simd -- buffer_remap_fast 2>&1; then
    echo "VERIFY_FAILED: buffer_remap_fast test failed"
    exit 1
fi

echo "VERIFY_SUCCESS"
exit 0
