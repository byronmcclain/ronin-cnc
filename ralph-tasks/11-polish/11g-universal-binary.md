# Task 11g: Universal Binary Build System

## Dependencies
- Task 11f (App Bundle) should be complete
- Rust toolchain with both targets installed

## Context
Modern Macs use two CPU architectures:
- **x86_64 (Intel)**: Older Macs (pre-2020)
- **arm64 (Apple Silicon)**: M1/M2/M3 Macs (2020+)

A Universal Binary (aka "fat binary") contains code for both architectures in a single file. This allows:
- Single download for all Mac users
- Native performance on both platforms
- No Rosetta 2 emulation needed on Apple Silicon

This task creates:
1. Script to build both architectures
2. Script to create universal static library (lipo)
3. CMake configuration for universal builds
4. Verification that both architectures work

## Objective
Create a universal binary build system that:
1. Builds Rust code for both x86_64 and arm64
2. Creates a universal static library with lipo
3. Builds the final app bundle for both architectures
4. Provides scripts for CI/CD automation

## Deliverables
- [ ] `scripts/build-universal.sh` - Main build script
- [ ] `scripts/build-rust-targets.sh` - Rust-specific build
- [ ] `platform/macos/universal-build.cmake` - CMake toolchain file
- [ ] Updated CMakeLists.txt for universal build support
- [ ] Verification passes on current architecture

## Technical Background

### Rust Targets
```bash
# Install targets
rustup target add x86_64-apple-darwin
rustup target add aarch64-apple-darwin

# Build for each
cargo build --target x86_64-apple-darwin --release
cargo build --target aarch64-apple-darwin --release
```

### lipo Command
```bash
# Create universal library
lipo -create \
    target/x86_64-apple-darwin/release/libredalert_platform.a \
    target/aarch64-apple-darwin/release/libredalert_platform.a \
    -output target/universal/libredalert_platform.a

# Verify
lipo -info target/universal/libredalert_platform.a
# Output: Architectures in the fat file: x86_64 arm64
```

### CMake Universal Build
CMake can build for multiple architectures with:
```cmake
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
```

However, the Rust library must already be universal.

## Files to Create/Modify

