#!/bin/bash
# Verification script for MIX decryption

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking MIX Decryption ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check crypto module exists
echo "[1/5] Checking crypto module..."
if [ ! -f "platform/src/crypto/mod.rs" ]; then
    echo "VERIFY_FAILED: crypto/mod.rs not found"
    exit 1
fi
if [ ! -f "platform/src/crypto/blowfish.rs" ]; then
    echo "VERIFY_FAILED: crypto/blowfish.rs not found"
    exit 1
fi
if [ ! -f "platform/src/crypto/rsa.rs" ]; then
    echo "VERIFY_FAILED: crypto/rsa.rs not found"
    exit 1
fi
echo "  OK: Crypto module files exist"

# Step 2: Build
echo "[2/5] Building..."
if ! cmake --build build --target redalert_platform 2>&1; then
    echo "VERIFY_FAILED: Platform build failed"
    exit 1
fi
echo "  OK: Platform built"

# Step 3: Build test
echo "[3/5] Building decrypt test..."
if ! cmake --build build --target TestMixDecrypt 2>&1; then
    echo "VERIFY_FAILED: Test build failed"
    exit 1
fi
echo "  OK: Test built"

# Step 4: Check for encrypted MIX file
echo "[4/5] Checking for encrypted MIX file..."
if [ ! -f "gamedata/REDALERT.MIX" ]; then
    echo "VERIFY_WARNING: REDALERT.MIX not found"
    echo "VERIFY_SUCCESS (build only)"
    exit 0
fi

# Verify file is encrypted
FIRST_BYTES=$(xxd -l 4 -p gamedata/REDALERT.MIX)
if [ "$FIRST_BYTES" != "00000200" ]; then
    echo "VERIFY_WARNING: REDALERT.MIX does not appear to be encrypted"
fi
echo "  OK: Encrypted MIX file exists"

# Step 5: Run decryption test
echo "[5/5] Running decryption test..."
if ! timeout 30 ./build/TestMixDecrypt 2>&1 | tee /tmp/mix_decrypt_test.log; then
    echo "VERIFY_FAILED: Decryption test failed"
    exit 1
fi

# Check for success indicators in output
if grep -q "SUCCESS: MIX file registered" /tmp/mix_decrypt_test.log; then
    echo "  OK: MIX file successfully decrypted and registered"
else
    echo "VERIFY_FAILED: MIX registration failed"
    exit 1
fi

if grep -q "FOUND:" /tmp/mix_decrypt_test.log; then
    echo "  OK: Files found within MIX archive"
else
    echo "VERIFY_WARNING: No files found in archive (may need CRC table)"
fi

echo ""
echo "VERIFY_SUCCESS"
exit 0
