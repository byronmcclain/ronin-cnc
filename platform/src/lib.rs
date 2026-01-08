//! Red Alert Platform Layer
//!
//! Provides cross-platform abstractions for the Red Alert macOS port.
//! This library exposes a C-compatible FFI for integration with the
//! existing C++ codebase.

use once_cell::sync::Lazy;
use std::sync::Mutex;

pub mod app;
pub mod assets;
pub mod audio;
pub mod blit;
pub mod compression;
pub mod config;
pub mod error;
pub mod ffi;
pub mod files;
pub mod graphics;
pub mod input;
pub mod logging;
pub mod memory;
pub mod network;
pub mod perf;
pub mod timer;
pub mod util;

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

// Re-export config FFI functions
pub use config::{
    Platform_GetConfigPath,
    Platform_GetSavesPath,
    Platform_GetLogPath,
    Platform_GetCachePath,
    Platform_GetDataPath,
    Platform_EnsureDirectories,
};

// Re-export logging FFI functions
pub use logging::{
    Platform_Log_Init,
    Platform_Log_Shutdown,
    Platform_LogMessage,
    Platform_Log_Info,
    Platform_Log_SetLevel,
    Platform_Log_GetLevel,
    Platform_Log_Flush,
};

// Re-export display FFI functions
pub use graphics::{
    Platform_IsRetinaDisplay,
    Platform_GetDisplayScale,
    Platform_ToggleFullscreen,
    Platform_SetFullscreen,
    Platform_IsFullscreen,
    Platform_GetWindowSize,
    Platform_GetDrawableSize,
    Platform_GetFullscreenState,
};

// Re-export app lifecycle FFI functions
pub use app::{
    Platform_GetAppState,
    Platform_IsAppActive,
    Platform_IsAppBackground,
    Platform_RequestQuit,
    Platform_ShouldQuit,
    Platform_ClearQuitRequest,
};

// Re-export perf/timing FFI functions
pub use perf::{
    Platform_GetFPS,
    Platform_GetFrameTime,
    Platform_FrameStart,
    Platform_FrameEnd,
    Platform_GetFrameCount,
};

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
