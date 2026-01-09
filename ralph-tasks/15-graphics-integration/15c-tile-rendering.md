# Task 15c: Tile/Template Rendering

## Dependencies
- Task 15a (GraphicsBuffer) must be complete
- Task 15b (Shape Drawing) must be complete
- Platform template loading must work (`Platform_Template_*` APIs)
- Asset system (MIX file loading) must be functional
- CellClass from Phase 14 must exist

## Context
Red Alert renders terrain using **tile templates**. Each map cell (24×24 pixels) is filled by a tile from a template file. Templates group related tiles together (e.g., a river template might have 12 tiles for different water/shore configurations).

Templates are stored in theater-specific MIX files:
- `TEMPERAT.MIX` - Temperate theater (green grass, trees)
- `SNOW.MIX` - Snow theater (white snow, frozen water)
- `INTERIOR.MIX` - Interior theater (building interiors)

Each theater has different graphics but the same template IDs, allowing maps to work across theaters.

This task creates a tile rendering system that:
1. Loads and caches theater templates
2. Draws individual tiles to the screen
3. Handles overlays (roads, rivers, walls, etc.)
4. Integrates with CellClass for map rendering

## Objective
Create a comprehensive tile rendering system that draws terrain from template files to a GraphicsBuffer, supporting all three theaters and overlay rendering.

## Technical Background

### Template File Format (.TMP)
Template files contain one or more 24×24 tiles:
```
Header:
  uint16_t width           // Template width in cells
  uint16_t height          // Template height in cells
  uint16_t tile_count      // Number of actual tiles
  uint16_t unknown
  uint32_t tile_size       // Size of each tile (usually 576 = 24*24)
  uint32_t image_offset    // Offset to tile data
  uint32_t palette_offset  // Usually 0 (uses theater palette)
  uint32_t remap_offset    // Offset to land type data
  uint32_t tile_data_offset // Offset to individual tiles

Tile Data:
  For each tile: 576 bytes of 8-bit indexed pixels
```

### Icon Data (.ICN)
Some templates use ICN format (icon files):
```
Header:
  uint32_t size
  uint16_t width, height   // Icon dimensions (24×24)
  uint16_t frame_count

Per Icon:
  576 bytes (24×24 pixels)
```

### Template Types
Templates are identified by `TemplateType` enum values:
```cpp
enum TemplateType : int16_t {
    TEMPLATE_NONE = -1,
    TEMPLATE_CLEAR1 = 0,   // Clear terrain variants
    TEMPLATE_CLEAR2,
    TEMPLATE_CLEAR3,
    // ... many more
    TEMPLATE_WATER1,       // Water templates
    TEMPLATE_SHORE1,       // Shore transitions
    TEMPLATE_ROAD1,        // Road templates
    // ... etc
};
```

### Overlay Types
Overlays are drawn on top of base terrain:
```cpp
enum OverlayType : int8_t {
    OVERLAY_NONE = -1,
    OVERLAY_SANDBAG_WALL = 0,
    OVERLAY_CYCLONE_WALL,
    OVERLAY_BRICK_WALL,
    OVERLAY_BARBWIRE_WALL,
    OVERLAY_WOOD_WALL,
    OVERLAY_TIBERIUM1,     // Ore/gems
    OVERLAY_ROAD,          // Roads
    // ... etc
};
```

### Tile Size
```
TILE_WIDTH = 24 pixels
TILE_HEIGHT = 24 pixels
TILE_SIZE = 576 bytes (24 × 24)
```

### Theater Palettes
Each theater has its own 256-color palette:
- `TEMPERAT.PAL` - Greens and browns
- `SNOW.PAL` - Whites and blues
- `INTERIOR.PAL` - Grays and metals

## Deliverables
- [ ] `include/game/graphics/tile_renderer.h` - Tile renderer class
- [ ] `src/game/graphics/tile_renderer.cpp` - Implementation
- [ ] `include/game/graphics/template_manager.h` - Template loading/caching
- [ ] `src/game/graphics/template_manager.cpp` - Implementation
- [ ] `src/test_tile_rendering.cpp` - Test program
- [ ] `ralph-tasks/verify/check-tile-rendering.sh` - Verification script
- [ ] All tests pass

## Files to Create

