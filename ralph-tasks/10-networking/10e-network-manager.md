# Task 10e: Network Manager Unified API

## Dependencies
- Tasks 10a-10d must be complete (network init, host, client, packets)

## Context
The Network Manager provides a unified interface for both host (server) and client functionality. This simplifies game code by providing a single API for:
- Hosting a game session
- Joining an existing game
- Sending/receiving packets
- Querying connection state

The manager maintains global state to replace the original game's global networking variables.

## Objective
Create the NetworkManager with:
1. Global singleton manager
2. `host_game()` to create and host a game
3. `join_game()` to connect to an existing game
4. `update()` to service the network each frame
5. `send_data()` and `receive_data()` for packet exchange
6. `is_host()` and `is_connected()` for state queries
7. FFI exports for C++ game integration

## Deliverables
- [ ] `platform/src/network/manager.rs` - NetworkManager implementation
- [ ] Update `platform/src/network/mod.rs` to export manager
- [ ] FFI exports for manager functions
- [ ] Unit tests
- [ ] Verification passes

## Technical Background

### Why a Manager?
The original game had scattered networking code and global state. The manager:
- Centralizes networking state
- Provides a clean API for host/join
- Handles mode switching (none -> host or client)
- Simplifies the C++ integration

### Manager States
```
NetworkMode::None     - Not connected, no session
NetworkMode::Hosting  - Running as server, accepting connections
NetworkMode::Joined   - Connected to server as client
```

## Files to Create/Modify

