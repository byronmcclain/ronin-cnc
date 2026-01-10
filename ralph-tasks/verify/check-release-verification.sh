#!/bin/bash
set -e
echo "=== Task 18j Verification: Release Verification ==="
ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"

# Check all required files exist
for file in \
    "scripts/release/verify_release.sh" \
    "scripts/release/run_all_tests.sh" \
    "scripts/release/check_bundle.sh" \
    "scripts/release/generate_changelog.sh" \
    "docs/RELEASE_CHECKLIST.md" \
    "docs/CHANGELOG.md" \
    "docs/HARDWARE_REQUIREMENTS.md" \
    "src/test/release_tests.cpp"
do
    test -f "$ROOT_DIR/$file" || { echo "FAIL: $file missing"; exit 1; }
    echo "  Found: $file"
done

# Verify scripts are valid bash
echo ""
echo "Validating shell scripts..."
for script in "$ROOT_DIR/scripts/release/"*.sh; do
    bash -n "$script" || { echo "FAIL: $script has syntax errors"; exit 1; }
    echo "  Valid: $(basename "$script")"
done

# Build and run release tests
echo ""
echo "Building ReleaseTests..."
cd "$BUILD_DIR"
cmake .. > /dev/null 2>&1
cmake --build . --target ReleaseTests > /dev/null 2>&1

echo "Running ReleaseTests..."
if ./ReleaseTests 2>&1; then
    echo "  ReleaseTests PASSED"
else
    echo "FAIL: ReleaseTests failed"
    exit 1
fi

echo ""
echo "=== Task 18j Verification PASSED ==="
