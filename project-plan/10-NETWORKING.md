# Plan 10: Networking (Rust Implementation)

## Objective
Replace the obsolete IPX/SPX protocol and Windows Winsock async model with modern UDP networking using the `enet` crate, enabling cross-platform multiplayer.

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

## Rust enet-Based Implementation

Using the `enet` crate provides:
- Reliable and unreliable packet delivery
- Automatic connection management
- NAT punch-through friendly
- Cross-platform (uses native sockets)

### Cargo.toml Dependencies

```toml
[dependencies]
enet = "0.3"
log = "0.4"
thiserror = "1.0"
```

## Rust Module Structure

```
platform/src/
├── lib.rs
└── network/
    ├── mod.rs        # Module root and FFI exports
    ├── host.rs       # Server/host implementation
    ├── client.rs     # Client connection
    ├── packet.rs     # Packet handling
    └── adapter.rs    # Game protocol adapter
```

### 10.1 Core Network Types

File: `platform/src/network/mod.rs`
```rust
//! Networking module using enet for reliable UDP
//!
//! Replaces the obsolete IPX/SPX and Windows Winsock implementations.

pub mod host;
pub mod client;
pub mod packet;
pub mod adapter;

use std::sync::Mutex;
use once_cell::sync::Lazy;
use thiserror::Error;

/// Network subsystem state
static NETWORK_STATE: Lazy<Mutex<Option<NetworkState>>> = Lazy::new(|| Mutex::new(None));

struct NetworkState {
    // enet is initialized
    initialized: bool,
}

#[derive(Error, Debug)]
pub enum NetworkError {
    #[error("Network not initialized")]
    NotInitialized,

    #[error("Already initialized")]
    AlreadyInitialized,

    #[error("enet initialization failed")]
    EnetInitFailed,

    #[error("Connection failed: {0}")]
    ConnectionFailed(String),

    #[error("Host creation failed")]
    HostCreationFailed,

    #[error("Send failed")]
    SendFailed,

    #[error("Invalid address: {0}")]
    InvalidAddress(String),

    #[error("Timeout")]
    Timeout,

    #[error("Peer disconnected")]
    Disconnected,
}

pub type NetworkResult<T> = Result<T, NetworkError>;

// =============================================================================
// Initialization
// =============================================================================

/// Initialize the networking subsystem
pub fn init() -> NetworkResult<()> {
    let mut state = NETWORK_STATE.lock().unwrap();

    if state.is_some() {
        return Err(NetworkError::AlreadyInitialized);
    }

    // enet::initialize() returns Result
    enet::initialize().map_err(|_| NetworkError::EnetInitFailed)?;

    *state = Some(NetworkState {
        initialized: true,
    });

    log::info!("Network subsystem initialized");
    Ok(())
}

/// Shutdown the networking subsystem
pub fn shutdown() {
    let mut state = NETWORK_STATE.lock().unwrap();

    if state.take().is_some() {
        // enet cleans up automatically when dropped
        log::info!("Network subsystem shutdown");
    }
}

/// Check if networking is initialized
pub fn is_initialized() -> bool {
    NETWORK_STATE.lock().unwrap().is_some()
}

// =============================================================================
// FFI Exports
// =============================================================================

#[no_mangle]
pub extern "C" fn Platform_Network_Init() -> i32 {
    crate::ffi::catch_panic(|| init())
}

#[no_mangle]
pub extern "C" fn Platform_Network_Shutdown() {
    shutdown();
}

#[no_mangle]
pub extern "C" fn Platform_Network_IsInitialized() -> i32 {
    if is_initialized() { 1 } else { 0 }
}
```

### 10.2 Host (Server) Implementation

