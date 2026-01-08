# Task 10f: IPX/Winsock Compatibility Headers

## Dependencies
- Tasks 10a-10e must be complete (full networking implementation)

## Context
The original Red Alert game code references IPX (Internetwork Packet Exchange) and Winsock (Windows Socket) APIs extensively. Since we've replaced these with enet-based networking, we need compatibility headers that:
1. Stub out IPX functions (always fail - IPX is obsolete)
2. Provide minimal Winsock type definitions
3. Direct networking to our Platform_Network* functions

These headers allow the original game code to compile with minimal modifications.

## Objective
Create compatibility headers:
1. `include/compat/network_wrapper.h` - Main compatibility header
2. `include/compat/ipx_stub.h` - IPX stub definitions
3. `include/compat/winsock_compat.h` - Winsock type stubs
4. Update cbindgen to include these headers

## Deliverables
- [ ] `include/compat/network_wrapper.h` - Combined wrapper
- [ ] `include/compat/ipx_stub.h` - IPX stubs
- [ ] `include/compat/winsock_compat.h` - Winsock type definitions
- [ ] Headers compile with clang
- [ ] Verification passes

## Technical Background

### IPX Protocol (Obsolete)
IPX was Novell's networking protocol, used for LAN games in the 90s:
- No longer supported by modern operating systems
- All IPX calls should fail gracefully
- Game should fall back to other networking

### Winsock APIs Used (SESSION.H, TCPIP.CPP)
```cpp
// Initialization
WSAStartup(), WSACleanup()

// Socket operations
socket(), bind(), listen(), accept()
connect(), closesocket()

// Data transfer
send(), recv(), sendto(), recvfrom()

// Async model
WSAAsyncSelect()
WSAAsyncGetHostByName()
```

### Strategy
- Provide type definitions so code compiles
- Stub functions return error codes
- Point developers to Platform_Network* instead

## Files to Create

### include/compat/network_wrapper.h (NEW)
```c
/*
 * Network Compatibility Wrapper
 *
 * Provides compatibility definitions for code that referenced
 * IPX and Winsock APIs. These are completely replaced by the
 * Platform_Network* functions from platform.h.
 *
 * IMPORTANT: Do not use these stub APIs for new code!
 * Use Platform_Network*, Platform_Host_*, Platform_Client_* instead.
 */

#ifndef NETWORK_WRAPPER_H
#define NETWORK_WRAPPER_H

#include "platform.h"

/* Include stub definitions */
#include "compat/ipx_stub.h"
#include "compat/winsock_compat.h"

/*
 * =============================================================================
 * Network Availability Checks
 * =============================================================================
 */

/*
 * IPX is no longer available on any platform.
 * Always returns 0 (false).
 */
static inline int Is_IPX_Available(void) {
    return 0;
}

/*
 * Winsock stubs are available for compilation but non-functional.
 * Use Platform_Network_IsInitialized() instead.
 */
static inline int Is_Winsock_Available(void) {
    return Platform_Network_IsInitialized();
}

/*
 * Check if networking is available.
 * This is the preferred function to use.
 */
static inline int Is_Network_Available(void) {
    return Platform_Network_IsInitialized();
}

/*
 * =============================================================================
 * Migration Macros
 *
 * These macros can be used to gradually migrate code from
 * old Winsock calls to new Platform_Network calls.
 * =============================================================================
 */

/* Example migration helpers (uncomment as needed):
 *
 * #define WSAStartup(ver, data)  Platform_Network_Init()
 * #define WSACleanup()           Platform_Network_Shutdown()
 *
 * For full migration, replace direct socket operations with
 * Platform_NetworkManager_* or Platform_Host_*/Platform_Client_* calls.
 */

#endif /* NETWORK_WRAPPER_H */
```

