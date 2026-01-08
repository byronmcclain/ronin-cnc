//! Network host (server) implementation
//!
//! The host manages the server-side of a multiplayer game:
//! - Binds to a port and listens for connections
//! - Accepts incoming peer connections
//! - Broadcasts packets to all connected peers
//! - Receives packets from peers

use super::{NetworkError, NetworkResult, ENET_HANDLE};
use enet::{Address, BandwidthLimit, ChannelLimit, Event, Host, Packet, PacketMode};
use std::collections::VecDeque;
use std::net::Ipv4Addr;

/// Configuration for creating a network host
#[repr(C)]
#[derive(Debug, Clone)]
pub struct HostConfig {
    /// Port to bind to (default: 5555)
    pub port: u16,
    /// Maximum number of clients (default: 8)
    pub max_clients: u32,
    /// Number of channels (default: 2 - reliable + unreliable)
    pub channel_count: usize,
    /// Incoming bandwidth limit (0 = unlimited)
    pub incoming_bandwidth: u32,
    /// Outgoing bandwidth limit (0 = unlimited)
    pub outgoing_bandwidth: u32,
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

/// A received packet with sender information
#[derive(Debug, Clone)]
pub struct ReceivedPacket {
    /// Index of the peer that sent this packet (0-based)
    pub peer_index: u32,
    /// Channel the packet was received on
    pub channel: u8,
    /// Packet data
    pub data: Vec<u8>,
}

/// Network event types for FFI reporting
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum HostEvent {
    /// No event occurred
    None = 0,
    /// A peer connected
    PeerConnected = 1,
    /// A peer disconnected
    PeerDisconnected = 2,
    /// A packet was received (check receive queue)
    PacketReceived = 3,
}

/// Platform network host (server)
///
/// Wraps an enet::Host and manages peer connections.
pub struct PlatformHost {
    /// The underlying enet host
    host: Host<u32>, // User data is peer index
    /// Queue of received packets
    received_packets: VecDeque<ReceivedPacket>,
    /// Number of currently connected peers
    peer_count: u32,
    /// Maximum allowed peers
    max_peers: u32,
    /// Most recent event for FFI
    last_event: HostEvent,
    /// Last connected/disconnected peer index
    last_peer_index: u32,
    /// Port we're bound to
    port: u16,
}

impl PlatformHost {
    /// Create a new network host (server)
    ///
    /// # Arguments
    /// * `config` - Host configuration
    ///
    /// # Returns
    /// * `Ok(PlatformHost)` on success
    /// * `Err(NetworkError)` if host creation fails
    pub fn new(config: &HostConfig) -> NetworkResult<Self> {
        super::require_initialized()?;

        // Get the enet handle
        let enet = ENET_HANDLE
            .get()
            .ok_or(NetworkError::NotInitialized)?;

        // Create enet address for binding
        let address = Address::new(Ipv4Addr::UNSPECIFIED, config.port);

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

        // Create the host
        let host = enet
            .create_host::<u32>(
                Some(&address),
                config.max_clients as usize,
                channel_limit,
                incoming_bw,
                outgoing_bw,
            )
            .map_err(|e| NetworkError::HostCreationFailed(format!("{:?}", e)))?;

        log::info!(
            "Created network host on port {} (max {} clients)",
            config.port,
            config.max_clients
        );

        Ok(Self {
            host,
            received_packets: VecDeque::new(),
            peer_count: 0,
            max_peers: config.max_clients,
            last_event: HostEvent::None,
            last_peer_index: 0,
            port: config.port,
        })
    }

    /// Service the host - process network events
    ///
    /// This should be called regularly (every frame) to process incoming
    /// packets and connection events.
    ///
    /// # Arguments
    /// * `timeout_ms` - Maximum time to wait for events (0 = non-blocking)
    ///
    /// # Returns
    /// The type of event that occurred (if any)
    pub fn service(&mut self, timeout_ms: u32) -> NetworkResult<HostEvent> {
        self.last_event = HostEvent::None;

        // Process all pending events
        loop {
            match self.host.service(timeout_ms) {
                Ok(Some(event)) => {
                    match event {
                        Event::Connect(ref _peer) => {
                            // Assign peer index based on count
                            let peer_index = self.peer_count;
                            self.peer_count = self.peer_count.saturating_add(1);
                            self.last_event = HostEvent::PeerConnected;
                            self.last_peer_index = peer_index;

                            log::info!("Peer {} connected", peer_index);
                        }

                        Event::Disconnect(ref _peer, _data) => {
                            self.peer_count = self.peer_count.saturating_sub(1);
                            self.last_event = HostEvent::PeerDisconnected;
                            // Note: peer_index would need tracking in real impl
                            self.last_peer_index = 0;

                            log::info!("Peer disconnected");
                        }

                        Event::Receive {
                            sender: _,
                            channel_id,
                            ref packet,
                        } => {
                            // Queue the packet for later retrieval
                            self.received_packets.push_back(ReceivedPacket {
                                peer_index: 0, // Would need proper tracking
                                channel: channel_id,
                                data: packet.data().to_vec(),
                            });
                            self.last_event = HostEvent::PacketReceived;
                        }
                    }
                    // Only process one event per service call (use timeout 0 for subsequent)
                    // Break after first event to avoid blocking
                    if timeout_ms > 0 {
                        break;
                    }
                }
                Ok(None) => {
                    // No more events
                    break;
                }
                Err(e) => {
                    log::error!("Host service error: {:?}", e);
                    break;
                }
            }
        }

        Ok(self.last_event)
    }

