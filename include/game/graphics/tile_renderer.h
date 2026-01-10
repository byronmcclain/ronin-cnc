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

// Include map.h for TheaterType definition (avoid redefinition)
// NOTE: map.h includes gscreen.h, coord.h but NOT tile_renderer.h, so no circular dependency
#include "game/map.h"

// Include cell.h for OverlayType, LandType definitions (avoid redefinition)
#include "game/cell.h"

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

// TheaterType is defined in game/map.h (THEATER_NONE, THEATER_TEMPERATE, THEATER_SNOW, THEATER_INTERIOR)

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

    TEMPLATE_COUNT
};

// =============================================================================
// Overlay Type & Land Type
// =============================================================================
// NOTE: OverlayType and LandType are now defined in game/cell.h
// to avoid redefinition conflicts. Use those definitions directly.

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
     */
    bool DrawTileWithLand(GraphicsBuffer& buffer, int x, int y,
                          TemplateType tmpl, int icon, LandType* land_out);

    /**
     * Draw clear terrain (default fill)
     *
     * Uses random clear template for visual variation.
     */
    void DrawClear(GraphicsBuffer& buffer, int x, int y, uint32_t seed = 0);

    // =========================================================================
    // Overlay Drawing
    // =========================================================================

    /**
     * Draw an overlay tile
     */
    bool DrawOverlay(GraphicsBuffer& buffer, int x, int y,
                     OverlayType overlay, int frame);

    // =========================================================================
    // Template Information
    // =========================================================================

    /**
     * Get template data (loads if needed)
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
