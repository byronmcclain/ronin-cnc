# Task 12d: MIX File Manager (Multi-Archive Search)

## Dependencies
- Task 12c (MIX Lookup) must be complete
- Platform file I/O and CRC functions available

## Context
Red Alert uses multiple MIX files that must be searched in a specific order. The MixManager provides a unified interface for accessing files across all mounted MIX archives, implementing the search priority system from the original game.

## Objective
Create a MixManager singleton class that:
1. Maintains ordered list of mounted MIX files
2. Searches across all MIX files for requested assets
3. Handles theater-specific MIX files (TEMPERAT, SNOW, etc.)
4. Provides global access point for all asset loading

## Technical Background

### MIX File Priority (Original Game)
The original game loads MIX files in a specific order. Files in later-loaded archives take precedence:

```
Priority (highest to lowest):
1. Expansion packs (EXPAND2.MIX, EXPAND.MIX)
2. Main game data (REDALERT.MIX)
3. CD-specific content (CD1/MAIN.MIX or CD2/MAIN.MIX)
4. Theater files (TEMPERAT.MIX, SNOW.MIX, etc.)
5. Secondary data (SETUP.MIX, GENERAL.MIX)
```

### Search Order
When looking for a file:
1. Search most recently mounted MIX first
2. Continue through mounted list
3. Return first match found (allows override)

### Theater System
Each theater (terrain type) has its own MIX file:
- `TEMPERAT.MIX` - Temperate (European)
- `SNOW.MIX` - Snow/Winter
- `DESERT.MIX` - Desert (North Africa)
- `INTERIOR.MIX` - Interior/Base
- `JUNGLE.MIX` - Jungle (optional)

Theater MIX files contain:
- Terrain tiles and templates
- Theater-specific palettes
- Theater-specific sprites

## Deliverables
- [ ] `include/game/mix_manager.h` - MixManager class
- [ ] `src/game/mix_manager.cpp` - Implementation
- [ ] Global singleton access
- [ ] Test program
- [ ] Verification script

## Files to Create

