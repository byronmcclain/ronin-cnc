# Task 12b: MIX File Header Parsing

## Dependencies
- Task 12a (CRC FFI) must be complete
- Platform file I/O functions available

## Context
MIX files are Westwood's archive format containing all game assets. Before we can access any files within a MIX archive, we must parse its header to build an index of contained files. This task implements the header parsing logic, handling both standard and extended (encrypted/signed) MIX formats.

## Objective
Create a MixFile class in C++ that:
1. Opens a MIX file from disk
2. Parses the header (standard and extended formats)
3. Loads the file index (SubBlock entries) into memory
4. Provides file count and validation

## Technical Background

### MIX Header Formats

#### Standard Format (Red Alert 1)
```
Offset  Size  Type      Description
------  ----  --------  -----------
0x0000  2     uint16    File count (number of SubBlocks)
0x0002  4     uint32    Total data size (sum of all embedded file sizes)
0x0006  12*N  SubBlock  Array of N file entries, sorted by CRC
???     ???   raw       Concatenated file data
```

#### Extended Format (with encryption/digest)
```
Offset  Size  Type      Description
------  ----  --------  -----------
0x0000  2     uint16    Zero marker (0x0000) - indicates extended format
0x0002  2     uint16    Flags:
                        - Bit 0 (0x01): Has SHA-1 digest at end
                        - Bit 1 (0x02): Header is RSA encrypted
0x0004  ...   varies    If encrypted: encrypted header block
                        If not encrypted: standard FileHeader + SubBlocks
```

### SubBlock Entry (12 bytes each)
```
Offset  Size  Type    Description
------  ----  ------  -----------
0x0000  4     int32   CRC32 of uppercase filename (signed for sorting!)
0x0004  4     uint32  Offset from start of data section
0x0008  4     uint32  Size of file in bytes
```

### Critical Implementation Details

1. **CRC is signed int32**: SubBlocks are sorted by CRC as a SIGNED value. This affects binary search.

2. **Extended format detection**: If first 2 bytes are 0x0000, it's extended format.

3. **Data offset calculation**: After reading header + entries, the current file position marks where data begins.

4. **Encrypted headers**: We will NOT support encrypted MIX files in this implementation. Most game data MIX files are not encrypted.

## Deliverables
- [ ] `include/game/mix_file.h` - MixFile class declaration
- [ ] `src/game/mix_file.cpp` - MixFile implementation
- [ ] `src/test_mix_header.cpp` - Test program
- [ ] CMake integration
- [ ] Verification script passes

## Files to Create

### include/game/mix_file.h (NEW)
```cpp
#ifndef MIX_FILE_H
#define MIX_FILE_H

#include <cstdint>
#include <vector>
#include <string>
#include "platform.h"

/// Entry for a single file within a MIX archive
/// Matches original Westwood SubBlock structure
struct MixEntry {
    int32_t  crc;       // CRC32 of uppercase filename (SIGNED for sort order!)
    uint32_t offset;    // Offset from start of data section
    uint32_t size;      // Size of file in bytes

    // Comparison for binary search (signed comparison!)
    bool operator<(const MixEntry& other) const {
        return crc < other.crc;
    }
};

/// MIX archive file reader
///
/// Handles parsing MIX file headers and provides access to contained files.
/// Does not cache file data - reads on demand from disk.
class MixFile {
public:
    /// Construct MixFile from path
    /// Does NOT open the file - call Open() first
    MixFile();
    explicit MixFile(const char* path);
    ~MixFile();

    // Non-copyable
    MixFile(const MixFile&) = delete;
    MixFile& operator=(const MixFile&) = delete;

    // Movable
    MixFile(MixFile&& other) noexcept;
    MixFile& operator=(MixFile&& other) noexcept;

    /// Open and parse a MIX file
    /// @param path Path to MIX file
    /// @return true if successfully opened and parsed
    bool Open(const char* path);

    /// Close the MIX file
    void Close();

    /// Check if file is open and valid
    bool IsValid() const { return is_valid_; }
    bool IsOpen() const { return file_ != nullptr; }

    /// Get number of files in archive
    int GetFileCount() const { return static_cast<int>(entries_.size()); }

    /// Get total data size (sum of all file sizes)
    uint32_t GetDataSize() const { return data_size_; }

    /// Get offset where data section begins
    uint32_t GetDataStart() const { return data_start_; }

    /// Check if MIX uses extended format
    bool IsExtended() const { return is_extended_; }

    /// Check if header was encrypted (we don't support this)
    bool IsEncrypted() const { return is_encrypted_; }

    /// Check if MIX has SHA digest
    bool HasDigest() const { return has_digest_; }

    /// Get path to MIX file
    const char* GetPath() const { return path_.c_str(); }

    /// Get entry by index (for enumeration)
    /// @param index Entry index [0, GetFileCount())
    /// @return Pointer to entry or nullptr if out of range
    const MixEntry* GetEntry(int index) const;

    /// Find entry by CRC (uses binary search)
    /// @param crc CRC32 of uppercase filename
    /// @return Pointer to entry or nullptr if not found
    const MixEntry* FindByCRC(int32_t crc) const;

    /// Check if file exists by name
    /// @param filename Filename to look up (case insensitive)
    bool Contains(const char* filename) const;

    /// Debug: print all entries
    void DumpEntries() const;

private:
    bool ParseHeader();
    bool ParseStandardHeader();
    bool ParseExtendedHeader();
    bool ReadEntries(int count);

    PlatformFile* file_;
    std::string path_;
    std::vector<MixEntry> entries_;

    uint32_t data_size_;    // Total size of all files
    uint32_t data_start_;   // File offset where data begins

    bool is_valid_;
    bool is_extended_;
    bool is_encrypted_;
    bool has_digest_;
};

#endif // MIX_FILE_H
```

