# Task 12g: Template/Tile Loading

## Dependencies
- Task 12e (Palette Loading) must be complete
- Task 12f (Shape Loading) recommended but not required

## Context
Templates in Red Alert define terrain tiles - the ground graphics that make up the map. Each theater (TEMPERATE, SNOW, DESERT, etc.) has its own set of templates stored in theater-specific files. This task implements template loading for map rendering.

## Objective
Create a Template system that:
1. Loads template definitions (.INI or binary format)
2. Loads tile graphics (24x24 pixel tiles)
3. Provides tile lookup for map rendering
4. Handles theater-specific content

## Technical Background

### Theater Files Structure
Each theater has associated files:
```
TEMPERAT.MIX contains:
  - temperat.ini (or binary equivalent)
  - Multiple .TEM files (template graphics)
  - Theater-specific overlays

Theater naming:
  - TEMPERATE -> .TEM extension
  - SNOW -> .SNO extension
  - DESERT -> .DES extension
  - INTERIOR -> .INT extension
```

### Template Format
Templates are collections of tiles arranged in a pattern:
```
Example: 2x2 road intersection
  [0][1]
  [2][3]

Each tile is 24x24 pixels (576 bytes of indexed color)
```

### Template Types (from original game)
| Type | Name | Description |
|------|------|-------------|
| CLEAR | Clear terrain | Passable ground |
| WATER | Water | Impassable water |
| ROAD | Roads | Faster movement |
| ROUGH | Rough terrain | Slower movement |
| RIVER | Rivers | Impassable water |
| CLIFF | Cliffs | Impassable terrain |
| SHORE | Shorelines | Water edges |

### Tile Data
Each tile contains:
- 24x24 pixels of indexed color data
- Passability flags (for pathfinding)
- Height information (for layering)

### Template Files
Red Alert stores template data in:
- `.TEM/.SNO/.DES/.INT` files - tile graphics
- Icon data within MIX files
- Template definitions potentially in RULES.INI or separate files

## Deliverables
- [ ] `include/game/template.h` - Template structures
- [ ] `src/game/template.cpp` - Template loader
- [ ] Theater tile loading
- [ ] Basic tile drawing
- [ ] Test program
- [ ] Verification script

## Files to Create

