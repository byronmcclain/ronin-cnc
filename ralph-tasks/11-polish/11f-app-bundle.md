# Task 11f: macOS App Bundle Structure

## Dependencies
- CMake build system (Phase 01) must be complete
- Platform crate compiles successfully

## Context
This task creates the macOS application bundle (.app) structure. A proper bundle allows:
- Double-click to run from Finder
- Proper icon display
- Dock integration
- macOS security (Gatekeeper) compatibility
- Future code signing and notarization

The bundle structure follows Apple's guidelines for macOS applications.

## Objective
Create a macOS app bundle structure with:
1. Info.plist with proper metadata
2. CMake configuration for bundle creation
3. Icon file placeholder
4. Scripts for bundle assembly
5. Proper directory structure

## Deliverables
- [ ] `platform/macos/Info.plist.in` - CMake-configured Info.plist template
- [ ] `platform/macos/RedAlert.icns` - Placeholder icon (or script to generate)
- [ ] `scripts/create-bundle.sh` - Script to assemble the bundle
- [ ] Updated `CMakeLists.txt` with bundle configuration
- [ ] Verification passes with bundle creation

## Technical Background

### Bundle Structure
```
RedAlert.app/
├── Contents/
│   ├── Info.plist           <- Application metadata
│   ├── PkgInfo              <- Package type identifier
│   ├── MacOS/
│   │   └── RedAlert         <- Main executable
│   ├── Resources/
│   │   ├── RedAlert.icns    <- Application icon
│   │   ├── en.lproj/        <- Localization (optional)
│   │   └── data/            <- Game data files (optional)
│   └── Frameworks/          <- Bundled libraries (if any)
```

### Info.plist Keys
| Key | Description |
|-----|-------------|
| CFBundleName | Short name displayed in menus |
| CFBundleDisplayName | Full name displayed in Finder |
| CFBundleIdentifier | Unique reverse-DNS identifier |
| CFBundleVersion | Build number |
| CFBundleShortVersionString | User-visible version |
| CFBundleExecutable | Name of executable in MacOS/ |
| CFBundleIconFile | Name of icon file (without extension) |
| LSMinimumSystemVersion | Minimum macOS version |
| NSHighResolutionCapable | Retina display support |

### CMake Bundle Support
CMake can create bundles automatically with `MACOSX_BUNDLE TRUE`. We provide a custom Info.plist template for full control.

## Files to Create/Modify

### platform/macos/Info.plist.in (NEW)
```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <!-- Bundle Identification -->
    <key>CFBundleName</key>
    <string>Red Alert</string>

    <key>CFBundleDisplayName</key>
    <string>Command &amp; Conquer: Red Alert</string>

    <key>CFBundleIdentifier</key>
    <string>@MACOSX_BUNDLE_GUI_IDENTIFIER@</string>

    <!-- Version Information -->
    <key>CFBundleVersion</key>
    <string>@MACOSX_BUNDLE_BUNDLE_VERSION@</string>

    <key>CFBundleShortVersionString</key>
    <string>@MACOSX_BUNDLE_SHORT_VERSION_STRING@</string>

    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>

    <!-- Bundle Type -->
    <key>CFBundlePackageType</key>
    <string>APPL</string>

    <key>CFBundleSignature</key>
    <string>????</string>

    <!-- Executable -->
    <key>CFBundleExecutable</key>
    <string>@MACOSX_BUNDLE_EXECUTABLE_NAME@</string>

    <!-- Icon -->
    <key>CFBundleIconFile</key>
    <string>@MACOSX_BUNDLE_ICON_FILE@</string>

    <!-- System Requirements -->
    <key>LSMinimumSystemVersion</key>
    <string>@CMAKE_OSX_DEPLOYMENT_TARGET@</string>

    <!-- Display Settings -->
    <key>NSHighResolutionCapable</key>
    <true/>

    <key>NSSupportsAutomaticGraphicsSwitching</key>
    <true/>

    <!-- Architecture Priority (prefer ARM64 on Apple Silicon) -->
    <key>LSArchitecturePriority</key>
    <array>
        <string>arm64</string>
        <string>x86_64</string>
    </array>

    <!-- Application Category -->
    <key>LSApplicationCategoryType</key>
    <string>public.app-category.strategy-games</string>

    <!-- Copyright -->
    <key>NSHumanReadableCopyright</key>
    <string>Copyright © 2024. Original game © 1996 Westwood Studios/Electronic Arts.</string>

    <!-- Document Types (for save files, optional) -->
    <!--
    <key>CFBundleDocumentTypes</key>
    <array>
        <dict>
            <key>CFBundleTypeExtensions</key>
            <array>
                <string>sav</string>
            </array>
            <key>CFBundleTypeName</key>
            <string>Red Alert Save Game</string>
            <key>CFBundleTypeRole</key>
            <string>Editor</string>
        </dict>
    </array>
    -->

    <!-- Privacy Descriptions (if needed) -->
    <!--
    <key>NSMicrophoneUsageDescription</key>
    <string>Voice chat in multiplayer games.</string>
    -->
</dict>
</plist>
```