### src/game/mix_file.cpp (NEW)
```cpp
#include "game/mix_file.h"
#include <cstring>
#include <algorithm>
#include <cstdio>

// Header structures matching original Westwood format
#pragma pack(push, 1)
struct StandardHeader {
    uint16_t count;  // Number of files
    uint32_t size;   // Total data size
};

struct ExtendedHeader {
    uint16_t zero;   // Always 0 for extended format
    uint16_t flags;  // Bit 0: digest, Bit 1: encrypted
};

struct RawSubBlock {
    int32_t  crc;    // CRC32 (signed!)
    uint32_t offset;
    uint32_t size;
};
#pragma pack(pop)

static_assert(sizeof(StandardHeader) == 6, "StandardHeader must be 6 bytes");
static_assert(sizeof(ExtendedHeader) == 4, "ExtendedHeader must be 4 bytes");
static_assert(sizeof(RawSubBlock) == 12, "RawSubBlock must be 12 bytes");

MixFile::MixFile()
    : file_(nullptr)
    , data_size_(0)
    , data_start_(0)
    , is_valid_(false)
    , is_extended_(false)
    , is_encrypted_(false)
    , has_digest_(false)
{
}

MixFile::MixFile(const char* path)
    : MixFile()
{
    Open(path);
}

MixFile::~MixFile() {
    Close();
}

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
{
    other.file_ = nullptr;
    other.is_valid_ = false;
}

MixFile& MixFile::operator=(MixFile&& other) noexcept {
    if (this != &other) {
        Close();
        file_ = other.file_;
        path_ = std::move(other.path_);
        entries_ = std::move(other.entries_);
        data_size_ = other.data_size_;
        data_start_ = other.data_start_;
        is_valid_ = other.is_valid_;
        is_extended_ = other.is_extended_;
        is_encrypted_ = other.is_encrypted_;
        has_digest_ = other.has_digest_;

        other.file_ = nullptr;
        other.is_valid_ = false;
    }
    return *this;
}

bool MixFile::Open(const char* path) {
    Close();

    if (!path || !path[0]) {
        return false;
    }

    path_ = path;
    file_ = Platform_File_Open(path, PLATFORM_FILE_READ);

    if (!file_) {
        fprintf(stderr, "MixFile: Failed to open '%s'\n", path);
        return false;
    }

    if (!ParseHeader()) {
        Close();
        return false;
    }

    is_valid_ = true;
    return true;
}

void MixFile::Close() {
    if (file_) {
        Platform_File_Close(file_);
        file_ = nullptr;
    }
    entries_.clear();
    data_size_ = 0;
    data_start_ = 0;
    is_valid_ = false;
    is_extended_ = false;
    is_encrypted_ = false;
    has_digest_ = false;
}

bool MixFile::ParseHeader() {
    // Read first 4 bytes to detect format
    ExtendedHeader ext;
    if (Platform_File_Read(file_, &ext, sizeof(ext)) != sizeof(ext)) {
        fprintf(stderr, "MixFile: Failed to read header from '%s'\n", path_.c_str());
        return false;
    }

    // Check for extended format (first 2 bytes are 0)
    if (ext.zero == 0) {
        is_extended_ = true;
        has_digest_ = (ext.flags & 0x01) != 0;
        is_encrypted_ = (ext.flags & 0x02) != 0;

        if (is_encrypted_) {
            fprintf(stderr, "MixFile: Encrypted MIX files not supported ('%s')\n", path_.c_str());
            return false;
        }

        return ParseExtendedHeader();
    } else {
        // Standard format - first 2 bytes are file count
        is_extended_ = false;
        return ParseStandardHeader();
    }
}

bool MixFile::ParseStandardHeader() {
    // Seek back to start
    Platform_File_Seek(file_, 0, PLATFORM_SEEK_SET);

    StandardHeader hdr;
    if (Platform_File_Read(file_, &hdr, sizeof(hdr)) != sizeof(hdr)) {
        fprintf(stderr, "MixFile: Failed to read standard header\n");
        return false;
    }

    if (hdr.count == 0 || hdr.count > 10000) {
        fprintf(stderr, "MixFile: Invalid file count %u\n", hdr.count);
        return false;
    }

    data_size_ = hdr.size;

    if (!ReadEntries(hdr.count)) {
        return false;
    }

    // Data starts immediately after header + entries
    data_start_ = sizeof(StandardHeader) + (hdr.count * sizeof(RawSubBlock));
    return true;
}

bool MixFile::ParseExtendedHeader() {
    // We've already read the extended header marker
    // Now read the standard header that follows
    StandardHeader hdr;
    if (Platform_File_Read(file_, &hdr, sizeof(hdr)) != sizeof(hdr)) {
        fprintf(stderr, "MixFile: Failed to read extended standard header\n");
        return false;
    }

    if (hdr.count == 0 || hdr.count > 10000) {
        fprintf(stderr, "MixFile: Invalid file count %u\n", hdr.count);
        return false;
    }

    data_size_ = hdr.size;

    if (!ReadEntries(hdr.count)) {
        return false;
    }

    // Data starts after extended header + standard header + entries
    data_start_ = sizeof(ExtendedHeader) + sizeof(StandardHeader) + (hdr.count * sizeof(RawSubBlock));
    return true;
}

bool MixFile::ReadEntries(int count) {
    entries_.clear();
    entries_.reserve(count);

    for (int i = 0; i < count; i++) {
        RawSubBlock raw;
        if (Platform_File_Read(file_, &raw, sizeof(raw)) != sizeof(raw)) {
            fprintf(stderr, "MixFile: Failed to read entry %d\n", i);
            return false;
        }

        MixEntry entry;
        entry.crc = raw.crc;      // Keep as signed!
        entry.offset = raw.offset;
        entry.size = raw.size;
        entries_.push_back(entry);
    }

    // Entries should already be sorted by CRC, but verify
    // Note: Must use signed comparison!
    bool sorted = std::is_sorted(entries_.begin(), entries_.end(),
        [](const MixEntry& a, const MixEntry& b) {
            return a.crc < b.crc;
        });

    if (!sorted) {
        fprintf(stderr, "MixFile: Warning - entries not sorted by CRC, sorting now\n");
        std::sort(entries_.begin(), entries_.end());
    }

    return true;
}

const MixEntry* MixFile::GetEntry(int index) const {
    if (index < 0 || index >= static_cast<int>(entries_.size())) {
        return nullptr;
    }
    return &entries_[index];
}

const MixEntry* MixFile::FindByCRC(int32_t crc) const {
    // Binary search using signed comparison
    auto it = std::lower_bound(entries_.begin(), entries_.end(), crc,
        [](const MixEntry& entry, int32_t c) {
            return entry.crc < c;
        });

    if (it != entries_.end() && it->crc == crc) {
        return &(*it);
    }
    return nullptr;
}

bool MixFile::Contains(const char* filename) const {
    if (!filename) return false;

    int32_t crc = static_cast<int32_t>(Platform_CRC_ForMixLookup(filename));
    return FindByCRC(crc) != nullptr;
}

void MixFile::DumpEntries() const {
    printf("MIX File: %s\n", path_.c_str());
    printf("  Format: %s\n", is_extended_ ? "Extended" : "Standard");
    printf("  Files: %d\n", GetFileCount());
    printf("  Data Size: %u bytes\n", data_size_);
    printf("  Data Start: 0x%08X\n", data_start_);
    printf("  Encrypted: %s\n", is_encrypted_ ? "Yes" : "No");
    printf("  Digest: %s\n", has_digest_ ? "Yes" : "No");
    printf("\n  Entries:\n");
    printf("  %-4s  %-12s  %-10s  %-10s\n", "Idx", "CRC", "Offset", "Size");
    printf("  %-4s  %-12s  %-10s  %-10s\n", "---", "---", "------", "----");

    for (size_t i = 0; i < entries_.size() && i < 20; i++) {
        const auto& e = entries_[i];
        printf("  %-4zu  0x%08X  0x%08X  %10u\n",
               i, static_cast<uint32_t>(e.crc), e.offset, e.size);
    }

    if (entries_.size() > 20) {
        printf("  ... and %zu more entries\n", entries_.size() - 20);
    }
}
```