### include/compat/ipx_stub.h (NEW)
```c
/*
 * IPX Protocol Stub Definitions
 *
 * IPX (Internetwork Packet Exchange) was a networking protocol
 * used by Novell NetWare. It has been obsolete since the early 2000s
 * and is not supported by modern operating systems.
 *
 * These stub definitions allow legacy code to compile while
 * ensuring all IPX operations fail safely.
 */

#ifndef IPX_STUB_H
#define IPX_STUB_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * =============================================================================
 * IPX Address Types
 * =============================================================================
 */

/* IPX network address (6 bytes for node, 4 for network) */
typedef struct {
    unsigned char Network[4];   /* Network number */
    unsigned char Node[6];      /* Node address (MAC-like) */
    unsigned short Socket;      /* Socket number */
} IPXAddress;

/* IPX header structure (stub) */
typedef struct {
    unsigned short Checksum;
    unsigned short Length;
    unsigned char TransportControl;
    unsigned char PacketType;
    IPXAddress Destination;
    IPXAddress Source;
} IPXHeader;

/*
 * =============================================================================
 * IPX Error Codes
 * =============================================================================
 */

#define IPX_OK                      0
#define IPX_NOT_INSTALLED          -1
#define IPX_NO_SOCKETS             -2
#define IPX_SOCKET_ERROR           -3
#define IPX_SEND_FAILED            -4
#define IPX_RECEIVE_FAILED         -5
#define IPX_INVALID_PACKET         -6

/*
 * =============================================================================
 * IPX Stub Functions
 *
 * All functions return error codes indicating IPX is unavailable.
 * =============================================================================
 */

/*
 * Initialize IPX subsystem.
 * Always fails - IPX is not supported.
 *
 * Returns: IPX_NOT_INSTALLED
 */
static inline int IPX_Initialise(void) {
    return IPX_NOT_INSTALLED;
}

static inline int IPX_Initialize(void) {
    return IPX_NOT_INSTALLED;
}

/*
 * Shutdown IPX subsystem.
 * No-op since IPX was never initialized.
 */
static inline void IPX_Shutdown(void) {
    /* Nothing to do */
}

/*
 * Open an IPX socket.
 * Always fails - IPX is not supported.
 *
 * Returns: IPX_NOT_INSTALLED
 */
static inline int IPX_Open_Socket(unsigned short socket_num) {
    (void)socket_num;
    return IPX_NOT_INSTALLED;
}

/*
 * Close an IPX socket.
 * No-op.
 */
static inline void IPX_Close_Socket(unsigned short socket_num) {
    (void)socket_num;
}

/*
 * Send an IPX packet.
 * Always fails.
 *
 * Returns: IPX_NOT_INSTALLED
 */
static inline int IPX_Send_Packet(
    void *buffer,
    int size,
    IPXAddress *address,
    unsigned short socket_num
) {
    (void)buffer;
    (void)size;
    (void)address;
    (void)socket_num;
    return IPX_NOT_INSTALLED;
}

/*
 * Receive an IPX packet.
 * Always fails.
 *
 * Returns: IPX_NOT_INSTALLED
 */
static inline int IPX_Get_Packet(
    void *buffer,
    int *size,
    IPXAddress *address,
    unsigned short socket_num
) {
    (void)buffer;
    (void)size;
    (void)address;
    (void)socket_num;
    return IPX_NOT_INSTALLED;
}

/*
 * Broadcast an IPX packet.
 * Always fails.
 *
 * Returns: IPX_NOT_INSTALLED
 */
static inline int IPX_Broadcast_Packet(
    void *buffer,
    int size,
    unsigned short socket_num
) {
    (void)buffer;
    (void)size;
    (void)socket_num;
    return IPX_NOT_INSTALLED;
}

/*
 * Get local IPX address.
 * Fills address with zeros (no valid address).
 *
 * Returns: IPX_NOT_INSTALLED
 */
static inline int IPX_Get_Local_Target(
    IPXAddress *address,
    unsigned short socket_num
) {
    if (address) {
        /* Zero out the address */
        for (int i = 0; i < 4; i++) address->Network[i] = 0;
        for (int i = 0; i < 6; i++) address->Node[i] = 0;
        address->Socket = socket_num;
    }
    return IPX_NOT_INSTALLED;
}

/*
 * Check if IPX is installed.
 * Always returns 0 (not installed).
 */
static inline int IPX_Is_Present(void) {
    return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* IPX_STUB_H */
```

