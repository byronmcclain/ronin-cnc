# Task 15a: GraphicsBuffer Wrapper Class

## Dependencies
- Phase 14 (Game Code Port) must be complete
- Platform graphics subsystem must be functional (`Platform_Graphics_*` APIs)
- Platform buffer operations must work (`Platform_Buffer_*` APIs)
- GScreenClass must exist in `include/game/gscreen.h`

## Context
The original Red Alert uses `GraphicBufferClass` (from WIN32LIB) for all screen rendering. This class wraps DirectDraw surfaces and provides methods like `Lock()`, `Unlock()`, `Get_Buffer()`, and `Blit_To_Primary()`.

Our port replaces this with a `GraphicsBuffer` class that wraps the platform layer. This is the **foundation for all rendering** - every sprite, tile, UI element, and effect goes through this buffer. Getting this right is critical.

This task creates a clean, efficient wrapper that:
1. Provides the same interface as the original `GraphicBufferClass`
2. Uses the platform layer for actual rendering
3. Handles the 8-bit palettized pixel format correctly
4. Includes bounds checking and safety features

## Objective
Create a `GraphicsBuffer` class that wraps platform layer graphics, providing a familiar interface for the ported game code while using modern platform APIs underneath.

## Technical Background

### Original GraphicBufferClass (WIN32LIB)
```cpp
// Original usage pattern from RED ALERT code:
GraphicBufferClass SeenBuff(640, 400, NULL, 640);  // Back buffer
GraphicBufferClass HidPage(640, 400, NULL, 640);   // Hidden page

// Lock for direct pixel access
SeenBuff.Lock();
unsigned char* pixels = (unsigned char*)SeenBuff.Get_Buffer();
int pitch = SeenBuff.Get_Width();  // Note: width, not pitch!

// Draw a pixel
pixels[y * pitch + x] = color_index;

// Unlock and display
SeenBuff.Unlock();
SeenBuff.Blit_To_Primary();  // Copy to primary surface
```

### Key Differences in Our Implementation
1. **No DirectDraw**: We use SDL2 through the platform layer
2. **Pitch vs Width**: Our platform layer properly separates pitch from width
3. **Single Back Buffer**: Platform layer manages the back buffer; we provide access
4. **Palette Conversion**: Platform layer converts 8-bit indexed to 32-bit RGBA

### Platform Layer APIs Used
```c
// From platform.h:
int32_t Platform_Graphics_GetBackBuffer(uint8_t** pixels,
    int32_t* width, int32_t* height, int32_t* pitch);
void Platform_Graphics_ClearBackBuffer(uint8_t color);
int32_t Platform_Graphics_Flip(void);

// Buffer operations:
void Platform_Buffer_Clear(uint8_t* buffer, int32_t size, uint8_t value);
void Platform_Buffer_FillRect(uint8_t* buffer, int32_t pitch, ...);
void Platform_Buffer_ToBuffer(uint8_t* dest, ...);  // Opaque blit
void Platform_Buffer_ToBufferTrans(uint8_t* dest, ...);  // Transparent blit
void Platform_Buffer_Remap(uint8_t* buffer, ...);  // Color remap
```

### Memory Layout
The back buffer is a linear array of 8-bit palette indices:
```
Offset = y * pitch + x

For 640x400 buffer with pitch=640:
Row 0: pixels[0..639]
Row 1: pixels[640..1279]
...
Row 399: pixels[255360..255999]

Total size: 640 * 400 = 256,000 bytes
```

### Coordinate System
- Origin (0,0) is top-left corner
- X increases rightward (0 to 639)
- Y increases downward (0 to 399)
- This matches original Red Alert and standard graphics conventions

## Deliverables
- [ ] `include/game/graphics/graphics_buffer.h` - Header file with class definition
- [ ] `src/game/graphics/graphics_buffer.cpp` - Implementation
- [ ] `src/test_graphics_buffer.cpp` - Comprehensive test program
- [ ] `ralph-tasks/verify/check-graphics-buffer.sh` - Verification script
- [ ] CMakeLists.txt updated with new files
- [ ] All tests pass

## Files to Create

### include/game/graphics/graphics_buffer.h
```cpp
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
```

