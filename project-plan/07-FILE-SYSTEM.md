# Plan 07: File System (Rust std::fs)

## Objective
Replace Windows file I/O with Rust's std::fs and std::path, implementing proper path handling for macOS including case-insensitive file lookup and MIX archive support.

## Current State Analysis

### Windows File I/O in Codebase

**Primary Files:**
- `WIN32LIB/RAWFILE/RAWFILE.CPP`
- `CODE/CCFILE.CPP`, `CODE/CCFILE.H`
- `CODE/CDFILE.CPP`, `CODE/CDFILE.H`
- `CODE/MIXFILE.CPP`, `CODE/MIXFILE.H`

**Key Issues:**
- Backslash path separators: `DATA\CONQUER.MIX`
- Drive letters: `D:\GAMES\RA\`
- Case-insensitive filenames (Windows)

## Rust Implementation

### 7.1 Path Utilities

File: `platform/src/files/path.rs`
```rust
//! Path normalization and utilities

use std::path::{Path, PathBuf};
use std::fs;

/// Normalize a Windows-style path for Unix
pub fn normalize(path: &str) -> String {
    let mut result = path.to_string();

    // Convert backslashes to forward slashes
    result = result.replace('\\', "/");

    // Remove drive letter if present (e.g., "C:")
    if result.len() >= 2 && result.chars().nth(1) == Some(':') {
        result = result[2..].to_string();
    }

    // Remove leading slash if path should be relative
    if result.starts_with('/') && !result.starts_with("//") {
        result = result[1..].to_string();
    }

    result
}

/// Find a file with case-insensitive matching
pub fn find_case_insensitive(dir: &Path, filename: &str) -> Option<PathBuf> {
    let lower_name = filename.to_lowercase();

    if let Ok(entries) = fs::read_dir(dir) {
        for entry in entries.flatten() {
            let entry_name = entry.file_name().to_string_lossy().to_string();
            if entry_name.to_lowercase() == lower_name {
                return Some(entry.path());
            }
        }
    }

    None
}

/// Search for a file, handling case-insensitivity and path normalization
pub fn find_file(path: &str) -> Option<PathBuf> {
    let normalized = normalize(path);
    let path = Path::new(&normalized);

    // First, try the exact path
    if path.exists() {
        return Some(path.to_path_buf());
    }

    // Try case-insensitive search
    if let Some(parent) = path.parent() {
        if let Some(filename) = path.file_name() {
            if let Some(found) = find_case_insensitive(parent, &filename.to_string_lossy()) {
                return Some(found);
            }
        }
    }

    // Try common data directories
    let data_dirs = ["data", "DATA", "Data"];
    for dir in &data_dirs {
        let try_path = PathBuf::from(dir).join(&normalized);
        if try_path.exists() {
            return Some(try_path);
        }
    }

    None
}

/// Get the path separator for the current platform
pub fn separator() -> char {
    std::path::MAIN_SEPARATOR
}

/// Join path components
pub fn join(base: &str, path: &str) -> String {
    let base = Path::new(base);
    let joined = base.join(normalize(path));
    joined.to_string_lossy().to_string()
}

/// Get the directory containing a path
pub fn dirname(path: &str) -> String {
    let path = Path::new(path);
    path.parent()
        .map(|p| p.to_string_lossy().to_string())
        .unwrap_or_else(|| ".".to_string())
}

/// Get the filename from a path
pub fn basename(path: &str) -> String {
    let path = Path::new(path);
    path.file_name()
        .map(|p| p.to_string_lossy().to_string())
        .unwrap_or_default()
}

/// Get the file extension
pub fn extension(path: &str) -> String {
    let path = Path::new(path);
    path.extension()
        .map(|p| p.to_string_lossy().to_string())
        .unwrap_or_default()
}
```

### 7.2 File Operations

File: `platform/src/files/mod.rs`
```rust
//! File system abstraction

pub mod path;

use crate::error::PlatformError;
use crate::PlatformResult;
use std::fs::{self, File, OpenOptions};
use std::io::{Read, Write, Seek, SeekFrom, BufReader, BufWriter};
use std::path::PathBuf;
use once_cell::sync::Lazy;
use std::sync::Mutex;

/// File open mode
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub enum FileMode {
    Read = 1,
    Write = 2,
    ReadWrite = 3,
    Append = 4,
}

/// Seek origin
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub enum SeekOrigin {
    Start = 0,
    Current = 1,
    End = 2,
}