### include/compat/winsock_compat.h (NEW)
```c
/*
 * Winsock Compatibility Types
 *
 * Provides type definitions that allow code written for Windows
 * Winsock to compile on other platforms. Actual socket operations
 * should use Platform_Network* functions instead.
 *
 * Note: These are STUB definitions for compilation compatibility only.
 * They do not provide actual networking functionality.
 */

#ifndef WINSOCK_COMPAT_H
#define WINSOCK_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Only define these if not on Windows */
#ifndef _WIN32

/*
 * =============================================================================
 * Basic Types
 * =============================================================================
 */

/* Socket handle type */
typedef int SOCKET;

#define INVALID_SOCKET  ((SOCKET)-1)
#define SOCKET_ERROR    (-1)

/* Boolean and standard types */
#ifndef BOOL
typedef int BOOL;
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef unsigned int UINT;
typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;

/* Handle types */
typedef void* HANDLE;
typedef void* HWND;
typedef unsigned int WPARAM;
typedef long LPARAM;

/*
 * =============================================================================
 * Address Structures
 * =============================================================================
 */

/* Internet address */
struct in_addr {
    union {
        struct { unsigned char s_b1, s_b2, s_b3, s_b4; } S_un_b;
        struct { unsigned short s_w1, s_w2; } S_un_w;
        unsigned int S_addr;
    } S_un;
};

#define s_addr S_un.S_addr

/* Socket address (IPv4) */
struct sockaddr_in {
    short sin_family;
    unsigned short sin_port;
    struct in_addr sin_addr;
    char sin_zero[8];
};

/* Generic socket address */
struct sockaddr {
    unsigned short sa_family;
    char sa_data[14];
};

/* Host entry (from gethostbyname) */
struct hostent {
    char *h_name;
    char **h_aliases;
    short h_addrtype;
    short h_length;
    char **h_addr_list;
};

#define h_addr h_addr_list[0]

/*
 * =============================================================================
 * Winsock Constants
 * =============================================================================
 */

/* Address families */
#define AF_INET     2
#define AF_IPX      6   /* Obsolete */
#define PF_INET     AF_INET

/* Socket types */
#define SOCK_STREAM 1   /* TCP */
#define SOCK_DGRAM  2   /* UDP */

/* Protocol types */
#define IPPROTO_TCP 6
#define IPPROTO_UDP 17

/* Socket options */
#define SOL_SOCKET  0xFFFF
#define SO_REUSEADDR 4
#define SO_BROADCAST 32

/* Async select events (for WSAAsyncSelect) */
#define FD_READ     0x01
#define FD_WRITE    0x02
#define FD_OOB      0x04
#define FD_ACCEPT   0x08
#define FD_CONNECT  0x10
#define FD_CLOSE    0x20

/* Error codes */
#define WSABASEERR          10000
#define WSAEINTR            (WSABASEERR + 4)
#define WSAEWOULDBLOCK      (WSABASEERR + 35)
#define WSAEINPROGRESS      (WSABASEERR + 36)
#define WSAECONNREFUSED     (WSABASEERR + 61)
#define WSAETIMEDOUT        (WSABASEERR + 60)
#define WSAENOTCONN         (WSABASEERR + 57)

/*
 * =============================================================================
 * Winsock Data Structures
 * =============================================================================
 */

/* Winsock startup data */
typedef struct {
    WORD wVersion;
    WORD wHighVersion;
    char szDescription[257];
    char szSystemStatus[129];
    unsigned short iMaxSockets;
    unsigned short iMaxUdpDg;
    char *lpVendorInfo;
} WSADATA;

/*
 * =============================================================================
 * Stub Function Declarations
 *
 * These provide compilation compatibility but return errors.
 * Use Platform_Network* functions for actual networking.
 * =============================================================================
 */

/* Initialization */
static inline int WSAStartup(WORD wVersionRequested, WSADATA *lpWSAData) {
    (void)wVersionRequested;
    if (lpWSAData) {
        lpWSAData->wVersion = wVersionRequested;
        lpWSAData->wHighVersion = wVersionRequested;
    }
    /* Return 0 (success) but actual operations won't work */
    return 0;
}

static inline int WSACleanup(void) {
    return 0;
}

static inline int WSAGetLastError(void) {
    return WSAENOTCONN;
}

/* Socket operations - all fail */
static inline SOCKET socket(int af, int type, int protocol) {
    (void)af; (void)type; (void)protocol;
    return INVALID_SOCKET;
}

static inline int closesocket(SOCKET s) {
    (void)s;
    return 0;
}

static inline int bind(SOCKET s, const struct sockaddr *addr, int namelen) {
    (void)s; (void)addr; (void)namelen;
    return SOCKET_ERROR;
}

static inline int listen(SOCKET s, int backlog) {
    (void)s; (void)backlog;
    return SOCKET_ERROR;
}

static inline SOCKET accept(SOCKET s, struct sockaddr *addr, int *addrlen) {
    (void)s; (void)addr; (void)addrlen;
    return INVALID_SOCKET;
}

static inline int connect(SOCKET s, const struct sockaddr *name, int namelen) {
    (void)s; (void)name; (void)namelen;
    return SOCKET_ERROR;
}

static inline int send(SOCKET s, const char *buf, int len, int flags) {
    (void)s; (void)buf; (void)len; (void)flags;
    return SOCKET_ERROR;
}

static inline int recv(SOCKET s, char *buf, int len, int flags) {
    (void)s; (void)buf; (void)len; (void)flags;
    return SOCKET_ERROR;
}

static inline int sendto(SOCKET s, const char *buf, int len, int flags,
                         const struct sockaddr *to, int tolen) {
    (void)s; (void)buf; (void)len; (void)flags; (void)to; (void)tolen;
    return SOCKET_ERROR;
}

static inline int recvfrom(SOCKET s, char *buf, int len, int flags,
                           struct sockaddr *from, int *fromlen) {
    (void)s; (void)buf; (void)len; (void)flags; (void)from; (void)fromlen;
    return SOCKET_ERROR;
}

/* Async operations - all fail */
static inline int WSAAsyncSelect(SOCKET s, HWND hWnd, unsigned int wMsg, long lEvent) {
    (void)s; (void)hWnd; (void)wMsg; (void)lEvent;
    return SOCKET_ERROR;
}

static inline HANDLE WSAAsyncGetHostByName(HWND hWnd, unsigned int wMsg,
                                            const char *name, char *buf, int buflen) {
    (void)hWnd; (void)wMsg; (void)name; (void)buf; (void)buflen;
    return NULL;
}

/* Utility functions */
static inline unsigned short htons(unsigned short hostshort) {
    /* Convert to network byte order (big-endian) */
    return ((hostshort >> 8) & 0xFF) | ((hostshort & 0xFF) << 8);
}

static inline unsigned short ntohs(unsigned short netshort) {
    /* Convert from network byte order */
    return ((netshort >> 8) & 0xFF) | ((netshort & 0xFF) << 8);
}

static inline unsigned int htonl(unsigned int hostlong) {
    return ((hostlong >> 24) & 0xFF) |
           ((hostlong >> 8) & 0xFF00) |
           ((hostlong & 0xFF00) << 8) |
           ((hostlong & 0xFF) << 24);
}

static inline unsigned int ntohl(unsigned int netlong) {
    return htonl(netlong);
}

static inline unsigned int inet_addr(const char *cp) {
    (void)cp;
    return 0xFFFFFFFF; /* INADDR_NONE */
}

static inline char* inet_ntoa(struct in_addr in) {
    static char buffer[16];
    unsigned char *bytes = (unsigned char *)&in.s_addr;
    /* Simple implementation */
    buffer[0] = '0';
    buffer[1] = '\0';
    (void)bytes;
    return buffer;
}

static inline struct hostent* gethostbyname(const char *name) {
    (void)name;
    return NULL;
}

#endif /* !_WIN32 */

#ifdef __cplusplus
}
#endif

#endif /* WINSOCK_COMPAT_H */
```

