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
    data->width = 1;
    data->height = 1;

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
        if (tile_pixels && buffer.IsLocked()) {
            int buf_w = buffer.Get_Width();
            int buf_h = buffer.Get_Height();

            // Check bounds
            if (x < buf_w && y < buf_h && x + TILE_WIDTH > 0 && y + TILE_HEIGHT > 0) {
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

                if (width > 0 && height > 0) {
                    buffer.Blit_From_Raw(tile_pixels, TILE_WIDTH,
                                         src_x, src_y, TILE_WIDTH, TILE_HEIGHT,
                                         dst_x, dst_y, width, height);
                    return;
                }
            }
        }
    }

    // Fallback: solid color
    if (buffer.IsLocked()) {
        // Calculate clipped rect
        int dst_x = x < 0 ? 0 : x;
        int dst_y = y < 0 ? 0 : y;
        int width = TILE_WIDTH - (x < 0 ? -x : 0);
        int height = TILE_HEIGHT - (y < 0 ? -y : 0);

        int buf_w = buffer.Get_Width();
        int buf_h = buffer.Get_Height();

        if (dst_x + width > buf_w) width = buf_w - dst_x;
        if (dst_y + height > buf_h) height = buf_h - dst_y;

        if (width > 0 && height > 0) {
            buffer.Fill_Rect(dst_x, dst_y, width, height, 21);  // Green-ish
        }
    }
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
