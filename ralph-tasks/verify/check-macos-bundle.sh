#!/bin/bash
set -e
echo "=== Task 18i Verification: macOS App Bundle ==="
ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"

# Check required files
for file in \
    "cmake/MacOSBundle.cmake" \
    "resources/macos/Info.plist.in" \
    "resources/macos/PkgInfo" \
    "resources/macos/Entitlements.plist" \
    "scripts/create_bundle.sh" \
    "scripts/sign_bundle.sh" \
    "scripts/create_dmg.sh"
do
    test -f "$ROOT_DIR/$file" || { echo "FAIL: $file missing"; exit 1; }
    echo "  Found: $file"
done

# Make scripts executable
chmod +x "$ROOT_DIR/scripts/"*.sh

# Reconfigure CMake to pick up new bundle settings
echo ""
echo "Reconfiguring CMake..."
cd "$BUILD_DIR" && cmake ..

# Build bundle
echo ""
echo "Building RedAlertGame bundle..."
cmake --build . --target RedAlertGame

# Verify bundle structure
BUNDLE="$BUILD_DIR/RedAlertGame.app"
if [ -d "$BUNDLE" ]; then
    echo ""
    echo "Verifying bundle structure..."
    test -f "$BUNDLE/Contents/Info.plist" || { echo "FAIL: Info.plist missing"; exit 1; }
    echo "  Found: Contents/Info.plist"
    test -f "$BUNDLE/Contents/MacOS/RedAlertGame" || { echo "FAIL: Executable missing"; exit 1; }
    echo "  Found: Contents/MacOS/RedAlertGame"

    # Check optional files
    if [ -f "$BUNDLE/Contents/PkgInfo" ]; then
        echo "  Found: Contents/PkgInfo"
    fi
    if [ -f "$BUNDLE/Contents/Resources/icon.icns" ]; then
        echo "  Found: Contents/Resources/icon.icns"
    fi

    # Verify executable
    echo ""
    echo "Executable info:"
    file "$BUNDLE/Contents/MacOS/RedAlertGame"

    echo ""
    echo "Bundle structure verified successfully"
else
    echo "FAIL: Bundle not created at $BUNDLE"
    exit 1
fi

echo ""
echo "=== Task 18i Verification PASSED ==="
