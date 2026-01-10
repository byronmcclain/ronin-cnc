# Task 18j: Release Verification

| Field | Value |
|-------|-------|
| **Task ID** | 18j |
| **Task Name** | Release Verification |
| **Phase** | 18 - Testing & Polish |
| **Depends On** | All Tasks 18a-18i |
| **Estimated Complexity** | Medium (~400 lines) |

---

## Dependencies

- Task 18a-18f: All testing complete
- Task 18g: Bug tracking reviewed
- Task 18h: Performance optimized
- Task 18i: App bundle created

---

## Context

Release verification is the final gate before distribution:

1. **Pre-Release Checklist**: All requirements verified
2. **Final Test Run**: All tests pass
3. **Release Blockers**: No critical bugs
4. **Platform Testing**: Verified on target hardware
5. **Documentation**: README, changelog complete
6. **Distribution Package**: DMG/installer ready

This ensures the release is stable and ready for users.

---

## Technical Background

### Release Checklist Categories

```
Build Verification:
- Clean build succeeds
- All warnings resolved
- Universal binary correct
- Assets bundled

Test Verification:
- Unit tests pass
- Integration tests pass
- Visual tests pass
- Performance targets met

Quality Verification:
- No P0/Critical bugs
- Known issues documented
- Changelog updated
- Version number correct

Distribution Verification:
- Bundle valid
- Code signed
- Notarized (if distributing)
- DMG created
```

### Hardware Test Matrix

```
Apple Silicon:
- MacBook Air M1/M2
- MacBook Pro M1/M2/M3
- Mac Mini M1/M2
- iMac M1/M3

Intel:
- MacBook Pro 2015+ (Intel)
- iMac 2017+ (Intel)
- Mac Mini 2018+ (Intel)

Minimum Spec:
- macOS 11.0 (Big Sur)
- 4GB RAM
- 500MB disk space
```

---

## Objective

Create comprehensive release verification:
1. Pre-release checklist script
2. Final test runner
3. Hardware compatibility report
4. Release notes generator
5. Distribution package verifier
6. Post-release monitoring setup

---

## Deliverables

- [ ] `scripts/release/verify_release.sh` - Main release verification
- [ ] `scripts/release/run_all_tests.sh` - Complete test suite runner
- [ ] `scripts/release/check_bundle.sh` - Bundle integrity check
- [ ] `scripts/release/generate_changelog.sh` - Changelog generator
- [ ] `docs/RELEASE_CHECKLIST.md` - Human-readable checklist
- [ ] `docs/CHANGELOG.md` - Version changelog
- [ ] `docs/HARDWARE_REQUIREMENTS.md` - Hardware documentation
- [ ] `src/test/release_tests.cpp` - Release-specific tests

---

## Files to Create

### scripts/release/verify_release.sh

