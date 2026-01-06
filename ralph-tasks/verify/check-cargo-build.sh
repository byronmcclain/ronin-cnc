#!/bin/bash
# Verification script for Rust crate build

set -e

cd "$(dirname "$0")/../../platform"

echo "=== Checking Cargo Build ==="

# Attempt build
if ! cargo build --release 2>&1; then
    echo "VERIFY_FAILED: Cargo build failed"
    exit 1
fi

# Check library exists
if [ ! -f target/release/libredalert_platform.a ]; then
    echo "VERIFY_FAILED: Static library not found"
    exit 1
fi

# Check library has expected symbols
if ! nm target/release/libredalert_platform.a 2>/dev/null | grep -q "Platform_Init"; then
    echo "VERIFY_FAILED: Platform_Init symbol not found in library"
    exit 1
fi

echo "VERIFY_SUCCESS"
exit 0