## Verification Command
```bash
ralph-tasks/verify/check-compat-headers.sh
```

## Verification Script
Create `ralph-tasks/verify/check-compat-headers.sh`:
```bash
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

# Step 2: Create compat directory if needed
mkdir -p include/compat

# Step 3: Test C compilation
echo "Step 3: Testing C compilation..."
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

# Step 4: Test C++ compilation
echo "Step 4: Testing C++ compilation..."
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

# Step 5: Check IPX stub returns correct values
echo "Step 5: Verifying stub behavior..."

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
```

## Implementation Steps
1. Create `include/compat/` directory
2. Create `include/compat/network_wrapper.h`
3. Create `include/compat/ipx_stub.h`
4. Create `include/compat/winsock_compat.h`
5. Test C compilation with clang
6. Test C++ compilation with clang++
7. Run verification script

## Success Criteria
- All headers exist and compile cleanly
- IPX functions return error codes
- Winsock types are defined correctly
- Code compiles from both C and C++

## Common Issues

### "platform.h not found"
Run `cargo build --features cbindgen-run` first to generate platform.h

### Redefinition errors
Headers use include guards and #ifndef _WIN32 to prevent conflicts

### C++ compilation errors
Use `extern "C"` when including from C++

## Completion Promise
When verification passes, output:
```
<promise>TASK_10F_COMPLETE</promise>
```

## Escape Hatch
If stuck after 8 iterations:
- Document blocking issues in `ralph-tasks/blocked/10f.md`
- Output: `<promise>TASK_10F_BLOCKED</promise>`

## Max Iterations
8
