//! Path discovery and management for macOS
//!
//! Standard macOS locations:
//! - Config: ~/Library/Application Support/RedAlert/
//! - Saves:  ~/Library/Application Support/RedAlert/saves/
//! - Logs:   ~/Library/Application Support/RedAlert/logs/
//! - Cache:  ~/Library/Caches/RedAlert/

use std::path::PathBuf;
use std::os::raw::c_char;

/// Application identifier used in paths
pub const APP_NAME: &str = "RedAlert";

/// All configuration paths in one struct
#[derive(Debug, Clone)]
pub struct ConfigPaths {
    /// Root config directory
    pub config: PathBuf,
    /// Save games directory
    pub saves: PathBuf,
    /// Log files directory
    pub logs: PathBuf,
    /// Cache directory
    pub cache: PathBuf,
    /// Game data directory
    pub data: PathBuf,
}

impl ConfigPaths {
    /// Create ConfigPaths with all standard locations
    pub fn new() -> Self {
        Self {
            config: get_config_path(),
            saves: get_saves_path(),
            logs: get_log_path(),
            cache: get_cache_path(),
            data: get_data_path(),
        }
    }
}

impl Default for ConfigPaths {
    fn default() -> Self {
        Self::new()
    }
}

// =============================================================================
// Path Discovery Functions
// =============================================================================

/// Get the home directory
fn get_home_dir() -> Option<PathBuf> {
    // Try $HOME environment variable first
    if let Ok(home) = std::env::var("HOME") {
        let path = PathBuf::from(home);
        if path.is_dir() {
            return Some(path);
        }
    }

    // On macOS, we could also try NSHomeDirectory() via objc
    // but $HOME is standard and reliable
    None
}

/// Get the base Application Support directory for this app
/// Returns: ~/Library/Application Support/RedAlert/
/// Fallback: ./RedAlert/ if HOME not available
pub fn get_config_path() -> PathBuf {
    if let Some(home) = get_home_dir() {
        home.join("Library")
            .join("Application Support")
            .join(APP_NAME)
    } else {
        // Fallback to current directory
        PathBuf::from(".").join(APP_NAME)
    }
}

/// Get the saves directory
/// Returns: ~/Library/Application Support/RedAlert/saves/
pub fn get_saves_path() -> PathBuf {
    get_config_path().join("saves")
}

/// Get the log files directory
/// Returns: ~/Library/Application Support/RedAlert/logs/
pub fn get_log_path() -> PathBuf {
    get_config_path().join("logs")
}

/// Get the cache directory
/// Returns: ~/Library/Caches/RedAlert/
/// Fallback: ./RedAlert/cache/ if HOME not available
pub fn get_cache_path() -> PathBuf {
    if let Some(home) = get_home_dir() {
        home.join("Library")
            .join("Caches")
            .join(APP_NAME)
    } else {
        PathBuf::from(".").join(APP_NAME).join("cache")
    }
}

/// Get the game data directory
///
/// Search order:
/// 1. App bundle Resources/data/ (for packaged app)
/// 2. ./data/ (for development)
/// 3. ../data/ (for build directory)
///
/// Returns the first path that exists, or ./data/ as default
pub fn get_data_path() -> PathBuf {
    // Check for bundled app Resources
    if let Ok(exe_path) = std::env::current_exe() {
        // App bundle: RedAlert.app/Contents/MacOS/RedAlert
        // Resources:  RedAlert.app/Contents/Resources/data/
        let bundle_resources = exe_path
            .parent() // MacOS/
            .and_then(|p| p.parent()) // Contents/
            .map(|p| p.join("Resources").join("data"));

        if let Some(ref path) = bundle_resources {
            if path.is_dir() {
                return path.clone();
            }
        }
    }

    // Check ./data/
    let local_data = PathBuf::from("data");
    if local_data.is_dir() {
        return local_data;
    }

    // Check ../data/ (common when running from build/)
    let parent_data = PathBuf::from("..").join("data");
    if parent_data.is_dir() {
        return parent_data;
    }

    // Default to ./data/ even if it doesn't exist yet
    local_data
}

/// Ensure all configuration directories exist
///
/// Creates:
/// - Config directory
/// - Saves directory
/// - Logs directory
/// - Cache directory
///
/// Returns Ok(()) if all directories exist or were created successfully.
pub fn ensure_directories() -> std::io::Result<()> {
    std::fs::create_dir_all(get_config_path())?;
    std::fs::create_dir_all(get_saves_path())?;
    std::fs::create_dir_all(get_log_path())?;
    std::fs::create_dir_all(get_cache_path())?;
    Ok(())
}

