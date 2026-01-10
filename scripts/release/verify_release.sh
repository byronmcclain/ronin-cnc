#!/bin/bash
# verify_release.sh - Complete Release Verification
# Task 18j - Release Verification

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

VERSION="${VERSION:-1.0.0}"
BUNDLE_PATH="${BUILD_DIR}/RedAlertGame.app"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

PASS_COUNT=0
FAIL_COUNT=0
WARN_COUNT=0

pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((PASS_COUNT++))
}

fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    ((FAIL_COUNT++))
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
    ((WARN_COUNT++))
}

section() {
    echo ""
    echo "================================================================"
    echo " $1"
    echo "================================================================"
}

echo "================================================================"
echo "           RED ALERT - RELEASE VERIFICATION"
echo "                    Version: ${VERSION}"
echo "================================================================"

#==============================================================================
section "1. BUILD VERIFICATION"
#==============================================================================

# Check clean build
echo "Performing build..."
cd "$BUILD_DIR"
if cmake --build . --target RedAlertGame > /dev/null 2>&1; then
    pass "Build succeeds"
else
    fail "Build failed"
fi

# Check bundle exists
if [ -d "$BUNDLE_PATH" ]; then
    pass "App bundle created"
else
    fail "App bundle not found"
fi

# Check executable exists
if [ -f "$BUNDLE_PATH/Contents/MacOS/RedAlertGame" ]; then
    pass "Executable exists"

    # Check architecture
    if file "$BUNDLE_PATH/Contents/MacOS/RedAlertGame" | grep -q "universal"; then
        pass "Universal binary (Intel + Apple Silicon)"
    elif file "$BUNDLE_PATH/Contents/MacOS/RedAlertGame" | grep -q "arm64"; then
        warn "Apple Silicon only (not universal)"
    elif file "$BUNDLE_PATH/Contents/MacOS/RedAlertGame" | grep -q "x86_64"; then
        warn "Intel only (not universal)"
    else
        fail "Invalid executable"
    fi
else
    fail "Executable missing"
fi

#==============================================================================
section "2. TEST VERIFICATION"
#==============================================================================

# Run unit tests
echo "Running unit tests..."
if [ -f "$BUILD_DIR/UnitTests" ] && "$BUILD_DIR/UnitTests" > /dev/null 2>&1; then
    pass "Unit tests pass"
else
    fail "Unit tests failed or not built"
fi

# Run integration tests
echo "Running integration tests..."
if [ -f "$BUILD_DIR/IntegrationTests" ] && "$BUILD_DIR/IntegrationTests" > /dev/null 2>&1; then
    pass "Integration tests pass"
else
    fail "Integration tests failed or not built"
fi

# Run performance tests
echo "Running performance tests..."
if [ -f "$BUILD_DIR/PerformanceTests" ] && "$BUILD_DIR/PerformanceTests" > /dev/null 2>&1; then
    pass "Performance tests pass"
else
    fail "Performance tests failed or not built"
fi

# Run gameplay tests
echo "Running gameplay tests..."
if [ -f "$BUILD_DIR/GameplayTests" ] && "$BUILD_DIR/GameplayTests" > /dev/null 2>&1; then
    pass "Gameplay tests pass"
else
    fail "Gameplay tests failed or not built"
fi

# Run regression tests
echo "Running regression tests..."
if [ -f "$BUILD_DIR/RegressionTests" ] && "$BUILD_DIR/RegressionTests" > /dev/null 2>&1; then
    pass "Regression tests pass"
else
    fail "Regression tests failed or not built"
fi

#==============================================================================
section "3. QUALITY VERIFICATION"
#==============================================================================

# Check for release blockers
echo "Checking for release blockers..."
BLOCKER_COUNT=0
if [ -f "$ROOT_DIR/docs/KNOWN_ISSUES.md" ]; then
    # Count P0/Critical bugs
    BLOCKER_COUNT=$(grep -c "P0\|CRITICAL\|BLOCKER" "$ROOT_DIR/docs/KNOWN_ISSUES.md" 2>/dev/null || echo "0")
fi

if [ "$BLOCKER_COUNT" -eq 0 ]; then
    pass "No release blockers"
else
    fail "$BLOCKER_COUNT release blockers found"
fi

# Check version number
INFO_PLIST="$BUNDLE_PATH/Contents/Info.plist"
if [ -f "$INFO_PLIST" ]; then
    BUNDLE_VERSION=$(/usr/libexec/PlistBuddy -c "Print :CFBundleShortVersionString" "$INFO_PLIST" 2>/dev/null || echo "unknown")
    if [ "$BUNDLE_VERSION" = "$VERSION" ]; then
        pass "Version number correct: $VERSION"
    else
        warn "Version mismatch: expected $VERSION, got $BUNDLE_VERSION"
    fi
fi

# Check changelog
if [ -f "$ROOT_DIR/docs/CHANGELOG.md" ]; then
    pass "CHANGELOG.md exists"
else
    fail "CHANGELOG.md missing"
fi

#==============================================================================
section "4. BUNDLE VERIFICATION"
#==============================================================================

# Check bundle structure using check_bundle.sh
if [ -f "$SCRIPT_DIR/check_bundle.sh" ]; then
    if "$SCRIPT_DIR/check_bundle.sh" "$BUNDLE_PATH" > /dev/null 2>&1; then
        pass "Bundle structure valid"
    else
        fail "Bundle structure invalid"
    fi
fi

# Check code signature
echo "Verifying code signature..."
if codesign --verify --verbose "$BUNDLE_PATH" 2>/dev/null; then
    pass "Code signature valid"
else
    warn "Not code signed (required for distribution)"
fi

#==============================================================================
section "5. DOCUMENTATION VERIFICATION"
#==============================================================================

# Check required docs
REQUIRED_DOCS=(
    "docs/CHANGELOG.md"
    "docs/KNOWN_ISSUES.md"
    "docs/HARDWARE_REQUIREMENTS.md"
    "docs/RELEASE_CHECKLIST.md"
)

for doc in "${REQUIRED_DOCS[@]}"; do
    if [ -f "$ROOT_DIR/$doc" ]; then
        pass "$doc exists"
    else
        fail "$doc missing"
    fi
done

#==============================================================================
section "6. SUMMARY"
#==============================================================================

echo ""
echo "Results:"
echo "  Passed:   $PASS_COUNT"
echo "  Failed:   $FAIL_COUNT"
echo "  Warnings: $WARN_COUNT"
echo ""

if [ "$FAIL_COUNT" -eq 0 ]; then
    echo -e "${GREEN}================================================================${NC}"
    echo -e "${GREEN}              RELEASE VERIFICATION PASSED${NC}"
    echo -e "${GREEN}              The build is ready for release.${NC}"
    echo -e "${GREEN}================================================================${NC}"
    exit 0
else
    echo -e "${RED}================================================================${NC}"
    echo -e "${RED}              RELEASE VERIFICATION FAILED${NC}"
    echo -e "${RED}              Fix $FAIL_COUNT issue(s) before release.${NC}"
    echo -e "${RED}================================================================${NC}"
    exit 1
fi