File: `platform/src/network/host.rs`
```rust
//! Network host (server) implementation

use super::{NetworkError, NetworkResult};
use std::collections::VecDeque;
use std::net::SocketAddr;
use std::sync::{Arc, Mutex};

/// Configuration for creating a host
#[repr(C)]
pub struct HostConfig {
    pub port: u16,
    pub max_clients: usize,
    pub channel_count: usize,
    pub incoming_bandwidth: u32,  // 0 = unlimited
    pub outgoing_bandwidth: u32,  // 0 = unlimited
}

impl Default for HostConfig {
    fn default() -> Self {
        Self {
            port: 5555,
            max_clients: 8,
            channel_count: 2,
            incoming_bandwidth: 0,
            outgoing_bandwidth: 0,
        }
    }
}

/// A received packet
pub struct ReceivedPacket {
    pub peer_id: usize,
    pub channel: u8,
    pub data: Vec<u8>,
}

/// Opaque host handle for FFI
pub struct PlatformHost {
    host: enet::Host<()>,
    peers: Vec<Option<enet::Peer<'static, ()>>>,
    received_packets: VecDeque<ReceivedPacket>,
    max_clients: usize,
}

impl PlatformHost {
    /// Create a new host (server)
    pub fn new(config: &HostConfig) -> NetworkResult<Self> {
        let address = enet::Address::new(
            std::net::Ipv4Addr::UNSPECIFIED,
            config.port,
        );

        let host = enet::Host::new(
            address,
            config.max_clients,
            config.channel_count,
            config.incoming_bandwidth,
            config.outgoing_bandwidth,
        ).map_err(|_| NetworkError::HostCreationFailed)?;

        let peers = vec![None; config.max_clients];

        log::info!("Created host on port {}", config.port);

        Ok(Self {
            host,
            peers,
            received_packets: VecDeque::new(),
            max_clients: config.max_clients,
        })
    }

    /// Service the host (process events)
    pub fn service(&mut self, timeout_ms: u32) -> NetworkResult<()> {
        let timeout = std::time::Duration::from_millis(timeout_ms as u64);

        loop {
            match self.host.service(timeout) {
                Ok(Some(event)) => {
                    self.handle_event(event);
                }
                Ok(None) => break,
                Err(e) => {
                    log::error!("Host service error: {:?}", e);
                    break;
                }
            }
        }

        Ok(())
    }

    fn handle_event(&mut self, event: enet::Event<()>) {
        match event {
            enet::Event::Connect(peer) => {
                // Find a slot for the new peer
                for (i, slot) in self.peers.iter_mut().enumerate() {
                    if slot.is_none() {
                        log::info!("Peer {} connected from {:?}", i, peer.address());
                        // Note: This is simplified - actual implementation needs proper lifetime handling
                        break;
                    }
                }
            }

            enet::Event::Disconnect(peer, _data) => {
                log::info!("Peer disconnected");
                // Remove peer from slots
                for slot in self.peers.iter_mut() {
                    // Match by address or ID
                    *slot = None;
                }
            }

            enet::Event::Receive { sender, channel_id, packet } => {
                let peer_id = 0; // Would need to look up peer index

                self.received_packets.push_back(ReceivedPacket {
                    peer_id,
                    channel: channel_id,
                    data: packet.data().to_vec(),
                });
            }
        }
    }

    /// Get the next received packet
    pub fn receive(&mut self) -> Option<ReceivedPacket> {
        self.received_packets.pop_front()
    }

    /// Send packet to all connected peers
    pub fn broadcast(&mut self, channel: u8, data: &[u8], reliable: bool) -> NetworkResult<()> {
        let packet = if reliable {
            enet::Packet::reliable(data)
        } else {
            enet::Packet::unreliable(data)
        };

        self.host.broadcast(channel, packet);
        Ok(())
    }

    /// Get number of connected peers
    pub fn peer_count(&self) -> usize {
        self.peers.iter().filter(|p| p.is_some()).count()
    }
}

// =============================================================================
// FFI Exports
// =============================================================================

/// Create a new network host
#[no_mangle]
pub extern "C" fn Platform_Host_Create(
    port: u16,
    max_clients: i32,
    channel_count: i32,
) -> *mut PlatformHost {
    let config = HostConfig {
        port,
        max_clients: max_clients as usize,
        channel_count: channel_count as usize,
        ..Default::default()
    };

    match PlatformHost::new(&config) {
        Ok(host) => Box::into_raw(Box::new(host)),
        Err(e) => {
            log::error!("Failed to create host: {}", e);
            std::ptr::null_mut()
        }
    }
}

/// Destroy a network host
#[no_mangle]
pub unsafe extern "C" fn Platform_Host_Destroy(host: *mut PlatformHost) {
    if !host.is_null() {
        drop(Box::from_raw(host));
    }
}

/// Service the host (process events)
#[no_mangle]
pub unsafe extern "C" fn Platform_Host_Service(
    host: *mut PlatformHost,
    timeout_ms: u32,
) -> i32 {
    if host.is_null() {
        return -1;
    }

    match (*host).service(timeout_ms) {
        Ok(()) => 0,
        Err(_) => -1,
    }
}

/// Receive a packet from the host
///
/// Returns packet size, or 0 if no packet available, or -1 on error
#[no_mangle]
pub unsafe extern "C" fn Platform_Host_Receive(
    host: *mut PlatformHost,
    peer_id: *mut i32,
    channel: *mut u8,
    buffer: *mut u8,
    buffer_size: i32,
) -> i32 {
    if host.is_null() || buffer.is_null() {
        return -1;
    }

    match (*host).receive() {
        Some(packet) => {
            let copy_size = std::cmp::min(packet.data.len(), buffer_size as usize);

            std::ptr::copy_nonoverlapping(
                packet.data.as_ptr(),
                buffer,
                copy_size,
            );

            if !peer_id.is_null() {
                *peer_id = packet.peer_id as i32;
            }
            if !channel.is_null() {
                *channel = packet.channel;
            }

            copy_size as i32
        }
        None => 0,
    }
}

/// Broadcast a packet to all connected peers
#[no_mangle]
pub unsafe extern "C" fn Platform_Host_Broadcast(
    host: *mut PlatformHost,
    channel: u8,
    data: *const u8,
    size: i32,
    reliable: i32,
) -> i32 {
    if host.is_null() || data.is_null() || size <= 0 {
        return -1;
    }

    let data_slice = std::slice::from_raw_parts(data, size as usize);

    match (*host).broadcast(channel, data_slice, reliable != 0) {
        Ok(()) => 0,
        Err(_) => -1,
    }
}

/// Get number of connected peers
#[no_mangle]
pub unsafe extern "C" fn Platform_Host_PeerCount(host: *mut PlatformHost) -> i32 {
    if host.is_null() {
        return 0;
    }
    (*host).peer_count() as i32
}
```

