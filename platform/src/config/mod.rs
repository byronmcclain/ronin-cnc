//! Configuration and path management for Red Alert
//!
//! Provides standard macOS paths for:
//! - User configuration (settings, preferences)
//! - Save games
//! - Log files
//! - Cached data
//! - Game data assets

pub mod paths;

pub use paths::{
    get_config_path,
    get_saves_path,
    get_log_path,
    get_cache_path,
    get_data_path,
    ensure_directories,
    ConfigPaths,
};

// Re-export FFI functions for cbindgen
pub use paths::{
    Platform_GetConfigPath,
    Platform_GetSavesPath,
    Platform_GetLogPath,
    Platform_GetCachePath,
    Platform_GetDataPath,
    Platform_EnsureDirectories,
};
