//! Network client implementation
//!
//! The client connects to a host (server) to join a multiplayer game:
//! - Initiates connection to server address
//! - Sends packets to server
//! - Receives packets from server

use super::{NetworkError, NetworkResult, ENET_HANDLE};
use enet::{Address, BandwidthLimit, ChannelLimit, Event, Host, Packet, PacketMode};
use std::collections::VecDeque;
use std::net::Ipv4Addr;
use std::time::{Duration, Instant};

/// Configuration for client connection
#[repr(C)]
#[derive(Debug, Clone)]
pub struct ConnectConfig {
    /// Number of channels (should match server)
    pub channel_count: usize,
    /// Connection timeout in milliseconds
    pub timeout_ms: u32,
    /// Incoming bandwidth limit (0 = unlimited)
    pub incoming_bandwidth: u32,
    /// Outgoing bandwidth limit (0 = unlimited)
    pub outgoing_bandwidth: u32,
}

impl Default for ConnectConfig {
    fn default() -> Self {
        Self {
            channel_count: 2,
            timeout_ms: 5000,
            incoming_bandwidth: 0,
            outgoing_bandwidth: 0,
        }
    }
}

/// Client connection state
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ClientState {
    /// Not connected
    Disconnected = 0,
    /// Connection in progress
    Connecting = 1,
    /// Connected to server
    Connected = 2,
    /// Connection failed or lost
    Failed = 3,
}

/// Network event types for client
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ClientEvent {
    /// No event occurred
    None = 0,
    /// Connected to server
    Connected = 1,
    /// Disconnected from server
    Disconnected = 2,
    /// Packet received (check receive queue)
    PacketReceived = 3,
    /// Connection failed
    ConnectionFailed = 4,
}

/// Platform network client
///
/// Connects to a network host and exchanges packets.
pub struct PlatformClient {
    /// The underlying enet host (in client mode)
    host: Host<()>,
    /// Queue of received packets
    received_packets: VecDeque<Vec<u8>>,
    /// Current connection state
    state: ClientState,
    /// Last event for FFI
    last_event: ClientEvent,
    /// Server address we're connecting/connected to
    server_address: String,
    /// Server port
    server_port: u16,
    /// Channel for received packets
    last_channel: u8,
}

impl PlatformClient {
    /// Create a new client (does not connect yet)
    ///
    /// # Arguments
    /// * `config` - Connection configuration
    ///
    /// # Returns
    /// * `Ok(PlatformClient)` on success
    /// * `Err(NetworkError)` if client creation fails
    pub fn new(config: &ConnectConfig) -> NetworkResult<Self> {
        super::require_initialized()?;

        // Get the enet handle
        let enet = ENET_HANDLE.get().ok_or(NetworkError::NotInitialized)?;

        // Convert bandwidth limits
        let incoming_bw = if config.incoming_bandwidth == 0 {
            BandwidthLimit::Unlimited
        } else {
            BandwidthLimit::Limited(config.incoming_bandwidth)
        };
        let outgoing_bw = if config.outgoing_bandwidth == 0 {
            BandwidthLimit::Unlimited
        } else {
            BandwidthLimit::Limited(config.outgoing_bandwidth)
        };

        // Convert channel limit
        let channel_limit = if config.channel_count == 0 {
            ChannelLimit::Maximum
        } else {
            ChannelLimit::Limited(config.channel_count)
        };

        // Create client-mode host (no specific binding - pass None for address)
        let host = enet
            .create_host::<()>(
                None, // No binding address for client
                1,    // Max peers (just the server)
                channel_limit,
                incoming_bw,
                outgoing_bw,
            )
            .map_err(|e| NetworkError::HostCreationFailed(format!("Client: {:?}", e)))?;

        log::info!("Created network client");

        Ok(Self {
            host,
            received_packets: VecDeque::new(),
            state: ClientState::Disconnected,
            last_event: ClientEvent::None,
            server_address: String::new(),
            server_port: 0,
            last_channel: 0,
        })
    }

