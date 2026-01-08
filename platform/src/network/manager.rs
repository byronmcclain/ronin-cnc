//! Network Manager - Unified networking API
//!
//! Provides a high-level interface for game networking that wraps
//! the lower-level host and client implementations.

use super::client::{ConnectConfig, PlatformClient};
use super::host::{HostConfig, PlatformHost};
use super::packet::{serialize_packet, GamePacketCode, GamePacketHeader};
use super::{NetworkError, NetworkResult};
use std::collections::VecDeque;

/// Network operating mode
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum NetworkMode {
    /// Not in a network session
    #[default]
    None = 0,
    /// Hosting a game (server mode)
    Hosting = 1,
    /// Joined a game (client mode)
    Joined = 2,
}

/// Received packet info (unified format)
#[derive(Debug, Clone)]
pub struct NetworkPacket {
    /// Peer that sent the packet (0 = host when in client mode)
    pub peer_id: u32,
    /// Channel received on
    pub channel: u8,
    /// Packet data
    pub data: Vec<u8>,
}

/// Game session information
#[repr(C)]
#[derive(Debug, Clone)]
pub struct GameSessionInfo {
    /// Game/session name
    pub name: [u8; 64],
    /// Port the game is hosted on
    pub port: u16,
    /// Number of players currently connected
    pub player_count: u8,
    /// Maximum players allowed
    pub max_players: u8,
    /// Local player ID
    pub local_player_id: u8,
}

impl Default for GameSessionInfo {
    fn default() -> Self {
        Self {
            name: [0u8; 64],
            port: 5555,
            player_count: 0,
            max_players: 8,
            local_player_id: 0,
        }
    }
}

impl GameSessionInfo {
    /// Set the game name from a string
    pub fn set_name(&mut self, name: &str) {
        let bytes = name.as_bytes();
        let len = bytes.len().min(63);
        self.name[..len].copy_from_slice(&bytes[..len]);
        self.name[len] = 0; // Null terminate
    }

    /// Get the game name as a string
    pub fn get_name(&self) -> &str {
        let end = self.name.iter().position(|&b| b == 0).unwrap_or(64);
        std::str::from_utf8(&self.name[..end]).unwrap_or("")
    }
}

/// Network Manager
///
/// Manages the network session state and provides unified host/client API.
pub struct NetworkManager {
    /// Current operating mode
    mode: NetworkMode,
    /// Host instance (when hosting)
    host: Option<Box<PlatformHost>>,
    /// Client instance (when joined)
    client: Option<Box<PlatformClient>>,
    /// Session information
    session: GameSessionInfo,
    /// Received packets queue
    received_packets: VecDeque<NetworkPacket>,
    /// Packet sequence counter
    sequence: u32,
}

impl NetworkManager {
    /// Create a new network manager
    pub fn new() -> Self {
        Self {
            mode: NetworkMode::None,
            host: None,
            client: None,
            session: GameSessionInfo::default(),
            received_packets: VecDeque::new(),
            sequence: 0,
        }
    }

    /// Get the current network mode
    pub fn mode(&self) -> NetworkMode {
        self.mode
    }

    /// Check if currently hosting a game
    pub fn is_host(&self) -> bool {
        self.mode == NetworkMode::Hosting
    }

    /// Check if connected (either as host or client)
    pub fn is_connected(&self) -> bool {
        match self.mode {
            NetworkMode::None => false,
            NetworkMode::Hosting => true, // Host is always "connected"
            NetworkMode::Joined => self
                .client
                .as_ref()
                .map(|c| c.is_connected())
                .unwrap_or(false),
        }
    }

    /// Get session info
    pub fn session_info(&self) -> &GameSessionInfo {
        &self.session
    }

