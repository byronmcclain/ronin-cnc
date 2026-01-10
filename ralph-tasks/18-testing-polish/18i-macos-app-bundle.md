# Task 18i: macOS App Bundle

| Field | Value |
|-------|-------|
| **Task ID** | 18i |
| **Task Name** | macOS App Bundle |
| **Phase** | 18 - Testing & Polish |
| **Depends On** | All core systems, Assets integrated |
| **Estimated Complexity** | Medium (~350 lines) |

---

## Dependencies

- All game systems implemented and working
- Asset loading functional
- CMake build system

---

## Context

macOS applications are distributed as `.app` bundles - special directory structures:

```
RedAlert.app/
├── Contents/
│   ├── Info.plist          # Application metadata
│   ├── PkgInfo             # Package type indicator
│   ├── MacOS/
│   │   └── RedAlert        # Main executable
│   ├── Resources/
│   │   ├── icon.icns       # Application icon
│   │   ├── Assets/         # Game data files
│   │   └── *.lproj/        # Localization
│   └── Frameworks/         # Bundled libraries
```

Key requirements:
1. **Info.plist**: Required metadata (bundle ID, version, capabilities)
2. **Code Signing**: Required for distribution outside App Store
3. **Notarization**: Required for Gatekeeper approval
4. **Universal Binary**: Support both Intel and Apple Silicon

---

## Technical Background

### Bundle Structure

```
Bundle Contents:
- MacOS/          : Executable binaries
- Resources/      : Data files, images, localizations
- Frameworks/     : Embedded frameworks and dylibs
- PlugIns/        : Plugin bundles
- SharedSupport/  : Support files
```

### Info.plist Keys

```xml
Essential Keys:
- CFBundleIdentifier     : Unique bundle ID (com.westwood.redalert)
- CFBundleName           : Display name
- CFBundleVersion        : Build number
- CFBundleShortVersionString : Marketing version
- CFBundleExecutable     : Main executable name
- NSHighResolutionCapable : Retina display support
- LSMinimumSystemVersion : Minimum macOS version
```

### Code Signing

```bash
# Ad-hoc signing (for development)
codesign --force --deep --sign - RedAlert.app

# Developer ID signing (for distribution)
codesign --force --deep --sign "Developer ID Application: Name" \
    --options runtime \
    --timestamp \
    RedAlert.app
```

### Notarization

```bash
# Submit for notarization
xcrun notarytool submit RedAlert.zip \
    --apple-id "email@example.com" \
    --team-id "TEAMID" \
    --password "@keychain:AC_PASSWORD"

# Staple ticket to app
xcrun stapler staple RedAlert.app
```

---

## Objective

Create a proper macOS app bundle:
1. Configure CMake to generate .app bundle
2. Create Info.plist with correct metadata
3. Set up resource copying
4. Create app icon
5. Script for code signing
6. Script for notarization

---

## Deliverables

- [ ] `cmake/MacOSBundle.cmake` - Bundle configuration
- [ ] `resources/macos/Info.plist.in` - Info.plist template
- [ ] `resources/macos/PkgInfo` - Package info file
- [ ] `resources/macos/icon.iconset/` - Icon source images
- [ ] `scripts/create_bundle.sh` - Bundle creation script
- [ ] `scripts/sign_bundle.sh` - Code signing script
- [ ] `scripts/notarize_bundle.sh` - Notarization script
- [ ] `scripts/create_dmg.sh` - DMG creation script
- [ ] CMakeLists.txt updated

---

## Files to Create

### cmake/MacOSBundle.cmake