// =============================================================================
// FFI Helper Functions
// =============================================================================

/// Copy a path string to a C buffer
///
/// # Safety
/// - buffer must point to valid memory of at least buffer_size bytes
/// - buffer_size must be > 0 for any useful output
///
/// # Returns
/// - Number of bytes written (excluding null terminator) on success
/// - -1 if buffer is null or buffer_size <= 0
unsafe fn copy_path_to_buffer(
    path: &PathBuf,
    buffer: *mut c_char,
    buffer_size: i32,
) -> i32 {
    if buffer.is_null() || buffer_size <= 0 {
        return -1;
    }

    let path_str = path.to_string_lossy();
    let bytes = path_str.as_bytes();
    let max_copy = (buffer_size - 1) as usize; // Reserve space for null terminator
    let copy_len = bytes.len().min(max_copy);

    // Copy the path bytes
    std::ptr::copy_nonoverlapping(
        bytes.as_ptr(),
        buffer as *mut u8,
        copy_len,
    );

    // Null terminate
    *buffer.add(copy_len) = 0;

    copy_len as i32
}

// =============================================================================
// FFI Exports
// =============================================================================

/// Get the configuration directory path
///
/// Copies the path to the provided buffer.
///
/// # Parameters
/// - `buffer`: Output buffer for null-terminated UTF-8 path string
/// - `buffer_size`: Size of buffer in bytes
///
/// # Returns
/// - Number of bytes written (excluding null) on success
/// - -1 on error (null buffer or insufficient size)
///
/// # Example Path
/// `/Users/username/Library/Application Support/RedAlert`
#[no_mangle]
pub unsafe extern "C" fn Platform_GetConfigPath(
    buffer: *mut c_char,
    buffer_size: i32,
) -> i32 {
    copy_path_to_buffer(&get_config_path(), buffer, buffer_size)
}

/// Get the saves directory path
///
/// # Parameters
/// - `buffer`: Output buffer for null-terminated UTF-8 path string
/// - `buffer_size`: Size of buffer in bytes
///
/// # Returns
/// - Number of bytes written (excluding null) on success
/// - -1 on error
///
/// # Example Path
/// `/Users/username/Library/Application Support/RedAlert/saves`
#[no_mangle]
pub unsafe extern "C" fn Platform_GetSavesPath(
    buffer: *mut c_char,
    buffer_size: i32,
) -> i32 {
    copy_path_to_buffer(&get_saves_path(), buffer, buffer_size)
}

/// Get the log files directory path
///
/// # Parameters
/// - `buffer`: Output buffer for null-terminated UTF-8 path string
/// - `buffer_size`: Size of buffer in bytes
///
/// # Returns
/// - Number of bytes written (excluding null) on success
/// - -1 on error
///
/// # Example Path
/// `/Users/username/Library/Application Support/RedAlert/logs`
#[no_mangle]
pub unsafe extern "C" fn Platform_GetLogPath(
    buffer: *mut c_char,
    buffer_size: i32,
) -> i32 {
    copy_path_to_buffer(&get_log_path(), buffer, buffer_size)
}

/// Get the cache directory path
///
/// # Parameters
/// - `buffer`: Output buffer for null-terminated UTF-8 path string
/// - `buffer_size`: Size of buffer in bytes
///
/// # Returns
/// - Number of bytes written (excluding null) on success
/// - -1 on error
///
/// # Example Path
/// `/Users/username/Library/Caches/RedAlert`
#[no_mangle]
pub unsafe extern "C" fn Platform_GetCachePath(
    buffer: *mut c_char,
    buffer_size: i32,
) -> i32 {
    copy_path_to_buffer(&get_cache_path(), buffer, buffer_size)
}

/// Get the game data directory path
///
/// Searches for data in standard locations and returns the first that exists.
///
/// # Parameters
/// - `buffer`: Output buffer for null-terminated UTF-8 path string
/// - `buffer_size`: Size of buffer in bytes
///
/// # Returns
/// - Number of bytes written (excluding null) on success
/// - -1 on error
///
/// # Search Order
/// 1. App bundle Resources/data/
/// 2. ./data/
/// 3. ../data/
#[no_mangle]
pub unsafe extern "C" fn Platform_GetDataPath(
    buffer: *mut c_char,
    buffer_size: i32,
) -> i32 {
    copy_path_to_buffer(&get_data_path(), buffer, buffer_size)
}

