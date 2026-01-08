#!/bin/bash
#
# Build universal binary for macOS (Intel + Apple Silicon)
#
# Usage: ./scripts/build-universal.sh [release|debug]
#

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

BUILD_TYPE="${1:-release}"
BUILD_DIR="$ROOT_DIR/build"

echo "=== Building Universal Binary (Intel + Apple Silicon) ==="
echo "Build type: $BUILD_TYPE"

# Check if we're on macOS
if [[ "$(uname)" != "Darwin" ]]; then
    echo "Error: Universal binaries can only be built on macOS"
    exit 1
fi

# Create build directories
mkdir -p "$BUILD_DIR/x86_64"
mkdir -p "$BUILD_DIR/arm64"
mkdir -p "$BUILD_DIR/universal"

# Determine Rust build flags
if [ "$BUILD_TYPE" == "release" ]; then
    CARGO_FLAGS="--release"
    RUST_BUILD_DIR="release"
else
    CARGO_FLAGS=""
    RUST_BUILD_DIR="debug"
fi

# Build Rust library for x86_64 (Intel)
echo ""
echo "=== Building x86_64 (Intel) ==="
cd "$ROOT_DIR/platform"
cargo build $CARGO_FLAGS --target x86_64-apple-darwin

# Build Rust library for aarch64 (Apple Silicon)
echo ""
echo "=== Building aarch64 (Apple Silicon) ==="
cargo build $CARGO_FLAGS --target aarch64-apple-darwin

# Create universal Rust library
echo ""
echo "=== Creating Universal Rust Library ==="
X86_LIB="$ROOT_DIR/platform/target/x86_64-apple-darwin/$RUST_BUILD_DIR/libredalert_platform.a"
ARM_LIB="$ROOT_DIR/platform/target/aarch64-apple-darwin/$RUST_BUILD_DIR/libredalert_platform.a"
UNIVERSAL_LIB="$BUILD_DIR/universal/libredalert_platform.a"

if [ -f "$X86_LIB" ] && [ -f "$ARM_LIB" ]; then
    lipo -create "$X86_LIB" "$ARM_LIB" -output "$UNIVERSAL_LIB"
    echo "Created: $UNIVERSAL_LIB"
    lipo -info "$UNIVERSAL_LIB"
else
    echo "Warning: Could not create universal library"
    [ ! -f "$X86_LIB" ] && echo "  Missing: $X86_LIB"
    [ ! -f "$ARM_LIB" ] && echo "  Missing: $ARM_LIB"
fi

# Generate header (if cbindgen feature available)
echo ""
echo "=== Generating C Header ==="
cargo build $CARGO_FLAGS --features cbindgen-run 2>/dev/null || echo "Header generation skipped"

# Build C++ code with CMake (if CMakeLists.txt exists)
if [ -f "$ROOT_DIR/CMakeLists.txt" ]; then
    echo ""
    echo "=== Building C++ Code ==="

    # Build for x86_64
    cmake -B "$BUILD_DIR/x86_64" -S "$ROOT_DIR" \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE^}" \
        -DCMAKE_OSX_ARCHITECTURES=x86_64 \
        -G Ninja 2>/dev/null || true
    cmake --build "$BUILD_DIR/x86_64" 2>/dev/null || true

    # Build for arm64
    cmake -B "$BUILD_DIR/arm64" -S "$ROOT_DIR" \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE^}" \
        -DCMAKE_OSX_ARCHITECTURES=arm64 \
        -G Ninja 2>/dev/null || true
    cmake --build "$BUILD_DIR/arm64" 2>/dev/null || true

    # Create universal executable
    if [ -f "$BUILD_DIR/x86_64/RedAlert" ] && [ -f "$BUILD_DIR/arm64/RedAlert" ]; then
        echo ""
        echo "=== Creating Universal Executable ==="
        lipo -create \
            "$BUILD_DIR/x86_64/RedAlert" \
            "$BUILD_DIR/arm64/RedAlert" \
            -output "$BUILD_DIR/universal/RedAlert"
        chmod +x "$BUILD_DIR/universal/RedAlert"
        echo "Created: $BUILD_DIR/universal/RedAlert"
        lipo -info "$BUILD_DIR/universal/RedAlert"
    fi
fi

echo ""
echo "=== Build Complete ==="
echo "Universal library: $BUILD_DIR/universal/libredalert_platform.a"
[ -f "$BUILD_DIR/universal/RedAlert" ] && echo "Universal executable: $BUILD_DIR/universal/RedAlert"
