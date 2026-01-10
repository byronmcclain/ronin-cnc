#!/bin/bash
# create_bundle.sh - Create macOS App Bundle
# Task 18i - macOS App Bundle

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

BUNDLE_NAME="RedAlert"
BUNDLE_PATH="${BUILD_DIR}/${BUNDLE_NAME}.app"

echo "=== Creating macOS App Bundle ==="

# Build first
echo "Building..."
cmake --build "$BUILD_DIR" --config Release

# Verify bundle was created
if [ ! -d "$BUNDLE_PATH" ]; then
    echo "ERROR: Bundle not created at $BUNDLE_PATH"
    exit 1
fi

# Verify structure
echo "Verifying bundle structure..."

required_files=(
    "Contents/Info.plist"
    "Contents/MacOS/${BUNDLE_NAME}"
)

for file in "${required_files[@]}"; do
    if [ ! -e "${BUNDLE_PATH}/${file}" ]; then
        echo "ERROR: Missing ${file}"
        exit 1
    fi
done

# Check for optional files
optional_files=(
    "Contents/PkgInfo"
    "Contents/Resources/icon.icns"
)

for file in "${optional_files[@]}"; do
    if [ -e "${BUNDLE_PATH}/${file}" ]; then
        echo "  Found: ${file}"
    else
        echo "  Optional missing: ${file}"
    fi
done

# Verify executable
echo "Verifying executable..."
if ! file "${BUNDLE_PATH}/Contents/MacOS/${BUNDLE_NAME}" | grep -q "Mach-O"; then
    echo "ERROR: Executable is not valid Mach-O binary"
    exit 1
fi

# Check for universal binary
if file "${BUNDLE_PATH}/Contents/MacOS/${BUNDLE_NAME}" | grep -q "universal"; then
    echo "  Universal binary (Intel + Apple Silicon)"
else
    echo "  Single architecture binary"
fi

# Check assets
if [ -d "${BUNDLE_PATH}/Contents/Resources/Assets" ]; then
    ASSET_COUNT=$(find "${BUNDLE_PATH}/Contents/Resources/Assets" -type f | wc -l)
    echo "  Assets directory: ${ASSET_COUNT} files"
else
    echo "  No assets directory"
fi

echo ""
echo "=== Bundle created successfully ==="
echo "Location: ${BUNDLE_PATH}"
echo ""
echo "Next steps:"
echo "  1. ./scripts/sign_bundle.sh     # Code sign for distribution"
echo "  2. ./scripts/notarize_bundle.sh # Notarize for Gatekeeper"
echo "  3. ./scripts/create_dmg.sh      # Create DMG for distribution"