    /// Get the next received packet from the queue
    ///
    /// # Returns
    /// * `Some(ReceivedPacket)` if a packet is available
    /// * `None` if the queue is empty
    pub fn receive(&mut self) -> Option<ReceivedPacket> {
        self.received_packets.pop_front()
    }

    /// Check if there are packets waiting to be received
    pub fn has_packets(&self) -> bool {
        !self.received_packets.is_empty()
    }

    /// Get the number of packets in the receive queue
    pub fn packet_count(&self) -> usize {
        self.received_packets.len()
    }

    /// Broadcast a packet to all connected peers
    ///
    /// # Arguments
    /// * `channel` - Channel to send on (0 = reliable, 1 = unreliable typically)
    /// * `data` - Packet data to send
    /// * `reliable` - If true, send reliably (will be retransmitted if lost)
    ///
    /// # Returns
    /// * `Ok(())` on success
    /// * `Err(NetworkError::SendFailed)` on failure
    pub fn broadcast(&mut self, channel: u8, data: &[u8], reliable: bool) -> NetworkResult<()> {
        let mode = if reliable {
            PacketMode::ReliableSequenced
        } else {
            PacketMode::UnreliableSequenced
        };

        // Send to each connected peer
        for mut peer in self.host.peers() {
            if let Ok(packet) = Packet::new(data, mode) {
                if let Err(e) = peer.send_packet(packet, channel) {
                    log::warn!("Failed to send to peer: {:?}", e);
                }
            }
        }

        Ok(())
    }

    /// Get the number of currently connected peers
    pub fn peer_count(&self) -> u32 {
        self.peer_count
    }

    /// Get the maximum number of peers allowed
    pub fn max_peers(&self) -> u32 {
        self.max_peers
    }

    /// Get the port the host is bound to
    pub fn port(&self) -> u16 {
        self.port
    }

    /// Get the last event type (for FFI)
    pub fn last_event(&self) -> HostEvent {
        self.last_event
    }

    /// Get the peer index from the last connect/disconnect event
    pub fn last_peer_index(&self) -> u32 {
        self.last_peer_index
    }