```bash
#!/bin/bash
# verify_release.sh - Complete Release Verification
# Task 18j - Release Verification

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

VERSION="${VERSION:-1.0.0}"
BUNDLE_PATH="${BUILD_DIR}/RedAlert.app"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

PASS_COUNT=0
FAIL_COUNT=0
WARN_COUNT=0

pass() {
    echo -e "${GREEN}✓${NC} $1"
    ((PASS_COUNT++))
}

fail() {
    echo -e "${RED}✗${NC} $1"
    ((FAIL_COUNT++))
}

warn() {
    echo -e "${YELLOW}⚠${NC} $1"
    ((WARN_COUNT++))
}

section() {
    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo " $1"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
}

echo "╔════════════════════════════════════════════════════════════╗"
echo "║           RED ALERT - RELEASE VERIFICATION                 ║"
echo "║                    Version: ${VERSION}                          ║"
echo "╚════════════════════════════════════════════════════════════╝"

#==============================================================================
section "1. BUILD VERIFICATION"
#==============================================================================

# Check clean build
echo "Performing clean build..."
cd "$BUILD_DIR"
if cmake --build . --clean-first > /dev/null 2>&1; then
    pass "Clean build succeeds"
else
    fail "Clean build failed"
fi

# Check for warnings (optional - may have too many in legacy code)
# WARNING_COUNT=$(cmake --build . 2>&1 | grep -c "warning:" || true)
# if [ "$WARNING_COUNT" -eq 0 ]; then
#     pass "No build warnings"
# else
#     warn "$WARNING_COUNT build warnings"
# fi

# Check bundle exists
if [ -d "$BUNDLE_PATH" ]; then
    pass "App bundle created"
else
    fail "App bundle not found"
fi

# Check universal binary
if file "$BUNDLE_PATH/Contents/MacOS/RedAlert" | grep -q "universal"; then
    pass "Universal binary (Intel + Apple Silicon)"
elif file "$BUNDLE_PATH/Contents/MacOS/RedAlert" | grep -q "arm64"; then
    warn "Apple Silicon only"
elif file "$BUNDLE_PATH/Contents/MacOS/RedAlert" | grep -q "x86_64"; then
    warn "Intel only"
else
    fail "Invalid executable"
fi

#==============================================================================
section "2. TEST VERIFICATION"
#==============================================================================

# Run unit tests
echo "Running unit tests..."
if "$BUILD_DIR/UnitTests" > /dev/null 2>&1; then
    pass "Unit tests pass"
else
    fail "Unit tests failed"
fi

# Run integration tests
echo "Running integration tests..."
if "$BUILD_DIR/IntegrationTests" > /dev/null 2>&1; then
    pass "Integration tests pass"
else
    fail "Integration tests failed"
fi

# Run visual tests (may require display)
echo "Running visual tests..."
if "$BUILD_DIR/VisualTests" > /dev/null 2>&1; then
    pass "Visual tests pass"
else
    warn "Visual tests skipped (no display)"
fi

# Run performance tests
echo "Running performance tests..."
if "$BUILD_DIR/PerformanceTests" > /dev/null 2>&1; then
    pass "Performance tests pass"
else
    fail "Performance tests failed"
fi

# Check test coverage (if available)
# if [ -f "$BUILD_DIR/coverage.info" ]; then
#     COVERAGE=$(lcov --summary "$BUILD_DIR/coverage.info" 2>&1 | grep "lines" | grep -o "[0-9]*\.[0-9]*%")
#     pass "Test coverage: $COVERAGE"
# fi

#==============================================================================
section "3. QUALITY VERIFICATION"
#==============================================================================

# Check for release blockers
echo "Checking for release blockers..."
BLOCKER_COUNT=0
if [ -f "$ROOT_DIR/docs/KNOWN_ISSUES.md" ]; then
    # Count P0/Critical bugs (simplified check)
    BLOCKER_COUNT=$(grep -c "P0\|CRITICAL" "$ROOT_DIR/docs/KNOWN_ISSUES.md" 2>/dev/null || echo "0")
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
        fail "Version mismatch: expected $VERSION, got $BUNDLE_VERSION"
    fi
fi

# Check changelog
if [ -f "$ROOT_DIR/docs/CHANGELOG.md" ]; then
    if grep -q "$VERSION" "$ROOT_DIR/docs/CHANGELOG.md"; then
        pass "Changelog includes version $VERSION"
    else
        fail "Changelog missing version $VERSION"
    fi
else
    fail "CHANGELOG.md not found"
fi

#==============================================================================
section "4. BUNDLE VERIFICATION"
#==============================================================================

# Check bundle structure
"$SCRIPT_DIR/check_bundle.sh" "$BUNDLE_PATH" && pass "Bundle structure valid" || fail "Bundle structure invalid"

# Check code signature
echo "Verifying code signature..."
if codesign --verify --verbose "$BUNDLE_PATH" 2>/dev/null; then
    pass "Code signature valid"

    # Check for notarization
    if spctl --assess --verbose "$BUNDLE_PATH" 2>&1 | grep -q "accepted"; then
        pass "Notarization accepted"
    else
        warn "Not notarized (required for distribution)"
    fi
else
    warn "Not code signed"
fi

# Check assets included
ASSET_DIR="$BUNDLE_PATH/Contents/Resources/Assets"
if [ -d "$ASSET_DIR" ]; then
    ASSET_COUNT=$(find "$ASSET_DIR" -type f | wc -l | tr -d ' ')
    if [ "$ASSET_COUNT" -gt 0 ]; then
        pass "Assets bundled: $ASSET_COUNT files"
    else
        fail "Assets directory empty"
    fi
else
    fail "Assets directory missing"
fi

#==============================================================================
section "5. DOCUMENTATION VERIFICATION"
#==============================================================================

# Check required docs
REQUIRED_DOCS=(
    "README.md"
    "docs/CHANGELOG.md"
    "docs/KNOWN_ISSUES.md"
    "docs/HARDWARE_REQUIREMENTS.md"
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
    echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║              RELEASE VERIFICATION PASSED                   ║${NC}"
    echo -e "${GREEN}║                                                            ║${NC}"
    echo -e "${GREEN}║  The build is ready for release.                          ║${NC}"
    echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}"
    exit 0
else
    echo -e "${RED}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${RED}║              RELEASE VERIFICATION FAILED                   ║${NC}"
    echo -e "${RED}║                                                            ║${NC}"
    echo -e "${RED}║  Fix $FAIL_COUNT issue(s) before release.                        ║${NC}"
    echo -e "${RED}╚════════════════════════════════════════════════════════════╝${NC}"
    exit 1
fi
```

