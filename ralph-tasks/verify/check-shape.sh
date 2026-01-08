#!/bin/bash
# Verification script for shape loading

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Shape Loading ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check source files
echo "[1/4] Checking source files..."
if [ ! -f "include/game/shape.h" ]; then
    echo "VERIFY_FAILED: shape.h not found"
    exit 1
fi
if [ ! -f "src/game/shape.cpp" ]; then
    echo "VERIFY_FAILED: shape.cpp not found"
    exit 1
fi
echo "  OK: Source files exist"

# Step 2: Build
echo "[2/4] Building..."
if ! cmake --build build --target TestShape 2>&1; then
    echo "VERIFY_FAILED: Build failed"
    exit 1
fi
echo "  OK: Build successful"

# Step 3: Check for MIX file
echo "[3/4] Checking for MIX file..."
if [ ! -f "gamedata/REDALERT.MIX" ]; then
    echo "VERIFY_WARNING: REDALERT.MIX not found"
    echo "VERIFY_SUCCESS (build only)"
    exit 0
fi
echo "  OK: REDALERT.MIX exists"

# Step 4: Run test
echo "[4/4] Running shape test..."
if ! timeout 30 ./build/TestShape MOUSE.SHP 2>&1; then
    echo "VERIFY_FAILED: Shape test failed"
    exit 1
fi
echo "  OK: Shape test passed"

echo ""
echo "VERIFY_SUCCESS"
exit 0
