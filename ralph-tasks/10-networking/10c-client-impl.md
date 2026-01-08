# Task 10c: Client Implementation

## Dependencies
- Task 10a must be complete (network initialization)
- Task 10b should be complete (host implementation for testing)

## Context
This task implements the network client functionality. In multiplayer, most players connect as clients to a host's game session. The client initiates a connection to a known address and port, then exchanges packets with the server.

**Connection Flow**:
1. Client creates a local host (without binding to a specific port)
2. Client initiates connection to server address:port
3. Server accepts the connection
4. Both sides can now exchange packets

## Objective
Create the client implementation with:
1. `ConnectConfig` structure for connection parameters
2. `PlatformClient` struct that wraps client-mode enet::Host
3. `connect()` method to initiate connection
4. `service()` method to process events
5. `send()` method to send to server
6. `receive()` method to get packets from server
7. FFI exports for C++ usage

## Deliverables
- [ ] `platform/src/network/client.rs` - Client implementation
- [ ] Update `platform/src/network/mod.rs` to export client module
- [ ] FFI exports for client creation, connection, send, receive
- [ ] Unit tests for client creation
- [ ] Verification passes

## Technical Background

### Client vs Host in enet
- **Host (server)**: `Host::new(address, ...)` - binds to specific port
- **Client**: `Host::create_client(...)` - doesn't bind, only connects out

### Connection Lifecycle
```
1. Create client host (no binding)
2. Initiate connection: host.connect(server_address, channels, user_data)
3. Service to receive Event::Connect
4. Exchange packets
5. Service to receive Event::Disconnect or call peer.disconnect()
```

### Async Connection
Connection is asynchronous - `connect()` returns immediately and you must poll `service()` to get the Connect event. We provide both:
- `connect_blocking()` - waits for connection
- `connect_async()` - returns immediately, check `is_connected()`

## Files to Create/Modify

### platform/src/network/client.rs (NEW)
```rust
//! Network client implementation
//!
//! The client connects to a host (server) to join a multiplayer game:
//! - Initiates connection to server address
//! - Sends packets to server
//! - Receives packets from server

use super::{NetworkError, NetworkResult};
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
    host: enet::Host<()>,
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

        // Create client-mode host (no specific binding)
        let host = enet::Host::<()>::create_client(
            1,  // Max peers (just the server)
            config.channel_count,
            config.incoming_bandwidth,
            config.outgoing_bandwidth,
        ).map_err(|e| NetworkError::HostCreationFailed(format!("Client: {:?}", e)))?;

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
        let ip: Ipv4Addr = address.parse()
            .map_err(|_| NetworkError::InvalidAddress(address.to_string()))?;

        let enet_addr = enet::Address::new(ip, port);

        // Initiate connection
        self.host.connect(&enet_addr, channel_count, 0)
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
                        "Connection rejected or failed".to_string()
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
        let timeout = Duration::from_millis(timeout_ms as u64);
        self.last_event = ClientEvent::None;

        loop {
            match self.host.service(timeout) {
                Ok(Some(event)) => {
                    match event {
                        enet::Event::Connect(_peer) => {
                            self.state = ClientState::Connected;
                            self.last_event = ClientEvent::Connected;
                            log::info!("Connected to server at {}:{}",
                                self.server_address, self.server_port);
                        }

                        enet::Event::Disconnect(_peer, _data) => {
                            self.state = ClientState::Disconnected;
                            self.last_event = ClientEvent::Disconnected;
                            log::info!("Disconnected from server");
                        }

                        enet::Event::Receive { channel_id, packet, .. } => {
                            self.received_packets.push_back(packet.data().to_vec());
                            self.last_channel = channel_id;
                            self.last_event = ClientEvent::PacketReceived;
                        }
                    }
                    continue;
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

        let packet = if reliable {
            enet::Packet::reliable(data)
        } else {
            enet::Packet::unreliable(data)
        };

        // Get the server peer and send
        // In client mode, there's only one peer (the server)
        for mut peer in self.host.peers() {
            peer.send(channel, packet.clone())
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
        if self.state == ClientState::Connected {
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
    if (*client).is_connected() { 1 } else { 0 }
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
    if (*client).has_packets() { 1 } else { 0 }
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
            assert_eq!(Platform_Client_Connect(std::ptr::null_mut(), addr, 5555), -1);

            Platform_Client_Disconnect(std::ptr::null_mut());  // Should not panic
            Platform_Client_Flush(std::ptr::null_mut());       // Should not panic
            Platform_Client_Destroy(std::ptr::null_mut());     // Should not panic
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
```

