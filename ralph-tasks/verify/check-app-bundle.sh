#!/bin/bash
# Verification script for app bundle structure

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking App Bundle Structure ==="

cd "$ROOT_DIR"

# Step 1: Check Info.plist template exists
echo "Step 1: Checking Info.plist.in template..."
if [ ! -f "platform/macos/Info.plist.in" ]; then
    echo "VERIFY_FAILED: platform/macos/Info.plist.in not found"
    exit 1
fi
echo "  Info.plist.in template exists"

# Step 2: Check create-bundle.sh script
echo "Step 2: Checking create-bundle.sh script..."
if [ ! -f "scripts/create-bundle.sh" ]; then
    echo "VERIFY_FAILED: scripts/create-bundle.sh not found"
    exit 1
fi
if [ ! -x "scripts/create-bundle.sh" ]; then
    echo "VERIFY_FAILED: scripts/create-bundle.sh not executable"
    exit 1
fi
echo "  create-bundle.sh script exists and is executable"

# Step 3: Verify Info.plist template has required keys
echo "Step 3: Verifying Info.plist template..."
for key in CFBundleExecutable CFBundleIdentifier CFBundleName CFBundleVersion NSHighResolutionCapable; do
    if ! grep -q "$key" "platform/macos/Info.plist.in"; then
        echo "VERIFY_FAILED: $key not found in Info.plist.in"
        exit 1
    fi
done
echo "  All required keys present in Info.plist.in"

# Step 4: Test bundle creation (structure only)
echo "Step 4: Testing bundle creation..."
TEMP_BUILD=$(mktemp -d)
./scripts/create-bundle.sh "$TEMP_BUILD" "$TEMP_BUILD" > /dev/null 2>&1

if [ ! -d "$TEMP_BUILD/RedAlert.app/Contents/MacOS" ]; then
    rm -rf "$TEMP_BUILD"
    echo "VERIFY_FAILED: Bundle structure not created correctly"
    exit 1
fi

if [ ! -f "$TEMP_BUILD/RedAlert.app/Contents/Info.plist" ]; then
    rm -rf "$TEMP_BUILD"
    echo "VERIFY_FAILED: Info.plist not generated"
    exit 1
fi

rm -rf "$TEMP_BUILD"
echo "  Bundle structure verified"

echo ""
echo "VERIFY_SUCCESS"
exit 0
