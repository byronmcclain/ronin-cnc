# Task 02b: Core FFI Exports

## Dependencies
- Task 02a must be complete (error types and panic catching)
- `platform/src/error.rs` exists and tests pass

## Context
Now that we have proper error handling, we can expand the FFI surface to include logging, error retrieval, and configuration structures that will be needed by the C++ game code.

## Objective
Create a dedicated FFI module with core platform exports including logging, error handling, and version information.

## Deliverables
- [ ] `platform/src/ffi/mod.rs` - Core FFI exports
- [ ] Update `lib.rs` to use FFI module
- [ ] LogLevel enum exported
- [ ] Platform_GetError, Platform_SetError, Platform_ClearError functions
- [ ] Platform_Log function
- [ ] Header regenerated with new exports

## Files to Create/Modify

### platform/src/ffi/mod.rs (NEW)
```rust
//! FFI exports for C/C++ interop
//!
//! All functions in this module use C calling convention and are
//! designed to be called from the game's C++ code.

use crate::error::{catch_panic, catch_panic_or, get_error, set_error, clear_error};
use crate::{PLATFORM_STATE, PlatformState, PlatformResult, PlatformError, PLATFORM_VERSION};
use std::ffi::{c_char, CStr};

/// Log level for Platform_Log
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum LogLevel {
    Debug = 0,
    Info = 1,
    Warn = 2,
    Error = 3,
}

// =============================================================================
// Platform Lifecycle
// =============================================================================

/// Initialize the platform layer
#[no_mangle]
pub extern "C" fn Platform_Init() -> i32 {
    catch_panic(|| {
        let mut state = PLATFORM_STATE.lock().map_err(|_| PlatformError::Sdl("Lock poisoned".into()))?;
        if state.is_some() {
            return Err(PlatformError::AlreadyInitialized);
        }
        *state = Some(PlatformState { initialized: true });
        Ok(())
    })
}

/// Shutdown the platform layer
#[no_mangle]
pub extern "C" fn Platform_Shutdown() -> i32 {
    catch_panic(|| {
        let mut state = PLATFORM_STATE.lock().map_err(|_| PlatformError::Sdl("Lock poisoned".into()))?;
        if state.is_none() {
            return Err(PlatformError::NotInitialized);
        }
        *state = None;
        Ok(())
    })
}

/// Check if platform is initialized
#[no_mangle]
pub extern "C" fn Platform_IsInitialized() -> bool {
    catch_panic_or(false, || {
        PLATFORM_STATE.lock().map(|s| s.is_some()).unwrap_or(false)
    })
}

/// Get platform version
#[no_mangle]
pub extern "C" fn Platform_GetVersion() -> i32 {
    PLATFORM_VERSION
}

// =============================================================================
// Error Handling
// =============================================================================

/// Get the last error message
/// Returns number of bytes written, or -1 on error
/// Buffer will be null-terminated if there's space
#[no_mangle]
pub extern "C" fn Platform_GetLastError(buffer: *mut i8, buffer_size: i32) -> i32 {
    catch_panic_or(-1, || {
        if buffer.is_null() || buffer_size <= 0 {
            return -1;
        }

        let error = get_error();
        let bytes = error.as_bytes();
        let copy_len = std::cmp::min(bytes.len(), (buffer_size - 1) as usize);

        unsafe {
            std::ptr::copy_nonoverlapping(bytes.as_ptr(), buffer as *mut u8, copy_len);
            *buffer.add(copy_len) = 0; // null terminator
        }

        copy_len as i32
    })
}

/// Get error as static string pointer (valid until next error)
/// Returns null if no error
#[no_mangle]
pub extern "C" fn Platform_GetErrorString() -> *const c_char {
    static mut ERROR_BUF: [u8; 512] = [0; 512];

    catch_panic_or(std::ptr::null(), || {
        let error = get_error();
        if error.is_empty() {
            return std::ptr::null();
        }

        unsafe {
            let bytes = error.as_bytes();
            let len = std::cmp::min(bytes.len(), ERROR_BUF.len() - 1);
            ERROR_BUF[..len].copy_from_slice(&bytes[..len]);
            ERROR_BUF[len] = 0;
            ERROR_BUF.as_ptr() as *const c_char
        }
    })
}

/// Set an error message from C string
#[no_mangle]
pub unsafe extern "C" fn Platform_SetError(msg: *const c_char) {
    if msg.is_null() {
        return;
    }

    if let Ok(s) = CStr::from_ptr(msg).to_str() {
        set_error(s);
    }
}

/// Clear the last error
#[no_mangle]
pub extern "C" fn Platform_ClearError() {
    clear_error();
}

// =============================================================================
// Logging
// =============================================================================

/// Log a message at the specified level
#[no_mangle]
pub unsafe extern "C" fn Platform_Log(level: LogLevel, msg: *const c_char) {
    if msg.is_null() {
        return;
    }

    let msg_str = match CStr::from_ptr(msg).to_str() {
        Ok(s) => s,
        Err(_) => return,
    };

    match level {
        LogLevel::Debug => eprintln!("[DEBUG] {}", msg_str),
        LogLevel::Info => eprintln!("[INFO] {}", msg_str),
        LogLevel::Warn => eprintln!("[WARN] {}", msg_str),
        LogLevel::Error => eprintln!("[ERROR] {}", msg_str),
    }
}

/// Log a formatted debug message
#[no_mangle]
pub unsafe extern "C" fn Platform_LogDebug(msg: *const c_char) {
    Platform_Log(LogLevel::Debug, msg);
}

/// Log a formatted info message
#[no_mangle]
pub unsafe extern "C" fn Platform_LogInfo(msg: *const c_char) {
    Platform_Log(LogLevel::Info, msg);
}

/// Log a formatted warning message
#[no_mangle]
pub unsafe extern "C" fn Platform_LogWarn(msg: *const c_char) {
    Platform_Log(LogLevel::Warn, msg);
}

/// Log a formatted error message
#[no_mangle]
pub unsafe extern "C" fn Platform_LogError(msg: *const c_char) {
    Platform_Log(LogLevel::Error, msg);
}
```

