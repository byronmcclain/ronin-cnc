#!/bin/bash
# Verification script for MIX file header parsing

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking MIX Header Parsing ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check source files exist
echo "[1/5] Checking source files..."
if [ ! -f "include/game/mix_file.h" ]; then
    echo "VERIFY_FAILED: include/game/mix_file.h not found"
    exit 1
fi
if [ ! -f "src/game/mix_file.cpp" ]; then
    echo "VERIFY_FAILED: src/game/mix_file.cpp not found"
    exit 1
fi
echo "  OK: Source files exist"

# Step 2: Build
echo "[2/5] Building..."
if ! cmake --build build --target game_mix 2>&1; then
    echo "VERIFY_FAILED: game_mix build failed"
    exit 1
fi
if ! cmake --build build --target TestMixHeader 2>&1; then
    echo "VERIFY_FAILED: TestMixHeader build failed"
    exit 1
fi
echo "  OK: Build successful"

# Step 3: Check for test MIX file
echo "[3/5] Checking for test MIX file..."
if [ -f "gamedata/REDALERT.MIX" ]; then
    TEST_MIX="gamedata/REDALERT.MIX"
elif [ -f "gamedata/SETUP.MIX" ]; then
    TEST_MIX="gamedata/SETUP.MIX"
else
    echo "VERIFY_WARNING: No MIX files found in gamedata/"
    echo "  Skipping runtime test - only build verified"
    echo ""
    echo "VERIFY_SUCCESS (build only)"
    exit 0
fi
echo "  OK: Using $TEST_MIX"

# Step 4: Run header test
echo "[4/5] Running header test..."
if ! ./build/TestMixHeader "$TEST_MIX"; then
    echo "VERIFY_FAILED: TestMixHeader failed"
    exit 1
fi
echo "  OK: Header test passed"

# Step 5: Verify file count is reasonable
echo "[5/5] Verifying file count..."
FILE_COUNT=$(./build/TestMixHeader "$TEST_MIX" 2>&1 | grep "Files:" | awk '{print $2}')
if [ -z "$FILE_COUNT" ] || [ "$FILE_COUNT" -lt 1 ]; then
    echo "VERIFY_FAILED: Could not determine file count or count is 0"
    exit 1
fi
echo "  OK: MIX contains $FILE_COUNT files"

echo ""
echo "VERIFY_SUCCESS"
exit 0
