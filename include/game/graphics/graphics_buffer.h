/**
 * GraphicsBuffer - Platform-independent graphics buffer wrapper
 *
 * Provides a clean interface for pixel manipulation, blitting, and
 * rendering that wraps the platform layer. This replaces the original
 * WIN32LIB GraphicBufferClass.
 *
 * Usage:
 *   GraphicsBuffer& screen = GraphicsBuffer::Screen();
 *   screen.Lock();
 *   screen.Put_Pixel(100, 100, 15);  // Draw white pixel
 *   screen.Fill_Rect(10, 10, 50, 50, 4);  // Red rectangle
 *   screen.Unlock();
 *   screen.Flip();  // Display to screen
 *
 * Threading:
 *   NOT thread-safe. All rendering must happen on main thread.
 *
 * Original: WIN32LIB/GBUFFER.H, WIN32LIB/GBUFFER.CPP
 */

#ifndef GAME_GRAPHICS_BUFFER_H
#define GAME_GRAPHICS_BUFFER_H

#include <cstdint>
#include <cstddef>

// Forward declare platform types
struct PlatformSurface;

/**
 * GraphicsBuffer - 8-bit palettized graphics buffer
 *
 * Can represent either the main screen buffer (singleton) or
 * an off-screen surface for caching/compositing.
 */
class GraphicsBuffer {
public:
    // =========================================================================
    // Constants
    // =========================================================================

    static constexpr int DEFAULT_WIDTH = 640;
    static constexpr int DEFAULT_HEIGHT = 400;
    static constexpr int BITS_PER_PIXEL = 8;

    // Transparent color index (standard for Red Alert sprites)
    static constexpr uint8_t TRANSPARENT_COLOR = 0;

    // =========================================================================
    // Singleton Access (Main Screen Buffer)
    // =========================================================================

    /**
     * Get the main screen buffer singleton
     *
     * This wraps the platform layer's back buffer and is the primary
     * target for all game rendering.
     *
     * @return Reference to the screen buffer
     */
    static GraphicsBuffer& Screen();

    /**
     * Check if screen buffer is initialized
     */
    static bool IsScreenInitialized();

    // =========================================================================
    // Construction / Destruction
    // =========================================================================

    /**
     * Create an off-screen buffer
     *
     * Use this for caching rendered content or intermediate compositing.
     * The buffer is allocated with Platform_Alloc and must be freed.
     *
     * @param width  Buffer width in pixels
     * @param height Buffer height in pixels
     */
    GraphicsBuffer(int width, int height);

    /**
     * Wrap an existing buffer (non-owning)
     *
     * Use this to create a GraphicsBuffer interface around existing
     * pixel data without taking ownership.
     *
     * @param pixels Pointer to pixel data (must remain valid)
     * @param width  Buffer width
     * @param height Buffer height
     * @param pitch  Row stride in bytes
     */
    GraphicsBuffer(uint8_t* pixels, int width, int height, int pitch);

    /**
     * Destructor - frees owned buffer if any
     */
    ~GraphicsBuffer();

    // Non-copyable (buffer ownership)
    GraphicsBuffer(const GraphicsBuffer&) = delete;
    GraphicsBuffer& operator=(const GraphicsBuffer&) = delete;

    // Move semantics
    GraphicsBuffer(GraphicsBuffer&& other) noexcept;
    GraphicsBuffer& operator=(GraphicsBuffer&& other) noexcept;

    // =========================================================================
    // Buffer State
    // =========================================================================

    /**
     * Lock the buffer for direct pixel access
     *
     * Must be called before any drawing operations.
     * For the screen buffer, this locks the platform back buffer.
     * Nested locks are tracked and handled correctly.
     *
     * @return true if lock succeeded
     */
    bool Lock();

    /**
     * Unlock the buffer
     *
     * Call when done with direct pixel access.
     * Must match each Lock() call.
     */
    void Unlock();

    /**
     * Check if buffer is currently locked
     */
    bool IsLocked() const { return lock_count_ > 0; }

