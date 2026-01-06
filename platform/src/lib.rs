//! Red Alert Platform Layer
//!
//! Provides cross-platform abstractions for the Red Alert macOS port.
//! This library exposes a C-compatible FFI for integration with the
//! existing C++ codebase.

use once_cell::sync::Lazy;
use std::sync::Mutex;

pub mod error;
pub mod ffi;
pub mod graphics;

pub use error::PlatformError;
pub use graphics::DisplayMode;

/// Platform version
pub const PLATFORM_VERSION: i32 = 1;

/// Platform state
pub(crate) static PLATFORM_STATE: Lazy<Mutex<Option<PlatformState>>> = Lazy::new(|| Mutex::new(None));

pub(crate) struct PlatformState {
    pub initialized: bool,
}

/// Result type for platform operations (FFI-compatible)
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PlatformResult {
    Success = 0,
    AlreadyInitialized = 1,
    NotInitialized = 2,
    InitFailed = 3,
}

// Re-export FFI functions at crate root for cbindgen
pub use ffi::*;

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
