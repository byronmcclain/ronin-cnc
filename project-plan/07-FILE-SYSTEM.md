# Plan 07: File System

## Objective
Replace Windows file I/O with POSIX file operations and implement proper path handling for macOS, including support for the MIX archive format.

## Current State Analysis

### Windows File I/O in Codebase

**Primary Files:**
- `WIN32LIB/RAWFILE/RAWFILE.CPP`
- `CODE/CCFILE.CPP`, `CODE/CCFILE.H`
- `CODE/CDFILE.CPP`, `CODE/CDFILE.H`
- `CODE/MIXFILE.CPP`, `CODE/MIXFILE.H`
- `WWFLAT32/FILE/` directory

**Windows APIs Used:**
```cpp
// File operations
CreateFile(path, access, share, security, creation, flags, template);
ReadFile(handle, buffer, size, &bytes_read, overlapped);
WriteFile(handle, buffer, size, &bytes_written, overlapped);
CloseHandle(handle);
SetFilePointer(handle, distance, high, method);
GetFileSize(handle, high);

// File attributes
GetFileAttributes(path);
SetFileAttributes(path, attributes);

// Directory enumeration
FindFirstFile(pattern, &find_data);
FindNextFile(handle, &find_data);
FindClose(handle);
```

**File Classes:**
```cpp
class RawFileClass {
    HANDLE Handle;
    char* Filename;
    int Rights;  // Read/Write

    virtual int Open(int rights);
    virtual void Close();
    virtual int Read(void* buffer, int size);
    virtual int Write(void* buffer, int size);
    virtual int Seek(int pos, int whence);
    virtual int Size();
};

class BufferIOFileClass : public RawFileClass {
    // Adds buffered I/O
};

class CCFileClass : public BufferIOFileClass {
    // Adds MIX file support
};
```

**Path Issues:**
- Backslash path separators: `DATA\CONQUER.MIX`
- Drive letters: `D:\GAMES\RA\`
- Case-insensitive filenames (DOS/Windows)
- CD-ROM detection and paths

## Implementation Strategy

### 7.1 POSIX File Implementation

File: `src/platform/file_posix.cpp`
```cpp
#include "platform/platform_file.h"
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <algorithm>
#include <string>

namespace {

std::string g_base_path;
std::string g_pref_path;

// Case-insensitive file finder
std::string Find_Case_Insensitive(const std::string& dir, const std::string& filename) {
    DIR* d = opendir(dir.c_str());
    if (!d) return "";

    std::string lower_name = filename;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

    struct dirent* entry;
    while ((entry = readdir(d)) != nullptr) {
        std::string entry_lower = entry->d_name;
        std::transform(entry_lower.begin(), entry_lower.end(), entry_lower.begin(), ::tolower);

        if (entry_lower == lower_name) {
            closedir(d);
            return std::string(entry->d_name);
        }
    }

    closedir(d);
    return "";
}

} // anonymous namespace

//=============================================================================
// Path Utilities
//=============================================================================

void Platform_File_Init(const char* org_name, const char* app_name) {
    // Get executable path
    char path[1024];
    uint32_t size = sizeof(path);

    #ifdef PLATFORM_MACOS
    // macOS-specific: get bundle path
    // For command-line app, use current directory
    if (getcwd(path, sizeof(path))) {
        g_base_path = path;
        g_base_path += "/";
    }
    #endif

    // Preferences path (for saves, config)
    const char* home = getenv("HOME");
    if (home) {
        g_pref_path = home;
        g_pref_path += "/Library/Application Support/";
        g_pref_path += app_name;
        g_pref_path += "/";

        // Create directory if it doesn't exist
        mkdir(g_pref_path.c_str(), 0755);
    }
}

const char* Platform_GetBasePath(void) {
    return g_base_path.c_str();
}

const char* Platform_GetPrefPath(void) {
    return g_pref_path.c_str();
}

const char* Platform_GetSeparator(void) {
    return "/";
}

void Platform_NormalizePath(char* path) {
    if (!path) return;

    // Convert backslashes to forward slashes
    for (char* p = path; *p; p++) {
        if (*p == '\\') *p = '/';
    }

    // Remove drive letter if present (e.g., "C:")
    if (path[0] && path[1] == ':') {
        memmove(path, path + 2, strlen(path + 2) + 1);
    }

    // Remove leading slash if path is relative
    // (Windows paths like \DATA become /DATA, we want DATA)
    if (path[0] == '/' && path[1] != '/') {
        memmove(path, path + 1, strlen(path));
    }
}

