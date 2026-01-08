//! Logging infrastructure for Red Alert
//!
//! Provides console and file logging with:
//! - Multiple log levels (Error, Warn, Info, Debug, Trace)
//! - Timestamped entries
//! - Thread-safe file output
//! - FFI exports for C++ code

use log::{Level, LevelFilter, Log, Metadata, Record};
use once_cell::sync::OnceCell;
use std::fs::File;
use std::io::{BufWriter, Write};
use std::os::raw::c_char;
use std::sync::atomic::{AtomicU8, Ordering};
use std::sync::Mutex;
use std::time::SystemTime;

/// Global logger instance
static LOGGER: OnceCell<GameLogger> = OnceCell::new();

/// Current log level (atomic for lock-free reads)
static LOG_LEVEL: AtomicU8 = AtomicU8::new(2); // Default: Info

/// Log file handle
static LOG_FILE: OnceCell<Mutex<Option<BufWriter<File>>>> = OnceCell::new();

/// The custom logger implementation
pub struct GameLogger {
    /// Whether to write to stderr
    console_enabled: bool,
    /// Whether to write to file
    file_enabled: bool,
}

impl GameLogger {
    /// Create a new logger
    fn new(console_enabled: bool, file_enabled: bool) -> Self {
        Self {
            console_enabled,
            file_enabled,
        }
    }
}

impl Log for GameLogger {
    fn enabled(&self, metadata: &Metadata) -> bool {
        let current_level = LOG_LEVEL.load(Ordering::Relaxed);
        metadata.level() as u8 <= current_level + 1
    }

    fn log(&self, record: &Record) {
        if !self.enabled(record.metadata()) {
            return;
        }

        // Format the log message
        let now = SystemTime::now()
            .duration_since(SystemTime::UNIX_EPOCH)
            .unwrap_or_default();
        let secs = now.as_secs();
        let millis = now.subsec_millis();

        // Calculate time components (simple UTC)
        let total_secs = secs % 86400; // seconds since midnight
        let hours = total_secs / 3600;
        let minutes = (total_secs % 3600) / 60;
        let seconds = total_secs % 60;

        let message = format!(
            "[{:02}:{:02}:{:02}.{:03}] [{:5}] [{}] {}\n",
            hours,
            minutes,
            seconds,
            millis,
            record.level(),
            record.target(),
            record.args()
        );

        // Write to console
        if self.console_enabled {
            // Use stderr so it doesn't interfere with stdout
            eprint!("{}", message);
        }

        // Write to file
        if self.file_enabled {
            if let Some(file_mutex) = LOG_FILE.get() {
                if let Ok(mut guard) = file_mutex.lock() {
                    if let Some(ref mut writer) = *guard {
                        let _ = writer.write_all(message.as_bytes());
                        // Don't flush every write for performance
                        // Flush periodically or on shutdown
                    }
                }
            }
        }
    }

    fn flush(&self) {
        if let Some(file_mutex) = LOG_FILE.get() {
            if let Ok(mut guard) = file_mutex.lock() {
                if let Some(ref mut writer) = *guard {
                    let _ = writer.flush();
                }
            }
        }
    }
}

// =============================================================================
// Initialization and Configuration
// =============================================================================

/// Initialize the logging system
///
/// Creates a log file in the logs directory with a timestamp in the filename.
/// Sets up both console (stderr) and file logging.
///
/// # Returns
/// - `Ok(())` if initialization succeeded
/// - `Err(String)` with error description if failed
pub fn init() -> Result<(), String> {
    // Ensure directories exist first
    if let Err(e) = crate::config::ensure_directories() {
        return Err(format!("Failed to create log directory: {}", e));
    }

    // Generate log filename with timestamp
    let now = SystemTime::now()
        .duration_since(SystemTime::UNIX_EPOCH)
        .unwrap_or_default()
        .as_secs();

    let log_path = crate::config::get_log_path()
        .join(format!("redalert_{}.log", now));

    // Open log file
    let file = File::create(&log_path)
        .map_err(|e| format!("Failed to create log file {:?}: {}", log_path, e))?;

    let writer = BufWriter::with_capacity(8192, file);

    // Store file handle
    LOG_FILE
        .set(Mutex::new(Some(writer)))
        .map_err(|_| "Logging already initialized".to_string())?;

    // Create and register logger
    let logger = GameLogger::new(true, true);

    LOGGER
        .set(logger)
        .map_err(|_| "Logger already initialized".to_string())?;

    // Set as global logger
    log::set_logger(LOGGER.get().unwrap())
        .map_err(|e| format!("Failed to set logger: {}", e))?;

    // Set max level based on current setting
    update_max_level();

    log::info!("Logging initialized to {:?}", log_path);
    Ok(())
}

