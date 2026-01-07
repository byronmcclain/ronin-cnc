//! Path normalization and utilities

use std::path::{Path, PathBuf};
use std::fs;

/// Normalize a Windows-style path for Unix
/// - Converts backslashes to forward slashes
/// - Removes drive letters (e.g., "C:")
/// - Handles relative paths correctly
/// - Preserves Unix absolute paths (starting with /)
pub fn normalize(path: &str) -> String {
    let mut result = path.to_string();

    // Convert backslashes to forward slashes
    result = result.replace('\\', "/");

    // Check if this looks like a Windows path with drive letter
    let has_drive_letter = result.len() >= 2 &&
        result.chars().nth(0).map(|c| c.is_ascii_alphabetic()).unwrap_or(false) &&
        result.chars().nth(1) == Some(':');

    // Remove drive letter if present (e.g., "C:")
    if has_drive_letter {
        result = result[2..].to_string();

        // After removing drive letter, if it starts with /, remove it
        // to make it a relative path (e.g., "C:\foo" -> "foo")
        if result.starts_with('/') {
            result = result[1..].to_string();
        }
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
