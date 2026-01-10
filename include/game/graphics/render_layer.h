/**
 * Render Layer Definitions for Draw Ordering
 *
 * Defines the layer system used to ensure objects are drawn
 * in the correct back-to-front order.
 *
 * Original: CODE/DISPLAY.CPP
 */

#ifndef GAME_GRAPHICS_RENDER_LAYER_H
#define GAME_GRAPHICS_RENDER_LAYER_H

#include <cstdint>

// =============================================================================
// Render Layer Enumeration
// =============================================================================

/**
 * RenderLayer - Layer identifiers for rendering order
 *
 * Objects are drawn from lowest to highest layer value.
 */
enum class RenderLayer : int {
    TERRAIN = 0,      // Base terrain tiles
    SMUDGE = 1,       // Craters, scorch marks
    OVERLAY = 2,      // Ore, gems, walls
    BIB = 3,          // Building foundation
    SHADOW = 4,       // Ground shadows
    GROUND = 5,       // Infantry, vehicles on ground
    BUILDING = 6,     // Structures
    AIR = 7,          // Aircraft, parachutes
    PROJECTILE = 8,   // Bullets, missiles, explosions
    EFFECT = 9,       // Visual effects (smoke, fire)
    SELECTION = 10,   // Selection boxes
    SHROUD = 11,      // Fog of war
    UI = 12,          // Sidebar, radar, etc.
    CURSOR = 13,      // Mouse cursor (topmost)

    COUNT = 14
};

// =============================================================================
// Forward Declarations
// =============================================================================

class GraphicsBuffer;

// =============================================================================
// Renderable Interface
// =============================================================================

/**
 * IRenderable - Interface for any object that can be drawn
 */
class IRenderable {
public:
    virtual ~IRenderable() = default;

    /**
     * Get the layer this object belongs to
     */
    virtual RenderLayer GetRenderLayer() const = 0;

    /**
     * Get Y coordinate for depth sorting (higher Y = drawn later)
     */
    virtual int GetSortY() const = 0;

    /**
     * Draw the object at screen coordinates
     */
    virtual void Draw(GraphicsBuffer& buffer, int screen_x, int screen_y) = 0;

    /**
     * Get world coordinates for visibility testing
     */
    virtual int GetWorldX() const = 0;
    virtual int GetWorldY() const = 0;

    /**
     * Get bounding box for dirty rect tracking
     */
    virtual void GetBounds(int& x, int& y, int& width, int& height) const = 0;
};

// =============================================================================
// Dirty Rectangle
// =============================================================================

/**
 * DirtyRect - Rectangle that needs redrawing
 */
struct DirtyRect {
    int x, y;
    int width, height;

    DirtyRect() : x(0), y(0), width(0), height(0) {}
    DirtyRect(int x_, int y_, int w, int h) : x(x_), y(y_), width(w), height(h) {}

    /**
     * Check if two rects overlap
     */
    bool Overlaps(const DirtyRect& other) const {
        return !(x + width <= other.x || other.x + other.width <= x ||
                 y + height <= other.y || other.y + other.height <= y);
    }

    /**
     * Merge with another rect (union)
     */
    void Merge(const DirtyRect& other) {
        int new_x = (x < other.x) ? x : other.x;
        int new_y = (y < other.y) ? y : other.y;
        int new_right = (x + width > other.x + other.width) ? x + width : other.x + other.width;
        int new_bottom = (y + height > other.y + other.height) ? y + height : other.y + other.height;
        x = new_x;
        y = new_y;
        width = new_right - x;
        height = new_bottom - y;
    }

    /**
     * Check if empty
     */
    bool IsEmpty() const { return width <= 0 || height <= 0; }
};

// =============================================================================
// Render Queue Entry
// =============================================================================

/**
 * RenderEntry - Entry in render queue for sorting
 */
struct RenderEntry {
    IRenderable* object;
    int sort_y;
    RenderLayer layer;

    RenderEntry(IRenderable* obj, int y, RenderLayer lay)
        : object(obj), sort_y(y), layer(lay) {}

    /**
     * Sort by layer first, then by Y coordinate
     */
    bool operator<(const RenderEntry& other) const {
        if (layer != other.layer) {
            return static_cast<int>(layer) < static_cast<int>(other.layer);
        }
        return sort_y < other.sort_y;
    }
};

// =============================================================================
// Render Statistics
// =============================================================================

/**
 * RenderStats - Performance monitoring data
 */
struct RenderStats {
    int terrain_tiles_drawn;
    int objects_drawn;
    int dirty_rects_count;
    int pixels_filled;
    float frame_time_ms;

    RenderStats() { Reset(); }

    void Reset() {
        terrain_tiles_drawn = 0;
        objects_drawn = 0;
        dirty_rects_count = 0;
        pixels_filled = 0;
        frame_time_ms = 0.0f;
    }
};

// =============================================================================
// Viewport Structure
// =============================================================================

/**
 * Viewport - Visible area of the game world
 */
struct Viewport {
    int x;       // World X coordinate of viewport left edge
    int y;       // World Y coordinate of viewport top edge
    int width;   // Viewport width in pixels
    int height;  // Viewport height in pixels

    Viewport() : x(0), y(0), width(480), height(384) {}
    Viewport(int x_, int y_, int w, int h) : x(x_), y(y_), width(w), height(h) {}
};

#endif // GAME_GRAPHICS_RENDER_LAYER_H