### src/game/graphics/graphics_buffer.cpp
```cpp
/**
 * GraphicsBuffer Implementation
 *
 * Wraps platform layer graphics for game rendering.
 */

#include "game/graphics/graphics_buffer.h"
#include "platform.h"
#include <algorithm>
#include <cstring>

// =============================================================================
// Static Singleton
// =============================================================================

// Screen buffer singleton instance
static GraphicsBuffer* s_screen_buffer = nullptr;

GraphicsBuffer& GraphicsBuffer::Screen() {
    if (!s_screen_buffer) {
        s_screen_buffer = new GraphicsBuffer();
    }
    return *s_screen_buffer;
}

bool GraphicsBuffer::IsScreenInitialized() {
    return s_screen_buffer != nullptr && Platform_Graphics_IsInitialized();
}

// =============================================================================
// Construction / Destruction
// =============================================================================

// Private constructor for screen singleton
GraphicsBuffer::GraphicsBuffer()
    : pixels_(nullptr)
    , width_(DEFAULT_WIDTH)
    , height_(DEFAULT_HEIGHT)
    , pitch_(DEFAULT_WIDTH)
    , lock_count_(0)
    , is_screen_(true)
    , owns_buffer_(false)
{
    // Don't get buffer here - wait for Lock()
    // Platform might not be initialized yet
}

// Create off-screen buffer
GraphicsBuffer::GraphicsBuffer(int width, int height)
    : pixels_(nullptr)
    , width_(width)
    , height_(height)
    , pitch_(width)  // Tight packing for off-screen buffers
    , lock_count_(0)
    , is_screen_(false)
    , owns_buffer_(true)
{
    if (width > 0 && height > 0) {
        size_t size = static_cast<size_t>(pitch_) * height_;
        pixels_ = static_cast<uint8_t*>(Platform_Alloc(size, 0));
        if (pixels_) {
            Platform_ZeroMemory(pixels_, size);
        }
    }
}

// Wrap existing buffer (non-owning)
GraphicsBuffer::GraphicsBuffer(uint8_t* pixels, int width, int height, int pitch)
    : pixels_(pixels)
    , width_(width)
    , height_(height)
    , pitch_(pitch)
    , lock_count_(0)
    , is_screen_(false)
    , owns_buffer_(false)
{
}

// Destructor
GraphicsBuffer::~GraphicsBuffer() {
    if (owns_buffer_ && pixels_) {
        Platform_Free(pixels_, static_cast<size_t>(pitch_) * height_);
        pixels_ = nullptr;
    }

    // Clear singleton pointer if this was the screen buffer
    if (is_screen_ && s_screen_buffer == this) {
        s_screen_buffer = nullptr;
    }
}

// Move constructor
GraphicsBuffer::GraphicsBuffer(GraphicsBuffer&& other) noexcept
    : pixels_(other.pixels_)
    , width_(other.width_)
    , height_(other.height_)
    , pitch_(other.pitch_)
    , lock_count_(other.lock_count_)
    , is_screen_(other.is_screen_)
    , owns_buffer_(other.owns_buffer_)
{
    other.pixels_ = nullptr;
    other.owns_buffer_ = false;
    other.lock_count_ = 0;
}

// Move assignment
GraphicsBuffer& GraphicsBuffer::operator=(GraphicsBuffer&& other) noexcept {
    if (this != &other) {
        // Free existing buffer if owned
        if (owns_buffer_ && pixels_) {
            Platform_Free(pixels_, static_cast<size_t>(pitch_) * height_);
        }

        pixels_ = other.pixels_;
        width_ = other.width_;
        height_ = other.height_;
        pitch_ = other.pitch_;
        lock_count_ = other.lock_count_;
        is_screen_ = other.is_screen_;
        owns_buffer_ = other.owns_buffer_;

        other.pixels_ = nullptr;
        other.owns_buffer_ = false;
        other.lock_count_ = 0;
    }
    return *this;
}

// =============================================================================
// Buffer State
// =============================================================================

bool GraphicsBuffer::Lock() {
    if (is_screen_) {
        // For screen buffer, get fresh pointer from platform layer
        if (lock_count_ == 0) {
            RefreshScreenBuffer();
        }
        if (!pixels_) {
            return false;
        }
    } else {
        // For off-screen buffers, just check we have a buffer
        if (!pixels_) {
            return false;
        }
    }

    lock_count_++;
    return true;
}

void GraphicsBuffer::Unlock() {
    if (lock_count_ > 0) {
        lock_count_--;
    }
}

void GraphicsBuffer::RefreshScreenBuffer() {
    if (!Platform_Graphics_IsInitialized()) {
        pixels_ = nullptr;
        return;
    }

    int32_t w, h, p;
    if (Platform_Graphics_GetBackBuffer(&pixels_, &w, &h, &p) == 0) {
        width_ = w;
        height_ = h;
        pitch_ = p;
    } else {
        pixels_ = nullptr;
    }
}

// =============================================================================
// Pixel Operations
// =============================================================================

void GraphicsBuffer::Put_Pixel(int x, int y, uint8_t color) {
    if (!IsLocked()) return;
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;

    pixels_[y * pitch_ + x] = color;
}

uint8_t GraphicsBuffer::Get_Pixel(int x, int y) const {
    if (!IsLocked()) return 0;
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return 0;

    return pixels_[y * pitch_ + x];
}

void GraphicsBuffer::Draw_HLine(int x, int y, int length, uint8_t color) {
    if (!IsLocked()) return;
    if (length <= 0) return;

    // Clip Y
    if (y < 0 || y >= height_) return;

    // Clip X start
    if (x < 0) {
        length += x;
        x = 0;
    }

    // Clip X end
    if (x + length > width_) {
        length = width_ - x;
    }

    if (length <= 0) return;

    // Use platform function for efficiency
    Platform_Buffer_HLine(pixels_, pitch_, width_, height_, x, y, length, color);
}

void GraphicsBuffer::Draw_VLine(int x, int y, int length, uint8_t color) {
    if (!IsLocked()) return;
    if (length <= 0) return;

    // Clip X
    if (x < 0 || x >= width_) return;

    // Clip Y start
    if (y < 0) {
        length += y;
        y = 0;
    }

    // Clip Y end
    if (y + length > height_) {
        length = height_ - y;
    }

    if (length <= 0) return;

    // Use platform function for efficiency
    Platform_Buffer_VLine(pixels_, pitch_, width_, height_, x, y, length, color);
}

void GraphicsBuffer::Draw_Rect(int x, int y, int w, int h, uint8_t color) {
    if (!IsLocked()) return;
    if (w <= 0 || h <= 0) return;

    // Draw four lines
    Draw_HLine(x, y, w, color);              // Top
    Draw_HLine(x, y + h - 1, w, color);      // Bottom
    Draw_VLine(x, y, h, color);              // Left
    Draw_VLine(x + w - 1, y, h, color);      // Right
}

void GraphicsBuffer::Fill_Rect(int x, int y, int w, int h, uint8_t color) {
    if (!IsLocked()) return;
    if (!ClipRect(x, y, w, h)) return;

    // Use platform function for efficiency
    Platform_Buffer_FillRect(pixels_, pitch_, width_, height_, x, y, w, h, color);
}

void GraphicsBuffer::Clear(uint8_t color) {
    if (is_screen_ && !IsLocked()) {
        // For screen buffer, can use platform clear without locking
        Platform_Graphics_ClearBackBuffer(color);
        return;
    }

    if (!IsLocked()) return;

    // For off-screen or already-locked screen buffer
    if (pitch_ == width_) {
        // Tight packing - can clear entire buffer at once
        Platform_Buffer_Clear(pixels_, width_ * height_, color);
    } else {
        // Row by row
        for (int y = 0; y < height_; y++) {
            Platform_MemSet(pixels_ + y * pitch_, color, width_);
        }
    }
}

// =============================================================================
// Blitting Operations
// =============================================================================

void GraphicsBuffer::Blit_From(const GraphicsBuffer& src,
                               int src_x, int src_y,
                               int dst_x, int dst_y,
                               int width, int height) {
    if (!IsLocked() || !src.IsLocked()) return;

    // Default to full source dimensions
    if (width <= 0) width = src.width_;
    if (height <= 0) height = src.height_;

    // Clip operation
    if (!ClipBlit(src_x, src_y, src.width_, src.height_,
                  dst_x, dst_y, width_, height_,
                  width, height)) {
        return;
    }

    // Use platform blit
    Platform_Buffer_ToBuffer(
        pixels_, pitch_, width_, height_,
        src.pixels_, src.pitch_, src.width_, src.height_,
        dst_x, dst_y, src_x, src_y, width, height);
}

void GraphicsBuffer::Blit_From_Trans(const GraphicsBuffer& src,
                                     int src_x, int src_y,
                                     int dst_x, int dst_y,
                                     int width, int height) {
    if (!IsLocked() || !src.IsLocked()) return;

    // Default to full source dimensions
    if (width <= 0) width = src.width_;
    if (height <= 0) height = src.height_;

    // Clip operation
    if (!ClipBlit(src_x, src_y, src.width_, src.height_,
                  dst_x, dst_y, width_, height_,
                  width, height)) {
        return;
    }

    // Use platform transparent blit
    Platform_Buffer_ToBufferTrans(
        pixels_, pitch_, width_, height_,
        src.pixels_, src.pitch_, src.width_, src.height_,
        dst_x, dst_y, src_x, src_y, width, height);
}

void GraphicsBuffer::Blit_From_Raw(const uint8_t* src_pixels, int src_pitch,
                                   int src_x, int src_y, int src_w, int src_h,
                                   int dst_x, int dst_y,
                                   int width, int height) {
    if (!IsLocked() || !src_pixels) return;

    // Clip operation
    if (!ClipBlit(src_x, src_y, src_w, src_h,
                  dst_x, dst_y, width_, height_,
                  width, height)) {
        return;
    }

    // Use platform blit
    Platform_Buffer_ToBuffer(
        pixels_, pitch_, width_, height_,
        src_pixels, src_pitch, src_w, src_h,
        dst_x, dst_y, src_x, src_y, width, height);
}

void GraphicsBuffer::Blit_From_Raw_Trans(const uint8_t* src_pixels, int src_pitch,
                                         int src_x, int src_y, int src_w, int src_h,
                                         int dst_x, int dst_y,
                                         int width, int height,
                                         uint8_t trans_color) {
    if (!IsLocked() || !src_pixels) return;

    // Clip operation
    if (!ClipBlit(src_x, src_y, src_w, src_h,
                  dst_x, dst_y, width_, height_,
                  width, height)) {
        return;
    }

    // Use platform transparent blit with custom key
    Platform_Buffer_ToBufferTransKey(
        pixels_, pitch_, width_, height_,
        src_pixels, src_pitch, src_w, src_h,
        dst_x, dst_y, src_x, src_y, width, height,
        trans_color);
}

// =============================================================================
// Color Remapping
// =============================================================================

void GraphicsBuffer::Remap(int x, int y, int w, int h, const uint8_t* remap_table) {
    if (!IsLocked() || !remap_table) return;
    if (!ClipRect(x, y, w, h)) return;

    // Use platform remap
    Platform_Buffer_Remap(pixels_, pitch_, x, y, w, h, remap_table);
}

void GraphicsBuffer::Blit_Remap(const GraphicsBuffer& src,
                                int src_x, int src_y,
                                int dst_x, int dst_y,
                                int width, int height,
                                const uint8_t* remap_table,
                                bool transparent) {
    if (!IsLocked() || !src.IsLocked() || !remap_table) return;

    // Clip operation
    if (!ClipBlit(src_x, src_y, src.width_, src.height_,
                  dst_x, dst_y, width_, height_,
                  width, height)) {
        return;
    }

    // Use platform remap copy
    // Note: For transparent + remap, we need to handle this manually
    // since Platform_Buffer_RemapCopy doesn't have transparency option

    if (!transparent) {
        Platform_Buffer_RemapCopy(
            pixels_, pitch_, dst_x, dst_y,
            src.pixels_, src.pitch_, src_x, src_y,
            width, height, remap_table);
    } else {
        // Manual transparent remap - slightly slower but correct
        for (int row = 0; row < height; row++) {
            const uint8_t* src_row = src.pixels_ + (src_y + row) * src.pitch_ + src_x;
            uint8_t* dst_row = pixels_ + (dst_y + row) * pitch_ + dst_x;

            for (int col = 0; col < width; col++) {
                uint8_t pixel = src_row[col];
                if (pixel != TRANSPARENT_COLOR) {
                    dst_row[col] = remap_table[pixel];
                }
            }
        }
    }
}

// =============================================================================
// Display Operations
// =============================================================================

bool GraphicsBuffer::Flip() {
    if (!is_screen_) {
        return false;  // Can only flip screen buffer
    }

    // Make sure we're not locked
    if (lock_count_ > 0) {
        // Force unlock for flip
        lock_count_ = 0;
    }

    return Platform_Graphics_Flip() == 0;
}

// =============================================================================
// Clipping Helpers
// =============================================================================

bool GraphicsBuffer::ClipBlit(
    int& src_x, int& src_y, int src_w, int src_h,
    int& dst_x, int& dst_y, int dst_w, int dst_h,
    int& width, int& height)
{
    // Clip to source bounds
    if (src_x < 0) {
        width += src_x;
        dst_x -= src_x;
        src_x = 0;
    }
    if (src_y < 0) {
        height += src_y;
        dst_y -= src_y;
        src_y = 0;
    }
    if (src_x + width > src_w) {
        width = src_w - src_x;
    }
    if (src_y + height > src_h) {
        height = src_h - src_y;
    }

    // Clip to destination bounds
    if (dst_x < 0) {
        width += dst_x;
        src_x -= dst_x;
        dst_x = 0;
    }
    if (dst_y < 0) {
        height += dst_y;
        src_y -= dst_y;
        dst_y = 0;
    }
    if (dst_x + width > dst_w) {
        width = dst_w - dst_x;
    }
    if (dst_y + height > dst_h) {
        height = dst_h - dst_y;
    }

    // Check if anything remains
    return width > 0 && height > 0;
}

bool GraphicsBuffer::ClipRect(int& x, int& y, int& w, int& h) const {
    // Clip to buffer bounds
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (x + w > width_) {
        w = width_ - x;
    }
    if (y + h > height_) {
        h = height_ - y;
    }

    return w > 0 && h > 0;
}
```

