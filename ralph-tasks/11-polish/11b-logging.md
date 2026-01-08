# Task 11b: Logging Infrastructure

## Dependencies
- Task 11a (Configuration Paths) must be complete
- The `log` crate is already a dependency

## Context
This task creates a comprehensive logging system that writes to both the console and a file. Good logging is essential for:
- Debugging issues users encounter
- Tracking down race conditions in multiplayer
- Performance profiling
- Post-mortem crash analysis

The logging system uses the existing `log` crate facade with a custom logger implementation that:
- Writes to stderr for immediate console output
- Writes to a file in the logs directory (from Task 11a)
- Supports log levels (Error, Warn, Info, Debug, Trace)
- Includes timestamps for correlation
- Provides FFI exports for C++ code

## Objective
Create a logging module that:
1. Implements the `log::Log` trait for custom logging
2. Writes to both stderr and a log file
3. Uses the logs directory from the config module
4. Supports configurable log levels
5. Provides FFI exports for C++ code
6. Is thread-safe and performant

## Deliverables
- [ ] `platform/src/logging/mod.rs` - Logging implementation
- [ ] FFI exports: `Platform_Log_Init`, `Platform_Log_Shutdown`, `Platform_Log`, `Platform_Log_SetLevel`, `Platform_Log_Flush`
- [ ] Header contains FFI exports
- [ ] Unit tests pass
- [ ] Verification passes

## Technical Background

### Log Levels
| Level | Value | Usage |
|-------|-------|-------|
| Error | 0 | Unrecoverable errors, crashes |
| Warn | 1 | Recoverable issues, degraded mode |
| Info | 2 | Major events, state changes |
| Debug | 3 | Detailed operational info |
| Trace | 4 | Very detailed debugging |

### Log Format
```
[2024-01-07 15:30:45.123] [INFO] [network::client] Connected to server 192.168.1.1:5000
[2024-01-07 15:30:45.456] [DEBUG] [graphics::surface] Allocated surface 640x400
```

### Thread Safety
- Use `Mutex` for file handle access
- Log calls from any thread are serialized
- Keep critical sections short to minimize contention

### Log File Rotation
For this initial implementation:
- Create new log file each session (timestamped filename)
- Don't rotate during session
- Future enhancement: size-based rotation

## Files to Create/Modify

### platform/src/logging/mod.rs (NEW)
```rust
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
        .map_err(|_| "Logging already initialized")?;

    // Create and register logger
    let logger = GameLogger::new(true, true);

    LOGGER
        .set(logger)
        .map_err(|_| "Logger already initialized")?;

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

/// Log a message
///
/// # Parameters
/// - `level`: Log level (0=Error, 1=Warn, 2=Info, 3=Debug, 4=Trace)
/// - `message`: Null-terminated UTF-8 message string
///
/// # Safety
/// - `message` must be a valid null-terminated C string
/// - `message` must point to valid memory for the duration of the call
#[no_mangle]
pub unsafe extern "C" fn Platform_Log(level: i32, message: *const c_char) {
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
            Platform_Log(2, std::ptr::null());
            Platform_Log_Info(std::ptr::null(), std::ptr::null());
        }
    }

    #[test]
    fn test_ffi_log_with_message() {
        let msg = std::ffi::CString::new("Test FFI log message").unwrap();
        unsafe {
            Platform_Log(2, msg.as_ptr());
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
```

### platform/src/lib.rs (MODIFY)
Add the logging module after the config module:
```rust
pub mod logging;
```

And add re-exports:
```rust
// Re-export logging FFI functions
pub use logging::{
    Platform_Log_Init,
    Platform_Log_Shutdown,
    Platform_Log,
    Platform_Log_Info,
    Platform_Log_SetLevel,
    Platform_Log_GetLevel,
    Platform_Log_Flush,
};
```

## Verification Command
```bash
ralph-tasks/verify/check-logging.sh
```