### src/test_mix_header.cpp (NEW)
```cpp
// Test MIX file header parsing
#include <cstdio>
#include <cstdlib>
#include "game/mix_file.h"
#include "platform.h"

void print_usage(const char* prog) {
    fprintf(stderr, "Usage: %s <mixfile.MIX>\n", prog);
    fprintf(stderr, "  Parses and displays MIX file header information\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char* mixpath = argv[1];

    printf("=== MIX Header Parser Test ===\n\n");

    // Initialize platform (for file I/O)
    if (Platform_Init() != 0) {
        fprintf(stderr, "Failed to initialize platform\n");
        return 1;
    }

    MixFile mix;
    if (!mix.Open(mixpath)) {
        fprintf(stderr, "Failed to open MIX file: %s\n", mixpath);
        Platform_Shutdown();
        return 1;
    }

    // Dump header info
    mix.DumpEntries();

    // Test: Find first entry by CRC
    if (mix.GetFileCount() > 0) {
        const MixEntry* first = mix.GetEntry(0);
        printf("\nTest: Find first entry by CRC...\n");
        const MixEntry* found = mix.FindByCRC(first->crc);
        if (found) {
            printf("  PASS: Found entry with CRC 0x%08X\n",
                   static_cast<uint32_t>(first->crc));
        } else {
            printf("  FAIL: Could not find entry with CRC 0x%08X\n",
                   static_cast<uint32_t>(first->crc));
        }
    }

    // Test: Contains check for non-existent file
    printf("\nTest: Contains check for non-existent file...\n");
    if (!mix.Contains("THIS_FILE_DOES_NOT_EXIST_12345.XYZ")) {
        printf("  PASS: Non-existent file correctly not found\n");
    } else {
        printf("  FAIL: Non-existent file incorrectly reported as found\n");
    }

    // Test: Check some known filenames (may or may not exist)
    const char* test_files[] = {
        "RULES.INI",
        "MOUSE.SHP",
        "TEMPERAT.PAL",
        "PALETTE.PAL",
        nullptr
    };

    printf("\nTest: Check known filenames...\n");
    for (int i = 0; test_files[i]; i++) {
        bool found = mix.Contains(test_files[i]);
        printf("  %s: %s\n", test_files[i], found ? "FOUND" : "not found");
    }

    mix.Close();
    Platform_Shutdown();

    printf("\n=== MIX Header Test Complete ===\n");
    return 0;
}
```

