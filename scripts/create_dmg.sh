#!/bin/bash
# create_dmg.sh - Create DMG for Distribution
# Task 18i - macOS App Bundle

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

BUNDLE_PATH="${1:-${BUILD_DIR}/RedAlert.app}"
DMG_PATH="${2:-${BUILD_DIR}/RedAlert.dmg}"

BUNDLE_NAME=$(basename "$BUNDLE_PATH" .app)
VOLUME_NAME="Red Alert"
TEMP_DIR=$(mktemp -d)

echo "=== Creating DMG ==="
echo "Bundle: ${BUNDLE_PATH}"
echo "DMG: ${DMG_PATH}"
echo ""

if [ ! -d "$BUNDLE_PATH" ]; then
    echo "ERROR: Bundle not found"
    exit 1
fi

# Remove existing DMG
rm -f "$DMG_PATH"

# Create staging directory
echo "Creating staging directory..."
mkdir -p "${TEMP_DIR}/dmg"
cp -R "$BUNDLE_PATH" "${TEMP_DIR}/dmg/"

# Create Applications symlink
ln -s /Applications "${TEMP_DIR}/dmg/Applications"

# Optional: Add README
cat > "${TEMP_DIR}/dmg/README.txt" << 'EOF'
Red Alert macOS Port
====================

Installation:
1. Drag Red Alert to Applications folder
2. Double-click to launch

Game Data:
Place original game data files in:
~/Library/Application Support/RedAlert/

For more information, visit:
https://github.com/example/redalert-macos

(c) 1996 Westwood Studios / Electronic Arts
EOF

# Create DMG
echo "Creating DMG image..."
hdiutil create \
    -volname "$VOLUME_NAME" \
    -srcfolder "${TEMP_DIR}/dmg" \
    -ov \
    -format UDZO \
    "$DMG_PATH"

# Cleanup
rm -rf "$TEMP_DIR"

# Sign the DMG (if identity available)
if [ -n "$SIGNING_IDENTITY" ] && [ "$SIGNING_IDENTITY" != "-" ]; then
    echo "Signing DMG..."
    codesign --sign "$SIGNING_IDENTITY" --timestamp "$DMG_PATH"
fi

# Get final size
DMG_SIZE=$(du -h "$DMG_PATH" | cut -f1)

echo ""
echo "=== DMG created successfully ==="
echo "Location: ${DMG_PATH}"
echo "Size: ${DMG_SIZE}"
echo ""
echo "To distribute:"
echo "  1. Upload DMG to distribution server"
echo "  2. Provide SHA256: $(shasum -a 256 "$DMG_PATH" | cut -d' ' -f1)"
