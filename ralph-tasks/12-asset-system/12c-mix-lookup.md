# Task 12c: MIX File Data Lookup and Retrieval

## Dependencies
- Task 12a (CRC FFI) must be complete
- Task 12b (MIX Header) must be complete

## Context
Now that we can parse MIX headers and find file entries by CRC, we need to actually retrieve the file data. This task extends the MixFile class to read file contents from the archive, either into a provided buffer or into newly allocated memory.

## Objective
Extend MixFile class to:
1. Read file data by filename or CRC
2. Support both allocated and user-provided buffers
3. Handle file caching (optional, for frequently accessed files)
4. Provide streaming interface for large files

## Technical Background

### Data Layout in MIX Files
```
[Header] [Entries] [File1 Data] [File2 Data] ... [FileN Data] [Optional Digest]

File data location = data_start + entry.offset
File data size = entry.size
```

### File Access Patterns
1. **Single read**: Load entire file into memory (most common)
2. **Partial read**: Read portion of file (for streaming audio/video)
3. **Cached read**: Keep file in memory for repeated access

### Memory Management Considerations
- Some files are small (palettes: 768 bytes)
- Some files are large (terrain: megabytes)
- Game frequently re-reads same files
- Original game cached entire MIX files in RAM

## Deliverables
- [ ] Extended `MixFile` class with data retrieval methods
- [ ] Memory management for loaded file data
- [ ] Test program that loads and verifies file data
- [ ] Verification script passes

## Files to Modify/Create

### include/game/mix_file.h (MODIFY)
Add data retrieval methods:

```cpp
// Add to MixFile class in mix_file.h

public:
    // === Data Retrieval Methods ===

    /// Get file size by name
    /// @param filename Filename to look up
    /// @return File size in bytes, or 0 if not found
    uint32_t GetFileSize(const char* filename) const;

    /// Get file size by CRC
    /// @param crc CRC32 of uppercase filename
    /// @return File size in bytes, or 0 if not found
    uint32_t GetFileSizeByCRC(int32_t crc) const;

    /// Load file data into newly allocated buffer
    /// Caller is responsible for freeing the returned buffer with delete[]
    /// @param filename Filename to load
    /// @param out_size Receives file size (can be null)
    /// @return Pointer to allocated data, or nullptr on failure
    uint8_t* LoadFile(const char* filename, uint32_t* out_size = nullptr);

    /// Load file data by CRC
    /// @param crc CRC32 of uppercase filename
    /// @param out_size Receives file size (can be null)
    /// @return Pointer to allocated data, or nullptr on failure
    uint8_t* LoadFileByCRC(int32_t crc, uint32_t* out_size = nullptr);

    /// Load file data into provided buffer
    /// @param filename Filename to load
    /// @param buffer Buffer to load into
    /// @param buffer_size Size of provided buffer
    /// @return Number of bytes read, or 0 on failure
    uint32_t LoadFileInto(const char* filename, void* buffer, uint32_t buffer_size);

    /// Load file data by CRC into provided buffer
    /// @param crc CRC32 of uppercase filename
    /// @param buffer Buffer to load into
    /// @param buffer_size Size of provided buffer
    /// @return Number of bytes read, or 0 on failure
    uint32_t LoadFileIntoByCRC(int32_t crc, void* buffer, uint32_t buffer_size);

    /// Read partial file data (for streaming)
    /// @param filename Filename to read from
    /// @param offset Offset within file
    /// @param buffer Buffer to read into
    /// @param count Number of bytes to read
    /// @return Number of bytes actually read
    uint32_t ReadFilePartial(const char* filename, uint32_t offset,
                             void* buffer, uint32_t count);

    // === Caching Support ===

    /// Cache all data in memory (loads entire MIX file)
    /// After caching, file access is from memory instead of disk
    /// @return true if caching succeeded
    bool CacheAll();

    /// Check if MIX data is cached
    bool IsCached() const { return cached_data_ != nullptr; }

    /// Free cached data
    void FreeCache();

    /// Get pointer to cached file data (only valid if cached)
    /// @param filename Filename to get
    /// @param out_size Receives file size
    /// @return Pointer to cached data, or nullptr if not cached/not found
    const uint8_t* GetCachedFile(const char* filename, uint32_t* out_size) const;

private:
    /// Helper to read file data from disk or cache
    uint32_t ReadFileData(const MixEntry* entry, void* buffer, uint32_t buffer_size);

    /// Cached data (if CacheAll() was called)
    uint8_t* cached_data_;
    uint32_t cached_size_;
```

