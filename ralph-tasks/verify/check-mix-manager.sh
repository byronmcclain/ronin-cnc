#!/bin/bash
# Verification script for MIX Manager

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking MIX Manager ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check source files
echo "[1/4] Checking source files..."
if [ ! -f "include/game/mix_manager.h" ]; then
    echo "VERIFY_FAILED: mix_manager.h not found"
    exit 1
fi
if [ ! -f "src/game/mix_manager.cpp" ]; then
    echo "VERIFY_FAILED: mix_manager.cpp not found"
    exit 1
fi
echo "  OK: Source files exist"

# Step 2: Build
echo "[2/4] Building..."
if ! cmake --build build --target TestMixManager 2>&1; then
    echo "VERIFY_FAILED: Build failed"
    exit 1
fi
echo "  OK: Build successful"

# Step 3: Check gamedata directory
echo "[3/4] Checking gamedata..."
if [ ! -d "gamedata" ]; then
    echo "VERIFY_WARNING: gamedata directory not found"
    echo "VERIFY_SUCCESS (build only)"
    exit 0
fi
echo "  OK: gamedata exists"

# Step 4: Run manager test
echo "[4/4] Running manager test..."
if ! ./build/TestMixManager gamedata; then
    echo "VERIFY_FAILED: Manager test failed"
    exit 1
fi
echo "  OK: Manager test passed"

echo ""
echo "VERIFY_SUCCESS"
exit 0
