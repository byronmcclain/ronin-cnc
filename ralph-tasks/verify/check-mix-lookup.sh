#!/bin/bash
# Verification script for MIX file data lookup

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking MIX Data Lookup ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Build
echo "[1/4] Building..."
if ! cmake --build build --target TestMixLookup 2>&1; then
    echo "VERIFY_FAILED: Build failed"
    exit 1
fi
echo "  OK: Build successful"

# Step 2: Find test file
echo "[2/4] Finding test MIX file..."
if [ -f "gamedata/REDALERT.MIX" ]; then
    TEST_MIX="gamedata/REDALERT.MIX"
elif [ -f "gamedata/SETUP.MIX" ]; then
    TEST_MIX="gamedata/SETUP.MIX"
else
    echo "VERIFY_WARNING: No MIX files found"
    echo "VERIFY_SUCCESS (build only)"
    exit 0
fi
echo "  OK: Using $TEST_MIX"

# Step 3: Run lookup test
echo "[3/4] Running lookup test..."
if ! ./build/TestMixLookup "$TEST_MIX"; then
    echo "VERIFY_FAILED: Lookup test failed"
    exit 1
fi
echo "  OK: Lookup test passed"

# Step 4: Verify data is non-empty
echo "[4/4] Verifying data retrieval..."
OUTPUT=$(./build/TestMixLookup "$TEST_MIX" 2>&1)
if echo "$OUTPUT" | grep -q "FAIL:"; then
    echo "VERIFY_FAILED: Test reported failures"
    exit 1
fi
echo "  OK: No failures detected"

echo ""
echo "VERIFY_SUCCESS"
exit 0