### platform/src/network/mod.rs (MODIFY)
Add after existing exports:
```rust
pub mod client;

pub use client::{ConnectConfig, PlatformClient, ClientState, ClientEvent};
```

## Verification Command
```bash
ralph-tasks/verify/check-client-impl.sh
```

## Verification Script
Create `ralph-tasks/verify/check-client-impl.sh`:
```bash
#!/bin/bash
# Verification script for client implementation (Task 10c)

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Client Implementation ==="

cd "$ROOT_DIR"

# Step 1: Check client.rs exists
echo "Step 1: Checking client.rs exists..."
if [ ! -f "platform/src/network/client.rs" ]; then
    echo "VERIFY_FAILED: platform/src/network/client.rs not found"
    exit 1
fi
echo "  client.rs exists"

# Step 2: Check mod.rs exports client
echo "Step 2: Checking mod.rs exports client..."
if ! grep -q 'pub mod client' platform/src/network/mod.rs; then
    echo "VERIFY_FAILED: client module not exported in mod.rs"
    exit 1
fi
echo "  client module exported"

# Step 3: Build
echo "Step 3: Building platform crate..."
if ! cargo build --manifest-path platform/Cargo.toml --release 2>&1; then
    echo "VERIFY_FAILED: cargo build failed"
    exit 1
fi
echo "  Build successful"

# Step 4: Run client tests
echo "Step 4: Running client tests..."
if ! cargo test --manifest-path platform/Cargo.toml --release -- client --test-threads=1 2>&1; then
    echo "VERIFY_FAILED: client tests failed"
    exit 1
fi
echo "  Tests passed"

# Step 5: Generate and check header
echo "Step 5: Checking FFI exports in header..."
cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1

REQUIRED_EXPORTS=(
    "Platform_Client_Create"
    "Platform_Client_Destroy"
    "Platform_Client_Connect"
    "Platform_Client_Service"
    "Platform_Client_IsConnected"
    "Platform_Client_Send"
    "Platform_Client_Receive"
    "Platform_Client_Disconnect"
)

for func in "${REQUIRED_EXPORTS[@]}"; do
    if ! grep -q "$func" include/platform.h; then
        echo "VERIFY_FAILED: $func not found in platform.h"
        exit 1
    fi
    echo "  Found $func in header"
done

echo ""
echo "=== All checks passed ==="
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Create `platform/src/network/client.rs`
2. Update `platform/src/network/mod.rs` to export client module
3. Build and test
4. Verify FFI exports
5. Run verification script

## Success Criteria
- Client can be created
- Connection timeout works correctly
- FFI exports are null-safe
- All unit tests pass

## Common Issues

### Connection always times out
- This is expected when no server is running
- Tests are designed to handle this case

### "peer iterator empty"
- enet's peer iterator may behave unexpectedly
- Check that client host is in connected state before sending

### Send returns Disconnected
- Must be in Connected state to send
- Call service() after connect_async() to complete connection

## Completion Promise
When verification passes, output:
```
<promise>TASK_10C_COMPLETE</promise>
```

## Escape Hatch
If stuck after 15 iterations:
- Document blocking issues in `ralph-tasks/blocked/10c.md`
- Output: `<promise>TASK_10C_BLOCKED</promise>`

## Max Iterations
15