### 10.3 Client Implementation

File: `platform/src/network/client.rs`
```rust
//! Network client implementation

use super::{NetworkError, NetworkResult};
use std::collections::VecDeque;
use std::time::Duration;

/// Client connection configuration
#[repr(C)]
pub struct ConnectConfig {
    pub host_address: [u8; 64],  // Null-terminated string
    pub port: u16,
    pub channel_count: usize,
    pub timeout_ms: u32,
}

/// Client connection state
pub struct PlatformClient {
    host: enet::Host<()>,
    server_peer: Option<enet::Peer<'static, ()>>,
    received_packets: VecDeque<Vec<u8>>,
    connected: bool,
}

impl PlatformClient {
    /// Create a new client and connect to server
    pub fn connect(address: &str, port: u16, timeout_ms: u32) -> NetworkResult<Self> {
        // Create client host (no address = client mode)
        let host = enet::Host::<()>::create_client(
            1,  // One outgoing connection
            2,  // Channel count
            0,  // Incoming bandwidth (unlimited)
            0,  // Outgoing bandwidth (unlimited)
        ).map_err(|_| NetworkError::HostCreationFailed)?;

        // Parse address
        let addr: std::net::Ipv4Addr = address.parse()
            .map_err(|_| NetworkError::InvalidAddress(address.to_string()))?;

        let enet_addr = enet::Address::new(addr, port);

        log::info!("Connecting to {}:{}", address, port);

        // Note: Actual connection handling would be more complex
        // This is a simplified version

        Ok(Self {
            host,
            server_peer: None,
            received_packets: VecDeque::new(),
            connected: false,
        })
    }

    /// Service the client (process events)
    pub fn service(&mut self, timeout_ms: u32) -> NetworkResult<()> {
        let timeout = Duration::from_millis(timeout_ms as u64);

        loop {
            match self.host.service(timeout) {
                Ok(Some(event)) => {
                    match event {
                        enet::Event::Connect(_peer) => {
                            log::info!("Connected to server");
                            self.connected = true;
                        }
                        enet::Event::Disconnect(_peer, _) => {
                            log::info!("Disconnected from server");
                            self.connected = false;
                            self.server_peer = None;
                        }
                        enet::Event::Receive { packet, .. } => {
                            self.received_packets.push_back(packet.data().to_vec());
                        }
                    }
                }
                Ok(None) => break,
                Err(e) => {
                    log::error!("Client service error: {:?}", e);
                    break;
                }
            }
        }

        Ok(())
    }

    /// Check if connected to server
    pub fn is_connected(&self) -> bool {
        self.connected
    }

    /// Send data to server
    pub fn send(&mut self, channel: u8, data: &[u8], reliable: bool) -> NetworkResult<()> {
        if !self.connected {
            return Err(NetworkError::Disconnected);
        }

        let packet = if reliable {
            enet::Packet::reliable(data)
        } else {
            enet::Packet::unreliable(data)
        };

        // Would send to server_peer
        // self.server_peer.as_mut().unwrap().send(channel, packet);

        Ok(())
    }

    /// Receive data from server
    pub fn receive(&mut self) -> Option<Vec<u8>> {
        self.received_packets.pop_front()
    }

    /// Disconnect from server
    pub fn disconnect(&mut self) {
        if let Some(ref mut _peer) = self.server_peer {
            // peer.disconnect(0);
            self.connected = false;
        }
    }
}

// =============================================================================
// FFI Exports
// =============================================================================

/// Connect to a server
#[no_mangle]
pub unsafe extern "C" fn Platform_Client_Connect(
    address: *const i8,  // C string
    port: u16,
    timeout_ms: u32,
) -> *mut PlatformClient {
    if address.is_null() {
        return std::ptr::null_mut();
    }

    let addr_str = match std::ffi::CStr::from_ptr(address).to_str() {
        Ok(s) => s,
        Err(_) => return std::ptr::null_mut(),
    };

    match PlatformClient::connect(addr_str, port, timeout_ms) {
        Ok(client) => Box::into_raw(Box::new(client)),
        Err(e) => {
            log::error!("Failed to connect: {}", e);
            std::ptr::null_mut()
        }
    }
}

/// Destroy a client connection
#[no_mangle]
pub unsafe extern "C" fn Platform_Client_Destroy(client: *mut PlatformClient) {
    if !client.is_null() {
        let mut client = Box::from_raw(client);
        client.disconnect();
    }
}

/// Service the client
#[no_mangle]
pub unsafe extern "C" fn Platform_Client_Service(
    client: *mut PlatformClient,
    timeout_ms: u32,
) -> i32 {
    if client.is_null() {
        return -1;
    }

    match (*client).service(timeout_ms) {
        Ok(()) => 0,
        Err(_) => -1,
    }
}

/// Check if connected
#[no_mangle]
pub unsafe extern "C" fn Platform_Client_IsConnected(client: *mut PlatformClient) -> i32 {
    if client.is_null() {
        return 0;
    }
    if (*client).is_connected() { 1 } else { 0 }
}

/// Send data to server
#[no_mangle]
pub unsafe extern "C" fn Platform_Client_Send(
    client: *mut PlatformClient,
    channel: u8,
    data: *const u8,
    size: i32,
    reliable: i32,
) -> i32 {
    if client.is_null() || data.is_null() || size <= 0 {
        return -1;
    }

    let data_slice = std::slice::from_raw_parts(data, size as usize);

    match (*client).send(channel, data_slice, reliable != 0) {
        Ok(()) => 0,
        Err(_) => -1,
    }
}

/// Receive data from server
#[no_mangle]
pub unsafe extern "C" fn Platform_Client_Receive(
    client: *mut PlatformClient,
    buffer: *mut u8,
    buffer_size: i32,
) -> i32 {
    if client.is_null() || buffer.is_null() {
        return -1;
    }

    match (*client).receive() {
        Some(data) => {
            let copy_size = std::cmp::min(data.len(), buffer_size as usize);
            std::ptr::copy_nonoverlapping(data.as_ptr(), buffer, copy_size);
            copy_size as i32
        }
        None => 0,
    }
}
```