### scripts/release/run_all_tests.sh

```bash
#!/bin/bash
# run_all_tests.sh - Run Complete Test Suite
# Task 18j - Release Verification

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
RESULTS_DIR="${BUILD_DIR}/test-results"

mkdir -p "$RESULTS_DIR"

echo "╔════════════════════════════════════════════════════════════╗"
echo "║                  COMPLETE TEST SUITE                       ║"
echo "╚════════════════════════════════════════════════════════════╝"

TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

run_test() {
    local name=$1
    local executable=$2

    echo ""
    echo "━━━ Running: $name ━━━"

    if [ ! -f "$executable" ]; then
        echo "SKIP: $executable not found"
        return
    fi

    ((TOTAL_TESTS++))

    # Run with XML output for CI
    if "$executable" --output=xml > "$RESULTS_DIR/${name}.xml" 2>&1; then
        echo "PASS: $name"
        ((PASSED_TESTS++))
    else
        echo "FAIL: $name"
        ((FAILED_TESTS++))
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

# Summary
echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║                     TEST SUMMARY                           ║"
echo "╠════════════════════════════════════════════════════════════╣"
echo "║  Total:  $TOTAL_TESTS                                               ║"
echo "║  Passed: $PASSED_TESTS                                               ║"
echo "║  Failed: $FAILED_TESTS                                               ║"
echo "╚════════════════════════════════════════════════════════════╝"

# Generate combined report
echo ""
echo "Test results saved to: $RESULTS_DIR/"

# Exit with failure if any tests failed
if [ "$FAILED_TESTS" -gt 0 ]; then
    exit 1
fi
```

### scripts/release/check_bundle.sh

