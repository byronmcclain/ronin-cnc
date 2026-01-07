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
                .ok_or_else(|| PlatformError::Io(format!("File not found: {}", filename)))?
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
        .ok_or_else(|| PlatformError::Io(format!("Source file not found: {}", src)))?;
    let dst_path = PathBuf::from(path::normalize(dst));
    Ok(fs::copy(src_path, dst_path)?)
}

/// Get file size without opening
pub fn get_file_size(path_str: &str) -> Result<i64, PlatformError> {
    if let Some(file_path) = path::find_file(path_str) {
        let meta = fs::metadata(file_path)?;
        Ok(meta.len() as i64)
    } else {
        Err(PlatformError::Io(format!("File not found: {}", path_str)))
    }
}

// =============================================================================
// Directory Operations
// =============================================================================

/// Directory entry information
#[repr(C)]
pub struct DirEntry {
    /// Filename (null-terminated, max 260 chars)
    pub name: [u8; 260],
    /// Is this entry a directory?
    pub is_directory: bool,
    /// File size in bytes (0 for directories)
    pub size: i64,
}

impl DirEntry {
    /// Get the filename as a string
    pub fn name_str(&self) -> &str {
        let len = self.name.iter().position(|&c| c == 0).unwrap_or(self.name.len());
        std::str::from_utf8(&self.name[..len]).unwrap_or("")
    }
}

/// Directory iterator
pub struct PlatformDir {
    iter: fs::ReadDir,
    path: PathBuf,
}

impl PlatformDir {
    /// Open a directory for iteration
    pub fn open(path_str: &str) -> Result<Self, PlatformError> {
        let normalized = path::normalize(path_str);
        let path = PathBuf::from(&normalized);

        if !path.exists() {
            return Err(PlatformError::Io(format!("Directory not found: {}", path_str)));
        }

        let iter = fs::read_dir(&path)?;
        Ok(Self { iter, path })
    }

    /// Read the next directory entry
    /// Returns None when no more entries
    pub fn read(&mut self) -> Option<DirEntry> {
        loop {
            let entry = self.iter.next()?.ok()?;
            let filename = entry.file_name();
            let name_str = filename.to_string_lossy();

            // Skip . and ..
            if name_str == "." || name_str == ".." {
                continue;
            }

            let mut name = [0u8; 260];
            let bytes = name_str.as_bytes();
            let len = bytes.len().min(259);
            name[..len].copy_from_slice(&bytes[..len]);

            let metadata = entry.metadata().ok();
            let is_directory = metadata.as_ref().map(|m| m.is_dir()).unwrap_or(false);
            let size = metadata.as_ref().map(|m| m.len() as i64).unwrap_or(0);

            return Some(DirEntry {
                name,
                is_directory,
                size,
            });
        }
    }

    /// Get the directory path
    pub fn path(&self) -> &PathBuf {
        &self.path
    }
}

/// List all files in a directory
pub fn list_files(dir_path: &str) -> Result<Vec<String>, PlatformError> {
    let mut dir = PlatformDir::open(dir_path)?;
    let mut files = Vec::new();

    while let Some(entry) = dir.read() {
        if !entry.is_directory {
            files.push(entry.name_str().to_string());
        }
    }

    Ok(files)
}

/// List all subdirectories in a directory
pub fn list_directories(dir_path: &str) -> Result<Vec<String>, PlatformError> {
    let mut dir = PlatformDir::open(dir_path)?;
    let mut dirs = Vec::new();

    while let Some(entry) = dir.read() {
        if entry.is_directory {
            dirs.push(entry.name_str().to_string());
        }
    }

    Ok(dirs)
}

/// Find files matching an extension (case-insensitive)
pub fn find_files_with_extension(dir_path: &str, ext: &str) -> Result<Vec<String>, PlatformError> {
    let mut dir = PlatformDir::open(dir_path)?;
    let mut files = Vec::new();
    let ext_lower = ext.to_lowercase();
    let ext_with_dot = if ext_lower.starts_with('.') {
        ext_lower
    } else {
        format!(".{}", ext_lower)
    };

    while let Some(entry) = dir.read() {
        if !entry.is_directory {
            let name = entry.name_str().to_lowercase();
            if name.ends_with(&ext_with_dot) {
                files.push(entry.name_str().to_string());
            }
        }
    }

    Ok(files)
}