### CMakeLists.txt (MODIFY)
Add to CMakeLists.txt:

```cmake
# MIX file library (game assets)
add_library(game_mix STATIC
    src/game/mix_file.cpp
)
target_include_directories(game_mix PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(game_mix PUBLIC redalert_platform)

# MIX header test
add_executable(TestMixHeader src/test_mix_header.cpp)
target_link_libraries(TestMixHeader PRIVATE game_mix)
```

## Verification Command
```bash
ralph-tasks/verify/check-mix-header.sh
```

## Verification Script

### ralph-tasks/verify/check-mix-header.sh (NEW)
```bash
#!/bin/bash
# Verification script for MIX file header parsing

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking MIX Header Parsing ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check source files exist
echo "[1/5] Checking source files..."
if [ ! -f "include/game/mix_file.h" ]; then
    echo "VERIFY_FAILED: include/game/mix_file.h not found"
    exit 1
fi
if [ ! -f "src/game/mix_file.cpp" ]; then
    echo "VERIFY_FAILED: src/game/mix_file.cpp not found"
    exit 1
fi
echo "  OK: Source files exist"

# Step 2: Build
echo "[2/5] Building..."
if ! cmake --build build --target game_mix 2>&1; then
    echo "VERIFY_FAILED: game_mix build failed"
    exit 1
fi
if ! cmake --build build --target TestMixHeader 2>&1; then
    echo "VERIFY_FAILED: TestMixHeader build failed"
    exit 1
fi
echo "  OK: Build successful"

# Step 3: Check for test MIX file
echo "[3/5] Checking for test MIX file..."
if [ -f "gamedata/REDALERT.MIX" ]; then
    TEST_MIX="gamedata/REDALERT.MIX"
elif [ -f "gamedata/SETUP.MIX" ]; then
    TEST_MIX="gamedata/SETUP.MIX"
else
    echo "VERIFY_WARNING: No MIX files found in gamedata/"
    echo "  Skipping runtime test - only build verified"
    echo ""
    echo "VERIFY_SUCCESS (build only)"
    exit 0
fi
echo "  OK: Using $TEST_MIX"

# Step 4: Run header test
echo "[4/5] Running header test..."
if ! ./build/TestMixHeader "$TEST_MIX"; then
    echo "VERIFY_FAILED: TestMixHeader failed"
    exit 1
fi
echo "  OK: Header test passed"

# Step 5: Verify file count is reasonable
echo "[5/5] Verifying file count..."
FILE_COUNT=$(./build/TestMixHeader "$TEST_MIX" 2>&1 | grep "Files:" | awk '{print $2}')
if [ -z "$FILE_COUNT" ] || [ "$FILE_COUNT" -lt 1 ]; then
    echo "VERIFY_FAILED: Could not determine file count or count is 0"
    exit 1
fi
echo "  OK: MIX contains $FILE_COUNT files"

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Create directory: `mkdir -p include/game src/game`
2. Create `include/game/mix_file.h`
3. Create `src/game/mix_file.cpp`
4. Create `src/test_mix_header.cpp`
5. Update `CMakeLists.txt` to build game_mix library and test
6. Run `cmake --build build --target TestMixHeader`
7. Test with: `./build/TestMixHeader gamedata/REDALERT.MIX`
8. Run verification script

## Success Criteria
- MixFile class compiles without errors
- Opens REDALERT.MIX successfully
- Correctly detects standard vs extended format
- Reports correct file count (should be > 0)
- Entries are sorted by signed CRC
- Binary search finds existing entries
- Non-existent files return nullptr

## Common Issues

### "Platform_File_Open not found"
- Ensure platform.h is included
- Check that redalert_platform is linked

### "Invalid file count"
- Check endianness - MIX files are little-endian
- Verify file is not corrupted

### Entries not sorted correctly
- Remember CRC is SIGNED int32, not unsigned
- Binary search must use signed comparison

### Extended format fails
- Extended format has 4-byte header before standard header
- We don't support encrypted MIX files

### Data start offset is wrong
- Standard: 6 + (count * 12) bytes
- Extended: 4 + 6 + (count * 12) bytes

## Completion Promise
When verification passes, output:
```
<promise>TASK_12B_COMPLETE</promise>
```

## Escape Hatch
If stuck after 15 iterations:
- Document what's blocking in `ralph-tasks/blocked/12b.md`
- List attempted approaches
- Output: `<promise>TASK_12B_BLOCKED</promise>`

## Max Iterations
15
