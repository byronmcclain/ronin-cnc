# Plan 10: Networking

## Objective
Replace the obsolete IPX/SPX protocol and Windows Winsock async model with modern UDP networking using the enet library, enabling cross-platform multiplayer.

## Current State Analysis

### IPX Protocol (Obsolete)

**Files:**
- `IPX/` directory (20+ files)
- `CODE/IPX.CPP`, `IPX.H`
- `CODE/IPXADDR.CPP`, `IPXADDR.H`
- `CODE/IPXCONN.CPP`, `IPXMGR.CPP`
- `CODE/IPX95.CPP`, `IPX95.H`
- `CODE/IPXPROT.ASM`, `IPXREAL.ASM`

**IPX Characteristics:**
- LAN-only protocol (no internet routing)
- Connectionless (like UDP)
- Hardware addressing (network:node format)
- Broadcast-based game discovery
- No longer supported by modern operating systems

**Recommendation: Remove entirely and replace with UDP**

### Winsock Implementation

**Files:**
- `CODE/TCPIP.CPP`, `TCPIP.H`
- `CODE/WSPIPX.CPP`, `WSPIPX.H`
- `CODE/WSPUDP.CPP`, `WSPUDP.H`
- `CODE/WSPROTO.CPP`, `WSPROTO.H`

**Winsock APIs Used:**
```cpp
WSAStartup(), WSACleanup();
socket(), bind(), listen(), accept(), connect(), closesocket();
send(), recv(), sendto(), recvfrom();
WSAAsyncSelect();  // Async socket model
WSAAsyncGetHostByName(), WSAAsyncGetHostByAddr();
```

**Key Classes:**
```cpp
class TcpipManagerClass {
    SOCKET ListenSocket;
    SOCKET UDPSocket;
    InternetBufferType ReceiveBuffers[WS_NUM_RX_BUFFERS];
    InternetBufferType TransmitBuffers[WS_NUM_TX_BUFFERS];
};
```

## enet-Based Implementation

Using enet provides:
- Reliable and unreliable packet delivery
- Automatic connection management
- NAT punch-through friendly
- Cross-platform (POSIX sockets)

### 10.1 Network Manager

File: `src/platform/network_enet.cpp`
```cpp
#include "platform/platform_network.h"
#include <enet/enet.h>
#include <string>
#include <vector>
#include <mutex>
#include <queue>

namespace {

bool g_network_initialized = false;

} // anonymous namespace

//=============================================================================
// Initialization
//=============================================================================

bool Platform_Network_Init(void) {
    if (g_network_initialized) return true;

    if (enet_initialize() != 0) {
        Platform_SetError("enet_initialize failed");
        return false;
    }

    g_network_initialized = true;
    return true;
}

void Platform_Network_Shutdown(void) {
    if (!g_network_initialized) return;

    enet_deinitialize();
    g_network_initialized = false;
}
```

### 10.2 Host (Server) Implementation

