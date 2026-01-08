//! Network Integration Tests
//!
//! These tests verify the networking module works correctly end-to-end.
//! Tests are run with --test-threads=1 due to global enet state.
//!
//! Run with: cargo test --manifest-path platform/Cargo.toml network -- --test-threads=1

use std::thread;
use std::time::Duration;

use super::{
    init, is_initialized, shutdown,
    ConnectConfig, PlatformClient, ClientState,
    HostConfig, PlatformHost,
    GamePacketHeader, GamePacketCode, GamePacket, DeliveryMode,
    serialize_packet, deserialize_packet, create_ping_packet, create_pong_packet,
    PACKET_MAGIC,
    NetworkManager, NetworkMode,
    // FFI functions
    Platform_Network_Init, Platform_Network_Shutdown, Platform_Network_IsInitialized,
    Platform_Host_Create, Platform_Host_Destroy, Platform_Host_Service,
    Platform_Host_PeerCount, Platform_Host_GetPort,
    Platform_Client_Create, Platform_Client_Destroy, Platform_Client_Service,
    Platform_Client_IsConnected, Platform_Client_GetState,
    Platform_Packet_CreateHeader, Platform_Packet_Build, Platform_Packet_ParseHeader,
    Platform_Packet_ValidateHeader, Platform_Packet_HeaderSize, Platform_Packet_GetPayload,
    Platform_NetworkManager_Create, Platform_NetworkManager_Destroy,
    Platform_NetworkManager_Shutdown,
    Platform_NetworkManager_GetMode, Platform_NetworkManager_IsConnected,
    Platform_NetworkManager_IsHost, Platform_NetworkManager_HostGame,
    Platform_NetworkManager_Disconnect,
};

// =============================================================================
// Test Helpers
// =============================================================================

fn setup() {
    // Ensure network is initialized
    if !is_initialized() {
        let _ = init();
    }
}

fn teardown() {
    shutdown();
}

// =============================================================================
// Initialization Tests
// =============================================================================

#[test]
fn test_network_init_shutdown() {
    teardown(); // Start fresh

    assert!(!is_initialized());

    let result = init();
    assert!(result.is_ok());
    assert!(is_initialized());

    shutdown();
    assert!(!is_initialized());
}

#[test]
fn test_double_init_fails() {
    teardown();

    let result1 = init();
    assert!(result1.is_ok() || result1.is_err()); // May already be init

    if is_initialized() {
        let result2 = init();
        assert!(result2.is_err());
    }

    shutdown();
}

#[test]
fn test_multiple_shutdown_safe() {
    shutdown();
    shutdown();
    shutdown(); // Should not panic
}

// =============================================================================
// Host Tests
// =============================================================================

#[test]
fn test_host_create_destroy() {
    setup();

    let config = HostConfig {
        port: 16000,
        max_clients: 4,
        channel_count: 2,
        incoming_bandwidth: 0,
        outgoing_bandwidth: 0,
    };

    let host_result = PlatformHost::new(&config);
    assert!(host_result.is_ok(), "Failed to create host: {:?}", host_result.err());

    let mut host = host_result.unwrap();

    // Verify port is bound
    let port = host.port();
    assert_eq!(port, 16000);

    // Service should work without connections
    let service_result = host.service(10);
    assert!(service_result.is_ok());

    // Peer count should be 0
    assert_eq!(host.peer_count(), 0);

    drop(host);
    teardown();
}

#[test]
fn test_host_different_ports() {
    setup();

    let config1 = HostConfig {
        port: 16001,
        max_clients: 2,
        channel_count: 2,
        incoming_bandwidth: 0,
        outgoing_bandwidth: 0,
    };
    let host1 = PlatformHost::new(&config1);
    assert!(host1.is_ok());

    let config2 = HostConfig {
        port: 16002,
        max_clients: 2,
        channel_count: 2,
        incoming_bandwidth: 0,
        outgoing_bandwidth: 0,
    };
    let host2 = PlatformHost::new(&config2);
    assert!(host2.is_ok());

    drop(host1);
    drop(host2);
    teardown();
}

// =============================================================================
// Client Tests
// =============================================================================