```cmake
# cmake/MacOSBundle.cmake
# macOS App Bundle Configuration
# Task 18i - macOS App Bundle

# Only apply on macOS
if(NOT APPLE)
    return()
endif()

#==============================================================================
# Bundle Configuration
#==============================================================================

set(MACOSX_BUNDLE_BUNDLE_NAME "Red Alert")
set(MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.westwood.redalert")
set(MACOSX_BUNDLE_COPYRIGHT "Copyright © 1996 Westwood Studios / EA")
set(MACOSX_BUNDLE_ICON_FILE "icon.icns")
set(MACOSX_BUNDLE_INFO_STRING "Red Alert macOS Port")

# Minimum macOS version
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

# Universal binary (Intel + Apple Silicon)
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures")

#==============================================================================
# Bundle Target Function
#==============================================================================

function(configure_macos_bundle TARGET_NAME)
    # Enable bundle
    set_target_properties(${TARGET_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/resources/macos/Info.plist.in"

        # Output location
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"

        # Install RPATH for bundled libraries
        INSTALL_RPATH "@executable_path/../Frameworks"
        BUILD_WITH_INSTALL_RPATH TRUE
    )

    # Get bundle path
    set(BUNDLE_PATH "$<TARGET_BUNDLE_DIR:${TARGET_NAME}>")

    # Copy icon
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${BUNDLE_PATH}/Contents/Resources"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/resources/macos/icon.icns"
            "${BUNDLE_PATH}/Contents/Resources/"
        COMMENT "Copying app icon"
    )

    # Copy PkgInfo
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/resources/macos/PkgInfo"
            "${BUNDLE_PATH}/Contents/"
        COMMENT "Copying PkgInfo"
    )

    # Copy game assets
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${BUNDLE_PATH}/Contents/Resources/Assets"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "${BUNDLE_PATH}/Contents/Resources/Assets"
        COMMENT "Copying game assets"
    )

    # Copy embedded frameworks (if any)
    # add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    #     COMMAND ${CMAKE_COMMAND} -E make_directory
    #         "${BUNDLE_PATH}/Contents/Frameworks"
    #     COMMAND ${CMAKE_COMMAND} -E copy_if_different
    #         "${SDL2_LIBRARY}/SDL2.framework"
    #         "${BUNDLE_PATH}/Contents/Frameworks/"
    #     COMMENT "Copying frameworks"
    # )
endfunction()

#==============================================================================
# Icon Generation
#==============================================================================

function(generate_app_icon)
    # Generate .icns from .iconset directory
    set(ICONSET_DIR "${CMAKE_SOURCE_DIR}/resources/macos/icon.iconset")
    set(ICNS_FILE "${CMAKE_SOURCE_DIR}/resources/macos/icon.icns")

    # Only regenerate if iconset is newer
    add_custom_command(
        OUTPUT ${ICNS_FILE}
        COMMAND iconutil -c icns -o ${ICNS_FILE} ${ICONSET_DIR}
        DEPENDS ${ICONSET_DIR}
        COMMENT "Generating app icon"
    )

    add_custom_target(AppIcon DEPENDS ${ICNS_FILE})
endfunction()

#==============================================================================
# Code Signing Target
#==============================================================================

function(add_codesign_target TARGET_NAME)
    set(BUNDLE_PATH "$<TARGET_BUNDLE_DIR:${TARGET_NAME}>")

    add_custom_target(codesign
        COMMAND codesign --force --deep --sign - "${BUNDLE_PATH}"
        DEPENDS ${TARGET_NAME}
        COMMENT "Ad-hoc code signing"
    )

    add_custom_target(codesign-release
        COMMAND "${CMAKE_SOURCE_DIR}/scripts/sign_bundle.sh" "${BUNDLE_PATH}"
        DEPENDS ${TARGET_NAME}
        COMMENT "Release code signing"
    )
endfunction()

#==============================================================================
# DMG Creation Target
#==============================================================================

function(add_dmg_target TARGET_NAME)
    set(BUNDLE_PATH "$<TARGET_BUNDLE_DIR:${TARGET_NAME}>")
    set(DMG_NAME "${TARGET_NAME}-${PROJECT_VERSION}.dmg")

    add_custom_target(dmg
        COMMAND "${CMAKE_SOURCE_DIR}/scripts/create_dmg.sh"
            "${BUNDLE_PATH}"
            "${CMAKE_BINARY_DIR}/${DMG_NAME}"
        DEPENDS ${TARGET_NAME}
        COMMENT "Creating DMG"
    )
endfunction()
```