/// Shutdown the logging system
///
/// Flushes any buffered data and closes the log file.
pub fn shutdown() {
    // Log shutdown message first
    log::info!("Logging system shutting down");

    // Flush and close file
    if let Some(file_mutex) = LOG_FILE.get() {
        if let Ok(mut guard) = file_mutex.lock() {
            if let Some(ref mut writer) = *guard {
                let _ = writer.flush();
            }
            *guard = None;
        }
    }
}

/// Set the log level
///
/// # Arguments
/// - `level`: 0=Error, 1=Warn, 2=Info, 3=Debug, 4=Trace
pub fn set_level(level: u8) {
    let level = level.min(4); // Clamp to valid range
    LOG_LEVEL.store(level, Ordering::Relaxed);
    update_max_level();
    log::debug!("Log level set to {}", level);
}

/// Get the current log level
pub fn get_level() -> u8 {
    LOG_LEVEL.load(Ordering::Relaxed)
}

/// Flush the log buffer to disk
pub fn flush() {
    if let Some(logger) = LOGGER.get() {
        logger.flush();
    }
}

/// Update the global max log level filter
fn update_max_level() {
    let level = LOG_LEVEL.load(Ordering::Relaxed);
    let filter = match level {
        0 => LevelFilter::Error,
        1 => LevelFilter::Warn,
        2 => LevelFilter::Info,
        3 => LevelFilter::Debug,
        _ => LevelFilter::Trace,
    };
    log::set_max_level(filter);
}

/// Log a message at the specified level
///
/// This is primarily for FFI use. Rust code should use the log macros.
pub fn log_message(level: u8, message: &str) {
    let log_level = match level {
        0 => Level::Error,
        1 => Level::Warn,
        2 => Level::Info,
        3 => Level::Debug,
        _ => Level::Trace,
    };

    log::log!(target: "game", log_level, "{}", message);
}

// =============================================================================
// FFI Exports
// =============================================================================

/// Initialize the logging system
///
/// Creates a timestamped log file in the standard logs directory.
/// Should be called early in program startup.
///
/// # Returns
/// - 0 on success
/// - -1 if already initialized
/// - -2 if failed to create log file
/// - -3 if failed to create log directory
#[no_mangle]
pub extern "C" fn Platform_Log_Init() -> i32 {
    match init() {
        Ok(()) => 0,
        Err(e) if e.contains("already") => -1,
        Err(e) if e.contains("directory") => {
            eprintln!("Log init error: {}", e);
            -3
        }
        Err(e) => {
            eprintln!("Log init error: {}", e);
            -2
        }
    }
}

/// Shutdown the logging system
///
/// Flushes buffered data and closes the log file.
/// Safe to call multiple times.
#[no_mangle]
pub extern "C" fn Platform_Log_Shutdown() {
    shutdown();
}

/// Log a message by level number
///
/// # Parameters
/// - `level`: Log level (0=Error, 1=Warn, 2=Info, 3=Debug, 4=Trace)
/// - `message`: Null-terminated UTF-8 message string
///
/// # Safety
/// - `message` must be a valid null-terminated C string
/// - `message` must point to valid memory for the duration of the call
///
/// Note: Use Platform_Log (from ffi module) for enum-based logging,
/// or this function for integer-based level specification.
#[no_mangle]
pub unsafe extern "C" fn Platform_LogMessage(level: i32, message: *const c_char) {
    if message.is_null() {
        return;
    }

    let msg = match std::ffi::CStr::from_ptr(message).to_str() {
        Ok(s) => s,
        Err(_) => return, // Invalid UTF-8, ignore
    };

    log_message(level as u8, msg);
}