    /// Host a new game session
    ///
    /// # Arguments
    /// * `name` - Game session name
    /// * `port` - Port to host on
    /// * `max_players` - Maximum allowed players
    pub fn host_game(&mut self, name: &str, port: u16, max_players: u8) -> NetworkResult<()> {
        // Can't host while already in a session
        if self.mode != NetworkMode::None {
            self.disconnect();
        }

        let config = HostConfig {
            port,
            max_clients: max_players as u32,
            channel_count: 2,
            ..Default::default()
        };

        let host = PlatformHost::new(&config)?;

        self.host = Some(Box::new(host));
        self.mode = NetworkMode::Hosting;
        self.session.set_name(name);
        self.session.port = port;
        self.session.max_players = max_players;
        self.session.player_count = 1; // Host is player 1
        self.session.local_player_id = 0; // Host is always player 0

        log::info!("Hosting game '{}' on port {}", name, port);
        Ok(())
    }

    /// Join an existing game session
    ///
    /// # Arguments
    /// * `address` - Server IP address
    /// * `port` - Server port
    /// * `timeout_ms` - Connection timeout in milliseconds
    pub fn join_game(&mut self, address: &str, port: u16, timeout_ms: u32) -> NetworkResult<()> {
        if self.mode != NetworkMode::None {
            self.disconnect();
        }

        let config = ConnectConfig {
            channel_count: 2,
            timeout_ms,
            ..Default::default()
        };

        let mut client = PlatformClient::new(&config)?;
        client.connect_blocking(address, port, 2, timeout_ms)?;

        self.client = Some(Box::new(client));
        self.mode = NetworkMode::Joined;
        self.session.port = port;
        // Player ID will be assigned by host

        log::info!("Joined game at {}:{}", address, port);
        Ok(())
    }

    /// Join a game asynchronously (non-blocking)
    ///
    /// Call update() to complete the connection, then check is_connected().
    pub fn join_game_async(&mut self, address: &str, port: u16) -> NetworkResult<()> {
        if self.mode != NetworkMode::None {
            self.disconnect();
        }

        let config = ConnectConfig::default();
        let mut client = PlatformClient::new(&config)?;
        client.connect_async(address, port, 2)?;

        self.client = Some(Box::new(client));
        self.mode = NetworkMode::Joined;
        self.session.port = port;

        log::info!("Connecting to game at {}:{}", address, port);
        Ok(())
    }

    /// Disconnect from current session
    pub fn disconnect(&mut self) {
        match self.mode {
            NetworkMode::Hosting => {
                self.host = None;
                log::info!("Stopped hosting game");
            }
            NetworkMode::Joined => {
                if let Some(mut client) = self.client.take() {
                    client.disconnect();
                }
                log::info!("Left game");
            }
            NetworkMode::None => {}
        }

        self.mode = NetworkMode::None;
        self.session = GameSessionInfo::default();
        self.received_packets.clear();
    }

    /// Update network state - call every frame
    ///
    /// Processes incoming packets and connection events.
    /// Returns the number of packets received.
    pub fn update(&mut self) -> NetworkResult<usize> {
        let initial_count = self.received_packets.len();

        match self.mode {
            NetworkMode::None => {}

            NetworkMode::Hosting => {
                if let Some(ref mut host) = self.host {
                    // Service the host
                    host.service(0)?;

                    // Collect received packets
                    while let Some(packet) = host.receive() {
                        self.received_packets.push_back(NetworkPacket {
                            peer_id: packet.peer_index,
                            channel: packet.channel,
                            data: packet.data,
                        });
                    }

                    self.session.player_count = (host.peer_count() + 1) as u8;
                }
            }

            NetworkMode::Joined => {
                if let Some(ref mut client) = self.client {
                    // Service the client
                    client.service(0)?;

                    // Collect received packets
                    while let Some(data) = client.receive() {
                        self.received_packets.push_back(NetworkPacket {
                            peer_id: 0, // From host
                            channel: client.last_channel(),
                            data,
                        });
                    }
                }
            }
        }

        Ok(self.received_packets.len() - initial_count)
    }