### include/game/template.h (NEW)
```cpp
#ifndef TEMPLATE_H
#define TEMPLATE_H

#include <cstdint>
#include <vector>
#include <string>
#include <map>

/// Tile dimensions
constexpr int TILE_WIDTH = 24;
constexpr int TILE_HEIGHT = 24;
constexpr int TILE_PIXELS = TILE_WIDTH * TILE_HEIGHT;  // 576

/// Terrain passability flags
enum TileFlags : uint8_t {
    TILE_PASSABLE      = 0x00,
    TILE_IMPASSABLE    = 0x01,
    TILE_WATER         = 0x02,
    TILE_CLIFF         = 0x04,
    TILE_BUILDING_OK   = 0x08,
};

/// Single terrain tile (24x24)
struct Tile {
    uint8_t pixels[TILE_PIXELS];  // Indexed color data
    uint8_t flags;                // Passability
    uint8_t height;               // Layer height

    Tile() : flags(TILE_PASSABLE), height(0) {
        memset(pixels, 0, sizeof(pixels));
    }

    /// Draw tile to buffer
    void Draw(uint8_t* buffer, int pitch, int buf_width, int buf_height,
              int x, int y) const;
};

/// Template definition (multi-tile terrain piece)
struct TemplateType {
    std::string name;
    int width;          // Width in tiles
    int height;         // Height in tiles
    std::vector<int> tile_indices;  // Index into theater tile array (-1 = empty)

    TemplateType() : width(0), height(0) {}

    /// Get tile index at position
    int GetTileIndex(int tx, int ty) const {
        if (tx < 0 || tx >= width || ty < 0 || ty >= height) return -1;
        return tile_indices[ty * width + tx];
    }
};

/// Theater terrain manager
class TheaterTiles {
public:
    TheaterTiles();
    ~TheaterTiles();

    /// Load theater tiles from MIX
    /// @param theater_name Theater name (e.g., "TEMPERAT")
    /// @return true if loaded successfully
    bool Load(const char* theater_name);

    /// Unload current theater
    void Unload();

    /// Check if loaded
    bool IsLoaded() const { return !tiles_.empty(); }

    /// Get tile count
    int GetTileCount() const { return static_cast<int>(tiles_.size()); }

    /// Get specific tile
    const Tile* GetTile(int index) const;

    /// Get tile by template and position
    const Tile* GetTemplateTile(int template_type, int tile_x, int tile_y) const;

    /// Get template definition
    const TemplateType* GetTemplate(int type) const;

    /// Get template count
    int GetTemplateCount() const { return static_cast<int>(templates_.size()); }

    /// Draw a tile to buffer
    void DrawTile(uint8_t* buffer, int pitch, int buf_width, int buf_height,
                  int x, int y, int tile_index) const;

    /// Get theater name
    const char* GetTheaterName() const { return theater_name_.c_str(); }

private:
    bool LoadTileGraphics(const char* theater_name);
    bool LoadTemplateDefinitions(const char* theater_name);
    bool ParseTemplateFile(const uint8_t* data, uint32_t size);

    std::vector<Tile> tiles_;
    std::vector<TemplateType> templates_;
    std::string theater_name_;
};

/// Global template manager
class TemplateManager {
public:
    static TemplateManager& Instance();

    /// Load theater
    bool LoadTheater(const char* theater_name);

    /// Unload current theater
    void UnloadTheater();

    /// Get current theater tiles
    TheaterTiles& GetTheater() { return theater_; }
    const TheaterTiles& GetTheater() const { return theater_; }

    /// Quick access to tile
    const Tile* GetTile(int index) const { return theater_.GetTile(index); }

    /// Draw tile at position
    void DrawTile(uint8_t* buffer, int pitch, int buf_w, int buf_h,
                  int x, int y, int tile_index) const {
        theater_.DrawTile(buffer, pitch, buf_w, buf_h, x, y, tile_index);
    }

private:
    TemplateManager() = default;
    TheaterTiles theater_;
};

/// Convenience macro
#define TheTemplateManager TemplateManager::Instance()

#endif // TEMPLATE_H
```