### resources/macos/Info.plist.in

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <!-- Bundle Identification -->
    <key>CFBundleIdentifier</key>
    <string>${MACOSX_BUNDLE_GUI_IDENTIFIER}</string>

    <key>CFBundleName</key>
    <string>${MACOSX_BUNDLE_BUNDLE_NAME}</string>

    <key>CFBundleDisplayName</key>
    <string>${MACOSX_BUNDLE_BUNDLE_NAME}</string>

    <!-- Version Information -->
    <key>CFBundleVersion</key>
    <string>${MACOSX_BUNDLE_BUNDLE_VERSION}</string>

    <key>CFBundleShortVersionString</key>
    <string>${MACOSX_BUNDLE_SHORT_VERSION_STRING}</string>

    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>

    <!-- Executable -->
    <key>CFBundleExecutable</key>
    <string>${MACOSX_BUNDLE_EXECUTABLE_NAME}</string>

    <key>CFBundlePackageType</key>
    <string>APPL</string>

    <!-- Icons -->
    <key>CFBundleIconFile</key>
    <string>${MACOSX_BUNDLE_ICON_FILE}</string>

    <key>CFBundleIconName</key>
    <string>AppIcon</string>

    <!-- Copyright -->
    <key>NSHumanReadableCopyright</key>
    <string>${MACOSX_BUNDLE_COPYRIGHT}</string>

    <!-- System Requirements -->
    <key>LSMinimumSystemVersion</key>
    <string>11.0</string>

    <!-- Display Settings -->
    <key>NSHighResolutionCapable</key>
    <true/>

    <key>NSSupportsAutomaticGraphicsSwitching</key>
    <true/>

    <!-- Category -->
    <key>LSApplicationCategoryType</key>
    <string>public.app-category.strategy-games</string>

    <!-- Document Types (for save files) -->
    <key>CFBundleDocumentTypes</key>
    <array>
        <dict>
            <key>CFBundleTypeName</key>
            <string>Red Alert Save File</string>
            <key>CFBundleTypeExtensions</key>
            <array>
                <string>sav</string>
            </array>
            <key>CFBundleTypeRole</key>
            <string>Editor</string>
        </dict>
    </array>

    <!-- Privacy Descriptions (for sandbox) -->
    <key>NSMicrophoneUsageDescription</key>
    <string>Red Alert requires microphone access for voice chat.</string>

    <!-- Capabilities -->
    <key>NSPrincipalClass</key>
    <string>NSApplication</string>

    <!-- App Transport Security (for any network access) -->
    <key>NSAppTransportSecurity</key>
    <dict>
        <key>NSAllowsArbitraryLoads</key>
        <false/>
    </dict>

    <!-- Exported/Imported UTIs -->
    <key>UTExportedTypeDeclarations</key>
    <array>
        <dict>
            <key>UTTypeIdentifier</key>
            <string>com.westwood.redalert.savegame</string>
            <key>UTTypeDescription</key>
            <string>Red Alert Save Game</string>
            <key>UTTypeConformsTo</key>
            <array>
                <string>public.data</string>
            </array>
            <key>UTTypeTagSpecification</key>
            <dict>
                <key>public.filename-extension</key>
                <array>
                    <string>sav</string>
                </array>
            </dict>
        </dict>
    </array>

    <!-- Sparkle Update Framework (if used) -->
    <!--
    <key>SUFeedURL</key>
    <string>https://example.com/appcast.xml</string>
    -->
</dict>
</plist>
```

### resources/macos/PkgInfo

```
APPL????
```

### resources/macos/Entitlements.plist

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <!-- Hardened Runtime -->
    <key>com.apple.security.cs.allow-unsigned-executable-memory</key>
    <false/>

    <key>com.apple.security.cs.disable-library-validation</key>
    <false/>

    <!-- App Sandbox (optional - for App Store) -->
    <!--
    <key>com.apple.security.app-sandbox</key>
    <true/>

    <key>com.apple.security.files.user-selected.read-write</key>
    <true/>

    <key>com.apple.security.network.client</key>
    <true/>
    -->

    <!-- File Access -->
    <key>com.apple.security.files.user-selected.read-write</key>
    <true/>

    <!-- Audio -->
    <key>com.apple.security.device.audio-input</key>
    <false/>
</dict>
</plist>
```

### scripts/create_bundle.sh

```bash
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
    "Contents/PkgInfo"
    "Contents/MacOS/${BUNDLE_NAME}"
    "Contents/Resources/icon.icns"
)

for file in "${required_files[@]}"; do
    if [ ! -e "${BUNDLE_PATH}/${file}" ]; then
        echo "ERROR: Missing ${file}"
        exit 1
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
    echo "✓ Universal binary (Intel + Apple Silicon)"
else
    echo "⚠ Single architecture binary"
fi

# Check assets
if [ -d "${BUNDLE_PATH}/Contents/Resources/Assets" ]; then
    ASSET_COUNT=$(find "${BUNDLE_PATH}/Contents/Resources/Assets" -type f | wc -l)
    echo "✓ Assets directory: ${ASSET_COUNT} files"
else
    echo "⚠ No assets directory"
fi

echo ""
echo "=== Bundle created successfully ==="
echo "Location: ${BUNDLE_PATH}"
echo ""
echo "Next steps:"
echo "  1. ./scripts/sign_bundle.sh     # Code sign for distribution"
echo "  2. ./scripts/notarize_bundle.sh # Notarize for Gatekeeper"
echo "  3. ./scripts/create_dmg.sh      # Create DMG for distribution"
```

