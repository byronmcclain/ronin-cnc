#!/bin/bash
# Verification script for game project structure

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Game Project Structure ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check directories exist
echo "[1/5] Checking directory structure..."
for dir in src/game src/game/core; do
    if [ ! -d "$dir" ]; then
        echo "VERIFY_FAILED: $dir not found"
        exit 1
    fi
done
echo "  OK: Directories exist"

# Step 2: Check header files exist
echo "[2/5] Checking header files..."
for header in src/game/game.h src/game/core/types.h src/game/core/rtti.h src/game/core/globals.h; do
    if [ ! -f "$header" ]; then
        echo "VERIFY_FAILED: $header not found"
        exit 1
    fi
done
echo "  OK: Headers exist"

# Step 3: Configure CMake
echo "[3/5] Configuring CMake..."
CMAKE_POLICY_VERSION_MINIMUM=3.5 cmake -B build -S . 2>&1 || {
    echo "VERIFY_FAILED: CMake configure failed"
    exit 1
}
echo "  OK: CMake configured"

# Step 4: Build test
echo "[4/5] Building TestGameStructure..."
cmake --build build --target TestGameStructure 2>&1 || {
    echo "VERIFY_FAILED: Build failed"
    exit 1
}
echo "  OK: Build succeeded"

# Step 5: Run test
echo "[5/5] Running TestGameStructure..."
if ! ./build/TestGameStructure; then
    echo "VERIFY_FAILED: Test failed"
    exit 1
fi

echo ""
echo "VERIFY_SUCCESS"
exit 0