### src/game/template.cpp (NEW)
```cpp
#include "game/template.h"
#include "game/mix_manager.h"
#include <cstring>
#include <cstdio>
#include <algorithm>

// === Tile ===

void Tile::Draw(uint8_t* buffer, int pitch, int buf_width, int buf_height,
                int x, int y) const {
    for (int ty = 0; ty < TILE_HEIGHT; ty++) {
        int screen_y = y + ty;
        if (screen_y < 0 || screen_y >= buf_height) continue;

        for (int tx = 0; tx < TILE_WIDTH; tx++) {
            int screen_x = x + tx;
            if (screen_x < 0 || screen_x >= buf_width) continue;

            buffer[screen_y * pitch + screen_x] = pixels[ty * TILE_WIDTH + tx];
        }
    }
}

// === TheaterTiles ===

TheaterTiles::TheaterTiles() {
}

TheaterTiles::~TheaterTiles() {
    Unload();
}

bool TheaterTiles::Load(const char* theater_name) {
    Unload();

    if (!theater_name) {
        return false;
    }

    theater_name_ = theater_name;

    // Load tile graphics first
    if (!LoadTileGraphics(theater_name)) {
        fprintf(stderr, "TheaterTiles: Failed to load tile graphics for %s\n", theater_name);
        Unload();
        return false;
    }

    // Load template definitions (optional - may be embedded or in rules)
    LoadTemplateDefinitions(theater_name);

    printf("TheaterTiles: Loaded %s (%d tiles, %d templates)\n",
           theater_name, GetTileCount(), GetTemplateCount());

    return true;
}

void TheaterTiles::Unload() {
    tiles_.clear();
    templates_.clear();
    theater_name_.clear();
}

bool TheaterTiles::LoadTileGraphics(const char* theater_name) {
    // Determine file extension based on theater
    std::string ext;
    if (strcasecmp(theater_name, "TEMPERAT") == 0) {
        ext = ".TEM";
    } else if (strcasecmp(theater_name, "SNOW") == 0) {
        ext = ".SNO";
    } else if (strcasecmp(theater_name, "DESERT") == 0) {
        ext = ".DES";
    } else if (strcasecmp(theater_name, "INTERIOR") == 0) {
        ext = ".INT";
    } else if (strcasecmp(theater_name, "JUNGLE") == 0) {
        ext = ".JUN";
    } else {
        ext = ".TEM";  // Default
    }

    // Try to load TILES.xxx or similar
    // The exact filename varies by theater
    // Common patterns: TILES.TEM, TEMPLATE.TEM, etc.

    std::string tiles_file = std::string("TILES") + ext;

    uint32_t size;
    uint8_t* data = MixManager::Instance().LoadFile(tiles_file.c_str(), &size);

    if (!data) {
        // Try alternate naming
        tiles_file = std::string("TEMPLATE") + ext;
        data = MixManager::Instance().LoadFile(tiles_file.c_str(), &size);
    }

    if (!data) {
        // Try theater name directly
        tiles_file = std::string(theater_name) + ext;
        data = MixManager::Instance().LoadFile(tiles_file.c_str(), &size);
    }

    if (!data) {
        fprintf(stderr, "TheaterTiles: Could not find tile file for %s\n", theater_name);

        // Create some default tiles for testing
        printf("TheaterTiles: Creating default test tiles\n");

        // Create 16 test tiles with different shades
        tiles_.resize(16);
        for (int i = 0; i < 16; i++) {
            uint8_t shade = static_cast<uint8_t>(i * 16);
            memset(tiles_[i].pixels, shade, TILE_PIXELS);
            tiles_[i].flags = TILE_PASSABLE;
        }

        return true;  // Continue with defaults
    }

    // Parse tile data
    // Format varies, but typically:
    // - Header with tile count
    // - Raw 24x24 pixel data for each tile

    // Simple format: just raw tiles, size / TILE_PIXELS = count
    int tile_count = size / TILE_PIXELS;

    if (tile_count > 0) {
        tiles_.resize(tile_count);

        for (int i = 0; i < tile_count; i++) {
            memcpy(tiles_[i].pixels, data + i * TILE_PIXELS, TILE_PIXELS);
            tiles_[i].flags = TILE_PASSABLE;  // Default
            tiles_[i].height = 0;
        }
    }

    delete[] data;
    return tile_count > 0;
}

bool TheaterTiles::LoadTemplateDefinitions(const char* theater_name) {
    // Template definitions might be in:
    // - RULES.INI (TemplateDefs section)
    // - Separate .INI file
    // - Binary format

    // For now, create some basic default templates
    // This will be expanded when we parse actual game data

    // Template 0: Clear terrain (single tile)
    TemplateType clear;
    clear.name = "CLEAR";
    clear.width = 1;
    clear.height = 1;
    clear.tile_indices.push_back(0);
    templates_.push_back(clear);

    // Template 1: 2x2 rough terrain
    TemplateType rough;
    rough.name = "ROUGH";
    rough.width = 2;
    rough.height = 2;
    rough.tile_indices = {1, 2, 3, 4};
    templates_.push_back(rough);

    // More templates would be loaded from game data...

    return true;
}

const Tile* TheaterTiles::GetTile(int index) const {
    if (index < 0 || index >= static_cast<int>(tiles_.size())) {
        return nullptr;
    }
    return &tiles_[index];
}

const Tile* TheaterTiles::GetTemplateTile(int template_type, int tile_x, int tile_y) const {
    const TemplateType* tmpl = GetTemplate(template_type);
    if (!tmpl) return nullptr;

    int tile_idx = tmpl->GetTileIndex(tile_x, tile_y);
    if (tile_idx < 0) return nullptr;

    return GetTile(tile_idx);
}

const TemplateType* TheaterTiles::GetTemplate(int type) const {
    if (type < 0 || type >= static_cast<int>(templates_.size())) {
        return nullptr;
    }
    return &templates_[type];
}

void TheaterTiles::DrawTile(uint8_t* buffer, int pitch, int buf_width, int buf_height,
                            int x, int y, int tile_index) const {
    const Tile* tile = GetTile(tile_index);
    if (tile) {
        tile->Draw(buffer, pitch, buf_width, buf_height, x, y);
    }
}

// === TemplateManager ===

TemplateManager& TemplateManager::Instance() {
    static TemplateManager instance;
    return instance;
}

bool TemplateManager::LoadTheater(const char* theater_name) {
    return theater_.Load(theater_name);
}

void TemplateManager::UnloadTheater() {
    theater_.Unload();
}
```

