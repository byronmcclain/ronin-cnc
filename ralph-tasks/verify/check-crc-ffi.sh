#!/bin/bash
# Verification script for CRC32 FFI export

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking CRC32 FFI Export ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check that crc module exists
echo "[1/6] Checking crc module exists..."
if [ ! -f "platform/src/util/crc.rs" ]; then
    echo "VERIFY_FAILED: platform/src/util/crc.rs not found"
    exit 1
fi
echo "  OK: crc.rs exists"

# Step 2: Run Rust unit tests for CRC
echo "[2/6] Running Rust CRC tests..."
if ! cargo test --manifest-path platform/Cargo.toml -- crc --nocapture 2>&1; then
    echo "VERIFY_FAILED: Rust CRC tests failed"
    exit 1
fi
echo "  OK: Rust tests pass"

# Step 3: Build platform library
echo "[3/6] Building platform library..."
if ! cargo build --manifest-path platform/Cargo.toml --release 2>&1; then
    echo "VERIFY_FAILED: Platform build failed"
    exit 1
fi
echo "  OK: Platform builds"

# Step 4: Check that FFI exports are in header
echo "[4/6] Checking FFI exports in platform.h..."
if ! grep -q "Platform_CRC_Calculate" include/platform.h; then
    echo "VERIFY_FAILED: Platform_CRC_Calculate not found in platform.h"
    exit 1
fi
if ! grep -q "Platform_CRC_ForMixLookup" include/platform.h; then
    echo "VERIFY_FAILED: Platform_CRC_ForMixLookup not found in platform.h"
    exit 1
fi
echo "  OK: FFI exports present in header"

# Step 5: Build C++ test
echo "[5/6] Building C++ test..."
if ! cmake --build build --target TestCRC 2>&1; then
    echo "VERIFY_FAILED: TestCRC build failed"
    exit 1
fi
echo "  OK: TestCRC builds"

# Step 6: Run C++ test
echo "[6/6] Running C++ test..."
if ! ./build/TestCRC; then
    echo "VERIFY_FAILED: TestCRC failed"
    exit 1
fi
echo "  OK: TestCRC passes"

echo ""
echo "VERIFY_SUCCESS"
exit 0
