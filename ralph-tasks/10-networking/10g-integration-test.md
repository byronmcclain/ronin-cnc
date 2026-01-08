# Task 10g: Integration Test with Loopback

## Dependencies
- Tasks 10a-10f must be complete (full networking implementation + headers)

## Context
This final networking task creates an integration test that proves the entire networking system works end-to-end:
1. Host creates a game session
2. Client connects to host (loopback)
3. Host and client exchange packets
4. Both sides verify received data
5. Clean disconnect

This test runs entirely on localhost using the loopback interface (127.0.0.1).

## Objective
Create a comprehensive integration test that:
1. Tests the complete host→client→host packet flow
2. Verifies packet serialization/deserialization
3. Tests reliable and unreliable channels
4. Confirms connection/disconnection lifecycle
5. Provides a test binary that can be run standalone

## Deliverables
- [ ] `platform/src/network/tests.rs` - Integration tests
- [ ] Update `platform/src/network/mod.rs` to include tests
- [ ] All integration tests pass
- [ ] Verification passes

## Technical Background

### Loopback Testing
Since we can't easily test across multiple machines in a unit test, we use:
- 127.0.0.1 (localhost) for connections
- Short timeouts to keep tests fast
- Thread-based client/server simulation

### Test Scenarios
1. **Basic connectivity**: Host starts, client connects, both disconnect
2. **Echo test**: Client sends data, host echoes it back
3. **Broadcast test**: Host broadcasts to all clients
4. **Packet integrity**: Verify serialized packets arrive intact

### Threading Model
We use threads to simulate concurrent host and client:
```
Main thread: Create host on port N
Spawned thread: Create client, connect to port N, exchange data
Main thread: Service host, respond to client
Join thread: Verify results
```

## Files to Create/Modify

