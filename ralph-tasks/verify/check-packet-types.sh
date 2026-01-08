#!/bin/bash
# Verification script for packet types (Task 10d)

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Packet Types ==="

cd "$ROOT_DIR"

# Step 1: Check packet.rs exists
echo "Step 1: Checking packet.rs exists..."
if [ ! -f "platform/src/network/packet.rs" ]; then
    echo "VERIFY_FAILED: platform/src/network/packet.rs not found"
    exit 1
fi
echo "  packet.rs exists"

# Step 2: Check mod.rs exports packet
echo "Step 2: Checking mod.rs exports packet..."
if ! grep -q 'pub mod packet' platform/src/network/mod.rs; then
    echo "VERIFY_FAILED: packet module not exported in mod.rs"
    exit 1
fi
echo "  packet module exported"

# Step 3: Build
echo "Step 3: Building platform crate..."
if ! cargo build --manifest-path platform/Cargo.toml --release 2>&1; then
    echo "VERIFY_FAILED: cargo build failed"
    exit 1
fi
echo "  Build successful"

# Step 4: Run packet tests
echo "Step 4: Running packet tests..."
if ! cargo test --manifest-path platform/Cargo.toml --release -- packet --test-threads=1 2>&1; then
    echo "VERIFY_FAILED: packet tests failed"
    exit 1
fi
echo "  Tests passed"

# Step 5: Check FFI exports
echo "Step 5: Checking FFI exports in header..."
cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1

REQUIRED_EXPORTS=(
    "Platform_Packet_CreateHeader"
    "Platform_Packet_HeaderSize"
    "Platform_Packet_ValidateHeader"
    "Platform_Packet_ParseHeader"
    "Platform_Packet_GetPayload"
    "Platform_Packet_Build"
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