### scripts/create-bundle.sh (NEW)
```bash
#!/bin/bash
# Create macOS application bundle for Red Alert
#
# Usage: ./scripts/create-bundle.sh [--release|--debug]
#
# This script:
# 1. Builds the project if needed
# 2. Creates the .app bundle structure
# 3. Copies executable and resources
# 4. Generates Info.plist from template

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# Configuration
APP_NAME="RedAlert"
BUNDLE_ID="com.redalert.game"
VERSION="1.0.0"
BUILD_NUMBER="1"
BUILD_TYPE="${1:-release}"

# Paths
BUILD_DIR="$ROOT_DIR/build"
BUNDLE_DIR="$BUILD_DIR/$APP_NAME.app"
CONTENTS_DIR="$BUNDLE_DIR/Contents"
MACOS_DIR="$CONTENTS_DIR/MacOS"
RESOURCES_DIR="$CONTENTS_DIR/Resources"
FRAMEWORKS_DIR="$CONTENTS_DIR/Frameworks"

echo "=== Creating $APP_NAME.app Bundle ==="
echo "Build type: $BUILD_TYPE"
echo "Bundle location: $BUNDLE_DIR"
echo ""

# Step 1: Ensure project is built
echo "Step 1: Building project..."
cd "$ROOT_DIR"

if [ ! -d "$BUILD_DIR" ]; then
    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
fi

cmake --build build

# Check executable exists
if [ ! -f "$BUILD_DIR/$APP_NAME" ]; then
    echo "ERROR: Executable not found at $BUILD_DIR/$APP_NAME"
    exit 1
fi
echo "  Executable built successfully"

# Step 2: Create bundle structure
echo "Step 2: Creating bundle structure..."
rm -rf "$BUNDLE_DIR"
mkdir -p "$MACOS_DIR"
mkdir -p "$RESOURCES_DIR"
mkdir -p "$FRAMEWORKS_DIR"
echo "  Bundle directories created"

# Step 3: Copy executable
echo "Step 3: Copying executable..."
cp "$BUILD_DIR/$APP_NAME" "$MACOS_DIR/"
chmod 755 "$MACOS_DIR/$APP_NAME"
echo "  Executable copied"

# Step 4: Generate Info.plist
echo "Step 4: Generating Info.plist..."
cat > "$CONTENTS_DIR/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key>
    <string>Red Alert</string>
    <key>CFBundleDisplayName</key>
    <string>Command &amp; Conquer: Red Alert</string>
    <key>CFBundleIdentifier</key>
    <string>$BUNDLE_ID</string>
    <key>CFBundleVersion</key>
    <string>$BUILD_NUMBER</string>
    <key>CFBundleShortVersionString</key>
    <string>$VERSION</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleSignature</key>
    <string>????</string>
    <key>CFBundleExecutable</key>
    <string>$APP_NAME</string>
    <key>CFBundleIconFile</key>
    <string>$APP_NAME</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.15</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSSupportsAutomaticGraphicsSwitching</key>
    <true/>
    <key>LSArchitecturePriority</key>
    <array>
        <string>arm64</string>
        <string>x86_64</string>
    </array>
    <key>LSApplicationCategoryType</key>
    <string>public.app-category.strategy-games</string>
    <key>NSHumanReadableCopyright</key>
    <string>Copyright 2024. Original game 1996 Westwood Studios/Electronic Arts.</string>
</dict>
</plist>
EOF
echo "  Info.plist created"

# Step 5: Create PkgInfo
echo "Step 5: Creating PkgInfo..."
echo -n "APPL????" > "$CONTENTS_DIR/PkgInfo"
echo "  PkgInfo created"

# Step 6: Copy/create icon
echo "Step 6: Setting up icon..."
if [ -f "$ROOT_DIR/platform/macos/RedAlert.icns" ]; then
    cp "$ROOT_DIR/platform/macos/RedAlert.icns" "$RESOURCES_DIR/"
    echo "  Icon copied"
elif [ -f "$ROOT_DIR/resources/RedAlert.icns" ]; then
    cp "$ROOT_DIR/resources/RedAlert.icns" "$RESOURCES_DIR/"
    echo "  Icon copied from resources"
else
    # Create a placeholder icon (1x1 white pixel icns is not valid, skip for now)
    echo "  WARNING: No icon file found, bundle will use generic icon"
fi

# Step 7: Copy game data if present
echo "Step 7: Checking for game data..."
if [ -d "$ROOT_DIR/data" ]; then
    echo "  Copying game data..."
    cp -r "$ROOT_DIR/data" "$RESOURCES_DIR/"
    echo "  Game data copied"
else
    echo "  No game data directory found (optional)"
fi

# Step 8: Verify bundle
echo "Step 8: Verifying bundle..."

# Check structure
if [ ! -f "$MACOS_DIR/$APP_NAME" ]; then
    echo "ERROR: Executable missing from bundle"
    exit 1
fi

if [ ! -f "$CONTENTS_DIR/Info.plist" ]; then
    echo "ERROR: Info.plist missing from bundle"
    exit 1
fi

if [ ! -f "$CONTENTS_DIR/PkgInfo" ]; then
    echo "ERROR: PkgInfo missing from bundle"
    exit 1
fi

# Verify executable can run
echo "  Testing executable..."
"$MACOS_DIR/$APP_NAME" --test-init 2>&1 || true

echo ""
echo "=== Bundle Created Successfully ==="
echo "Location: $BUNDLE_DIR"
echo ""
echo "To run:"
echo "  open $BUNDLE_DIR"
echo ""
echo "To verify structure:"
echo "  ls -la $CONTENTS_DIR/"
echo ""
```