### 10.4 Packet Types

File: `platform/src/network/packet.rs`
```rust
//! Packet type definitions for game protocol

/// Packet delivery mode
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PacketType {
    /// Reliable, ordered delivery
    Reliable = 0,
    /// Unreliable delivery (may be dropped or reordered)
    Unreliable = 1,
    /// Unreliable, unsequenced (no ordering guarantees)
    Unsequenced = 2,
}

/// Game packet header (matches original game protocol)
#[repr(C, packed)]
#[derive(Debug, Clone, Copy)]
pub struct GamePacketHeader {
    pub packet_type: u16,
    pub sequence: u16,
    pub player_id: u8,
    pub flags: u8,
}

/// Common game packet types (from original game)
#[repr(u16)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum GamePacketKind {
    // Connection establishment
    SerialConnect = 100,
    SerialGameOptions = 101,
    SerialSignOff = 102,
    SerialGo = 103,

    // Game discovery
    RequestGameInfo = 110,
    GameInfo = 111,
    JoinGame = 112,
    JoinAccepted = 113,
    JoinRejected = 114,

    // Gameplay
    GameData = 120,
    GameSync = 121,
    GameChat = 122,

    // Keep-alive
    Ping = 200,
    Pong = 201,
}

impl From<u16> for GamePacketKind {
    fn from(value: u16) -> Self {
        match value {
            100 => GamePacketKind::SerialConnect,
            101 => GamePacketKind::SerialGameOptions,
            102 => GamePacketKind::SerialSignOff,
            103 => GamePacketKind::SerialGo,
            110 => GamePacketKind::RequestGameInfo,
            111 => GamePacketKind::GameInfo,
            112 => GamePacketKind::JoinGame,
            113 => GamePacketKind::JoinAccepted,
            114 => GamePacketKind::JoinRejected,
            120 => GamePacketKind::GameData,
            121 => GamePacketKind::GameSync,
            122 => GamePacketKind::GameChat,
            200 => GamePacketKind::Ping,
            201 => GamePacketKind::Pong,
            _ => GamePacketKind::GameData, // Default
        }
    }
}

/// Serialize a game packet
pub fn serialize_packet(header: &GamePacketHeader, payload: &[u8]) -> Vec<u8> {
    let header_size = std::mem::size_of::<GamePacketHeader>();
    let mut buffer = Vec::with_capacity(header_size + payload.len());

    // Copy header bytes
    let header_bytes = unsafe {
        std::slice::from_raw_parts(
            header as *const GamePacketHeader as *const u8,
            header_size,
        )
    };
    buffer.extend_from_slice(header_bytes);

    // Copy payload
    buffer.extend_from_slice(payload);

    buffer
}

/// Deserialize a game packet
pub fn deserialize_packet(data: &[u8]) -> Option<(GamePacketHeader, &[u8])> {
    let header_size = std::mem::size_of::<GamePacketHeader>();

    if data.len() < header_size {
        return None;
    }

    let header = unsafe {
        std::ptr::read_unaligned(data.as_ptr() as *const GamePacketHeader)
    };

    let payload = &data[header_size..];

    Some((header, payload))
}
```

