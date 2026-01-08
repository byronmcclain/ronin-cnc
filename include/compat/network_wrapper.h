/**
 * network_wrapper.h - Winsock and IPX Compatibility Layer
 *
 * This header provides stub definitions for obsolete Windows socket types
 * and the completely obsolete IPX protocol. The actual networking is now
 * handled by the Rust platform layer using the enet library.
 *
 * Purpose:
 * - Provide Winsock type definitions for code that references them
 * - Stub out IPX functions (always return failure/unavailable)
 * - Allow original game code to compile without Windows headers
 *
 * Usage:
 * - Include this header instead of winsock.h or ipx headers
 * - All actual networking should go through Platform_* FFI functions
 */

#ifndef NETWORK_WRAPPER_H
#define NETWORK_WRAPPER_H

#include "platform.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==========================================================================
 * Winsock Compatibility Layer
 * ========================================================================== */

/* Socket type - use int for cross-platform compatibility */
typedef int SOCKET;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR   (-1)

/* Address family constants */
#define AF_INET     2
#define AF_IPX      6   /* Obsolete - IPX not supported */
#define AF_UNSPEC   0

/* Socket types */
#define SOCK_STREAM 1
#define SOCK_DGRAM  2
#define SOCK_RAW    3

/* Protocol constants */
#define IPPROTO_TCP 6
#define IPPROTO_UDP 17

/* Socket options */
#define SOL_SOCKET      0xFFFF
#define SO_REUSEADDR    0x0004
#define SO_BROADCAST    0x0020
#define SO_RCVBUF       0x1002
#define SO_SNDBUF       0x1001

/* I/O control */
#define FIONREAD        0x541B
#define FIONBIO         0x5421

/* Shutdown modes */
#define SD_RECEIVE      0
#define SD_SEND         1
#define SD_BOTH         2

/* Address structures */
struct in_addr {
    uint32_t s_addr;
};

struct sockaddr_in {
    int16_t sin_family;
    uint16_t sin_port;
    struct in_addr sin_addr;
    char sin_zero[8];
};

struct sockaddr {
    uint16_t sa_family;
    char sa_data[14];
};

/* Host entry for name resolution */
struct hostent {
    char* h_name;
    char** h_aliases;
    int16_t h_addrtype;
    int16_t h_length;
    char** h_addr_list;
#define h_addr h_addr_list[0]
};

/* WSADATA for WSAStartup */
typedef struct WSAData {
    uint16_t wVersion;
    uint16_t wHighVersion;
    char szDescription[257];
    char szSystemStatus[129];
    uint16_t iMaxSockets;
    uint16_t iMaxUdpDg;
    char* lpVendorInfo;
} WSADATA;

/* ==========================================================================
 * Winsock Function Stubs
 * All return error - use Platform_Network_* functions instead
 * ========================================================================== */

/**
 * WSAStartup - Initialize Winsock (stub)
 * @param wVersionRequested Requested Winsock version
 * @param lpWSAData Output WSADATA structure
 * @return Always 0 (success) - actual init done by Platform_Network_Init
 */
static inline int WSAStartup(uint16_t wVersionRequested, WSADATA* lpWSAData)
{
    (void)wVersionRequested;
    if (lpWSAData) {
        lpWSAData->wVersion = 0x0202;
        lpWSAData->wHighVersion = 0x0202;
        lpWSAData->iMaxSockets = 0;
        lpWSAData->iMaxUdpDg = 0;
    }
    return 0;
}

/**
 * WSACleanup - Cleanup Winsock (stub)
 * @return Always 0 (success) - actual cleanup done by Platform_Network_Shutdown
 */
static inline int WSACleanup(void)
{
    return 0;
}

/**
 * WSAGetLastError - Get last Winsock error (stub)
 * @return Always 0 (no error)
 */
static inline int WSAGetLastError(void)
{
    return 0;
}

/**
 * WSASetLastError - Set last Winsock error (stub)
 * @param iError Error code to set (ignored)
 */
static inline void WSASetLastError(int iError)
{
    (void)iError;
}

/**
 * WSAAsyncSelect - Async socket mode (stub)
 * @return -1 (not supported - use Platform_* polling instead)
 */
static inline int WSAAsyncSelect(SOCKET s, void* hWnd, unsigned int wMsg, long lEvent)
{
    (void)s;
    (void)hWnd;
    (void)wMsg;
    (void)lEvent;
    return -1;  /* Not supported */
}

/**
 * closesocket - Close a socket (stub)
 * @param s Socket to close (ignored)
 * @return Always 0
 */
static inline int closesocket(SOCKET s)
{
    (void)s;
    return 0;
}