### CMakeLists.txt (MODIFY)
Add bundle configuration. This goes near the end of CMakeLists.txt:
```cmake
# =============================================================================
# macOS App Bundle Configuration
# =============================================================================

if(APPLE)
    # Bundle metadata
    set(MACOSX_BUNDLE_BUNDLE_NAME "Red Alert")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.redalert.game")
    set(MACOSX_BUNDLE_EXECUTABLE_NAME "RedAlert")
    set(MACOSX_BUNDLE_ICON_FILE "RedAlert")
    set(MACOSX_BUNDLE_COPYRIGHT "Original game © 1996 Westwood Studios/Electronic Arts")

    # Use custom Info.plist template if it exists
    if(EXISTS "${CMAKE_SOURCE_DIR}/platform/macos/Info.plist.in")
        set(MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/platform/macos/Info.plist.in")
    endif()

    # Set bundle properties on RedAlert target
    set_target_properties(RedAlert PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${MACOSX_BUNDLE_INFO_PLIST}"
        MACOSX_BUNDLE_BUNDLE_NAME "${MACOSX_BUNDLE_BUNDLE_NAME}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${MACOSX_BUNDLE_BUNDLE_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${MACOSX_BUNDLE_SHORT_VERSION_STRING}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${MACOSX_BUNDLE_GUI_IDENTIFIER}"
        MACOSX_BUNDLE_ICON_FILE "${MACOSX_BUNDLE_ICON_FILE}"
        MACOSX_BUNDLE_COPYRIGHT "${MACOSX_BUNDLE_COPYRIGHT}"
    )

    # Copy icon to Resources if it exists
    if(EXISTS "${CMAKE_SOURCE_DIR}/platform/macos/RedAlert.icns")
        set_source_files_properties(
            "${CMAKE_SOURCE_DIR}/platform/macos/RedAlert.icns"
            PROPERTIES MACOSX_PACKAGE_LOCATION "Resources"
        )
        target_sources(RedAlert PRIVATE
            "${CMAKE_SOURCE_DIR}/platform/macos/RedAlert.icns"
        )
    endif()

    # Custom target to copy game data to bundle
    add_custom_command(TARGET RedAlert POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_DIR:RedAlert>/Contents/Resources"
        COMMENT "Ensuring Resources directory exists"
    )

    # Copy data directory if it exists
    if(EXISTS "${CMAKE_SOURCE_DIR}/data")
        add_custom_command(TARGET RedAlert POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/data"
                "$<TARGET_BUNDLE_DIR:RedAlert>/Contents/Resources/data"
            COMMENT "Copying game data to bundle"
        )
    endif()
endif()
```

