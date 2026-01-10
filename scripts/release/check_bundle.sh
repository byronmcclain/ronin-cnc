#!/bin/bash
# check_bundle.sh - Verify Bundle Integrity
# Task 18j - Release Verification

BUNDLE_PATH="${1:-.}"

if [ ! -d "$BUNDLE_PATH" ]; then
    echo "ERROR: Bundle not found: $BUNDLE_PATH"
    exit 1
fi

BUNDLE_NAME=$(basename "$BUNDLE_PATH" .app)
echo "Checking bundle: $BUNDLE_PATH"

ERRORS=0

check() {
    local path=$1
    local desc=$2

    if [ -e "$BUNDLE_PATH/$path" ]; then
        echo "  [OK] $desc"
    else
        echo "  [MISSING] $desc ($path)"
        ((ERRORS++)) || true
    fi
}

check_dir() {
    local path=$1
    local desc=$2

    if [ -d "$BUNDLE_PATH/$path" ]; then
        local count=$(find "$BUNDLE_PATH/$path" -type f 2>/dev/null | wc -l | tr -d ' ')
        echo "  [OK] $desc ($count files)"
    else
        echo "  [MISSING] $desc ($path)"
        ((ERRORS++)) || true
    fi
}

echo ""
echo "Structure:"
check "Contents/Info.plist" "Info.plist"
check "Contents/PkgInfo" "PkgInfo"
check "Contents/MacOS/${BUNDLE_NAME}" "Executable"

echo ""
echo "Resources:"
if [ -d "$BUNDLE_PATH/Contents/Resources" ]; then
    echo "  [OK] Resources directory"
    if [ -f "$BUNDLE_PATH/Contents/Resources/icon.icns" ]; then
        echo "  [OK] App Icon"
    else
        echo "  [OPTIONAL] App Icon (not found)"
    fi
else
    echo "  [MISSING] Resources directory"
fi

echo ""
echo "Executable check:"
EXE="$BUNDLE_PATH/Contents/MacOS/${BUNDLE_NAME}"
if [ -f "$EXE" ]; then
    # Check it's executable
    if [ -x "$EXE" ]; then
        echo "  [OK] Executable permissions"
    else
        echo "  [ERROR] Not executable"
        ((ERRORS++)) || true
    fi

    # Check architecture
    ARCH=$(file "$EXE")
    if echo "$ARCH" | grep -q "Mach-O"; then
        echo "  [OK] Valid Mach-O binary"
        echo "  Architecture: $(echo "$ARCH" | grep -o 'executable.*')"
    else
        echo "  [ERROR] Invalid binary format"
        ((ERRORS++)) || true
    fi

    # Check minimum macOS version
    if command -v otool > /dev/null 2>&1; then
        if otool -l "$EXE" 2>/dev/null | grep -q "minos"; then
            MIN_OS=$(otool -l "$EXE" 2>/dev/null | grep -A2 "LC_BUILD_VERSION" | grep "minos" | awk '{print $2}' | head -1)
            if [ -n "$MIN_OS" ]; then
                echo "  Minimum macOS: $MIN_OS"
            fi
        fi
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