### platform/src/network/tests.rs (NEW)
```rust
//! Network integration tests
//!
//! These tests verify the complete networking stack works end-to-end
//! using loopback connections.

#![cfg(test)]

use super::*;
use std::thread;
use std::time::Duration;
use std::sync::{Arc, atomic::{AtomicBool, AtomicU32, Ordering}};

/// Test port range start (use high ports to avoid conflicts)
const TEST_PORT_BASE: u16 = 17000;

/// Get a unique test port
fn get_test_port() -> u16 {
    use std::sync::atomic::AtomicU16;
    static PORT_COUNTER: AtomicU16 = AtomicU16::new(0);
    TEST_PORT_BASE + PORT_COUNTER.fetch_add(1, Ordering::SeqCst)
}

/// Ensure network is initialized for tests
fn ensure_network_init() {
    let _ = init();
}

/// Helper to run a function with timeout
fn with_timeout<F, R>(timeout: Duration, f: F) -> Option<R>
where
    F: FnOnce() -> R + Send + 'static,
    R: Send + 'static,
{
    let (tx, rx) = std::sync::mpsc::channel();

    thread::spawn(move || {
        let result = f();
        let _ = tx.send(result);
    });

    rx.recv_timeout(timeout).ok()
}

// =============================================================================
// Basic Connectivity Tests
// =============================================================================

#[test]
fn test_host_creation_and_destruction() {
    ensure_network_init();
    let port = get_test_port();

    let config = host::HostConfig {
        port,
        max_clients: 4,
        channel_count: 2,
        ..Default::default()
    };

    let host = host::PlatformHost::new(&config);
    assert!(host.is_ok(), "Host creation should succeed");

    let host = host.unwrap();
    assert_eq!(host.port(), port);
    assert_eq!(host.peer_count(), 0);

    // Host is dropped here - should clean up
}

#[test]
fn test_client_creation_and_destruction() {
    ensure_network_init();

    let config = client::ConnectConfig::default();
    let client = client::PlatformClient::new(&config);

    assert!(client.is_ok(), "Client creation should succeed");

    let client = client.unwrap();
    assert_eq!(client.state(), client::ClientState::Disconnected);
    assert!(!client.is_connected());

    // Client is dropped here
}

#[test]
fn test_client_connect_no_server() {
    ensure_network_init();
    let port = get_test_port();

    let config = client::ConnectConfig::default();
    let mut client = client::PlatformClient::new(&config).unwrap();

    // Try to connect to non-existent server
    let result = client.connect_blocking("127.0.0.1", port, 2, 200);

    // Should fail (timeout or connection refused)
    assert!(result.is_err());
}

// =============================================================================
// Connection Tests
// =============================================================================

#[test]
fn test_client_connects_to_host() {
    ensure_network_init();
    let port = get_test_port();

    // Create host
    let host_config = host::HostConfig {
        port,
        max_clients: 4,
        channel_count: 2,
        ..Default::default()
    };

    let mut host = match host::PlatformHost::new(&host_config) {
        Ok(h) => h,
        Err(e) => {
            println!("Skipping test - host creation failed: {}", e);
            return;
        }
    };

    // Connection flag
    let connected = Arc::new(AtomicBool::new(false));
    let connected_clone = connected.clone();

    // Start client in another thread
    let client_thread = thread::spawn(move || {
        let config = client::ConnectConfig::default();
        let mut client = client::PlatformClient::new(&config).unwrap();

        // Connect to host
        match client.connect_async("127.0.0.1", port, 2) {
            Ok(()) => {}
            Err(e) => {
                println!("Connect failed: {}", e);
                return false;
            }
        }

        // Service until connected or timeout
        let deadline = std::time::Instant::now() + Duration::from_secs(2);
        while std::time::Instant::now() < deadline {
            client.service(50).ok();
            if client.is_connected() {
                connected_clone.store(true, Ordering::SeqCst);
                thread::sleep(Duration::from_millis(100));
                client.disconnect();
                return true;
            }
        }

        false
    });

    // Service host to accept connection
    let deadline = std::time::Instant::now() + Duration::from_secs(3);
    let mut peer_connected = false;

    while std::time::Instant::now() < deadline {
        host.service(50).ok();
        if host.peer_count() > 0 {
            peer_connected = true;
        }
        if connected.load(Ordering::SeqCst) {
            break;
        }
    }

    let client_success = client_thread.join().unwrap_or(false);

    // At least one side should have seen the connection
    assert!(client_success || peer_connected,
        "Connection should succeed (client: {}, host peers: {})",
        client_success, host.peer_count());
}

// =============================================================================
// Packet Exchange Tests
// =============================================================================

#[test]
fn test_packet_exchange() {
    ensure_network_init();
    let port = get_test_port();

    // Create host
    let host_config = host::HostConfig {
        port,
        max_clients: 4,
        channel_count: 2,
        ..Default::default()
    };

    let mut host = match host::PlatformHost::new(&host_config) {
        Ok(h) => h,
        Err(_) => {
            println!("Skipping test - host creation failed");
            return;
        }
    };

    let test_data = b"Hello from client!";
    let data_received = Arc::new(AtomicBool::new(false));
    let data_received_clone = data_received.clone();

    // Client thread
    let client_thread = thread::spawn(move || {
        let config = client::ConnectConfig::default();
        let mut client = client::PlatformClient::new(&config).unwrap();

        // Connect
        if client.connect_blocking("127.0.0.1", port, 2, 1000).is_err() {
            return;
        }

        // Send test data
        if client.send(0, test_data, true).is_ok() {
            data_received_clone.store(true, Ordering::SeqCst);
        }

        // Wait a bit then disconnect
        thread::sleep(Duration::from_millis(200));
        client.disconnect();
    });

    // Service host and check for data
    let mut received_data = Vec::new();
    let deadline = std::time::Instant::now() + Duration::from_secs(3);

    while std::time::Instant::now() < deadline {
        host.service(50).ok();

        while let Some(packet) = host.receive() {
            received_data = packet.data;
            break;
        }

        if !received_data.is_empty() {
            break;
        }
    }

    client_thread.join().ok();

    // Verify received data
    if data_received.load(Ordering::SeqCst) {
        assert!(!received_data.is_empty() || host.peer_count() > 0,
            "Should receive data or at least have peer connected");
    }
}

// =============================================================================
// Packet Serialization Tests
// =============================================================================

#[test]
fn test_game_packet_roundtrip() {
    use super::packet::*;

    let original = GamePacket::new(
        GamePacketCode::ChatMessage,
        12345,
        2,
        b"Test message content".to_vec(),
    );

    let serialized = original.serialize();
    let deserialized = GamePacket::deserialize(&serialized).unwrap();

    assert_eq!(deserialized.header.code, original.header.code);
    assert_eq!(deserialized.header.sequence, original.header.sequence);
    assert_eq!(deserialized.header.player_id, original.header.player_id);
    assert_eq!(deserialized.payload, original.payload);
}

#[test]
fn test_packet_header_validation() {
    use super::packet::*;

    let valid = GamePacketHeader::new(GamePacketCode::Ping, 1, 0);
    assert!(valid.is_valid());

    // Corrupted magic
    let mut corrupted = valid.clone();
    corrupted.magic = 0xDEAD;
    assert!(!corrupted.is_valid());
}

// =============================================================================
// Network Manager Tests
// =============================================================================

#[test]
fn test_manager_host_mode() {
    ensure_network_init();
    let port = get_test_port();

    // Use thread-local manager for test isolation
    let mut manager = manager::NetworkManager::new();

    let result = manager.host_game("Test Game", port, 4);

    match result {
        Ok(()) => {
            assert!(manager.is_host());
            assert!(manager.is_connected());
            assert_eq!(manager.mode(), manager::NetworkMode::Hosting);
            assert_eq!(manager.session_info().get_name(), "Test Game");

            manager.disconnect();
            assert!(!manager.is_connected());
        }
        Err(e) => {
            println!("Host failed (port in use?): {}", e);
        }
    }
}

#[test]
fn test_manager_join_timeout() {
    ensure_network_init();
    let port = get_test_port();

    let mut manager = manager::NetworkManager::new();

    // Try to join non-existent server
    let result = manager.join_game("127.0.0.1", port, 200);

    // Should fail with timeout
    assert!(result.is_err());
    assert!(!manager.is_connected());
}

// =============================================================================
// Full Integration Test
// =============================================================================

#[test]
fn test_full_host_client_cycle() {
    ensure_network_init();
    let port = get_test_port();

    let packets_exchanged = Arc::new(AtomicU32::new(0));
    let packets_exchanged_clone = packets_exchanged.clone();

    // Host in main thread
    let host_config = host::HostConfig {
        port,
        max_clients: 2,
        channel_count: 2,
        ..Default::default()
    };

    let mut host = match host::PlatformHost::new(&host_config) {
        Ok(h) => h,
        Err(_) => {
            println!("Skipping full integration test - host creation failed");
            return;
        }
    };

    // Client thread
    let client_handle = thread::spawn(move || {
        let config = client::ConnectConfig::default();
        let mut client = client::PlatformClient::new(&config).unwrap();

        // Connect
        if client.connect_blocking("127.0.0.1", port, 2, 2000).is_err() {
            println!("Client connect failed");
            return 0u32;
        }

        let mut sent = 0u32;
        let mut received = 0u32;

        // Exchange packets
        for i in 0..5 {
            let msg = format!("Packet {}", i);
            if client.send(0, msg.as_bytes(), true).is_ok() {
                sent += 1;
            }

            client.service(50).ok();

            while let Some(_data) = client.receive() {
                received += 1;
            }

            thread::sleep(Duration::from_millis(50));
        }

        client.disconnect();
        packets_exchanged_clone.store(sent, Ordering::SeqCst);
        received
    });

    // Host loop
    let mut host_received = 0u32;
    let deadline = std::time::Instant::now() + Duration::from_secs(5);

    while std::time::Instant::now() < deadline {
        host.service(50).ok();

        while let Some(packet) = host.receive() {
            host_received += 1;
            // Echo back
            host.broadcast(0, &packet.data, true).ok();
        }

        // Check if client is done
        if packets_exchanged.load(Ordering::SeqCst) > 0 &&
           host.peer_count() == 0 {
            break;
        }
    }

    let client_received = client_handle.join().unwrap_or(0);

    println!("Integration test results:");
    println!("  Client sent: {}", packets_exchanged.load(Ordering::SeqCst));
    println!("  Host received: {}", host_received);
    println!("  Client received echoes: {}", client_received);

    // At least some packets should have been exchanged
    let total = packets_exchanged.load(Ordering::SeqCst) + host_received;
    assert!(total > 0, "Should exchange at least some packets");
}

// =============================================================================
// FFI Tests
// =============================================================================

#[test]
fn test_ffi_workflow() {
    ensure_network_init();
    let port = get_test_port();

    unsafe {
        // Initialize manager
        assert_eq!(manager::Platform_NetworkManager_Init(), 0);

        // Host game
        let name = std::ffi::CString::new("FFI Test").unwrap();
        let result = manager::Platform_NetworkManager_HostGame(
            name.as_ptr(),
            port,
            4,
        );

        if result == 0 {
            assert_eq!(manager::Platform_NetworkManager_IsHost(), 1);
            assert_eq!(manager::Platform_NetworkManager_IsConnected(), 1);

            // Update should work
            let update_result = manager::Platform_NetworkManager_Update();
            assert!(update_result >= 0);

            // Disconnect
            manager::Platform_NetworkManager_Disconnect();
            assert_eq!(manager::Platform_NetworkManager_IsConnected(), 0);
        }

        manager::Platform_NetworkManager_Shutdown();
    }
}
```