## Verification Script
Create `ralph-tasks/verify/check-logging.sh`:
```bash
#!/bin/bash
# Verification script for logging infrastructure

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Logging Infrastructure ==="

cd "$ROOT_DIR"

# Step 1: Check module file exists
echo "Step 1: Checking module file exists..."
if [ ! -f "platform/src/logging/mod.rs" ]; then
    # Check if it might be in a different location
    if [ -f "platform/src/logging.rs" ]; then
        echo "  Found at platform/src/logging.rs"
    else
        echo "VERIFY_FAILED: logging module not found"
        exit 1
    fi
fi
echo "  Logging module exists"

# Step 2: Check lib.rs has logging module
echo "Step 2: Checking lib.rs includes logging module..."
if ! grep -q "pub mod logging" platform/src/lib.rs; then
    echo "VERIFY_FAILED: logging module not declared in lib.rs"
    exit 1
fi
echo "  Logging module declared in lib.rs"

# Step 3: Build the platform crate
echo "Step 3: Building platform crate..."
if ! cargo build --manifest-path platform/Cargo.toml --release 2>&1; then
    echo "VERIFY_FAILED: cargo build failed"
    exit 1
fi
echo "  Build successful"

# Step 4: Run logging tests
echo "Step 4: Running logging tests..."
if ! cargo test --manifest-path platform/Cargo.toml --release -- logging --test-threads=1 2>&1; then
    echo "VERIFY_FAILED: logging tests failed"
    exit 1
fi
echo "  Tests passed"

# Step 5: Generate and check header
echo "Step 5: Checking FFI exports in header..."
if ! cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1; then
    echo "VERIFY_FAILED: cbindgen header generation failed"
    exit 1
fi

# Check for logging FFI exports
for func in Platform_Log_Init Platform_Log_Shutdown Platform_Log Platform_Log_SetLevel Platform_Log_Flush; do
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
1. Create directory `platform/src/logging/` (or file `platform/src/logging.rs`)
2. Create `platform/src/logging/mod.rs` with logging implementation
3. Add `pub mod logging;` to `platform/src/lib.rs`
4. Add FFI re-exports to `platform/src/lib.rs`
5. Run `cargo build --manifest-path platform/Cargo.toml`
6. Run `cargo test --manifest-path platform/Cargo.toml -- logging --test-threads=1`
7. Run `cargo build --manifest-path platform/Cargo.toml --features cbindgen-run`
8. Verify header contains exports
9. Run verification script

## Success Criteria
- Logger initializes successfully
- Log messages appear on stderr
- Log file is created in logs directory
- FFI functions work from C code
- All unit tests pass
- Verification script exits with 0

## Common Issues

### "Logging already initialized"
The logger uses OnceCell and can only be initialized once. In tests, use `--test-threads=1`.

### "Failed to create log directory"
Ensure Task 11a (config paths) is complete and `ensure_directories()` works.

### "Logger not set"
The `log::set_logger` must be called. Ensure initialization completes successfully.

### Missing log messages
Check that the log level is set appropriately. Default is Info (2).

### Log file empty
Call `Platform_Log_Flush()` or `flush()` before checking the file. Buffered I/O may not be written immediately.

## Integration Notes

### Existing log usage
The codebase already uses `log::info!()`, `log::debug!()`, etc. These will automatically work once the logger is initialized.

### C++ Usage
```cpp
#include "platform.h"

void init_game() {
    Platform_Log_Init();
    Platform_Log_SetLevel(3); // Debug level

    Platform_Log(2, "Game initializing...");
    // or
    Platform_Log_Info("game", "Starting main loop");
}

void shutdown_game() {
    Platform_Log_Flush();
    Platform_Log_Shutdown();
}
```

## Completion Promise
When verification passes, output:
```
<promise>TASK_11B_COMPLETE</promise>
```

## Escape Hatch
If stuck after 10 iterations:
- Document what's blocking in `ralph-tasks/blocked/11b.md`
- List attempted approaches
- Output: `<promise>TASK_11B_BLOCKED</promise>`

## Max Iterations
10
