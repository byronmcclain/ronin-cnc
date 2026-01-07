# Task 07b: File Operations

## Dependencies
- Task 07a must be complete (Path Utilities)

## Context
The file operations module provides a cross-platform file I/O abstraction replacing Windows file APIs. It wraps Rust's std::fs with an interface compatible with the original game's file handling.

## Objective
Create the PlatformFile struct with methods for opening, reading, writing, and seeking. Include file mode enums and proper error handling.

## Deliverables
- [ ] `platform/src/files/mod.rs` - Complete file operations module
- [ ] `FileMode` and `SeekOrigin` enums
- [ ] `PlatformFile` struct with open/read/write/seek/close
- [ ] Utility functions: exists, get_file_size, copy_file, delete_file
- [ ] Verification passes

## Files to Create/Modify

### platform/src/files/mod.rs (REPLACE/EXTEND)
```rust
//! File system abstraction
//!
//! Provides cross-platform file I/O with Windows path compatibility.

pub mod path;

use crate::error::PlatformError;
use std::fs::{self, File, OpenOptions};
use std::io::{Read, Write, Seek, SeekFrom};
use std::path::PathBuf;
use once_cell::sync::Lazy;
use std::sync::Mutex;

/// File open mode
#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub enum FileMode {
    Read = 1,
    Write = 2,
    ReadWrite = 3,
    Append = 4,
}

/// Seek origin
#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub enum SeekOrigin {
    Start = 0,
    Current = 1,
    End = 2,
}

/// Global paths for base and preferences directories
static PATHS: Lazy<Mutex<Paths>> = Lazy::new(|| Mutex::new(Paths::default()));

#[derive(Default)]
struct Paths {
    base_path: String,
    pref_path: String,
}

/// Platform file handle
pub struct PlatformFile {
    file: File,
    path: PathBuf,
    mode: FileMode,
    size: i64,
}

impl PlatformFile {
    /// Open a file with the specified mode
    pub fn open(filename: &str, mode: FileMode) -> Result<Self, PlatformError> {
        let normalized = path::normalize(filename);

        // For reading, try to find the file (case-insensitive)
        let file_path = if matches!(mode, FileMode::Read | FileMode::ReadWrite) {
            path::find_file(&normalized)
                .ok_or_else(|| PlatformError::Io(std::io::Error::new(
                    std::io::ErrorKind::NotFound,
                    format!("File not found: {}", filename)
                )))?
        } else {
            PathBuf::from(&normalized)
        };

        let file = match mode {
            FileMode::Read => File::open(&file_path)?,
            FileMode::Write => File::create(&file_path)?,
            FileMode::ReadWrite => OpenOptions::new()
                .read(true)
                .write(true)
                .create(true)
                .open(&file_path)?,
            FileMode::Append => OpenOptions::new()
                .append(true)
                .create(true)
                .open(&file_path)?,
        };

        let size = file.metadata().map(|m| m.len() as i64).unwrap_or(0);

        Ok(Self {
            file,
            path: file_path,
            mode,
            size,
        })
    }

    /// Read bytes from file into buffer
    /// Returns number of bytes read
    pub fn read(&mut self, buffer: &mut [u8]) -> Result<usize, PlatformError> {
        Ok(self.file.read(buffer)?)
    }

    /// Read entire file contents
    pub fn read_all(&mut self) -> Result<Vec<u8>, PlatformError> {
        let mut buffer = Vec::new();
        self.file.read_to_end(&mut buffer)?;
        Ok(buffer)
    }

    /// Write bytes to file
    /// Returns number of bytes written
    pub fn write(&mut self, buffer: &[u8]) -> Result<usize, PlatformError> {
        Ok(self.file.write(buffer)?)
    }

    /// Seek to position in file
    pub fn seek(&mut self, offset: i64, origin: SeekOrigin) -> Result<u64, PlatformError> {
        let pos = match origin {
            SeekOrigin::Start => SeekFrom::Start(offset as u64),
            SeekOrigin::Current => SeekFrom::Current(offset),
            SeekOrigin::End => SeekFrom::End(offset),
        };
        Ok(self.file.seek(pos)?)
    }

    /// Get current position in file
    pub fn tell(&mut self) -> Result<u64, PlatformError> {
        Ok(self.file.stream_position()?)
    }

    /// Get file size
    pub fn size(&self) -> i64 {
        self.size
    }

    /// Check if at end of file
    pub fn eof(&mut self) -> bool {
        if let Ok(pos) = self.tell() {
            pos >= self.size as u64
        } else {
            true
        }
    }

    /// Flush buffers to disk
    pub fn flush(&mut self) -> Result<(), PlatformError> {
        Ok(self.file.flush()?)
    }

    /// Get the file path
    pub fn path(&self) -> &PathBuf {
        &self.path
    }

    /// Get the file mode
    pub fn mode(&self) -> FileMode {
        self.mode
    }
}

// =============================================================================
// Path Configuration
// =============================================================================

/// Initialize file system paths
pub fn init(org_name: &str, app_name: &str) {
    if let Ok(mut paths) = PATHS.lock() {
        // Base path (executable directory or current directory)
        paths.base_path = std::env::current_exe()
            .ok()
            .and_then(|p| p.parent().map(|p| p.to_string_lossy().to_string()))
            .unwrap_or_else(|| ".".to_string());

        // Preferences path (~/Library/Application Support/AppName on macOS)
        #[cfg(target_os = "macos")]
        {
            if let Some(home) = std::env::var_os("HOME") {
                let pref_dir = PathBuf::from(home)
                    .join("Library")
                    .join("Application Support")
                    .join(app_name);

                // Create if doesn't exist
                let _ = fs::create_dir_all(&pref_dir);

                paths.pref_path = pref_dir.to_string_lossy().to_string();
            }
        }

        #[cfg(not(target_os = "macos"))]
        {
            // Fallback for other platforms
            paths.pref_path = paths.base_path.clone();
        }

        let _ = org_name; // Suppress unused warning
    }
}

/// Get base path (executable directory)
pub fn get_base_path() -> String {
    PATHS.lock()
        .map(|p| p.base_path.clone())
        .unwrap_or_else(|_| ".".to_string())
}

/// Get preferences path (user data directory)
pub fn get_pref_path() -> String {
    PATHS.lock()
        .map(|p| p.pref_path.clone())
        .unwrap_or_else(|_| ".".to_string())
}

// =============================================================================
// Utility Functions
// =============================================================================

/// Check if a file exists (with case-insensitive lookup)
pub fn exists(path_str: &str) -> bool {
    path::find_file(path_str).is_some()
}

/// Check if path is a directory
pub fn is_directory(path_str: &str) -> bool {
    let normalized = path::normalize(path_str);
    PathBuf::from(&normalized).is_dir()
}

/// Create a directory (and all parent directories)
pub fn create_directory(path_str: &str) -> Result<(), PlatformError> {
    let normalized = path::normalize(path_str);
    fs::create_dir_all(&normalized)?;
    Ok(())
}

/// Delete a file
pub fn delete_file(path_str: &str) -> Result<(), PlatformError> {
    if let Some(file_path) = path::find_file(path_str) {
        fs::remove_file(file_path)?;
    }
    Ok(())
}

/// Delete a directory (and all contents)
pub fn delete_directory(path_str: &str) -> Result<(), PlatformError> {
    let normalized = path::normalize(path_str);
    fs::remove_dir_all(&normalized)?;
    Ok(())
}

/// Copy a file
pub fn copy_file(src: &str, dst: &str) -> Result<u64, PlatformError> {
    let src_path = path::find_file(src)
        .ok_or_else(|| PlatformError::Io(std::io::Error::new(
            std::io::ErrorKind::NotFound,
            format!("Source file not found: {}", src)
        )))?;
    let dst_path = PathBuf::from(path::normalize(dst));
    Ok(fs::copy(src_path, dst_path)?)
}

/// Get file size without opening
pub fn get_file_size(path_str: &str) -> Result<i64, PlatformError> {
    if let Some(file_path) = path::find_file(path_str) {
        let meta = fs::metadata(file_path)?;
        Ok(meta.len() as i64)
    } else {
        Err(PlatformError::Io(std::io::Error::new(
            std::io::ErrorKind::NotFound,
            format!("File not found: {}", path_str)
        )))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Write as IoWrite;

    #[test]
    fn test_file_write_read() {
        let test_path = "/tmp/platform_file_test.txt";
        let test_data = b"Hello, Platform!";

        // Write
        {
            let mut file = PlatformFile::open(test_path, FileMode::Write).unwrap();
            file.write(test_data).unwrap();
        }

        // Read
        {
            let mut file = PlatformFile::open(test_path, FileMode::Read).unwrap();
            let mut buffer = vec![0u8; test_data.len()];
            let bytes_read = file.read(&mut buffer).unwrap();
            assert_eq!(bytes_read, test_data.len());
            assert_eq!(&buffer, test_data);
        }

        // Cleanup
        let _ = fs::remove_file(test_path);
    }

    #[test]
    fn test_file_seek() {
        let test_path = "/tmp/platform_seek_test.txt";

        // Create test file
        {
            let mut f = fs::File::create(test_path).unwrap();
            f.write_all(b"0123456789").unwrap();
        }

        let mut file = PlatformFile::open(test_path, FileMode::Read).unwrap();

        // Seek from start
        file.seek(5, SeekOrigin::Start).unwrap();
        assert_eq!(file.tell().unwrap(), 5);

        // Seek from current
        file.seek(2, SeekOrigin::Current).unwrap();
        assert_eq!(file.tell().unwrap(), 7);

        // Seek from end
        file.seek(-3, SeekOrigin::End).unwrap();
        assert_eq!(file.tell().unwrap(), 7);

        // Cleanup
        let _ = fs::remove_file(test_path);
    }
}
```