/// Ensure all configuration directories exist
///
/// Creates the config, saves, logs, and cache directories if they don't exist.
///
/// # Returns
/// - 0 on success (all directories exist or were created)
/// - -1 on error (failed to create one or more directories)
#[no_mangle]
pub extern "C" fn Platform_EnsureDirectories() -> i32 {
    match ensure_directories() {
        Ok(()) => {
            log::debug!("Configuration directories ensured");
            0
        }
        Err(e) => {
            log::error!("Failed to create configuration directories: {}", e);
            -1
        }
    }
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;
    use std::ffi::CStr;

    #[test]
    fn test_get_config_path_not_empty() {
        let path = get_config_path();
        assert!(!path.as_os_str().is_empty());
        // Should contain our app name
        assert!(path.to_string_lossy().contains(APP_NAME));
    }

    #[test]
    fn test_get_saves_path_is_under_config() {
        let config = get_config_path();
        let saves = get_saves_path();
        assert!(saves.starts_with(&config));
        assert!(saves.to_string_lossy().contains("saves"));
    }

    #[test]
    fn test_get_log_path_is_under_config() {
        let config = get_config_path();
        let logs = get_log_path();
        assert!(logs.starts_with(&config));
        assert!(logs.to_string_lossy().contains("logs"));
    }

    #[test]
    fn test_get_cache_path_not_empty() {
        let path = get_cache_path();
        assert!(!path.as_os_str().is_empty());
        assert!(path.to_string_lossy().contains(APP_NAME));
    }

    #[test]
    fn test_get_data_path_not_empty() {
        let path = get_data_path();
        assert!(!path.as_os_str().is_empty());
    }

    #[test]
    fn test_config_paths_struct() {
        let paths = ConfigPaths::new();
        assert!(!paths.config.as_os_str().is_empty());
        assert!(!paths.saves.as_os_str().is_empty());
        assert!(!paths.logs.as_os_str().is_empty());
        assert!(!paths.cache.as_os_str().is_empty());
        assert!(!paths.data.as_os_str().is_empty());
    }

    #[test]
    fn test_ensure_directories_creates_dirs() {
        // This test actually creates directories
        // They're in standard locations so this is safe
        let result = ensure_directories();
        assert!(result.is_ok());

        // Verify directories exist
        assert!(get_config_path().is_dir() || get_config_path().exists());
        assert!(get_saves_path().is_dir() || get_saves_path().exists());
        assert!(get_log_path().is_dir() || get_log_path().exists());
        assert!(get_cache_path().is_dir() || get_cache_path().exists());
    }

    #[test]
    fn test_ffi_get_config_path() {
        let mut buffer = [0i8; 512];
        let len = unsafe {
            Platform_GetConfigPath(buffer.as_mut_ptr(), buffer.len() as i32)
        };

        assert!(len > 0);

        // Convert to string and verify
        let path_str = unsafe {
            CStr::from_ptr(buffer.as_ptr()).to_string_lossy()
        };
        assert!(path_str.contains(APP_NAME));
    }

    #[test]
    fn test_ffi_null_buffer_returns_error() {
        let len = unsafe {
            Platform_GetConfigPath(std::ptr::null_mut(), 100)
        };
        assert_eq!(len, -1);
    }

    #[test]
    fn test_ffi_zero_size_returns_error() {
        let mut buffer = [0i8; 10];
        let len = unsafe {
            Platform_GetConfigPath(buffer.as_mut_ptr(), 0)
        };
        assert_eq!(len, -1);
    }

    #[test]
    fn test_ffi_small_buffer_truncates() {
        let mut buffer = [0i8; 10];
        let len = unsafe {
            Platform_GetConfigPath(buffer.as_mut_ptr(), 10)
        };

        // Should truncate to 9 chars + null
        assert!(len <= 9);
        assert!(len >= 0);

        // Should be null terminated
        let path_str = unsafe {
            CStr::from_ptr(buffer.as_ptr()).to_string_lossy()
        };
        assert_eq!(path_str.len(), len as usize);
    }

    #[test]
    fn test_ffi_ensure_directories() {
        let result = Platform_EnsureDirectories();
        assert_eq!(result, 0);
    }

    #[test]
    fn test_paths_use_library_on_macos() {
        // On macOS with valid HOME, should use Library paths
        if std::env::var("HOME").is_ok() {
            let config = get_config_path();
            let config_str = config.to_string_lossy();

            // Should be in Library/Application Support
            assert!(
                config_str.contains("Library/Application Support") ||
                config_str.contains("Library\\Application Support"),
                "Config path should be in Library/Application Support: {}",
                config_str
            );
        }
    }
}
