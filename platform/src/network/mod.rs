//! Networking module using enet for reliable UDP
//!
//! Replaces the obsolete IPX/SPX and Windows Winsock implementations.
//!
//! The original game used:
//! - IPX (Internetwork Packet Exchange): Obsolete LAN protocol
//! - Winsock: Windows socket API with WSAAsyncSelect
//!
//! This module provides:
//! - Modern UDP-based networking via enet
//! - Cross-platform support (macOS, Linux, Windows)
//! - Reliable and unreliable packet delivery
//! - Connection management

pub mod client;
pub mod host;
pub mod manager;
pub mod packet;

pub use client::{ClientEvent, ClientState, ConnectConfig, PlatformClient};
pub use host::{HostConfig, HostEvent, PlatformHost, ReceivedPacket};
pub use manager::{GameSessionInfo, NetworkManager, NetworkMode, NetworkPacket};
pub use packet::{
    create_chat_packet, create_ping_packet, create_pong_packet, deserialize_packet,
    serialize_packet, DeliveryMode, GamePacket, GamePacketCode, GamePacketHeader, PACKET_MAGIC,
};

// Re-export host FFI functions for cbindgen
pub use host::{
    Platform_Host_Broadcast,
    Platform_Host_Create,
    Platform_Host_Destroy,
    Platform_Host_Flush,
    Platform_Host_GetPort,
    Platform_Host_HasPackets,
    Platform_Host_PeerCount,
    Platform_Host_Receive,
    Platform_Host_Service,
};

// Re-export client FFI functions for cbindgen
pub use client::{
    Platform_Client_Connect,
    Platform_Client_ConnectBlocking,
    Platform_Client_Create,
    Platform_Client_Destroy,
    Platform_Client_Disconnect,
    Platform_Client_Flush,
    Platform_Client_GetState,
    Platform_Client_HasPackets,
    Platform_Client_IsConnected,
    Platform_Client_Receive,
    Platform_Client_Send,
    Platform_Client_Service,
};

// Re-export packet FFI functions for cbindgen
pub use packet::{
    Platform_Packet_Build,
    Platform_Packet_CreateHeader,
    Platform_Packet_GetPayload,
    Platform_Packet_HeaderSize,
    Platform_Packet_ParseHeader,
    Platform_Packet_ValidateHeader,
};

// Re-export manager FFI functions for cbindgen
pub use manager::{
    Platform_NetworkManager_Create,
    Platform_NetworkManager_Destroy,
    Platform_NetworkManager_Disconnect,
    Platform_NetworkManager_GetLocalPlayerId,
    Platform_NetworkManager_GetMode,
    Platform_NetworkManager_HasPackets,
    Platform_NetworkManager_HostGame,
    Platform_NetworkManager_Init,
    Platform_NetworkManager_IsConnected,
    Platform_NetworkManager_IsHost,
    Platform_NetworkManager_JoinGame,
    Platform_NetworkManager_JoinGameAsync,
    Platform_NetworkManager_PeerCount,
    Platform_NetworkManager_ReceiveData,
    Platform_NetworkManager_SendData,
    Platform_NetworkManager_Shutdown,
    Platform_NetworkManager_Update,
};

use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;
use once_cell::sync::OnceCell;
use thiserror::Error;

/// Global initialization state
static INITIALIZED: AtomicBool = AtomicBool::new(false);

/// enet library handle (ensures enet stays initialized)
/// We store the Enet instance to keep it alive
static ENET_HANDLE: OnceCell<Arc<enet::Enet>> = OnceCell::new();

// =============================================================================
// Error Types
// =============================================================================

/// Network subsystem errors
#[derive(Error, Debug, Clone, PartialEq, Eq)]
pub enum NetworkError {
    #[error("Network subsystem not initialized")]
    NotInitialized,

    #[error("Network subsystem already initialized")]
    AlreadyInitialized,

    #[error("enet library initialization failed")]
    EnetInitFailed,

    #[error("Failed to create network host: {0}")]
    HostCreationFailed(String),

    #[error("Failed to connect: {0}")]
    ConnectionFailed(String),

    #[error("Send operation failed")]
    SendFailed,

    #[error("Invalid network address: {0}")]
    InvalidAddress(String),

    #[error("Connection timed out")]
    Timeout,

    #[error("Peer disconnected")]
    Disconnected,

    #[error("Invalid peer ID: {0}")]
    InvalidPeer(usize),

    #[error("Buffer too small: needed {needed}, got {got}")]
    BufferTooSmall { needed: usize, got: usize },

    #[error("Maximum connections reached")]
    MaxConnectionsReached,
}

/// Result type for network operations
pub type NetworkResult<T> = Result<T, NetworkError>;

