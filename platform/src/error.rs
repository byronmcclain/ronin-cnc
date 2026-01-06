//! Platform error types and FFI safety utilities

use std::sync::Mutex;
use once_cell::sync::Lazy;

/// Platform error enum
#[derive(Debug, Clone)]
pub enum PlatformError {
    Sdl(String),
    Audio(String),
    Graphics(String),
    Io(String),
    Network(String),
    AlreadyInitialized,
    NotInitialized,
    InvalidParameter(String),
    NullPointer,
}

impl std::fmt::Display for PlatformError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            PlatformError::Sdl(s) => write!(f, "SDL error: {}", s),
            PlatformError::Audio(s) => write!(f, "Audio error: {}", s),
            PlatformError::Graphics(s) => write!(f, "Graphics error: {}", s),
            PlatformError::Io(s) => write!(f, "IO error: {}", s),
            PlatformError::Network(s) => write!(f, "Network error: {}", s),
            PlatformError::AlreadyInitialized => write!(f, "Already initialized"),
            PlatformError::NotInitialized => write!(f, "Not initialized"),
            PlatformError::InvalidParameter(s) => write!(f, "Invalid parameter: {}", s),
            PlatformError::NullPointer => write!(f, "Null pointer"),
        }
    }
}

impl std::error::Error for PlatformError {}

impl From<std::io::Error> for PlatformError {
    fn from(e: std::io::Error) -> Self {
        PlatformError::Io(e.to_string())
    }
}

/// Thread-safe last error storage
static LAST_ERROR: Lazy<Mutex<String>> = Lazy::new(|| Mutex::new(String::new()));

/// Set the last error message
pub fn set_error(msg: impl Into<String>) {
    if let Ok(mut guard) = LAST_ERROR.lock() {
        *guard = msg.into();
    }
}

/// Get the last error message
pub fn get_error() -> String {
    LAST_ERROR.lock().map(|g| g.clone()).unwrap_or_default()
}

/// Clear the last error
pub fn clear_error() {
    if let Ok(mut guard) = LAST_ERROR.lock() {
        guard.clear();
    }
}

/// Convert Result to C error code (0 = success, negative = error)
pub fn result_to_code<T>(result: Result<T, PlatformError>) -> i32 {
    match result {
        Ok(_) => 0,
        Err(e) => {
            set_error(e.to_string());
            match e {
                PlatformError::AlreadyInitialized => -1,
                PlatformError::NotInitialized => -2,
                PlatformError::InvalidParameter(_) => -3,
                PlatformError::NullPointer => -4,
                PlatformError::Sdl(_) => -10,
                PlatformError::Graphics(_) => -11,
                PlatformError::Audio(_) => -12,
                PlatformError::Io(_) => -13,
                PlatformError::Network(_) => -14,
            }
        }
    }
}

/// Catch panics at FFI boundary and convert to error code
pub fn catch_panic<F, T>(f: F) -> i32
where
    F: FnOnce() -> Result<T, PlatformError> + std::panic::UnwindSafe,
{
    match std::panic::catch_unwind(f) {
        Ok(result) => result_to_code(result),
        Err(payload) => {
            let msg = if let Some(s) = payload.downcast_ref::<&str>() {
                format!("Panic: {}", s)
            } else if let Some(s) = payload.downcast_ref::<String>() {
                format!("Panic: {}", s)
            } else {
                "Panic: unknown error".to_string()
            };
            set_error(msg);
            -999
        }
    }
}

/// Catch panics and return a default value
pub fn catch_panic_or<F, T>(default: T, f: F) -> T
where
    F: FnOnce() -> T + std::panic::UnwindSafe,
{
    std::panic::catch_unwind(f).unwrap_or(default)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_error_to_code() {
        assert_eq!(result_to_code::<()>(Ok(())), 0);
        assert_eq!(result_to_code::<()>(Err(PlatformError::AlreadyInitialized)), -1);
        assert_eq!(result_to_code::<()>(Err(PlatformError::NotInitialized)), -2);
    }

    #[test]
    fn test_error_message() {
        clear_error();
        set_error("test error");
        assert_eq!(get_error(), "test error");
        clear_error();
        assert_eq!(get_error(), "");
    }

    #[test]
    fn test_catch_panic() {
        let result = catch_panic(|| -> Result<(), PlatformError> {
            panic!("test panic");
        });
        assert_eq!(result, -999);
        assert!(get_error().contains("Panic"));
    }

    #[test]
    fn test_catch_panic_success() {
        let result = catch_panic(|| -> Result<i32, PlatformError> {
            Ok(42)
        });
        assert_eq!(result, 0);
    }
}