### platform/macos/create-icns.sh (NEW)
```bash
#!/bin/bash
# Create a placeholder .icns file from a source image
#
# Usage: ./create-icns.sh <source.png> <output.icns>
#
# Requires: iconutil (included with Xcode Command Line Tools)

set -e

if [ $# -lt 2 ]; then
    echo "Usage: $0 <source.png> <output.icns>"
    echo ""
    echo "Creates an .icns file from a source PNG image."
    echo "Source image should be at least 1024x1024 pixels."
    exit 1
fi

SOURCE="$1"
OUTPUT="$2"

if [ ! -f "$SOURCE" ]; then
    echo "Error: Source file not found: $SOURCE"
    exit 1
fi

# Create iconset directory
ICONSET="${OUTPUT%.icns}.iconset"
rm -rf "$ICONSET"
mkdir -p "$ICONSET"

# Generate all required sizes
echo "Generating icon sizes..."
for size in 16 32 64 128 256 512; do
    sips -z $size $size "$SOURCE" --out "$ICONSET/icon_${size}x${size}.png" >/dev/null 2>&1
    double=$((size * 2))
    sips -z $double $double "$SOURCE" --out "$ICONSET/icon_${size}x${size}@2x.png" >/dev/null 2>&1
done

# Create 1024 for 512@2x
sips -z 1024 1024 "$SOURCE" --out "$ICONSET/icon_512x512@2x.png" >/dev/null 2>&1

# Convert to icns
echo "Creating icns file..."
iconutil -c icns "$ICONSET" -o "$OUTPUT"

# Cleanup
rm -rf "$ICONSET"

echo "Created: $OUTPUT"
```

## Verification Command
```bash
ralph-tasks/verify/check-app-bundle.sh
```

