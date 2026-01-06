#!/bin/bash
# Verification script for Task 05c: Frame Rate Control

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Frame Rate Control ==="

cd "$ROOT_DIR"

# Step 1: Build with cbindgen
echo "Building platform library..."
if ! cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1; then
    echo "VERIFY_FAILED: Cargo build failed"
    exit 1
fi

# Step 2: Check header has frame rate functions
if ! grep -q "Platform_Frame_Begin" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Frame_Begin not in header"
    exit 1
fi

if ! grep -q "Platform_Frame_End" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Frame_End not in header"
    exit 1
fi

if ! grep -q "Platform_Frame_GetFPS" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Frame_GetFPS not in header"
    exit 1
fi

# Step 3: Check timer module has FrameRateController
if ! grep -q "FrameRateController" platform/src/timer/mod.rs; then
    echo "VERIFY_FAILED: FrameRateController not in timer module"
    exit 1
fi

echo "VERIFY_SUCCESS"
exit 0