### platform/src/network/manager.rs (NEW)
```rust
//! Network Manager - Unified networking API
//!
//! Provides a high-level interface for game networking that wraps
//! the lower-level host and client implementations.

use super::{NetworkError, NetworkResult};
use super::host::{PlatformHost, HostConfig, ReceivedPacket as HostPacket};
use super::client::{PlatformClient, ConnectConfig, ClientState};
use super::packet::{GamePacketHeader, GamePacketCode, GamePacket, serialize_packet};
use std::collections::VecDeque;
use std::sync::Mutex;
use once_cell::sync::Lazy;

/// Global network manager instance
static NETWORK_MANAGER: Lazy<Mutex<NetworkManager>> =
    Lazy::new(|| Mutex::new(NetworkManager::new()));

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
            NetworkMode::Joined => {
                self.client.as_ref()
                    .map(|c| c.is_connected())
                    .unwrap_or(false)
            }
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
    pub fn send_packet(&mut self, code: GamePacketCode, payload: &[u8], reliable: bool) -> NetworkResult<()> {
        let header = GamePacketHeader::new(code, self.next_sequence(), self.session.local_player_id);
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
            NetworkMode::Hosting => {
                self.host.as_ref().map(|h| h.peer_count()).unwrap_or(0)
            }
            NetworkMode::Joined => {
                if self.is_connected() { 1 } else { 0 }
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
// Global Manager Access
// =============================================================================

fn with_manager<F, R>(f: F) -> R
where
    F: FnOnce(&mut NetworkManager) -> R,
{
    let mut manager = NETWORK_MANAGER.lock().unwrap();
    f(&mut manager)
}

// =============================================================================
// FFI Exports
// =============================================================================

/// Initialize the network manager
///
/// Must be called after Platform_Network_Init()
/// Returns 0 on success
#[no_mangle]
pub extern "C" fn Platform_NetworkManager_Init() -> i32 {
    // Just ensure the lazy static is initialized
    let _ = NETWORK_MANAGER.lock();
    0
}

/// Shutdown the network manager
#[no_mangle]
pub extern "C" fn Platform_NetworkManager_Shutdown() {
    with_manager(|m| m.disconnect());
}

/// Get current network mode
///
/// Returns: 0=None, 1=Hosting, 2=Joined
#[no_mangle]
pub extern "C" fn Platform_NetworkManager_GetMode() -> i32 {
    with_manager(|m| m.mode() as i32)
}

/// Host a new game
///
/// Returns 0 on success, -1 on error
#[no_mangle]
pub unsafe extern "C" fn Platform_NetworkManager_HostGame(
    name: *const std::os::raw::c_char,
    port: u16,
    max_players: i32,
) -> i32 {
    if name.is_null() {
        return -1;
    }

    let name_str = match std::ffi::CStr::from_ptr(name).to_str() {
        Ok(s) => s,
        Err(_) => return -1,
    };

    with_manager(|m| {
        match m.host_game(name_str, port, max_players.max(1).min(8) as u8) {
            Ok(()) => 0,
            Err(_) => -1,
        }
    })
}

/// Join an existing game
///
/// Returns 0 on success, -1 on error
#[no_mangle]
pub unsafe extern "C" fn Platform_NetworkManager_JoinGame(
    address: *const std::os::raw::c_char,
    port: u16,
    timeout_ms: u32,
) -> i32 {
    if address.is_null() {
        return -1;
    }

    let addr_str = match std::ffi::CStr::from_ptr(address).to_str() {
        Ok(s) => s,
        Err(_) => return -1,
    };

    with_manager(|m| {
        match m.join_game(addr_str, port, timeout_ms) {
            Ok(()) => 0,
            Err(_) => -1,
        }
    })
}

/// Join a game asynchronously
#[no_mangle]
pub unsafe extern "C" fn Platform_NetworkManager_JoinGameAsync(
    address: *const std::os::raw::c_char,
    port: u16,
) -> i32 {
    if address.is_null() {
        return -1;
    }

    let addr_str = match std::ffi::CStr::from_ptr(address).to_str() {
        Ok(s) => s,
        Err(_) => return -1,
    };

    with_manager(|m| {
        match m.join_game_async(addr_str, port) {
            Ok(()) => 0,
            Err(_) => -1,
        }
    })
}

/// Disconnect from current session
#[no_mangle]
pub extern "C" fn Platform_NetworkManager_Disconnect() {
    with_manager(|m| m.disconnect());
}

/// Update network - call every frame
///
/// Returns number of packets received, or -1 on error
#[no_mangle]
pub extern "C" fn Platform_NetworkManager_Update() -> i32 {
    with_manager(|m| {
        match m.update() {
            Ok(count) => count as i32,
            Err(_) => -1,
        }
    })
}

/// Check if hosting
#[no_mangle]
pub extern "C" fn Platform_NetworkManager_IsHost() -> i32 {
    with_manager(|m| if m.is_host() { 1 } else { 0 })
}

/// Check if connected
#[no_mangle]
pub extern "C" fn Platform_NetworkManager_IsConnected() -> i32 {
    with_manager(|m| if m.is_connected() { 1 } else { 0 })
}

/// Get peer count
#[no_mangle]
pub extern "C" fn Platform_NetworkManager_PeerCount() -> i32 {
    with_manager(|m| m.peer_count() as i32)
}

/// Send data
///
/// Returns 0 on success, -1 on error
#[no_mangle]
pub unsafe extern "C" fn Platform_NetworkManager_SendData(
    channel: u8,
    data: *const u8,
    size: i32,
    reliable: i32,
) -> i32 {
    if data.is_null() || size <= 0 {
        return -1;
    }

    let slice = std::slice::from_raw_parts(data, size as usize);

    with_manager(|m| {
        match m.send_data(channel, slice, reliable != 0) {
            Ok(()) => 0,
            Err(_) => -1,
        }
    })
}

/// Receive data
///
/// Returns bytes copied, 0 if no data, -1 on error
#[no_mangle]
pub unsafe extern "C" fn Platform_NetworkManager_ReceiveData(
    peer_id: *mut u32,
    channel: *mut u8,
    buffer: *mut u8,
    buffer_size: i32,
) -> i32 {
    if buffer.is_null() || buffer_size <= 0 {
        return -1;
    }

    with_manager(|m| {
        match m.receive() {
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
    })
}

/// Check if packets are available
#[no_mangle]
pub extern "C" fn Platform_NetworkManager_HasPackets() -> i32 {
    with_manager(|m| if m.has_packets() { 1 } else { 0 })
}

/// Get local player ID
#[no_mangle]
pub extern "C" fn Platform_NetworkManager_GetLocalPlayerId() -> i32 {
    with_manager(|m| m.session_info().local_player_id as i32)
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

    fn reset_manager() {
        with_manager(|m| {
            m.disconnect();
        });
    }

    #[test]
    fn test_manager_initial_state() {
        ensure_init();
        reset_manager();

        with_manager(|m| {
            assert_eq!(m.mode(), NetworkMode::None);
            assert!(!m.is_host());
            assert!(!m.is_connected());
            assert_eq!(m.peer_count(), 0);
        });
    }

    #[test]
    fn test_host_game() {
        ensure_init();
        reset_manager();

        with_manager(|m| {
            let result = m.host_game("Test Game", 16000, 4);

            match result {
                Ok(()) => {
                    assert_eq!(m.mode(), NetworkMode::Hosting);
                    assert!(m.is_host());
                    assert!(m.is_connected());
                    assert_eq!(m.session_info().get_name(), "Test Game");
                    assert_eq!(m.session_info().port, 16000);
                    m.disconnect();
                }
                Err(e) => {
                    // Port might be in use
                    println!("Host failed (port in use?): {}", e);
                }
            }
        });
    }

    #[test]
    fn test_disconnect() {
        ensure_init();
        reset_manager();

        with_manager(|m| {
            // Disconnect when not connected should be fine
            m.disconnect();
            assert_eq!(m.mode(), NetworkMode::None);
        });
    }

    #[test]
    fn test_ffi_manager() {
        ensure_init();
        Platform_NetworkManager_Shutdown();

        assert_eq!(Platform_NetworkManager_Init(), 0);
        assert_eq!(Platform_NetworkManager_GetMode(), 0); // None
        assert_eq!(Platform_NetworkManager_IsHost(), 0);
        assert_eq!(Platform_NetworkManager_IsConnected(), 0);
        assert_eq!(Platform_NetworkManager_PeerCount(), 0);

        Platform_NetworkManager_Disconnect(); // Should not panic

        Platform_NetworkManager_Shutdown();
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
```