```cpp
//=============================================================================
// Host Structure
//=============================================================================

struct PlatformHost {
    ENetHost* host;
    std::vector<ENetPeer*> peers;
    std::queue<ReceivedPacket> received_packets;
    std::mutex mutex;
};

PlatformHost* Platform_Host_Create(const HostConfig* config) {
    if (!g_network_initialized) return nullptr;

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = config->port;

    ENetHost* host = enet_host_create(
        &address,
        config->max_clients,
        config->channel_count,
        0,  // Incoming bandwidth (0 = unlimited)
        0   // Outgoing bandwidth (0 = unlimited)
    );

    if (!host) {
        Platform_SetError("enet_host_create failed");
        return nullptr;
    }

    PlatformHost* phost = new PlatformHost();
    phost->host = host;
    return phost;
}

void Platform_Host_Destroy(PlatformHost* host) {
    if (!host) return;

    // Disconnect all peers
    for (ENetPeer* peer : host->peers) {
        enet_peer_disconnect(peer, 0);
    }

    // Wait for disconnects
    ENetEvent event;
    while (enet_host_service(host->host, &event, 1000) > 0) {
        if (event.type == ENET_EVENT_TYPE_RECEIVE) {
            enet_packet_destroy(event.packet);
        }
    }

    // Force disconnect remaining
    for (ENetPeer* peer : host->peers) {
        enet_peer_reset(peer);
    }

    enet_host_destroy(host->host);
    delete host;
}

void Platform_Host_Service(PlatformHost* host, uint32_t timeout_ms) {
    if (!host) return;

    ENetEvent event;

    while (enet_host_service(host->host, &event, timeout_ms) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                std::lock_guard<std::mutex> lock(host->mutex);
                host->peers.push_back(event.peer);
                Platform_Log(LOG_INFO, "Peer connected from %x:%u",
                            event.peer->address.host, event.peer->address.port);
                break;
            }

            case ENET_EVENT_TYPE_RECEIVE: {
                std::lock_guard<std::mutex> lock(host->mutex);

                ReceivedPacket packet;
                packet.peer = (PlatformPeer*)event.peer;
                packet.channel = event.channelID;
                packet.size = event.packet->dataLength;
                packet.data = malloc(packet.size);
                memcpy(packet.data, event.packet->data, packet.size);

                host->received_packets.push(packet);
                enet_packet_destroy(event.packet);
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT: {
                std::lock_guard<std::mutex> lock(host->mutex);
                auto it = std::find(host->peers.begin(), host->peers.end(), event.peer);
                if (it != host->peers.end()) {
                    host->peers.erase(it);
                }
                Platform_Log(LOG_INFO, "Peer disconnected");
                break;
            }

            default:
                break;
        }

        timeout_ms = 0;  // Only block on first iteration
    }
}
```

### 10.3 Client Implementation

```cpp
//=============================================================================
// Client Connection
//=============================================================================

PlatformPeer* Platform_Connect(const ConnectConfig* config, uint32_t timeout_ms) {
    if (!g_network_initialized) return nullptr;

    // Create client host
    ENetHost* client = enet_host_create(
        nullptr,  // No address = client
        1,        // Only 1 outgoing connection
        config->channel_count,
        0, 0
    );

    if (!client) {
        Platform_SetError("enet_host_create failed for client");
        return nullptr;
    }

    // Resolve address
    ENetAddress address;
    enet_address_set_host(&address, config->host_address);
    address.port = config->port;

    // Initiate connection
    ENetPeer* peer = enet_host_connect(client, &address, config->channel_count, 0);
    if (!peer) {
        enet_host_destroy(client);
        Platform_SetError("enet_host_connect failed");
        return nullptr;
    }

    // Wait for connection
    ENetEvent event;
    if (enet_host_service(client, &event, timeout_ms) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT) {
        Platform_Log(LOG_INFO, "Connected to %s:%u", config->host_address, config->port);
        return (PlatformPeer*)peer;
    }

    enet_peer_reset(peer);
    enet_host_destroy(client);
    Platform_SetError("Connection timed out");
    return nullptr;
}

void Platform_Disconnect(PlatformPeer* peer) {
    if (!peer) return;

    ENetPeer* epeer = (ENetPeer*)peer;
    enet_peer_disconnect(epeer, 0);

    // Wait for acknowledgment
    ENetEvent event;
    ENetHost* host = epeer->host;

    while (enet_host_service(host, &event, 1000) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_RECEIVE:
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                Platform_Log(LOG_INFO, "Disconnected");
                return;
            default:
                break;
        }
    }

    enet_peer_reset(epeer);
}

bool Platform_IsConnected(PlatformPeer* peer) {
    if (!peer) return false;
    ENetPeer* epeer = (ENetPeer*)peer;
    return epeer->state == ENET_PEER_STATE_CONNECTED;
}
```

### 10.4 Sending and Receiving