    /// Initiate connection to server (async)
    ///
    /// This starts the connection process. You must call `service()` to
    /// complete the connection and check `is_connected()` or `state()`.
    ///
    /// # Arguments
    /// * `address` - Server IP address (e.g., "127.0.0.1")
    /// * `port` - Server port
    /// * `channel_count` - Number of channels to use
    ///
    /// # Returns
    /// * `Ok(())` if connection initiated
    /// * `Err(NetworkError)` if failed to start connection
    pub fn connect_async(
        &mut self,
        address: &str,
        port: u16,
        channel_count: usize,
    ) -> NetworkResult<()> {
        if self.state == ClientState::Connected || self.state == ClientState::Connecting {
            return Err(NetworkError::AlreadyInitialized);
        }

        // Parse address
        let ip: Ipv4Addr = address
            .parse()
            .map_err(|_| NetworkError::InvalidAddress(address.to_string()))?;

        let enet_addr = Address::new(ip, port);

        // Initiate connection
        let _peer = self
            .host
            .connect(&enet_addr, channel_count, 0)
            .map_err(|e| NetworkError::ConnectionFailed(format!("{:?}", e)))?;

        self.state = ClientState::Connecting;
        self.server_address = address.to_string();
        self.server_port = port;

        log::info!("Connecting to {}:{}", address, port);
        Ok(())
    }

    /// Connect to server with timeout (blocking)
    ///
    /// Blocks until connected or timeout expires.
    ///
    /// # Arguments
    /// * `address` - Server IP address
    /// * `port` - Server port
    /// * `channel_count` - Number of channels
    /// * `timeout_ms` - Maximum time to wait for connection
    ///
    /// # Returns
    /// * `Ok(())` if connected successfully
    /// * `Err(NetworkError::Timeout)` if connection timed out
    /// * `Err(NetworkError)` for other failures
    pub fn connect_blocking(
        &mut self,
        address: &str,
        port: u16,
        channel_count: usize,
        timeout_ms: u32,
    ) -> NetworkResult<()> {
        self.connect_async(address, port, channel_count)?;

        let deadline = Instant::now() + Duration::from_millis(timeout_ms as u64);

        while Instant::now() < deadline {
            self.service(100)?;

            match self.state {
                ClientState::Connected => {
                    return Ok(());
                }
                ClientState::Failed | ClientState::Disconnected => {
                    return Err(NetworkError::ConnectionFailed(
                        "Connection rejected or failed".to_string(),
                    ));
                }
                ClientState::Connecting => {
                    // Keep waiting
                    continue;
                }
            }
        }

        self.state = ClientState::Failed;
        Err(NetworkError::Timeout)
    }

    /// Service the client - process network events
    ///
    /// Must be called regularly to process incoming packets and
    /// connection state changes.
    ///
    /// # Arguments
    /// * `timeout_ms` - Maximum time to wait for events (0 = non-blocking)
    ///
    /// # Returns
    /// The type of event that occurred (if any)
    pub fn service(&mut self, timeout_ms: u32) -> NetworkResult<ClientEvent> {
        self.last_event = ClientEvent::None;

        loop {
            match self.host.service(timeout_ms) {
                Ok(Some(event)) => {
                    match event {
                        Event::Connect(ref _peer) => {
                            self.state = ClientState::Connected;
                            self.last_event = ClientEvent::Connected;
                            log::info!(
                                "Connected to server at {}:{}",
                                self.server_address,
                                self.server_port
                            );
                        }

                        Event::Disconnect(ref _peer, _data) => {
                            self.state = ClientState::Disconnected;
                            self.last_event = ClientEvent::Disconnected;
                            log::info!("Disconnected from server");
                        }

                        Event::Receive {
                            channel_id,
                            ref packet,
                            ..
                        } => {
                            self.received_packets.push_back(packet.data().to_vec());
                            self.last_channel = channel_id;
                            self.last_event = ClientEvent::PacketReceived;
                        }
                    }
                    // Only process one event if we have a timeout (break after first)
                    if timeout_ms > 0 {
                        break;
                    }
                }
                Ok(None) => break,
                Err(e) => {
                    log::error!("Client service error: {:?}", e);
                    break;
                }
            }
        }

        Ok(self.last_event)
    }