    /**
     * Get raw pixel buffer pointer
     *
     * Only valid while locked. Returns nullptr if not locked.
     */
    uint8_t* Get_Buffer() { return IsLocked() ? pixels_ : nullptr; }
    const uint8_t* Get_Buffer() const { return IsLocked() ? pixels_ : nullptr; }

    /**
     * Get buffer dimensions
     */
    int Get_Width() const { return width_; }
    int Get_Height() const { return height_; }
    int Get_Pitch() const { return pitch_; }

    /**
     * Get total buffer size in bytes
     */
    size_t Get_Size() const { return static_cast<size_t>(pitch_) * height_; }

    /**
     * Check if this is the main screen buffer
     */
    bool IsScreenBuffer() const { return is_screen_; }

    // =========================================================================
    // Pixel Operations
    // =========================================================================

    /**
     * Set a single pixel
     *
     * @param x     X coordinate (0 to width-1)
     * @param y     Y coordinate (0 to height-1)
     * @param color Palette index (0-255)
     */
    void Put_Pixel(int x, int y, uint8_t color);

    /**
     * Get a single pixel
     *
     * @param x X coordinate
     * @param y Y coordinate
     * @return Palette index at (x,y), or 0 if out of bounds
     */
    uint8_t Get_Pixel(int x, int y) const;

    /**
     * Draw a horizontal line
     *
     * @param x      Starting X coordinate
     * @param y      Y coordinate
     * @param length Line length in pixels
     * @param color  Palette index
     */
    void Draw_HLine(int x, int y, int length, uint8_t color);

    /**
     * Draw a vertical line
     *
     * @param x      X coordinate
     * @param y      Starting Y coordinate
     * @param length Line length in pixels
     * @param color  Palette index
     */
    void Draw_VLine(int x, int y, int length, uint8_t color);

    /**
     * Draw a rectangle outline
     *
     * @param x, y   Top-left corner
     * @param w, h   Width and height
     * @param color  Palette index
     */
    void Draw_Rect(int x, int y, int w, int h, uint8_t color);

    /**
     * Fill a rectangle
     *
     * @param x, y   Top-left corner
     * @param w, h   Width and height
     * @param color  Palette index
     */
    void Fill_Rect(int x, int y, int w, int h, uint8_t color);

    /**
     * Clear entire buffer to a color
     *
     * @param color Palette index (default 0 = black/transparent)
     */
    void Clear(uint8_t color = 0);

    // =========================================================================
    // Blitting Operations
    // =========================================================================

    /**
     * Blit from another buffer (opaque copy)
     *
     * Copies all pixels, including color 0.
     *
     * @param src    Source buffer
     * @param src_x  Source region X
     * @param src_y  Source region Y
     * @param dst_x  Destination X
     * @param dst_y  Destination Y
     * @param width  Region width (0 = full source width)
     * @param height Region height (0 = full source height)
     */
    void Blit_From(const GraphicsBuffer& src,
                   int src_x, int src_y,
                   int dst_x, int dst_y,
                   int width = 0, int height = 0);

    /**
     * Blit from another buffer with transparency
     *
     * Skips pixels with color index 0 (transparent).
     *
     * @param src    Source buffer
     * @param src_x  Source region X
     * @param src_y  Source region Y
     * @param dst_x  Destination X
     * @param dst_y  Destination Y
     * @param width  Region width (0 = full source width)
     * @param height Region height (0 = full source height)
     */
    void Blit_From_Trans(const GraphicsBuffer& src,
                         int src_x, int src_y,
                         int dst_x, int dst_y,
                         int width = 0, int height = 0);

    /**
     * Blit from raw pixel data (opaque)
     *
     * @param src_pixels Source pixel array
     * @param src_pitch  Source row stride
     * @param src_x      Source region X
     * @param src_y      Source region Y
     * @param src_w      Source total width (for bounds)
     * @param src_h      Source total height (for bounds)
     * @param dst_x      Destination X
     * @param dst_y      Destination Y
     * @param width      Region width
     * @param height     Region height
     */
    void Blit_From_Raw(const uint8_t* src_pixels, int src_pitch,
                       int src_x, int src_y, int src_w, int src_h,
                       int dst_x, int dst_y,
                       int width, int height);

