# Task 07a: Path Utilities

## Dependencies
- Task 06d must be complete (Memory Integration Test)
- Phase 01 Build System complete

## Context
The file system module provides path normalization and case-insensitive file lookup to handle Windows-style paths (backslashes, drive letters) on macOS. This is essential for loading game assets that use hardcoded Windows paths.

## Objective
Create the path utilities module with functions for normalizing Windows paths to Unix, case-insensitive file lookup, and path manipulation (join, dirname, basename, extension).

## Deliverables
- [ ] `platform/src/files/path.rs` - Path utility functions
- [ ] `platform/src/files/mod.rs` - Module definition (stub for now)
- [ ] Path normalization (backslash to forward slash, remove drive letters)
- [ ] Case-insensitive file lookup
- [ ] Path manipulation functions
- [ ] Verification passes

## Files to Create/Modify

### platform/src/files/path.rs (NEW)
```rust
//! Path normalization and utilities

use std::path::{Path, PathBuf};
use std::fs;

/// Normalize a Windows-style path for Unix
/// - Converts backslashes to forward slashes
/// - Removes drive letters (e.g., "C:")
/// - Handles relative paths correctly
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

/// Find a file with case-insensitive matching in a directory
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
    let path_obj = Path::new(&normalized);

    // First, try the exact path
    if path_obj.exists() {
        return Some(path_obj.to_path_buf());
    }

    // Try case-insensitive search in parent directory
    if let Some(parent) = path_obj.parent() {
        if let Some(filename) = path_obj.file_name() {
            // If parent doesn't exist, try current directory
            let search_dir = if parent.as_os_str().is_empty() || !parent.exists() {
                Path::new(".")
            } else {
                parent
            };

            if let Some(found) = find_case_insensitive(search_dir, &filename.to_string_lossy()) {
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

        // Also try case-insensitive in data directory
        let data_path = Path::new(dir);
        if data_path.exists() {
            if let Some(found) = find_case_insensitive(data_path, &normalized) {
                return Some(found);
            }
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

/// Get the file extension (without the dot)
pub fn extension(path: &str) -> String {
    let path = Path::new(path);
    path.extension()
        .map(|p| p.to_string_lossy().to_string())
        .unwrap_or_default()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_normalize_backslashes() {
        assert_eq!(normalize("DATA\\CONQUER.MIX"), "DATA/CONQUER.MIX");
        assert_eq!(normalize("foo\\bar\\baz.txt"), "foo/bar/baz.txt");
    }

    #[test]
    fn test_normalize_drive_letter() {
        assert_eq!(normalize("C:\\Games\\RA"), "Games/RA");
        assert_eq!(normalize("D:\\DATA\\FILE.DAT"), "DATA/FILE.DAT");
    }

    #[test]
    fn test_path_functions() {
        assert_eq!(dirname("foo/bar/baz.txt"), "foo/bar");
        assert_eq!(basename("foo/bar/baz.txt"), "baz.txt");
        assert_eq!(extension("foo/bar/baz.txt"), "txt");
        assert_eq!(join("foo", "bar"), "foo/bar");
    }
}
```

### platform/src/files/mod.rs (NEW - stub)
```rust
//! File system abstraction
//!
//! Provides cross-platform file I/O with Windows path compatibility.

pub mod path;

// File operations will be added in task 07b
```

### platform/src/lib.rs (MODIFY)
Add after `pub mod memory;`:
```rust
pub mod files;
```

## Verification Command
```bash
cd /Users/ooartist/Src/Ronin_CnC && \
cargo build --manifest-path platform/Cargo.toml 2>&1 && \
cargo test --manifest-path platform/Cargo.toml -- path 2>&1 && \
grep -q "pub fn normalize" platform/src/files/path.rs && \
grep -q "pub fn find_case_insensitive" platform/src/files/path.rs && \
grep -q "pub fn find_file" platform/src/files/path.rs && \
echo "VERIFY_SUCCESS"
```

## Implementation Steps
1. Create `platform/src/files/` directory
2. Create `platform/src/files/path.rs` with path utilities
3. Create `platform/src/files/mod.rs` as module root
4. Add `pub mod files;` to `platform/src/lib.rs`
5. Implement `normalize()` for Windows-to-Unix path conversion
6. Implement `find_case_insensitive()` for directory searches
7. Implement `find_file()` combining normalization and case-insensitive lookup
8. Implement helper functions (join, dirname, basename, extension)
9. Run tests and verify

## Completion Promise
When verification passes, output:
```
<promise>TASK_07A_COMPLETE</promise>
```

Then proceed to **Task 07b: File Operations**.

## Escape Hatch
If stuck after 10 iterations:
- Document what's blocking in `ralph-tasks/blocked/07a.md`
- List attempted approaches
- Output: `<promise>TASK_07A_BLOCKED</promise>`

## Max Iterations
10
