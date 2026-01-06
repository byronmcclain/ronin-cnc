#!/bin/bash
# Verification script for Task 05a: Basic Timing Functions

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Basic Timing Functions ==="

cd "$ROOT_DIR"

# Step 1: Build with cbindgen
echo "Building platform library..."
if ! cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1; then
    echo "VERIFY_FAILED: Cargo build failed"
    exit 1
fi

# Step 2: Check timer module exists
if [ ! -f "platform/src/timer/mod.rs" ]; then
    echo "VERIFY_FAILED: platform/src/timer/mod.rs not found"
    exit 1
fi

# Step 3: Check header has timer functions
if ! grep -q "Platform_Timer_GetTicks" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Timer_GetTicks not in header"
    exit 1
fi

if ! grep -q "Platform_Timer_GetPerformanceCounter" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Timer_GetPerformanceCounter not in header"
    exit 1
fi

if ! grep -q "Platform_Timer_Delay" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Timer_Delay not in header"
    exit 1
fi

echo "VERIFY_SUCCESS"
exit 0