### include/game/graphics/tile_renderer.h
```cpp
/**
 * TileRenderer - Terrain Tile Rendering System
 *
 * Draws terrain tiles from template files to a GraphicsBuffer.
 * Handles theater switching, tile caching, and overlay rendering.
 *
 * Usage:
 *   TileRenderer::Instance().SetTheater(THEATER_TEMPERATE);
 *   TileRenderer::Instance().DrawTile(buffer, x, y, template_type, icon);
 *
 * Original: CODE/DISPLAY.CPP, CODE/CELL.CPP
 */

#ifndef GAME_GRAPHICS_TILE_RENDERER_H
#define GAME_GRAPHICS_TILE_RENDERER_H

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

// Forward declarations
class GraphicsBuffer;
struct PlatformTemplate;

// =============================================================================
// Constants
// =============================================================================

// Tile dimensions
constexpr int TILE_WIDTH = 24;
constexpr int TILE_HEIGHT = 24;
constexpr int TILE_SIZE = TILE_WIDTH * TILE_HEIGHT;  // 576 bytes

// =============================================================================
// Theater Type
// =============================================================================

enum TheaterType : int8_t {
    THEATER_NONE = -1,
    THEATER_TEMPERATE = 0,
    THEATER_SNOW = 1,
    THEATER_INTERIOR = 2,
    THEATER_COUNT = 3
};

/**
 * Get theater name for file loading
 */
const char* Theater_Name(TheaterType theater);

/**
 * Get theater file extension
 */
const char* Theater_Extension(TheaterType theater);

// =============================================================================
// Template Type
// =============================================================================

/**
 * TemplateType - Identifies terrain templates
 *
 * Each template type corresponds to a .TMP file in the theater MIX.
 * The template may contain multiple tiles (icons).
 */
enum TemplateType : int16_t {
    TEMPLATE_NONE = -1,

    // Clear terrain (grass/snow/floor)
    TEMPLATE_CLEAR1 = 0,

    // Water tiles
    TEMPLATE_WATER1 = 1,
    TEMPLATE_WATER2 = 2,

    // Shore tiles (water-land transitions)
    TEMPLATE_SHORE1 = 3,
    TEMPLATE_SHORE2 = 4,
    TEMPLATE_SHORE3 = 5,
    TEMPLATE_SHORE4 = 6,
    TEMPLATE_SHORE5 = 7,
    TEMPLATE_SHORE6 = 8,
    TEMPLATE_SHORE7 = 9,
    TEMPLATE_SHORE8 = 10,

    // Cliffs
    TEMPLATE_CLIFF1 = 11,
    TEMPLATE_CLIFF2 = 12,

    // Roads
    TEMPLATE_ROAD1 = 13,
    TEMPLATE_ROAD2 = 14,
    TEMPLATE_ROAD3 = 15,

    // Rough terrain
    TEMPLATE_ROUGH1 = 16,
    TEMPLATE_ROUGH2 = 17,

    // Add more as needed from original TemplateType enum
    // See CODE/DEFINES.H for complete list

    TEMPLATE_COUNT
};

// =============================================================================
// Overlay Type
// =============================================================================

/**
 * OverlayType - Identifies overlay graphics
 *
 * Overlays are drawn on top of base terrain (walls, ore, roads).
 */
enum OverlayType : int8_t {
    OVERLAY_NONE = -1,
    OVERLAY_SANDBAG_WALL = 0,
    OVERLAY_CYCLONE_WALL = 1,
    OVERLAY_BRICK_WALL = 2,
    OVERLAY_BARBWIRE_WALL = 3,
    OVERLAY_WOOD_WALL = 4,
    OVERLAY_GOLD1 = 5,
    OVERLAY_GOLD2 = 6,
    OVERLAY_GOLD3 = 7,
    OVERLAY_GOLD4 = 8,
    OVERLAY_GEMS1 = 9,
    OVERLAY_GEMS2 = 10,
    OVERLAY_GEMS3 = 11,
    OVERLAY_GEMS4 = 12,
    OVERLAY_V12 = 13,   // Civilian buildings
    OVERLAY_V13 = 14,
    OVERLAY_V14 = 15,
    OVERLAY_V15 = 16,
    OVERLAY_V16 = 17,
    OVERLAY_V17 = 18,
    OVERLAY_V18 = 19,
    OVERLAY_FLAG = 20,  // Capture the flag
    OVERLAY_WOOD_FENCE = 21,
    OVERLAY_COUNT
};

// =============================================================================
// Land Type
// =============================================================================

/**
 * LandType - Terrain passability type
 *
 * Each tile position maps to a land type that affects unit movement.
 */
enum LandType : int8_t {
    LAND_CLEAR = 0,     // Normal passable terrain
    LAND_ROAD = 1,      // Road (faster movement)
    LAND_WATER = 2,     // Deep water (ships only)
    LAND_ROCK = 3,      // Impassable rock
    LAND_WALL = 4,      // Wall (destroyable obstacle)
    LAND_TIBERIUM = 5,  // Ore/gems (harvestable)
    LAND_BEACH = 6,     // Beach (amphibious landing)
    LAND_ROUGH = 7,     // Rough terrain (slower)
    LAND_RIVER = 8,     // Shallow water
    LAND_COUNT
};

// =============================================================================
// Template Data
// =============================================================================

/**
 * TemplateData - Cached template information
 */
struct TemplateData {
    TemplateType type;              // Template identifier
    int width;                      // Width in cells
    int height;                     // Height in cells
    int tile_count;                 // Number of tiles
    std::vector<uint8_t> pixels;    // All tile pixels (tile_count * 576)
    std::vector<LandType> land;     // Land type per tile position

    /**
     * Get pixel data for a specific tile
     */
    const uint8_t* GetTile(int index) const {
        if (index < 0 || index >= tile_count) return nullptr;
        return pixels.data() + index * TILE_SIZE;
    }

    /**
     * Get land type for a specific tile
     */
    LandType GetLandType(int index) const {
        if (index < 0 || index >= static_cast<int>(land.size())) return LAND_CLEAR;
        return land[index];
    }
};

// =============================================================================
// TileRenderer Class
// =============================================================================

/**
 * TileRenderer - Singleton for terrain tile rendering
 */
class TileRenderer {
public:
    // =========================================================================
    // Singleton
    // =========================================================================

    static TileRenderer& Instance();

    // Non-copyable
    TileRenderer(const TileRenderer&) = delete;
    TileRenderer& operator=(const TileRenderer&) = delete;

    // =========================================================================
    // Theater Management
    // =========================================================================

    /**
     * Set the current theater
     *
     * Loads theater palette and clears template cache.
     *
     * @param theater Theater to use
     * @return true if loaded successfully
     */
    bool SetTheater(TheaterType theater);

    /**
     * Get current theater
     */
    TheaterType GetTheater() const { return current_theater_; }

    /**
     * Check if a theater is loaded
     */
    bool IsTheaterLoaded() const { return current_theater_ != THEATER_NONE; }

    // =========================================================================
    // Tile Drawing
    // =========================================================================

    /**
     * Draw a single terrain tile
     *
     * @param buffer     Target graphics buffer (must be locked)
     * @param x          Screen X coordinate
     * @param y          Screen Y coordinate
     * @param tmpl       Template type
     * @param icon       Icon index within template (0-based)
     * @return true if drawn successfully
     */
    bool DrawTile(GraphicsBuffer& buffer, int x, int y,
                  TemplateType tmpl, int icon);

    /**
     * Draw a terrain tile with land type lookup
     *
     * Same as DrawTile but also returns the land type.
     *
     * @param land_out Output land type (can be null)
     */
    bool DrawTileWithLand(GraphicsBuffer& buffer, int x, int y,
                          TemplateType tmpl, int icon, LandType* land_out);

    /**
     * Draw clear terrain (default fill)
     *
     * Uses random clear template for visual variation.
     *
     * @param buffer Target buffer
     * @param x, y   Screen coordinates
     * @param seed   Random seed for variation
     */
    void DrawClear(GraphicsBuffer& buffer, int x, int y, uint32_t seed = 0);

    // =========================================================================
    // Overlay Drawing
    // =========================================================================

    /**
     * Draw an overlay tile
     *
     * @param buffer  Target graphics buffer
     * @param x, y    Screen coordinates
     * @param overlay Overlay type
     * @param frame   Frame/variant index
     * @return true if drawn successfully
     */
    bool DrawOverlay(GraphicsBuffer& buffer, int x, int y,
                     OverlayType overlay, int frame);

    // =========================================================================
    // Template Information
    // =========================================================================

    /**
     * Get template data (loads if needed)
     *
     * @param tmpl Template type
     * @return Pointer to template data, or nullptr if not found
     */
    const TemplateData* GetTemplate(TemplateType tmpl);

    /**
     * Get number of tiles in a template
     */
    int GetTileCount(TemplateType tmpl);

    /**
     * Get land type for a template tile
     */
    LandType GetLandType(TemplateType tmpl, int icon);

    // =========================================================================
    // Cache Management
    // =========================================================================

    /**
     * Preload all templates for current theater
     */
    void PreloadAllTemplates();

    /**
     * Clear template cache
     */
    void ClearCache();

    /**
     * Get cache memory usage in bytes
     */
    size_t GetCacheSize() const;

private:
    TileRenderer();
    ~TileRenderer();

    // =========================================================================
    // Private Data
    // =========================================================================

    TheaterType current_theater_;

    // Template cache (indexed by TemplateType)
    std::unordered_map<int, std::unique_ptr<TemplateData>> template_cache_;

    // Overlay shape data (indexed by OverlayType)
    std::unordered_map<int, std::vector<uint8_t>> overlay_cache_;

    // =========================================================================
    // Private Methods
    // =========================================================================

    /**
     * Load a template from the current theater
     */
    TemplateData* LoadTemplate(TemplateType tmpl);

    /**
     * Load an overlay shape
     */
    bool LoadOverlay(OverlayType overlay);

    /**
     * Get filename for template type
     */
    std::string GetTemplateFilename(TemplateType tmpl) const;

    /**
     * Get filename for overlay type
     */
    std::string GetOverlayFilename(OverlayType overlay) const;
};

// =============================================================================
// Convenience Functions
// =============================================================================

/**
 * Draw a cell's terrain to screen
 *
 * Reads template info from CellClass and draws to buffer.
 *
 * @param buffer  Target buffer
 * @param cell_x  Cell X coordinate
 * @param cell_y  Cell Y coordinate
 * @param screen_x Screen X for drawing
 * @param screen_y Screen Y for drawing
 */
void Draw_Cell_Terrain(GraphicsBuffer& buffer,
                       int cell_x, int cell_y,
                       int screen_x, int screen_y);

/**
 * Draw a cell's overlay to screen
 */
void Draw_Cell_Overlay(GraphicsBuffer& buffer,
                       int cell_x, int cell_y,
                       int screen_x, int screen_y);

#endif // GAME_GRAPHICS_TILE_RENDERER_H
```