    /**
     * Blit from raw pixel data with transparency
     *
     * @param trans_color Color index to treat as transparent
     */
    void Blit_From_Raw_Trans(const uint8_t* src_pixels, int src_pitch,
                             int src_x, int src_y, int src_w, int src_h,
                             int dst_x, int dst_y,
                             int width, int height,
                             uint8_t trans_color = TRANSPARENT_COLOR);

    // =========================================================================
    // Color Remapping
    // =========================================================================

    /**
     * Apply a color remap table to a region
     *
     * For each pixel in the region: pixel = remap_table[pixel]
     *
     * Used for:
     * - House colors (player/enemy tinting)
     * - Damage effects (red flash)
     * - Shadow effects
     *
     * @param x, y        Region top-left
     * @param w, h        Region size
     * @param remap_table 256-byte lookup table
     */
    void Remap(int x, int y, int w, int h, const uint8_t* remap_table);

    /**
     * Blit with color remapping
     *
     * Combines blit and remap in one operation for efficiency.
     *
     * @param src         Source buffer
     * @param src_x       Source region X
     * @param src_y       Source region Y
     * @param dst_x       Destination X
     * @param dst_y       Destination Y
     * @param width       Region width
     * @param height      Region height
     * @param remap_table 256-byte lookup table
     * @param transparent If true, skip color 0
     */
    void Blit_Remap(const GraphicsBuffer& src,
                    int src_x, int src_y,
                    int dst_x, int dst_y,
                    int width, int height,
                    const uint8_t* remap_table,
                    bool transparent = true);

    // =========================================================================
    // Display Operations (Screen Buffer Only)
    // =========================================================================

    /**
     * Flip buffer to screen
     *
     * Only valid for the screen buffer. Converts the 8-bit indexed
     * buffer to 32-bit RGBA using the current palette and displays it.
     *
     * @return true on success
     */
    bool Flip();

private:
    // =========================================================================
    // Private Members
    // =========================================================================

    uint8_t* pixels_;      // Pixel data pointer
    int width_;            // Width in pixels
    int height_;           // Height in pixels
    int pitch_;            // Row stride in bytes
    int lock_count_;       // Nested lock counter
    bool is_screen_;       // True if this is the main screen buffer
    bool owns_buffer_;     // True if we should free pixels_ on destruction

    // =========================================================================
    // Private Construction (for screen singleton)
    // =========================================================================

    // Private default constructor for screen singleton
    GraphicsBuffer();

    // Refresh screen buffer pointer from platform layer
    void RefreshScreenBuffer();

    // =========================================================================
    // Clipping Helpers
    // =========================================================================

    /**
     * Clip a blit operation to buffer bounds
     *
     * Adjusts all coordinates and dimensions to stay within both
     * source and destination buffers.
     *
     * @return true if any visible region remains after clipping
     */
    static bool ClipBlit(
        int& src_x, int& src_y, int src_w, int src_h,
        int& dst_x, int& dst_y, int dst_w, int dst_h,
        int& width, int& height);

    /**
     * Clip a rectangle to buffer bounds
     */
    bool ClipRect(int& x, int& y, int& w, int& h) const;
};

// =============================================================================
// Convenience Typedefs (Compatibility with Original Code)
// =============================================================================

// Original code used these names
using GraphicBufferClass = GraphicsBuffer;

// Global screen buffer reference (matches original "SeenBuff" global)
#define SeenBuff GraphicsBuffer::Screen()

// Hidden page (for double-buffering compositing)
// In our implementation, platform handles double-buffering,
// so this is just an alias to the same screen buffer
#define HidPage GraphicsBuffer::Screen()

#endif // GAME_GRAPHICS_BUFFER_H
