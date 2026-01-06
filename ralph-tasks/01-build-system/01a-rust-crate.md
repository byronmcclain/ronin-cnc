# Task 01a: Rust Crate Skeleton

## Dependencies
- None (this is the first task)

## Context
This is the foundation task. We need a minimal Rust crate that compiles as a static library and exports C-compatible functions. This crate will eventually contain all platform abstraction code.

## Objective
Create the `platform/` Rust crate with proper structure, dependencies, and minimal FFI exports.

## Deliverables
- [ ] `platform/Cargo.toml` - Crate configuration with all dependencies
- [ ] `platform/src/lib.rs` - Crate root with module declarations
- [ ] `platform/src/ffi/mod.rs` - FFI utilities (catch_panic, error codes)
- [ ] Static library builds successfully

## Files to Create

### platform/Cargo.toml
```toml
[package]
name = "redalert-platform"
version = "0.1.0"
edition = "2021"
description = "Platform abstraction layer for Red Alert macOS port"

[lib]
name = "redalert_platform"
crate-type = ["staticlib"]

[features]
default = []
cbindgen-run = []

[dependencies]
sdl2 = { version = "0.36", features = ["bundled", "static-link"] }
rodio = { version = "0.17", default-features = false, features = ["wav", "vorbis"] }
enet = "0.3"
log = "0.4"
env_logger = "0.10"
thiserror = "1.0"
libc = "0.2"
once_cell = "1.18"

[build-dependencies]
cbindgen = "0.26"

[profile.release]
lto = true
codegen-units = 1
panic = "abort"

[profile.dev]
panic = "abort"
```

### platform/src/lib.rs
```rust
//! Red Alert Platform Layer
//!
//! Provides platform abstraction for the macOS port.

#![allow(clippy::missing_safety_doc)]

pub mod ffi;

use std::sync::OnceLock;
use log::info;

/// Platform initialization state
static INITIALIZED: OnceLock<bool> = OnceLock::new();

/// Initialize the platform layer
pub fn init() -> Result<(), PlatformError> {
    if INITIALIZED.get().is_some() {
        return Err(PlatformError::AlreadyInitialized);
    }

    env_logger::init();
    info!("Platform layer initializing...");

    INITIALIZED.set(true).ok();
    info!("Platform layer initialized");
    Ok(())
}

/// Shutdown the platform layer
pub fn shutdown() {
    info!("Platform layer shutting down");
}

/// Platform error types
#[derive(Debug, thiserror::Error)]
pub enum PlatformError {
    #[error("Already initialized")]
    AlreadyInitialized,

    #[error("Not initialized")]
    NotInitialized,

    #[error("SDL error: {0}")]
    Sdl(String),

    #[error("IO error: {0}")]
    Io(#[from] std::io::Error),

    #[error("Null pointer")]
    NullPointer,

    #[error("Invalid data")]
    InvalidData,

    #[error("Buffer too small")]
    BufferTooSmall,
}

pub type PlatformResult<T> = Result<T, PlatformError>;

// FFI Exports
#[no_mangle]
pub extern "C" fn Platform_Init() -> i32 {
    ffi::catch_panic(|| init())
}

#[no_mangle]
pub extern "C" fn Platform_Shutdown() {
    shutdown();
}

#[no_mangle]
pub extern "C" fn Platform_GetVersion() -> i32 {
    1  // Version 1.0
}
```

### platform/src/ffi/mod.rs
```rust
//! FFI utilities for safe Rust-to-C boundary

use crate::{PlatformError, PlatformResult};
use log::error;
use std::sync::Mutex;
use once_cell::sync::Lazy;

/// Last error message for C code to retrieve
static LAST_ERROR: Lazy<Mutex<String>> = Lazy::new(|| Mutex::new(String::new()));

/// Set the last error message
pub fn set_error(msg: &str) {
    if let Ok(mut guard) = LAST_ERROR.lock() {
        *guard = msg.to_string();
    }
}

/// Convert Result to C error code
/// 0 = success, negative = error
pub fn result_to_code<T>(result: PlatformResult<T>) -> i32 {
    match result {
        Ok(_) => 0,
        Err(e) => {
            let code = match &e {
                PlatformError::AlreadyInitialized => -1,
                PlatformError::NotInitialized => -2,
                PlatformError::Sdl(_) => -3,
                PlatformError::Io(_) => -4,
                PlatformError::NullPointer => -5,
                PlatformError::InvalidData => -6,
                PlatformError::BufferTooSmall => -7,
            };
            set_error(&e.to_string());
            error!("Platform error: {}", e);
            code
        }
    }
}

/// Catch panics at FFI boundary and convert to error code
pub fn catch_panic<F, T>(f: F) -> i32
where
    F: FnOnce() -> PlatformResult<T> + std::panic::UnwindSafe,
{
    match std::panic::catch_unwind(f) {
        Ok(result) => result_to_code(result),
        Err(_) => {
            set_error("Panic caught at FFI boundary");
            error!("Panic caught at FFI boundary!");
            -999
        }
    }
}

/// Get last error message
#[no_mangle]
pub unsafe extern "C" fn Platform_GetLastError(
    buffer: *mut i8,
    buffer_size: i32,
) -> i32 {
    if buffer.is_null() || buffer_size <= 0 {
        return -1;
    }

    let msg = match LAST_ERROR.lock() {
        Ok(guard) => guard.clone(),
        Err(_) => return -1,
    };

    let bytes = msg.as_bytes();
    let copy_len = std::cmp::min(bytes.len(), (buffer_size - 1) as usize);

    std::ptr::copy_nonoverlapping(
        bytes.as_ptr() as *const i8,
        buffer,
        copy_len,
    );

    // Null terminate
    *buffer.add(copy_len) = 0;

    copy_len as i32
}
```

## Verification Command
```bash
cd platform && cargo build --release && \
  test -f target/release/libredalert_platform.a && \
  echo "VERIFY_SUCCESS"
```

## Implementation Steps
1. Create the `platform/` directory if it doesn't exist
2. Write `Cargo.toml` with all dependencies
3. Write `src/lib.rs` with module structure and basic exports
4. Write `src/ffi/mod.rs` with error handling utilities
5. Run `cargo build --release`
6. If build fails, read error message and fix
7. Repeat until build succeeds
8. Verify static library exists

## Completion Promise
When verification passes (cargo build succeeds and .a file exists), output:
<promise>TASK_01A_COMPLETE</promise>

## Escape Hatch
If stuck after 15 iterations:
- Document what's blocking in `ralph-tasks/blocked/01a.md`
- Include full cargo error output
- List what was attempted
- Output: <promise>TASK_01A_BLOCKED</promise>

## Max Iterations
15