/**
 * socket - Create a socket (stub)
 * @return INVALID_SOCKET (use Platform_Host/Client_Create instead)
 */
static inline SOCKET socket_create(int af, int type, int protocol)
{
    (void)af;
    (void)type;
    (void)protocol;
    return INVALID_SOCKET;
}

/**
 * bind - Bind socket to address (stub)
 * @return -1 (not supported)
 */
static inline int bind_socket(SOCKET s, const struct sockaddr* name, int namelen)
{
    (void)s;
    (void)name;
    (void)namelen;
    return -1;
}

/**
 * listen - Listen for connections (stub)
 * @return -1 (not supported - use Platform_Host_Create instead)
 */
static inline int listen_socket(SOCKET s, int backlog)
{
    (void)s;
    (void)backlog;
    return -1;
}

/**
 * connect - Connect to server (stub)
 * @return -1 (not supported - use Platform_Client_Connect instead)
 */
static inline int connect_socket(SOCKET s, const struct sockaddr* name, int namelen)
{
    (void)s;
    (void)name;
    (void)namelen;
    return -1;
}

/**
 * send/recv - Socket I/O (stubs)
 * @return -1 (not supported - use Platform_* functions instead)
 */
static inline int send_socket(SOCKET s, const char* buf, int len, int flags)
{
    (void)s;
    (void)buf;
    (void)len;
    (void)flags;
    return -1;
}

static inline int recv_socket(SOCKET s, char* buf, int len, int flags)
{
    (void)s;
    (void)buf;
    (void)len;
    (void)flags;
    return -1;
}

static inline int sendto_socket(SOCKET s, const char* buf, int len, int flags,
                                const struct sockaddr* to, int tolen)
{
    (void)s;
    (void)buf;
    (void)len;
    (void)flags;
    (void)to;
    (void)tolen;
    return -1;
}

static inline int recvfrom_socket(SOCKET s, char* buf, int len, int flags,
                                  struct sockaddr* from, int* fromlen)
{
    (void)s;
    (void)buf;
    (void)len;
    (void)flags;
    (void)from;
    (void)fromlen;
    return -1;
}

/* Address manipulation */
static inline uint32_t htonl_compat(uint32_t hostlong)
{
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return ((hostlong >> 24) & 0xFF) |
           ((hostlong >> 8) & 0xFF00) |
           ((hostlong << 8) & 0xFF0000) |
           ((hostlong << 24) & 0xFF000000);
#else
    return hostlong;
#endif
}

static inline uint16_t htons_compat(uint16_t hostshort)
{
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return (uint16_t)((hostshort >> 8) | (hostshort << 8));
#else
    return hostshort;
#endif
}

static inline uint32_t ntohl_compat(uint32_t netlong)
{
    return htonl_compat(netlong);
}

static inline uint16_t ntohs_compat(uint16_t netshort)
{
    return htons_compat(netshort);
}

#ifndef htonl
#define htonl htonl_compat
#endif
#ifndef htons
#define htons htons_compat
#endif
#ifndef ntohl
#define ntohl ntohl_compat
#endif
#ifndef ntohs
#define ntohs ntohs_compat
#endif

/**
 * inet_addr - Convert string to address (stub)
 * @param cp IP address string
 * @return 0 (use Platform_Client_Connect with string address instead)
 */
static inline uint32_t inet_addr_compat(const char* cp)
{
    (void)cp;
    return 0;
}

#ifndef inet_addr
#define inet_addr inet_addr_compat
#endif

/**
 * gethostbyname - DNS lookup (stub)
 * @param name Hostname to resolve
 * @return NULL (not supported - use Platform_Client_Connect with IP)
 */
static inline struct hostent* gethostbyname_compat(const char* name)
{
    (void)name;
    return NULL;
}

#ifndef gethostbyname
#define gethostbyname gethostbyname_compat
#endif

/* ==========================================================================
 * IPX Compatibility Layer
 * IPX (Internetwork Packet Exchange) is completely obsolete.
 * All functions return failure/unavailable.
 * ========================================================================== */

/* IPX address types */
#define NSPROTO_IPX     1000
#define NSPROTO_SPX     1256
#define NSPROTO_SPXII   1257

/* IPX address structure (32-bit DOS/Windows era format) */
typedef struct IPXAddress {
    uint8_t network[4];
    uint8_t node[6];
    uint8_t socket[2];
} IPXADDRESS;

/* IPX Header (from original game) */
typedef struct IPXHeader {
    uint16_t checksum;
    uint16_t length;
    uint8_t transport_control;
    uint8_t packet_type;
    IPXADDRESS dest;
    IPXADDRESS source;
} IPXHEADER;

/* IPX Address Class (stub for C++ code) */
struct IPXAddressClass {
    IPXADDRESS address;
};