## Verification Script
Create `ralph-tasks/verify/check-app-bundle.sh`:
```bash
#!/bin/bash
# Verification script for app bundle structure

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking App Bundle Structure ==="

cd "$ROOT_DIR"

# Step 1: Check template files exist
echo "Step 1: Checking bundle template files..."
if [ ! -f "platform/macos/Info.plist.in" ]; then
    echo "VERIFY_FAILED: platform/macos/Info.plist.in not found"
    exit 1
fi
echo "  Info.plist.in exists"

# Step 2: Check create-bundle script exists
echo "Step 2: Checking bundle creation script..."
if [ ! -f "scripts/create-bundle.sh" ]; then
    echo "VERIFY_FAILED: scripts/create-bundle.sh not found"
    exit 1
fi
if [ ! -x "scripts/create-bundle.sh" ]; then
    chmod +x "scripts/create-bundle.sh"
fi
echo "  create-bundle.sh exists and is executable"

# Step 3: Check CMakeLists.txt has bundle configuration
echo "Step 3: Checking CMake bundle configuration..."
if ! grep -q "MACOSX_BUNDLE" CMakeLists.txt; then
    echo "VERIFY_FAILED: MACOSX_BUNDLE not found in CMakeLists.txt"
    exit 1
fi
echo "  CMake bundle configuration found"

# Step 4: Build the project
echo "Step 4: Building project..."
mkdir -p build
cd build

if ! cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release 2>&1; then
    echo "VERIFY_FAILED: cmake configuration failed"
    exit 1
fi

if ! cmake --build . 2>&1; then
    echo "VERIFY_FAILED: cmake build failed"
    exit 1
fi
echo "  Build successful"

cd "$ROOT_DIR"

# Step 5: Check bundle was created
echo "Step 5: Checking bundle creation..."
BUNDLE_DIR="build/RedAlert.app"

if [ -d "$BUNDLE_DIR" ]; then
    echo "  Bundle directory exists"
else
    # Try running the script
    echo "  Bundle not created by CMake, trying script..."
    ./scripts/create-bundle.sh || true
fi

# Verify bundle structure
if [ ! -d "$BUNDLE_DIR" ]; then
    echo "VERIFY_FAILED: Bundle directory not created"
    exit 1
fi

# Step 6: Verify bundle contents
echo "Step 6: Verifying bundle contents..."

if [ ! -f "$BUNDLE_DIR/Contents/Info.plist" ]; then
    echo "VERIFY_FAILED: Info.plist not found in bundle"
    exit 1
fi
echo "  Info.plist present"

if [ ! -f "$BUNDLE_DIR/Contents/MacOS/RedAlert" ]; then
    echo "VERIFY_FAILED: Executable not found in bundle"
    exit 1
fi
echo "  Executable present"

if [ ! -f "$BUNDLE_DIR/Contents/PkgInfo" ]; then
    # PkgInfo is optional but recommended
    echo "  WARNING: PkgInfo not present (optional)"
else
    echo "  PkgInfo present"
fi

# Step 7: Verify Info.plist contents
echo "Step 7: Verifying Info.plist contents..."
PLIST="$BUNDLE_DIR/Contents/Info.plist"

for key in CFBundleName CFBundleIdentifier CFBundleExecutable; do
    if ! grep -q "$key" "$PLIST"; then
        echo "VERIFY_FAILED: $key not found in Info.plist"
        exit 1
    fi
    echo "  $key found"
done

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Create directory `platform/macos/`
2. Create `platform/macos/Info.plist.in`
3. Create `scripts/create-bundle.sh` and make it executable
4. Create `platform/macos/create-icns.sh` (optional helper)
5. Update `CMakeLists.txt` with bundle configuration
6. Run `cmake -B build -G Ninja && cmake --build build`
7. Verify bundle at `build/RedAlert.app/`
8. Run verification script

## Success Criteria
- Bundle directory created at build/RedAlert.app/
- Info.plist contains required keys
- Executable is in Contents/MacOS/
- Bundle can be opened with Finder (double-click)
- Verification script exits with 0

## Common Issues

### "Bundle not created"
Ensure `MACOSX_BUNDLE TRUE` is set on the target in CMakeLists.txt.

### "Executable not found"
Check that the target name matches throughout (RedAlert).

### "Info.plist template not found"
CMake looks for Info.plist.in relative to CMAKE_SOURCE_DIR. Verify the path.

### "Icon not showing"
The icon must be a valid .icns file and referenced in Info.plist's CFBundleIconFile (without extension).

### "Cannot open bundle"
Check Console.app for errors. Common issues:
- Missing code signature (for notarized distribution)
- Wrong architecture
- Missing frameworks

## Creating an Icon
If you have a source image (1024x1024 PNG):
```bash
chmod +x platform/macos/create-icns.sh
./platform/macos/create-icns.sh myicon.png platform/macos/RedAlert.icns
```

For now, a placeholder or no icon is acceptable.

## Completion Promise
When verification passes, output:
```
<promise>TASK_11F_COMPLETE</promise>
```

## Escape Hatch
If stuck after 12 iterations:
- Document what's blocking in `ralph-tasks/blocked/11f.md`
- List attempted approaches
- Output: `<promise>TASK_11F_BLOCKED</promise>`

## Max Iterations
12