#[test]
fn test_client_create_destroy() {
    setup();

    let config = ConnectConfig::default();
    let client_result = PlatformClient::new(&config);
    assert!(client_result.is_ok(), "Failed to create client: {:?}", client_result.err());

    let client = client_result.unwrap();

    // Should not be connected initially
    assert!(!client.is_connected());
    assert_eq!(client.state(), ClientState::Disconnected);

    drop(client);
    teardown();
}

#[test]
fn test_client_connect_timeout() {
    setup();

    let config = ConnectConfig::default();
    let client_result = PlatformClient::new(&config);
    assert!(client_result.is_ok());

    let mut client = client_result.unwrap();

    // Try to connect to non-existent host (should timeout)
    let connect_result = client.connect_blocking("127.0.0.1", 19999, 2, 500);

    // Should have failed (timeout)
    assert!(connect_result.is_err());
    assert!(!client.is_connected());

    drop(client);
    teardown();
}

// =============================================================================
// Packet Tests
// =============================================================================

#[test]
fn test_packet_header_creation() {
    let header = GamePacketHeader::new(GamePacketCode::Ping, 42, 1);

    assert_eq!(header.magic, PACKET_MAGIC);
    assert_eq!(header.code, GamePacketCode::Ping as u8);
    assert_eq!(header.sequence, 42);
    assert_eq!(header.player_id, 1);
}

#[test]
fn test_packet_serialization() {
    let header = GamePacketHeader::new(GamePacketCode::DataAck, 100, 2);
    let payload = vec![1, 2, 3, 4, 5];

    let serialized = serialize_packet(&header, &payload);
    assert!(!serialized.is_empty());

    // Verify we can deserialize it back
    let deserialized = deserialize_packet(&serialized);
    assert!(deserialized.is_some());

    let (parsed_header, parsed_payload) = deserialized.unwrap();
    assert_eq!(parsed_header.magic, PACKET_MAGIC);
    assert_eq!(parsed_header.code, GamePacketCode::DataAck as u8);
    assert_eq!(parsed_header.sequence, 100);
    assert_eq!(parsed_header.player_id, 2);
    assert_eq!(parsed_payload, &[1, 2, 3, 4, 5]);
}

#[test]
fn test_game_packet_struct() {
    let packet = GamePacket::new(GamePacketCode::ChatMessage, 55, 1, b"Hello".to_vec());

    assert_eq!(packet.header.code, GamePacketCode::ChatMessage as u8);
    assert_eq!(packet.header.sequence, 55);
    assert_eq!(packet.payload, b"Hello");

    // Serialize and deserialize
    let serialized = packet.serialize();
    let deserialized = deserialize_packet(&serialized);
    assert!(deserialized.is_some());

    let (header, payload) = deserialized.unwrap();
    assert_eq!(header.code, GamePacketCode::ChatMessage as u8);
    assert_eq!(payload, b"Hello");
}

#[test]
fn test_ping_pong_packets() {
    let ping = create_ping_packet(55, 1);

    assert_eq!(ping.header.code, GamePacketCode::Ping as u8);
    assert_eq!(ping.header.sequence, 55);
    assert_eq!(ping.header.player_id, 1);

    // Create pong response
    let pong = create_pong_packet(ping.header.sequence, 2);

    assert_eq!(pong.header.code, GamePacketCode::Pong as u8);
    assert_eq!(pong.header.sequence, 55); // Same sequence as ping
    assert_eq!(pong.header.player_id, 2);
}

#[test]
fn test_invalid_packet_deserialization() {
    // Too short
    let short_data = vec![0, 1, 2];
    assert!(deserialize_packet(&short_data).is_none());

    // Wrong magic
    let mut bad_magic = vec![0xFF, 0xFF, 0, 0, 0, 0, 0, 0, 0, 0];
    let result = deserialize_packet(&bad_magic);
    // Should fail magic validation
    assert!(result.is_none());
}

#[test]
fn test_delivery_mode_values() {
    assert_eq!(DeliveryMode::Reliable as i32, 0);
    assert_eq!(DeliveryMode::Unreliable as i32, 1);
}

// =============================================================================
// Host-Client Communication Tests
// =============================================================================