### src/game/graphics/tile_renderer.cpp
```cpp
/**
 * TileRenderer Implementation
 */

#include "game/graphics/tile_renderer.h"
#include "game/graphics/graphics_buffer.h"
#include "platform.h"
#include <cstring>
#include <cstdlib>

// =============================================================================
// Theater Info
// =============================================================================

static const char* THEATER_NAMES[] = {
    "TEMPERAT",
    "SNOW",
    "INTERIOR"
};

static const char* THEATER_EXTENSIONS[] = {
    ".TMP",
    ".SNO",
    ".INT"
};

const char* Theater_Name(TheaterType theater) {
    if (theater < 0 || theater >= THEATER_COUNT) {
        return "TEMPERAT";
    }
    return THEATER_NAMES[theater];
}

const char* Theater_Extension(TheaterType theater) {
    if (theater < 0 || theater >= THEATER_COUNT) {
        return ".TMP";
    }
    return THEATER_EXTENSIONS[theater];
}

// =============================================================================
// Template Filenames
// =============================================================================

// Map template types to base filenames
static const char* TEMPLATE_FILENAMES[] = {
    "CLEAR1",   // TEMPLATE_CLEAR1
    "W1",       // TEMPLATE_WATER1
    "W2",       // TEMPLATE_WATER2
    "SH1",      // TEMPLATE_SHORE1
    "SH2",      // TEMPLATE_SHORE2
    "SH3",      // TEMPLATE_SHORE3
    "SH4",      // TEMPLATE_SHORE4
    "SH5",      // TEMPLATE_SHORE5
    "SH6",      // TEMPLATE_SHORE6
    "SH7",      // TEMPLATE_SHORE7
    "SH8",      // TEMPLATE_SHORE8
    "CL1",      // TEMPLATE_CLIFF1
    "CL2",      // TEMPLATE_CLIFF2
    "RD01",     // TEMPLATE_ROAD1
    "RD02",     // TEMPLATE_ROAD2
    "RD03",     // TEMPLATE_ROAD3
    "RG01",     // TEMPLATE_ROUGH1
    "RG02",     // TEMPLATE_ROUGH2
};

// Overlay filenames (these are SHP files)
static const char* OVERLAY_FILENAMES[] = {
    "SBAG",     // OVERLAY_SANDBAG_WALL
    "CYCL",     // OVERLAY_CYCLONE_WALL
    "BRIK",     // OVERLAY_BRICK_WALL
    "BARB",     // OVERLAY_BARBWIRE_WALL
    "WOOD",     // OVERLAY_WOOD_WALL
    "GOLD01",   // OVERLAY_GOLD1
    "GOLD02",   // OVERLAY_GOLD2
    "GOLD03",   // OVERLAY_GOLD3
    "GOLD04",   // OVERLAY_GOLD4
    "GEM01",    // OVERLAY_GEMS1
    "GEM02",    // OVERLAY_GEMS2
    "GEM03",    // OVERLAY_GEMS3
    "GEM04",    // OVERLAY_GEMS4
    "V12",      // OVERLAY_V12
    "V13",      // OVERLAY_V13
    "V14",      // OVERLAY_V14
    "V15",      // OVERLAY_V15
    "V16",      // OVERLAY_V16
    "V17",      // OVERLAY_V17
    "V18",      // OVERLAY_V18
    "FLAGFLY",  // OVERLAY_FLAG
    "FENC",     // OVERLAY_WOOD_FENCE
};

// =============================================================================
// TileRenderer Singleton
// =============================================================================

TileRenderer& TileRenderer::Instance() {
    static TileRenderer instance;
    return instance;
}

TileRenderer::TileRenderer()
    : current_theater_(THEATER_NONE)
{
}

TileRenderer::~TileRenderer() {
    ClearCache();
}

// =============================================================================
// Theater Management
// =============================================================================

bool TileRenderer::SetTheater(TheaterType theater) {
    if (theater < 0 || theater >= THEATER_COUNT) {
        return false;
    }

    if (theater == current_theater_) {
        return true;  // Already loaded
    }

    // Clear existing cache
    ClearCache();

    // Load theater palette
    char palette_name[64];
    snprintf(palette_name, sizeof(palette_name), "%s.PAL", Theater_Name(theater));

    uint8_t palette_data[768];
    if (Platform_Palette_Load(palette_name, palette_data) == 0) {
        // Apply palette
        PaletteEntry entries[256];
        for (int i = 0; i < 256; i++) {
            entries[i].r = palette_data[i * 3 + 0];
            entries[i].g = palette_data[i * 3 + 1];
            entries[i].b = palette_data[i * 3 + 2];
        }
        Platform_Graphics_SetPalette(entries, 0, 256);
    }

    current_theater_ = theater;
    return true;
}

// =============================================================================
// Template Loading
// =============================================================================

std::string TileRenderer::GetTemplateFilename(TemplateType tmpl) const {
    if (tmpl < 0 || tmpl >= TEMPLATE_COUNT) {
        return "";
    }

    std::string filename = TEMPLATE_FILENAMES[tmpl];
    filename += Theater_Extension(current_theater_);
    return filename;
}

TemplateData* TileRenderer::LoadTemplate(TemplateType tmpl) {
    if (current_theater_ == THEATER_NONE) {
        return nullptr;
    }

    std::string filename = GetTemplateFilename(tmpl);
    if (filename.empty()) {
        return nullptr;
    }

    // Get file size
    int32_t size = Platform_Mix_GetSize(filename.c_str());
    if (size <= 0) {
        // Try with .TMP extension if theater-specific failed
        filename = TEMPLATE_FILENAMES[tmpl];
        filename += ".TMP";
        size = Platform_Mix_GetSize(filename.c_str());
        if (size <= 0) {
            return nullptr;
        }
    }

    // Read file data
    std::vector<uint8_t> file_data(size);
    if (Platform_Mix_Read(filename.c_str(), file_data.data(), size) != size) {
        return nullptr;
    }

    // Parse template using platform layer
    PlatformTemplate* platform_tmpl = Platform_Template_LoadFromMemory(
        file_data.data(), size, TILE_WIDTH, TILE_HEIGHT);

    if (!platform_tmpl) {
        return nullptr;
    }

    // Create template data
    auto data = std::make_unique<TemplateData>();
    data->type = tmpl;
    data->tile_count = Platform_Template_GetTileCount(platform_tmpl);

    int32_t tw, th;
    Platform_Template_GetTileSize(platform_tmpl, &tw, &th);
    data->width = 1;  // Most templates are 1 cell wide
    data->height = 1; // Will be overridden for multi-tile templates

    // Allocate pixel storage
    data->pixels.resize(data->tile_count * TILE_SIZE);

    // Read all tiles
    for (int i = 0; i < data->tile_count; i++) {
        Platform_Template_GetTile(platform_tmpl,
                                  i,
                                  data->pixels.data() + i * TILE_SIZE,
                                  TILE_SIZE);
    }

    // Initialize land types (default to LAND_CLEAR)
    data->land.resize(data->tile_count, LAND_CLEAR);

    // Set land types based on template type
    // This is a simplified version - real code would read from TMP file
    switch (tmpl) {
        case TEMPLATE_WATER1:
        case TEMPLATE_WATER2:
            std::fill(data->land.begin(), data->land.end(), LAND_WATER);
            break;
        case TEMPLATE_SHORE1:
        case TEMPLATE_SHORE2:
        case TEMPLATE_SHORE3:
        case TEMPLATE_SHORE4:
        case TEMPLATE_SHORE5:
        case TEMPLATE_SHORE6:
        case TEMPLATE_SHORE7:
        case TEMPLATE_SHORE8:
            std::fill(data->land.begin(), data->land.end(), LAND_BEACH);
            break;
        case TEMPLATE_CLIFF1:
        case TEMPLATE_CLIFF2:
            std::fill(data->land.begin(), data->land.end(), LAND_ROCK);
            break;
        case TEMPLATE_ROAD1:
        case TEMPLATE_ROAD2:
        case TEMPLATE_ROAD3:
            std::fill(data->land.begin(), data->land.end(), LAND_ROAD);
            break;
        case TEMPLATE_ROUGH1:
        case TEMPLATE_ROUGH2:
            std::fill(data->land.begin(), data->land.end(), LAND_ROUGH);
            break;
        default:
            break;
    }

    Platform_Template_Free(platform_tmpl);

    // Store in cache and return
    TemplateData* ptr = data.get();
    template_cache_[static_cast<int>(tmpl)] = std::move(data);
    return ptr;
}

const TemplateData* TileRenderer::GetTemplate(TemplateType tmpl) {
    // Check cache first
    auto it = template_cache_.find(static_cast<int>(tmpl));
    if (it != template_cache_.end()) {
        return it->second.get();
    }

    // Load and cache
    return LoadTemplate(tmpl);
}

int TileRenderer::GetTileCount(TemplateType tmpl) {
    const TemplateData* data = GetTemplate(tmpl);
    return data ? data->tile_count : 0;
}

LandType TileRenderer::GetLandType(TemplateType tmpl, int icon) {
    const TemplateData* data = GetTemplate(tmpl);
    return data ? data->GetLandType(icon) : LAND_CLEAR;
}

// =============================================================================
// Tile Drawing
// =============================================================================

bool TileRenderer::DrawTile(GraphicsBuffer& buffer, int x, int y,
                             TemplateType tmpl, int icon) {
    return DrawTileWithLand(buffer, x, y, tmpl, icon, nullptr);
}

bool TileRenderer::DrawTileWithLand(GraphicsBuffer& buffer, int x, int y,
                                     TemplateType tmpl, int icon,
                                     LandType* land_out) {
    // Handle TEMPLATE_NONE as clear terrain
    if (tmpl == TEMPLATE_NONE) {
        DrawClear(buffer, x, y, x ^ y);
        if (land_out) *land_out = LAND_CLEAR;
        return true;
    }

    const TemplateData* data = GetTemplate(tmpl);
    if (!data) {
        // Template not found - draw clear
        DrawClear(buffer, x, y, x ^ y);
        if (land_out) *land_out = LAND_CLEAR;
        return false;
    }

    // Validate icon index
    if (icon < 0 || icon >= data->tile_count) {
        icon = 0;  // Default to first tile
    }

    // Get tile pixels
    const uint8_t* tile_pixels = data->GetTile(icon);
    if (!tile_pixels) {
        DrawClear(buffer, x, y, x ^ y);
        if (land_out) *land_out = LAND_CLEAR;
        return false;
    }

    // Get land type
    if (land_out) {
        *land_out = data->GetLandType(icon);
    }

    // Check if buffer is locked
    if (!buffer.IsLocked()) {
        return false;
    }

    // Clip to buffer bounds
    int buf_w = buffer.Get_Width();
    int buf_h = buffer.Get_Height();

    // Completely off-screen?
    if (x >= buf_w || y >= buf_h || x + TILE_WIDTH <= 0 || y + TILE_HEIGHT <= 0) {
        return true;  // Nothing to draw, but not an error
    }

    // Calculate clipping
    int src_x = 0;
    int src_y = 0;
    int dst_x = x;
    int dst_y = y;
    int width = TILE_WIDTH;
    int height = TILE_HEIGHT;

    if (x < 0) {
        src_x = -x;
        width += x;
        dst_x = 0;
    }
    if (y < 0) {
        src_y = -y;
        height += y;
        dst_y = 0;
    }
    if (dst_x + width > buf_w) {
        width = buf_w - dst_x;
    }
    if (dst_y + height > buf_h) {
        height = buf_h - dst_y;
    }

    if (width <= 0 || height <= 0) {
        return true;
    }

    // Blit tile to buffer (opaque - tiles fill entire cell)
    buffer.Blit_From_Raw(tile_pixels, TILE_WIDTH,
                         src_x, src_y, TILE_WIDTH, TILE_HEIGHT,
                         dst_x, dst_y, width, height);

    return true;
}

void TileRenderer::DrawClear(GraphicsBuffer& buffer, int x, int y, uint32_t seed) {
    // Use a simple hash to pick clear variation
    int variation = (seed * 2654435761u) % 4;

    // Try to draw clear1 template
    const TemplateData* data = GetTemplate(TEMPLATE_CLEAR1);
    if (data && data->tile_count > 0) {
        int icon = variation % data->tile_count;
        const uint8_t* tile_pixels = data->GetTile(icon);
        if (tile_pixels) {
            buffer.Blit_From_Raw(tile_pixels, TILE_WIDTH,
                                 0, 0, TILE_WIDTH, TILE_HEIGHT,
                                 x, y, TILE_WIDTH, TILE_HEIGHT);
            return;
        }
    }

    // Fallback: solid color
    buffer.Fill_Rect(x, y, TILE_WIDTH, TILE_HEIGHT, 21);  // Green-ish
}

// =============================================================================
// Overlay Drawing
// =============================================================================

std::string TileRenderer::GetOverlayFilename(OverlayType overlay) const {
    if (overlay < 0 || overlay >= OVERLAY_COUNT) {
        return "";
    }
    return std::string(OVERLAY_FILENAMES[overlay]) + ".SHP";
}

bool TileRenderer::LoadOverlay(OverlayType overlay) {
    std::string filename = GetOverlayFilename(overlay);
    if (filename.empty()) {
        return false;
    }

    // Load shape file
    PlatformShape* shape = Platform_Shape_Load(filename.c_str());
    if (!shape) {
        return false;
    }

    int frame_count = Platform_Shape_GetFrameCount(shape);
    if (frame_count <= 0) {
        Platform_Shape_Free(shape);
        return false;
    }

    // Cache all frames
    std::vector<uint8_t> all_frames(frame_count * TILE_SIZE);

    for (int i = 0; i < frame_count; i++) {
        Platform_Shape_GetFrame(shape, i,
                                all_frames.data() + i * TILE_SIZE,
                                TILE_SIZE);
    }

    Platform_Shape_Free(shape);

    overlay_cache_[static_cast<int>(overlay)] = std::move(all_frames);
    return true;
}

bool TileRenderer::DrawOverlay(GraphicsBuffer& buffer, int x, int y,
                                OverlayType overlay, int frame) {
    if (overlay == OVERLAY_NONE) {
        return true;  // Nothing to draw
    }

    // Check cache
    auto it = overlay_cache_.find(static_cast<int>(overlay));
    if (it == overlay_cache_.end()) {
        // Try to load
        if (!LoadOverlay(overlay)) {
            return false;
        }
        it = overlay_cache_.find(static_cast<int>(overlay));
        if (it == overlay_cache_.end()) {
            return false;
        }
    }

    const std::vector<uint8_t>& frames = it->second;
    int frame_count = static_cast<int>(frames.size() / TILE_SIZE);

    if (frame < 0 || frame >= frame_count) {
        frame = 0;
    }

    const uint8_t* pixels = frames.data() + frame * TILE_SIZE;

    // Draw with transparency (overlays have transparent pixels)
    buffer.Blit_From_Raw_Trans(pixels, TILE_WIDTH,
                                0, 0, TILE_WIDTH, TILE_HEIGHT,
                                x, y, TILE_WIDTH, TILE_HEIGHT,
                                0);  // Transparent color

    return true;
}

// =============================================================================
// Cache Management
// =============================================================================

void TileRenderer::PreloadAllTemplates() {
    for (int i = 0; i < TEMPLATE_COUNT; i++) {
        GetTemplate(static_cast<TemplateType>(i));
    }
}

void TileRenderer::ClearCache() {
    template_cache_.clear();
    overlay_cache_.clear();
}

size_t TileRenderer::GetCacheSize() const {
    size_t total = 0;

    for (const auto& pair : template_cache_) {
        total += pair.second->pixels.size();
        total += pair.second->land.size() * sizeof(LandType);
    }

    for (const auto& pair : overlay_cache_) {
        total += pair.second.size();
    }

    return total;
}

// =============================================================================
// Convenience Functions
// =============================================================================

void Draw_Cell_Terrain(GraphicsBuffer& buffer,
                       int cell_x, int cell_y,
                       int screen_x, int screen_y) {
    // This would integrate with CellClass from Phase 14
    // For now, just draw clear terrain
    TileRenderer::Instance().DrawClear(buffer, screen_x, screen_y,
                                        cell_x ^ (cell_y * 127));
}

void Draw_Cell_Overlay(GraphicsBuffer& buffer,
                       int cell_x, int cell_y,
                       int screen_x, int screen_y) {
    // This would integrate with CellClass from Phase 14
    // For now, nothing to draw
    (void)buffer;
    (void)cell_x;
    (void)cell_y;
    (void)screen_x;
    (void)screen_y;
}
```