### src/test_template.cpp (NEW)
```cpp
// Test template/tile loading
#include <cstdio>
#include <cstring>
#include "game/template.h"
#include "game/palette.h"
#include "game/mix_manager.h"
#include "platform.h"

void draw_tile_grid() {
    uint8_t* pixels;
    int32_t width, height, pitch;

    Platform_Graphics_LockBackBuffer();
    Platform_Graphics_GetBackBuffer(&pixels, &width, &height, &pitch);

    // Clear background
    memset(pixels, 0, pitch * height);

    // Draw tiles in a grid
    TemplateManager& tm = TemplateManager::Instance();
    int tile_count = tm.GetTheater().GetTileCount();

    int tiles_per_row = width / TILE_WIDTH;
    int drawn = 0;

    for (int ty = 0; ty * TILE_HEIGHT < height && drawn < tile_count; ty++) {
        for (int tx = 0; tx < tiles_per_row && drawn < tile_count; tx++, drawn++) {
            int px = tx * TILE_WIDTH;
            int py = ty * TILE_HEIGHT;
            tm.DrawTile(pixels, pitch, width, height, px, py, drawn);
        }
    }

    Platform_Graphics_UnlockBackBuffer();
    Platform_Graphics_Flip();
}

int main(int argc, char* argv[]) {
    const char* theater_name = "TEMPERAT";
    if (argc > 1) {
        theater_name = argv[1];
    }

    printf("=== Template Loading Test ===\n\n");

    if (Platform_Init() != 0) {
        fprintf(stderr, "Platform init failed\n");
        return 1;
    }

    if (Platform_Graphics_Init() != 0) {
        fprintf(stderr, "Graphics init failed\n");
        Platform_Shutdown();
        return 1;
    }

    // Initialize systems
    MixManager& mgr = MixManager::Instance();
    mgr.Initialize("gamedata");
    mgr.Mount("REDALERT.MIX");

    // Try to mount theater MIX
    std::string theater_mix = std::string(theater_name) + ".MIX";
    mgr.Mount(theater_mix.c_str());

    // Load palette
    std::string pal_name = std::string(theater_name) + ".PAL";
    if (!PaletteManager::Instance().LoadPalette(pal_name.c_str())) {
        // Fallback to TEMPERAT.PAL
        PaletteManager::Instance().LoadPalette("TEMPERAT.PAL");
    }

    // Test 1: Load theater
    printf("Test 1: Load theater '%s'...\n", theater_name);
    if (!TemplateManager::Instance().LoadTheater(theater_name)) {
        fprintf(stderr, "  FAIL: Could not load theater\n");
        mgr.Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return 1;
    }
    printf("  PASS: Loaded %d tiles\n", TemplateManager::Instance().GetTheater().GetTileCount());

    // Test 2: Draw tile grid
    printf("\nTest 2: Draw tile grid...\n");
    draw_tile_grid();
    printf("  PASS: Grid drawn\n");

    Platform_Timer_Delay(2000);

    // Test 3: Individual tile access
    printf("\nTest 3: Individual tile access...\n");
    for (int i = 0; i < std::min(5, TemplateManager::Instance().GetTheater().GetTileCount()); i++) {
        const Tile* tile = TemplateManager::Instance().GetTile(i);
        if (tile) {
            // Sample some pixels
            printf("  Tile %d: flags=0x%02X, corners: [%02X,%02X,%02X,%02X]\n",
                   i, tile->flags,
                   tile->pixels[0],  // top-left
                   tile->pixels[TILE_WIDTH-1],  // top-right
                   tile->pixels[(TILE_HEIGHT-1)*TILE_WIDTH],  // bottom-left
                   tile->pixels[TILE_PIXELS-1]);  // bottom-right
        }
    }

    // Test 4: Template lookup
    printf("\nTest 4: Template definitions...\n");
    const TheaterTiles& theater = TemplateManager::Instance().GetTheater();
    for (int i = 0; i < theater.GetTemplateCount(); i++) {
        const TemplateType* tmpl = theater.GetTemplate(i);
        if (tmpl) {
            printf("  Template %d: '%s' (%dx%d)\n",
                   i, tmpl->name.c_str(), tmpl->width, tmpl->height);
        }
    }

    // Cleanup
    TemplateManager::Instance().UnloadTheater();
    mgr.Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();

    printf("\n=== Template Test Complete ===\n");
    return 0;
}
```

