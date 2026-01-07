# Task 07d: FFI File Exports

## Dependencies
- Task 07c must be complete (Directory Operations)

## Context
The FFI exports expose the file system module to C++ code. This includes all file operations, directory iteration, path utilities, and helper functions needed by the game engine.

## Objective
Create C-compatible FFI functions for all file system operations and verify integration with C++ code through a test in main.cpp.

## Deliverables
- [ ] FFI exports in `platform/src/ffi/mod.rs`
- [ ] Path functions: GetBasePath, GetPrefPath, NormalizePath
- [ ] File functions: Open, Close, Read, Write, Seek, Tell, Size, Eof, Exists
- [ ] Directory functions: Open, Read, Close, CreateDirectory, IsDirectory
- [ ] C++ integration test in `src/main.cpp`
- [ ] cbindgen header generation
- [ ] Verification passes

## Files to Create/Modify

### platform/src/ffi/mod.rs (EXTEND)
Add file system FFI exports:
```rust
// =============================================================================
// File System FFI
// =============================================================================

use crate::files::{self, PlatformFile, PlatformDir, DirEntry, FileMode, SeekOrigin};

/// Get base path (executable directory)
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

/// Get preferences path (user data directory)
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

/// Initialize file system
#[no_mangle]
pub unsafe extern "C" fn Platform_Files_Init(org_name: *const c_char, app_name: *const c_char) {
    let org = if org_name.is_null() {
        ""
    } else {
        CStr::from_ptr(org_name).to_str().unwrap_or("")
    };

    let app = if app_name.is_null() {
        "RedAlert"
    } else {
        CStr::from_ptr(app_name).to_str().unwrap_or("RedAlert")
    };

    files::init(org, app);
}

/// Normalize a path (convert Windows to Unix style, modifies in place)
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
            set_error(e.to_string());
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

/// Flush file buffers
#[no_mangle]
pub unsafe extern "C" fn Platform_File_Flush(file: *mut PlatformFile) -> bool {
    if file.is_null() {
        return false;
    }

    (*file).flush().is_ok()
}

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

/// Get file size without opening
#[no_mangle]
pub unsafe extern "C" fn Platform_File_GetSize(path: *const c_char) -> i64 {
    if path.is_null() {
        return -1;
    }

    if let Ok(s) = CStr::from_ptr(path).to_str() {
        files::get_file_size(s).unwrap_or(-1)
    } else {
        -1
    }
}

/// Delete a file
#[no_mangle]
pub unsafe extern "C" fn Platform_File_Delete(path: *const c_char) -> bool {
    if path.is_null() {
        return false;
    }

    if let Ok(s) = CStr::from_ptr(path).to_str() {
        files::delete_file(s).is_ok()
    } else {
        false
    }
}

/// Copy a file
#[no_mangle]
pub unsafe extern "C" fn Platform_File_Copy(src: *const c_char, dst: *const c_char) -> i64 {
    if src.is_null() || dst.is_null() {
        return -1;
    }

    let src_str = match CStr::from_ptr(src).to_str() {
        Ok(s) => s,
        Err(_) => return -1,
    };
    let dst_str = match CStr::from_ptr(dst).to_str() {
        Ok(s) => s,
        Err(_) => return -1,
    };

    files::copy_file(src_str, dst_str).map(|n| n as i64).unwrap_or(-1)
}

/// Open a directory for iteration
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

/// Check if path is a directory
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

/// Create a directory
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

/// Delete a directory
#[no_mangle]
pub unsafe extern "C" fn Platform_DeleteDirectory(path: *const c_char) -> bool {
    if path.is_null() {
        return false;
    }

    if let Ok(s) = CStr::from_ptr(path).to_str() {
        files::delete_directory(s).is_ok()
    } else {
        false
    }
}
```