    /// Send data to peers
    ///
    /// When hosting: broadcasts to all clients
    /// When joined: sends to host
    pub fn send_data(&mut self, channel: u8, data: &[u8], reliable: bool) -> NetworkResult<()> {
        match self.mode {
            NetworkMode::None => Err(NetworkError::NotInitialized),

            NetworkMode::Hosting => {
                if let Some(ref mut host) = self.host {
                    host.broadcast(channel, data, reliable)
                } else {
                    Err(NetworkError::NotInitialized)
                }
            }

            NetworkMode::Joined => {
                if let Some(ref mut client) = self.client {
                    client.send(channel, data, reliable)
                } else {
                    Err(NetworkError::Disconnected)
                }
            }
        }
    }

    /// Send a game packet
    pub fn send_packet(
        &mut self,
        code: GamePacketCode,
        payload: &[u8],
        reliable: bool,
    ) -> NetworkResult<()> {
        let header =
            GamePacketHeader::new(code, self.next_sequence(), self.session.local_player_id);
        let packet = serialize_packet(&header, payload);
        let channel = if reliable { 0 } else { 1 };
        self.send_data(channel, &packet, reliable)
    }

    /// Receive next packet
    pub fn receive(&mut self) -> Option<NetworkPacket> {
        self.received_packets.pop_front()
    }

    /// Check if packets are available
    pub fn has_packets(&self) -> bool {
        !self.received_packets.is_empty()
    }

    /// Get packet count
    pub fn packet_count(&self) -> usize {
        self.received_packets.len()
    }

    /// Get peer count
    pub fn peer_count(&self) -> u32 {
        match self.mode {
            NetworkMode::Hosting => self.host.as_ref().map(|h| h.peer_count()).unwrap_or(0),
            NetworkMode::Joined => {
                if self.is_connected() {
                    1
                } else {
                    0
                }
            }
            NetworkMode::None => 0,
        }
    }

    /// Get next sequence number
    fn next_sequence(&mut self) -> u32 {
        let seq = self.sequence;
        self.sequence = self.sequence.wrapping_add(1);
        seq
    }
}

impl Default for NetworkManager {
    fn default() -> Self {
        Self::new()
    }
}

// =============================================================================
// FFI Exports
// =============================================================================

/// Create a new network manager
///
/// Returns pointer to manager, or null on failure
#[no_mangle]
pub extern "C" fn Platform_NetworkManager_Create() -> *mut NetworkManager {
    Box::into_raw(Box::new(NetworkManager::new()))
}

/// Destroy a network manager
///
/// # Safety
/// * `manager` must be a valid pointer from Platform_NetworkManager_Create
#[no_mangle]
pub unsafe extern "C" fn Platform_NetworkManager_Destroy(manager: *mut NetworkManager) {
    if !manager.is_null() {
        let _ = Box::from_raw(manager);
    }
}

/// Initialize the network manager (compatibility alias)
#[no_mangle]
pub extern "C" fn Platform_NetworkManager_Init() -> i32 {
    0 // No-op, manager is created separately
}

/// Shutdown the network manager (compatibility alias)
#[no_mangle]
pub unsafe extern "C" fn Platform_NetworkManager_Shutdown(manager: *mut NetworkManager) {
    if !manager.is_null() {
        (*manager).disconnect();
    }
}

/// Get current network mode
///
/// Returns: 0=None, 1=Hosting, 2=Joined, -1=error
#[no_mangle]
pub unsafe extern "C" fn Platform_NetworkManager_GetMode(manager: *mut NetworkManager) -> i32 {
    if manager.is_null() {
        return -1;
    }
    (*manager).mode() as i32
}

/// Host a new game
///
/// Returns 0 on success, -1 on error
#[no_mangle]
pub unsafe extern "C" fn Platform_NetworkManager_HostGame(
    manager: *mut NetworkManager,
    name: *const std::os::raw::c_char,
    port: u16,
    max_players: i32,
) -> i32 {
    if manager.is_null() || name.is_null() {
        return -1;
    }

    let name_str = match std::ffi::CStr::from_ptr(name).to_str() {
        Ok(s) => s,
        Err(_) => return -1,
    };

    match (*manager).host_game(name_str, port, max_players.max(1).min(8) as u8) {
        Ok(()) => 0,
        Err(_) => -1,
    }
}

