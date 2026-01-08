#!/bin/bash
# Verification script for asset system integration

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Asset System Integration ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check source file
echo "[1/5] Checking source file..."
if [ ! -f "src/test_assets_integration.cpp" ]; then
    echo "VERIFY_FAILED: test_assets_integration.cpp not found"
    exit 1
fi
echo "  OK: Source file exists"

# Step 2: Build all dependencies
echo "[2/5] Building asset system..."
if ! cmake --build build --target game_assets 2>&1; then
    echo "VERIFY_FAILED: game_assets build failed"
    exit 1
fi
echo "  OK: game_assets built"

# Step 3: Build integration test
echo "[3/5] Building integration test..."
if ! cmake --build build --target TestAssetsIntegration 2>&1; then
    echo "VERIFY_FAILED: TestAssetsIntegration build failed"
    exit 1
fi
echo "  OK: TestAssetsIntegration built"

# Step 4: Check for game data
echo "[4/5] Checking game data..."
if [ ! -d "gamedata" ]; then
    echo "VERIFY_WARNING: gamedata directory not found"
    echo "VERIFY_SUCCESS (build only)"
    exit 0
fi

if [ ! -f "gamedata/REDALERT.MIX" ]; then
    echo "VERIFY_WARNING: REDALERT.MIX not found"
    echo "VERIFY_SUCCESS (build only)"
    exit 0
fi
echo "  OK: Game data exists"

# Step 5: Run integration test
echo "[5/5] Running integration test..."
if ! timeout 60 ./build/TestAssetsIntegration 2>&1; then
    echo "VERIFY_FAILED: Integration test failed"
    exit 1
fi
echo "  OK: Integration test passed"

echo ""
echo "VERIFY_SUCCESS"
exit 0
