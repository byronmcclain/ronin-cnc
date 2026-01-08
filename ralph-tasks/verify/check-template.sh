#!/bin/bash
# Verification script for template loading

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Template Loading ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check source files
echo "[1/4] Checking source files..."
if [ ! -f "include/game/template.h" ]; then
    echo "VERIFY_FAILED: template.h not found"
    exit 1
fi
if [ ! -f "src/game/template.cpp" ]; then
    echo "VERIFY_FAILED: template.cpp not found"
    exit 1
fi
echo "  OK: Source files exist"

# Step 2: Build
echo "[2/4] Building..."
if ! cmake --build build --target TestTemplate 2>&1; then
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
echo "  OK: MIX file exists"

# Step 4: Run test
echo "[4/4] Running template test..."
if ! timeout 15 ./build/TestTemplate TEMPERAT 2>&1; then
    echo "VERIFY_FAILED: Template test failed"
    exit 1
fi
echo "  OK: Template test passed"

echo ""
echo "VERIFY_SUCCESS"
exit 0