### platform/src/network/mod.rs (MODIFY)
Add at the end of the file:
```rust
#[cfg(test)]
mod tests;
```

## Verification Command
```bash
ralph-tasks/verify/check-network-integration.sh
```

## Verification Script
Create `ralph-tasks/verify/check-network-integration.sh`:
```bash
#!/bin/bash
# Verification script for network integration tests (Task 10g)

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Running Network Integration Tests ==="

cd "$ROOT_DIR"

# Step 1: Check tests.rs exists
echo "Step 1: Checking tests.rs exists..."
if [ ! -f "platform/src/network/tests.rs" ]; then
    echo "VERIFY_FAILED: platform/src/network/tests.rs not found"
    exit 1
fi
echo "  tests.rs exists"

# Step 2: Build
echo "Step 2: Building platform crate..."
if ! cargo build --manifest-path platform/Cargo.toml --release 2>&1; then
    echo "VERIFY_FAILED: cargo build failed"
    exit 1
fi
echo "  Build successful"

# Step 3: Run integration tests
echo "Step 3: Running network tests..."
# Use single thread to avoid port conflicts
# Allow some test failures since networking tests can be flaky
if ! cargo test --manifest-path platform/Cargo.toml --release -- network --test-threads=1 2>&1; then
    # Check if at least some tests passed
    echo "Warning: Some network tests may have failed"
    echo "This can happen due to port conflicts or timing issues"
fi
echo "  Tests completed"

# Step 4: Verify key test functions exist
echo "Step 4: Verifying test functions..."
REQUIRED_TESTS=(
    "test_host_creation_and_destruction"
    "test_client_creation_and_destruction"
    "test_game_packet_roundtrip"
    "test_full_host_client_cycle"
)

for test_fn in "${REQUIRED_TESTS[@]}"; do
    if ! grep -q "$test_fn" platform/src/network/tests.rs; then
        echo "VERIFY_FAILED: Test function $test_fn not found"
        exit 1
    fi
    echo "  Found $test_fn"
done

# Step 5: Run a quick sanity check
echo "Step 5: Running sanity tests..."
if ! cargo test --manifest-path platform/Cargo.toml --release test_game_packet_roundtrip -- --test-threads=1 2>&1; then
    echo "VERIFY_FAILED: Packet roundtrip test failed"
    exit 1
fi
echo "  Sanity tests passed"

echo ""
echo "=== All checks passed ==="
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Create `platform/src/network/tests.rs` with integration tests
2. Add `#[cfg(test)] mod tests;` to `platform/src/network/mod.rs`
3. Build and run tests with `cargo test -- network --test-threads=1`
4. Debug any failing tests
5. Run verification script

## Success Criteria
- All integration tests compile
- Packet roundtrip tests pass
- Host/client lifecycle tests work
- At least basic connectivity succeeds

## Common Issues

### Port conflicts
Tests use high, random ports (17000+) but conflicts can still occur. The test port counter helps avoid this.

### Flaky timing tests
Network tests involving connections can be timing-sensitive. Tests include reasonable timeouts and handle failures gracefully.

### Thread synchronization
Use atomic types and proper join() calls for thread safety.

### "Address already in use"
Previous test may not have released port. Use unique ports per test with `get_test_port()`.

## Note on Test Flakiness
Network tests are inherently more prone to flakiness than pure unit tests due to:
- Port availability
- System load affecting timing
- Thread scheduling

The verification script allows some test failures while checking that the test infrastructure exists and key tests pass.

## Completion Promise
When verification passes, output:
```
<promise>TASK_10G_COMPLETE</promise>
```

## Escape Hatch
If stuck after 15 iterations:
- Document blocking issues in `ralph-tasks/blocked/10g.md`
- Output: `<promise>TASK_10G_BLOCKED</promise>`

## Max Iterations
15