### Update platform/src/lib.rs
Replace existing FFI functions with module reference:
```rust
//! Red Alert Platform Layer

use once_cell::sync::Lazy;
use std::sync::Mutex;

pub mod error;
pub mod ffi;

pub use error::PlatformError;

/// Platform version
pub const PLATFORM_VERSION: i32 = 1;

/// Result type alias
pub type PlatformResult<T> = Result<T, PlatformError>;

/// Platform state
pub(crate) static PLATFORM_STATE: Lazy<Mutex<Option<PlatformState>>> =
    Lazy::new(|| Mutex::new(None));

pub(crate) struct PlatformState {
    pub initialized: bool,
}

// Re-export FFI functions at crate root for cbindgen
pub use ffi::*;
```

## Verification Command
```bash
cd platform && \
cargo build --features cbindgen-run 2>&1 && \
grep -q "Platform_Log" ../include/platform.h && \
grep -q "Platform_GetErrorString" ../include/platform.h && \
grep -q "LogLevel" ../include/platform.h && \
clang -fsyntax-only -x c ../include/platform.h 2>&1 && \
echo "VERIFY_SUCCESS"
```

## Implementation Steps
1. Create `platform/src/ffi/` directory
2. Create `platform/src/ffi/mod.rs` with FFI exports
3. Update `platform/src/lib.rs` to use FFI module
4. Remove duplicate function definitions from lib.rs
5. Run `cargo build --features cbindgen-run`
6. Verify header contains new exports
7. Verify header compiles with clang
8. If any step fails, analyze error and fix

## Expected Header Additions
```c
typedef enum LogLevel {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_ERROR = 3,
} LogLevel;

void Platform_Log(enum LogLevel level, const char *msg);
void Platform_LogDebug(const char *msg);
void Platform_LogInfo(const char *msg);
void Platform_LogWarn(const char *msg);
void Platform_LogError(const char *msg);
const char *Platform_GetErrorString(void);
void Platform_SetError(const char *msg);
void Platform_ClearError(void);
```

## Common Issues

### "unresolved import ffi"
- Ensure `pub mod ffi;` is in lib.rs

### Duplicate symbol errors
- Remove old Platform_* functions from lib.rs when moving to ffi/mod.rs

### Header not regenerated
- Ensure `pub use ffi::*;` re-exports at crate root

## Completion Promise
When verification passes (header generated with all exports), output:
<promise>TASK_02B_COMPLETE</promise>

## Escape Hatch
If stuck after 12 iterations:
- Document blocking issue in `ralph-tasks/blocked/02b.md`
- Output: <promise>TASK_02B_BLOCKED</promise>

## Max Iterations
12