### scripts/build-universal.sh (NEW)
```bash
#!/bin/bash
# Build universal binary for macOS (Intel + Apple Silicon)
#
# Usage: ./scripts/build-universal.sh [--debug]
#
# Creates:
# - target/universal/libredalert_platform.a (universal static library)
# - build-universal/RedAlert.app (universal app bundle)

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# Configuration
BUILD_TYPE="${1:-release}"
CARGO_PROFILE="release"
CMAKE_BUILD_TYPE="Release"

if [ "$BUILD_TYPE" == "--debug" ]; then
    CARGO_PROFILE="dev"
    CMAKE_BUILD_TYPE="Debug"
fi

echo "=============================================="
echo "Building Universal Binary for macOS"
echo "=============================================="
echo ""
echo "Build type: $CMAKE_BUILD_TYPE"
echo "Cargo profile: $CARGO_PROFILE"
echo ""

cd "$ROOT_DIR"

# Step 1: Check Rust targets are installed
echo "Step 1: Checking Rust targets..."
if ! rustup target list --installed | grep -q "x86_64-apple-darwin"; then
    echo "  Installing x86_64-apple-darwin target..."
    rustup target add x86_64-apple-darwin
fi
if ! rustup target list --installed | grep -q "aarch64-apple-darwin"; then
    echo "  Installing aarch64-apple-darwin target..."
    rustup target add aarch64-apple-darwin
fi
echo "  Rust targets ready"

# Step 2: Build Rust library for both architectures
echo ""
echo "Step 2: Building Rust library for x86_64..."
if [ "$CARGO_PROFILE" == "release" ]; then
    cargo build --manifest-path platform/Cargo.toml --target x86_64-apple-darwin --release
else
    cargo build --manifest-path platform/Cargo.toml --target x86_64-apple-darwin
fi

echo ""
echo "Step 3: Building Rust library for arm64..."
if [ "$CARGO_PROFILE" == "release" ]; then
    cargo build --manifest-path platform/Cargo.toml --target aarch64-apple-darwin --release
else
    cargo build --manifest-path platform/Cargo.toml --target aarch64-apple-darwin
fi

# Step 4: Create universal library
echo ""
echo "Step 4: Creating universal library with lipo..."
UNIVERSAL_DIR="$ROOT_DIR/target/universal"
mkdir -p "$UNIVERSAL_DIR"

X86_LIB="$ROOT_DIR/target/x86_64-apple-darwin/$CARGO_PROFILE/libredalert_platform.a"
ARM_LIB="$ROOT_DIR/target/aarch64-apple-darwin/$CARGO_PROFILE/libredalert_platform.a"
UNIVERSAL_LIB="$UNIVERSAL_DIR/libredalert_platform.a"

if [ ! -f "$X86_LIB" ]; then
    echo "ERROR: x86_64 library not found: $X86_LIB"
    exit 1
fi

if [ ! -f "$ARM_LIB" ]; then
    echo "ERROR: arm64 library not found: $ARM_LIB"
    exit 1
fi

lipo -create "$X86_LIB" "$ARM_LIB" -output "$UNIVERSAL_LIB"
echo "  Created: $UNIVERSAL_LIB"

# Verify universal library
echo ""
echo "Step 5: Verifying universal library..."
lipo -info "$UNIVERSAL_LIB"

# Check for both architectures
if ! lipo -info "$UNIVERSAL_LIB" | grep -q "x86_64"; then
    echo "ERROR: x86_64 architecture missing from universal library"
    exit 1
fi
if ! lipo -info "$UNIVERSAL_LIB" | grep -q "arm64"; then
    echo "ERROR: arm64 architecture missing from universal library"
    exit 1
fi
echo "  Universal library verified"

# Step 6: Build CMake project with universal library
echo ""
echo "Step 6: Building C++ project with universal library..."

BUILD_DIR="$ROOT_DIR/build-universal"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure CMake with universal architectures and our library path
cmake "$ROOT_DIR" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
    -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" \
    -DUNIVERSAL_RUST_LIB="$UNIVERSAL_LIB"

cmake --build .

echo ""
echo "Step 7: Verifying universal executable..."
EXECUTABLE="$BUILD_DIR/RedAlert.app/Contents/MacOS/RedAlert"

if [ -f "$EXECUTABLE" ]; then
    lipo -info "$EXECUTABLE"

    if lipo -info "$EXECUTABLE" | grep -q "x86_64" && \
       lipo -info "$EXECUTABLE" | grep -q "arm64"; then
        echo "  Executable is universal (x86_64 + arm64)"
    else
        echo "  WARNING: Executable may not be fully universal"
    fi
else
    # Try standalone executable
    EXECUTABLE="$BUILD_DIR/RedAlert"
    if [ -f "$EXECUTABLE" ]; then
        lipo -info "$EXECUTABLE"
    else
        echo "WARNING: Could not find executable to verify"
    fi
fi

echo ""
echo "=============================================="
echo "Universal Build Complete!"
echo "=============================================="
echo ""
echo "Universal library: $UNIVERSAL_LIB"
echo "Build directory:   $BUILD_DIR"
echo ""
if [ -d "$BUILD_DIR/RedAlert.app" ]; then
    echo "App bundle: $BUILD_DIR/RedAlert.app"
    echo ""
    echo "To run:"
    echo "  open $BUILD_DIR/RedAlert.app"
fi
```

