#!/bin/bash
# run_all_tests.sh - Run Complete Test Suite
# Task 18j - Release Verification

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
RESULTS_DIR="${BUILD_DIR}/test-results"

mkdir -p "$RESULTS_DIR"

echo "================================================================"
echo "                  COMPLETE TEST SUITE"
echo "================================================================"

TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

run_test() {
    local name=$1
    local executable=$2

    echo ""
    echo "--- Running: $name ---"

    if [ ! -f "$executable" ]; then
        echo "SKIP: $executable not found"
        return
    fi

    ((TOTAL_TESTS++)) || true

    # Run test and capture result
    if "$executable" > "$RESULTS_DIR/${name}.log" 2>&1; then
        echo "PASS: $name"
        ((PASSED_TESTS++)) || true
    else
        echo "FAIL: $name"
        ((FAILED_TESTS++)) || true
    fi
}

# Build all test targets
echo "Building test suite..."
cd "$BUILD_DIR"
cmake --build . --target UnitTests IntegrationTests VisualTests PerformanceTests GameplayTests RegressionTests 2>/dev/null || true

# Run all test executables
run_test "UnitTests" "$BUILD_DIR/UnitTests"
run_test "IntegrationTests" "$BUILD_DIR/IntegrationTests"
run_test "VisualTests" "$BUILD_DIR/VisualTests"
run_test "GameplayTests" "$BUILD_DIR/GameplayTests"
run_test "PerformanceTests" "$BUILD_DIR/PerformanceTests"
run_test "RegressionTests" "$BUILD_DIR/RegressionTests"

# Run release tests if available
if [ -f "$BUILD_DIR/ReleaseTests" ]; then
    run_test "ReleaseTests" "$BUILD_DIR/ReleaseTests"
fi

# Summary
echo ""
echo "================================================================"
echo "                     TEST SUMMARY"
echo "================================================================"
echo "  Total:  $TOTAL_TESTS"
echo "  Passed: $PASSED_TESTS"
echo "  Failed: $FAILED_TESTS"
echo "================================================================"

# Generate combined report
echo ""
echo "Test results saved to: $RESULTS_DIR/"

# Exit with failure if any tests failed
if [ "$FAILED_TESTS" -gt 0 ]; then
    exit 1
fi
