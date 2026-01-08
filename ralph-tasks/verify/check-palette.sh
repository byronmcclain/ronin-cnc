#!/bin/bash
# Verification script for palette loading

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Palette Loading ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check source files
echo "[1/4] Checking source files..."
if [ ! -f "include/game/palette.h" ]; then
    echo "VERIFY_FAILED: palette.h not found"
    exit 1
fi
if [ ! -f "src/game/palette.cpp" ]; then
    echo "VERIFY_FAILED: palette.cpp not found"
    exit 1
fi
echo "  OK: Source files exist"

# Step 2: Build
echo "[2/4] Building..."
if ! cmake --build build --target TestPalette 2>&1; then
    echo "VERIFY_FAILED: Build failed"
    exit 1
fi
echo "  OK: Build successful"

# Step 3: Check for palette file
echo "[3/4] Checking for palette files..."
if [ ! -f "gamedata/REDALERT.MIX" ]; then
    echo "VERIFY_WARNING: REDALERT.MIX not found"
    echo "VERIFY_SUCCESS (build only)"
    exit 0
fi
echo "  OK: REDALERT.MIX exists"

# Step 4: Run test (with timeout for visual test)
echo "[4/4] Running palette test..."
# Use timeout to prevent hanging on visual display
if ! timeout 30 ./build/TestPalette TEMPERAT.PAL 2>&1; then
    echo "VERIFY_FAILED: Palette test failed"
    exit 1
fi
echo "  OK: Palette test passed"

echo ""
echo "VERIFY_SUCCESS"
exit 0