/// Count entries in a directory
pub fn count_entries(dir_path: &str) -> Result<(usize, usize), PlatformError> {
    let mut dir = PlatformDir::open(dir_path)?;
    let mut file_count = 0;
    let mut dir_count = 0;

    while let Some(entry) = dir.read() {
        if entry.is_directory {
            dir_count += 1;
        } else {
            file_count += 1;
        }
    }

    Ok((file_count, dir_count))
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Write as IoWrite;

    #[test]
    fn test_file_write_read() {
        // Use unique filename with thread ID
        let test_path = format!("/tmp/platform_file_test_{}.txt", std::process::id());
        let test_data = b"Hello, Platform!";

        // Cleanup first in case of previous failure
        let _ = fs::remove_file(&test_path);

        // Write
        {
            let mut file = PlatformFile::open(&test_path, FileMode::Write).unwrap();
            file.write(test_data).unwrap();
        }

        // Read
        {
            let mut file = PlatformFile::open(&test_path, FileMode::Read).unwrap();
            let mut buffer = vec![0u8; test_data.len()];
            let bytes_read = file.read(&mut buffer).unwrap();
            assert_eq!(bytes_read, test_data.len());
            assert_eq!(&buffer, test_data);
        }

        // Cleanup
        let _ = fs::remove_file(&test_path);
    }

    #[test]
    fn test_file_seek() {
        // Use unique filename with thread ID
        let test_path = format!("/tmp/platform_seek_test_{}.txt", std::process::id());

        // Cleanup first
        let _ = fs::remove_file(&test_path);

        // Create test file
        {
            let mut f = fs::File::create(&test_path).unwrap();
            f.write_all(b"0123456789").unwrap();
        }

        let mut file = PlatformFile::open(&test_path, FileMode::Read).unwrap();

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
        let _ = fs::remove_file(&test_path);
    }

    #[test]
    fn test_dir_iteration() {
        // Create test directory with unique name
        let test_dir = format!("/tmp/platform_dir_test_{}", std::process::id());
        let _ = fs::remove_dir_all(&test_dir);
        fs::create_dir_all(&test_dir).unwrap();

        // Create test files
        fs::write(format!("{}/file1.txt", test_dir), "test1").unwrap();
        fs::write(format!("{}/file2.txt", test_dir), "test2").unwrap();
        fs::write(format!("{}/data.mix", test_dir), "mixdata").unwrap();
        fs::create_dir(format!("{}/subdir", test_dir)).unwrap();

        // Test iteration
        let mut dir = PlatformDir::open(&test_dir).unwrap();
        let mut count = 0;
        while let Some(_entry) = dir.read() {
            count += 1;
        }
        assert_eq!(count, 4); // 3 files + 1 directory

        // Test list_files
        let files = list_files(&test_dir).unwrap();
        assert_eq!(files.len(), 3);

        // Test list_directories
        let dirs = list_directories(&test_dir).unwrap();
        assert_eq!(dirs.len(), 1);

        // Test find_files_with_extension
        let txt_files = find_files_with_extension(&test_dir, "txt").unwrap();
        assert_eq!(txt_files.len(), 2);

        let mix_files = find_files_with_extension(&test_dir, ".MIX").unwrap();
        assert_eq!(mix_files.len(), 1);

        // Test count_entries
        let (fc, dc) = count_entries(&test_dir).unwrap();
        assert_eq!(fc, 3);
        assert_eq!(dc, 1);

        // Cleanup
        let _ = fs::remove_dir_all(&test_dir);
    }

    #[test]
    fn test_dir_entry_name() {
        let mut entry = DirEntry {
            name: [0u8; 260],
            is_directory: false,
            size: 100,
        };

        let test_name = b"testfile.txt";
        entry.name[..test_name.len()].copy_from_slice(test_name);

        assert_eq!(entry.name_str(), "testfile.txt");
    }
}