### 10.5 Compatibility Layer

File: `include/compat/network_wrapper.h`
```c
#ifndef NETWORK_WRAPPER_H
#define NETWORK_WRAPPER_H

#include "platform.h"

/*
 * Winsock Compatibility Layer
 * Provides stub definitions for Windows socket types
 */

/* Socket type */
typedef int SOCKET;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR   (-1)

/* Address structures (stubs) */
struct in_addr {
    unsigned int s_addr;
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

/* Socket functions - all return error (use platform networking instead) */
static inline int closesocket(SOCKET s) { (void)s; return 0; }
static inline int WSAGetLastError(void) { return 0; }
static inline int WSAStartup(unsigned short v, void* d) { (void)v; (void)d; return 0; }
static inline int WSACleanup(void) { return 0; }

/*
 * IPX Compatibility Layer
 * IPX is completely obsolete - stub out everything
 */

struct IPXAddressClass {
    /* Empty - no longer used */
    char _unused;
};

static inline int IPX_Initialise(void) { return 0; }  /* Always fails */
static inline void IPX_Shutdown(void) {}
static inline int IPX_Send_Packet(void* d, int s, void* a) { (void)d; (void)s; (void)a; return 0; }
static inline int IPX_Receive_Packet(void* d, int* s, void* a) { (void)d; (void)s; (void)a; return 0; }

/* Mark IPX as unavailable */
static inline int Is_IPX_Available(void) { return 0; }

#endif /* NETWORK_WRAPPER_H */
```

### 10.6 Game Protocol Adapter