/// Join an existing game
///
/// Returns 0 on success, -1 on error
#[no_mangle]
pub unsafe extern "C" fn Platform_NetworkManager_JoinGame(
    manager: *mut NetworkManager,
    address: *const std::os::raw::c_char,
    port: u16,
    timeout_ms: u32,
) -> i32 {
    if manager.is_null() || address.is_null() {
        return -1;
    }

    let addr_str = match std::ffi::CStr::from_ptr(address).to_str() {
        Ok(s) => s,
        Err(_) => return -1,
    };

    match (*manager).join_game(addr_str, port, timeout_ms) {
        Ok(()) => 0,
        Err(_) => -1,
    }
}

/// Join a game asynchronously
#[no_mangle]
pub unsafe extern "C" fn Platform_NetworkManager_JoinGameAsync(
    manager: *mut NetworkManager,
    address: *const std::os::raw::c_char,
    port: u16,
) -> i32 {
    if manager.is_null() || address.is_null() {
        return -1;
    }

    let addr_str = match std::ffi::CStr::from_ptr(address).to_str() {
        Ok(s) => s,
        Err(_) => return -1,
    };

    match (*manager).join_game_async(addr_str, port) {
        Ok(()) => 0,
        Err(_) => -1,
    }
}

/// Disconnect from current session
#[no_mangle]
pub unsafe extern "C" fn Platform_NetworkManager_Disconnect(manager: *mut NetworkManager) {
    if !manager.is_null() {
        (*manager).disconnect();
    }
}

/// Update network - call every frame
///
/// Returns number of packets received, or -1 on error
#[no_mangle]
pub unsafe extern "C" fn Platform_NetworkManager_Update(manager: *mut NetworkManager) -> i32 {
    if manager.is_null() {
        return -1;
    }

    match (*manager).update() {
        Ok(count) => count as i32,
        Err(_) => -1,
    }
}

/// Check if hosting
#[no_mangle]
pub unsafe extern "C" fn Platform_NetworkManager_IsHost(manager: *mut NetworkManager) -> i32 {
    if manager.is_null() {
        return 0;
    }
    if (*manager).is_host() {
        1
    } else {
        0
    }
}

/// Check if connected
#[no_mangle]
pub unsafe extern "C" fn Platform_NetworkManager_IsConnected(manager: *mut NetworkManager) -> i32 {
    if manager.is_null() {
        return 0;
    }
    if (*manager).is_connected() {
        1
    } else {
        0
    }
}

/// Get peer count
#[no_mangle]
pub unsafe extern "C" fn Platform_NetworkManager_PeerCount(manager: *mut NetworkManager) -> i32 {
    if manager.is_null() {
        return 0;
    }
    (*manager).peer_count() as i32
}

/// Send data
///
/// Returns 0 on success, -1 on error
#[no_mangle]
pub unsafe extern "C" fn Platform_NetworkManager_SendData(
    manager: *mut NetworkManager,
    channel: u8,
    data: *const u8,
    size: i32,
    reliable: i32,
) -> i32 {
    if manager.is_null() || data.is_null() || size <= 0 {
        return -1;
    }

    let slice = std::slice::from_raw_parts(data, size as usize);

    match (*manager).send_data(channel, slice, reliable != 0) {
        Ok(()) => 0,
        Err(_) => -1,
    }
}

