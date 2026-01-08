#!/bin/bash
# Verification script for host implementation (Task 10b)

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Host Implementation ==="

cd "$ROOT_DIR"

# Step 1: Check host.rs exists
echo "Step 1: Checking host.rs exists..."
if [ ! -f "platform/src/network/host.rs" ]; then
    echo "VERIFY_FAILED: platform/src/network/host.rs not found"
    exit 1
fi
echo "  host.rs exists"

# Step 2: Check mod.rs exports host
echo "Step 2: Checking mod.rs exports host..."
if ! grep -q 'pub mod host' platform/src/network/mod.rs; then
    echo "VERIFY_FAILED: host module not exported in mod.rs"
    exit 1
fi
echo "  host module exported"

# Step 3: Build the platform crate
echo "Step 3: Building platform crate..."
if ! cargo build --manifest-path platform/Cargo.toml --release 2>&1; then
    echo "VERIFY_FAILED: cargo build failed"
    exit 1
fi
echo "  Build successful"

# Step 4: Run host tests
echo "Step 4: Running host tests..."
if ! cargo test --manifest-path platform/Cargo.toml --release -- host --test-threads=1 2>&1; then
    echo "VERIFY_FAILED: host tests failed"
    exit 1
fi
echo "  Tests passed"

# Step 5: Generate and check header
echo "Step 5: Checking FFI exports in header..."
if ! cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1; then
    echo "VERIFY_FAILED: cbindgen header generation failed"
    exit 1
fi

# Check for host FFI exports
REQUIRED_EXPORTS=(
    "Platform_Host_Create"
    "Platform_Host_Destroy"
    "Platform_Host_Service"
    "Platform_Host_Receive"
    "Platform_Host_Broadcast"
    "Platform_Host_PeerCount"
    "Platform_Host_HasPackets"
    "Platform_Host_Flush"
)

for func in "${REQUIRED_EXPORTS[@]}"; do
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
