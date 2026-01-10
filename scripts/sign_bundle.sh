#!/bin/bash
# sign_bundle.sh - Code Sign macOS App Bundle
# Task 18i - macOS App Bundle

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

BUNDLE_PATH="${1:-${BUILD_DIR}/RedAlert.app}"
ENTITLEMENTS="${ROOT_DIR}/resources/macos/Entitlements.plist"

# Signing identity (change for your certificate)
# Use "Developer ID Application: Your Name (TEAMID)" for distribution
# Use "-" for ad-hoc signing (development only)
SIGNING_IDENTITY="${SIGNING_IDENTITY:--}"

echo "=== Code Signing macOS App Bundle ==="
echo "Bundle: ${BUNDLE_PATH}"
echo "Identity: ${SIGNING_IDENTITY}"
echo ""

if [ ! -d "$BUNDLE_PATH" ]; then
    echo "ERROR: Bundle not found at ${BUNDLE_PATH}"
    exit 1
fi

# Remove any existing signatures
echo "Removing existing signatures..."
codesign --remove-signature "$BUNDLE_PATH" 2>/dev/null || true

# Sign embedded frameworks first (if any)
FRAMEWORKS_DIR="${BUNDLE_PATH}/Contents/Frameworks"
if [ -d "$FRAMEWORKS_DIR" ]; then
    echo "Signing embedded frameworks..."
    find "$FRAMEWORKS_DIR" -name "*.framework" -o -name "*.dylib" | while read framework; do
        echo "  Signing: $(basename "$framework")"
        codesign --force --sign "$SIGNING_IDENTITY" \
            --options runtime \
            --timestamp \
            "$framework"
    done
fi

# Sign the main bundle
echo "Signing main bundle..."
if [ "$SIGNING_IDENTITY" = "-" ]; then
    # Ad-hoc signing (no entitlements, no hardened runtime)
    codesign --force --deep --sign - "$BUNDLE_PATH"
else
    # Release signing with entitlements and hardened runtime
    codesign --force --deep --sign "$SIGNING_IDENTITY" \
        --options runtime \
        --entitlements "$ENTITLEMENTS" \
        --timestamp \
        "$BUNDLE_PATH"
fi

# Verify signature
echo ""
echo "Verifying signature..."
codesign --verify --verbose=2 "$BUNDLE_PATH"

# Check Gatekeeper acceptance
echo ""
echo "Checking Gatekeeper..."
spctl --assess --verbose "$BUNDLE_PATH" 2>&1 || true

echo ""
echo "=== Code signing complete ==="