// =============================================================================
// Initialization
// =============================================================================

/// Initialize the networking subsystem
///
/// Must be called before any other network operations.
/// Safe to call multiple times (subsequent calls return AlreadyInitialized).
///
/// # Returns
/// - `Ok(())` on successful initialization
/// - `Err(NetworkError::AlreadyInitialized)` if already initialized
/// - `Err(NetworkError::EnetInitFailed)` if enet fails to initialize
pub fn init() -> NetworkResult<()> {
    // Check if already initialized
    if INITIALIZED.load(Ordering::SeqCst) {
        return Err(NetworkError::AlreadyInitialized);
    }

    // Initialize enet library
    ENET_HANDLE.get_or_try_init(|| {
        enet::Enet::new()
            .map(Arc::new)
            .map_err(|_| NetworkError::EnetInitFailed)
    })?;

    INITIALIZED.store(true, Ordering::SeqCst);
    log::info!("Network subsystem initialized (enet)");
    Ok(())
}

/// Shutdown the networking subsystem
///
/// Safe to call multiple times or without prior initialization.
/// After shutdown, `init()` can be called again.
pub fn shutdown() {
    if INITIALIZED.swap(false, Ordering::SeqCst) {
        // enet cleanup happens automatically when Enet handle is dropped
        // but since we use OnceCell, it persists - this is fine
        log::info!("Network subsystem shutdown");
    }
}

/// Check if networking is initialized
pub fn is_initialized() -> bool {
    INITIALIZED.load(Ordering::SeqCst)
}

/// Get a reference to verify enet is available
///
/// This is used internally to ensure enet is initialized before operations.
pub(crate) fn require_initialized() -> NetworkResult<()> {
    if is_initialized() {
        Ok(())
    } else {
        Err(NetworkError::NotInitialized)
    }
}

// =============================================================================
// FFI Exports
// =============================================================================

/// Initialize the network subsystem
///
/// # Returns
/// - 0 on success
/// - -1 if already initialized
/// - -2 if enet initialization failed
#[no_mangle]
pub extern "C" fn Platform_Network_Init() -> i32 {
    match init() {
        Ok(()) => 0,
        Err(NetworkError::AlreadyInitialized) => -1,
        Err(NetworkError::EnetInitFailed) => -2,
        Err(_) => -3,
    }
}

/// Shutdown the network subsystem
#[no_mangle]
pub extern "C" fn Platform_Network_Shutdown() {
    shutdown();
}

/// Check if network is initialized
///
/// # Returns
/// - 1 if initialized
/// - 0 if not initialized
#[no_mangle]
pub extern "C" fn Platform_Network_IsInitialized() -> i32 {
    if is_initialized() { 1 } else { 0 }
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
#[path = "tests_integration.rs"]
mod tests_integration;

#[cfg(test)]
mod tests {
    use super::*;

    // Note: Tests must be run with --test-threads=1 due to global state
    // cargo test --manifest-path platform/Cargo.toml network -- --test-threads=1

    fn reset_state() {
        // Reset the initialized flag for testing
        INITIALIZED.store(false, Ordering::SeqCst);
    }

    #[test]
    fn test_init_shutdown_cycle() {
        reset_state();

        // Should initialize successfully
        assert!(!is_initialized());
        let result = init();
        // First init should succeed (or be already init from other test)
        assert!(result.is_ok() || result == Err(NetworkError::AlreadyInitialized));

        if result.is_ok() {
            assert!(is_initialized());

            // Shutdown should work
            shutdown();
            assert!(!is_initialized());
        }
    }

    #[test]
    fn test_double_init() {
        reset_state();

        // First init
        let _ = init();

        // Second init should fail if first succeeded
        if is_initialized() {
            assert_eq!(init(), Err(NetworkError::AlreadyInitialized));
        }

        shutdown();
    }

    #[test]
    fn test_shutdown_without_init() {
        reset_state();

        // Should not panic
        shutdown();
        shutdown();
        shutdown();
    }

    #[test]
    fn test_ffi_init_shutdown() {
        reset_state();

        assert_eq!(Platform_Network_IsInitialized(), 0);

        let result = Platform_Network_Init();
        // Either success (0) or already init (-1)
        assert!(result == 0 || result == -1);

        if result == 0 {
            assert_eq!(Platform_Network_IsInitialized(), 1);
            Platform_Network_Shutdown();
            assert_eq!(Platform_Network_IsInitialized(), 0);
        }
    }

    #[test]
    fn test_require_initialized() {
        reset_state();

        assert_eq!(require_initialized(), Err(NetworkError::NotInitialized));

        if init().is_ok() {
            assert!(require_initialized().is_ok());
            shutdown();
        }
    }
}