    /// Check if connected to server
    pub fn is_connected(&self) -> bool {
        self.state == ClientState::Connected
    }

    /// Get current connection state
    pub fn state(&self) -> ClientState {
        self.state
    }

    /// Send data to server
    ///
    /// # Arguments
    /// * `channel` - Channel to send on (0 typically reliable, 1 unreliable)
    /// * `data` - Packet data
    /// * `reliable` - If true, packet will be retransmitted until acknowledged
    ///
    /// # Returns
    /// * `Ok(())` on success
    /// * `Err(NetworkError::Disconnected)` if not connected
    pub fn send(&mut self, channel: u8, data: &[u8], reliable: bool) -> NetworkResult<()> {
        if self.state != ClientState::Connected {
            return Err(NetworkError::Disconnected);
        }

        let mode = if reliable {
            PacketMode::ReliableSequenced
        } else {
            PacketMode::UnreliableSequenced
        };

        let packet =
            Packet::new(data, mode).map_err(|_| NetworkError::SendFailed)?;

        // Get the server peer and send
        // In client mode, there's only one peer (the server)
        for mut peer in self.host.peers() {
            peer.send_packet(packet, channel)
                .map_err(|_| NetworkError::SendFailed)?;
            return Ok(());
        }

        Err(NetworkError::Disconnected)
    }

    /// Receive data from server
    ///
    /// # Returns
    /// * `Some(Vec<u8>)` if a packet is available
    /// * `None` if no packets in queue
    pub fn receive(&mut self) -> Option<Vec<u8>> {
        self.received_packets.pop_front()
    }

    /// Check if there are packets waiting
    pub fn has_packets(&self) -> bool {
        !self.received_packets.is_empty()
    }

    /// Get number of packets in queue
    pub fn packet_count(&self) -> usize {
        self.received_packets.len()
    }

    /// Get the channel of the last received packet
    pub fn last_channel(&self) -> u8 {
        self.last_channel
    }

    /// Disconnect from server
    pub fn disconnect(&mut self) {
        if self.state == ClientState::Connected || self.state == ClientState::Connecting {
            for mut peer in self.host.peers() {
                peer.disconnect(0);
            }
            self.host.flush();
            log::info!("Disconnect initiated");
        }
        self.state = ClientState::Disconnected;
    }

    /// Get the server address we're connected/connecting to
    pub fn server_address(&self) -> &str {
        &self.server_address
    }

    /// Get the server port
    pub fn server_port(&self) -> u16 {
        self.server_port
    }

    /// Flush pending outgoing packets
    pub fn flush(&mut self) {
        self.host.flush();
    }
}

impl Drop for PlatformClient {
    fn drop(&mut self) {
        self.disconnect();
    }
}

// =============================================================================
// FFI Exports
// =============================================================================

/// Create a new network client
///
/// # Arguments
/// * `channel_count` - Number of channels (should match server)
///
/// # Returns
/// * Pointer to PlatformClient on success
/// * null on failure
#[no_mangle]
pub extern "C" fn Platform_Client_Create(channel_count: i32) -> *mut PlatformClient {
    let config = ConnectConfig {
        channel_count: channel_count.max(1) as usize,
        ..Default::default()
    };

    match PlatformClient::new(&config) {
        Ok(client) => Box::into_raw(Box::new(client)),
        Err(e) => {
            log::error!("Failed to create client: {}", e);
            std::ptr::null_mut()
        }
    }
}

