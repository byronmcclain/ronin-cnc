# Phase 13: Asset System (MIX Files)

## Overview

All Red Alert game assets are stored in MIX archive files. This phase implements the complete asset loading pipeline from raw MIX files to usable game data.

---

## MIX File Format

### Header Structure
```
Offset  Size  Description
------  ----  -----------
0x0000  2     File count (uint16)
0x0002  4     Total data size (uint32)
```

### Extended Format Detection
If first 2 bytes are `0x0000`:
```
Offset  Size  Description
------  ----  -----------
0x0000  2     Zero marker (0x0000)
0x0002  2     Flags:
              - Bit 0: Has SHA digest
              - Bit 1: Header encrypted (RSA)
0x0004  ...   Encrypted header block (if flag set)
```

### File Index Entry (12 bytes each)
```
Offset  Size  Description
------  ----  -----------
0x0000  4     CRC32 of filename (used as ID)
0x0004  4     Offset from data section start
0x0008  4     File size in bytes
```

### Data Section
Raw concatenated file data, uncompressed.

---

## CRC32 Calculation

Filenames are identified by CRC32 hash (already implemented in `platform/src/util/crc.rs`):

```rust
// From platform layer - already working
pub fn calculate_crc32(data: &[u8]) -> u32
```

For MIX lookups:
1. Convert filename to uppercase
2. Calculate CRC32
3. Binary search sorted index

---

## Implementation Tasks

### Task 13.1: MIX Archive Parser

**File:** `src/game/mix_file.cpp`

```cpp
#include "platform.h"
#include <cstdint>
#include <vector>
#include <string>

struct MixEntry {
    uint32_t crc;      // Filename CRC
    uint32_t offset;   // Data offset
    uint32_t size;     // File size
};

class MixFile {
public:
    MixFile(const char* path);
    ~MixFile();

    bool IsValid() const;

    // Find file by name (calculates CRC internally)
    bool Contains(const char* filename) const;

    // Get file data (returns pointer to internal buffer or nullptr)
    const uint8_t* GetFileData(const char* filename, uint32_t* out_size) const;

    // Get file data by CRC directly
    const uint8_t* GetFileDataByCRC(uint32_t crc, uint32_t* out_size) const;

    // Load entire file into provided buffer
    bool LoadFile(const char* filename, void* buffer, uint32_t buffer_size) const;

private:
    PlatformFile* file_;
    std::vector<MixEntry> entries_;
    uint32_t data_offset_;
    bool is_encrypted_;
    bool has_digest_;

    // Cached file data (lazy loaded)
    mutable std::vector<uint8_t> cache_;
    mutable uint32_t cached_crc_;

    uint32_t CalculateCRC(const char* filename) const;
    int BinarySearch(uint32_t crc) const;
};
```

**Verification:**
```bash
# Test loading REDALERT.MIX
./build/TestMix gamedata/REDALERT.MIX
# Should print file count and list some entries
```

---

### Task 13.2: Global MIX Manager

**File:** `src/game/mix_manager.cpp`

```cpp
class MixManager {
public:
    static MixManager& Instance();

    // Initialize with game data path
    bool Initialize(const char* game_path);
    void Shutdown();

    // Mount a MIX file (adds to search path)
    bool Mount(const char* mix_name);
    void Unmount(const char* mix_name);

    // Find file across all mounted MIX files
    const uint8_t* FindFile(const char* filename, uint32_t* out_size);

    // Load specific asset types
    bool LoadPalette(const char* name, uint8_t* palette_256_rgb);
    bool LoadShape(const char* name, ShapeData* out_shape);

private:
    std::vector<MixFile*> mounted_;
    std::string game_path_;
};
```

**Required MIX files to mount:**
```cpp
// Core game (always needed)
MixManager::Mount("REDALERT.MIX");

// Theater-specific (load based on map)
MixManager::Mount("TEMPERAT.MIX");  // or SNOW, INTERIOR, etc.

// Expansion content (optional)
MixManager::Mount("EXPAND.MIX");
MixManager::Mount("EXPAND2.MIX");
```

---

### Task 13.3: Palette Loading

**File:** `src/game/palette.cpp`

Red Alert uses 256-color palettes (768 bytes = 256 * 3 RGB).

```cpp
struct Palette {
    uint8_t colors[256][3];  // RGB values (0-63 range in original)

    void LoadFromData(const uint8_t* data, size_t size);
    void ApplyToPlatform();  // Calls Platform_Graphics_SetPalette

    // Convert from 6-bit (0-63) to 8-bit (0-255)
    void ConvertTo8Bit();
};

// Known palette files in REDALERT.MIX:
// - "PALETTE.PAL" - Main game palette
// - "SNOW.PAL" - Snow theater
// - "DESERT.PAL" - Desert theater
// - "TEMPERAT.PAL" - Temperate theater
```

**Verification:**
```bash
# Load and display palette
./build/TestPalette gamedata/REDALERT.MIX TEMPERAT.PAL
# Should show 256-color gradient
```

---

### Task 13.4: Shape/Sprite Loading

**File:** `src/game/shape.cpp`

Shapes are sprite graphics with multiple frames.