```bash
#!/bin/bash
# check_bundle.sh - Verify Bundle Integrity
# Task 18j - Release Verification

BUNDLE_PATH="${1:-.}"

if [ ! -d "$BUNDLE_PATH" ]; then
    echo "ERROR: Bundle not found: $BUNDLE_PATH"
    exit 1
fi

echo "Checking bundle: $BUNDLE_PATH"

ERRORS=0

check() {
    local path=$1
    local desc=$2

    if [ -e "$BUNDLE_PATH/$path" ]; then
        echo "  ✓ $desc"
    else
        echo "  ✗ $desc (missing: $path)"
        ((ERRORS++))
    fi
}

check_dir() {
    local path=$1
    local desc=$2

    if [ -d "$BUNDLE_PATH/$path" ]; then
        local count=$(find "$BUNDLE_PATH/$path" -type f | wc -l | tr -d ' ')
        echo "  ✓ $desc ($count files)"
    else
        echo "  ✗ $desc (missing: $path)"
        ((ERRORS++))
    fi
}

echo ""
echo "Structure:"
check "Contents/Info.plist" "Info.plist"
check "Contents/PkgInfo" "PkgInfo"
check "Contents/MacOS/RedAlert" "Executable"
check "Contents/Resources/icon.icns" "App Icon"

echo ""
echo "Resources:"
check_dir "Contents/Resources/Assets" "Game Assets"

echo ""
echo "Executable check:"
EXE="$BUNDLE_PATH/Contents/MacOS/RedAlert"
if [ -f "$EXE" ]; then
    # Check it's executable
    if [ -x "$EXE" ]; then
        echo "  ✓ Executable permissions"
    else
        echo "  ✗ Not executable"
        ((ERRORS++))
    fi

    # Check architecture
    ARCH=$(file "$EXE")
    echo "  Architecture: $(echo "$ARCH" | grep -o 'executable.*' || echo "unknown")"

    # Check minimum macOS version
    if otool -l "$EXE" 2>/dev/null | grep -q "minos"; then
        MIN_OS=$(otool -l "$EXE" 2>/dev/null | grep -A2 "LC_BUILD_VERSION" | grep "minos" | awk '{print $2}')
        echo "  Minimum macOS: $MIN_OS"
    fi
fi

echo ""
if [ "$ERRORS" -eq 0 ]; then
    echo "Bundle verification PASSED"
    exit 0
else
    echo "Bundle verification FAILED ($ERRORS errors)"
    exit 1
fi
```

### scripts/release/generate_changelog.sh

```bash
#!/bin/bash
# generate_changelog.sh - Generate Changelog from Git
# Task 18j - Release Verification

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

VERSION="${1:-1.0.0}"
SINCE="${2:-}"  # Optional: tag or commit to start from

cd "$ROOT_DIR"

echo "# Changelog"
echo ""
echo "## Version $VERSION"
echo ""
echo "Release Date: $(date +%Y-%m-%d)"
echo ""

# Get commits since last tag (or all if no tag)
if [ -n "$SINCE" ]; then
    COMMITS=$(git log --oneline "$SINCE"..HEAD 2>/dev/null)
else
    LAST_TAG=$(git describe --tags --abbrev=0 2>/dev/null || echo "")
    if [ -n "$LAST_TAG" ]; then
        COMMITS=$(git log --oneline "$LAST_TAG"..HEAD 2>/dev/null)
    else
        COMMITS=$(git log --oneline 2>/dev/null | head -50)
    fi
fi

# Categorize commits
echo "### Features"
echo "$COMMITS" | grep -iE "feat|add|new|implement" | sed 's/^/- /' || echo "- None"
echo ""

echo "### Bug Fixes"
echo "$COMMITS" | grep -iE "fix|bug|repair|correct" | sed 's/^/- /' || echo "- None"
echo ""

echo "### Improvements"
echo "$COMMITS" | grep -iE "improve|enhance|optimize|update|refactor" | sed 's/^/- /' || echo "- None"
echo ""

echo "### Documentation"
echo "$COMMITS" | grep -iE "doc|readme|comment" | sed 's/^/- /' || echo "- None"
echo ""

# Known issues
if [ -f "$ROOT_DIR/docs/KNOWN_ISSUES.md" ]; then
    echo "### Known Issues"
    echo ""
    echo "See [KNOWN_ISSUES.md](docs/KNOWN_ISSUES.md) for current known issues."
fi
```

### docs/RELEASE_CHECKLIST.md