### src/test_graphics_buffer.cpp
```cpp
/**
 * GraphicsBuffer Test Program
 *
 * Comprehensive tests for the GraphicsBuffer wrapper class.
 * Tests both functional correctness and visual output.
 */

#include "game/graphics/graphics_buffer.h"
#include "platform.h"
#include <cstdio>
#include <cstring>

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

bool test_offscreen_buffer_creation() {
    TEST_START("off-screen buffer creation");

    // Create various sizes
    GraphicsBuffer buf1(100, 100);
    ASSERT(buf1.Get_Width() == 100, "Width should be 100");
    ASSERT(buf1.Get_Height() == 100, "Height should be 100");
    ASSERT(!buf1.IsScreenBuffer(), "Should not be screen buffer");

    // Lock should succeed
    ASSERT(buf1.Lock(), "Lock should succeed");
    ASSERT(buf1.IsLocked(), "Should be locked");
    ASSERT(buf1.Get_Buffer() != nullptr, "Buffer should not be null");

    buf1.Unlock();
    ASSERT(!buf1.IsLocked(), "Should be unlocked");

    // Zero-size buffer should have null pixels
    GraphicsBuffer buf_zero(0, 0);
    ASSERT(!buf_zero.Lock(), "Zero-size buffer lock should fail");

    TEST_PASS();
    return true;
}

bool test_pixel_operations() {
    TEST_START("pixel operations");

    GraphicsBuffer buf(64, 64);
    ASSERT(buf.Lock(), "Lock should succeed");

    // Clear and verify
    buf.Clear(0);
    ASSERT(buf.Get_Pixel(0, 0) == 0, "Pixel should be 0 after clear");

    // Put/Get pixel
    buf.Put_Pixel(10, 10, 123);
    ASSERT(buf.Get_Pixel(10, 10) == 123, "Pixel should be 123");

    // Out of bounds should be safe
    buf.Put_Pixel(-1, 10, 99);  // Should not crash
    buf.Put_Pixel(10, -1, 99);
    buf.Put_Pixel(100, 10, 99);  // Beyond buffer
    buf.Put_Pixel(10, 100, 99);
    ASSERT(buf.Get_Pixel(-1, 10) == 0, "Out of bounds should return 0");
    ASSERT(buf.Get_Pixel(100, 10) == 0, "Out of bounds should return 0");

    buf.Unlock();
    TEST_PASS();
    return true;
}

bool test_line_drawing() {
    TEST_START("line drawing");

    GraphicsBuffer buf(64, 64);
    ASSERT(buf.Lock(), "Lock should succeed");
    buf.Clear(0);

    // Horizontal line
    buf.Draw_HLine(10, 20, 30, 5);
    ASSERT(buf.Get_Pixel(10, 20) == 5, "HLine start pixel");
    ASSERT(buf.Get_Pixel(39, 20) == 5, "HLine end pixel");
    ASSERT(buf.Get_Pixel(40, 20) == 0, "HLine should not extend");
    ASSERT(buf.Get_Pixel(10, 19) == 0, "HLine should not affect other rows");

    // Vertical line
    buf.Clear(0);
    buf.Draw_VLine(30, 10, 20, 7);
    ASSERT(buf.Get_Pixel(30, 10) == 7, "VLine start pixel");
    ASSERT(buf.Get_Pixel(30, 29) == 7, "VLine end pixel");
    ASSERT(buf.Get_Pixel(30, 30) == 0, "VLine should not extend");

    // Clipped lines
    buf.Clear(0);
    buf.Draw_HLine(-5, 5, 20, 8);  // Starts off-screen
    ASSERT(buf.Get_Pixel(0, 5) == 8, "Clipped HLine should start at edge");
    ASSERT(buf.Get_Pixel(14, 5) == 8, "Clipped HLine should end correctly");

    buf.Unlock();
    TEST_PASS();
    return true;
}

bool test_rectangle_operations() {
    TEST_START("rectangle operations");

    GraphicsBuffer buf(64, 64);
    ASSERT(buf.Lock(), "Lock should succeed");
    buf.Clear(0);

    // Fill rectangle
    buf.Fill_Rect(10, 10, 20, 20, 42);
    ASSERT(buf.Get_Pixel(10, 10) == 42, "Fill_Rect corner");
    ASSERT(buf.Get_Pixel(29, 29) == 42, "Fill_Rect opposite corner");
    ASSERT(buf.Get_Pixel(9, 10) == 0, "Fill_Rect should not extend left");
    ASSERT(buf.Get_Pixel(30, 10) == 0, "Fill_Rect should not extend right");

    // Draw rectangle (outline)
    buf.Clear(0);
    buf.Draw_Rect(10, 10, 10, 10, 55);
    ASSERT(buf.Get_Pixel(10, 10) == 55, "Draw_Rect corner");
    ASSERT(buf.Get_Pixel(19, 10) == 55, "Draw_Rect top edge");
    ASSERT(buf.Get_Pixel(10, 19) == 55, "Draw_Rect left edge");
    ASSERT(buf.Get_Pixel(15, 15) == 0, "Draw_Rect interior should be empty");

    // Clipped rectangle
    buf.Clear(0);
    buf.Fill_Rect(-5, -5, 20, 20, 99);
    ASSERT(buf.Get_Pixel(0, 0) == 99, "Clipped Fill_Rect at origin");
    ASSERT(buf.Get_Pixel(14, 14) == 99, "Clipped Fill_Rect visible portion");

    buf.Unlock();
    TEST_PASS();
    return true;
}

bool test_blitting() {
    TEST_START("blitting operations");

    // Create source buffer with pattern
    GraphicsBuffer src(32, 32);
    ASSERT(src.Lock(), "Src lock should succeed");
    src.Clear(0);
    src.Fill_Rect(8, 8, 16, 16, 100);  // Center square

    // Create destination buffer
    GraphicsBuffer dst(64, 64);
    ASSERT(dst.Lock(), "Dst lock should succeed");
    dst.Clear(0);

    // Opaque blit
    dst.Blit_From(src, 0, 0, 10, 10, 32, 32);
    ASSERT(dst.Get_Pixel(10, 10) == 0, "Blit corner (from src transparent)");
    ASSERT(dst.Get_Pixel(18, 18) == 100, "Blit center square");

    // Transparent blit
    dst.Clear(50);  // Fill with different color
    dst.Blit_From_Trans(src, 0, 0, 0, 0, 32, 32);
    ASSERT(dst.Get_Pixel(0, 0) == 50, "Trans blit should preserve background");
    ASSERT(dst.Get_Pixel(8, 8) == 100, "Trans blit should copy non-transparent");

    src.Unlock();
    dst.Unlock();
    TEST_PASS();
    return true;
}

bool test_color_remapping() {
    TEST_START("color remapping");

    GraphicsBuffer buf(32, 32);
    ASSERT(buf.Lock(), "Lock should succeed");

    // Create remap table that adds 10 to each color
    uint8_t remap[256];
    for (int i = 0; i < 256; i++) {
        remap[i] = (i + 10) & 0xFF;
    }

    // Fill with known values
    buf.Clear(0);
    buf.Fill_Rect(5, 5, 10, 10, 20);  // Value 20

    // Apply remap to region
    buf.Remap(5, 5, 10, 10, remap);
    ASSERT(buf.Get_Pixel(5, 5) == 30, "Remap should add 10 (20 -> 30)");
    ASSERT(buf.Get_Pixel(0, 0) == 0, "Remap should not affect outside region");

    buf.Unlock();
    TEST_PASS();
    return true;
}

bool test_screen_buffer() {
    TEST_START("screen buffer singleton");

    // Check initialization status
    ASSERT(GraphicsBuffer::IsScreenInitialized(), "Screen should be initialized");

    // Get screen buffer
    GraphicsBuffer& screen = GraphicsBuffer::Screen();
    ASSERT(screen.IsScreenBuffer(), "Should be screen buffer");
    ASSERT(screen.Get_Width() == 640, "Screen width should be 640");
    ASSERT(screen.Get_Height() == 400, "Screen height should be 400");

    // Lock and draw
    ASSERT(screen.Lock(), "Screen lock should succeed");
    screen.Clear(0);

    // Draw a test pattern
    for (int y = 0; y < screen.Get_Height(); y++) {
        for (int x = 0; x < screen.Get_Width(); x++) {
            screen.Put_Pixel(x, y, (x + y) & 0xFF);
        }
    }

    screen.Unlock();

    // Flip should succeed
    ASSERT(screen.Flip(), "Flip should succeed");

    // Get same instance again
    GraphicsBuffer& screen2 = GraphicsBuffer::Screen();
    ASSERT(&screen == &screen2, "Singleton should return same instance");

    TEST_PASS();
    return true;
}

bool test_nested_locks() {
    TEST_START("nested locks");

    GraphicsBuffer buf(32, 32);

    ASSERT(!buf.IsLocked(), "Should start unlocked");

    ASSERT(buf.Lock(), "First lock should succeed");
    ASSERT(buf.IsLocked(), "Should be locked");

    ASSERT(buf.Lock(), "Second lock should succeed");
    ASSERT(buf.IsLocked(), "Should still be locked");

    buf.Unlock();
    ASSERT(buf.IsLocked(), "Should still be locked after one unlock");

    buf.Unlock();
    ASSERT(!buf.IsLocked(), "Should be unlocked after matching unlocks");

    // Extra unlock should be safe
    buf.Unlock();
    ASSERT(!buf.IsLocked(), "Extra unlock should be safe");

    TEST_PASS();
    return true;
}

bool test_move_semantics() {
    TEST_START("move semantics");

    GraphicsBuffer buf1(32, 32);
    ASSERT(buf1.Lock(), "Lock should succeed");
    buf1.Fill_Rect(0, 0, 32, 32, 42);
    buf1.Unlock();

    // Move construct
    GraphicsBuffer buf2(std::move(buf1));
    ASSERT(buf2.Get_Width() == 32, "Moved buffer should have width");
    ASSERT(buf2.Lock(), "Moved buffer should be lockable");
    ASSERT(buf2.Get_Pixel(0, 0) == 42, "Moved buffer should have data");
    buf2.Unlock();

    // Original should be empty
    ASSERT(!buf1.Lock(), "Original should not be lockable after move");

    TEST_PASS();
    return true;
}

// =============================================================================
// Visual Test
// =============================================================================

void run_visual_test() {
    printf("\n=== Visual Test ===\n");
    printf("Drawing test pattern to screen...\n");

    GraphicsBuffer& screen = GraphicsBuffer::Screen();

    // Set up a simple grayscale palette
    PaletteEntry palette[256];
    for (int i = 0; i < 256; i++) {
        palette[i].r = i;
        palette[i].g = i;
        palette[i].b = i;
    }
    Platform_Graphics_SetPalette(palette, 0, 256);

    screen.Lock();
    screen.Clear(0);

    // Draw color bands
    for (int y = 0; y < 100; y++) {
        for (int x = 0; x < 640; x++) {
            screen.Put_Pixel(x, y, (x * 256 / 640) & 0xFF);
        }
    }

    // Draw rectangles
    for (int i = 0; i < 8; i++) {
        screen.Fill_Rect(50 + i * 70, 120, 50, 50, 32 * (i + 1));
    }

    // Draw outlined rectangles
    for (int i = 0; i < 8; i++) {
        screen.Draw_Rect(50 + i * 70, 200, 50, 50, 255);
    }

    // Draw lines
    for (int i = 0; i < 10; i++) {
        screen.Draw_HLine(50, 280 + i * 10, 540, 128 + i * 10);
    }

    screen.Unlock();
    screen.Flip();

    printf("Test pattern displayed. Waiting 2 seconds...\n");
    Platform_Delay(2000);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("==========================================\n");
    printf("GraphicsBuffer Test Suite\n");
    printf("==========================================\n\n");

    // Initialize platform
    if (Platform_Init() != PLATFORM_RESULT_SUCCESS) {
        printf("ERROR: Failed to initialize platform\n");
        return 1;
    }

    // Initialize graphics
    if (Platform_Graphics_Init() != 0) {
        printf("ERROR: Failed to initialize graphics\n");
        Platform_Shutdown();
        return 1;
    }

    printf("=== Unit Tests ===\n\n");

    // Run unit tests
    test_offscreen_buffer_creation();
    test_pixel_operations();
    test_line_drawing();
    test_rectangle_operations();
    test_blitting();
    test_color_remapping();
    test_screen_buffer();
    test_nested_locks();
    test_move_semantics();

    printf("\n------------------------------------------\n");
    printf("Tests: %d/%d passed\n", tests_passed, tests_run);
    printf("------------------------------------------\n");

    // Run visual test if all unit tests passed
    if (tests_passed == tests_run) {
        run_visual_test();
    }

    // Cleanup
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

### ralph-tasks/verify/check-graphics-buffer.sh
```bash
#!/bin/bash
# Verification script for GraphicsBuffer wrapper

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking GraphicsBuffer Implementation ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check files exist
echo "[1/5] Checking file existence..."
FILES=(
    "include/game/graphics/graphics_buffer.h"
    "src/game/graphics/graphics_buffer.cpp"
    "src/test_graphics_buffer.cpp"
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
    include/game/graphics/graphics_buffer.h 2>&1; then
    echo "VERIFY_FAILED: Header has syntax errors"
    exit 1
fi
echo "  OK: Header compiles"

# Step 3: Build test executable
echo "[3/5] Building TestGraphicsBuffer..."
if ! cmake --build build --target TestGraphicsBuffer 2>&1; then
    echo "VERIFY_FAILED: Build failed"
    exit 1
fi
echo "  OK: Build succeeded"

# Step 4: Check executable exists
echo "[4/5] Checking executable..."
if [ ! -f "build/TestGraphicsBuffer" ]; then
    echo "VERIFY_FAILED: TestGraphicsBuffer executable not found"
    exit 1
fi
echo "  OK: Executable exists"

# Step 5: Run tests (headless check - skip visual tests)
echo "[5/5] Running unit tests..."
# Run with timeout and capture output
if timeout 30 ./build/TestGraphicsBuffer 2>&1 | grep -q "All tests PASSED"; then
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

Add to the main CMakeLists.txt:
```cmake
# =============================================================================
# Graphics Integration Library
# =============================================================================

# Create game graphics directory
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/include/game/graphics)
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/src/game/graphics)

# Graphics sources
set(GAME_GRAPHICS_SOURCES
    src/game/graphics/graphics_buffer.cpp
)

# Add to game library or create separate target
target_sources(game_core PRIVATE ${GAME_GRAPHICS_SOURCES})

target_include_directories(game_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include/game/graphics
)

# =============================================================================
# GraphicsBuffer Test
# =============================================================================

add_executable(TestGraphicsBuffer src/test_graphics_buffer.cpp)

target_include_directories(TestGraphicsBuffer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/game/graphics
)

target_link_libraries(TestGraphicsBuffer PRIVATE
    game_core
    redalert_platform
)

add_test(NAME TestGraphicsBuffer COMMAND TestGraphicsBuffer)
```

## Implementation Steps

1. Create directory structure:
   ```bash
   mkdir -p include/game/graphics
   mkdir -p src/game/graphics
   ```

2. Create `include/game/graphics/graphics_buffer.h` with the complete header

3. Create `src/game/graphics/graphics_buffer.cpp` with the implementation

4. Create `src/test_graphics_buffer.cpp` with the test program

5. Update CMakeLists.txt to include new files

6. Create verification script at `ralph-tasks/verify/check-graphics-buffer.sh`

7. Make verification script executable:
   ```bash
   chmod +x ralph-tasks/verify/check-graphics-buffer.sh
   ```

8. Configure and build:
   ```bash
   cmake -B build
   cmake --build build --target TestGraphicsBuffer
   ```

9. Run tests:
   ```bash
   ./build/TestGraphicsBuffer
   ```

10. Run verification:
    ```bash
    ralph-tasks/verify/check-graphics-buffer.sh
    ```

## Success Criteria

- [ ] `GraphicsBuffer` class compiles without warnings
- [ ] Screen buffer singleton accessible via `GraphicsBuffer::Screen()`
- [ ] Off-screen buffer creation works (various sizes)
- [ ] Lock/Unlock mechanics work correctly (including nested locks)
- [ ] Pixel operations work (Put_Pixel, Get_Pixel, Clear)
- [ ] Line drawing works (HLine, VLine, with clipping)
- [ ] Rectangle operations work (Fill_Rect, Draw_Rect, with clipping)
- [ ] Blitting works (opaque and transparent)
- [ ] Color remapping works
- [ ] Move semantics work correctly
- [ ] Visual test displays correct pattern
- [ ] All unit tests pass
- [ ] Verification script passes

## Common Issues

### Platform Not Initialized
**Symptom**: `Screen()` returns buffer with null pixels
**Solution**: Ensure `Platform_Init()` and `Platform_Graphics_Init()` are called first

### Lock/Unlock Mismatch
**Symptom**: Operations fail silently or crash
**Solution**: Always pair Lock/Unlock calls, check IsLocked() before drawing

### Pitch vs Width Confusion
**Symptom**: Graphics appear skewed or corrupted
**Solution**: Always use `Get_Pitch()` for row calculations, not `Get_Width()`

### Clipping Edge Cases
**Symptom**: Crashes or garbage pixels at screen edges
**Solution**: The provided ClipBlit/ClipRect functions handle all edge cases

### Memory Leaks
**Symptom**: Memory usage grows over time
**Solution**: Ensure off-screen buffers are properly destroyed

## Completion Promise
When verification passes, output:
<promise>TASK_15A_COMPLETE</promise>

## Escape Hatch
If stuck after 15 iterations:
- Document what's blocking in `ralph-tasks/blocked/15a.md`
- List attempted approaches
- Output:
<promise>TASK_15A_BLOCKED</promise>

## Max Iterations
15