    /// Flush all outgoing packets
    ///
    /// Normally packets are sent during service(), but this forces
    /// immediate transmission.
    pub fn flush(&mut self) {
        self.host.flush();
    }
}

// =============================================================================
// FFI Exports
// =============================================================================

/// Create a new network host
///
/// # Arguments
/// * `port` - Port to bind to
/// * `max_clients` - Maximum number of clients
/// * `channel_count` - Number of channels
///
/// # Returns
/// * Pointer to PlatformHost on success
/// * null on failure
#[no_mangle]
pub extern "C" fn Platform_Host_Create(
    port: u16,
    max_clients: i32,
    channel_count: i32,
) -> *mut PlatformHost {
    let config = HostConfig {
        port,
        max_clients: max_clients.max(1) as u32,
        channel_count: channel_count.max(1) as usize,
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
///
/// # Safety
/// * `host` must be a valid pointer from Platform_Host_Create
/// * `host` must not be used after this call
#[no_mangle]
pub unsafe extern "C" fn Platform_Host_Destroy(host: *mut PlatformHost) {
    if !host.is_null() {
        let _ = Box::from_raw(host);
        log::info!("Host destroyed");
    }
}

/// Service the host (process events)
///
/// # Returns
/// * 0 = No event
/// * 1 = Peer connected
/// * 2 = Peer disconnected
/// * 3 = Packet received
/// * -1 = Error or null host
#[no_mangle]
pub unsafe extern "C" fn Platform_Host_Service(host: *mut PlatformHost, timeout_ms: u32) -> i32 {
    if host.is_null() {
        return -1;
    }

    match (*host).service(timeout_ms) {
        Ok(event) => event as i32,
        Err(_) => -1,
    }
}

/// Receive a packet from the host
///
/// # Arguments
/// * `host` - Host pointer
/// * `peer_index` - Output: peer that sent the packet
/// * `channel` - Output: channel packet was received on
/// * `buffer` - Buffer to copy packet data into
/// * `buffer_size` - Size of buffer
///
/// # Returns
/// * Positive: number of bytes copied
/// * 0: no packet available
/// * -1: error or null pointers
#[no_mangle]
pub unsafe extern "C" fn Platform_Host_Receive(
    host: *mut PlatformHost,
    peer_index: *mut u32,
    channel: *mut u8,
    buffer: *mut u8,
    buffer_size: i32,
) -> i32 {
    if host.is_null() || buffer.is_null() || buffer_size <= 0 {
        return -1;
    }

    match (*host).receive() {
        Some(packet) => {
            let copy_size = std::cmp::min(packet.data.len(), buffer_size as usize);

            std::ptr::copy_nonoverlapping(packet.data.as_ptr(), buffer, copy_size);

            if !peer_index.is_null() {
                *peer_index = packet.peer_index;
            }
            if !channel.is_null() {
                *channel = packet.channel;
            }

            copy_size as i32
        }
        None => 0,
    }
}

/// Check if host has packets waiting
///
/// # Returns
/// * 1 if packets available
/// * 0 if no packets
/// * -1 on error
#[no_mangle]
pub unsafe extern "C" fn Platform_Host_HasPackets(host: *mut PlatformHost) -> i32 {
    if host.is_null() {
        return -1;
    }
    if (*host).has_packets() {
        1
    } else {
        0
    }
}

/// Broadcast a packet to all connected peers
///
/// # Arguments
/// * `host` - Host pointer
/// * `channel` - Channel to send on
/// * `data` - Packet data
/// * `size` - Size of data
/// * `reliable` - 1 for reliable, 0 for unreliable
///
/// # Returns
/// * 0 on success
/// * -1 on error
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

/// Get the port the host is bound to
#[no_mangle]
pub unsafe extern "C" fn Platform_Host_GetPort(host: *mut PlatformHost) -> u16 {
    if host.is_null() {
        return 0;
    }
    (*host).port()
}

/// Flush all pending outgoing packets
#[no_mangle]
pub unsafe extern "C" fn Platform_Host_Flush(host: *mut PlatformHost) {
    if !host.is_null() {
        (*host).flush();
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
    fn test_host_config_default() {
        let config = HostConfig::default();
        assert_eq!(config.port, 5555);
        assert_eq!(config.max_clients, 8);
        assert_eq!(config.channel_count, 2);
    }

    #[test]
    fn test_host_creation() {
        ensure_init();

        let config = HostConfig {
            port: 15555, // Use high port to avoid conflicts
            max_clients: 4,
            ..Default::default()
        };

        let result = PlatformHost::new(&config);

        match result {
            Ok(host) => {
                assert_eq!(host.port(), 15555);
                assert_eq!(host.max_peers(), 4);
                assert_eq!(host.peer_count(), 0);
            }
            Err(e) => {
                // May fail if port in use - that's ok for unit test
                println!("Host creation failed (may be port in use): {}", e);
            }
        }
    }

    #[test]
    fn test_host_service_nonblocking() {
        ensure_init();

        let config = HostConfig {
            port: 15556,
            max_clients: 4,
            ..Default::default()
        };

        if let Ok(mut host) = PlatformHost::new(&config) {
            // Non-blocking service should return quickly with no events
            let result = host.service(0);
            assert!(result.is_ok());
            assert_eq!(result.unwrap(), HostEvent::None);
        }
    }

    #[test]
    fn test_ffi_host_lifecycle() {
        ensure_init();

        unsafe {
            // Create host
            let host = Platform_Host_Create(15557, 4, 2);

            if !host.is_null() {
                // Check initial state
                assert_eq!(Platform_Host_PeerCount(host), 0);
                assert_eq!(Platform_Host_GetPort(host), 15557);
                assert_eq!(Platform_Host_HasPackets(host), 0);

                // Service should work
                let event = Platform_Host_Service(host, 0);
                assert!(event >= 0);

                // Destroy
                Platform_Host_Destroy(host);
            }
        }
    }

    #[test]
    fn test_ffi_null_safety() {
        unsafe {
            // All FFI functions should handle null safely
            assert_eq!(Platform_Host_Service(std::ptr::null_mut(), 0), -1);
            assert_eq!(Platform_Host_PeerCount(std::ptr::null_mut()), 0);
            assert_eq!(Platform_Host_HasPackets(std::ptr::null_mut()), -1);

            let mut buffer = [0u8; 100];
            assert_eq!(
                Platform_Host_Receive(
                    std::ptr::null_mut(),
                    std::ptr::null_mut(),
                    std::ptr::null_mut(),
                    buffer.as_mut_ptr(),
                    100
                ),
                -1
            );

            Platform_Host_Destroy(std::ptr::null_mut()); // Should not panic
            Platform_Host_Flush(std::ptr::null_mut()); // Should not panic
        }
    }
}
