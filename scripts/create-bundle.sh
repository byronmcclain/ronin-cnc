#!/bin/bash
#
# Create macOS .app bundle for Red Alert
#
# Usage: ./scripts/create-bundle.sh [build_dir] [output_dir]
#

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

BUILD_DIR="${1:-$ROOT_DIR/build}"
OUTPUT_DIR="${2:-$ROOT_DIR}"

# Configuration
APP_NAME="RedAlert"
BUNDLE_NAME="$APP_NAME.app"
BUNDLE_ID="com.westwood.redalert"
VERSION="1.0.0"
BUILD_NUMBER="1"

echo "=== Creating macOS App Bundle ==="
echo "Build dir: $BUILD_DIR"
echo "Output dir: $OUTPUT_DIR"

# Create bundle structure
BUNDLE_PATH="$OUTPUT_DIR/$BUNDLE_NAME"
rm -rf "$BUNDLE_PATH"
mkdir -p "$BUNDLE_PATH/Contents/MacOS"
mkdir -p "$BUNDLE_PATH/Contents/Resources"
mkdir -p "$BUNDLE_PATH/Contents/Frameworks"

# Copy executable
if [ -f "$BUILD_DIR/$APP_NAME" ]; then
    cp "$BUILD_DIR/$APP_NAME" "$BUNDLE_PATH/Contents/MacOS/"
    chmod +x "$BUNDLE_PATH/Contents/MacOS/$APP_NAME"
    echo "Copied executable"
else
    echo "Warning: Executable not found at $BUILD_DIR/$APP_NAME"
fi

# Generate Info.plist
PLIST_TEMPLATE="$ROOT_DIR/platform/macos/Info.plist.in"
if [ -f "$PLIST_TEMPLATE" ]; then
    sed -e "s/@EXECUTABLE_NAME@/$APP_NAME/g" \
        -e "s/@BUNDLE_IDENTIFIER@/$BUNDLE_ID/g" \
        -e "s/@VERSION@/$VERSION/g" \
        -e "s/@BUILD_NUMBER@/$BUILD_NUMBER/g" \
        "$PLIST_TEMPLATE" > "$BUNDLE_PATH/Contents/Info.plist"
    echo "Generated Info.plist"
else
    # Create minimal Info.plist
    cat > "$BUNDLE_PATH/Contents/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>$APP_NAME</string>
    <key>CFBundleIdentifier</key>
    <string>$BUNDLE_ID</string>
    <key>CFBundleName</key>
    <string>Red Alert</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleVersion</key>
    <string>$VERSION</string>
    <key>NSHighResolutionCapable</key>
    <true/>
</dict>
</plist>
EOF
    echo "Created minimal Info.plist"
fi

# Create PkgInfo
echo -n "APPL????" > "$BUNDLE_PATH/Contents/PkgInfo"

# Copy data files if they exist
if [ -d "$ROOT_DIR/data" ]; then
    cp -R "$ROOT_DIR/data" "$BUNDLE_PATH/Contents/Resources/"
    echo "Copied data files"
fi

# Copy SDL2 framework if present
SDL2_FRAMEWORK="/Library/Frameworks/SDL2.framework"
if [ -d "$SDL2_FRAMEWORK" ]; then
    cp -R "$SDL2_FRAMEWORK" "$BUNDLE_PATH/Contents/Frameworks/"
    echo "Copied SDL2 framework"
fi

# Fix dylib paths (if needed)
if [ -f "$BUNDLE_PATH/Contents/MacOS/$APP_NAME" ]; then
    # Update SDL2 rpath
    install_name_tool -add_rpath "@executable_path/../Frameworks" \
        "$BUNDLE_PATH/Contents/MacOS/$APP_NAME" 2>/dev/null || true
fi

echo ""
echo "Bundle created at: $BUNDLE_PATH"
echo ""

# Verify bundle
if [ -f "$BUNDLE_PATH/Contents/MacOS/$APP_NAME" ]; then
    echo "Bundle contents:"
    ls -la "$BUNDLE_PATH/Contents/"
    ls -la "$BUNDLE_PATH/Contents/MacOS/"
else
    echo "Note: Executable not present - bundle structure only"
fi
