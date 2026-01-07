# Task 07c: Directory Operations

## Dependencies
- Task 07b must be complete (File Operations)

## Context
Directory iteration is needed for scanning game data directories, finding files by extension, and supporting mod loading. The PlatformDir struct provides a cross-platform directory iterator.

## Objective
Add directory iteration support with PlatformDir struct and DirEntry. Include functions for listing directory contents and matching file patterns.

## Deliverables
- [ ] `DirEntry` struct for directory entries
- [ ] `PlatformDir` struct for directory iteration
- [ ] Directory utility functions
- [ ] Integration with existing file module
- [ ] Verification passes

## Files to Create/Modify

### platform/src/files/mod.rs (EXTEND)
Add after the existing code:
```rust
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
            return Err(PlatformError::Io(std::io::Error::new(
                std::io::ErrorKind::NotFound,
                format!("Directory not found: {}", path_str)
            )));
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
mod dir_tests {
    use super::*;

    #[test]
    fn test_dir_iteration() {
        // Create test directory
        let test_dir = "/tmp/platform_dir_test";
        let _ = fs::remove_dir_all(test_dir);
        fs::create_dir_all(test_dir).unwrap();

        // Create test files
        fs::write(format!("{}/file1.txt", test_dir), "test1").unwrap();
        fs::write(format!("{}/file2.txt", test_dir), "test2").unwrap();
        fs::write(format!("{}/data.mix", test_dir), "mixdata").unwrap();
        fs::create_dir(format!("{}/subdir", test_dir)).unwrap();

        // Test iteration
        let mut dir = PlatformDir::open(test_dir).unwrap();
        let mut count = 0;
        while let Some(_entry) = dir.read() {
            count += 1;
        }
        assert_eq!(count, 4); // 3 files + 1 directory

        // Test list_files
        let files = list_files(test_dir).unwrap();
        assert_eq!(files.len(), 3);

        // Test list_directories
        let dirs = list_directories(test_dir).unwrap();
        assert_eq!(dirs.len(), 1);

        // Test find_files_with_extension
        let txt_files = find_files_with_extension(test_dir, "txt").unwrap();
        assert_eq!(txt_files.len(), 2);

        let mix_files = find_files_with_extension(test_dir, ".MIX").unwrap();
        assert_eq!(mix_files.len(), 1);

        // Test count_entries
        let (fc, dc) = count_entries(test_dir).unwrap();
        assert_eq!(fc, 3);
        assert_eq!(dc, 1);

        // Cleanup
        let _ = fs::remove_dir_all(test_dir);
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
```

## Verification Command
```bash
cd /Users/ooartist/Src/Ronin_CnC && \
cargo build --manifest-path platform/Cargo.toml 2>&1 && \
cargo test --manifest-path platform/Cargo.toml -- dir 2>&1 && \
grep -q "pub struct DirEntry" platform/src/files/mod.rs && \
grep -q "pub struct PlatformDir" platform/src/files/mod.rs && \
grep -q "pub fn list_files" platform/src/files/mod.rs && \
grep -q "pub fn find_files_with_extension" platform/src/files/mod.rs && \
echo "VERIFY_SUCCESS"
```

## Implementation Steps
1. Add `DirEntry` struct with name buffer, is_directory flag, and size
2. Add `name_str()` helper method to DirEntry
3. Implement `PlatformDir` struct wrapping fs::ReadDir
4. Implement `PlatformDir::open()` with path normalization
5. Implement `PlatformDir::read()` with . and .. filtering
6. Add utility functions: `list_files()`, `list_directories()`
7. Add `find_files_with_extension()` for pattern matching
8. Add `count_entries()` for directory statistics
9. Write tests and verify

## Completion Promise
When verification passes, output:
```
<promise>TASK_07C_COMPLETE</promise>
```

Then proceed to **Task 07d: FFI File Exports**.

## Escape Hatch
If stuck after 10 iterations:
- Document what's blocking in `ralph-tasks/blocked/07c.md`
- List attempted approaches
- Output: `<promise>TASK_07C_BLOCKED</promise>`

## Max Iterations
10