### src/test_tile_rendering.cpp
```cpp
/**
 * Tile Rendering Test Program
 */

#include "game/graphics/tile_renderer.h"
#include "game/graphics/graphics_buffer.h"
#include "platform.h"
#include <cstdio>

// =============================================================================
// Test Utilities
// =============================================================================

static int tests_run = 0;
static int tests_passed = 0;

#define TEST_START(name) do { \
    printf("  Testing %s... ", name); \
    tests_run++; \
} while(0)

#define TEST_PASS() do { \
    printf("PASS\n"); \
    tests_passed++; \
} while(0)

#define TEST_FAIL(msg) do { \
    printf("FAIL: %s\n", msg); \
    return false; \
} while(0)

#define ASSERT(cond, msg) do { \
    if (!(cond)) TEST_FAIL(msg); \
} while(0)

// =============================================================================
// Unit Tests
// =============================================================================

bool test_theater_functions() {
    TEST_START("theater helper functions");

    // Test theater names
    ASSERT(strcmp(Theater_Name(THEATER_TEMPERATE), "TEMPERAT") == 0,
           "Temperate name wrong");
    ASSERT(strcmp(Theater_Name(THEATER_SNOW), "SNOW") == 0,
           "Snow name wrong");
    ASSERT(strcmp(Theater_Name(THEATER_INTERIOR), "INTERIOR") == 0,
           "Interior name wrong");

    // Test extensions
    ASSERT(strcmp(Theater_Extension(THEATER_TEMPERATE), ".TMP") == 0,
           "Temperate extension wrong");
    ASSERT(strcmp(Theater_Extension(THEATER_SNOW), ".SNO") == 0,
           "Snow extension wrong");
    ASSERT(strcmp(Theater_Extension(THEATER_INTERIOR), ".INT") == 0,
           "Interior extension wrong");

    // Test invalid theater
    ASSERT(Theater_Name((TheaterType)-1) != nullptr, "Invalid theater should return default");

    TEST_PASS();
    return true;
}

bool test_tile_renderer_singleton() {
    TEST_START("tile renderer singleton");

    TileRenderer& renderer1 = TileRenderer::Instance();
    TileRenderer& renderer2 = TileRenderer::Instance();

    ASSERT(&renderer1 == &renderer2, "Singleton should return same instance");
    ASSERT(!renderer1.IsTheaterLoaded(), "Should not have theater loaded initially");

    TEST_PASS();
    return true;
}

bool test_land_types() {
    TEST_START("land types");

    // Verify land type enum values
    ASSERT(LAND_CLEAR == 0, "LAND_CLEAR should be 0");
    ASSERT(LAND_ROAD == 1, "LAND_ROAD should be 1");
    ASSERT(LAND_WATER == 2, "LAND_WATER should be 2");
    ASSERT(LAND_COUNT == 9, "Should have 9 land types");

    TEST_PASS();
    return true;
}

bool test_template_types() {
    TEST_START("template types");

    ASSERT(TEMPLATE_NONE == -1, "TEMPLATE_NONE should be -1");
    ASSERT(TEMPLATE_CLEAR1 == 0, "TEMPLATE_CLEAR1 should be 0");
    ASSERT(TEMPLATE_COUNT > 0, "Should have positive template count");

    TEST_PASS();
    return true;
}

bool test_overlay_types() {
    TEST_START("overlay types");

    ASSERT(OVERLAY_NONE == -1, "OVERLAY_NONE should be -1");
    ASSERT(OVERLAY_SANDBAG_WALL == 0, "OVERLAY_SANDBAG_WALL should be 0");
    ASSERT(OVERLAY_COUNT > 0, "Should have positive overlay count");

    TEST_PASS();
    return true;
}

bool test_clear_terrain() {
    TEST_START("clear terrain drawing");

    GraphicsBuffer& screen = GraphicsBuffer::Screen();
    screen.Lock();
    screen.Clear(0);

    // Draw some clear terrain cells
    TileRenderer& renderer = TileRenderer::Instance();

    // Even without theater loaded, DrawClear should work
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 10; x++) {
            renderer.DrawClear(screen, x * TILE_WIDTH, y * TILE_HEIGHT, x ^ y);
        }
    }

    screen.Unlock();

    // Verify something was drawn
    screen.Lock();
    bool found_nonzero = false;
    for (int y = 0; y < 5 * TILE_HEIGHT && !found_nonzero; y++) {
        for (int x = 0; x < 10 * TILE_WIDTH && !found_nonzero; x++) {
            if (screen.Get_Pixel(x, y) != 0) {
                found_nonzero = true;
            }
        }
    }
    screen.Unlock();

    ASSERT(found_nonzero, "DrawClear should have drawn something");

    TEST_PASS();
    return true;
}

bool test_tile_clipping() {
    TEST_START("tile clipping");

    GraphicsBuffer buf(64, 64);
    buf.Lock();
    buf.Clear(0);

    TileRenderer& renderer = TileRenderer::Instance();

    // Draw partially off-screen tiles (should not crash)
    renderer.DrawClear(buf, -12, 10, 0);   // Left edge
    renderer.DrawClear(buf, 10, -12, 1);   // Top edge
    renderer.DrawClear(buf, 50, 10, 2);    // Right edge
    renderer.DrawClear(buf, 10, 50, 3);    // Bottom edge
    renderer.DrawClear(buf, -100, 10, 4);  // Way off left
    renderer.DrawClear(buf, 10, 200, 5);   // Way off bottom

    buf.Unlock();

    TEST_PASS();
    return true;
}

// =============================================================================
// Visual Test
// =============================================================================

void run_visual_test() {
    printf("\n=== Visual Tile Test ===\n");

    // Set up a simple palette
    uint8_t palette[768];
    for (int i = 0; i < 256; i++) {
        palette[i * 3 + 0] = i;
        palette[i * 3 + 1] = i;
        palette[i * 3 + 2] = i;
    }

    // Add some colors for variety
    // Greens for terrain
    for (int i = 16; i < 32; i++) {
        palette[i * 3 + 0] = 0;
        palette[i * 3 + 1] = (i - 16) * 8 + 64;
        palette[i * 3 + 2] = 0;
    }
    // Blues for water
    for (int i = 32; i < 48; i++) {
        palette[i * 3 + 0] = 0;
        palette[i * 3 + 1] = 0;
        palette[i * 3 + 2] = (i - 32) * 8 + 64;
    }
    // Browns for roads
    for (int i = 48; i < 64; i++) {
        palette[i * 3 + 0] = (i - 48) * 4 + 64;
        palette[i * 3 + 1] = (i - 48) * 3 + 32;
        palette[i * 3 + 2] = (i - 48) * 2;
    }

    Platform_Graphics_SetPalette((PaletteEntry*)palette, 0, 256);

    GraphicsBuffer& screen = GraphicsBuffer::Screen();
    screen.Lock();
    screen.Clear(0);

    printf("Drawing grid of clear terrain tiles...\n");

    TileRenderer& renderer = TileRenderer::Instance();

    // Draw a grid of tiles
    int cols = screen.Get_Width() / TILE_WIDTH;
    int rows = screen.Get_Height() / TILE_HEIGHT;

    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            // Vary the seed for different patterns
            uint32_t seed = x * 31 + y * 17;
            renderer.DrawClear(screen, x * TILE_WIDTH, y * TILE_HEIGHT, seed);
        }
    }

    // Draw some "water" tiles (blue rectangles for visualization)
    for (int y = 5; y < 8; y++) {
        for (int x = 10; x < 15; x++) {
            screen.Fill_Rect(x * TILE_WIDTH, y * TILE_HEIGHT,
                            TILE_WIDTH, TILE_HEIGHT, 40);
        }
    }

    // Draw a "road" (brown rectangles)
    for (int x = 0; x < cols; x++) {
        screen.Fill_Rect(x * TILE_WIDTH, 10 * TILE_HEIGHT,
                        TILE_WIDTH, TILE_HEIGHT, 56);
    }

    // Draw grid lines to show tile boundaries
    for (int y = 0; y <= rows; y++) {
        screen.Draw_HLine(0, y * TILE_HEIGHT, screen.Get_Width(), 100);
    }
    for (int x = 0; x <= cols; x++) {
        screen.Draw_VLine(x * TILE_WIDTH, 0, screen.Get_Height(), 100);
    }

    screen.Unlock();
    screen.Flip();

    printf("Tile grid displayed. Waiting 3 seconds...\n");
    Platform_Delay(3000);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("==========================================\n");
    printf("Tile Rendering Test Suite\n");
    printf("==========================================\n\n");

    // Initialize platform
    if (Platform_Init() != PLATFORM_RESULT_SUCCESS) {
        printf("ERROR: Failed to initialize platform\n");
        return 1;
    }

    if (Platform_Graphics_Init() != 0) {
        printf("ERROR: Failed to initialize graphics\n");
        Platform_Shutdown();
        return 1;
    }

    printf("=== Unit Tests ===\n\n");

    test_theater_functions();
    test_tile_renderer_singleton();
    test_land_types();
    test_template_types();
    test_overlay_types();
    test_clear_terrain();
    test_tile_clipping();

    printf("\n------------------------------------------\n");
    printf("Tests: %d/%d passed\n", tests_passed, tests_run);
    printf("------------------------------------------\n");

    if (tests_passed == tests_run) {
        run_visual_test();
    }

    Platform_Graphics_Shutdown();
    Platform_Shutdown();

    printf("\n==========================================\n");
    if (tests_passed == tests_run) {
        printf("All tests PASSED!\n");
    } else {
        printf("Some tests FAILED\n");
    }
    printf("==========================================\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
```