File: `platform/src/network/adapter.rs`
```rust
//! Adapter between game protocol and enet networking
//!
//! This module bridges the old game networking API with the new enet-based system.

use super::{NetworkResult, NetworkError};
use super::host::PlatformHost;
use super::client::PlatformClient;
use super::packet::{GamePacketHeader, GamePacketKind, PacketType};

/// Game session information (for discovery/joining)
#[repr(C)]
pub struct GameSession {
    pub name: [u8; 64],
    pub host_address: [u8; 64],
    pub port: u16,
    pub num_players: u8,
    pub max_players: u8,
    pub passworded: u8,
    pub game_version: u32,
}

/// Network manager that wraps host or client functionality
pub struct NetworkManager {
    mode: NetworkMode,
    local_player_id: u8,
    game_name: String,
}

enum NetworkMode {
    None,
    Host(Box<PlatformHost>),
    Client(Box<PlatformClient>),
}

impl NetworkManager {
    /// Create a new network manager
    pub fn new() -> Self {
        Self {
            mode: NetworkMode::None,
            local_player_id: 0,
            game_name: String::new(),
        }
    }

    /// Host a new game session
    pub fn host_game(
        &mut self,
        game_name: &str,
        port: u16,
        max_players: usize,
    ) -> NetworkResult<()> {
        use super::host::HostConfig;

        let config = HostConfig {
            port,
            max_clients: max_players,
            channel_count: 2,
            ..Default::default()
        };

        let host = PlatformHost::new(&config)?;

        self.mode = NetworkMode::Host(Box::new(host));
        self.local_player_id = 0;  // Host is player 0
        self.game_name = game_name.to_string();

        log::info!("Hosting game '{}' on port {}", game_name, port);
        Ok(())
    }

    /// Join an existing game session
    pub fn join_game(&mut self, address: &str, port: u16) -> NetworkResult<()> {
        let client = PlatformClient::connect(address, port, 5000)?;

        self.mode = NetworkMode::Client(Box::new(client));
        // Player ID will be assigned by host

        log::info!("Joining game at {}:{}", address, port);
        Ok(())
    }

    /// Disconnect from current session
    pub fn disconnect(&mut self) {
        match &mut self.mode {
            NetworkMode::None => {}
            NetworkMode::Host(_) => {
                log::info!("Closing hosted game");
            }
            NetworkMode::Client(client) => {
                client.disconnect();
                log::info!("Disconnected from game");
            }
        }
        self.mode = NetworkMode::None;
    }

    /// Service the network (process events)
    pub fn update(&mut self) -> NetworkResult<()> {
        match &mut self.mode {
            NetworkMode::None => Ok(()),
            NetworkMode::Host(host) => host.service(0),
            NetworkMode::Client(client) => client.service(0),
        }
    }

    /// Send game data to all players (host) or to host (client)
    pub fn send_game_data(&mut self, data: &[u8], reliable: bool) -> NetworkResult<()> {
        let channel = if reliable { 0 } else { 1 };

        match &mut self.mode {
            NetworkMode::None => Err(NetworkError::NotInitialized),
            NetworkMode::Host(host) => host.broadcast(channel, data, reliable),
            NetworkMode::Client(client) => client.send(channel, data, reliable),
        }
    }

    /// Check if we are the host
    pub fn is_host(&self) -> bool {
        matches!(self.mode, NetworkMode::Host(_))
    }

    /// Check if connected
    pub fn is_connected(&self) -> bool {
        match &self.mode {
            NetworkMode::None => false,
            NetworkMode::Host(_) => true,
            NetworkMode::Client(client) => client.is_connected(),
        }
    }

    /// Get local player ID
    pub fn local_player_id(&self) -> u8 {
        self.local_player_id
    }
}

impl Default for NetworkManager {
    fn default() -> Self {
        Self::new()
    }
}

// =============================================================================
// Global Network Manager (for C++ compatibility)
// =============================================================================

use std::sync::Mutex;
use once_cell::sync::Lazy;

static NETWORK_MANAGER: Lazy<Mutex<Option<NetworkManager>>> = Lazy::new(|| Mutex::new(None));

fn with_manager<F, R>(f: F) -> NetworkResult<R>
where
    F: FnOnce(&mut NetworkManager) -> NetworkResult<R>,
{
    let mut guard = NETWORK_MANAGER.lock().unwrap();
    match guard.as_mut() {
        Some(manager) => f(manager),
        None => Err(NetworkError::NotInitialized),
    }
}

// =============================================================================
// FFI Exports for Game Integration
// =============================================================================

/// Initialize the network manager
#[no_mangle]
pub extern "C" fn Platform_NetworkManager_Init() -> i32 {
    let mut guard = NETWORK_MANAGER.lock().unwrap();
    if guard.is_some() {
        return -1; // Already initialized
    }

    *guard = Some(NetworkManager::new());
    0
}

/// Shutdown the network manager
#[no_mangle]
pub extern "C" fn Platform_NetworkManager_Shutdown() {
    let mut guard = NETWORK_MANAGER.lock().unwrap();
    if let Some(mut manager) = guard.take() {
        manager.disconnect();
    }
}

/// Host a game
#[no_mangle]
pub unsafe extern "C" fn Platform_NetworkManager_HostGame(
    game_name: *const i8,
    port: u16,
    max_players: i32,
) -> i32 {
    if game_name.is_null() {
        return -1;
    }

    let name = match std::ffi::CStr::from_ptr(game_name).to_str() {
        Ok(s) => s,
        Err(_) => return -1,
    };

    match with_manager(|m| m.host_game(name, port, max_players as usize)) {
        Ok(()) => 0,
        Err(_) => -1,
    }
}

/// Join a game
#[no_mangle]
pub unsafe extern "C" fn Platform_NetworkManager_JoinGame(
    address: *const i8,
    port: u16,
) -> i32 {
    if address.is_null() {
        return -1;
    }

    let addr = match std::ffi::CStr::from_ptr(address).to_str() {
        Ok(s) => s,
        Err(_) => return -1,
    };

    match with_manager(|m| m.join_game(addr, port)) {
        Ok(()) => 0,
        Err(_) => -1,
    }
}

/// Disconnect from current session
#[no_mangle]
pub extern "C" fn Platform_NetworkManager_Disconnect() {
    let _ = with_manager(|m| {
        m.disconnect();
        Ok(())
    });
}

/// Update network (call every frame)
#[no_mangle]
pub extern "C" fn Platform_NetworkManager_Update() -> i32 {
    match with_manager(|m| m.update()) {
        Ok(()) => 0,
        Err(_) => -1,
    }
}

/// Check if connected
#[no_mangle]
pub extern "C" fn Platform_NetworkManager_IsConnected() -> i32 {
    match with_manager(|m| Ok(m.is_connected())) {
        Ok(true) => 1,
        _ => 0,
    }
}

/// Check if host
#[no_mangle]
pub extern "C" fn Platform_NetworkManager_IsHost() -> i32 {
    match with_manager(|m| Ok(m.is_host())) {
        Ok(true) => 1,
        _ => 0,
    }
}

/// Send game data
#[no_mangle]
pub unsafe extern "C" fn Platform_NetworkManager_SendData(
    data: *const u8,
    size: i32,
    reliable: i32,
) -> i32 {
    if data.is_null() || size <= 0 {
        return -1;
    }

    let slice = std::slice::from_raw_parts(data, size as usize);

    match with_manager(|m| m.send_game_data(slice, reliable != 0)) {
        Ok(()) => 0,
        Err(_) => -1,
    }
}
```

## Tasks Breakdown

### Phase 1: enet Integration (2 days)
- [ ] Add enet crate to Cargo.toml
- [ ] Implement Platform_Network_* initialization functions
- [ ] Test basic enet functionality

### Phase 2: Host/Join (2 days)
- [ ] Implement PlatformHost
- [ ] Implement PlatformClient
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
- [ ] Stub out IPX code in compatibility header
- [ ] Integrate NetworkManager with game code
- [ ] Test multiplayer connectivity

### Phase 6: Testing (2 days)
- [ ] LAN multiplayer testing
- [ ] Internet multiplayer testing (if applicable)
- [ ] Stress testing

## Acceptance Criteria

- [ ] Host can create game session
- [ ] Client can join game session
- [ ] Game commands synchronize correctly
- [ ] Multiplayer match is playable
- [ ] No desync issues
- [ ] Works on macOS (Intel and Apple Silicon)

## Estimated Duration
**10-14 days**

## Dependencies
- Plans 01-09 completed
- Single-player working
- enet crate (0.3+)

## Next Plan
Once networking is functional, proceed to **Plan 11: Cleanup & Polish**