bool Platform_PathExists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0;
}

bool Platform_IsDirectory(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

bool Platform_CreateDirectory(const char* path) {
    return mkdir(path, 0755) == 0;
}
```

### 7.2 File Operations

```cpp
//=============================================================================
// File Handle
//=============================================================================

struct PlatformFile {
    FILE* handle;
    std::string path;
    FileMode mode;
    int64_t size;
};

PlatformFile* Platform_File_Open(const char* path, FileMode mode) {
    // Normalize path
    std::string normalized = path;
    for (char& c : normalized) {
        if (c == '\\') c = '/';
    }

    // Determine fopen mode
    const char* fmode;
    if (mode == FILE_READ) {
        fmode = "rb";
    } else if (mode == FILE_WRITE) {
        fmode = "wb";
    } else if (mode == FILE_APPEND) {
        fmode = "ab";
    } else if (mode == (FILE_READ | FILE_WRITE)) {
        fmode = "r+b";
    } else {
        return nullptr;
    }

    // Try to open file
    FILE* handle = fopen(normalized.c_str(), fmode);

    // If read failed, try case-insensitive search
    if (!handle && (mode & FILE_READ)) {
        // Split into directory and filename
        size_t last_sep = normalized.rfind('/');
        std::string dir = (last_sep != std::string::npos)
                          ? normalized.substr(0, last_sep)
                          : ".";
        std::string filename = (last_sep != std::string::npos)
                               ? normalized.substr(last_sep + 1)
                               : normalized;

        std::string real_name = Find_Case_Insensitive(dir, filename);
        if (!real_name.empty()) {
            std::string real_path = dir + "/" + real_name;
            handle = fopen(real_path.c_str(), fmode);
        }
    }

    if (!handle) {
        Platform_SetError("Failed to open file: %s", path);
        return nullptr;
    }

    PlatformFile* file = new PlatformFile();
    file->handle = handle;
    file->path = normalized;
    file->mode = mode;

    // Get file size
    fseek(handle, 0, SEEK_END);
    file->size = ftell(handle);
    fseek(handle, 0, SEEK_SET);

    return file;
}

void Platform_File_Close(PlatformFile* file) {
    if (file) {
        if (file->handle) {
            fclose(file->handle);
        }
        delete file;
    }
}

int Platform_File_Read(PlatformFile* file, void* buffer, int size) {
    if (!file || !file->handle) return -1;
    return (int)fread(buffer, 1, size, file->handle);
}

int Platform_File_Write(PlatformFile* file, const void* buffer, int size) {
    if (!file || !file->handle) return -1;
    return (int)fwrite(buffer, 1, size, file->handle);
}

bool Platform_File_Seek(PlatformFile* file, int64_t offset, SeekOrigin origin) {
    if (!file || !file->handle) return false;

    int whence;
    switch (origin) {
        case SEEK_FROM_START: whence = SEEK_SET; break;
        case SEEK_FROM_CURRENT: whence = SEEK_CUR; break;
        case SEEK_FROM_END: whence = SEEK_END; break;
        default: return false;
    }

    return fseek(file->handle, (long)offset, whence) == 0;
}

int64_t Platform_File_Tell(PlatformFile* file) {
    if (!file || !file->handle) return -1;
    return ftell(file->handle);
}

int64_t Platform_File_Size(PlatformFile* file) {
    if (!file) return -1;
    return file->size;
}

bool Platform_File_Eof(PlatformFile* file) {
    if (!file || !file->handle) return true;
    return feof(file->handle) != 0;
}
```

### 7.3 Directory Enumeration

```cpp
//=============================================================================
// Directory Operations
//=============================================================================

struct PlatformDir {
    DIR* handle;
    std::string path;
};

PlatformDir* Platform_Dir_Open(const char* path) {
    std::string normalized = path;
    for (char& c : normalized) {
        if (c == '\\') c = '/';
    }

    DIR* dir = opendir(normalized.c_str());
    if (!dir) {
        Platform_SetError("Failed to open directory: %s", path);
        return nullptr;
    }

    PlatformDir* pdir = new PlatformDir();
    pdir->handle = dir;
    pdir->path = normalized;
    return pdir;
}

bool Platform_Dir_Read(PlatformDir* dir, DirEntry* entry) {
    if (!dir || !dir->handle) return false;

    struct dirent* d = readdir(dir->handle);
    if (!d) return false;

    // Skip . and ..
    while (d && (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0)) {
        d = readdir(dir->handle);
    }
    if (!d) return false;

    strncpy(entry->name, d->d_name, sizeof(entry->name) - 1);
    entry->name[sizeof(entry->name) - 1] = '\0';

    // Get file info
    std::string full_path = dir->path + "/" + d->d_name;
    struct stat st;
    if (stat(full_path.c_str(), &st) == 0) {
        entry->is_directory = S_ISDIR(st.st_mode);
        entry->size = st.st_size;
    } else {
        entry->is_directory = false;
        entry->size = 0;
    }

    return true;
}

void Platform_Dir_Close(PlatformDir* dir) {
    if (dir) {
        if (dir->handle) {
            closedir(dir->handle);
        }
        delete dir;
    }
}
```

### 7.4 Windows Compatibility Wrappers

File: `include/compat/file_wrapper.h`
```cpp
#ifndef FILE_WRAPPER_H
#define FILE_WRAPPER_H

#include "platform/platform_file.h"
#include <cstring>

//=============================================================================
// Windows Handle Types
//=============================================================================

#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)

//=============================================================================
// CreateFile
//=============================================================================

#define GENERIC_READ            0x80000000
#define GENERIC_WRITE           0x40000000
#define FILE_SHARE_READ         0x00000001
#define FILE_SHARE_WRITE        0x00000002
#define CREATE_NEW              1
#define CREATE_ALWAYS           2
#define OPEN_EXISTING           3
#define OPEN_ALWAYS             4
#define TRUNCATE_EXISTING       5

inline HANDLE CreateFile(const char* path, DWORD access, DWORD share,
                         void* security, DWORD creation, DWORD flags,
                         HANDLE template_file) {
    (void)share;
    (void)security;
    (void)flags;
    (void)template_file;

    FileMode mode = 0;
    if (access & GENERIC_READ) mode |= FILE_READ;
    if (access & GENERIC_WRITE) mode |= FILE_WRITE;

    // Handle creation disposition
    if (creation == CREATE_ALWAYS || creation == TRUNCATE_EXISTING) {
        mode = FILE_WRITE;  // Overwrite
    }

    PlatformFile* file = Platform_File_Open(path, mode);
    return file ? (HANDLE)file : INVALID_HANDLE_VALUE;
}

inline BOOL ReadFile(HANDLE handle, void* buffer, DWORD size,
                     DWORD* bytes_read, void* overlapped) {
    (void)overlapped;
    PlatformFile* file = (PlatformFile*)handle;
    int result = Platform_File_Read(file, buffer, size);
    if (bytes_read) *bytes_read = (result >= 0) ? result : 0;
    return result >= 0;
}

inline BOOL WriteFile(HANDLE handle, const void* buffer, DWORD size,
                      DWORD* bytes_written, void* overlapped) {
    (void)overlapped;
    PlatformFile* file = (PlatformFile*)handle;
    int result = Platform_File_Write(file, buffer, size);
    if (bytes_written) *bytes_written = (result >= 0) ? result : 0;
    return result >= 0;
}

inline BOOL CloseHandle(HANDLE handle) {
    Platform_File_Close((PlatformFile*)handle);
    return TRUE;
}

#define FILE_BEGIN      0
#define FILE_CURRENT    1
#define FILE_END        2

inline DWORD SetFilePointer(HANDLE handle, LONG distance, LONG* high,
                            DWORD method) {
    (void)high;  // We don't support 64-bit seeks through this API
    SeekOrigin origin;
    switch (method) {
        case FILE_BEGIN: origin = SEEK_FROM_START; break;
        case FILE_CURRENT: origin = SEEK_FROM_CURRENT; break;
        case FILE_END: origin = SEEK_FROM_END; break;
        default: return (DWORD)-1;
    }
    Platform_File_Seek((PlatformFile*)handle, distance, origin);
    return (DWORD)Platform_File_Tell((PlatformFile*)handle);
}

inline DWORD GetFileSize(HANDLE handle, DWORD* high) {
    if (high) *high = 0;
    return (DWORD)Platform_File_Size((PlatformFile*)handle);
}

//=============================================================================
// FindFirstFile / FindNextFile
//=============================================================================

typedef struct {
    DWORD dwFileAttributes;
    char cFileName[260];
    // Other fields not used
} WIN32_FIND_DATA;

#define FILE_ATTRIBUTE_DIRECTORY    0x10
#define FILE_ATTRIBUTE_NORMAL       0x80

struct FindHandle {
    PlatformDir* dir;
    std::string pattern;
};

inline HANDLE FindFirstFile(const char* pattern, WIN32_FIND_DATA* data) {
    // Extract directory from pattern
    std::string path = pattern;
    for (char& c : path) if (c == '\\') c = '/';

    size_t last_sep = path.rfind('/');
    std::string dir = (last_sep != std::string::npos) ? path.substr(0, last_sep) : ".";

    PlatformDir* pdir = Platform_Dir_Open(dir.c_str());
    if (!pdir) return INVALID_HANDLE_VALUE;

    FindHandle* fh = new FindHandle();
    fh->dir = pdir;
    fh->pattern = (last_sep != std::string::npos) ? path.substr(last_sep + 1) : path;

    // Read first entry
    DirEntry entry;
    if (Platform_Dir_Read(pdir, &entry)) {
        strcpy(data->cFileName, entry.name);
        data->dwFileAttributes = entry.is_directory ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
        return (HANDLE)fh;
    }

    Platform_Dir_Close(pdir);
    delete fh;
    return INVALID_HANDLE_VALUE;
}

inline BOOL FindNextFile(HANDLE handle, WIN32_FIND_DATA* data) {
    FindHandle* fh = (FindHandle*)handle;
    if (!fh) return FALSE;

    DirEntry entry;
    if (Platform_Dir_Read(fh->dir, &entry)) {
        strcpy(data->cFileName, entry.name);
        data->dwFileAttributes = entry.is_directory ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
        return TRUE;
    }
    return FALSE;
}

inline BOOL FindClose(HANDLE handle) {
    FindHandle* fh = (FindHandle*)handle;
    if (fh) {
        Platform_Dir_Close(fh->dir);
        delete fh;
    }
    return TRUE;
}

//=============================================================================
// File Attributes
//=============================================================================

inline DWORD GetFileAttributes(const char* path) {
    if (Platform_IsDirectory(path)) return FILE_ATTRIBUTE_DIRECTORY;
    if (Platform_PathExists(path)) return FILE_ATTRIBUTE_NORMAL;
    return (DWORD)-1;
}

inline BOOL SetFileAttributes(const char* path, DWORD attributes) {
    (void)path;
    (void)attributes;
    return TRUE;  // No-op on Unix
}

#endif // FILE_WRAPPER_H
```

### 7.5 MIX File Support

The MIX file format should work mostly unchanged, but we need to ensure path handling works:

File: `src/game/mixfile_compat.cpp`
```cpp
// Patch for MIXFILE.CPP to handle paths correctly

#include "platform/platform_file.h"

// Override the file open to use our path normalization
bool Mix_Open_File(const char* filename) {
    char normalized[256];
    strncpy(normalized, filename, sizeof(normalized));
    Platform_NormalizePath(normalized);

    // Use platform file open
    PlatformFile* file = Platform_File_Open(normalized, FILE_READ);
    if (!file) return false;

    // ... rest of MIX loading
    return true;
}
```

## Tasks Breakdown

### Phase 1: Path Handling (1 day)
- [ ] Implement path normalization (backslash → forward slash)
- [ ] Implement case-insensitive file lookup
- [ ] Handle drive letter removal
- [ ] Set up base path and preferences path

### Phase 2: File Operations (1.5 days)
- [ ] Implement `Platform_File_Open/Close/Read/Write`
- [ ] Implement seek and size operations
- [ ] Test with simple file I/O

### Phase 3: Directory Operations (0.5 days)
- [ ] Implement directory enumeration
- [ ] Test with file listing

### Phase 4: Windows Wrappers (1 day)
- [ ] Create `file_wrapper.h`
- [ ] Map `CreateFile`/`ReadFile`/`WriteFile`
- [ ] Map `FindFirstFile`/`FindNextFile`
- [ ] Test with game file loading

### Phase 5: MIX File Integration (1 day)
- [ ] Verify MIX file loading works
- [ ] Test asset loading from MIX archives
- [ ] Debug any path issues

## Testing Strategy

### Test 1: Path Normalization
Convert Windows paths and verify correctness.

### Test 2: Case-Insensitive Lookup
Find files regardless of case.

### Test 3: File Read/Write
Read and write test files.

### Test 4: MIX File Loading
Load CONQUER.MIX and extract assets.

## Acceptance Criteria

- [ ] Windows paths converted correctly
- [ ] Files found regardless of case
- [ ] MIX files load successfully
- [ ] Game assets load correctly
- [ ] Save games work

## Estimated Duration
**4-5 days**

## Dependencies
- Plan 06 (Memory Management) completed
- POSIX file APIs available

## Next Plan
Once file system is working, proceed to **Plan 08: Audio System**
