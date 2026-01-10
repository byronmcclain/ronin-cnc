/**
 * Render Pipeline - Main Graphics Orchestration
 *
 * Coordinates all rendering stages from terrain through UI.
 * Manages dirty rectangle tracking, layer sorting, and frame timing.
 *
 * Original: CODE/DISPLAY.CPP
 */

#ifndef GAME_GRAPHICS_RENDER_PIPELINE_H
#define GAME_GRAPHICS_RENDER_PIPELINE_H

#include "game/graphics/render_layer.h"
#include "game/graphics/graphics_buffer.h"
#include <cstdint>
#include <vector>
#include <memory>

// =============================================================================
// Forward Declarations
// =============================================================================

class TileRenderer;
class ShapeRenderer;
class PaletteManager;
class MouseCursor;
class SidebarRenderer;
class RadarRenderer;

// =============================================================================
// Constants
// =============================================================================

constexpr int MAX_DIRTY_RECTS = 128;
constexpr int DIRTY_RECT_MERGE_THRESHOLD = 32;  // Merge if overlap margin < this

// Default viewport dimensions (game area, excluding sidebar)
constexpr int DEFAULT_TACTICAL_WIDTH = 480;
constexpr int DEFAULT_TACTICAL_HEIGHT = 384;

// =============================================================================
// Render Pipeline Class
// =============================================================================

/**
 * RenderPipeline - Main rendering coordinator
 *
 * Manages the complete render frame:
 * 1. Collect dirty rectangles from changed objects
 * 2. Sort objects by layer and Y coordinate
 * 3. Render each layer in order
 * 4. Handle UI overlay (sidebar, radar)
 * 5. Draw cursor on top
 */
class RenderPipeline {
public:
    // Singleton access
    static RenderPipeline& Instance();

    // Prevent copying
    RenderPipeline(const RenderPipeline&) = delete;
    RenderPipeline& operator=(const RenderPipeline&) = delete;

    // =========================================================================
    // Initialization
    // =========================================================================

    /**
     * Initialize the render pipeline
     * @param screen_width Full screen width in pixels
     * @param screen_height Full screen height in pixels
     * @return true if successful
     */
    bool Initialize(int screen_width, int screen_height);

    /**
     * Shutdown and release resources
     */
    void Shutdown();

    /**
     * Check if initialized
     */
    bool IsInitialized() const { return initialized_; }

    // =========================================================================
    // Viewport Control
    // =========================================================================

    /**
     * Set the tactical viewport (game area, not including sidebar)
     */
    void SetTacticalViewport(int x, int y, int width, int height);

    /**
     * Get the current tactical viewport
     */
    const Viewport& GetTacticalViewport() const { return tactical_viewport_; }

    /**
     * Set the world scroll position
     */
    void SetScrollPosition(int world_x, int world_y);

    /**
     * Get the world scroll position
     */
    void GetScrollPosition(int* world_x, int* world_y) const;

    /**
     * Convert world coordinates to screen coordinates
     * @return true if visible on screen
     */
    bool WorldToScreen(int world_x, int world_y, int* screen_x, int* screen_y) const;

    /**
     * Convert screen coordinates to world coordinates
     */
    void ScreenToWorld(int screen_x, int screen_y, int* world_x, int* world_y) const;

    /**
     * Check if a world rectangle is visible
     */
    bool IsVisible(int world_x, int world_y, int width, int height) const;

    // =========================================================================
    // Dirty Rectangle Management
    // =========================================================================

    /**
     * Mark a screen area as needing redraw
     */
    void AddDirtyRect(int x, int y, int width, int height);

    /**
     * Mark a world area as needing redraw
     */
    void AddDirtyWorldRect(int world_x, int world_y, int width, int height);

    /**
     * Mark the entire screen as dirty
     */
    void MarkFullRedraw();

    /**
     * Clear all dirty rectangles
     */
    void ClearDirtyRects();

    /**
     * Get number of dirty rectangles
     */
    int GetDirtyRectCount() const { return static_cast<int>(dirty_rects_.size()); }

    /**
     * Enable/disable dirty rectangle optimization
     * When disabled, full screen is redrawn every frame
     */
    void SetDirtyRectEnabled(bool enabled) { dirty_rect_enabled_ = enabled; }
    bool IsDirtyRectEnabled() const { return dirty_rect_enabled_; }

    // =========================================================================
    // Renderable Management
    // =========================================================================

    /**
     * Add an object to be rendered this frame
     * Objects are sorted by layer, then by Y coordinate
     */
    void AddRenderable(IRenderable* obj);

    /**
     * Clear all renderables (call at start of frame)
     */
    void ClearRenderables();

    /**
     * Get number of queued renderables
     */
    int GetRenderableCount() const { return static_cast<int>(render_queue_.size()); }

    // =========================================================================
    // Frame Rendering
    // =========================================================================