```cpp
struct ShapeFrame {
    int16_t x_offset;     // Draw offset
    int16_t y_offset;
    uint16_t width;
    uint16_t height;
    uint8_t flags;        // Compression, etc.
    std::vector<uint8_t> pixels;  // Indexed color data
};

struct ShapeData {
    std::vector<ShapeFrame> frames;

    bool LoadFromData(const uint8_t* data, size_t size);
    void Draw(uint8_t* buffer, int pitch, int x, int y, int frame);
    void DrawRemapped(uint8_t* buffer, int pitch, int x, int y,
                      int frame, const uint8_t* remap_table);
};
```

**Shape file format:**
```
Header:
  uint16_t frame_count

Frame offsets (frame_count * 4 bytes):
  uint32_t offset_to_frame_data

Frame data (variable):
  int16_t x_offset
  int16_t y_offset
  uint16_t width
  uint16_t height
  uint8_t flags
  ... pixel data (RLE compressed or raw)
```

---

### Task 13.5: Template/Tile Loading

**File:** `src/game/template.cpp`

Map terrain uses tile templates.

```cpp
struct TileTemplate {
    uint16_t width;       // In tiles
    uint16_t height;
    std::vector<uint8_t*> tiles;  // 24x24 pixel tiles

    bool LoadFromData(const uint8_t* data, size_t size);
};

// Theater files contain:
// - TEMPLATES.BIN - Template definitions
// - TILES.BIN - Tile graphics
// - ICONS.SHP - Terrain icons
```

---

### Task 13.6: String Table Loading

**File:** `src/game/strings.cpp`

```cpp
class StringTable {
public:
    bool LoadFromMix(MixFile* mix, const char* filename);
    const char* GetString(int id) const;

private:
    std::vector<std::string> strings_;
};

// String files:
// - "CONQUER.ENG" - English strings
// - "CONQUER.FRE" - French strings
// - "CONQUER.GER" - German strings
```

---

## File Discovery

### Known Files in REDALERT.MIX

Based on CRC analysis and original game:

| Filename | CRC32 | Purpose |
|----------|-------|---------|
| PALETTE.PAL | varies | Main palette |
| MOUSE.SHP | varies | Mouse cursors |
| FONT8.FNT | varies | 8-pixel font |
| CONQUER.ENG | varies | English text |
| RULES.INI | varies | Game rules |
| ART.INI | varies | Art definitions |

### CRC Lookup Table

Create a lookup table for common files:
```cpp
// src/game/mix_crc_table.cpp
struct KnownFile {
    const char* name;
    uint32_t crc;
};

static const KnownFile KNOWN_FILES[] = {
    {"PALETTE.PAL", 0x????????},
    {"MOUSE.SHP", 0x????????},
    // ... etc
};
```

To generate CRCs, use the platform's CRC function or extract from working game.

---

## Testing Strategy

### Unit Tests

```cpp
// test_mix.cpp
void test_mix_open() {
    MixFile mix("gamedata/REDALERT.MIX");
    assert(mix.IsValid());
    assert(mix.GetFileCount() > 0);
}

void test_mix_find_file() {
    MixFile mix("gamedata/REDALERT.MIX");
    uint32_t size;
    const uint8_t* data = mix.GetFileData("RULES.INI", &size);
    assert(data != nullptr);
    assert(size > 0);
}

void test_palette_load() {
    // Load palette and verify 768 bytes
}

void test_shape_load() {
    // Load shape and verify frame count
}
```

### Integration Tests

```bash
#!/bin/bash
# verify/check-mix-loading.sh

./build/TestMix gamedata/REDALERT.MIX || exit 1
./build/TestMix gamedata/SETUP.MIX || exit 1
./build/TestMix gamedata/CD1/MAIN.MIX || exit 1

echo "All MIX files loaded successfully"
```

---

## CMake Integration

Add to `CMakeLists.txt`:

```cmake
# Asset system library
add_library(game_assets STATIC
    src/game/mix_file.cpp
    src/game/mix_manager.cpp
    src/game/palette.cpp
    src/game/shape.cpp
    src/game/template.cpp
    src/game/strings.cpp
)

target_include_directories(game_assets PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/game
)

target_link_libraries(game_assets PRIVATE redalert_platform)

# Test executable
add_executable(TestMix src/test_mix.cpp)
target_link_libraries(TestMix PRIVATE game_assets redalert_platform)
```

---

## Dependencies

### Platform Layer Functions Used

```c
// File I/O
Platform_File_Open(path, mode)
Platform_File_Read(file, buffer, size)
Platform_File_Seek(file, offset, origin)
Platform_File_Close(file)

// Graphics (for palette)
Platform_Graphics_SetPalette(palette_data)

// Memory
Platform_Alloc(size, flags)
Platform_Free(ptr, size)

// CRC (may need to expose from Rust)
// Currently internal - may need FFI export
```

### External References

- OpenRA MIX format documentation
- CnCNet asset extraction tools
- Original MIXFILE.H/CPP for reference

---

## Success Criteria

- [ ] Open and parse REDALERT.MIX header
- [ ] Enumerate all files in archive
- [ ] Load RULES.INI as text file
- [ ] Load and apply a palette
- [ ] Load and display a shape sprite
- [ ] Load theater tiles
- [ ] All verification scripts pass

---

## Estimated Effort

| Task | Hours |
|------|-------|
| 13.1 MIX Parser | 4-6 |
| 13.2 MIX Manager | 2-3 |
| 13.3 Palette Loading | 2-3 |
| 13.4 Shape Loading | 4-6 |
| 13.5 Template Loading | 3-4 |
| 13.6 String Table | 2-3 |
| Testing | 3-5 |
| **Total** | **20-30** |
