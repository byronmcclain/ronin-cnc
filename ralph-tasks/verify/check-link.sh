#!/bin/bash
# Verification script for mixed C++/Rust linking

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Mixed Link ==="

cd "$ROOT_DIR"

# Build everything
if ! cmake --build build 2>&1; then
    echo "VERIFY_FAILED: Build failed"
    exit 1
fi

# Check executable exists (could be in bundle or directly)
EXECUTABLE=""
if [ -f build/RedAlert.app/Contents/MacOS/RedAlert ]; then
    EXECUTABLE="build/RedAlert.app/Contents/MacOS/RedAlert"
elif [ -f build/RedAlert ]; then
    EXECUTABLE="build/RedAlert"
fi

if [ -z "$EXECUTABLE" ]; then
    echo "VERIFY_FAILED: Executable not found"
    exit 1
fi

# Check executable is valid
if ! file "$EXECUTABLE" | grep -q "Mach-O"; then
    echo "VERIFY_FAILED: Not a valid Mach-O executable"
    exit 1
fi

# Try to run it (should init and shutdown cleanly)
# Note: This assumes the executable exits cleanly
if timeout 5 "$EXECUTABLE" --test-init 2>&1 || [ $? -eq 0 ]; then
    echo "VERIFY_SUCCESS"
    exit 0
else
    echo "VERIFY_FAILED: Executable did not run cleanly"
    exit 1
fi