/// Global paths
static PATHS: Lazy<Mutex<Paths>> = Lazy::new(|| Mutex::new(Paths::default()));

#[derive(Default)]
struct Paths {
    base_path: String,
    pref_path: String,
}

/// File handle
pub struct PlatformFile {
    file: File,
    path: PathBuf,
    mode: FileMode,
    size: i64,
}

impl PlatformFile {
    /// Open a file
    pub fn open(filename: &str, mode: FileMode) -> PlatformResult<Self> {
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

    /// Read bytes from file
    pub fn read(&mut self, buffer: &mut [u8]) -> PlatformResult<usize> {
        Ok(self.file.read(buffer)?)
    }

    /// Write bytes to file
    pub fn write(&mut self, buffer: &[u8]) -> PlatformResult<usize> {
        Ok(self.file.write(buffer)?)
    }

    /// Seek to position
    pub fn seek(&mut self, offset: i64, origin: SeekOrigin) -> PlatformResult<u64> {
        let pos = match origin {
            SeekOrigin::Start => SeekFrom::Start(offset as u64),
            SeekOrigin::Current => SeekFrom::Current(offset),
            SeekOrigin::End => SeekFrom::End(offset),
        };
        Ok(self.file.seek(pos)?)
    }

    /// Get current position
    pub fn tell(&mut self) -> PlatformResult<u64> {
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

    /// Flush buffers
    pub fn flush(&mut self) -> PlatformResult<()> {
        Ok(self.file.flush()?)
    }
}

/// Directory entry
#[repr(C)]
pub struct DirEntry {
    pub name: [u8; 260],
    pub is_directory: bool,
    pub size: i64,
}

/// Directory iterator handle
pub struct PlatformDir {
    iter: fs::ReadDir,
    path: PathBuf,
}

impl PlatformDir {
    pub fn open(path: &str) -> PlatformResult<Self> {
        let normalized = path::normalize(path);
        let iter = fs::read_dir(&normalized)?;
        Ok(Self {
            iter,
            path: PathBuf::from(normalized),
        })
    }

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
}

// =============================================================================
// Path Configuration
// =============================================================================

/// Initialize file system with paths
pub fn init(org_name: &str, app_name: &str) {
    if let Ok(mut paths) = PATHS.lock() {
        // Base path (executable directory or current directory)
        paths.base_path = std::env::current_exe()
            .ok()
            .and_then(|p| p.parent().map(|p| p.to_string_lossy().to_string()))
            .unwrap_or_else(|| ".".to_string());

        // Preferences path (~/Library/Application Support/AppName)
        if let Some(home) = dirs::home_dir() {
            let pref_dir = home
                .join("Library")
                .join("Application Support")
                .join(app_name);

            // Create if doesn't exist
            let _ = fs::create_dir_all(&pref_dir);

            paths.pref_path = pref_dir.to_string_lossy().to_string();
        }

        log::info!("File system initialized");
        log::info!("  Base path: {}", paths.base_path);
        log::info!("  Pref path: {}", paths.pref_path);
    }
}

/// Get base path
pub fn get_base_path() -> String {
    PATHS.lock()
        .map(|p| p.base_path.clone())
        .unwrap_or_else(|_| ".".to_string())
}

/// Get preferences path
pub fn get_pref_path() -> String {
    PATHS.lock()
        .map(|p| p.pref_path.clone())
        .unwrap_or_else(|_| ".".to_string())
}

// =============================================================================
// Utility Functions
// =============================================================================

/// Check if a file exists
pub fn exists(path: &str) -> bool {
    path::find_file(path).is_some()
}

/// Check if path is a directory
pub fn is_directory(path: &str) -> bool {
    let normalized = path::normalize(path);
    PathBuf::from(&normalized).is_dir()
}

/// Create a directory
pub fn create_directory(path: &str) -> PlatformResult<()> {
    let normalized = path::normalize(path);
    fs::create_dir_all(&normalized)?;
    Ok(())
}

/// Delete a file
pub fn delete_file(path: &str) -> PlatformResult<()> {
    if let Some(file_path) = path::find_file(path) {
        fs::remove_file(file_path)?;
    }
    Ok(())
}

/// Delete a directory
pub fn delete_directory(path: &str) -> PlatformResult<()> {
    let normalized = path::normalize(path);
    fs::remove_dir_all(&normalized)?;
    Ok(())
}

/// Copy a file
pub fn copy_file(src: &str, dst: &str) -> PlatformResult<u64> {
    let src_path = path::find_file(src)
        .ok_or_else(|| PlatformError::Io(std::io::Error::new(
            std::io::ErrorKind::NotFound,
            format!("Source file not found: {}", src)
        )))?;
    let dst_path = PathBuf::from(path::normalize(dst));
    Ok(fs::copy(src_path, dst_path)?)
}

/// Get file size without opening
pub fn get_file_size(path: &str) -> PlatformResult<i64> {
    if let Some(file_path) = path::find_file(path) {
        let meta = fs::metadata(file_path)?;
        Ok(meta.len() as i64)
    } else {
        Err(PlatformError::Io(std::io::Error::new(
            std::io::ErrorKind::NotFound,
            format!("File not found: {}", path)
        )))
    }
}
```

### 7.3 FFI File Exports

File: `platform/src/ffi/files.rs`
```rust
//! File system FFI exports

use crate::files::{self, PlatformFile, PlatformDir, DirEntry, FileMode, SeekOrigin};
use crate::error::catch_panic;
use std::ffi::{c_char, CStr, c_void};

// =============================================================================
// Path Functions
// =============================================================================

/// Get base path
#[no_mangle]
pub extern "C" fn Platform_GetBasePath() -> *const c_char {
    static mut PATH_BUF: [u8; 512] = [0; 512];

    let path = files::get_base_path();
    let bytes = path.as_bytes();
    let len = bytes.len().min(511);

    unsafe {
        PATH_BUF[..len].copy_from_slice(&bytes[..len]);
        PATH_BUF[len] = 0;
        PATH_BUF.as_ptr() as *const c_char
    }
}

/// Get preferences path
#[no_mangle]
pub extern "C" fn Platform_GetPrefPath() -> *const c_char {
    static mut PATH_BUF: [u8; 512] = [0; 512];

    let path = files::get_pref_path();
    let bytes = path.as_bytes();
    let len = bytes.len().min(511);

    unsafe {
        PATH_BUF[..len].copy_from_slice(&bytes[..len]);
        PATH_BUF[len] = 0;
        PATH_BUF.as_ptr() as *const c_char
    }
}

/// Normalize a path (convert Windows to Unix)
#[no_mangle]
pub unsafe extern "C" fn Platform_NormalizePath(path: *mut c_char) {
    if path.is_null() {
        return;
    }

    let c_str = CStr::from_ptr(path);
    if let Ok(s) = c_str.to_str() {
        let normalized = files::path::normalize(s);
        let bytes = normalized.as_bytes();
        let len = bytes.len();

        std::ptr::copy_nonoverlapping(bytes.as_ptr(), path as *mut u8, len);
        *path.add(len) = 0;
    }
}

// =============================================================================
// File Operations
// =============================================================================

/// Open a file
#[no_mangle]
pub unsafe extern "C" fn Platform_File_Open(
    path: *const c_char,
    mode: FileMode,
) -> *mut PlatformFile {
    if path.is_null() {
        return std::ptr::null_mut();
    }

    let path_str = match CStr::from_ptr(path).to_str() {
        Ok(s) => s,
        Err(_) => return std::ptr::null_mut(),
    };

    match PlatformFile::open(path_str, mode) {
        Ok(file) => Box::into_raw(Box::new(file)),
        Err(e) => {
            crate::error::set_error(e.to_string());
            std::ptr::null_mut()
        }
    }
}

/// Close a file
#[no_mangle]
pub extern "C" fn Platform_File_Close(file: *mut PlatformFile) {
    if !file.is_null() {
        unsafe {
            drop(Box::from_raw(file));
        }
    }
}

/// Read from file
#[no_mangle]
pub unsafe extern "C" fn Platform_File_Read(
    file: *mut PlatformFile,
    buffer: *mut c_void,
    size: i32,
) -> i32 {
    if file.is_null() || buffer.is_null() || size <= 0 {
        return -1;
    }

    let file = &mut *file;
    let buf = std::slice::from_raw_parts_mut(buffer as *mut u8, size as usize);

    match file.read(buf) {
        Ok(n) => n as i32,
        Err(_) => -1,
    }
}

/// Write to file
#[no_mangle]
pub unsafe extern "C" fn Platform_File_Write(
    file: *mut PlatformFile,
    buffer: *const c_void,
    size: i32,
) -> i32 {
    if file.is_null() || buffer.is_null() || size <= 0 {
        return -1;
    }

    let file = &mut *file;
    let buf = std::slice::from_raw_parts(buffer as *const u8, size as usize);

    match file.write(buf) {
        Ok(n) => n as i32,
        Err(_) => -1,
    }
}

/// Seek in file
#[no_mangle]
pub unsafe extern "C" fn Platform_File_Seek(
    file: *mut PlatformFile,
    offset: i64,
    origin: SeekOrigin,
) -> i32 {
    if file.is_null() {
        return -1;
    }

    let file = &mut *file;
    match file.seek(offset, origin) {
        Ok(_) => 0,
        Err(_) => -1,
    }
}

/// Get file position
#[no_mangle]
pub unsafe extern "C" fn Platform_File_Tell(file: *mut PlatformFile) -> i64 {
    if file.is_null() {
        return -1;
    }

    let file = &mut *file;
    file.tell().map(|p| p as i64).unwrap_or(-1)
}

/// Get file size
#[no_mangle]
pub unsafe extern "C" fn Platform_File_Size(file: *const PlatformFile) -> i64 {
    if file.is_null() {
        return -1;
    }

    (*file).size()
}

/// Check if at end of file
#[no_mangle]
pub unsafe extern "C" fn Platform_File_Eof(file: *mut PlatformFile) -> bool {
    if file.is_null() {
        return true;
    }

    (*file).eof()
}

// =============================================================================
// Directory Operations
// =============================================================================

/// Open a directory for reading
#[no_mangle]
pub unsafe extern "C" fn Platform_Dir_Open(path: *const c_char) -> *mut PlatformDir {
    if path.is_null() {
        return std::ptr::null_mut();
    }

    let path_str = match CStr::from_ptr(path).to_str() {
        Ok(s) => s,
        Err(_) => return std::ptr::null_mut(),
    };

    match PlatformDir::open(path_str) {
        Ok(dir) => Box::into_raw(Box::new(dir)),
        Err(_) => std::ptr::null_mut(),
    }
}

/// Read next directory entry
#[no_mangle]
pub unsafe extern "C" fn Platform_Dir_Read(
    dir: *mut PlatformDir,
    entry: *mut DirEntry,
) -> bool {
    if dir.is_null() || entry.is_null() {
        return false;
    }

    if let Some(e) = (*dir).read() {
        *entry = e;
        true
    } else {
        false
    }
}

/// Close directory
#[no_mangle]
pub extern "C" fn Platform_Dir_Close(dir: *mut PlatformDir) {
    if !dir.is_null() {
        unsafe {
            drop(Box::from_raw(dir));
        }
    }
}

// =============================================================================
// Utility Functions
// =============================================================================

/// Check if file exists
#[no_mangle]
pub unsafe extern "C" fn Platform_File_Exists(path: *const c_char) -> bool {
    if path.is_null() {
        return false;
    }

    if let Ok(s) = CStr::from_ptr(path).to_str() {
        files::exists(s)
    } else {
        false
    }
}

/// Check if path is directory
#[no_mangle]
pub unsafe extern "C" fn Platform_IsDirectory(path: *const c_char) -> bool {
    if path.is_null() {
        return false;
    }

    if let Ok(s) = CStr::from_ptr(path).to_str() {
        files::is_directory(s)
    } else {
        false
    }
}

/// Create directory
#[no_mangle]
pub unsafe extern "C" fn Platform_CreateDirectory(path: *const c_char) -> bool {
    if path.is_null() {
        return false;
    }

    if let Ok(s) = CStr::from_ptr(path).to_str() {
        files::create_directory(s).is_ok()
    } else {
        false
    }
}
```

## Tasks Breakdown

### Phase 1: Path Handling (1 day)
- [ ] Implement path normalization
- [ ] Implement case-insensitive lookup
- [ ] Test path conversion

### Phase 2: File Operations (1.5 days)
- [ ] Implement file open/close
- [ ] Implement read/write/seek
- [ ] Test file I/O

### Phase 3: Directory Operations (0.5 days)
- [ ] Implement directory iteration
- [ ] Test directory listing

### Phase 4: FFI Exports (1 day)
- [ ] Create FFI functions
- [ ] Verify cbindgen output
- [ ] Test from C++

### Phase 5: MIX Integration (1 day)
- [ ] Test MIX file loading
- [ ] Debug path issues
- [ ] Verify asset loading

## Acceptance Criteria

- [ ] Windows paths converted correctly
- [ ] Files found regardless of case
- [ ] MIX files load successfully
- [ ] Save games work
- [ ] All FFI functions work

## Estimated Duration
**4-5 days**

## Dependencies
- Plan 06 (Memory) completed
- std::fs, std::path

## Next Plan
Once file system is working, proceed to **Plan 08: Audio System**