/// Log a formatted message (convenience function)
///
/// Logs at INFO level with a module prefix.
///
/// # Parameters
/// - `module`: Null-terminated module name (e.g., "graphics", "network")
/// - `message`: Null-terminated message string
#[no_mangle]
pub unsafe extern "C" fn Platform_Log_Info(
    module: *const c_char,
    message: *const c_char,
) {
    if message.is_null() {
        return;
    }

    let msg = match std::ffi::CStr::from_ptr(message).to_str() {
        Ok(s) => s,
        Err(_) => return,
    };

    let target = if !module.is_null() {
        std::ffi::CStr::from_ptr(module)
            .to_str()
            .unwrap_or("game")
    } else {
        "game"
    };

    log::log!(target: target, Level::Info, "{}", msg);
}

/// Set the log level
///
/// # Parameters
/// - `level`: 0=Error (least verbose), 1=Warn, 2=Info, 3=Debug, 4=Trace (most verbose)
///
/// Default level is 2 (Info).
#[no_mangle]
pub extern "C" fn Platform_Log_SetLevel(level: i32) {
    set_level(level as u8);
}

/// Get the current log level
///
/// # Returns
/// Current log level (0-4)
#[no_mangle]
pub extern "C" fn Platform_Log_GetLevel() -> i32 {
    get_level() as i32
}

/// Flush log buffer to disk
///
/// Call this periodically or before program exit to ensure
/// all log messages are written to the file.
#[no_mangle]
pub extern "C" fn Platform_Log_Flush() {
    flush();
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    // Note: Tests manipulate global state and should run with --test-threads=1

    #[test]
    fn test_log_level_setting() {
        // Default is Info (2)
        set_level(2);
        assert_eq!(get_level(), 2);

        set_level(0);
        assert_eq!(get_level(), 0);

        set_level(4);
        assert_eq!(get_level(), 4);

        // Values > 4 should be clamped
        set_level(10);
        assert_eq!(get_level(), 4);

        // Reset to default
        set_level(2);
    }

    #[test]
    fn test_log_level_ffi() {
        Platform_Log_SetLevel(3);
        assert_eq!(Platform_Log_GetLevel(), 3);

        Platform_Log_SetLevel(1);
        assert_eq!(Platform_Log_GetLevel(), 1);

        // Reset
        Platform_Log_SetLevel(2);
    }

    #[test]
    fn test_log_message_levels() {
        // These won't panic even without initialization
        log_message(0, "Error message");
        log_message(1, "Warning message");
        log_message(2, "Info message");
        log_message(3, "Debug message");
        log_message(4, "Trace message");
    }

    #[test]
    fn test_ffi_log_null_safety() {
        // Should not crash with null message
        unsafe {
            Platform_LogMessage(2, std::ptr::null());
            Platform_Log_Info(std::ptr::null(), std::ptr::null());
        }
    }

    #[test]
    fn test_ffi_log_with_message() {
        let msg = std::ffi::CString::new("Test FFI log message").unwrap();
        unsafe {
            Platform_LogMessage(2, msg.as_ptr());
        }
        // Should not crash
    }

    #[test]
    fn test_ffi_log_with_module() {
        let module = std::ffi::CString::new("test").unwrap();
        let msg = std::ffi::CString::new("Test module log").unwrap();
        unsafe {
            Platform_Log_Info(module.as_ptr(), msg.as_ptr());
        }
        // Should not crash
    }

    #[test]
    fn test_flush_without_init() {
        // Should not crash even without initialization
        flush();
        Platform_Log_Flush();
    }

    #[test]
    fn test_shutdown_without_init() {
        // Should not crash
        shutdown();
        Platform_Log_Shutdown();
    }
}