### include/game/mix_manager.h (NEW)
```cpp
#ifndef MIX_MANAGER_H
#define MIX_MANAGER_H

#include "game/mix_file.h"
#include <vector>
#include <string>
#include <memory>

/// Supported theater types
enum class Theater {
    TEMPERATE,
    SNOW,
    DESERT,
    INTERIOR,
    JUNGLE,
    NONE
};

/// Convert theater enum to MIX filename
const char* Theater_ToMixName(Theater theater);

/// Parse theater name to enum
Theater Theater_FromName(const char* name);

/// MIX File Manager - Global asset access
///
/// Manages multiple MIX files and provides unified search across all archives.
/// Implements search priority (later mounted = higher priority).
///
/// Usage:
/// @code
/// MixManager& mgr = MixManager::Instance();
/// mgr.Initialize("gamedata");
/// mgr.Mount("REDALERT.MIX");
/// mgr.MountTheater(Theater::TEMPERATE);
///
/// uint32_t size;
/// uint8_t* data = mgr.LoadFile("RULES.INI", &size);
/// @endcode
class MixManager {
public:
    /// Get singleton instance
    static MixManager& Instance();

    /// Initialize manager with game data path
    /// @param base_path Path to directory containing MIX files
    /// @return true if base path is valid
    bool Initialize(const char* base_path);

    /// Shutdown and unmount all files
    void Shutdown();

    /// Check if initialized
    bool IsInitialized() const { return initialized_; }

    /// Get base path
    const char* GetBasePath() const { return base_path_.c_str(); }

    // === MIX File Mounting ===

    /// Mount a MIX file (adds to search path)
    /// Later mounted files have higher priority
    /// @param mix_name MIX filename (relative to base_path or absolute)
    /// @return true if mounted successfully
    bool Mount(const char* mix_name);

    /// Mount with explicit path
    bool MountPath(const char* full_path);

    /// Unmount a MIX file
    /// @param mix_name Name of MIX to unmount
    void Unmount(const char* mix_name);

    /// Unmount all MIX files
    void UnmountAll();

    /// Check if MIX is mounted
    bool IsMounted(const char* mix_name) const;

    /// Get number of mounted MIX files
    int GetMountedCount() const { return static_cast<int>(mounted_.size()); }

    // === Theater Management ===

    /// Mount theater-specific MIX file
    /// Unmounts any previously mounted theater first
    bool MountTheater(Theater theater);

    /// Unmount current theater
    void UnmountTheater();

    /// Get current theater
    Theater GetCurrentTheater() const { return current_theater_; }

    // === File Access ===

    /// Check if file exists in any mounted MIX
    bool Contains(const char* filename) const;

    /// Get file size (searches all mounted MIX files)
    uint32_t GetFileSize(const char* filename) const;

    /// Load file data (caller must delete[])
    /// Searches all mounted MIX files in priority order
    /// @param filename File to load
    /// @param out_size Receives file size (can be null)
    /// @return Allocated buffer or nullptr
    uint8_t* LoadFile(const char* filename, uint32_t* out_size = nullptr);

    /// Load file into provided buffer
    /// @param filename File to load
    /// @param buffer Destination buffer
    /// @param buffer_size Size of buffer
    /// @return Bytes read or 0 on failure
    uint32_t LoadFileInto(const char* filename, void* buffer, uint32_t buffer_size);

    /// Get pointer to cached file data
    /// Only works for cached MIX files
    const uint8_t* GetCachedFile(const char* filename, uint32_t* out_size) const;

    /// Find which MIX file contains a file
    /// @param filename File to search for
    /// @return MIX filename or nullptr if not found
    const char* FindContainingMix(const char* filename) const;

    // === Caching ===

    /// Cache a specific MIX file
    bool CacheMix(const char* mix_name);

    /// Cache all mounted MIX files
    void CacheAll();

    /// Free cache for specific MIX
    void FreeCacheMix(const char* mix_name);

    /// Free all caches
    void FreeAllCaches();

    // === Debug ===

    /// Print list of mounted MIX files
    void DumpMounted() const;

    /// Print all files in all mounted MIX archives
    void DumpAllFiles() const;

private:
    MixManager();
    ~MixManager();

    // Prevent copying
    MixManager(const MixManager&) = delete;
    MixManager& operator=(const MixManager&) = delete;

    /// Find MIX file by name (case insensitive)
    MixFile* FindMix(const char* mix_name) const;

    /// Build full path to MIX file
    std::string BuildPath(const char* mix_name) const;

    std::vector<std::unique_ptr<MixFile>> mounted_;
    std::string base_path_;
    Theater current_theater_;
    std::string theater_mix_name_;
    bool initialized_;
};

/// Convenience macros for common operations
#define TheMixManager MixManager::Instance()

#endif // MIX_MANAGER_H
```

