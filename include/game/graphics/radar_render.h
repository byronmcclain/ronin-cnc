/**
 * Radar Renderer - Minimap Display
 *
 * Renders the radar minimap showing terrain, units, and structures.
 *
 * Original: CODE/RADAR.CPP
 */

#ifndef GAME_GRAPHICS_RADAR_RENDER_H
#define GAME_GRAPHICS_RADAR_RENDER_H

#include "game/graphics/graphics_buffer.h"
#include <cstdint>
#include <memory>

// =============================================================================
// Forward Declarations
// =============================================================================

class ITerrainProvider;

// =============================================================================
// Constants
// =============================================================================

// Radar dimensions (original game values)
constexpr int RADAR_WIDTH = 160;
constexpr int RADAR_HEIGHT = 136;
constexpr int RADAR_X = 480;  // Right side, in sidebar
constexpr int RADAR_Y = 16;   // Below tabs

// Radar cell pixel size (for 128x128 map)
constexpr int RADAR_CELL_SIZE = 1;  // Each cell is 1 pixel

// =============================================================================
// Radar State
// =============================================================================

enum class RadarState : int {
    DISABLED = 0,     // No radar building
    JAMMED = 1,       // Enemy has GPS scrambler
    ACTIVE = 2,       // Radar working normally
    SPYING = 3        // Showing enemy radar (spy infiltration)
};

// =============================================================================
// Radar Blip Types
// =============================================================================

/**
 * RadarBlip - Represents a unit/building on radar
 */
struct RadarBlip {
    int cell_x;        // Map cell X
    int cell_y;        // Map cell Y
    uint8_t color;     // Palette index for blip
    bool is_building;  // True for structures (draw larger)
    bool is_selected;  // Blink if selected
    bool is_enemy;     // Enemy unit/building

    RadarBlip()
        : cell_x(0)
        , cell_y(0)
        , color(0)
        , is_building(false)
        , is_selected(false)
        , is_enemy(false)
    {}

    RadarBlip(int x, int y, uint8_t c, bool bldg = false)
        : cell_x(x)
        , cell_y(y)
        , color(c)
        , is_building(bldg)
        , is_selected(false)
        , is_enemy(false)
    {}
};

// =============================================================================
// Radar Renderer Class
// =============================================================================

/**
 * RadarRenderer - Draws the radar minimap
 */
class RadarRenderer {
public:
    RadarRenderer();
    ~RadarRenderer();

    // Prevent copying
    RadarRenderer(const RadarRenderer&) = delete;
    RadarRenderer& operator=(const RadarRenderer&) = delete;

    // =========================================================================
    // Initialization
    // =========================================================================

    /**
     * Initialize radar renderer
     * @param map_width Map width in cells
     * @param map_height Map height in cells
     * @return true if successful
     */
    bool Initialize(int map_width, int map_height);

    /**
     * Set the terrain provider for map data
     */
    void SetTerrainProvider(ITerrainProvider* provider) { terrain_ = provider; }

    /**
     * Shutdown and release resources
     */
    void Shutdown();

    /**
     * Check if initialized
     */
    bool IsInitialized() const { return initialized_; }

    // =========================================================================
    // State
    // =========================================================================

    /**
     * Set radar state (disabled, jammed, active)
     */
    void SetState(RadarState state) { state_ = state; }

    /**
     * Get current radar state
     */
    RadarState GetState() const { return state_; }

    /**
     * Set owning player for color determination
     */
    void SetPlayerHouse(int house) { player_house_ = house; }

    // =========================================================================
    // Viewport
    // =========================================================================

    /**
     * Set the current viewport position (in cells)
     */
    void SetViewport(int cell_x, int cell_y, int width_cells, int height_cells);

    /**
     * Convert radar click to map cell
     * @return true if valid cell
     */
    bool RadarToCell(int radar_x, int radar_y, int* cell_x, int* cell_y) const;

    /**
     * Convert map cell to radar position
     */
    void CellToRadar(int cell_x, int cell_y, int* radar_x, int* radar_y) const;

    // =========================================================================
    // Blip Management
    // =========================================================================

    /**
     * Clear all blips
     */
    void ClearBlips();

    /**
     * Add a blip for a unit/building
     */
    void AddBlip(const RadarBlip& blip);

    /**
     * Add a blip with parameters
     */
    void AddBlip(int cell_x, int cell_y, uint8_t color, bool is_building = false);

    /**
     * Get blip count
     */
    int GetBlipCount() const { return blip_count_; }

    // =========================================================================
    // Rendering
    // =========================================================================

    /**
     * Update the radar image (call when map changes)
     */
    void UpdateTerrainImage();

    /**
     * Draw the complete radar
     * @param buffer Target graphics buffer
     */
    void Draw(GraphicsBuffer& buffer);

    /**
     * Draw terrain only
     */
    void DrawTerrain(GraphicsBuffer& buffer);

    /**
     * Draw unit/building blips
     */
    void DrawBlips(GraphicsBuffer& buffer);

    /**
     * Draw viewport rectangle
     */
    void DrawViewport(GraphicsBuffer& buffer);

    /**
     * Draw radar frame/border
     */
    void DrawFrame(GraphicsBuffer& buffer);

    /**
     * Draw jammed static effect
     */
    void DrawJammed(GraphicsBuffer& buffer);

    // =========================================================================
    // Hit Testing
    // =========================================================================

    /**
     * Check if point is in radar area
     */
    bool HitTest(int x, int y) const;

    // =========================================================================
    // Layout
    // =========================================================================

    /**
     * Set radar position
     */
    void SetPosition(int x, int y);

    /**
     * Get radar X position
     */
    int GetX() const { return radar_x_; }

    /**
     * Get radar Y position
     */
    int GetY() const { return radar_y_; }

    /**
     * Get radar width
     */
    int GetWidth() const { return radar_width_; }

    /**
     * Get radar height
     */
    int GetHeight() const { return radar_height_; }

    // =========================================================================
    // Animation
    // =========================================================================

    /**
     * Update animations (blinking, static)
     */
    void Update();

    /**
     * Set blink rate for selected units
     */
    void SetBlinkRate(int frames) { blink_rate_ = frames; }

private:
    // Internal helpers
    void CreateTerrainBuffer();
    uint8_t GetTerrainColor(int cell_x, int cell_y) const;
    void DrawPixel(GraphicsBuffer& buffer, int x, int y, uint8_t color);

    // State
    bool initialized_;
    RadarState state_;
    int player_house_;

    // Position and size
    int radar_x_;
    int radar_y_;
    int radar_width_;
    int radar_height_;

    // Map info
    int map_width_;
    int map_height_;
    float scale_x_;
    float scale_y_;

    // Viewport in cells
    int viewport_x_;
    int viewport_y_;
    int viewport_width_;
    int viewport_height_;

    // Terrain provider
    ITerrainProvider* terrain_;

    // Terrain image buffer (cached)
    std::unique_ptr<uint8_t[]> terrain_buffer_;
    bool terrain_dirty_;

    // Blips
    static constexpr int MAX_BLIPS = 512;
    RadarBlip blips_[MAX_BLIPS];
    int blip_count_;

    // Animation
    int blink_counter_;
    int blink_rate_;
    bool blink_state_;

    // Jammed static animation
    int static_frame_;
};

#endif // GAME_GRAPHICS_RADAR_RENDER_H