### src/game/mix_file.cpp (MODIFY)
Add data retrieval implementation:

```cpp
// Add to mix_file.cpp

// Update constructor to initialize cache
MixFile::MixFile()
    : file_(nullptr)
    , data_size_(0)
    , data_start_(0)
    , is_valid_(false)
    , is_extended_(false)
    , is_encrypted_(false)
    , has_digest_(false)
    , cached_data_(nullptr)
    , cached_size_(0)
{
}

// Update Close() to free cache
void MixFile::Close() {
    FreeCache();  // Add this line

    if (file_) {
        Platform_File_Close(file_);
        file_ = nullptr;
    }
    // ... rest of Close()
}

// Update move operations to handle cache
MixFile::MixFile(MixFile&& other) noexcept
    : file_(other.file_)
    , path_(std::move(other.path_))
    , entries_(std::move(other.entries_))
    , data_size_(other.data_size_)
    , data_start_(other.data_start_)
    , is_valid_(other.is_valid_)
    , is_extended_(other.is_extended_)
    , is_encrypted_(other.is_encrypted_)
    , has_digest_(other.has_digest_)
    , cached_data_(other.cached_data_)
    , cached_size_(other.cached_size_)
{
    other.file_ = nullptr;
    other.is_valid_ = false;
    other.cached_data_ = nullptr;
    other.cached_size_ = 0;
}

// === Data Size Methods ===

uint32_t MixFile::GetFileSize(const char* filename) const {
    if (!filename) return 0;
    int32_t crc = static_cast<int32_t>(Platform_CRC_ForMixLookup(filename));
    return GetFileSizeByCRC(crc);
}

uint32_t MixFile::GetFileSizeByCRC(int32_t crc) const {
    const MixEntry* entry = FindByCRC(crc);
    return entry ? entry->size : 0;
}

// === Data Loading Methods ===

uint8_t* MixFile::LoadFile(const char* filename, uint32_t* out_size) {
    if (!filename) return nullptr;
    int32_t crc = static_cast<int32_t>(Platform_CRC_ForMixLookup(filename));
    return LoadFileByCRC(crc, out_size);
}

uint8_t* MixFile::LoadFileByCRC(int32_t crc, uint32_t* out_size) {
    const MixEntry* entry = FindByCRC(crc);
    if (!entry) {
        if (out_size) *out_size = 0;
        return nullptr;
    }

    uint8_t* buffer = new (std::nothrow) uint8_t[entry->size];
    if (!buffer) {
        fprintf(stderr, "MixFile: Failed to allocate %u bytes\n", entry->size);
        if (out_size) *out_size = 0;
        return nullptr;
    }

    uint32_t read = ReadFileData(entry, buffer, entry->size);
    if (read != entry->size) {
        delete[] buffer;
        if (out_size) *out_size = 0;
        return nullptr;
    }

    if (out_size) *out_size = entry->size;
    return buffer;
}

uint32_t MixFile::LoadFileInto(const char* filename, void* buffer, uint32_t buffer_size) {
    if (!filename) return 0;
    int32_t crc = static_cast<int32_t>(Platform_CRC_ForMixLookup(filename));
    return LoadFileIntoByCRC(crc, buffer, buffer_size);
}

uint32_t MixFile::LoadFileIntoByCRC(int32_t crc, void* buffer, uint32_t buffer_size) {
    const MixEntry* entry = FindByCRC(crc);
    if (!entry || !buffer || buffer_size == 0) {
        return 0;
    }

    uint32_t to_read = (buffer_size < entry->size) ? buffer_size : entry->size;
    return ReadFileData(entry, buffer, to_read);
}

uint32_t MixFile::ReadFilePartial(const char* filename, uint32_t offset,
                                   void* buffer, uint32_t count) {
    if (!filename || !buffer || count == 0) return 0;

    int32_t crc = static_cast<int32_t>(Platform_CRC_ForMixLookup(filename));
    const MixEntry* entry = FindByCRC(crc);
    if (!entry) return 0;

    // Bounds check
    if (offset >= entry->size) return 0;
    uint32_t available = entry->size - offset;
    uint32_t to_read = (count < available) ? count : available;

    if (cached_data_) {
        // Read from cache
        const uint8_t* file_data = cached_data_ + entry->offset;
        memcpy(buffer, file_data + offset, to_read);
        return to_read;
    } else {
        // Read from disk
        if (!file_) return 0;

        uint32_t file_offset = data_start_ + entry->offset + offset;
        if (Platform_File_Seek(file_, file_offset, PLATFORM_SEEK_SET) != 0) {
            return 0;
        }

        int32_t read = Platform_File_Read(file_, buffer, to_read);
        return (read > 0) ? static_cast<uint32_t>(read) : 0;
    }
}

// === Internal Helper ===

uint32_t MixFile::ReadFileData(const MixEntry* entry, void* buffer, uint32_t buffer_size) {
    if (!entry || !buffer || buffer_size == 0) return 0;

    uint32_t to_read = (buffer_size < entry->size) ? buffer_size : entry->size;

    if (cached_data_) {
        // Read from cache
        const uint8_t* file_data = cached_data_ + entry->offset;
        memcpy(buffer, file_data, to_read);
        return to_read;
    } else {
        // Read from disk
        if (!file_) return 0;

        uint32_t file_offset = data_start_ + entry->offset;
        if (Platform_File_Seek(file_, file_offset, PLATFORM_SEEK_SET) != 0) {
            fprintf(stderr, "MixFile: Seek failed to offset 0x%08X\n", file_offset);
            return 0;
        }

        int32_t read = Platform_File_Read(file_, buffer, to_read);
        if (read < 0) {
            fprintf(stderr, "MixFile: Read failed\n");
            return 0;
        }
        return static_cast<uint32_t>(read);
    }
}

// === Caching ===

bool MixFile::CacheAll() {
    if (cached_data_) {
        return true;  // Already cached
    }

    if (!file_ || data_size_ == 0) {
        return false;
    }

    cached_data_ = new (std::nothrow) uint8_t[data_size_];
    if (!cached_data_) {
        fprintf(stderr, "MixFile: Failed to allocate %u bytes for cache\n", data_size_);
        return false;
    }

    if (Platform_File_Seek(file_, data_start_, PLATFORM_SEEK_SET) != 0) {
        delete[] cached_data_;
        cached_data_ = nullptr;
        return false;
    }

    int32_t read = Platform_File_Read(file_, cached_data_, data_size_);
    if (read != static_cast<int32_t>(data_size_)) {
        fprintf(stderr, "MixFile: Cache read failed, got %d of %u bytes\n",
                read, data_size_);
        delete[] cached_data_;
        cached_data_ = nullptr;
        return false;
    }

    cached_size_ = data_size_;
    return true;
}

void MixFile::FreeCache() {
    if (cached_data_) {
        delete[] cached_data_;
        cached_data_ = nullptr;
        cached_size_ = 0;
    }
}

const uint8_t* MixFile::GetCachedFile(const char* filename, uint32_t* out_size) const {
    if (!cached_data_) {
        if (out_size) *out_size = 0;
        return nullptr;
    }

    int32_t crc = static_cast<int32_t>(Platform_CRC_ForMixLookup(filename));
    const MixEntry* entry = FindByCRC(crc);
    if (!entry) {
        if (out_size) *out_size = 0;
        return nullptr;
    }

    if (out_size) *out_size = entry->size;
    return cached_data_ + entry->offset;
}
```

