/**
 * ShapeRenderer - SHP File Rendering System
 *
 * Draws sprites from SHP files with full support for transparency,
 * color remapping (house colors), and shadow effects.
 *
 * Usage:
 *   ShapeRenderer renderer;
 *   renderer.Load("MTNK.SHP");
 *   renderer.Draw(screen, x, y, frame);
 *   renderer.DrawRemapped(screen, x, y, frame, house_remap);
 *
 * Performance:
 *   Frame data is cached after first access for fast repeated drawing.
 *   Use ClearCache() to free memory if needed.
 *
 * Original: WIN32LIB/SHAPE.CPP, CODE/DISPLAY.CPP (Draw_Shape)
 */

#ifndef GAME_GRAPHICS_SHAPE_RENDERER_H
#define GAME_GRAPHICS_SHAPE_RENDERER_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <memory>

// Forward declarations
class GraphicsBuffer;
struct PlatformShape;

// =============================================================================
// Drawing Flags
// =============================================================================

/**
 * ShapeFlags - Drawing mode flags
 *
 * Can be combined with bitwise OR for multiple effects.
 */
enum ShapeFlags : uint32_t {
    SHAPE_NORMAL      = 0x0000,  // Standard transparent draw
    SHAPE_GHOST       = 0x0001,  // Semi-transparent (50% checkerboard)
    SHAPE_FADING      = 0x0002,  // Fade effect (darken)
    SHAPE_PREDATOR    = 0x0004,  // Stealth shimmer
    SHAPE_SHADOW      = 0x0008,  // Shadow-only (mask with darkening)
    SHAPE_FLAT        = 0x0010,  // Single color (for selection boxes)
    SHAPE_CENTER      = 0x0020,  // Center on coordinates
    SHAPE_FLIP_X      = 0x0040,  // Horizontal flip
    SHAPE_FLIP_Y      = 0x0080,  // Vertical flip
    SHAPE_PRIORITY    = 0x0100,  // Priority (for sorting)
};

// =============================================================================
// Frame Data Cache
// =============================================================================

/**
 * ShapeFrame - Cached decompressed frame data
 */
struct ShapeFrame {
    int16_t x_offset;       // Draw offset from shape origin
    int16_t y_offset;
    int16_t width;          // Frame dimensions
    int16_t height;
    std::vector<uint8_t> pixels;  // Decompressed pixel data (width * height)

    ShapeFrame() : x_offset(0), y_offset(0), width(0), height(0) {}

    size_t GetSize() const { return static_cast<size_t>(width) * height; }
    bool IsValid() const { return width > 0 && height > 0 && !pixels.empty(); }
};

// =============================================================================
// ShapeRenderer Class
// =============================================================================

/**
 * ShapeRenderer - Renders SHP sprites to GraphicsBuffer
 *
 * Each instance represents one loaded SHP file.
 * Thread-safety: NOT thread-safe. Use separate instances per thread.
 */
class ShapeRenderer {
public:
    // =========================================================================
    // Construction / Destruction
    // =========================================================================

    ShapeRenderer();
    ~ShapeRenderer();

    // Non-copyable (owns resources)
    ShapeRenderer(const ShapeRenderer&) = delete;
    ShapeRenderer& operator=(const ShapeRenderer&) = delete;

    // Move semantics
    ShapeRenderer(ShapeRenderer&& other) noexcept;
    ShapeRenderer& operator=(ShapeRenderer&& other) noexcept;

    // =========================================================================
    // Loading
    // =========================================================================

    /**
     * Load a shape file from MIX archives
     *
     * @param filename Shape filename (e.g., "MTNK.SHP", "MOUSE.SHP")
     * @return true if loaded successfully
     */
    bool Load(const char* filename);

    /**
     * Load a shape file from raw memory
     *
     * @param data     Raw SHP file data
     * @param size     Data size in bytes
     * @param name     Optional name for debugging
     * @return true if loaded successfully
     */
    bool LoadFromMemory(const uint8_t* data, int32_t size, const char* name = nullptr);

    /**
     * Check if a shape is loaded
     */
    bool IsLoaded() const { return shape_ != nullptr; }

