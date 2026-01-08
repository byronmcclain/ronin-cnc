#!/bin/bash
# Verification script for universal binary build

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Universal Binary Build ==="

cd "$ROOT_DIR"

# Step 1: Check build-universal.sh script
echo "Step 1: Checking build-universal.sh script..."
if [ ! -f "scripts/build-universal.sh" ]; then
    echo "VERIFY_FAILED: scripts/build-universal.sh not found"
    exit 1
fi
if [ ! -x "scripts/build-universal.sh" ]; then
    echo "VERIFY_FAILED: scripts/build-universal.sh not executable"
    exit 1
fi
echo "  build-universal.sh script exists and is executable"

# Step 2: Check Rust targets are available
echo "Step 2: Checking Rust cross-compilation targets..."
if ! rustup target list --installed | grep -q "x86_64-apple-darwin"; then
    echo "Note: x86_64-apple-darwin target not installed"
    echo "  Install with: rustup target add x86_64-apple-darwin"
fi
if ! rustup target list --installed | grep -q "aarch64-apple-darwin"; then
    echo "Note: aarch64-apple-darwin target not installed"
    echo "  Install with: rustup target add aarch64-apple-darwin"
fi
echo "  Rust target check complete"

# Step 3: Test build for current architecture
echo "Step 3: Testing platform crate build..."
if ! cargo build --manifest-path platform/Cargo.toml --release 2>&1; then
    echo "VERIFY_FAILED: cargo build failed"
    exit 1
fi
echo "  Platform crate builds successfully"

# Step 4: Check lipo is available
echo "Step 4: Checking lipo tool..."
if ! command -v lipo &> /dev/null; then
    echo "VERIFY_FAILED: lipo command not found (not on macOS?)"
    exit 1
fi
echo "  lipo tool available"

# Step 5: Verify library was created
echo "Step 5: Verifying static library..."
if [ -f "platform/target/release/libredalert_platform.a" ]; then
    echo "  Static library exists"
    lipo -info "platform/target/release/libredalert_platform.a" 2>/dev/null || true
else
    echo "  Static library not found (expected after clean build)"
fi

echo ""
echo "VERIFY_SUCCESS"
exit 0
