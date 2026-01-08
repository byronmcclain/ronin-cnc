#!/bin/bash
# Verification script for network manager (Task 10e)

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Network Manager ==="

cd "$ROOT_DIR"

# Step 1: Check manager.rs exists
echo "Step 1: Checking manager.rs exists..."
if [ ! -f "platform/src/network/manager.rs" ]; then
    echo "VERIFY_FAILED: platform/src/network/manager.rs not found"
    exit 1
fi
echo "  manager.rs exists"

# Step 2: Check mod.rs exports manager
echo "Step 2: Checking mod.rs exports manager..."
if ! grep -q 'pub mod manager' platform/src/network/mod.rs; then
    echo "VERIFY_FAILED: manager module not exported in mod.rs"
    exit 1
fi
echo "  manager module exported"

# Step 3: Build
echo "Step 3: Building platform crate..."
if ! cargo build --manifest-path platform/Cargo.toml --release 2>&1; then
    echo "VERIFY_FAILED: cargo build failed"
    exit 1
fi
echo "  Build successful"

# Step 4: Run manager tests
echo "Step 4: Running manager tests..."
if ! cargo test --manifest-path platform/Cargo.toml --release -- manager --test-threads=1 2>&1; then
    echo "VERIFY_FAILED: manager tests failed"
    exit 1
fi
echo "  Tests passed"

# Step 5: Check FFI exports
echo "Step 5: Checking FFI exports in header..."
cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1

REQUIRED_EXPORTS=(
    "Platform_NetworkManager_Init"
    "Platform_NetworkManager_Shutdown"
    "Platform_NetworkManager_HostGame"
    "Platform_NetworkManager_JoinGame"
    "Platform_NetworkManager_Disconnect"
    "Platform_NetworkManager_Update"
    "Platform_NetworkManager_IsHost"
    "Platform_NetworkManager_IsConnected"
    "Platform_NetworkManager_SendData"
    "Platform_NetworkManager_ReceiveData"
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