    /**
     * Unload the current shape and free resources
     */
    void Unload();

    /**
     * Get the loaded shape name
     */
    const char* GetName() const { return name_.c_str(); }

    // =========================================================================
    // Shape Information
    // =========================================================================

    /**
     * Get number of frames in the shape
     */
    int GetFrameCount() const;

    /**
     * Get maximum shape dimensions
     *
     * These are the largest width/height of any frame.
     */
    void GetSize(int* width, int* height) const;
    int GetWidth() const;
    int GetHeight() const;

    /**
     * Get specific frame dimensions
     *
     * @param frame Frame index
     * @param width Output width (null-safe)
     * @param height Output height (null-safe)
     * @return true if frame exists
     */
    bool GetFrameSize(int frame, int* width, int* height) const;

    /**
     * Get frame offset (draw position relative to origin)
     *
     * @param frame Frame index
     * @param x_offset Output X offset (null-safe)
     * @param y_offset Output Y offset (null-safe)
     * @return true if frame exists
     */
    bool GetFrameOffset(int frame, int* x_offset, int* y_offset) const;

    // =========================================================================
    // Drawing - Basic
    // =========================================================================

    /**
     * Draw a frame to a buffer (transparent, no remapping)
     *
     * @param buffer Target graphics buffer (must be locked)
     * @param x      Screen X coordinate
     * @param y      Screen Y coordinate
     * @param frame  Frame index (0 to frame_count-1)
     * @param flags  Drawing flags (ShapeFlags)
     * @return true if drawn successfully
     */
    bool Draw(GraphicsBuffer& buffer, int x, int y, int frame,
              uint32_t flags = SHAPE_NORMAL);

    /**
     * Draw with color remapping (for house colors)
     *
     * @param buffer      Target graphics buffer
     * @param x, y        Screen coordinates
     * @param frame       Frame index
     * @param remap_table 256-byte color lookup table
     * @param flags       Drawing flags
     * @return true if drawn successfully
     */
    bool DrawRemapped(GraphicsBuffer& buffer, int x, int y, int frame,
                      const uint8_t* remap_table, uint32_t flags = SHAPE_NORMAL);

    /**
     * Draw shadow (darkening effect using shape as mask)
     *
     * @param buffer       Target graphics buffer
     * @param x, y         Screen coordinates
     * @param frame        Frame index
     * @param shadow_table 256-byte shadow lookup table
     * @return true if drawn successfully
     */
    bool DrawShadow(GraphicsBuffer& buffer, int x, int y, int frame,
                    const uint8_t* shadow_table);

    // =========================================================================
    // Drawing - Advanced
    // =========================================================================

    /**
     * Draw with ghost effect (50% transparency via checkerboard)
     *
     * Creates semi-transparent effect by drawing only every other pixel.
     * Original used this for cloaked units and ghost effects.
     *
     * @param buffer Target graphics buffer
     * @param x, y   Screen coordinates
     * @param frame  Frame index
     * @param phase  Animation phase (0-1) for alternating pixels
     * @return true if drawn successfully
     */
    bool DrawGhost(GraphicsBuffer& buffer, int x, int y, int frame,
                   int phase = 0);

    /**
     * Draw with fading effect (darken towards black)
     *
     * @param buffer      Target graphics buffer
     * @param x, y        Screen coordinates
     * @param frame       Frame index
     * @param fade_table  256-byte fade lookup table
     * @param fade_level  Fade amount (0 = none, 15 = maximum)
     * @return true if drawn successfully
     */
    bool DrawFading(GraphicsBuffer& buffer, int x, int y, int frame,
                    const uint8_t* fade_table, int fade_level);

    /**
     * Draw with predator effect (stealth shimmer)
     *
     * @param buffer Target graphics buffer
     * @param x, y   Screen coordinates
     * @param frame  Frame index
     * @param phase  Animation phase for shimmer
     * @return true if drawn successfully
     */
    bool DrawPredator(GraphicsBuffer& buffer, int x, int y, int frame,
                      int phase);