### platform/src/network/mod.rs (MODIFY)
Add after existing exports:
```rust
pub mod manager;

pub use manager::{NetworkManager, NetworkMode, NetworkPacket, GameSessionInfo};
```

## Verification Command
```bash
ralph-tasks/verify/check-network-manager.sh
```

## Verification Script
Create `ralph-tasks/verify/check-network-manager.sh`:
```bash
#!/bin/bash
# Verification script for network manager (Task 10e)

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Network Manager ==="

cd "$ROOT_DIR"

# Step 1: Check manager.rs exists
echo "Step 1: Checking manager.rs exists..."
if [ ! -f "platform/src/network/manager.rs" ]; then
    echo "VERIFY_FAILED: platform/src/network/manager.rs not found"
    exit 1
fi
echo "  manager.rs exists"

# Step 2: Check mod.rs exports manager
echo "Step 2: Checking mod.rs exports manager..."
if ! grep -q 'pub mod manager' platform/src/network/mod.rs; then
    echo "VERIFY_FAILED: manager module not exported in mod.rs"
    exit 1
fi
echo "  manager module exported"

# Step 3: Build
echo "Step 3: Building platform crate..."
if ! cargo build --manifest-path platform/Cargo.toml --release 2>&1; then
    echo "VERIFY_FAILED: cargo build failed"
    exit 1
fi
echo "  Build successful"

# Step 4: Run manager tests
echo "Step 4: Running manager tests..."
if ! cargo test --manifest-path platform/Cargo.toml --release -- manager --test-threads=1 2>&1; then
    echo "VERIFY_FAILED: manager tests failed"
    exit 1
fi
echo "  Tests passed"

# Step 5: Check FFI exports
echo "Step 5: Checking FFI exports in header..."
cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1

REQUIRED_EXPORTS=(
    "Platform_NetworkManager_Init"
    "Platform_NetworkManager_Shutdown"
    "Platform_NetworkManager_HostGame"
    "Platform_NetworkManager_JoinGame"
    "Platform_NetworkManager_Disconnect"
    "Platform_NetworkManager_Update"
    "Platform_NetworkManager_IsHost"
    "Platform_NetworkManager_IsConnected"
    "Platform_NetworkManager_SendData"
    "Platform_NetworkManager_ReceiveData"
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
1. Create `platform/src/network/manager.rs`
2. Update `platform/src/network/mod.rs` to export manager
3. Build and run tests
4. Verify FFI exports
5. Run verification script

## Success Criteria
- Manager can host and disconnect
- FFI exports work correctly
- State transitions are correct
- All unit tests pass

## Common Issues

### "already borrowed" panic
The global mutex should prevent this. Ensure all FFI functions use `with_manager()`.

### Host fails due to port
Tests use high port numbers (16000+) to avoid conflicts.

### Connection timeout on join
Expected when no server - tests handle this case.

## Completion Promise
When verification passes, output:
```
<promise>TASK_10E_COMPLETE</promise>
```

## Escape Hatch
If stuck after 12 iterations:
- Document blocking issues in `ralph-tasks/blocked/10e.md`
- Output: `<promise>TASK_10E_BLOCKED</promise>`

## Max Iterations
12
