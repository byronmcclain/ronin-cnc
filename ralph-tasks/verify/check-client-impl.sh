#!/bin/bash
# Verification script for client implementation (Task 10c)

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Client Implementation ==="

cd "$ROOT_DIR"

# Step 1: Check client.rs exists
echo "Step 1: Checking client.rs exists..."
if [ ! -f "platform/src/network/client.rs" ]; then
    echo "VERIFY_FAILED: platform/src/network/client.rs not found"
    exit 1
fi
echo "  client.rs exists"

# Step 2: Check mod.rs exports client
echo "Step 2: Checking mod.rs exports client..."
if ! grep -q 'pub mod client' platform/src/network/mod.rs; then
    echo "VERIFY_FAILED: client module not exported in mod.rs"
    exit 1
fi
echo "  client module exported"

# Step 3: Build
echo "Step 3: Building platform crate..."
if ! cargo build --manifest-path platform/Cargo.toml --release 2>&1; then
    echo "VERIFY_FAILED: cargo build failed"
    exit 1
fi
echo "  Build successful"

# Step 4: Run client tests
echo "Step 4: Running client tests..."
if ! cargo test --manifest-path platform/Cargo.toml --release -- client --test-threads=1 2>&1; then
    echo "VERIFY_FAILED: client tests failed"
    exit 1
fi
echo "  Tests passed"

# Step 5: Generate and check header
echo "Step 5: Checking FFI exports in header..."
cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1

REQUIRED_EXPORTS=(
    "Platform_Client_Create"
    "Platform_Client_Destroy"
    "Platform_Client_Connect"
    "Platform_Client_Service"
    "Platform_Client_IsConnected"
    "Platform_Client_Send"
    "Platform_Client_Receive"
    "Platform_Client_Disconnect"
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
