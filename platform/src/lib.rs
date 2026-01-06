//! Red Alert Platform Layer
//!
//! Provides cross-platform abstractions for the Red Alert macOS port.
//! This library exposes a C-compatible FFI for integration with the
//! existing C++ codebase.

use once_cell::sync::Lazy;
use std::sync::Mutex;

/// Platform version
const PLATFORM_VERSION: i32 = 1;

/// Platform state
static PLATFORM_STATE: Lazy<Mutex<Option<PlatformState>>> = Lazy::new(|| Mutex::new(None));

/// Last error message
static LAST_ERROR: Lazy<Mutex<String>> = Lazy::new(|| Mutex::new(String::new()));

struct PlatformState {
    initialized: bool,
}

fn set_last_error(msg: &str) {
    if let Ok(mut error) = LAST_ERROR.lock() {
        *error = msg.to_string();
    }
}

/// Result type for platform operations
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PlatformResult {
    Success = 0,
    AlreadyInitialized = 1,
    NotInitialized = 2,
    InitFailed = 3,
}

/// Helper macro to catch panics at FFI boundary
macro_rules! catch_panic {
    ($default:expr, $body:expr) => {
        match std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| $body)) {
            Ok(result) => result,
            Err(_) => {
                eprintln!("Panic caught at FFI boundary");
                $default
            }
        }
    };
}

/// Initialize the platform layer.
///
/// # Safety
/// This function is safe to call from C code.
/// Must be called before any other platform functions.
#[no_mangle]
pub extern "C" fn Platform_Init() -> PlatformResult {
    catch_panic!(PlatformResult::InitFailed, {
        let mut state = PLATFORM_STATE.lock().unwrap();
        if state.is_some() {
            return PlatformResult::AlreadyInitialized;
        }

        *state = Some(PlatformState { initialized: true });
        PlatformResult::Success
    })
}

/// Shutdown the platform layer.
///
/// # Safety
/// This function is safe to call from C code.
/// Must be called to clean up platform resources.
#[no_mangle]
pub extern "C" fn Platform_Shutdown() -> PlatformResult {
    catch_panic!(PlatformResult::NotInitialized, {
        let mut state = PLATFORM_STATE.lock().unwrap();
        if state.is_none() {
            return PlatformResult::NotInitialized;
        }

        *state = None;
        PlatformResult::Success
    })
}

/// Check if the platform is initialized.
///
/// # Safety
/// This function is safe to call from C code.
#[no_mangle]
pub extern "C" fn Platform_IsInitialized() -> bool {
    catch_panic!(false, {
        let state = PLATFORM_STATE.lock().unwrap();
        state.is_some()
    })
}

/// Get the platform layer version.
///
/// # Safety
/// This function is safe to call from C code.
#[no_mangle]
pub extern "C" fn Platform_GetVersion() -> i32 {
    PLATFORM_VERSION
}

/// Get the last error message.
///
/// # Safety
/// This function is safe to call from C code.
/// The buffer must be valid for buffer_size bytes.
/// Returns the number of bytes written (excluding null terminator),
/// or -1 if buffer is null or too small.
#[no_mangle]
pub extern "C" fn Platform_GetLastError(buffer: *mut i8, buffer_size: i32) -> i32 {
    catch_panic!(-1, {
        if buffer.is_null() || buffer_size <= 0 {
            return -1;
        }

        let error = match LAST_ERROR.lock() {
            Ok(e) => e.clone(),
            Err(_) => return -1,
        };

        let bytes = error.as_bytes();
        let copy_len = std::cmp::min(bytes.len(), (buffer_size - 1) as usize);

        unsafe {
            std::ptr::copy_nonoverlapping(bytes.as_ptr(), buffer as *mut u8, copy_len);
            *buffer.add(copy_len) = 0; // null terminator
        }

        copy_len as i32
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_init_shutdown_cycle() {
        // Ensure clean state
        {
            let mut state = PLATFORM_STATE.lock().unwrap();
            *state = None;
        }

        assert_eq!(Platform_Init(), PlatformResult::Success);
        assert!(Platform_IsInitialized());
        assert_eq!(Platform_Init(), PlatformResult::AlreadyInitialized);
        assert_eq!(Platform_Shutdown(), PlatformResult::Success);
        assert!(!Platform_IsInitialized());
        assert_eq!(Platform_Shutdown(), PlatformResult::NotInitialized);
    }
}
