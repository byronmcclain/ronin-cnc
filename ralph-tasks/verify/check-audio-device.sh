#!/bin/bash
# Verification script for Task 08a: Audio Device Setup

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Audio Device Setup ==="

cd "$ROOT_DIR"

# Step 1: Check rodio dependency in Cargo.toml
echo "Checking rodio dependency..."
if ! grep -q "rodio" platform/Cargo.toml; then
    echo "VERIFY_FAILED: rodio dependency not in Cargo.toml"
    exit 1
fi

# Step 2: Check audio module exists
echo "Checking audio module..."
if [ ! -f "platform/src/audio/mod.rs" ]; then
    echo "VERIFY_FAILED: platform/src/audio/mod.rs not found"
    exit 1
fi

# Step 3: Build with cbindgen
echo "Building platform library..."
if ! cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1; then
    echo "VERIFY_FAILED: Cargo build failed"
    exit 1
fi

# Step 4: Check header has audio init functions
echo "Checking header for audio functions..."
if ! grep -q "Platform_Audio_Init" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Audio_Init not in header"
    exit 1
fi

if ! grep -q "Platform_Audio_Shutdown" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Audio_Shutdown not in header"
    exit 1
fi

if ! grep -q "Platform_Audio_SetMasterVolume" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Audio_SetMasterVolume not in header"
    exit 1
fi

if ! grep -q "Platform_Audio_GetMasterVolume" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Audio_GetMasterVolume not in header"
    exit 1
fi

echo "VERIFY_SUCCESS"
exit 0