/// Destroy a network client
///
/// # Safety
/// * `client` must be a valid pointer from Platform_Client_Create
#[no_mangle]
pub unsafe extern "C" fn Platform_Client_Destroy(client: *mut PlatformClient) {
    if !client.is_null() {
        let _ = Box::from_raw(client);
        log::info!("Client destroyed");
    }
}

/// Connect to a server (async)
///
/// Starts connection process. Call Service to complete.
///
/// # Arguments
/// * `client` - Client pointer
/// * `address` - Server address (C string, e.g., "127.0.0.1")
/// * `port` - Server port
///
/// # Returns
/// * 0 on success (connection initiated)
/// * -1 on error
#[no_mangle]
pub unsafe extern "C" fn Platform_Client_Connect(
    client: *mut PlatformClient,
    address: *const std::os::raw::c_char,
    port: u16,
) -> i32 {
    if client.is_null() || address.is_null() {
        return -1;
    }

    let addr_str = match std::ffi::CStr::from_ptr(address).to_str() {
        Ok(s) => s,
        Err(_) => return -1,
    };

    match (*client).connect_async(addr_str, port, 2) {
        Ok(()) => 0,
        Err(_) => -1,
    }
}

/// Connect to a server (blocking)
///
/// Blocks until connected or timeout.
///
/// # Returns
/// * 0 on success
/// * -1 on error or timeout
#[no_mangle]
pub unsafe extern "C" fn Platform_Client_ConnectBlocking(
    client: *mut PlatformClient,
    address: *const std::os::raw::c_char,
    port: u16,
    timeout_ms: u32,
) -> i32 {
    if client.is_null() || address.is_null() {
        return -1;
    }

    let addr_str = match std::ffi::CStr::from_ptr(address).to_str() {
        Ok(s) => s,
        Err(_) => return -1,
    };

    match (*client).connect_blocking(addr_str, port, 2, timeout_ms) {
        Ok(()) => 0,
        Err(_) => -1,
    }
}

/// Service the client (process events)
///
/// # Returns
/// * 0 = No event
/// * 1 = Connected
/// * 2 = Disconnected
/// * 3 = Packet received
/// * 4 = Connection failed
/// * -1 = Error
#[no_mangle]
pub unsafe extern "C" fn Platform_Client_Service(
    client: *mut PlatformClient,
    timeout_ms: u32,
) -> i32 {
    if client.is_null() {
        return -1;
    }

    match (*client).service(timeout_ms) {
        Ok(event) => event as i32,
        Err(_) => -1,
    }
}

/// Check if client is connected
///
/// # Returns
/// * 1 if connected
/// * 0 if not connected
#[no_mangle]
pub unsafe extern "C" fn Platform_Client_IsConnected(client: *mut PlatformClient) -> i32 {
    if client.is_null() {
        return 0;
    }
    if (*client).is_connected() {
        1
    } else {
        0
    }
}

/// Get client connection state
///
/// # Returns
/// * 0 = Disconnected
/// * 1 = Connecting
/// * 2 = Connected
/// * 3 = Failed
#[no_mangle]
pub unsafe extern "C" fn Platform_Client_GetState(client: *mut PlatformClient) -> i32 {
    if client.is_null() {
        return 0;
    }
    (*client).state() as i32
}