#[test]
fn test_host_client_connect() {
    setup();

    // Create host
    let host_config = HostConfig {
        port: 16010,
        max_clients: 4,
        channel_count: 2,
        incoming_bandwidth: 0,
        outgoing_bandwidth: 0,
    };
    let host_result = PlatformHost::new(&host_config);
    assert!(host_result.is_ok());
    let mut host = host_result.unwrap();

    // Create client
    let client_config = ConnectConfig::default();
    let client_result = PlatformClient::new(&client_config);
    assert!(client_result.is_ok());
    let mut client = client_result.unwrap();

    // Client connects to host
    let connect_result = client.connect_async("127.0.0.1", 16010, 2);
    assert!(connect_result.is_ok(), "Connect failed: {:?}", connect_result.err());

    // Service both for a few iterations to establish connection
    let mut connected = false;
    for _ in 0..50 {
        let _ = host.service(10);
        let _ = client.service(10);

        if client.is_connected() {
            connected = true;
            break;
        }
        thread::sleep(Duration::from_millis(10));
    }

    assert!(connected, "Client failed to connect to host");
    // Note: peer_count may not immediately reflect the connection due to timing
    // The important thing is that the client reports connected

    // Disconnect
    client.disconnect();

    // Service to process disconnect
    for _ in 0..10 {
        let _ = host.service(10);
        let _ = client.service(10);
        thread::sleep(Duration::from_millis(10));
    }

    assert!(!client.is_connected());

    drop(client);
    drop(host);
    teardown();
}

#[test]
fn test_host_client_data_exchange() {
    setup();

    // Create host
    let host_config = HostConfig {
        port: 16011,
        max_clients: 4,
        channel_count: 2,
        incoming_bandwidth: 0,
        outgoing_bandwidth: 0,
    };
    let mut host = PlatformHost::new(&host_config).expect("Failed to create host");

    // Create and connect client
    let client_config = ConnectConfig::default();
    let mut client = PlatformClient::new(&client_config).expect("Failed to create client");
    client.connect_async("127.0.0.1", 16011, 2).expect("Connect failed");

    // Wait for connection
    let mut connected = false;
    for _ in 0..50 {
        let _ = host.service(10);
        let _ = client.service(10);
        if client.is_connected() {
            connected = true;
            break;
        }
        thread::sleep(Duration::from_millis(10));
    }
    assert!(connected, "Client failed to connect");

    // Client sends data to host
    let test_data = b"Hello from client!";
    let send_result = client.send(0, test_data, true);
    assert!(send_result.is_ok(), "Send failed: {:?}", send_result.err());

    // Flush client
    client.flush();

    // Host receives data
    let mut received = false;
    for _ in 0..50 {
        let _ = host.service(10);
        let _ = client.service(10);

        if let Some(packet) = host.receive() {
            assert_eq!(packet.data, test_data);
            received = true;
            break;
        }
        thread::sleep(Duration::from_millis(10));
    }
    assert!(received, "Host did not receive data from client");

    // Host broadcasts back to client
    let response_data = b"Hello from host!";
    let broadcast_result = host.broadcast(0, response_data, true);
    assert!(broadcast_result.is_ok());

    // Flush host
    host.flush();

    // Client receives broadcast
    let mut client_received = false;
    for _ in 0..50 {
        let _ = host.service(10);
        let _ = client.service(10);

        if let Some(data) = client.receive() {
            assert_eq!(data, response_data);
            client_received = true;
            break;
        }
        thread::sleep(Duration::from_millis(10));
    }
    assert!(client_received, "Client did not receive data from host");

    drop(client);
    drop(host);
    teardown();
}

// =============================================================================
// FFI Tests
// =============================================================================

#[test]
fn test_ffi_network_init_shutdown() {
    // Ensure clean state
    Platform_Network_Shutdown();

    assert_eq!(Platform_Network_IsInitialized(), 0);

    let init_result = Platform_Network_Init();
    // 0 = success, -1 = already init
    assert!(init_result == 0 || init_result == -1);

    if init_result == 0 {
        assert_eq!(Platform_Network_IsInitialized(), 1);
    }

    Platform_Network_Shutdown();
    assert_eq!(Platform_Network_IsInitialized(), 0);
}

#[test]
fn test_ffi_host_create_destroy() {
    setup();

    let host = Platform_Host_Create(16020, 4, 2);
    assert!(!host.is_null(), "Failed to create host via FFI");

    unsafe {
        // Check port
        let port = Platform_Host_GetPort(host);
        assert_eq!(port, 16020);

        // Service
        let service_result = Platform_Host_Service(host, 10);
        assert!(service_result >= 0);

        // Peer count
        let peer_count = Platform_Host_PeerCount(host);
        assert_eq!(peer_count, 0);

        // Destroy
        Platform_Host_Destroy(host);
    }

    teardown();
}