### src/game/mix_manager.cpp (NEW)
```cpp
#include "game/mix_manager.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

// Theater name mapping
static const struct {
    Theater theater;
    const char* mix_name;
    const char* name;
} THEATER_INFO[] = {
    { Theater::TEMPERATE, "TEMPERAT.MIX", "TEMPERATE" },
    { Theater::SNOW,      "SNOW.MIX",     "SNOW" },
    { Theater::DESERT,    "DESERT.MIX",   "DESERT" },
    { Theater::INTERIOR,  "INTERIOR.MIX", "INTERIOR" },
    { Theater::JUNGLE,    "JUNGLE.MIX",   "JUNGLE" },
    { Theater::NONE,      nullptr,        "NONE" },
};

const char* Theater_ToMixName(Theater theater) {
    for (const auto& info : THEATER_INFO) {
        if (info.theater == theater) {
            return info.mix_name;
        }
    }
    return nullptr;
}

Theater Theater_FromName(const char* name) {
    if (!name) return Theater::NONE;

    for (const auto& info : THEATER_INFO) {
        if (info.name && strcasecmp(name, info.name) == 0) {
            return info.theater;
        }
    }
    return Theater::NONE;
}

// Singleton instance
MixManager& MixManager::Instance() {
    static MixManager instance;
    return instance;
}

MixManager::MixManager()
    : current_theater_(Theater::NONE)
    , initialized_(false)
{
}

MixManager::~MixManager() {
    Shutdown();
}

bool MixManager::Initialize(const char* base_path) {
    if (initialized_) {
        Shutdown();
    }

    if (!base_path || !base_path[0]) {
        fprintf(stderr, "MixManager: Invalid base path\n");
        return false;
    }

    base_path_ = base_path;

    // Ensure path ends with separator
    if (base_path_.back() != '/' && base_path_.back() != '\\') {
        base_path_ += '/';
    }

    initialized_ = true;
    printf("MixManager: Initialized with base path '%s'\n", base_path_.c_str());
    return true;
}

void MixManager::Shutdown() {
    UnmountAll();
    base_path_.clear();
    current_theater_ = Theater::NONE;
    theater_mix_name_.clear();
    initialized_ = false;
}

std::string MixManager::BuildPath(const char* mix_name) const {
    if (!mix_name) return "";

    // Check if already absolute path
    if (mix_name[0] == '/' || (mix_name[0] != '\0' && mix_name[1] == ':')) {
        return mix_name;
    }

    return base_path_ + mix_name;
}

MixFile* MixManager::FindMix(const char* mix_name) const {
    if (!mix_name) return nullptr;

    for (const auto& mix : mounted_) {
        // Compare just the filename part
        const char* mounted_name = mix->GetPath();

        // Get basename of mounted path
        const char* mounted_base = strrchr(mounted_name, '/');
        if (!mounted_base) mounted_base = strrchr(mounted_name, '\\');
        if (mounted_base) mounted_base++;
        else mounted_base = mounted_name;

        if (strcasecmp(mounted_base, mix_name) == 0) {
            return mix.get();
        }
    }
    return nullptr;
}

bool MixManager::Mount(const char* mix_name) {
    return MountPath(BuildPath(mix_name).c_str());
}

bool MixManager::MountPath(const char* full_path) {
    if (!initialized_) {
        fprintf(stderr, "MixManager: Not initialized\n");
        return false;
    }

    if (!full_path || !full_path[0]) {
        return false;
    }

    // Get basename for duplicate check
    const char* basename = strrchr(full_path, '/');
    if (!basename) basename = strrchr(full_path, '\\');
    if (basename) basename++;
    else basename = full_path;

    // Check if already mounted
    if (FindMix(basename)) {
        printf("MixManager: '%s' already mounted\n", basename);
        return true;
    }

    auto mix = std::make_unique<MixFile>();
    if (!mix->Open(full_path)) {
        fprintf(stderr, "MixManager: Failed to mount '%s'\n", full_path);
        return false;
    }

    printf("MixManager: Mounted '%s' (%d files)\n", basename, mix->GetFileCount());
    mounted_.push_back(std::move(mix));
    return true;
}

void MixManager::Unmount(const char* mix_name) {
    if (!mix_name) return;

    auto it = std::remove_if(mounted_.begin(), mounted_.end(),
        [mix_name](const std::unique_ptr<MixFile>& mix) {
            const char* path = mix->GetPath();
            const char* base = strrchr(path, '/');
            if (!base) base = strrchr(path, '\\');
            if (base) base++;
            else base = path;
            return strcasecmp(base, mix_name) == 0;
        });

    if (it != mounted_.end()) {
        printf("MixManager: Unmounted '%s'\n", mix_name);
        mounted_.erase(it, mounted_.end());
    }
}

void MixManager::UnmountAll() {
    mounted_.clear();
    current_theater_ = Theater::NONE;
    theater_mix_name_.clear();
}

bool MixManager::IsMounted(const char* mix_name) const {
    return FindMix(mix_name) != nullptr;
}

bool MixManager::MountTheater(Theater theater) {
    if (theater == Theater::NONE) {
        UnmountTheater();
        return true;
    }

    const char* mix_name = Theater_ToMixName(theater);
    if (!mix_name) {
        fprintf(stderr, "MixManager: Invalid theater\n");
        return false;
    }

    // Unmount current theater if different
    if (!theater_mix_name_.empty() && strcasecmp(theater_mix_name_.c_str(), mix_name) != 0) {
        UnmountTheater();
    }

    if (!Mount(mix_name)) {
        return false;
    }

    current_theater_ = theater;
    theater_mix_name_ = mix_name;
    return true;
}

void MixManager::UnmountTheater() {
    if (!theater_mix_name_.empty()) {
        Unmount(theater_mix_name_.c_str());
        theater_mix_name_.clear();
    }
    current_theater_ = Theater::NONE;
}

bool MixManager::Contains(const char* filename) const {
    if (!filename) return false;

    // Search in reverse order (later mounted = higher priority)
    for (auto it = mounted_.rbegin(); it != mounted_.rend(); ++it) {
        if ((*it)->Contains(filename)) {
            return true;
        }
    }
    return false;
}

uint32_t MixManager::GetFileSize(const char* filename) const {
    if (!filename) return 0;

    for (auto it = mounted_.rbegin(); it != mounted_.rend(); ++it) {
        uint32_t size = (*it)->GetFileSize(filename);
        if (size > 0) {
            return size;
        }
    }
    return 0;
}

uint8_t* MixManager::LoadFile(const char* filename, uint32_t* out_size) {
    if (!filename) {
        if (out_size) *out_size = 0;
        return nullptr;
    }

    for (auto it = mounted_.rbegin(); it != mounted_.rend(); ++it) {
        if ((*it)->Contains(filename)) {
            return (*it)->LoadFile(filename, out_size);
        }
    }

    if (out_size) *out_size = 0;
    return nullptr;
}

uint32_t MixManager::LoadFileInto(const char* filename, void* buffer, uint32_t buffer_size) {
    if (!filename || !buffer || buffer_size == 0) {
        return 0;
    }

    for (auto it = mounted_.rbegin(); it != mounted_.rend(); ++it) {
        if ((*it)->Contains(filename)) {
            return (*it)->LoadFileInto(filename, buffer, buffer_size);
        }
    }
    return 0;
}

const uint8_t* MixManager::GetCachedFile(const char* filename, uint32_t* out_size) const {
    if (!filename) {
        if (out_size) *out_size = 0;
        return nullptr;
    }

    for (auto it = mounted_.rbegin(); it != mounted_.rend(); ++it) {
        if ((*it)->IsCached() && (*it)->Contains(filename)) {
            return (*it)->GetCachedFile(filename, out_size);
        }
    }

    if (out_size) *out_size = 0;
    return nullptr;
}

const char* MixManager::FindContainingMix(const char* filename) const {
    if (!filename) return nullptr;

    for (auto it = mounted_.rbegin(); it != mounted_.rend(); ++it) {
        if ((*it)->Contains(filename)) {
            return (*it)->GetPath();
        }
    }
    return nullptr;
}

bool MixManager::CacheMix(const char* mix_name) {
    MixFile* mix = FindMix(mix_name);
    if (!mix) return false;
    return mix->CacheAll();
}

void MixManager::CacheAll() {
    for (auto& mix : mounted_) {
        mix->CacheAll();
    }
}

void MixManager::FreeCacheMix(const char* mix_name) {
    MixFile* mix = FindMix(mix_name);
    if (mix) {
        mix->FreeCache();
    }
}

void MixManager::FreeAllCaches() {
    for (auto& mix : mounted_) {
        mix->FreeCache();
    }
}

void MixManager::DumpMounted() const {
    printf("MixManager: %d MIX files mounted\n", GetMountedCount());
    printf("  Base path: %s\n", base_path_.c_str());
    printf("  Current theater: %s\n",
           current_theater_ != Theater::NONE ? theater_mix_name_.c_str() : "None");
    printf("\n  Mounted (search order, last searched first):\n");

    int idx = 0;
    for (auto it = mounted_.rbegin(); it != mounted_.rend(); ++it, ++idx) {
        const auto& mix = *it;
        printf("    %d. %s (%d files, %s)\n",
               idx + 1,
               mix->GetPath(),
               mix->GetFileCount(),
               mix->IsCached() ? "cached" : "disk");
    }
}

void MixManager::DumpAllFiles() const {
    printf("=== All Files in Mounted MIX Archives ===\n");
    for (const auto& mix : mounted_) {
        printf("\n--- %s ---\n", mix->GetPath());
        mix->DumpEntries();
    }
}
```

