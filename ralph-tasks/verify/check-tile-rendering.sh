#!/bin/bash
# Verification script for Tile Rendering System

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Tile Rendering Implementation ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check files exist
echo "[1/4] Checking file existence..."
FILES=(
    "include/game/graphics/tile_renderer.h"
    "src/game/graphics/tile_renderer.cpp"
    "src/test_tile_rendering.cpp"
)

for file in "${FILES[@]}"; do
    if [ ! -f "$file" ]; then
        echo "VERIFY_FAILED: $file not found"
        exit 1
    fi
done
echo "  OK: All source files exist"

# Step 2: Build test executable
echo "[2/4] Building TestTileRendering..."
if ! cmake --build build --target TestTileRendering 2>&1 | tail -20; then
    echo "VERIFY_FAILED: Build failed"
    exit 1
fi
echo "  OK: Build succeeded"

# Step 3: Check executable exists
echo "[3/4] Checking executable..."
if [ ! -f "build/TestTileRendering" ]; then
    echo "VERIFY_FAILED: TestTileRendering executable not found"
    exit 1
fi
echo "  OK: Executable exists"

# Step 4: Run tests
echo "[4/4] Running unit tests..."
if ./build/TestTileRendering --quick 2>&1 | tee /tmp/tile_test_output.txt | grep -q "All tests PASSED"; then
    echo "  OK: Tests passed"
else
    echo "Test output:"
    cat /tmp/tile_test_output.txt
    echo "VERIFY_FAILED: Tests did not pass"
    exit 1
fi

echo ""
echo "VERIFY_SUCCESS"
exit 0