### src/test_mix_lookup.cpp (NEW)
```cpp
// Test MIX file data retrieval
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "game/mix_file.h"
#include "platform.h"

bool test_load_file(MixFile& mix, const char* filename) {
    printf("Testing load of '%s'...\n", filename);

    // Check if file exists
    if (!mix.Contains(filename)) {
        printf("  SKIP: File not found in MIX\n");
        return true;  // Not a failure, just skip
    }

    // Get size
    uint32_t size = mix.GetFileSize(filename);
    printf("  Size: %u bytes\n", size);
    if (size == 0) {
        printf("  FAIL: Size is 0 for existing file\n");
        return false;
    }

    // Load file
    uint32_t loaded_size = 0;
    uint8_t* data = mix.LoadFile(filename, &loaded_size);
    if (!data) {
        printf("  FAIL: LoadFile returned null\n");
        return false;
    }
    if (loaded_size != size) {
        printf("  FAIL: Loaded size %u != expected %u\n", loaded_size, size);
        delete[] data;
        return false;
    }

    // Simple validation - check not all zeros or all 0xFF
    bool all_zero = true;
    bool all_ff = true;
    for (uint32_t i = 0; i < size && (all_zero || all_ff); i++) {
        if (data[i] != 0) all_zero = false;
        if (data[i] != 0xFF) all_ff = false;
    }
    if (all_zero || all_ff) {
        printf("  WARNING: Data appears to be all %s\n", all_zero ? "zeros" : "0xFF");
    }

    // Show first bytes
    printf("  First bytes: ");
    for (uint32_t i = 0; i < 16 && i < size; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");

    delete[] data;
    printf("  PASS\n");
    return true;
}

bool test_load_into_buffer(MixFile& mix, const char* filename) {
    if (!mix.Contains(filename)) {
        return true;  // Skip
    }

    printf("Testing LoadFileInto for '%s'...\n", filename);

    uint32_t size = mix.GetFileSize(filename);
    uint8_t* buffer = new uint8_t[size];

    uint32_t read = mix.LoadFileInto(filename, buffer, size);
    if (read != size) {
        printf("  FAIL: Read %u bytes, expected %u\n", read, size);
        delete[] buffer;
        return false;
    }

    // Compare with LoadFile result
    uint32_t loaded_size;
    uint8_t* loaded = mix.LoadFile(filename, &loaded_size);
    if (!loaded) {
        printf("  FAIL: LoadFile failed for comparison\n");
        delete[] buffer;
        return false;
    }

    if (memcmp(buffer, loaded, size) != 0) {
        printf("  FAIL: Data mismatch between LoadFile and LoadFileInto\n");
        delete[] buffer;
        delete[] loaded;
        return false;
    }

    delete[] buffer;
    delete[] loaded;
    printf("  PASS\n");
    return true;
}

bool test_partial_read(MixFile& mix, const char* filename) {
    if (!mix.Contains(filename)) {
        return true;  // Skip
    }

    uint32_t size = mix.GetFileSize(filename);
    if (size < 100) {
        return true;  // Skip small files
    }

    printf("Testing partial read for '%s'...\n", filename);

    // Read middle portion
    uint32_t offset = 50;
    uint32_t count = 20;
    uint8_t partial[20];

    uint32_t read = mix.ReadFilePartial(filename, offset, partial, count);
    if (read != count) {
        printf("  FAIL: Read %u bytes, expected %u\n", read, count);
        return false;
    }

    // Verify against full load
    uint8_t* full = mix.LoadFile(filename, nullptr);
    if (!full) {
        printf("  FAIL: Full load failed\n");
        return false;
    }

    if (memcmp(partial, full + offset, count) != 0) {
        printf("  FAIL: Partial read data mismatch\n");
        delete[] full;
        return false;
    }

    delete[] full;
    printf("  PASS\n");
    return true;
}

bool test_caching(MixFile& mix, const char* filename) {
    if (!mix.Contains(filename)) {
        return true;
    }

    printf("Testing caching...\n");

    // Cache all data
    if (!mix.CacheAll()) {
        printf("  FAIL: CacheAll failed\n");
        return false;
    }

    if (!mix.IsCached()) {
        printf("  FAIL: IsCached returns false after CacheAll\n");
        return false;
    }

    // Get cached pointer
    uint32_t cached_size;
    const uint8_t* cached = mix.GetCachedFile(filename, &cached_size);
    if (!cached) {
        printf("  FAIL: GetCachedFile returned null\n");
        return false;
    }

    // Compare with disk load (need to free cache first to force disk read)
    // Actually, LoadFile should use cache when available
    uint32_t loaded_size;
    uint8_t* loaded = mix.LoadFile(filename, &loaded_size);
    if (!loaded) {
        printf("  FAIL: LoadFile after cache failed\n");
        return false;
    }

    if (loaded_size != cached_size) {
        printf("  FAIL: Size mismatch\n");
        delete[] loaded;
        return false;
    }

    if (memcmp(cached, loaded, cached_size) != 0) {
        printf("  FAIL: Data mismatch\n");
        delete[] loaded;
        return false;
    }

    delete[] loaded;
    mix.FreeCache();

    if (mix.IsCached()) {
        printf("  FAIL: IsCached returns true after FreeCache\n");
        return false;
    }

    printf("  PASS\n");
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <mixfile.MIX>\n", argv[0]);
        return 1;
    }

    const char* mixpath = argv[1];

    printf("=== MIX File Lookup Test ===\n\n");

    if (Platform_Init() != 0) {
        fprintf(stderr, "Failed to initialize platform\n");
        return 1;
    }

    MixFile mix;
    if (!mix.Open(mixpath)) {
        fprintf(stderr, "Failed to open: %s\n", mixpath);
        Platform_Shutdown();
        return 1;
    }

    printf("Opened: %s (%d files)\n\n", mixpath, mix.GetFileCount());

    // Test files that might exist
    const char* test_files[] = {
        "RULES.INI",
        "TEMPERAT.PAL",
        "MOUSE.SHP",
        "CONQUER.ENG",
        nullptr
    };

    bool all_passed = true;

    for (int i = 0; test_files[i]; i++) {
        if (!test_load_file(mix, test_files[i])) {
            all_passed = false;
        }
        printf("\n");
    }

    // Pick first file that exists for additional tests
    const char* existing_file = nullptr;
    for (int i = 0; test_files[i]; i++) {
        if (mix.Contains(test_files[i])) {
            existing_file = test_files[i];
            break;
        }
    }

    // If no known files, use first entry
    if (!existing_file && mix.GetFileCount() > 0) {
        // We can't get filename from entry, but we can test by CRC
        printf("Note: No known files found, testing by entry index\n");
    }

    if (existing_file) {
        if (!test_load_into_buffer(mix, existing_file)) all_passed = false;
        if (!test_partial_read(mix, existing_file)) all_passed = false;
        if (!test_caching(mix, existing_file)) all_passed = false;
    }

    mix.Close();
    Platform_Shutdown();

    printf("\n=== %s ===\n", all_passed ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    return all_passed ? 0 : 1;
}
```