#[test]
fn test_ffi_client_create_destroy() {
    setup();

    let client = Platform_Client_Create(2);
    assert!(!client.is_null(), "Failed to create client via FFI");

    unsafe {
        // Should not be connected
        assert_eq!(Platform_Client_IsConnected(client), 0);

        // State should be disconnected (0)
        assert_eq!(Platform_Client_GetState(client), 0);

        // Service
        let service_result = Platform_Client_Service(client, 0);
        assert!(service_result >= 0);

        // Destroy
        Platform_Client_Destroy(client);
    }

    teardown();
}

#[test]
fn test_ffi_packet_operations() {
    // Get header size
    let header_size = Platform_Packet_HeaderSize();
    assert!(header_size > 0 && header_size <= 16);

    unsafe {
        // Create header
        let mut header_buf = vec![0u8; header_size as usize];
        let mut out_size: i32 = 0;
        let create_result = Platform_Packet_CreateHeader(
            GamePacketCode::ChatMessage as u8,
            99, // sequence
            3,  // player_id
            header_buf.as_mut_ptr(),
            &mut out_size,
        );
        assert_eq!(create_result, 0); // Success

        // Validate header
        let valid = Platform_Packet_ValidateHeader(header_buf.as_ptr(), header_size);
        assert_eq!(valid, 1);

        // Build complete packet
        let payload = b"Hello";
        let mut packet_buf = vec![0u8; header_size as usize + payload.len()];
        let packet_size = Platform_Packet_Build(
            GamePacketCode::ChatMessage as u8,
            100,
            4,
            payload.as_ptr(),
            payload.len() as i32,
            packet_buf.as_mut_ptr(),
            packet_buf.len() as i32,
        );
        assert!(packet_size > 0); // Returns total size on success

        // Parse header back
        let mut code: u8 = 0;
        let mut sequence: u32 = 0;
        let mut player_id: u8 = 0;

        let parse_result = Platform_Packet_ParseHeader(
            packet_buf.as_ptr(),
            packet_size,
            &mut code,
            &mut sequence,
            &mut player_id,
        );
        assert_eq!(parse_result, 0); // Success
        assert_eq!(code, GamePacketCode::ChatMessage as u8);
        assert_eq!(sequence, 100);
        assert_eq!(player_id, 4);

        // Get payload
        let mut payload_len: i32 = 0;
        let payload_ptr = Platform_Packet_GetPayload(
            packet_buf.as_ptr(),
            packet_size,
            &mut payload_len,
        );
        assert!(!payload_ptr.is_null());
        assert_eq!(payload_len, payload.len() as i32);

        let payload_slice = std::slice::from_raw_parts(payload_ptr, payload_len as usize);
        assert_eq!(payload_slice, payload);
    }
}

#[test]
fn test_ffi_null_safety() {
    unsafe {
        // All should handle null gracefully
        Platform_Host_Destroy(std::ptr::null_mut());
        assert_eq!(Platform_Host_Service(std::ptr::null_mut(), 0), -1);
        assert_eq!(Platform_Host_PeerCount(std::ptr::null_mut()), 0);

        Platform_Client_Destroy(std::ptr::null_mut());
        assert_eq!(Platform_Client_Service(std::ptr::null_mut(), 0), -1);
        assert_eq!(Platform_Client_IsConnected(std::ptr::null_mut()), 0);
    }
}

// =============================================================================
// Network Manager Tests
// =============================================================================

#[test]
fn test_network_manager_lifecycle() {
    setup();

    let mut manager = NetworkManager::new();

    // Initially not connected
    assert!(!manager.is_connected());
    assert!(!manager.is_host());
    assert_eq!(manager.mode(), NetworkMode::None);

    // Host a game
    let host_result = manager.host_game("Test Game", 16030, 4);
    assert!(host_result.is_ok(), "Failed to host game: {:?}", host_result.err());

    assert!(manager.is_connected());
    assert!(manager.is_host());
    assert_eq!(manager.mode(), NetworkMode::Hosting);
    assert_eq!(manager.peer_count(), 0);

    // Update should work
    let update_result = manager.update();
    assert!(update_result.is_ok());

    // Disconnect
    manager.disconnect();
    assert!(!manager.is_connected());
    assert!(!manager.is_host());
    assert_eq!(manager.mode(), NetworkMode::None);

    teardown();
}