### CMakeLists.txt (MODIFY)
```cmake
# Add template to game_assets library
target_sources(game_assets PRIVATE
    src/game/template.cpp
)

# Template test
add_executable(TestTemplate src/test_template.cpp)
target_link_libraries(TestTemplate PRIVATE game_assets)
```

## Verification Command
```bash
ralph-tasks/verify/check-template.sh
```

## Verification Script

### ralph-tasks/verify/check-template.sh (NEW)
```bash
#!/bin/bash
# Verification script for template loading

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Template Loading ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check source files
echo "[1/4] Checking source files..."
if [ ! -f "include/game/template.h" ]; then
    echo "VERIFY_FAILED: template.h not found"
    exit 1
fi
if [ ! -f "src/game/template.cpp" ]; then
    echo "VERIFY_FAILED: template.cpp not found"
    exit 1
fi
echo "  OK: Source files exist"

# Step 2: Build
echo "[2/4] Building..."
if ! cmake --build build --target TestTemplate 2>&1; then
    echo "VERIFY_FAILED: Build failed"
    exit 1
fi
echo "  OK: Build successful"

# Step 3: Check for MIX file
echo "[3/4] Checking for MIX file..."
if [ ! -f "gamedata/REDALERT.MIX" ]; then
    echo "VERIFY_WARNING: REDALERT.MIX not found"
    echo "VERIFY_SUCCESS (build only)"
    exit 0
fi
echo "  OK: MIX file exists"

# Step 4: Run test
echo "[4/4] Running template test..."
if ! timeout 15 ./build/TestTemplate TEMPERAT 2>&1; then
    echo "VERIFY_FAILED: Template test failed"
    exit 1
fi
echo "  OK: Template test passed"

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Create `include/game/template.h`
2. Create `src/game/template.cpp`
3. Update game_assets library in CMakeLists.txt
4. Create `src/test_template.cpp`
5. Build: `cmake --build build --target TestTemplate`
6. Test: `./build/TestTemplate TEMPERAT`
7. Run verification script

## Success Criteria
- Theater loads (even with default tiles)
- Tiles are 24x24 pixels
- Tile grid displays without crash
- Template definitions are accessible
- Multiple theaters can be loaded/unloaded

## Common Issues

### No tile file found
- Theater tiles may have various naming conventions
- Check if theater MIX is mounted
- Default tiles are generated for testing

### Wrong tile colors
- Ensure correct theater palette is loaded
- Check tile pixel format

### Tile alignment issues
- Verify TILE_WIDTH and TILE_HEIGHT are 24
- Check buffer pitch vs width

## Notes on Actual Game Format
The actual Red Alert tile format is more complex:
1. Tiles may be in separate theater MIX files
2. Template definitions may be in binary format
3. Some tiles use special encoding

This implementation provides a foundation. Full game compatibility requires:
- Analyzing actual theater files with hex editor
- Referencing OpenRA source for format details
- Iterative refinement based on visual comparison

## Completion Promise
When verification passes, output:
```
<promise>TASK_12G_COMPLETE</promise>
```

## Escape Hatch
If stuck after 15 iterations:
- Document in `ralph-tasks/blocked/12g.md`
- Note: Even with placeholder tiles, task is successful if structure works
- Output: `<promise>TASK_12G_BLOCKED</promise>`

## Max Iterations
15