### scripts/build-rust-targets.sh (NEW)
```bash
#!/bin/bash
# Build Rust platform crate for specified targets
#
# Usage: ./scripts/build-rust-targets.sh [--release] [targets...]
#
# Examples:
#   ./scripts/build-rust-targets.sh                    # Build for current arch (debug)
#   ./scripts/build-rust-targets.sh --release          # Build for current arch (release)
#   ./scripts/build-rust-targets.sh x86_64-apple-darwin aarch64-apple-darwin
#   ./scripts/build-rust-targets.sh --release all      # Build all macOS targets

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# Parse arguments
RELEASE=""
TARGETS=()

for arg in "$@"; do
    case "$arg" in
        --release)
            RELEASE="--release"
            ;;
        all)
            TARGETS+=("x86_64-apple-darwin" "aarch64-apple-darwin")
            ;;
        *)
            TARGETS+=("$arg")
            ;;
    esac
done

# Default to current architecture if no targets specified
if [ ${#TARGETS[@]} -eq 0 ]; then
    CURRENT_ARCH=$(uname -m)
    case "$CURRENT_ARCH" in
        x86_64)
            TARGETS=("x86_64-apple-darwin")
            ;;
        arm64)
            TARGETS=("aarch64-apple-darwin")
            ;;
        *)
            echo "Unknown architecture: $CURRENT_ARCH"
            exit 1
            ;;
    esac
fi

cd "$ROOT_DIR"

echo "Building platform crate for targets: ${TARGETS[*]}"
echo "Release mode: ${RELEASE:-debug}"
echo ""

for target in "${TARGETS[@]}"; do
    echo "=== Building for $target ==="

    # Ensure target is installed
    if ! rustup target list --installed | grep -q "$target"; then
        echo "Installing target: $target"
        rustup target add "$target"
    fi

    cargo build --manifest-path platform/Cargo.toml --target "$target" $RELEASE

    # Show output location
    if [ -n "$RELEASE" ]; then
        LIB_PATH="target/$target/release/libredalert_platform.a"
    else
        LIB_PATH="target/$target/debug/libredalert_platform.a"
    fi

    if [ -f "$LIB_PATH" ]; then
        echo "  Output: $LIB_PATH"
        echo "  Size: $(du -h "$LIB_PATH" | cut -f1)"
    else
        echo "  WARNING: Library not found at expected path"
    fi
    echo ""
done

echo "Build complete!"
```

### platform/macos/universal-build.cmake (NEW)
```cmake
# CMake toolchain file for universal binary builds
#
# Usage:
#   cmake -DCMAKE_TOOLCHAIN_FILE=platform/macos/universal-build.cmake ..

# Target macOS
set(CMAKE_SYSTEM_NAME Darwin)

# Build for both architectures
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures")

# Minimum macOS version
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")

# Use universal Rust library if available
if(EXISTS "${CMAKE_SOURCE_DIR}/target/universal/libredalert_platform.a")
    set(UNIVERSAL_RUST_LIB "${CMAKE_SOURCE_DIR}/target/universal/libredalert_platform.a"
        CACHE FILEPATH "Path to universal Rust library")
    message(STATUS "Found universal Rust library: ${UNIVERSAL_RUST_LIB}")
endif()

# Compiler settings for universal build
set(CMAKE_C_FLAGS_INIT "-arch x86_64 -arch arm64")
set(CMAKE_CXX_FLAGS_INIT "-arch x86_64 -arch arm64")
```

### CMakeLists.txt (MODIFY)
Add this section before the RedAlert target definition to support universal builds:
```cmake
# =============================================================================
# Universal Binary Support
# =============================================================================

# If a universal Rust library is provided, use it instead of Corrosion
if(DEFINED UNIVERSAL_RUST_LIB AND EXISTS "${UNIVERSAL_RUST_LIB}")
    message(STATUS "Using pre-built universal Rust library: ${UNIVERSAL_RUST_LIB}")

    # Create an imported library target
    add_library(redalert_platform STATIC IMPORTED)
    set_target_properties(redalert_platform PROPERTIES
        IMPORTED_LOCATION "${UNIVERSAL_RUST_LIB}"
    )

    # Don't use Corrosion for this build
    set(USE_CORROSION OFF)
else()
    # Use Corrosion for normal single-architecture builds
    set(USE_CORROSION ON)
endif()

if(USE_CORROSION)
    # ... existing Corrosion setup ...
endif()
```

## Verification Command
```bash
ralph-tasks/verify/check-universal-binary.sh
```