### CMakeLists.txt (MODIFY)
```cmake
# MIX lookup test
add_executable(TestMixLookup src/test_mix_lookup.cpp)
target_link_libraries(TestMixLookup PRIVATE game_mix)
```

## Verification Command
```bash
ralph-tasks/verify/check-mix-lookup.sh
```

## Verification Script

### ralph-tasks/verify/check-mix-lookup.sh (NEW)
```bash
#!/bin/bash
# Verification script for MIX file data lookup

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking MIX Data Lookup ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Build
echo "[1/4] Building..."
if ! cmake --build build --target TestMixLookup 2>&1; then
    echo "VERIFY_FAILED: Build failed"
    exit 1
fi
echo "  OK: Build successful"

# Step 2: Find test file
echo "[2/4] Finding test MIX file..."
if [ -f "gamedata/REDALERT.MIX" ]; then
    TEST_MIX="gamedata/REDALERT.MIX"
elif [ -f "gamedata/SETUP.MIX" ]; then
    TEST_MIX="gamedata/SETUP.MIX"
else
    echo "VERIFY_WARNING: No MIX files found"
    echo "VERIFY_SUCCESS (build only)"
    exit 0
fi
echo "  OK: Using $TEST_MIX"

# Step 3: Run lookup test
echo "[3/4] Running lookup test..."
if ! ./build/TestMixLookup "$TEST_MIX"; then
    echo "VERIFY_FAILED: Lookup test failed"
    exit 1
fi
echo "  OK: Lookup test passed"

# Step 4: Verify data is non-empty
echo "[4/4] Verifying data retrieval..."
OUTPUT=$(./build/TestMixLookup "$TEST_MIX" 2>&1)
if echo "$OUTPUT" | grep -q "FAIL:"; then
    echo "VERIFY_FAILED: Test reported failures"
    exit 1
fi
echo "  OK: No failures detected"

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Update `include/game/mix_file.h` with new methods
2. Update `src/game/mix_file.cpp` with implementations
3. Update constructor, destructor, move operations for cache
4. Create `src/test_mix_lookup.cpp`
5. Update `CMakeLists.txt`
6. Build: `cmake --build build --target TestMixLookup`
7. Test: `./build/TestMixLookup gamedata/REDALERT.MIX`
8. Run verification script

## Success Criteria
- File data loads correctly (matches expected size)
- LoadFile and LoadFileInto produce identical data
- Partial reads work correctly
- Caching loads all data and serves reads from memory
- FreeCache releases memory properly
- Non-existent files return null/0 gracefully

## Common Issues

### Data offset incorrect
- Verify `data_start_` is calculated correctly in ParseHeader
- Entry offset is RELATIVE to data_start_, not absolute

### Read returns wrong data
- Check seek offset calculation: `data_start_ + entry->offset`
- Verify endianness of offset field

### Cache fails for large files
- May need to check available memory
- Consider partial caching for very large MIX files

### Memory leak
- Ensure delete[] for all allocated buffers
- Verify move operations transfer ownership correctly

## Completion Promise
When verification passes, output:
```
<promise>TASK_12C_COMPLETE</promise>
```

## Escape Hatch
If stuck after 15 iterations:
- Document what's blocking in `ralph-tasks/blocked/12c.md`
- Output: `<promise>TASK_12C_BLOCKED</promise>`

## Max Iterations
15
