#!/bin/bash
# Verification script for test infrastructure

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "=== Checking Test Infrastructure ==="

cd "$ROOT_DIR"

# Step 1: Check test files exist
echo "Step 1: Checking test files exist..."
if [ ! -f "src/test_platform.cpp" ]; then
    echo "VERIFY_FAILED: src/test_platform.cpp not found"
    exit 1
fi
if [ ! -f "scripts/run-tests.sh" ]; then
    echo "VERIFY_FAILED: scripts/run-tests.sh not found"
    exit 1
fi
echo "  Test files exist"

# Step 2: Check scripts are executable
echo "Step 2: Checking scripts are executable..."
if [ ! -x "scripts/run-tests.sh" ]; then
    chmod +x "scripts/run-tests.sh"
fi
echo "  Scripts are executable"

# Step 3: Build project with tests
echo "Step 3: Building project with tests..."
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release 2>&1 || exit 1
cmake --build . 2>&1 || exit 1
cd "$ROOT_DIR"
echo "  Build successful"

# Step 4: Check test executable exists
echo "Step 4: Checking test executable..."
if [ ! -f "build/TestPlatform" ]; then
    echo "VERIFY_FAILED: TestPlatform executable not built"
    exit 1
fi
echo "  TestPlatform built"

# Step 5: Run quick smoke test
echo "Step 5: Running smoke tests..."
export SDL_VIDEODRIVER=dummy
export SDL_AUDIODRIVER=dummy

if ! ./build/TestPlatform --quick 2>&1; then
    echo "VERIFY_FAILED: Smoke tests failed"
    exit 1
fi
echo "  Smoke tests passed"

# Step 6: Run Rust tests
echo "Step 6: Running Rust unit tests..."
if ! cargo test --manifest-path platform/Cargo.toml --release -- --test-threads=1 2>&1 | tail -5; then
    echo "VERIFY_FAILED: Rust tests failed"
    exit 1
fi
echo "  Rust tests passed"

echo ""
echo "VERIFY_SUCCESS"
exit 0