/* ==========================================================================
 * IPX Function Stubs
 * All return 0 (failure) or do nothing - IPX is not available
 * ========================================================================== */

/**
 * IPX_Initialise - Initialize IPX (stub)
 * @return 0 (failure - IPX not available)
 */
static inline int IPX_Initialise(void)
{
    return 0;  /* Always fail - IPX not supported */
}

/**
 * IPX_Shutdown - Shutdown IPX (stub)
 */
static inline void IPX_Shutdown(void)
{
    /* No-op */
}

/**
 * IPX_Open_Socket - Open IPX socket (stub)
 * @param socket Socket number to open
 * @return 0 (failure)
 */
static inline int IPX_Open_Socket(int socket_num)
{
    (void)socket_num;
    return 0;
}

/**
 * IPX_Close_Socket - Close IPX socket (stub)
 * @param socket Socket number to close
 */
static inline void IPX_Close_Socket(int socket_num)
{
    (void)socket_num;
}

/**
 * IPX_Send_Packet - Send IPX packet (stub)
 * @param data Packet data
 * @param size Packet size
 * @param address Destination address
 * @return 0 (failure)
 */
static inline int IPX_Send_Packet(void* data, int size, void* address)
{
    (void)data;
    (void)size;
    (void)address;
    return 0;
}

/**
 * IPX_Receive_Packet - Receive IPX packet (stub)
 * @param data Buffer for packet data
 * @param size Pointer to store packet size
 * @param address Buffer for source address
 * @return 0 (no packet available)
 */
static inline int IPX_Receive_Packet(void* data, int* size, void* address)
{
    (void)data;
    (void)size;
    (void)address;
    return 0;
}

/**
 * IPX_Broadcast - Broadcast IPX packet (stub)
 * @param data Packet data
 * @param size Packet size
 * @return 0 (failure)
 */
static inline int IPX_Broadcast(void* data, int size)
{
    (void)data;
    (void)size;
    return 0;
}

/**
 * IPX_Get_Local_Address - Get local IPX address (stub)
 * @param address Buffer for local address
 * @return 0 (failure)
 */
static inline int IPX_Get_Local_Address(IPXADDRESS* address)
{
    if (address) {
        /* Zero out the address */
        for (int i = 0; i < 4; i++) address->network[i] = 0;
        for (int i = 0; i < 6; i++) address->node[i] = 0;
        address->socket[0] = 0;
        address->socket[1] = 0;
    }
    return 0;
}

/**
 * Is_IPX_Available - Check if IPX is available
 * @return 0 (IPX is never available)
 */
static inline int Is_IPX_Available(void)
{
    return 0;  /* IPX is obsolete and never available */
}

/**
 * IPX_Address_Compare - Compare two IPX addresses (stub)
 * @param addr1 First address
 * @param addr2 Second address
 * @return 0 (equal - both are invalid anyway)
 */
static inline int IPX_Address_Compare(const IPXADDRESS* addr1, const IPXADDRESS* addr2)
{
    (void)addr1;
    (void)addr2;
    return 0;
}

/* ==========================================================================
 * SPX (Sequenced Packet Exchange) Stubs
 * SPX provided reliable delivery over IPX - also obsolete
 * ========================================================================== */

/**
 * SPX_Initialise - Initialize SPX (stub)
 * @return 0 (failure - SPX not available)
 */
static inline int SPX_Initialise(void)
{
    return 0;
}

/**
 * SPX_Shutdown - Shutdown SPX (stub)
 */
static inline void SPX_Shutdown(void)
{
    /* No-op */
}

/**
 * Is_SPX_Available - Check if SPX is available
 * @return 0 (SPX is never available)
 */
static inline int Is_SPX_Available(void)
{
    return 0;
}

/* ==========================================================================
 * Migration Helpers
 * Helper macros to redirect old networking calls to platform layer
 * ========================================================================== */

/**
 * Redirect network initialization to platform layer
 * Old code: WSAStartup(MAKEWORD(2,2), &wsaData);
 * New code: Platform_Network_Init();
 */
#define INIT_NETWORK() Platform_Network_Init()

/**
 * Redirect network shutdown to platform layer
 * Old code: WSACleanup();
 * New code: Platform_Network_Shutdown();
 */
#define SHUTDOWN_NETWORK() Platform_Network_Shutdown()

/**
 * Check if networking is available
 * Old code: Is_IPX_Available() || WSAStartup succeeded
 * New code: Platform_Network_IsInitialized()
 */
#define IS_NETWORK_AVAILABLE() Platform_Network_IsInitialized()

#ifdef __cplusplus
}
#endif

#endif /* NETWORK_WRAPPER_H */