### src/test_mix_manager.cpp (NEW)
```cpp
// Test MIX Manager functionality
#include <cstdio>
#include <cstring>
#include "game/mix_manager.h"
#include "platform.h"

int main(int argc, char* argv[]) {
    const char* base_path = "gamedata";
    if (argc > 1) {
        base_path = argv[1];
    }

    printf("=== MIX Manager Test ===\n\n");

    if (Platform_Init() != 0) {
        fprintf(stderr, "Failed to initialize platform\n");
        return 1;
    }

    MixManager& mgr = MixManager::Instance();

    // Test 1: Initialize
    printf("Test 1: Initialize...\n");
    if (!mgr.Initialize(base_path)) {
        fprintf(stderr, "  FAIL: Initialize failed\n");
        Platform_Shutdown();
        return 1;
    }
    printf("  PASS: Initialized with '%s'\n\n", mgr.GetBasePath());

    // Test 2: Mount MIX files
    printf("Test 2: Mount MIX files...\n");

    const char* mix_files[] = {
        "REDALERT.MIX",
        "SETUP.MIX",
        "CD1/MAIN.MIX",
        "CD2/MAIN.MIX",
        nullptr
    };

    int mounted = 0;
    for (int i = 0; mix_files[i]; i++) {
        if (mgr.Mount(mix_files[i])) {
            printf("  Mounted: %s\n", mix_files[i]);
            mounted++;
        }
    }

    if (mounted == 0) {
        fprintf(stderr, "  FAIL: No MIX files could be mounted\n");
        mgr.Shutdown();
        Platform_Shutdown();
        return 1;
    }
    printf("  PASS: %d MIX files mounted\n\n", mounted);

    // Test 3: File search across MIX files
    printf("Test 3: File search...\n");

    const char* test_files[] = {
        "RULES.INI",
        "MOUSE.SHP",
        "TEMPERAT.PAL",
        nullptr
    };

    for (int i = 0; test_files[i]; i++) {
        const char* containing = mgr.FindContainingMix(test_files[i]);
        if (containing) {
            printf("  %s -> %s (%u bytes)\n",
                   test_files[i], containing, mgr.GetFileSize(test_files[i]));
        } else {
            printf("  %s -> NOT FOUND\n", test_files[i]);
        }
    }
    printf("\n");

    // Test 4: Load file data
    printf("Test 4: Load file data...\n");
    for (int i = 0; test_files[i]; i++) {
        if (mgr.Contains(test_files[i])) {
            uint32_t size;
            uint8_t* data = mgr.LoadFile(test_files[i], &size);
            if (data) {
                printf("  Loaded %s: %u bytes, first byte = 0x%02X\n",
                       test_files[i], size, data[0]);
                delete[] data;
            } else {
                printf("  FAIL: Could not load %s\n", test_files[i]);
            }
        }
    }
    printf("\n");

    // Test 5: Theater mounting
    printf("Test 5: Theater mounting...\n");
    if (mgr.MountTheater(Theater::TEMPERATE)) {
        printf("  Mounted TEMPERATE theater\n");
        printf("  Current theater MIX: %s\n",
               Theater_ToMixName(mgr.GetCurrentTheater()));
    } else {
        printf("  Note: TEMPERAT.MIX not available\n");
    }
    printf("\n");

    // Test 6: Priority test (later mounted = higher priority)
    printf("Test 6: Mount priority...\n");
    printf("  Search order (first match wins):\n");
    mgr.DumpMounted();
    printf("\n");

    // Test 7: Unmount
    printf("Test 7: Unmount test...\n");
    int before = mgr.GetMountedCount();
    mgr.UnmountTheater();
    int after = mgr.GetMountedCount();
    printf("  Before unmount: %d, after: %d\n", before, after);

    // Cleanup
    mgr.Shutdown();
    Platform_Shutdown();

    printf("\n=== MIX Manager Test Complete ===\n");
    return 0;
}
```