## Verification Script
Create `ralph-tasks/verify/check-universal-binary.sh`:
```bash
#!/bin/bash
# Verification script for universal binary build

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Universal Binary Build System ==="

cd "$ROOT_DIR"

# Step 1: Check scripts exist
echo "Step 1: Checking build scripts exist..."
for script in scripts/build-universal.sh scripts/build-rust-targets.sh; do
    if [ ! -f "$script" ]; then
        echo "VERIFY_FAILED: $script not found"
        exit 1
    fi
    if [ ! -x "$script" ]; then
        chmod +x "$script"
    fi
done
echo "  Build scripts exist and are executable"

# Step 2: Check CMake toolchain file
echo "Step 2: Checking CMake toolchain file..."
if [ ! -f "platform/macos/universal-build.cmake" ]; then
    echo "VERIFY_FAILED: platform/macos/universal-build.cmake not found"
    exit 1
fi
echo "  Toolchain file exists"

# Step 3: Check Rust targets are available
echo "Step 3: Checking Rust targets..."

# Just verify rustup is available
if ! command -v rustup &> /dev/null; then
    echo "VERIFY_FAILED: rustup not found"
    exit 1
fi
echo "  rustup available"

# Step 4: Build for current architecture (faster test)
echo "Step 4: Building for current architecture..."
CURRENT_ARCH=$(uname -m)
case "$CURRENT_ARCH" in
    x86_64)
        TARGET="x86_64-apple-darwin"
        ;;
    arm64)
        TARGET="aarch64-apple-darwin"
        ;;
    *)
        echo "Unknown architecture: $CURRENT_ARCH"
        exit 1
        ;;
esac

echo "  Current architecture: $CURRENT_ARCH ($TARGET)"

# Ensure target is installed
if ! rustup target list --installed | grep -q "$TARGET"; then
    echo "  Installing target..."
    rustup target add "$TARGET"
fi

# Build
if ! cargo build --manifest-path platform/Cargo.toml --target "$TARGET" --release 2>&1; then
    echo "VERIFY_FAILED: Cargo build failed for $TARGET"
    exit 1
fi
echo "  Built successfully for $TARGET"

# Step 5: Verify library exists
echo "Step 5: Verifying library..."
LIB_PATH="target/$TARGET/release/libredalert_platform.a"
if [ ! -f "$LIB_PATH" ]; then
    echo "VERIFY_FAILED: Library not found at $LIB_PATH"
    exit 1
fi

# Check library architecture
ARCH_INFO=$(lipo -info "$LIB_PATH" 2>&1 || true)
echo "  Library info: $ARCH_INFO"

# Step 6: Optional - test cross-compilation if both targets available
echo "Step 6: Checking cross-compilation capability..."
OTHER_TARGET=""
case "$CURRENT_ARCH" in
    x86_64)
        OTHER_TARGET="aarch64-apple-darwin"
        ;;
    arm64)
        OTHER_TARGET="x86_64-apple-darwin"
        ;;
esac

if rustup target list --installed | grep -q "$OTHER_TARGET"; then
    echo "  Cross-target available: $OTHER_TARGET"
    echo "  (Full universal build would include both architectures)"
else
    echo "  Cross-target $OTHER_TARGET not installed (optional)"
    echo "  Run 'rustup target add $OTHER_TARGET' to enable universal builds"
fi

echo ""
echo "VERIFY_SUCCESS"
echo ""
echo "To create a full universal binary, run:"
echo "  ./scripts/build-universal.sh"
exit 0
```

## Implementation Steps
1. Create `scripts/build-universal.sh` and make executable
2. Create `scripts/build-rust-targets.sh` and make executable
3. Create `platform/macos/universal-build.cmake`
4. Optionally update CMakeLists.txt for universal library support
5. Install Rust targets: `rustup target add x86_64-apple-darwin aarch64-apple-darwin`
6. Test single-architecture build first
7. Test full universal build (takes longer)
8. Run verification script

## Success Criteria
- Scripts are executable and have no syntax errors
- Single-architecture build succeeds
- Universal library created with lipo (on full test)
- Verification script exits with 0

## Common Issues

### "error: linker not found"
Xcode Command Line Tools must be installed:
```bash
xcode-select --install
```

### "target may not be installed"
Install the Rust targets:
```bash
rustup target add x86_64-apple-darwin
rustup target add aarch64-apple-darwin
```

### "lipo: can't figure out the architecture type"
The library wasn't built correctly. Check cargo build output for errors.

### "architectures don't match"
When building with CMake, ensure CMAKE_OSX_ARCHITECTURES matches the Rust library architecture.

### "building for 'x86_64' but attempting to link with file built for 'arm64'"
You're mixing architectures. Either:
- Build everything for one architecture
- Use the universal build script for both

## Performance Notes
- Universal builds take roughly 2x longer (building twice)
- Universal binaries are roughly 2x the size
- Each architecture runs native code, no emulation penalty
- Consider offering separate downloads if bundle size is critical

## Completion Promise
When verification passes, output:
```
<promise>TASK_11G_COMPLETE</promise>
```

## Escape Hatch
If stuck after 10 iterations:
- Document what's blocking in `ralph-tasks/blocked/11g.md`
- List attempted approaches
- Output: `<promise>TASK_11G_BLOCKED</promise>`

## Max Iterations
10