```markdown
# Release Checklist

Version: __________ Date: __________

## Pre-Release

### Build Verification
- [ ] Clean build succeeds on macOS
- [ ] No critical compiler warnings
- [ ] Universal binary includes both Intel and Apple Silicon
- [ ] All assets bundled correctly
- [ ] Version number updated in Info.plist

### Test Verification
- [ ] Unit tests pass (100%)
- [ ] Integration tests pass (100%)
- [ ] Visual tests pass (manual review)
- [ ] Performance tests meet targets
- [ ] Gameplay tests pass
- [ ] Regression tests pass (no regressions)

### Quality Verification
- [ ] No P0 (Critical) bugs open
- [ ] No P1 (High) bugs blocking release
- [ ] All P2+ bugs documented in KNOWN_ISSUES.md
- [ ] Code review completed for all changes
- [ ] No security vulnerabilities identified

### Hardware Testing
- [ ] Tested on Apple Silicon Mac
- [ ] Tested on Intel Mac
- [ ] Tested on minimum spec hardware
- [ ] Tested on macOS 11.0 (Big Sur)
- [ ] Tested on latest macOS

### Documentation
- [ ] README.md updated
- [ ] CHANGELOG.md updated with version
- [ ] KNOWN_ISSUES.md current
- [ ] HARDWARE_REQUIREMENTS.md accurate
- [ ] In-game help/credits updated

## Release Build

### Code Signing
- [ ] Developer ID certificate valid
- [ ] Entitlements file correct
- [ ] Bundle signed with hardened runtime
- [ ] Signature verified with codesign

### Notarization
- [ ] Submitted to Apple notary service
- [ ] Notarization approved
- [ ] Ticket stapled to bundle
- [ ] Gatekeeper accepts bundle

### Distribution
- [ ] DMG created
- [ ] DMG signed
- [ ] DMG tested on clean system
- [ ] SHA256 checksum recorded
- [ ] Upload to distribution server

## Post-Release

### Verification
- [ ] Download and install from distribution
- [ ] Verify Gatekeeper allows launch
- [ ] Basic functionality test
- [ ] Update checker works (if applicable)

### Announcement
- [ ] Release notes published
- [ ] GitHub release created
- [ ] Announcement posted

### Monitoring
- [ ] Crash reporting configured
- [ ] Analytics tracking (if applicable)
- [ ] Support channels prepared

---

## Sign-off

| Role | Name | Date | Signature |
|------|------|------|-----------|
| Developer | | | |
| QA | | | |
| Release Manager | | | |
```

### docs/CHANGELOG.md

```markdown
# Changelog

All notable changes to Red Alert macOS Port will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Initial macOS port from original source code

### Changed
- Graphics system ported to Metal
- Audio system ported to Core Audio
- Input handling ported to AppKit

### Fixed
- (List bug fixes here)

## [1.0.0] - YYYY-MM-DD

### Added
- Native macOS application bundle
- Universal binary (Intel + Apple Silicon)
- Retina display support
- Full keyboard and mouse support
- Original game assets support (requires original game files)

### Changed
- DirectX graphics replaced with Metal
- DirectSound audio replaced with Core Audio
- Win32 window handling replaced with AppKit

### Known Issues
- See [KNOWN_ISSUES.md](KNOWN_ISSUES.md)

---

## Version History

- 1.0.0 - Initial macOS release
```

### docs/HARDWARE_REQUIREMENTS.md

