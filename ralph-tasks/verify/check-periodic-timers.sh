#!/bin/bash
# Verification script for Task 05b: Periodic Timers

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Periodic Timers ==="

cd "$ROOT_DIR"

# Step 1: Build with cbindgen
echo "Building platform library..."
if ! cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1; then
    echo "VERIFY_FAILED: Cargo build failed"
    exit 1
fi

# Step 2: Check header has timer create/destroy
if ! grep -q "Platform_Timer_Create" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Timer_Create not in header"
    exit 1
fi

if ! grep -q "Platform_Timer_Destroy" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Timer_Destroy not in header"
    exit 1
fi

# Step 3: Check timer module has required code
if ! grep -q "TimerRegistry" platform/src/timer/mod.rs; then
    echo "VERIFY_FAILED: TimerRegistry not in timer module"
    exit 1
fi

if ! grep -q "create_timer" platform/src/timer/mod.rs; then
    echo "VERIFY_FAILED: create_timer not in timer module"
    exit 1
fi

echo "VERIFY_SUCCESS"
exit 0
