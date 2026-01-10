#!/bin/bash
set -e
echo "=== Task 18a Verification: Test Framework Foundation ==="

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"

echo "Checking files..."
for f in include/test/test_framework.h \
         include/test/test_fixtures.h \
         include/test/test_reporter.h \
         src/test/test_framework.cpp \
         src/test/test_fixtures.cpp \
         src/test/test_reporter.cpp; do
    [ -f "$ROOT_DIR/$f" ] || { echo "Missing: $f"; exit 1; }
    echo "  Found: $f"
done

echo ""
echo "Building..."
cd "$BUILD_DIR" && cmake --build . --target test_framework > /dev/null 2>&1
cmake --build . --target TestFrameworkSelfTest > /dev/null 2>&1
echo "  Built test_framework and TestFrameworkSelfTest"

echo ""
echo "Running self-test..."
"$BUILD_DIR/TestFrameworkSelfTest" || { echo "Self-test failed"; exit 1; }

echo ""
echo "Testing XML output..."
"$BUILD_DIR/TestFrameworkSelfTest" --xml /tmp/test_results.xml > /dev/null 2>&1
[ -f /tmp/test_results.xml ] || { echo "XML output not created"; exit 1; }
echo "  XML output: /tmp/test_results.xml"

echo ""
echo "Testing filtering..."
"$BUILD_DIR/TestFrameworkSelfTest" --category Assertions > /dev/null 2>&1
echo "  Category filtering works"

echo ""
echo "Testing list mode..."
"$BUILD_DIR/TestFrameworkSelfTest" --list > /dev/null 2>&1
echo "  List mode works"

echo ""
echo "=== Task 18a Verification PASSED ==="