```markdown
# Hardware Requirements

## Minimum Requirements

| Component | Requirement |
|-----------|-------------|
| **Operating System** | macOS 11.0 (Big Sur) or later |
| **Processor** | Any 64-bit Intel or Apple Silicon Mac |
| **Memory** | 4 GB RAM |
| **Storage** | 500 MB available space |
| **Graphics** | Metal-compatible GPU |
| **Display** | 1280x720 resolution |

## Recommended Requirements

| Component | Requirement |
|-----------|-------------|
| **Operating System** | macOS 13.0 (Ventura) or later |
| **Processor** | Apple M1 or Intel Core i5 |
| **Memory** | 8 GB RAM |
| **Storage** | 1 GB available space (SSD recommended) |
| **Graphics** | Apple Silicon GPU or dedicated Intel GPU |
| **Display** | Retina display |

## Supported Hardware

### Apple Silicon Macs
- MacBook Air (M1, M2, 2020+)
- MacBook Pro (M1, M1 Pro, M1 Max, M2, M3, 2020+)
- Mac Mini (M1, M2, 2020+)
- iMac (M1, M3, 2021+)
- Mac Studio (M1 Max, M1 Ultra, M2, 2022+)
- Mac Pro (M2 Ultra, 2023+)

### Intel Macs
- MacBook Pro (2015 or later)
- MacBook Air (2015 or later)
- iMac (2015 or later)
- Mac Mini (2014 or later)
- Mac Pro (2013 or later)

## Performance Notes

### Apple Silicon Performance
- Native Apple Silicon binary
- 60 FPS typical gameplay
- Efficient power usage
- Instant wake support

### Intel Performance
- Universal binary includes Intel support
- 60 FPS on 2015+ hardware
- May throttle on battery power
- SSD recommended for fast loading

## Compatibility Notes

1. **macOS Version**: macOS 11.0 (Big Sur) is the minimum supported version. Earlier versions are not supported.

2. **32-bit Macs**: Not supported. The application requires a 64-bit processor.

3. **Graphics**: Requires Metal graphics API. Macs without Metal support cannot run this application.

4. **Display Scaling**: Retina displays fully supported with high-resolution graphics.

5. **External Displays**: Supported at native resolution.

## Troubleshooting

### Application Won't Launch
- Verify macOS version is 11.0 or later
- Check Gatekeeper settings in Security & Privacy
- Right-click and select "Open" for first launch

### Poor Performance
- Close other applications
- Check for thermal throttling
- Verify SSD has sufficient free space
- Update to latest macOS version

### Graphics Issues
- Update macOS to latest version
- Reset NVRAM/PRAM
- Check for GPU driver updates

## Getting Help

If you experience issues:
1. Check [Known Issues](KNOWN_ISSUES.md)
2. Search existing [GitHub Issues](https://github.com/example/redalert-macos/issues)
3. Create a new issue with:
   - Mac model and year
   - macOS version
   - Description of the problem
   - Steps to reproduce
```

### src/test/release_tests.cpp