## Verification Command
```bash
cd /Users/ooartist/Src/Ronin_CnC && \
cargo build --manifest-path platform/Cargo.toml 2>&1 && \
cargo test --manifest-path platform/Cargo.toml -- files 2>&1 && \
grep -q "pub struct PlatformFile" platform/src/files/mod.rs && \
grep -q "pub fn open" platform/src/files/mod.rs && \
grep -q "pub fn read" platform/src/files/mod.rs && \
grep -q "pub fn write" platform/src/files/mod.rs && \
grep -q "pub fn seek" platform/src/files/mod.rs && \
echo "VERIFY_SUCCESS"
```

## Implementation Steps
1. Update `platform/src/files/mod.rs` with complete file operations
2. Define `FileMode` and `SeekOrigin` enums with C-compatible repr
3. Implement `PlatformFile` struct with file handle wrapper
4. Implement `open()` with case-insensitive file lookup for reads
5. Implement `read()`, `write()`, `seek()`, `tell()`
6. Add `flush()`, `eof()`, `size()` helper methods
7. Implement path configuration functions (init, get_base_path, get_pref_path)
8. Implement utility functions (exists, is_directory, create_directory, etc.)
9. Run tests and verify

## Completion Promise
When verification passes, output:
```
<promise>TASK_07B_COMPLETE</promise>
```

Then proceed to **Task 07c: Directory Operations**.

## Escape Hatch
If stuck after 12 iterations:
- Document what's blocking in `ralph-tasks/blocked/07b.md`
- List attempted approaches
- Output: `<promise>TASK_07B_BLOCKED</promise>`

## Max Iterations
12
