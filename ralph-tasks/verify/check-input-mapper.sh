#!/bin/bash
# Verification script for Input Mapper (Task 16b)

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Input Mapper Implementation ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check files exist
echo "[1/4] Checking file existence..."
FILES=(
    "include/game/input/game_action.h"
    "include/game/input/input_mapper.h"
    "src/game/input/input_mapper.cpp"
    "src/test_input_mapper.cpp"
)

for file in "${FILES[@]}"; do
    if [ ! -f "$file" ]; then
        echo "VERIFY_FAILED: $file not found"
        exit 1
    fi
done
echo "  OK: All source files exist"

# Step 2: Build test executable
echo "[2/4] Building TestInputMapper..."
if ! cmake --build build --target TestInputMapper 2>&1 | tail -20; then
    echo "VERIFY_FAILED: Build failed"
    exit 1
fi
echo "  OK: Build succeeded"

# Step 3: Check executable exists
echo "[3/4] Checking executable..."
if [ ! -f "build/TestInputMapper" ]; then
    echo "VERIFY_FAILED: TestInputMapper executable not found"
    exit 1
fi
echo "  OK: Executable exists"

# Step 4: Run tests
echo "[4/4] Running input mapper tests..."
if ./build/TestInputMapper --quick 2>&1 | tee /tmp/input_mapper_test_output.txt | grep -q "All tests PASSED"; then
    echo "  OK: Tests passed"
else
    echo "Test output:"
    cat /tmp/input_mapper_test_output.txt
    echo "VERIFY_FAILED: Tests did not pass"
    exit 1
fi

echo ""
echo "VERIFY_SUCCESS"
exit 0