    /**
     * Begin a new render frame
     * Clears render queue and prepares for new objects
     */
    void BeginFrame();

    /**
     * Render all queued objects to the back buffer
     */
    void RenderFrame();

    /**
     * End frame and present to screen
     */
    void EndFrame();

    /**
     * Convenience method: BeginFrame + RenderFrame + EndFrame
     */
    void DrawFrame();

    // =========================================================================
    // Individual Render Stages
    // =========================================================================

    /**
     * Render terrain tiles for visible area
     */
    void RenderTerrain();

    /**
     * Render ground smudges (craters, scorch marks)
     */
    void RenderSmudges();

    /**
     * Render overlays (ore, gems, walls)
     */
    void RenderOverlays();

    /**
     * Render building foundations
     */
    void RenderBibs();

    /**
     * Render shadows
     */
    void RenderShadows();

    /**
     * Render ground objects (infantry, vehicles)
     */
    void RenderGroundObjects();

    /**
     * Render buildings
     */
    void RenderBuildings();

    /**
     * Render air units (aircraft, parachutes)
     */
    void RenderAirUnits();

    /**
     * Render projectiles and explosions
     */
    void RenderProjectiles();

    /**
     * Render visual effects (smoke, fire)
     */
    void RenderEffects();

    /**
     * Render selection boxes
     */
    void RenderSelection();

    /**
     * Render shroud/fog of war
     */
    void RenderShroud();

    /**
     * Render UI elements (sidebar, radar)
     */
    void RenderUI();

    /**
     * Render mouse cursor (topmost)
     */
    void RenderCursor();

    // =========================================================================
    // UI Component Registration
    // =========================================================================

    /**
     * Register the sidebar renderer
     */
    void SetSidebarRenderer(SidebarRenderer* sidebar) { sidebar_ = sidebar; }

    /**
     * Register the radar renderer
     */
    void SetRadarRenderer(RadarRenderer* radar) { radar_ = radar; }

    /**
     * Set the terrain tile renderer
     */
    void SetTileRenderer(TileRenderer* tiles) { tile_renderer_ = tiles; }

    // =========================================================================
    // Statistics
    // =========================================================================

    /**
     * Get render statistics from last frame
     */
    const RenderStats& GetStats() const { return stats_; }

    /**
     * Reset statistics
     */
    void ResetStats() { stats_.Reset(); }

    // =========================================================================
    // Debug
    // =========================================================================

    /**
     * Enable debug visualization
     */
    void SetDebugMode(bool enabled) { debug_mode_ = enabled; }
    bool IsDebugMode() const { return debug_mode_; }

    /**
     * Draw debug overlays (dirty rects, layer info)
     */
    void DrawDebugOverlay();

private:
    RenderPipeline();
    ~RenderPipeline();

    // Internal helpers
    void MergeDirtyRects();
    void SortRenderQueue();
    void RenderLayer(RenderLayer layer);
    void ClipToViewport(DirtyRect& rect) const;

    // State
    bool initialized_;
    int screen_width_;
    int screen_height_;

    // Viewports
    Viewport tactical_viewport_;  // Game world view
    int scroll_x_;                // World X offset
    int scroll_y_;                // World Y offset

    // Dirty rectangle tracking
    std::vector<DirtyRect> dirty_rects_;
    bool dirty_rect_enabled_;
    bool full_redraw_pending_;

    // Render queue
    std::vector<RenderEntry> render_queue_;

    // Registered renderers
    TileRenderer* tile_renderer_;
    SidebarRenderer* sidebar_;
    RadarRenderer* radar_;

    // Statistics
    RenderStats stats_;

    // Debug
    bool debug_mode_;
};

// =============================================================================
// Terrain Callback Interface
// =============================================================================

/**
 * ITerrainProvider - Interface for map data access
 *
 * The render pipeline uses this to query terrain for visible cells
 */
class ITerrainProvider {
public:
    virtual ~ITerrainProvider() = default;

    /**
     * Get terrain tile index for a cell
     * @param cell_x Cell X coordinate
     * @param cell_y Cell Y coordinate
     * @return Tile index or -1 if invalid
     */
    virtual int GetTerrainTile(int cell_x, int cell_y) const = 0;

    /**
     * Get terrain icon (sub-tile) for a cell
     * @param cell_x Cell X coordinate
     * @param cell_y Cell Y coordinate
     * @return Icon index within the tile
     */
    virtual int GetTerrainIcon(int cell_x, int cell_y) const = 0;

    /**
     * Check if cell is within map bounds
     */
    virtual bool IsValidCell(int cell_x, int cell_y) const = 0;

    /**
     * Get map dimensions
     */
    virtual void GetMapSize(int* width, int* height) const = 0;
};

#endif // GAME_GRAPHICS_RENDER_PIPELINE_H