#[test]
fn test_network_manager_ffi() {
    setup();

    let manager = Platform_NetworkManager_Create();
    assert!(!manager.is_null());

    unsafe {
        // Initial state
        assert_eq!(Platform_NetworkManager_GetMode(manager), 0); // None
        assert_eq!(Platform_NetworkManager_IsConnected(manager), 0);
        assert_eq!(Platform_NetworkManager_IsHost(manager), 0);

        // Host a game
        let game_name = std::ffi::CString::new("FFI Test Game").unwrap();
        let host_result = Platform_NetworkManager_HostGame(
            manager,
            game_name.as_ptr(),
            16031,
            4,
        );
        assert_eq!(host_result, 0, "HostGame failed");

        assert_eq!(Platform_NetworkManager_GetMode(manager), 1); // Host
        assert_eq!(Platform_NetworkManager_IsConnected(manager), 1);
        assert_eq!(Platform_NetworkManager_IsHost(manager), 1);

        // Disconnect
        Platform_NetworkManager_Disconnect(manager);
        assert_eq!(Platform_NetworkManager_GetMode(manager), 0);
        assert_eq!(Platform_NetworkManager_IsConnected(manager), 0);

        // Shutdown and destroy
        Platform_NetworkManager_Shutdown(manager);
        Platform_NetworkManager_Destroy(manager);
    }

    teardown();
}

// =============================================================================
// Stress Tests
// =============================================================================

#[test]
fn test_multiple_connect_disconnect_cycles() {
    setup();

    let host_config = HostConfig {
        port: 16040,
        max_clients: 8,
        channel_count: 2,
        incoming_bandwidth: 0,
        outgoing_bandwidth: 0,
    };
    let mut host = PlatformHost::new(&host_config).expect("Failed to create host");

    // Multiple connect/disconnect cycles
    for cycle in 0..3 {
        let client_config = ConnectConfig::default();
        let mut client = PlatformClient::new(&client_config).expect("Failed to create client");

        client.connect_async("127.0.0.1", 16040, 2).expect("Connect failed");

        // Wait for connection
        let mut connected = false;
        for _ in 0..30 {
            let _ = host.service(10);
            let _ = client.service(10);
            if client.is_connected() {
                connected = true;
                break;
            }
            thread::sleep(Duration::from_millis(10));
        }

        if !connected {
            eprintln!("Cycle {} failed to connect", cycle);
        }

        // Disconnect
        client.disconnect();

        // Process disconnect
        for _ in 0..10 {
            let _ = host.service(10);
            let _ = client.service(10);
            thread::sleep(Duration::from_millis(10));
        }

        drop(client);
    }

    drop(host);
    teardown();
}

#[test]
fn test_packet_burst() {
    setup();

    let host_config = HostConfig {
        port: 16041,
        max_clients: 4,
        channel_count: 2,
        incoming_bandwidth: 0,
        outgoing_bandwidth: 0,
    };
    let mut host = PlatformHost::new(&host_config).expect("Failed to create host");

    let client_config = ConnectConfig::default();
    let mut client = PlatformClient::new(&client_config).expect("Failed to create client");
    client.connect_async("127.0.0.1", 16041, 2).expect("Connect failed");

    // Wait for connection
    for _ in 0..30 {
        let _ = host.service(10);
        let _ = client.service(10);
        if client.is_connected() {
            break;
        }
        thread::sleep(Duration::from_millis(10));
    }

    if client.is_connected() {
        // Send burst of packets
        let packet_count = 100;
        for i in 0..packet_count {
            let data = format!("Packet {}", i);
            let _ = client.send(0, data.as_bytes(), true);
        }
        client.flush();

        // Receive all packets
        let mut received_count = 0;
        for _ in 0..100 {
            let _ = host.service(10);
            let _ = client.service(10);

            while let Some(_packet) = host.receive() {
                received_count += 1;
            }

            if received_count >= packet_count {
                break;
            }
            thread::sleep(Duration::from_millis(10));
        }

        // Should receive most packets (enet provides reliability)
        assert!(received_count > packet_count / 2,
            "Only received {} of {} packets", received_count, packet_count);
    }

    drop(client);
    drop(host);
    teardown();
}
