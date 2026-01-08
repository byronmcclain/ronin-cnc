#!/bin/bash
# Verification script for compatibility headers (Task 10f)

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Compatibility Headers ==="

cd "$ROOT_DIR"

# Step 1: Check headers exist
echo "Step 1: Checking headers exist..."
REQUIRED_HEADERS=(
    "include/compat/network_wrapper.h"
    "include/compat/ipx_stub.h"
    "include/compat/winsock_compat.h"
)

for header in "${REQUIRED_HEADERS[@]}"; do
    if [ ! -f "$header" ]; then
        echo "VERIFY_FAILED: $header not found"
        exit 1
    fi
    echo "  Found $header"
done

# Step 2: Test C compilation
echo "Step 2: Testing C compilation..."
TEST_FILE=$(mktemp /tmp/compat_test_XXXXXX.c)
cat > "$TEST_FILE" << 'EOF'
#include "include/platform.h"
#include "include/compat/network_wrapper.h"

int main(void) {
    /* Test IPX stubs */
    int ipx_result = IPX_Initialise();
    (void)ipx_result;

    int ipx_avail = Is_IPX_Available();
    (void)ipx_avail;

    IPXAddress addr;
    IPX_Get_Local_Target(&addr, 0x1234);

#ifndef _WIN32
    /* Test Winsock stubs */
    WSADATA wsaData;
    WSAStartup(0x0202, &wsaData);
    WSACleanup();

    SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
    closesocket(s);

    unsigned short port = htons(5555);
    (void)port;
#endif

    /* Test network availability */
    int net_avail = Is_Network_Available();
    (void)net_avail;

    return 0;
}
EOF

if ! clang -c "$TEST_FILE" -I. -o /dev/null 2>&1; then
    echo "VERIFY_FAILED: C compilation failed"
    rm -f "$TEST_FILE"
    exit 1
fi
echo "  C compilation successful"

# Step 3: Test C++ compilation
echo "Step 3: Testing C++ compilation..."
TEST_FILE_CPP=$(mktemp /tmp/compat_test_XXXXXX.cpp)
cat > "$TEST_FILE_CPP" << 'EOF'
extern "C" {
#include "include/platform.h"
#include "include/compat/network_wrapper.h"
}

int main() {
    // Test from C++
    int ipx_avail = Is_IPX_Available();
    (void)ipx_avail;

    int net_avail = Is_Network_Available();
    (void)net_avail;

    IPXAddress addr{};
    (void)addr;

    return 0;
}
EOF

if ! clang++ -c "$TEST_FILE_CPP" -I. -o /dev/null 2>&1; then
    echo "VERIFY_FAILED: C++ compilation failed"
    rm -f "$TEST_FILE" "$TEST_FILE_CPP"
    exit 1
fi
echo "  C++ compilation successful"

rm -f "$TEST_FILE" "$TEST_FILE_CPP"

# Step 4: Check IPX stub returns correct values
echo "Step 4: Verifying stub behavior..."

# Verify IPX_NOT_INSTALLED is -1
if ! grep -q "IPX_NOT_INSTALLED.*-1" include/compat/ipx_stub.h; then
    echo "VERIFY_FAILED: IPX_NOT_INSTALLED not defined as -1"
    exit 1
fi
echo "  IPX_NOT_INSTALLED correctly defined"

# Verify Is_IPX_Available returns 0
if ! grep -q "Is_IPX_Available.*return 0" include/compat/network_wrapper.h; then
    echo "VERIFY_FAILED: Is_IPX_Available should return 0"
    exit 1
fi
echo "  Is_IPX_Available returns 0"

echo ""
echo "=== All checks passed ==="
echo "VERIFY_SUCCESS"
exit 0