### ralph-tasks/verify/check-tile-rendering.sh
```bash
#!/bin/bash
# Verification script for Tile Rendering System

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Tile Rendering Implementation ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check files exist
echo "[1/5] Checking file existence..."
FILES=(
    "include/game/graphics/tile_renderer.h"
    "src/game/graphics/tile_renderer.cpp"
    "src/test_tile_rendering.cpp"
)

for file in "${FILES[@]}"; do
    if [ ! -f "$file" ]; then
        echo "VERIFY_FAILED: $file not found"
        exit 1
    fi
done
echo "  OK: All source files exist"

# Step 2: Check header syntax
echo "[2/5] Checking header syntax..."
if ! clang++ -std=c++17 -fsyntax-only \
    -I include \
    -I include/game \
    -I include/game/graphics \
    include/game/graphics/tile_renderer.h 2>&1; then
    echo "VERIFY_FAILED: Header has syntax errors"
    exit 1
fi
echo "  OK: Header compiles"

# Step 3: Build test executable
echo "[3/5] Building TestTileRendering..."
if ! cmake --build build --target TestTileRendering 2>&1; then
    echo "VERIFY_FAILED: Build failed"
    exit 1
fi
echo "  OK: Build succeeded"

# Step 4: Check executable exists
echo "[4/5] Checking executable..."
if [ ! -f "build/TestTileRendering" ]; then
    echo "VERIFY_FAILED: TestTileRendering executable not found"
    exit 1
fi
echo "  OK: Executable exists"

# Step 5: Run tests
echo "[5/5] Running unit tests..."
if timeout 30 ./build/TestTileRendering 2>&1 | grep -q "All tests PASSED"; then
    echo "  OK: Tests passed"
else
    echo "VERIFY_FAILED: Tests did not pass"
    exit 1
fi

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## CMakeLists.txt Additions

```cmake
# Add tile rendering sources
target_sources(game_core PRIVATE
    src/game/graphics/tile_renderer.cpp
)

