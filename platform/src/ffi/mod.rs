//! FFI exports for C/C++ interop
//!
//! All functions in this module use C calling convention and are
//! designed to be called from the game's C++ code.

use crate::error::{catch_panic_or, get_error, set_error, clear_error};
use crate::{PLATFORM_STATE, PlatformState, PlatformResult, PLATFORM_VERSION};
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
pub extern "C" fn Platform_Init() -> PlatformResult {
    catch_panic_or(PlatformResult::InitFailed, || {
        let mut state = PLATFORM_STATE.lock().unwrap();
        if state.is_some() {
            return PlatformResult::AlreadyInitialized;
        }
        *state = Some(PlatformState { initialized: true });
        PlatformResult::Success
    })
}

/// Shutdown the platform layer
#[no_mangle]
pub extern "C" fn Platform_Shutdown() -> PlatformResult {
    catch_panic_or(PlatformResult::NotInitialized, || {
        let mut state = PLATFORM_STATE.lock().unwrap();
        if state.is_none() {
            return PlatformResult::NotInitialized;
        }
        *state = None;
        PlatformResult::Success
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

/// Get the last error message into a buffer
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

/// Static buffer for error string (thread-safe)
static ERROR_STRING_BUF: std::sync::Mutex<[u8; 512]> = std::sync::Mutex::new([0; 512]);

/// Get error as static string pointer (valid until next error)
/// Returns null if no error
#[no_mangle]
pub extern "C" fn Platform_GetErrorString() -> *const c_char {
    catch_panic_or(std::ptr::null(), || {
        let error = get_error();
        if error.is_empty() {
            return std::ptr::null();
        }

        if let Ok(mut buf) = ERROR_STRING_BUF.lock() {
            let bytes = error.as_bytes();
            let len = std::cmp::min(bytes.len(), buf.len() - 1);
            buf[..len].copy_from_slice(&bytes[..len]);
            buf[len] = 0;
            buf.as_ptr() as *const c_char
        } else {
            std::ptr::null()
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
