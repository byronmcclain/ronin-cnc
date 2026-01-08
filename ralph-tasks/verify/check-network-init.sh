#!/bin/bash
# Verification script for network initialization (Task 10a)

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Network Module Setup ==="

cd "$ROOT_DIR"

# Step 1: Check Cargo.toml has enet dependency
echo "Step 1: Checking enet dependency in Cargo.toml..."
if ! grep -q 'enet = "0.3"' platform/Cargo.toml; then
    echo "VERIFY_FAILED: enet dependency not found in Cargo.toml"
    exit 1
fi
echo "  enet dependency found"

# Step 2: Check network module exists
echo "Step 2: Checking network module exists..."
if [ ! -f "platform/src/network/mod.rs" ]; then
    echo "VERIFY_FAILED: platform/src/network/mod.rs not found"
    exit 1
fi
echo "  network/mod.rs exists"

# Step 3: Check lib.rs has network module
echo "Step 3: Checking lib.rs has network module..."
if ! grep -q 'pub mod network' platform/src/lib.rs; then
    echo "VERIFY_FAILED: network module not declared in lib.rs"
    exit 1
fi
echo "  network module declared"

# Step 4: Build the platform crate
echo "Step 4: Building platform crate..."
if ! cargo build --manifest-path platform/Cargo.toml --release 2>&1; then
    echo "VERIFY_FAILED: cargo build failed"
    exit 1
fi
echo "  Build successful"

# Step 5: Run network tests
echo "Step 5: Running network tests..."
if ! cargo test --manifest-path platform/Cargo.toml --release -- network --test-threads=1 2>&1; then
    echo "VERIFY_FAILED: network tests failed"
    exit 1
fi
echo "  Tests passed"

# Step 6: Generate and check header
echo "Step 6: Checking FFI exports in header..."
if ! cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1; then
    echo "VERIFY_FAILED: cbindgen header generation failed"
    exit 1
fi

if [ ! -f "include/platform.h" ]; then
    echo "VERIFY_FAILED: include/platform.h not found"
    exit 1
fi

# Check for network FFI exports
for func in Platform_Network_Init Platform_Network_Shutdown Platform_Network_IsInitialized; do
    if ! grep -q "$func" include/platform.h; then
        echo "VERIFY_FAILED: $func not found in platform.h"
        exit 1
    fi
    echo "  Found $func in header"
done

echo ""
echo "=== All checks passed ==="
echo "VERIFY_SUCCESS"
exit 0
