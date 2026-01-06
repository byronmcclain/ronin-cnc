#!/bin/bash
# Verification script for cbindgen header generation

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."
HEADER="$ROOT_DIR/include/platform.h"

echo "=== Checking cbindgen Header ==="

# Generate header
cd "$ROOT_DIR/platform"
if ! cargo build --features cbindgen-run 2>&1; then
    echo "VERIFY_FAILED: Cargo build with cbindgen failed"
    exit 1
fi

# Check header exists
if [ ! -f "$HEADER" ]; then
    echo "VERIFY_FAILED: platform.h not found at $HEADER"
    exit 1
fi

# Check for expected symbols
REQUIRED_SYMBOLS="Platform_Init Platform_Shutdown Platform_GetVersion Platform_GetLastError"
for sym in $REQUIRED_SYMBOLS; do
    if ! grep -q "$sym" "$HEADER"; then
        echo "VERIFY_FAILED: Symbol '$sym' not found in header"
        exit 1
    fi
done

# Check header compiles with C compiler
if ! clang -fsyntax-only -x c "$HEADER" 2>&1; then
    echo "VERIFY_FAILED: Header does not compile"
    exit 1
fi

# Check header compiles with C++ compiler
if ! clang++ -fsyntax-only -x c++ "$HEADER" 2>&1; then
    echo "VERIFY_FAILED: Header does not compile as C++"
    exit 1
fi

echo "VERIFY_SUCCESS"
exit 0
