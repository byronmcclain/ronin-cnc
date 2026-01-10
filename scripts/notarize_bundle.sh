#!/bin/bash
# notarize_bundle.sh - Notarize macOS App Bundle
# Task 18i - macOS App Bundle

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

BUNDLE_PATH="${1:-${BUILD_DIR}/RedAlert.app}"
BUNDLE_NAME=$(basename "$BUNDLE_PATH" .app)

# Notarization credentials (set these environment variables)
# APPLE_ID: Your Apple ID email
# TEAM_ID: Your team ID
# AC_PASSWORD: App-specific password (use keychain reference like @keychain:AC_PASSWORD)

if [ -z "$APPLE_ID" ] || [ -z "$TEAM_ID" ]; then
    echo "ERROR: Set APPLE_ID and TEAM_ID environment variables"
    echo ""
    echo "Example:"
    echo "  export APPLE_ID='your@email.com'"
    echo "  export TEAM_ID='ABCD1234'"
    echo "  export AC_PASSWORD='@keychain:AC_PASSWORD'"
    exit 1
fi

AC_PASSWORD="${AC_PASSWORD:-@keychain:AC_PASSWORD}"

echo "=== Notarizing macOS App Bundle ==="
echo "Bundle: ${BUNDLE_PATH}"
echo "Apple ID: ${APPLE_ID}"
echo "Team ID: ${TEAM_ID}"
echo ""

if [ ! -d "$BUNDLE_PATH" ]; then
    echo "ERROR: Bundle not found"
    exit 1
fi

# Verify code signature first
echo "Verifying code signature..."
codesign --verify --verbose "$BUNDLE_PATH" || {
    echo "ERROR: Bundle is not properly signed"
    exit 1
}

# Create ZIP for upload
ZIP_PATH="${BUILD_DIR}/${BUNDLE_NAME}.zip"
echo "Creating ZIP for upload..."
ditto -c -k --keepParent "$BUNDLE_PATH" "$ZIP_PATH"

# Submit for notarization
echo "Submitting for notarization..."
xcrun notarytool submit "$ZIP_PATH" \
    --apple-id "$APPLE_ID" \
    --team-id "$TEAM_ID" \
    --password "$AC_PASSWORD" \
    --wait

# Staple the ticket
echo ""
echo "Stapling notarization ticket..."
xcrun stapler staple "$BUNDLE_PATH"

# Verify staple
echo ""
echo "Verifying stapled ticket..."
xcrun stapler validate "$BUNDLE_PATH"

# Final Gatekeeper check
echo ""
echo "Final Gatekeeper verification..."
spctl --assess --verbose "$BUNDLE_PATH"

# Cleanup
rm -f "$ZIP_PATH"

echo ""
echo "=== Notarization complete ==="
echo "Bundle is ready for distribution"