```cpp
//=============================================================================
// Packet Transmission
//=============================================================================

bool Platform_Send(PlatformPeer* peer, int channel, const void* data, int size, PacketType type) {
    if (!peer || !data || size <= 0) return false;

    ENetPeer* epeer = (ENetPeer*)peer;

    enet_uint32 flags = 0;
    switch (type) {
        case PACKET_RELIABLE:
            flags = ENET_PACKET_FLAG_RELIABLE;
            break;
        case PACKET_UNRELIABLE:
            flags = 0;
            break;
        case PACKET_UNSEQUENCED:
            flags = ENET_PACKET_FLAG_UNSEQUENCED;
            break;
    }

    ENetPacket* packet = enet_packet_create(data, size, flags);
    if (!packet) return false;

    int result = enet_peer_send(epeer, channel, packet);
    return result == 0;
}

bool Platform_Receive(PlatformHost* host, ReceivedPacket* packet) {
    if (!host) return false;

    std::lock_guard<std::mutex> lock(host->mutex);

    if (host->received_packets.empty()) {
        return false;
    }

    *packet = host->received_packets.front();
    host->received_packets.pop();
    return true;
}

void Platform_FreePacket(ReceivedPacket* packet) {
    if (packet && packet->data) {
        free(packet->data);
        packet->data = nullptr;
    }
}

//=============================================================================
// Peer Information
//=============================================================================

void Platform_GetPeerAddress(PlatformPeer* peer, char* buffer, int buffer_size) {
    if (!peer || !buffer || buffer_size <= 0) return;

    ENetPeer* epeer = (ENetPeer*)peer;
    enet_address_get_host_ip(&epeer->address, buffer, buffer_size);
}

uint32_t Platform_GetPeerRTT(PlatformPeer* peer) {
    if (!peer) return 0;
    ENetPeer* epeer = (ENetPeer*)peer;
    return epeer->roundTripTime;
}
```

### 10.5 Game Protocol Adapter

Bridge between old IPX-style game code and new enet networking:

File: `src/game/network_adapter.cpp`
```cpp
#include "platform/platform_network.h"

//=============================================================================
// Game Packet Types (from original game)
//=============================================================================

enum GamePacketType {
    PACKET_SERIAL_CONNECT = 100,
    PACKET_SERIAL_GAME_OPTIONS,
    PACKET_SERIAL_SIGN_OFF,
    PACKET_SERIAL_GO,
    PACKET_REQUEST_GAME_INFO,
    PACKET_GAME_INFO,
    PACKET_JOIN_GAME,
    PACKET_JOIN_ACCEPTED,
    PACKET_GAME_DATA,
    // ... etc
};

//=============================================================================
// Session Discovery (Replaces IPX Broadcast)
//=============================================================================

// Use a simple approach: connect to known server or use lobby

struct GameSession {
    char name[64];
    char host[64];
    uint16_t port;
    int num_players;
    int max_players;
    bool passworded;
};

// For LAN discovery, could use UDP broadcast on local network
// For internet play, would need a lobby/master server

//=============================================================================
// Connection Manager (Replaces TcpipManagerClass)
//=============================================================================

class NetworkManager {
private:
    PlatformHost* host;
    PlatformPeer* server_peer;  // If client
    bool is_host;
    int player_id;

public:
    NetworkManager() : host(nullptr), server_peer(nullptr), is_host(false), player_id(0) {}

    bool HostGame(const char* game_name, uint16_t port, int max_players) {
        HostConfig config;
        config.port = port;
        config.max_clients = max_players;
        config.channel_count = 2;  // Reliable + unreliable

        host = Platform_Host_Create(&config);
        if (!host) return false;

        is_host = true;
        player_id = 0;  // Host is player 0
        return true;
    }

    bool JoinGame(const char* address, uint16_t port) {
        ConnectConfig config;
        config.host_address = address;
        config.port = port;
        config.channel_count = 2;

        server_peer = Platform_Connect(&config, 5000);
        if (!server_peer) return false;

        is_host = false;
        return true;
    }

    void Disconnect() {
        if (server_peer) {
            Platform_Disconnect(server_peer);
            server_peer = nullptr;
        }
        if (host) {
            Platform_Host_Destroy(host);
            host = nullptr;
        }
    }

    bool SendToAll(const void* data, int size, bool reliable) {
        // Implementation depends on host/client role
        return true;
    }

    bool SendTo(int player, const void* data, int size, bool reliable) {
        // Send to specific player
        return true;
    }

    void Update() {
        if (host) {
            Platform_Host_Service(host, 0);
        }
        // Process received packets...
    }
};

// Global network manager
NetworkManager* Network = nullptr;
```