### src/main.cpp (EXTEND)
Add file system test:
```cpp
int test_files() {
    TEST("File system");

    // Test path normalization
    char path[256] = "DATA\\CONQUER.MIX";
    Platform_NormalizePath(path);
    if (strcmp(path, "DATA/CONQUER.MIX") != 0) {
        FAIL("Path normalization failed");
    }

    // Test file write
    const char* test_file = "/tmp/platform_test_file.txt";
    const char* test_data = "Hello from C++!";

    PlatformFile* file = Platform_File_Open(test_file, FILE_MODE_WRITE);
    if (file == nullptr) {
        FAIL("Failed to open file for writing");
    }

    int written = Platform_File_Write(file, test_data, strlen(test_data));
    if (written != (int)strlen(test_data)) {
        Platform_File_Close(file);
        FAIL("Write returned wrong byte count");
    }
    Platform_File_Close(file);

    // Test file exists
    if (!Platform_File_Exists(test_file)) {
        FAIL("File should exist after writing");
    }

    // Test file size
    int64_t size = Platform_File_GetSize(test_file);
    if (size != (int64_t)strlen(test_data)) {
        FAIL("File size mismatch");
    }

    // Test file read
    file = Platform_File_Open(test_file, FILE_MODE_READ);
    if (file == nullptr) {
        FAIL("Failed to open file for reading");
    }

    if (Platform_File_Size(file) != (int64_t)strlen(test_data)) {
        Platform_File_Close(file);
        FAIL("File size from handle mismatch");
    }

    char buffer[256] = {0};
    int bytes_read = Platform_File_Read(file, buffer, sizeof(buffer) - 1);
    if (bytes_read != (int)strlen(test_data)) {
        Platform_File_Close(file);
        FAIL("Read returned wrong byte count");
    }

    if (strcmp(buffer, test_data) != 0) {
        Platform_File_Close(file);
        FAIL("Read data mismatch");
    }

    // Test seek and tell
    Platform_File_Seek(file, 0, SEEK_ORIGIN_START);
    if (Platform_File_Tell(file) != 0) {
        Platform_File_Close(file);
        FAIL("Tell after seek to start should be 0");
    }

    Platform_File_Seek(file, 6, SEEK_ORIGIN_START);
    if (Platform_File_Tell(file) != 6) {
        Platform_File_Close(file);
        FAIL("Tell after seek to 6 should be 6");
    }

    // Test eof
    Platform_File_Seek(file, 0, SEEK_ORIGIN_END);
    if (!Platform_File_Eof(file)) {
        Platform_File_Close(file);
        FAIL("Should be at EOF after seeking to end");
    }

    Platform_File_Close(file);

    // Test directory operations
    const char* test_dir = "/tmp/platform_test_dir";
    if (!Platform_CreateDirectory(test_dir)) {
        // May already exist, that's ok
    }

    if (!Platform_IsDirectory(test_dir)) {
        FAIL("Created path should be a directory");
    }

    // Test directory iteration
    PlatformDir* dir = Platform_Dir_Open("/tmp");
    if (dir == nullptr) {
        FAIL("Failed to open /tmp directory");
    }

    DirEntry entry;
    int entry_count = 0;
    while (Platform_Dir_Read(dir, &entry)) {
        entry_count++;
    }
    Platform_Dir_Close(dir);

    if (entry_count == 0) {
        FAIL("/tmp should have some entries");
    }

    // Cleanup
    Platform_File_Delete(test_file);
    Platform_DeleteDirectory(test_dir);

    PASS();
    return 0;
}
```

Update `run_tests()` to include file test:
```cpp
int run_tests() {
    // ... existing tests ...

    if (test_files() != 0) return 1;

    printf("All tests passed!\n");
    return 0;
}
```

## Verification Command
```bash
cd /Users/ooartist/Src/Ronin_CnC && \
cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1 && \
cmake --build build 2>&1 && \
./build/RedAlert --test 2>&1 | grep -q "All tests passed" && \
grep -q "Platform_File_Open" include/platform.h && \
grep -q "Platform_Dir_Open" include/platform.h && \
grep -q "Platform_NormalizePath" include/platform.h && \
echo "VERIFY_SUCCESS"
```

## Implementation Steps
1. Add FFI imports for file module types
2. Implement path FFI functions (GetBasePath, GetPrefPath, NormalizePath, Files_Init)
3. Implement file FFI functions (Open, Close, Read, Write, Seek, Tell, Size, Eof, Flush)
4. Implement file utility FFI (Exists, GetSize, Delete, Copy)
5. Implement directory FFI functions (Open, Read, Close)
6. Implement directory utility FFI (IsDirectory, CreateDirectory, DeleteDirectory)
7. Build with cbindgen to generate header
8. Add C++ test function `test_files()`
9. Update `run_tests()` to call file test
10. Build and run tests

## Completion Promise
When verification passes, output:
```
<promise>TASK_07D_COMPLETE</promise>
```

Then proceed to **Phase 08: Audio System**.

## Escape Hatch
If stuck after 15 iterations:
- Document what's blocking in `ralph-tasks/blocked/07d.md`
- List attempted approaches
- Output: `<promise>TASK_07D_BLOCKED</promise>`

## Max Iterations
15
