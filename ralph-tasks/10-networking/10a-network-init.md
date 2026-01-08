# Task 10a: Network Module Setup and enet Initialization

## Dependencies
- Phase 01-09 must be complete
- Platform crate structure in place

## Context
This task creates the foundation for the networking subsystem. The original game used IPX (obsolete LAN protocol) and Winsock for networking. We replace this with the `enet` crate, which provides reliable UDP networking suitable for games. This first task sets up the module structure, adds the enet dependency, and implements basic initialization/shutdown.

**Critical Note**: The `enet` crate (version 0.3) is a wrapper around the C library libenet. On macOS, this requires libenet to be available. We use the `enet-sys` crate which bundles libenet for ease of building.

## Objective
Create the network module skeleton with:
1. Add `enet` dependency to Cargo.toml
2. Create `platform/src/network/mod.rs` with module structure
3. Implement `NetworkError` type for error handling
4. Implement `init()` and `shutdown()` functions
5. Create FFI exports for C++ usage
6. Verify enet initialization works

## Deliverables
- [ ] `platform/Cargo.toml` updated with enet dependency
- [ ] `platform/src/network/mod.rs` - Module root with init/shutdown
- [ ] FFI exports: `Platform_Network_Init`, `Platform_Network_Shutdown`, `Platform_Network_IsInitialized`
- [ ] Header contains FFI exports
- [ ] Unit tests pass
- [ ] Verification passes

## Technical Background

### Why enet?
- Reliable and unreliable packet delivery (like TCP + UDP combined)
- Automatic connection management
- Built-in sequencing and acknowledgment
- Designed specifically for games
- Cross-platform (Windows, macOS, Linux)

### enet Architecture
```
enet::initialize() -> Must be called once at startup
enet::Host        -> Server or client host
enet::Peer        -> Connected peer (has lifetime tied to Host)
enet::Packet      -> Data packet
```

### Lifetime Challenges
The `enet` crate has complex lifetimes - `Peer<'a, T>` borrows from `Host`. This makes storing peers difficult. Our solution:
- Use handles/indices to identify peers
- Store peer state separately from actual enet peers
- Use scoped access patterns

## Files to Create/Modify

### platform/Cargo.toml (MODIFY)
Add enet dependency after existing dependencies:
```toml
[dependencies]
once_cell = "1.19"
thiserror = "1.0"
log = "0.4"
sdl2 = { version = "0.36", features = ["bundled", "static-link"] }
rodio = { version = "0.19", default-features = false, features = ["symphonia-all"] }
enet = "0.3"
```

### platform/src/network/mod.rs (NEW)
```rust
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

use std::sync::atomic::{AtomicBool, Ordering};
use once_cell::sync::OnceCell;
use thiserror::Error;

/// Global initialization state
static INITIALIZED: AtomicBool = AtomicBool::new(false);

/// enet library handle (ensures enet stays initialized)
static ENET_HANDLE: OnceCell<()> = OnceCell::new();

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
        enet::initialize()
            .map(|_| ())
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
fn require_initialized() -> NetworkResult<()> {
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
```

### platform/src/lib.rs (MODIFY)
Add after other module declarations:
```rust
pub mod network;
```

## Verification Command
```bash
ralph-tasks/verify/check-network-init.sh
```

## Verification Script
Create `ralph-tasks/verify/check-network-init.sh`:
```bash
#!/bin/bash
# Verification script for network initialization

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Network Module Setup ==="

cd "$ROOT_DIR"

# Step 1: Check Cargo.toml has enet dependency
echo "Step 1: Checking enet dependency in Cargo.toml..."
if ! grep -q 'enet = "0.3"' platform/Cargo.toml; then
    echo "VERIFY_FAILED: enet dependency not found in Cargo.toml"
    exit 1
fi
echo "  enet dependency found"

# Step 2: Check network module exists
echo "Step 2: Checking network module exists..."
if [ ! -f "platform/src/network/mod.rs" ]; then
    echo "VERIFY_FAILED: platform/src/network/mod.rs not found"
    exit 1
fi
echo "  network/mod.rs exists"

# Step 3: Build the platform crate
echo "Step 3: Building platform crate..."
if ! cargo build --manifest-path platform/Cargo.toml --release 2>&1; then
    echo "VERIFY_FAILED: cargo build failed"
    exit 1
fi
echo "  Build successful"

# Step 4: Run network tests
echo "Step 4: Running network tests..."
if ! cargo test --manifest-path platform/Cargo.toml --release -- network --test-threads=1 2>&1; then
    echo "VERIFY_FAILED: network tests failed"
    exit 1
fi
echo "  Tests passed"

# Step 5: Generate and check header
echo "Step 5: Checking FFI exports in header..."
if ! cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1; then
    echo "VERIFY_FAILED: cbindgen header generation failed"
    exit 1
fi

if [ ! -f "include/platform.h" ]; then
    echo "VERIFY_FAILED: include/platform.h not found"
    exit 1
fi

# Check for network FFI exports
for func in Platform_Network_Init Platform_Network_Shutdown Platform_Network_IsInitialized; do
    if ! grep -q "$func" include/platform.h; then
        echo "VERIFY_FAILED: $func not found in platform.h"
        exit 1
    fi
    echo "  Found $func in header"
done

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Edit `platform/Cargo.toml` to add enet dependency
2. Create directory `platform/src/network/`
3. Create `platform/src/network/mod.rs` with initialization code
4. Add `pub mod network;` to `platform/src/lib.rs`
5. Run `cargo build --manifest-path platform/Cargo.toml` to verify compilation
6. Run `cargo test --manifest-path platform/Cargo.toml -- network --test-threads=1`
7. Run `cargo build --manifest-path platform/Cargo.toml --features cbindgen-run`
8. Verify header contains exports
9. Run verification script

## Success Criteria
- enet crate compiles and links successfully
- Network initialization works without panics
- FFI exports appear in platform.h
- All unit tests pass
- Verification script exits with 0

## Common Issues

### "enet-sys failed to compile"
The enet-sys crate builds libenet from source. Ensure you have:
- A C compiler (clang/gcc)
- CMake (for libenet build)

On macOS: `xcode-select --install` if needed

### "cannot find value enet in this scope"
Ensure the enet crate is added to Cargo.toml dependencies.

### "undefined symbol: enet_initialize"
The static library isn't linking correctly. Check that:
- crate-type includes "staticlib"
- Build is in release mode

### Tests fail with "already initialized"
Tests must run single-threaded due to global state:
```bash
cargo test -- --test-threads=1
```

### OnceCell error
If `get_or_try_init` isn't available, ensure once_cell is version 1.19+.

## Platform-Specific Notes

### macOS
- Works on both Intel and Apple Silicon
- libenet is built from source by enet-sys
- No external dependencies required

### Linux (future)
- May need `libc-dev` installed
- Otherwise identical to macOS

### Windows (future)
- WinSock2 is linked automatically by enet-sys
- May need Visual Studio build tools

## Completion Promise
When verification passes, output:
```
<promise>TASK_10A_COMPLETE</promise>
```

## Escape Hatch
If stuck after 12 iterations:
- Document what's blocking in `ralph-tasks/blocked/10a.md`
- List attempted approaches
- Output: `<promise>TASK_10A_BLOCKED</promise>`

## Max Iterations
12