### 10.6 Compatibility Layer

File: `include/compat/network_wrapper.h`
```cpp
#ifndef NETWORK_WRAPPER_H
#define NETWORK_WRAPPER_H

#include "platform/platform_network.h"

//=============================================================================
// Winsock Compatibility
//=============================================================================

// Socket type
typedef int SOCKET;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR   (-1)

// Address structures
struct in_addr {
    uint32_t s_addr;
};

struct sockaddr_in {
    short sin_family;
    unsigned short sin_port;
    struct in_addr sin_addr;
    char sin_zero[8];
};

struct sockaddr {
    unsigned short sa_family;
    char sa_data[14];
};

// Socket functions (stubs - actual networking uses enet)
inline int closesocket(SOCKET s) { return 0; }
inline int WSAGetLastError() { return 0; }

//=============================================================================
// IPX Compatibility (Stub Everything)
//=============================================================================

// IPX is completely obsolete - stub out all functions
struct IPXAddressClass {
    // Empty - no longer used
};

inline bool IPX_Initialise() { return false; }
inline void IPX_Shutdown() {}
inline bool IPX_Send_Packet(void* data, int size, void* addr) { return false; }
inline bool IPX_Receive_Packet(void* data, int* size, void* addr) { return false; }

// Mark IPX as unavailable
inline bool Is_IPX_Available() { return false; }

#endif // NETWORK_WRAPPER_H
```

## Game Integration Strategy

### 10.7 Modifications to Game Code

The networking code in Red Alert is extensive. Key areas to modify:

1. **Session.cpp** - Game session management
2. **NetPlay.cpp** - Multiplayer game logic
3. **Queue.cpp** - Command queue synchronization

Approach:
1. Create adapter layer that mimics old API
2. Replace IPX calls with "not available"
3. Modify TCP/IP path to use enet
4. Update UI to remove IPX options

## Tasks Breakdown

### Phase 1: enet Integration (2 days)
- [ ] Add enet to build system
- [ ] Implement Platform_Network_* functions
- [ ] Test basic connectivity

### Phase 2: Host/Join (2 days)
- [ ] Implement host creation
- [ ] Implement client connection
- [ ] Test connection/disconnection

### Phase 3: Packet Transmission (2 days)
- [ ] Implement reliable/unreliable sending
- [ ] Implement packet receiving
- [ ] Test with ping-pong packets

### Phase 4: Game Protocol (3 days)
- [ ] Analyze existing game protocol
- [ ] Create protocol adapter
- [ ] Map game packets to enet

### Phase 5: Integration (3 days)
- [ ] Stub out IPX code
- [ ] Integrate with game networking
- [ ] Test multiplayer game

### Phase 6: Testing (2 days)
- [ ] LAN multiplayer testing
- [ ] Internet multiplayer testing
- [ ] Stress testing

## Acceptance Criteria

- [ ] Host can create game session
- [ ] Client can join game session
- [ ] Game commands synchronize
- [ ] Multiplayer match playable
- [ ] No desync issues

## Estimated Duration
**10-14 days**

## Dependencies
- Plans 01-09 completed
- Single-player working
- enet library

## Next Plan
Once networking is functional, proceed to **Plan 11: Cleanup & Polish**