# Tile Rendering Test
add_executable(TestTileRendering src/test_tile_rendering.cpp)

target_include_directories(TestTileRendering PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/game/graphics
)

target_link_libraries(TestTileRendering PRIVATE
    game_core
    redalert_platform
)

add_test(NAME TestTileRendering COMMAND TestTileRendering)
```

## Implementation Steps

1. Create `include/game/graphics/tile_renderer.h`
2. Create `src/game/graphics/tile_renderer.cpp`
3. Create `src/test_tile_rendering.cpp`
4. Update CMakeLists.txt
5. Create verification script
6. Build and test

## Success Criteria

- [ ] TileRenderer compiles without warnings
- [ ] Theater helper functions work
- [ ] Singleton pattern works correctly
- [ ] DrawClear produces visible output
- [ ] DrawTile handles clipping correctly
- [ ] Template caching works
- [ ] Land type queries return correct values
- [ ] Overlay drawing works (when overlays available)
- [ ] Cache memory tracking works
- [ ] Theater switching works (clears cache)
- [ ] All unit tests pass
- [ ] Visual test shows tile grid

## Common Issues

### Templates Not Loading
**Symptom**: All tiles show clear terrain
**Solution**: Ensure MIX files are registered with asset system

### Wrong Theater Palette
**Symptom**: Colors look wrong
**Solution**: Load correct .PAL file when switching theaters

### Tile Corruption
**Symptom**: Tiles look scrambled
**Solution**: Check TILE_SIZE matches actual template format

### Cache Memory Leak
**Symptom**: Memory grows continuously
**Solution**: Ensure ClearCache() is called on theater switch

## Completion Promise
When verification passes, output:
```
<promise>TASK_15C_COMPLETE</promise>
```

## Escape Hatch
If stuck after 15 iterations:
- Document what's blocking in `ralph-tasks/blocked/15c.md`
- Output: `<promise>TASK_15C_BLOCKED</promise>`

## Max Iterations
15