### CMakeLists.txt (MODIFY)
```cmake
# Add mix_manager to game_mix library
target_sources(game_mix PRIVATE
    src/game/mix_manager.cpp
)

# MIX manager test
add_executable(TestMixManager src/test_mix_manager.cpp)
target_link_libraries(TestMixManager PRIVATE game_mix)
```

## Verification Command
```bash
ralph-tasks/verify/check-mix-manager.sh
```

## Verification Script

### ralph-tasks/verify/check-mix-manager.sh (NEW)
```bash
#!/bin/bash
# Verification script for MIX Manager

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking MIX Manager ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check source files
echo "[1/4] Checking source files..."
if [ ! -f "include/game/mix_manager.h" ]; then
    echo "VERIFY_FAILED: mix_manager.h not found"
    exit 1
fi
if [ ! -f "src/game/mix_manager.cpp" ]; then
    echo "VERIFY_FAILED: mix_manager.cpp not found"
    exit 1
fi
echo "  OK: Source files exist"

# Step 2: Build
echo "[2/4] Building..."
if ! cmake --build build --target TestMixManager 2>&1; then
    echo "VERIFY_FAILED: Build failed"
    exit 1
fi
echo "  OK: Build successful"

# Step 3: Check gamedata directory
echo "[3/4] Checking gamedata..."
if [ ! -d "gamedata" ]; then
    echo "VERIFY_WARNING: gamedata directory not found"
    echo "VERIFY_SUCCESS (build only)"
    exit 0
fi
echo "  OK: gamedata exists"

# Step 4: Run manager test
echo "[4/4] Running manager test..."
if ! ./build/TestMixManager gamedata; then
    echo "VERIFY_FAILED: Manager test failed"
    exit 1
fi
echo "  OK: Manager test passed"

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Create `include/game/mix_manager.h`
2. Create `src/game/mix_manager.cpp`
3. Update game_mix library in CMakeLists.txt
4. Create `src/test_mix_manager.cpp`
5. Build: `cmake --build build --target TestMixManager`
6. Test: `./build/TestMixManager gamedata`
7. Run verification script

## Success Criteria
- Singleton instance accessible via `MixManager::Instance()`
- Multiple MIX files can be mounted
- Search finds files in priority order (last mounted first)
- Theater mounting/unmounting works
- File loading works across all mounted archives
- Duplicate mounts handled gracefully

## Common Issues

### "Not initialized"
- Call `Initialize()` before `Mount()`

### File not found in any MIX
- Check mount order
- Verify file exists with `MixFile::DumpEntries()`

### Theater MIX not found
- Theater files may be in subdirectory (e.g., gamedata/TEMPERAT.MIX)
- Check case sensitivity on Linux

### Search priority wrong
- Search is in REVERSE order (rbegin/rend)
- Last mounted = first searched

## Completion Promise
When verification passes, output:
```
<promise>TASK_12D_COMPLETE</promise>
```

## Escape Hatch
If stuck after 15 iterations:
- Document in `ralph-tasks/blocked/12d.md`
- Output: `<promise>TASK_12D_BLOCKED</promise>`

## Max Iterations
15