### scripts/sign_bundle.sh

```bash
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
```

### scripts/notarize_bundle.sh

```bash
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
```

### scripts/create_dmg.sh

```bash
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

© 1996 Westwood Studios / Electronic Arts
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
```

### scripts/generate_icons.sh

```bash
#!/bin/bash
# generate_icons.sh - Generate App Icon Set
# Task 18i - macOS App Bundle

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
ICONSET_DIR="${ROOT_DIR}/resources/macos/icon.iconset"
SOURCE_ICON="${1:-${ROOT_DIR}/resources/icon_1024.png}"

echo "=== Generating Icon Set ==="

if [ ! -f "$SOURCE_ICON" ]; then
    echo "ERROR: Source icon not found: ${SOURCE_ICON}"
    echo "Provide a 1024x1024 PNG as the source"
    exit 1
fi

# Create iconset directory
mkdir -p "$ICONSET_DIR"

# Generate all required sizes
# Format: icon_<size>x<size>[@2x].png
sizes=(16 32 128 256 512)

for size in "${sizes[@]}"; do
    # Standard resolution
    echo "Generating ${size}x${size}..."
    sips -z $size $size "$SOURCE_ICON" --out "${ICONSET_DIR}/icon_${size}x${size}.png"

    # Retina (@2x)
    double=$((size * 2))
    if [ $double -le 1024 ]; then
        echo "Generating ${size}x${size}@2x (${double}x${double})..."
        sips -z $double $double "$SOURCE_ICON" --out "${ICONSET_DIR}/icon_${size}x${size}@2x.png"
    fi
done

# Generate .icns
echo "Generating icon.icns..."
iconutil -c icns -o "${ROOT_DIR}/resources/macos/icon.icns" "$ICONSET_DIR"

echo ""
echo "=== Icon generation complete ==="
echo "Iconset: ${ICONSET_DIR}"
echo "ICNS: ${ROOT_DIR}/resources/macos/icon.icns"
```

---

## CMakeLists.txt Additions

```cmake
# Task 18i: macOS App Bundle

if(APPLE)
    # Include bundle configuration
    include(${CMAKE_SOURCE_DIR}/cmake/MacOSBundle.cmake)

    # Configure main executable as bundle
    configure_macos_bundle(RedAlert)

    # Add signing and packaging targets
    add_codesign_target(RedAlert)
    add_dmg_target(RedAlert)

    # Optional: Generate icon from source
    # generate_app_icon()
endif()
```

---

## Icon Requirements

Create `resources/macos/icon.iconset/` with these files:

```
icon.iconset/
├── icon_16x16.png      (16x16)
├── icon_16x16@2x.png   (32x32)
├── icon_32x32.png      (32x32)
├── icon_32x32@2x.png   (64x64)
├── icon_128x128.png    (128x128)
├── icon_128x128@2x.png (256x256)
├── icon_256x256.png    (256x256)
├── icon_256x256@2x.png (512x512)
├── icon_512x512.png    (512x512)
└── icon_512x512@2x.png (1024x1024)
```

---

## Verification Script

```bash
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
done

# Make scripts executable
chmod +x "$ROOT_DIR/scripts/"*.sh

# Build bundle (if CMake configured)
if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
    cd "$BUILD_DIR" && cmake --build . --target RedAlert

    # Verify bundle structure
    BUNDLE="$BUILD_DIR/RedAlert.app"
    if [ -d "$BUNDLE" ]; then
        test -f "$BUNDLE/Contents/Info.plist" || { echo "FAIL: Info.plist missing"; exit 1; }
        test -f "$BUNDLE/Contents/MacOS/RedAlert" || { echo "FAIL: Executable missing"; exit 1; }
        echo "Bundle structure verified"
    fi
fi

echo "=== Task 18i Verification PASSED ==="
```

---

## Success Criteria

1. CMake generates proper .app bundle
2. Info.plist contains correct metadata
3. Bundle contains executable, icon, and resources
4. Sign script produces valid signature
5. DMG script creates distributable image

---

## Completion Promise

When verification passes, output:
<promise>TASK_18I_COMPLETE</promise>

## Escape Hatch

If stuck after 15 iterations, output:
<promise>TASK_18I_BLOCKED</promise>

## Max Iterations
15