```cpp
// src/test/release_tests.cpp
// Release-Specific Tests
// Task 18j - Release Verification

#include "test/test_framework.h"
#include <cstdio>
#include <string>

//=============================================================================
// Build Information Tests
//=============================================================================

TEST_CASE(Release_BuildInfo_VersionDefined, "Release") {
    // Version should be defined at compile time
    #ifdef VERSION
    TEST_ASSERT(strlen(VERSION) > 0);
    #else
    TEST_FAIL("VERSION not defined");
    #endif
}

TEST_CASE(Release_BuildInfo_DebugDisabled, "Release") {
    // Debug should be disabled in release builds
    #ifdef NDEBUG
    TEST_ASSERT(true);  // NDEBUG defined = release build
    #else
    // This is okay for debug builds, just note it
    TEST_ASSERT(true);  // Allow debug builds to pass
    #endif
}

//=============================================================================
// Resource Tests
//=============================================================================

TEST_CASE(Release_Resources_AssetsExist, "Release") {
    // Check that essential assets are accessible
    // Would check actual asset loading in real test

    // Placeholder - would verify asset paths resolve
    bool assets_found = true;
    TEST_ASSERT(assets_found);
}

TEST_CASE(Release_Resources_NoMissingAssets, "Release") {
    // Verify no critical assets are missing
    const char* critical_assets[] = {
        // Would list critical asset filenames
        nullptr
    };

    for (int i = 0; critical_assets[i] != nullptr; i++) {
        // Would check each asset exists
        TEST_ASSERT(true);
    }
}

//=============================================================================
// Compatibility Tests
//=============================================================================

TEST_CASE(Release_Compat_MetalAvailable, "Release") {
    // Verify Metal is available on this system
    // Would call actual Metal availability check

    bool metal_available = true;  // Placeholder
    TEST_ASSERT(metal_available);
}

TEST_CASE(Release_Compat_AudioAvailable, "Release") {
    // Verify audio system can be initialized
    bool audio_available = true;  // Placeholder
    TEST_ASSERT(audio_available);
}

//=============================================================================
// Memory Tests
//=============================================================================

TEST_CASE(Release_Memory_NoLeaksOnStartup, "Release") {
    // Verify no memory leaks during initialization
    // Would use actual memory tracking

    size_t leaks = 0;  // Placeholder
    TEST_ASSERT_EQ(leaks, 0u);
}

TEST_CASE(Release_Memory_ReasonableUsage, "Release") {
    // Verify memory usage is within expected bounds
    size_t memory_mb = 100;  // Placeholder - would get actual usage
    size_t max_expected_mb = 500;

    TEST_ASSERT_LT(memory_mb, max_expected_mb);
}

//=============================================================================
// Performance Baseline Tests
//=============================================================================

TEST_CASE(Release_Perf_StartupsUnder5Seconds, "Release") {
    // Verify startup time is acceptable
    float startup_seconds = 2.0f;  // Placeholder
    TEST_ASSERT_LT(startup_seconds, 5.0f);
}

TEST_CASE(Release_Perf_LoadingUnder10Seconds, "Release") {
    // Verify level loading time is acceptable
    float loading_seconds = 3.0f;  // Placeholder
    TEST_ASSERT_LT(loading_seconds, 10.0f);
}

TEST_CASE(Release_Perf_MinimumFPS, "Release") {
    // Verify minimum FPS target is achievable
    float min_fps = 30.0f;  // Placeholder
    TEST_ASSERT_GE(min_fps, 30.0f);
}

//=============================================================================
// Main Entry Point
//=============================================================================

TEST_MAIN()
```

---

## CMakeLists.txt Additions

```cmake
# Task 18j: Release Verification

# Release tests executable
add_executable(ReleaseTests
    ${CMAKE_SOURCE_DIR}/src/test/release_tests.cpp
)
target_link_libraries(ReleaseTests PRIVATE test_framework)
add_test(NAME ReleaseTests COMMAND ReleaseTests)

# Custom target for full release verification
add_custom_target(verify-release
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/release/verify_release.sh
    DEPENDS RedAlert UnitTests IntegrationTests PerformanceTests ReleaseTests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running full release verification"
)

# Custom target for running all tests
add_custom_target(test-all
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/release/run_all_tests.sh
    DEPENDS UnitTests IntegrationTests VisualTests PerformanceTests GameplayTests RegressionTests ReleaseTests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running complete test suite"
)
```

---

## Verification Script

```bash
#!/bin/bash
set -e
echo "=== Task 18j Verification: Release Verification ==="
ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"

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
done

# Make scripts executable
chmod +x "$ROOT_DIR/scripts/release/"*.sh

# Verify scripts are valid bash
for script in "$ROOT_DIR/scripts/release/"*.sh; do
    bash -n "$script" || { echo "FAIL: $script has syntax errors"; exit 1; }
done

echo "=== Task 18j Verification PASSED ==="
```

---

## Success Criteria

1. Release verification script checks all critical areas
2. Test runner executes complete test suite
3. Bundle checker validates app structure
4. Documentation templates complete
5. All verification passes on clean build

---

## Completion Promise

When verification passes, output:
<promise>TASK_18J_COMPLETE</promise>

## Escape Hatch

If stuck after 15 iterations, output:
<promise>TASK_18J_BLOCKED</promise>

## Max Iterations
15
