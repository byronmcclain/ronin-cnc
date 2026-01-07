#!/bin/bash
# Verification script for Task 08b: PCM Sound Loading and Playback

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking PCM Sound Playback ==="

cd "$ROOT_DIR"

# Step 1: Build with cbindgen
echo "Building platform library..."
if ! cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1; then
    echo "VERIFY_FAILED: Cargo build failed"
    exit 1
fi

# Step 2: Check header has sound creation functions
echo "Checking header for sound functions..."
if ! grep -q "Platform_Sound_CreateFromMemory" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Sound_CreateFromMemory not in header"
    exit 1
fi

if ! grep -q "Platform_Sound_Destroy" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Sound_Destroy not in header"
    exit 1
fi

# Step 3: Check playback functions
if ! grep -q "Platform_Sound_Play" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Sound_Play not in header"
    exit 1
fi

if ! grep -q "Platform_Sound_Stop" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Sound_Stop not in header"
    exit 1
fi

if ! grep -q "Platform_Sound_StopAll" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Sound_StopAll not in header"
    exit 1
fi

if ! grep -q "Platform_Sound_IsPlaying" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Sound_IsPlaying not in header"
    exit 1
fi

# Step 4: Check volume control
if ! grep -q "Platform_Sound_SetVolume" include/platform.h; then
    echo "VERIFY_FAILED: Platform_Sound_SetVolume not in header"
    exit 1
fi

echo "VERIFY_SUCCESS"
exit 0
