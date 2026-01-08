#!/bin/bash
# Master test runner script
#
# Usage: ./scripts/run-tests.sh [--quick] [--rust-only] [--cpp-only]

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# Parse arguments
QUICK=""
RUST_TESTS=true
CPP_TESTS=true

for arg in "$@"; do
    case "$arg" in
        --quick|-q)
            QUICK="--quick"
            ;;
        --rust-only)
            CPP_TESTS=false
            ;;
        --cpp-only)
            RUST_TESTS=false
            ;;
    esac
done

echo "============================================"
echo "Running Platform Tests"
echo "============================================"
echo ""

FAILURES=0

# Run Rust tests
if [ "$RUST_TESTS" = true ]; then
    echo "=== Rust Tests ==="
    echo ""

    cd "$ROOT_DIR"

    # Unit tests
    echo "Running unit tests..."
    if cargo test --manifest-path platform/Cargo.toml --release -- --test-threads=1; then
        echo "  Rust unit tests PASSED"
    else
        echo "  Rust unit tests FAILED"
        FAILURES=$((FAILURES + 1))
    fi
    echo ""
fi

# Build and run C++ tests
if [ "$CPP_TESTS" = true ]; then
    echo "=== C++ Integration Tests ==="
    echo ""

    cd "$ROOT_DIR"

    # Ensure project is built
    if [ ! -d "build" ]; then
        echo "Building project..."
        cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    fi
    cmake --build build

    # Check for test executable
    TEST_EXE="build/TestPlatform"
    if [ ! -f "$TEST_EXE" ]; then
        echo "WARNING: TestPlatform not found, skipping C++ tests"
    else
        echo "Running C++ integration tests..."

        # Set SDL to use dummy driver for headless testing
        export SDL_VIDEODRIVER=dummy
        export SDL_AUDIODRIVER=dummy

        if "$TEST_EXE" $QUICK; then
            echo "  C++ integration tests PASSED"
        else
            echo "  C++ integration tests FAILED"
            FAILURES=$((FAILURES + 1))
        fi
    fi
    echo ""
fi

# Summary
echo "============================================"
if [ $FAILURES -eq 0 ]; then
    echo "All tests PASSED"
    exit 0
else
    echo "$FAILURES test suite(s) FAILED"
    exit 1
fi