    /**
     * Draw as solid flat color (for selection boxes, highlights)
     *
     * @param buffer Target graphics buffer
     * @param x, y   Screen coordinates
     * @param frame  Frame index
     * @param color  Solid color index
     * @return true if drawn successfully
     */
    bool DrawFlat(GraphicsBuffer& buffer, int x, int y, int frame,
                  uint8_t color);

    // =========================================================================
    // Cache Management
    // =========================================================================

    /**
     * Pre-cache all frames
     *
     * Decompresses and caches all frames in memory for fast drawing.
     * Call this for frequently-used shapes to avoid per-draw decompression.
     */
    void PrecacheAllFrames();

    /**
     * Pre-cache a specific frame
     *
     * @param frame Frame index to cache
     */
    void PrecacheFrame(int frame);

    /**
     * Clear the frame cache
     *
     * Frees memory used by cached frame data.
     */
    void ClearCache();

    /**
     * Get total cache memory usage in bytes
     */
    size_t GetCacheSize() const;

private:
    // =========================================================================
    // Private Members
    // =========================================================================

    PlatformShape* shape_;                   // Platform shape handle
    std::string name_;                       // Shape filename
    int width_;                              // Max width
    int height_;                             // Max height
    int frame_count_;                        // Number of frames
    std::vector<ShapeFrame> frame_cache_;    // Cached frame data

    // =========================================================================
    // Private Methods
    // =========================================================================

    /**
     * Get or decompress a frame
     *
     * Returns cached data if available, otherwise decompresses and caches.
     */
    const ShapeFrame* GetFrame(int frame);

    /**
     * Internal draw implementation
     *
     * All public Draw* methods call this with appropriate parameters.
     */
    bool DrawInternal(GraphicsBuffer& buffer, int x, int y,
                      const ShapeFrame& frame,
                      const uint8_t* remap_table,
                      uint32_t flags,
                      uint8_t flat_color);
};

// =============================================================================
// Convenience Function
// =============================================================================

/**
 * Draw a shape frame directly (loads shape temporarily)
 *
 * This is a convenience for one-off draws. For repeated drawing,
 * create a ShapeRenderer instance instead.
 *
 * @param buffer   Target graphics buffer
 * @param filename Shape filename
 * @param x, y     Screen coordinates
 * @param frame    Frame index
 * @param flags    Drawing flags
 * @return true if drawn successfully
 */
bool Draw_Shape(GraphicsBuffer& buffer, const char* filename,
                int x, int y, int frame, uint32_t flags = SHAPE_NORMAL);

// =============================================================================
// Shape Cache Manager
// =============================================================================

/**
 * ShapeCache - Global shape caching system
 *
 * Caches frequently-used shapes to avoid reloading from disk.
 * Use this for shapes that are drawn every frame (units, buildings).
 */
class ShapeCache {
public:
    /**
     * Get the global shape cache instance
     */
    static ShapeCache& Instance();

    /**
     * Get a shape renderer (loads if not cached)
     *
     * The returned pointer is owned by the cache - do not delete.
     *
     * @param filename Shape filename
     * @return Pointer to shape renderer, or nullptr on error
     */
    ShapeRenderer* Get(const char* filename);

    /**
     * Preload a shape into cache
     *
     * @param filename Shape filename
     * @return true if loaded successfully
     */
    bool Preload(const char* filename);

    /**
     * Remove a shape from cache
     *
     * @param filename Shape filename
     */
    void Remove(const char* filename);

    /**
     * Clear entire cache
     */
    void Clear();

    /**
     * Get number of cached shapes
     */
    int GetCount() const;

    /**
     * Get total memory usage
     */
    size_t GetMemoryUsage() const;

private:
    ShapeCache() = default;
    ~ShapeCache();

    ShapeCache(const ShapeCache&) = delete;
    ShapeCache& operator=(const ShapeCache&) = delete;

    struct CacheEntry {
        std::unique_ptr<ShapeRenderer> renderer;
        int access_count;
    };

    std::vector<std::pair<std::string, CacheEntry>> cache_;

    CacheEntry* Find(const char* filename);
};

#endif // GAME_GRAPHICS_SHAPE_RENDERER_H