/// Send data to server
///
/// # Arguments
/// * `client` - Client pointer
/// * `channel` - Channel to send on
/// * `data` - Packet data
/// * `size` - Data size
/// * `reliable` - 1 for reliable, 0 for unreliable
///
/// # Returns
/// * 0 on success
/// * -1 on error
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
///
/// # Arguments
/// * `client` - Client pointer
/// * `buffer` - Buffer to receive into
/// * `buffer_size` - Size of buffer
///
/// # Returns
/// * Positive: bytes received
/// * 0: no data available
/// * -1: error
#[no_mangle]
pub unsafe extern "C" fn Platform_Client_Receive(
    client: *mut PlatformClient,
    buffer: *mut u8,
    buffer_size: i32,
) -> i32 {
    if client.is_null() || buffer.is_null() || buffer_size <= 0 {
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

/// Check if client has packets waiting
#[no_mangle]
pub unsafe extern "C" fn Platform_Client_HasPackets(client: *mut PlatformClient) -> i32 {
    if client.is_null() {
        return -1;
    }
    if (*client).has_packets() {
        1
    } else {
        0
    }
}

/// Disconnect from server
#[no_mangle]
pub unsafe extern "C" fn Platform_Client_Disconnect(client: *mut PlatformClient) {
    if !client.is_null() {
        (*client).disconnect();
    }
}

/// Flush pending outgoing packets
#[no_mangle]
pub unsafe extern "C" fn Platform_Client_Flush(client: *mut PlatformClient) {
    if !client.is_null() {
        (*client).flush();
    }
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
    fn test_connect_config_default() {
        let config = ConnectConfig::default();
        assert_eq!(config.channel_count, 2);
        assert_eq!(config.timeout_ms, 5000);
    }

    #[test]
    fn test_client_creation() {
        ensure_init();

        let config = ConnectConfig::default();
        let result = PlatformClient::new(&config);

        match result {
            Ok(client) => {
                assert_eq!(client.state(), ClientState::Disconnected);
                assert!(!client.is_connected());
                assert!(!client.has_packets());
            }
            Err(e) => {
                println!("Client creation failed: {}", e);
            }
        }
    }

    #[test]
    fn test_client_connect_invalid_address() {
        ensure_init();

        let config = ConnectConfig::default();
        if let Ok(mut client) = PlatformClient::new(&config) {
            // Invalid address should fail
            let result = client.connect_async("not.an.ip.address", 5555, 2);
            assert!(result.is_err());
        }
    }

    #[test]
    fn test_client_connect_timeout() {
        ensure_init();

        let config = ConnectConfig::default();
        if let Ok(mut client) = PlatformClient::new(&config) {
            // Connect to localhost with very short timeout (no server running)
            let result = client.connect_blocking("127.0.0.1", 19999, 2, 100);

            // Should timeout since no server
            assert!(result.is_err());
            assert_eq!(client.state(), ClientState::Failed);
        }
    }

    #[test]
    fn test_ffi_client_lifecycle() {
        ensure_init();

        unsafe {
            let client = Platform_Client_Create(2);

            if !client.is_null() {
                assert_eq!(Platform_Client_IsConnected(client), 0);
                assert_eq!(Platform_Client_GetState(client), 0); // Disconnected
                assert_eq!(Platform_Client_HasPackets(client), 0);

                // Service should work even when disconnected
                let event = Platform_Client_Service(client, 0);
                assert!(event >= 0);

                Platform_Client_Destroy(client);
            }
        }
    }

    #[test]
    fn test_ffi_null_safety() {
        unsafe {
            assert_eq!(Platform_Client_IsConnected(std::ptr::null_mut()), 0);
            assert_eq!(Platform_Client_GetState(std::ptr::null_mut()), 0);
            assert_eq!(Platform_Client_HasPackets(std::ptr::null_mut()), -1);
            assert_eq!(Platform_Client_Service(std::ptr::null_mut(), 0), -1);

            let addr = b"127.0.0.1\0".as_ptr() as *const i8;
            assert_eq!(
                Platform_Client_Connect(std::ptr::null_mut(), addr, 5555),
                -1
            );

            Platform_Client_Disconnect(std::ptr::null_mut()); // Should not panic
            Platform_Client_Flush(std::ptr::null_mut()); // Should not panic
            Platform_Client_Destroy(std::ptr::null_mut()); // Should not panic
        }
    }

    #[test]
    fn test_send_when_disconnected() {
        ensure_init();

        let config = ConnectConfig::default();
        if let Ok(mut client) = PlatformClient::new(&config) {
            let result = client.send(0, b"test", true);
            assert!(result.is_err());
            assert_eq!(result.unwrap_err(), NetworkError::Disconnected);
        }
    }
}