/// Receive data
///
/// Returns bytes copied, 0 if no data, -1 on error
#[no_mangle]
pub unsafe extern "C" fn Platform_NetworkManager_ReceiveData(
    manager: *mut NetworkManager,
    peer_id: *mut u32,
    channel: *mut u8,
    buffer: *mut u8,
    buffer_size: i32,
) -> i32 {
    if manager.is_null() || buffer.is_null() || buffer_size <= 0 {
        return -1;
    }

    match (*manager).receive() {
        Some(packet) => {
            let copy_size = packet.data.len().min(buffer_size as usize);
            std::ptr::copy_nonoverlapping(packet.data.as_ptr(), buffer, copy_size);

            if !peer_id.is_null() {
                *peer_id = packet.peer_id;
            }
            if !channel.is_null() {
                *channel = packet.channel;
            }

            copy_size as i32
        }
        None => 0,
    }
}

/// Check if packets are available
#[no_mangle]
pub unsafe extern "C" fn Platform_NetworkManager_HasPackets(manager: *mut NetworkManager) -> i32 {
    if manager.is_null() {
        return -1;
    }
    if (*manager).has_packets() {
        1
    } else {
        0
    }
}

/// Get local player ID
#[no_mangle]
pub unsafe extern "C" fn Platform_NetworkManager_GetLocalPlayerId(
    manager: *mut NetworkManager,
) -> i32 {
    if manager.is_null() {
        return -1;
    }
    (*manager).session_info().local_player_id as i32
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    fn ensure_init() {
        let _ = super::super::init();
    }

    #[test]
    fn test_manager_initial_state() {
        ensure_init();

        let mut manager = NetworkManager::new();
        assert_eq!(manager.mode(), NetworkMode::None);
        assert!(!manager.is_host());
        assert!(!manager.is_connected());
        assert_eq!(manager.peer_count(), 0);
    }

    #[test]
    fn test_host_game() {
        ensure_init();

        let mut manager = NetworkManager::new();
        let result = manager.host_game("Test Game", 16000, 4);

        match result {
            Ok(()) => {
                assert_eq!(manager.mode(), NetworkMode::Hosting);
                assert!(manager.is_host());
                assert!(manager.is_connected());
                assert_eq!(manager.session_info().get_name(), "Test Game");
                assert_eq!(manager.session_info().port, 16000);
                manager.disconnect();
            }
            Err(e) => {
                // Port might be in use
                println!("Host failed (port in use?): {}", e);
            }
        }
    }

    #[test]
    fn test_disconnect() {
        ensure_init();

        let mut manager = NetworkManager::new();
        // Disconnect when not connected should be fine
        manager.disconnect();
        assert_eq!(manager.mode(), NetworkMode::None);
    }

    #[test]
    fn test_ffi_manager() {
        ensure_init();

        unsafe {
            let manager = Platform_NetworkManager_Create();
            assert!(!manager.is_null());

            assert_eq!(Platform_NetworkManager_GetMode(manager), 0); // None
            assert_eq!(Platform_NetworkManager_IsHost(manager), 0);
            assert_eq!(Platform_NetworkManager_IsConnected(manager), 0);
            assert_eq!(Platform_NetworkManager_PeerCount(manager), 0);

            Platform_NetworkManager_Disconnect(manager); // Should not panic

            Platform_NetworkManager_Destroy(manager);
        }
    }

    #[test]
    fn test_ffi_null_safety() {
        unsafe {
            assert_eq!(Platform_NetworkManager_GetMode(std::ptr::null_mut()), -1);
            assert_eq!(Platform_NetworkManager_IsHost(std::ptr::null_mut()), 0);
            assert_eq!(Platform_NetworkManager_IsConnected(std::ptr::null_mut()), 0);
            assert_eq!(Platform_NetworkManager_PeerCount(std::ptr::null_mut()), 0);

            Platform_NetworkManager_Disconnect(std::ptr::null_mut()); // Should not panic
            Platform_NetworkManager_Destroy(std::ptr::null_mut()); // Should not panic
        }
    }

    #[test]
    fn test_session_info() {
        let mut info = GameSessionInfo::default();
        info.set_name("My Game");
        assert_eq!(info.get_name(), "My Game");

        // Test long name truncation
        let long_name = "A".repeat(100);
        info.set_name(&long_name);
        assert_eq!(info.get_name().len(), 63);
    }
}
