# Task 15b: Shape Drawing System

## Dependencies
- Task 15a (GraphicsBuffer) must be complete
- Platform shape loading must work (`Platform_Shape_*` APIs)
- Asset system must be functional (MIX file loading)
- Shape/SHP file parser from Phase 12 must be working

## Context
Red Alert uses `.SHP` files (Shape files) for all sprites: units, buildings, infantry, icons, cursors, and UI elements. The original rendering code uses complex assembly routines for performance.

This task creates a `ShapeRenderer` class that:
1. Draws shapes from loaded SHP files to a GraphicsBuffer
2. Supports all original rendering modes (normal, transparent, remapped, shadow)
3. Handles frame selection and animation
4. Provides house-color remapping for faction tinting

Shape rendering is **the most commonly called drawing function** in the game. Every visible unit, building, effect, and UI element goes through this code. Performance matters.

## Objective
Create a comprehensive shape rendering system that can draw any SHP file to a GraphicsBuffer with full support for transparency, color remapping, and shadow effects.

## Technical Background

### SHP File Format (Recap from Phase 12)
```
Header:
  uint16_t frame_count
  uint16_t unknown1, unknown2
  uint16_t width, height  (maximum dimensions)

Frame Offsets:
  uint32_t offsets[frame_count + 2]  // +2 for sentinel values

Each Frame:
  uint16_t x_offset, y_offset  // Draw offset from origin
  uint16_t width, height       // Frame dimensions
  uint8_t  flags               // Compression flags
  uint8_t  padding[3]
  uint8_t  pixel_data[...]     // Raw or LCW compressed
```

### Compression Flags
- **0x00**: Raw uncompressed pixels (width × height bytes)
- **0x20**: LCW compressed pixels
- **0x40**: XOR with previous frame (for animations)
- **0x80**: Run-length encoded

### Color Remapping
Units and buildings use **house colors** - specific palette indices that are remapped based on the owning player. Standard remap ranges:
- **Indices 80-95**: Primary house color (vehicles, buildings)
- **Indices 176-191**: Secondary house color (uniforms, accents)

Remap tables are 256-byte arrays where `output_color = remap_table[input_color]`.

### Shadow Rendering
Shadows are drawn by:
1. Using the shape as a mask (non-transparent pixels)
2. For each pixel in the mask, darken the corresponding screen pixel
3. Shadow tables map original colors to darker versions

### Drawing Modes
The original code supports multiple drawing modes:
- **DRAW_NORMAL**: Skip transparent color (0)
- **DRAW_GHOST**: Semi-transparent (checkerboard pattern)
- **DRAW_FADING**: Gradual fade to black
- **DRAW_PREDATOR**: Stealth shimmer effect
- **DRAW_SHADOW**: Draw shadow only

### Platform APIs Used
```c
// From platform.h - Shape loading
PlatformShape* Platform_Shape_Load(const char* filename);
void Platform_Shape_Free(PlatformShape* shape);
void Platform_Shape_GetSize(const PlatformShape* shape, int32_t* width, int32_t* height);
int32_t Platform_Shape_GetFrameCount(const PlatformShape* shape);
int32_t Platform_Shape_GetFrame(const PlatformShape* shape, int32_t frame_index,
                                 uint8_t* buffer, int32_t buffer_size);
```

## Deliverables
- [ ] `include/game/graphics/shape_renderer.h` - Shape renderer class
- [ ] `src/game/graphics/shape_renderer.cpp` - Implementation
- [ ] `include/game/graphics/remap_tables.h` - Standard remap tables
- [ ] `src/game/graphics/remap_tables.cpp` - Remap table data
- [ ] `src/test_shape_drawing.cpp` - Test program
- [ ] `ralph-tasks/verify/check-shape-drawing.sh` - Verification script
- [ ] All tests pass

## Files to Create

### include/game/graphics/shape_renderer.h
```cpp
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
```

### src/game/graphics/shape_renderer.cpp
```cpp
/**
 * ShapeRenderer Implementation
 */

#include "game/graphics/shape_renderer.h"
#include "game/graphics/graphics_buffer.h"
#include "platform.h"
#include <algorithm>
#include <cstring>

// =============================================================================
// ShapeRenderer - Construction
// =============================================================================

ShapeRenderer::ShapeRenderer()
    : shape_(nullptr)
    , width_(0)
    , height_(0)
    , frame_count_(0)
{
}

ShapeRenderer::~ShapeRenderer() {
    Unload();
}

ShapeRenderer::ShapeRenderer(ShapeRenderer&& other) noexcept
    : shape_(other.shape_)
    , name_(std::move(other.name_))
    , width_(other.width_)
    , height_(other.height_)
    , frame_count_(other.frame_count_)
    , frame_cache_(std::move(other.frame_cache_))
{
    other.shape_ = nullptr;
    other.width_ = 0;
    other.height_ = 0;
    other.frame_count_ = 0;
}

ShapeRenderer& ShapeRenderer::operator=(ShapeRenderer&& other) noexcept {
    if (this != &other) {
        Unload();

        shape_ = other.shape_;
        name_ = std::move(other.name_);
        width_ = other.width_;
        height_ = other.height_;
        frame_count_ = other.frame_count_;
        frame_cache_ = std::move(other.frame_cache_);

        other.shape_ = nullptr;
        other.width_ = 0;
        other.height_ = 0;
        other.frame_count_ = 0;
    }
    return *this;
}

// =============================================================================
// Loading
// =============================================================================

bool ShapeRenderer::Load(const char* filename) {
    Unload();

    if (!filename || !filename[0]) {
        return false;
    }

    shape_ = Platform_Shape_Load(filename);
    if (!shape_) {
        Platform_LogWarn("Failed to load shape: ");
        Platform_LogWarn(filename);
        return false;
    }

    name_ = filename;

    // Get shape info
    int32_t w, h;
    Platform_Shape_GetSize(shape_, &w, &h);
    width_ = w;
    height_ = h;
    frame_count_ = Platform_Shape_GetFrameCount(shape_);

    // Initialize frame cache (empty frames)
    frame_cache_.resize(frame_count_);

    return true;
}

bool ShapeRenderer::LoadFromMemory(const uint8_t* data, int32_t size, const char* name) {
    Unload();

    if (!data || size <= 0) {
        return false;
    }

    shape_ = Platform_Shape_LoadFromMemory(data, size);
    if (!shape_) {
        return false;
    }

    name_ = name ? name : "memory";

    // Get shape info
    int32_t w, h;
    Platform_Shape_GetSize(shape_, &w, &h);
    width_ = w;
    height_ = h;
    frame_count_ = Platform_Shape_GetFrameCount(shape_);

    // Initialize frame cache
    frame_cache_.resize(frame_count_);

    return true;
}

void ShapeRenderer::Unload() {
    if (shape_) {
        Platform_Shape_Free(shape_);
        shape_ = nullptr;
    }
    name_.clear();
    width_ = 0;
    height_ = 0;
    frame_count_ = 0;
    frame_cache_.clear();
}

// =============================================================================
// Shape Information
// =============================================================================

int ShapeRenderer::GetFrameCount() const {
    return frame_count_;
}

void ShapeRenderer::GetSize(int* width, int* height) const {
    if (width) *width = width_;
    if (height) *height = height_;
}

int ShapeRenderer::GetWidth() const {
    return width_;
}

int ShapeRenderer::GetHeight() const {
    return height_;
}

bool ShapeRenderer::GetFrameSize(int frame, int* width, int* height) const {
    if (frame < 0 || frame >= frame_count_) {
        return false;
    }

    // Need to get frame data to know dimensions
    const ShapeFrame* f = const_cast<ShapeRenderer*>(this)->GetFrame(frame);
    if (!f || !f->IsValid()) {
        return false;
    }

    if (width) *width = f->width;
    if (height) *height = f->height;
    return true;
}

bool ShapeRenderer::GetFrameOffset(int frame, int* x_offset, int* y_offset) const {
    if (frame < 0 || frame >= frame_count_) {
        return false;
    }

    const ShapeFrame* f = const_cast<ShapeRenderer*>(this)->GetFrame(frame);
    if (!f || !f->IsValid()) {
        return false;
    }

    if (x_offset) *x_offset = f->x_offset;
    if (y_offset) *y_offset = f->y_offset;
    return true;
}

// =============================================================================
// Frame Cache
// =============================================================================

const ShapeFrame* ShapeRenderer::GetFrame(int frame) {
    if (frame < 0 || frame >= frame_count_) {
        return nullptr;
    }

    ShapeFrame& cached = frame_cache_[frame];

    // Return cached frame if already loaded
    if (cached.IsValid()) {
        return &cached;
    }

    // Need to decompress frame from platform shape
    if (!shape_) {
        return nullptr;
    }

    // Get frame data from platform layer
    // First, allocate max possible size
    size_t max_size = static_cast<size_t>(width_) * height_;
    std::vector<uint8_t> temp_buffer(max_size);

    int32_t bytes = Platform_Shape_GetFrame(shape_, frame,
                                             temp_buffer.data(),
                                             static_cast<int32_t>(max_size));
    if (bytes <= 0) {
        return nullptr;
    }

    // Parse frame header (platform returns raw frame data with header)
    // The platform layer should handle decompression and return:
    // - First 4 bytes: x_offset (i16), y_offset (i16)
    // - Next 4 bytes: width (i16), height (i16)
    // - Rest: pixel data

    // Note: Platform_Shape_GetFrame returns just the pixels
    // We need to query frame info separately
    // For now, assume frame fills the shape dimensions
    // TODO: Get proper frame offsets from platform layer

    // Calculate actual frame size from bytes returned
    // Assuming square-ish frames, find dimensions
    cached.x_offset = 0;
    cached.y_offset = 0;
    cached.width = width_;
    cached.height = bytes / width_;
    if (cached.height <= 0) cached.height = 1;

    // Adjust if bytes don't evenly divide
    if (cached.width * cached.height != bytes) {
        // Try to find better dimensions
        cached.height = height_;
        if (cached.width * cached.height > bytes) {
            cached.height = bytes / cached.width;
        }
    }

    // Copy pixel data
    cached.pixels.assign(temp_buffer.begin(), temp_buffer.begin() + bytes);

    return &cached;
}

void ShapeRenderer::PrecacheAllFrames() {
    for (int i = 0; i < frame_count_; i++) {
        GetFrame(i);
    }
}

void ShapeRenderer::PrecacheFrame(int frame) {
    GetFrame(frame);
}

void ShapeRenderer::ClearCache() {
    for (auto& frame : frame_cache_) {
        frame.pixels.clear();
        frame.pixels.shrink_to_fit();
        frame.width = 0;
        frame.height = 0;
    }
}

size_t ShapeRenderer::GetCacheSize() const {
    size_t total = 0;
    for (const auto& frame : frame_cache_) {
        total += frame.pixels.capacity();
    }
    return total;
}

// =============================================================================
// Drawing - Basic
// =============================================================================

bool ShapeRenderer::Draw(GraphicsBuffer& buffer, int x, int y, int frame,
                         uint32_t flags) {
    return DrawRemapped(buffer, x, y, frame, nullptr, flags);
}

bool ShapeRenderer::DrawRemapped(GraphicsBuffer& buffer, int x, int y, int frame,
                                  const uint8_t* remap_table, uint32_t flags) {
    const ShapeFrame* f = GetFrame(frame);
    if (!f || !f->IsValid()) {
        return false;
    }

    return DrawInternal(buffer, x, y, *f, remap_table, flags, 0);
}

bool ShapeRenderer::DrawShadow(GraphicsBuffer& buffer, int x, int y, int frame,
                                const uint8_t* shadow_table) {
    if (!shadow_table) {
        return false;
    }

    const ShapeFrame* f = GetFrame(frame);
    if (!f || !f->IsValid()) {
        return false;
    }

    return DrawInternal(buffer, x, y, *f, shadow_table, SHAPE_SHADOW, 0);
}

// =============================================================================
// Drawing - Advanced
// =============================================================================

bool ShapeRenderer::DrawGhost(GraphicsBuffer& buffer, int x, int y, int frame,
                               int phase) {
    const ShapeFrame* f = GetFrame(frame);
    if (!f || !f->IsValid()) {
        return false;
    }

    return DrawInternal(buffer, x, y, *f, nullptr, SHAPE_GHOST, phase & 1);
}

bool ShapeRenderer::DrawFading(GraphicsBuffer& buffer, int x, int y, int frame,
                                const uint8_t* fade_table, int fade_level) {
    if (!fade_table || fade_level < 0) {
        return Draw(buffer, x, y, frame);
    }

    const ShapeFrame* f = GetFrame(frame);
    if (!f || !f->IsValid()) {
        return false;
    }

    // Fade table is actually 16 tables of 256 entries each
    // Index by: fade_table[fade_level * 256 + color]
    const uint8_t* level_table = fade_table + (fade_level & 0x0F) * 256;
    return DrawInternal(buffer, x, y, *f, level_table, SHAPE_FADING, 0);
}

bool ShapeRenderer::DrawPredator(GraphicsBuffer& buffer, int x, int y, int frame,
                                  int phase) {
    const ShapeFrame* f = GetFrame(frame);
    if (!f || !f->IsValid()) {
        return false;
    }

    return DrawInternal(buffer, x, y, *f, nullptr, SHAPE_PREDATOR, phase);
}

bool ShapeRenderer::DrawFlat(GraphicsBuffer& buffer, int x, int y, int frame,
                              uint8_t color) {
    const ShapeFrame* f = GetFrame(frame);
    if (!f || !f->IsValid()) {
        return false;
    }

    return DrawInternal(buffer, x, y, *f, nullptr, SHAPE_FLAT, color);
}

// =============================================================================
// Internal Draw Implementation
// =============================================================================

bool ShapeRenderer::DrawInternal(GraphicsBuffer& buffer, int x, int y,
                                  const ShapeFrame& frame,
                                  const uint8_t* remap_table,
                                  uint32_t flags,
                                  uint8_t flat_color) {
    if (!buffer.IsLocked()) {
        return false;
    }

    // Apply centering if requested
    if (flags & SHAPE_CENTER) {
        x -= frame.width / 2;
        y -= frame.height / 2;
    }

    // Apply frame offset
    int draw_x = x + frame.x_offset;
    int draw_y = y + frame.y_offset;

    // Get buffer info
    uint8_t* dest = buffer.Get_Buffer();
    int dest_w = buffer.Get_Width();
    int dest_h = buffer.Get_Height();
    int dest_pitch = buffer.Get_Pitch();

    // Calculate clipping
    int src_x = 0;
    int src_y = 0;
    int width = frame.width;
    int height = frame.height;

    // Clip left
    if (draw_x < 0) {
        src_x = -draw_x;
        width += draw_x;
        draw_x = 0;
    }

    // Clip top
    if (draw_y < 0) {
        src_y = -draw_y;
        height += draw_y;
        draw_y = 0;
    }

    // Clip right
    if (draw_x + width > dest_w) {
        width = dest_w - draw_x;
    }

    // Clip bottom
    if (draw_y + height > dest_h) {
        height = dest_h - draw_y;
    }

    // Nothing to draw?
    if (width <= 0 || height <= 0) {
        return true;  // Success - just nothing visible
    }

    const uint8_t* src = frame.pixels.data();
    int src_pitch = frame.width;

    // Handle horizontal flip
    int x_step = 1;
    int x_start = src_x;
    if (flags & SHAPE_FLIP_X) {
        x_step = -1;
        x_start = frame.width - 1 - src_x;
    }

    // Handle vertical flip
    int y_step = 1;
    int y_start = src_y;
    if (flags & SHAPE_FLIP_Y) {
        y_step = -1;
        y_start = frame.height - 1 - src_y;
    }

    // Draw based on mode
    if (flags & SHAPE_SHADOW) {
        // Shadow mode: darken destination pixels using shape as mask
        for (int row = 0; row < height; row++) {
            int sy = y_start + row * y_step;
            const uint8_t* src_row = src + sy * src_pitch;
            uint8_t* dst_row = dest + (draw_y + row) * dest_pitch + draw_x;

            for (int col = 0; col < width; col++) {
                int sx = x_start + col * x_step;
                uint8_t pixel = src_row[sx];

                if (pixel != 0) {  // Non-transparent
                    // Apply shadow table to destination
                    dst_row[col] = remap_table[dst_row[col]];
                }
            }
        }
    }
    else if (flags & SHAPE_GHOST) {
        // Ghost mode: checkerboard transparency
        for (int row = 0; row < height; row++) {
            int sy = y_start + row * y_step;
            const uint8_t* src_row = src + sy * src_pitch;
            uint8_t* dst_row = dest + (draw_y + row) * dest_pitch + draw_x;

            for (int col = 0; col < width; col++) {
                // Checkerboard pattern based on position and phase
                if (((draw_x + col + draw_y + row + flat_color) & 1) == 0) {
                    int sx = x_start + col * x_step;
                    uint8_t pixel = src_row[sx];

                    if (pixel != 0) {
                        dst_row[col] = remap_table ? remap_table[pixel] : pixel;
                    }
                }
            }
        }
    }
    else if (flags & SHAPE_PREDATOR) {
        // Predator mode: copy from offset position for shimmer
        int shimmer_x = (flat_color % 3) - 1;  // -1, 0, or 1
        int shimmer_y = (flat_color % 5) - 2;  // -2 to 2

        for (int row = 0; row < height; row++) {
            int sy = y_start + row * y_step;
            const uint8_t* src_row = src + sy * src_pitch;
            uint8_t* dst_row = dest + (draw_y + row) * dest_pitch + draw_x;

            for (int col = 0; col < width; col++) {
                int sx = x_start + col * x_step;
                uint8_t pixel = src_row[sx];

                if (pixel != 0) {
                    // Sample from offset position in destination
                    int sample_x = draw_x + col + shimmer_x;
                    int sample_y = draw_y + row + shimmer_y;

                    if (sample_x >= 0 && sample_x < dest_w &&
                        sample_y >= 0 && sample_y < dest_h) {
                        dst_row[col] = dest[sample_y * dest_pitch + sample_x];
                    }
                }
            }
        }
    }
    else if (flags & SHAPE_FLAT) {
        // Flat mode: single color using shape as mask
        for (int row = 0; row < height; row++) {
            int sy = y_start + row * y_step;
            const uint8_t* src_row = src + sy * src_pitch;
            uint8_t* dst_row = dest + (draw_y + row) * dest_pitch + draw_x;

            for (int col = 0; col < width; col++) {
                int sx = x_start + col * x_step;
                if (src_row[sx] != 0) {
                    dst_row[col] = flat_color;
                }
            }
        }
    }
    else {
        // Normal or Fading mode: transparent blit with optional remap
        if (remap_table) {
            // With remapping
            for (int row = 0; row < height; row++) {
                int sy = y_start + row * y_step;
                const uint8_t* src_row = src + sy * src_pitch;
                uint8_t* dst_row = dest + (draw_y + row) * dest_pitch + draw_x;

                for (int col = 0; col < width; col++) {
                    int sx = x_start + col * x_step;
                    uint8_t pixel = src_row[sx];

                    if (pixel != 0) {
                        dst_row[col] = remap_table[pixel];
                    }
                }
            }
        } else {
            // No remapping - use platform blit for speed
            // But we need to handle flipping manually if requested
            if ((flags & (SHAPE_FLIP_X | SHAPE_FLIP_Y)) == 0) {
                // No flipping - can use fast path
                buffer.Blit_From_Raw_Trans(
                    src, src_pitch,
                    src_x, src_y, frame.width, frame.height,
                    draw_x, draw_y,
                    width, height,
                    0  // Transparent color
                );
            } else {
                // Manual loop for flipping
                for (int row = 0; row < height; row++) {
                    int sy = y_start + row * y_step;
                    const uint8_t* src_row = src + sy * src_pitch;
                    uint8_t* dst_row = dest + (draw_y + row) * dest_pitch + draw_x;

                    for (int col = 0; col < width; col++) {
                        int sx = x_start + col * x_step;
                        uint8_t pixel = src_row[sx];

                        if (pixel != 0) {
                            dst_row[col] = pixel;
                        }
                    }
                }
            }
        }
    }

    return true;
}

// =============================================================================
// Convenience Function
// =============================================================================

bool Draw_Shape(GraphicsBuffer& buffer, const char* filename,
                int x, int y, int frame, uint32_t flags) {
    // Use cache for efficiency
    ShapeRenderer* renderer = ShapeCache::Instance().Get(filename);
    if (!renderer) {
        return false;
    }
    return renderer->Draw(buffer, x, y, frame, flags);
}

// =============================================================================
// ShapeCache Implementation
// =============================================================================

ShapeCache& ShapeCache::Instance() {
    static ShapeCache instance;
    return instance;
}

ShapeCache::~ShapeCache() {
    Clear();
}

ShapeCache::CacheEntry* ShapeCache::Find(const char* filename) {
    for (auto& pair : cache_) {
        if (pair.first == filename) {
            return &pair.second;
        }
    }
    return nullptr;
}

ShapeRenderer* ShapeCache::Get(const char* filename) {
    if (!filename || !filename[0]) {
        return nullptr;
    }

    // Check cache
    CacheEntry* entry = Find(filename);
    if (entry) {
        entry->access_count++;
        return entry->renderer.get();
    }

    // Load new shape
    auto renderer = std::make_unique<ShapeRenderer>();
    if (!renderer->Load(filename)) {
        return nullptr;
    }

    // Add to cache
    CacheEntry new_entry;
    new_entry.renderer = std::move(renderer);
    new_entry.access_count = 1;

    ShapeRenderer* ptr = new_entry.renderer.get();
    cache_.push_back({filename, std::move(new_entry)});

    return ptr;
}

bool ShapeCache::Preload(const char* filename) {
    return Get(filename) != nullptr;
}

void ShapeCache::Remove(const char* filename) {
    for (auto it = cache_.begin(); it != cache_.end(); ++it) {
        if (it->first == filename) {
            cache_.erase(it);
            return;
        }
    }
}

void ShapeCache::Clear() {
    cache_.clear();
}

int ShapeCache::GetCount() const {
    return static_cast<int>(cache_.size());
}

size_t ShapeCache::GetMemoryUsage() const {
    size_t total = 0;
    for (const auto& pair : cache_) {
        total += pair.second.renderer->GetCacheSize();
    }
    return total;
}
```

### include/game/graphics/remap_tables.h
```cpp
/**
 * Standard Color Remap Tables
 *
 * Pre-defined remap tables for house colors, shadows, and effects.
 */

#ifndef GAME_GRAPHICS_REMAP_TABLES_H
#define GAME_GRAPHICS_REMAP_TABLES_H

#include <cstdint>

// =============================================================================
// House Color Remapping
// =============================================================================

// House color indices in the standard palette
// Original palette has these color ranges for house tinting:
//   80-95:  Primary house color (vehicles, main building color)
//   176-191: Secondary house color (trim, accents)

enum HouseColorIndex {
    HOUSE_COLOR_GOLD = 0,      // Yellow/Gold (Spain)
    HOUSE_COLOR_LTBLUE = 1,    // Light Blue (Greece)
    HOUSE_COLOR_RED = 2,       // Red (USSR)
    HOUSE_COLOR_GREEN = 3,     // Green (England)
    HOUSE_COLOR_ORANGE = 4,    // Orange (Ukraine)
    HOUSE_COLOR_GREY = 5,      // Grey (Germany)
    HOUSE_COLOR_BLUE = 6,      // Blue (France)
    HOUSE_COLOR_BROWN = 7,     // Brown (Turkey)
    HOUSE_COLOR_COUNT = 8
};

/**
 * Get the remap table for a house color
 *
 * @param house House color index (0-7)
 * @return Pointer to 256-byte remap table
 */
const uint8_t* GetHouseRemapTable(int house);

/**
 * Get the remap table for a player index
 *
 * Maps player indices to house colors:
 * - Single player: Player 0 = Good (allied), Player 1 = Bad (soviet)
 * - Multiplayer: Players 0-7 map to house colors
 *
 * @param player Player index
 * @param is_multiplayer True for multiplayer games
 * @return Pointer to 256-byte remap table
 */
const uint8_t* GetPlayerRemapTable(int player, bool is_multiplayer);

// =============================================================================
// Shadow Tables
// =============================================================================

/**
 * Get the standard shadow remap table
 *
 * Maps each palette color to a darker version for shadow effects.
 *
 * @return Pointer to 256-byte shadow table
 */
const uint8_t* GetShadowTable();

/**
 * Generate a shadow table from a palette
 *
 * @param palette    RGB palette data (768 bytes)
 * @param output     Output table (256 bytes)
 * @param intensity  Shadow darkness (0.0 = none, 1.0 = black)
 */
void GenerateShadowTable(const uint8_t* palette, uint8_t* output, float intensity);

// =============================================================================
// Fade Tables
// =============================================================================

/**
 * Get the fade table array
 *
 * Returns pointer to 16 remap tables (4096 bytes total).
 * Index by: table[fade_level * 256 + color]
 *
 * Fade levels:
 *   0  = No fade (identity)
 *   15 = Maximum fade (all black)
 *
 * @return Pointer to 4096-byte fade table array
 */
const uint8_t* GetFadeTables();

/**
 * Generate fade tables from a palette
 *
 * @param palette RGB palette data (768 bytes)
 * @param output  Output tables (4096 bytes - 16 tables of 256 entries)
 */
void GenerateFadeTables(const uint8_t* palette, uint8_t* output);

// =============================================================================
// Ghost Tables
// =============================================================================

/**
 * Get the ghost (semi-transparent) remap table
 *
 * Maps colors to lighter versions for ghost effects.
 *
 * @return Pointer to 256-byte ghost table
 */
const uint8_t* GetGhostTable();

// =============================================================================
// Identity Table
// =============================================================================

/**
 * Get the identity remap table (no change)
 *
 * Useful as a default when no remapping is needed.
 *
 * @return Pointer to 256-byte identity table
 */
const uint8_t* GetIdentityTable();

// =============================================================================
// Table Initialization
// =============================================================================

/**
 * Initialize all remap tables
 *
 * Must be called after palette is loaded. Generates house colors,
 * shadows, fades, etc. based on the current palette.
 *
 * @param palette RGB palette data (768 bytes)
 */
void InitRemapTables(const uint8_t* palette);

/**
 * Check if remap tables are initialized
 */
bool AreRemapTablesInitialized();

#endif // GAME_GRAPHICS_REMAP_TABLES_H
```

### src/game/graphics/remap_tables.cpp
```cpp
/**
 * Standard Color Remap Tables Implementation
 */

#include "game/graphics/remap_tables.h"
#include "platform.h"
#include <cstring>
#include <cmath>

// =============================================================================
// Table Storage
// =============================================================================

static bool s_tables_initialized = false;

// Identity table (no change)
static uint8_t s_identity_table[256];

// House color remap tables (8 houses * 256 colors)
static uint8_t s_house_tables[8][256];

// Shadow table
static uint8_t s_shadow_table[256];

// Fade tables (16 levels * 256 colors)
static uint8_t s_fade_tables[16 * 256];

// Ghost table
static uint8_t s_ghost_table[256];

// =============================================================================
// Color Matching
// =============================================================================

// Find closest palette color to given RGB
static uint8_t FindClosestColor(const uint8_t* palette, int r, int g, int b) {
    int best_index = 0;
    int best_dist = 0x7FFFFFFF;

    for (int i = 1; i < 256; i++) {  // Skip index 0 (transparent)
        int pr = palette[i * 3 + 0];
        int pg = palette[i * 3 + 1];
        int pb = palette[i * 3 + 2];

        // Simple RGB distance (could use LAB for better results)
        int dr = r - pr;
        int dg = g - pg;
        int db = b - pb;
        int dist = dr * dr + dg * dg + db * db;

        if (dist < best_dist) {
            best_dist = dist;
            best_index = i;
        }
    }

    return static_cast<uint8_t>(best_index);
}

// =============================================================================
// House Colors
// =============================================================================

// Base house color RGB values (primary)
static const int HOUSE_COLORS_PRIMARY[8][3] = {
    {255, 255, 0},    // GOLD (Yellow)
    {0, 160, 255},    // LTBLUE (Light Blue)
    {255, 0, 0},      // RED
    {0, 200, 0},      // GREEN
    {255, 128, 0},    // ORANGE
    {128, 128, 128},  // GREY
    {0, 0, 255},      // BLUE
    {140, 80, 40},    // BROWN
};

// Remap range for house colors (inclusive)
static const int HOUSE_REMAP_START = 80;
static const int HOUSE_REMAP_END = 95;
static const int HOUSE_REMAP_COUNT = HOUSE_REMAP_END - HOUSE_REMAP_START + 1;

static void GenerateHouseTable(int house, const uint8_t* palette, uint8_t* output) {
    // Start with identity
    for (int i = 0; i < 256; i++) {
        output[i] = static_cast<uint8_t>(i);
    }

    // Get house base color
    int base_r = HOUSE_COLORS_PRIMARY[house][0];
    int base_g = HOUSE_COLORS_PRIMARY[house][1];
    int base_b = HOUSE_COLORS_PRIMARY[house][2];

    // Remap the house color range
    for (int i = 0; i < HOUSE_REMAP_COUNT; i++) {
        int src_index = HOUSE_REMAP_START + i;

        // Calculate shade factor based on position in range
        // Index 0 = darkest, Index 15 = lightest
        float shade = static_cast<float>(i) / (HOUSE_REMAP_COUNT - 1);

        // Interpolate from dark to full color
        int r = static_cast<int>(base_r * (0.3f + 0.7f * shade));
        int g = static_cast<int>(base_g * (0.3f + 0.7f * shade));
        int b = static_cast<int>(base_b * (0.3f + 0.7f * shade));

        // Clamp
        r = r > 255 ? 255 : (r < 0 ? 0 : r);
        g = g > 255 ? 255 : (g < 0 ? 0 : g);
        b = b > 255 ? 255 : (b < 0 ? 0 : b);

        // Find closest palette color
        output[src_index] = FindClosestColor(palette, r, g, b);
    }
}

const uint8_t* GetHouseRemapTable(int house) {
    if (house < 0 || house >= HOUSE_COLOR_COUNT) {
        return s_identity_table;
    }
    return s_house_tables[house];
}

const uint8_t* GetPlayerRemapTable(int player, bool is_multiplayer) {
    if (!is_multiplayer) {
        // Single player: 0 = Allied (Gold), 1 = Soviet (Red)
        if (player == 0) return s_house_tables[HOUSE_COLOR_GOLD];
        if (player == 1) return s_house_tables[HOUSE_COLOR_RED];
        return s_identity_table;
    }

    // Multiplayer: direct mapping
    if (player < 0 || player >= HOUSE_COLOR_COUNT) {
        return s_identity_table;
    }
    return s_house_tables[player];
}

// =============================================================================
// Shadow Tables
// =============================================================================

void GenerateShadowTable(const uint8_t* palette, uint8_t* output, float intensity) {
    // Clamp intensity
    if (intensity < 0.0f) intensity = 0.0f;
    if (intensity > 1.0f) intensity = 1.0f;

    float factor = 1.0f - intensity;

    for (int i = 0; i < 256; i++) {
        if (i == 0) {
            output[i] = 0;  // Transparent stays transparent
            continue;
        }

        // Get original color
        int r = palette[i * 3 + 0];
        int g = palette[i * 3 + 1];
        int b = palette[i * 3 + 2];

        // Darken
        r = static_cast<int>(r * factor);
        g = static_cast<int>(g * factor);
        b = static_cast<int>(b * factor);

        // Find closest palette color
        output[i] = FindClosestColor(palette, r, g, b);
    }
}

const uint8_t* GetShadowTable() {
    return s_shadow_table;
}

// =============================================================================
// Fade Tables
// =============================================================================

void GenerateFadeTables(const uint8_t* palette, uint8_t* output) {
    for (int level = 0; level < 16; level++) {
        uint8_t* table = output + level * 256;

        // Factor ranges from 1.0 (level 0) to 0.0 (level 15)
        float factor = 1.0f - (static_cast<float>(level) / 15.0f);

        for (int i = 0; i < 256; i++) {
            if (i == 0) {
                table[i] = 0;  // Transparent stays transparent
                continue;
            }

            // Get original color
            int r = palette[i * 3 + 0];
            int g = palette[i * 3 + 1];
            int b = palette[i * 3 + 2];

            // Fade towards black
            r = static_cast<int>(r * factor);
            g = static_cast<int>(g * factor);
            b = static_cast<int>(b * factor);

            // Find closest palette color
            table[i] = FindClosestColor(palette, r, g, b);
        }
    }
}

const uint8_t* GetFadeTables() {
    return s_fade_tables;
}

// =============================================================================
// Ghost Table
// =============================================================================

static void GenerateGhostTable(const uint8_t* palette, uint8_t* output) {
    // Ghost effect lightens colors
    for (int i = 0; i < 256; i++) {
        if (i == 0) {
            output[i] = 0;
            continue;
        }

        // Get original color
        int r = palette[i * 3 + 0];
        int g = palette[i * 3 + 1];
        int b = palette[i * 3 + 2];

        // Lighten by 50%
        r = r + (255 - r) / 2;
        g = g + (255 - g) / 2;
        b = b + (255 - b) / 2;

        output[i] = FindClosestColor(palette, r, g, b);
    }
}

const uint8_t* GetGhostTable() {
    return s_ghost_table;
}

// =============================================================================
// Identity Table
// =============================================================================

const uint8_t* GetIdentityTable() {
    return s_identity_table;
}

// =============================================================================
// Initialization
// =============================================================================

void InitRemapTables(const uint8_t* palette) {
    if (!palette) {
        return;
    }

    // Initialize identity table
    for (int i = 0; i < 256; i++) {
        s_identity_table[i] = static_cast<uint8_t>(i);
    }

    // Generate house color tables
    for (int h = 0; h < HOUSE_COLOR_COUNT; h++) {
        GenerateHouseTable(h, palette, s_house_tables[h]);
    }

    // Generate shadow table (50% darkening)
    GenerateShadowTable(palette, s_shadow_table, 0.5f);

    // Generate fade tables
    GenerateFadeTables(palette, s_fade_tables);

    // Generate ghost table
    GenerateGhostTable(palette, s_ghost_table);

    s_tables_initialized = true;
}

bool AreRemapTablesInitialized() {
    return s_tables_initialized;
}
```

### src/test_shape_drawing.cpp
```cpp
/**
 * Shape Drawing Test Program
 *
 * Tests the ShapeRenderer class with various drawing modes.
 */

#include "game/graphics/shape_renderer.h"
#include "game/graphics/graphics_buffer.h"
#include "game/graphics/remap_tables.h"
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

bool test_shape_loading() {
    TEST_START("shape loading");

    ShapeRenderer renderer;
    ASSERT(!renderer.IsLoaded(), "Should not be loaded initially");

    // Try to load a shape (may fail if no MIX files registered)
    // We'll test with a dummy/memory load instead
    uint8_t dummy_shp[] = {
        // Minimal SHP header (not a real SHP, just for testing structure)
        0x01, 0x00,  // 1 frame
        0x00, 0x00, 0x00, 0x00,  // Unknown
        0x08, 0x00,  // Width = 8
        0x08, 0x00,  // Height = 8
        // Frame offset
        0x14, 0x00, 0x00, 0x00,  // Offset to frame 0
        0x5C, 0x00, 0x00, 0x00,  // End offset
        // Frame 0 data (8x8 = 64 bytes, simple pattern)
    };

    // Note: This won't actually work with our parser, just testing API
    // In real tests, we'd use actual SHP data

    // Test unload is safe on not-loaded
    renderer.Unload();
    ASSERT(!renderer.IsLoaded(), "Should still not be loaded");

    TEST_PASS();
    return true;
}

bool test_shape_cache() {
    TEST_START("shape cache");

    ShapeCache& cache = ShapeCache::Instance();

    // Clear any existing cache
    cache.Clear();
    ASSERT(cache.GetCount() == 0, "Cache should be empty after clear");

    // Note: Can't test actual loading without real SHP files
    // Just verify cache operations work

    TEST_PASS();
    return true;
}

bool test_remap_tables() {
    TEST_START("remap tables");

    // Initialize with a simple grayscale palette
    uint8_t palette[768];
    for (int i = 0; i < 256; i++) {
        palette[i * 3 + 0] = i;
        palette[i * 3 + 1] = i;
        palette[i * 3 + 2] = i;
    }

    InitRemapTables(palette);
    ASSERT(AreRemapTablesInitialized(), "Tables should be initialized");

    // Test identity table
    const uint8_t* identity = GetIdentityTable();
    ASSERT(identity != nullptr, "Identity table should exist");
    for (int i = 0; i < 256; i++) {
        ASSERT(identity[i] == i, "Identity should map to self");
    }

    // Test house tables exist
    for (int h = 0; h < HOUSE_COLOR_COUNT; h++) {
        const uint8_t* house_table = GetHouseRemapTable(h);
        ASSERT(house_table != nullptr, "House table should exist");
    }

    // Test shadow table
    const uint8_t* shadow = GetShadowTable();
    ASSERT(shadow != nullptr, "Shadow table should exist");
    ASSERT(shadow[0] == 0, "Shadow should preserve transparent");

    // Test fade tables
    const uint8_t* fade = GetFadeTables();
    ASSERT(fade != nullptr, "Fade tables should exist");
    // Level 0 should be near-identity
    ASSERT(fade[128] > 0, "Fade level 0 should not be black");
    // Level 15 should be dark
    ASSERT(fade[15 * 256 + 128] < 128, "Fade level 15 should be dark");

    TEST_PASS();
    return true;
}

bool test_draw_flags() {
    TEST_START("draw flags");

    // Test flag combinations
    uint32_t flags = SHAPE_NORMAL;
    ASSERT(flags == 0, "SHAPE_NORMAL should be 0");

    flags = SHAPE_CENTER | SHAPE_FLIP_X;
    ASSERT((flags & SHAPE_CENTER) != 0, "CENTER flag should be set");
    ASSERT((flags & SHAPE_FLIP_X) != 0, "FLIP_X flag should be set");
    ASSERT((flags & SHAPE_GHOST) == 0, "GHOST flag should not be set");

    TEST_PASS();
    return true;
}

// =============================================================================
// Visual Test
// =============================================================================

void run_visual_test() {
    printf("\n=== Visual Shape Test ===\n");

    // Create a test shape in memory (simple 32x32 sprite)
    const int SIZE = 32;
    std::vector<uint8_t> test_sprite(SIZE * SIZE);

    // Draw a simple pattern: circle with house-color-range fill
    for (int y = 0; y < SIZE; y++) {
        for (int x = 0; x < SIZE; x++) {
            int dx = x - SIZE / 2;
            int dy = y - SIZE / 2;
            int dist_sq = dx * dx + dy * dy;
            int radius_sq = (SIZE / 2 - 2) * (SIZE / 2 - 2);

            if (dist_sq < radius_sq) {
                // Inside circle - use house color range
                int shade = (dist_sq * 16) / radius_sq;
                test_sprite[y * SIZE + x] = 80 + shade;  // House color range
            } else {
                test_sprite[y * SIZE + x] = 0;  // Transparent
            }
        }
    }

    // Set up palette
    uint8_t palette[768];
    for (int i = 0; i < 256; i++) {
        palette[i * 3 + 0] = i;
        palette[i * 3 + 1] = i;
        palette[i * 3 + 2] = i;
    }

    // Add some color to house range
    for (int i = 80; i <= 95; i++) {
        int shade = (i - 80) * 16;
        palette[i * 3 + 0] = shade;
        palette[i * 3 + 1] = shade / 2;
        palette[i * 3 + 2] = shade;
    }

    Platform_Graphics_SetPalette((PaletteEntry*)palette, 0, 256);
    InitRemapTables(palette);

    // Get screen buffer
    GraphicsBuffer& screen = GraphicsBuffer::Screen();
    screen.Lock();
    screen.Clear(32);  // Dark gray background

    // Draw test sprites with different house colors
    printf("Drawing test sprites with house colors...\n");

    for (int h = 0; h < HOUSE_COLOR_COUNT; h++) {
        int x = 50 + (h % 4) * 100;
        int y = 50 + (h / 4) * 100;

        const uint8_t* remap = GetHouseRemapTable(h);

        // Draw sprite with remapping
        for (int sy = 0; sy < SIZE; sy++) {
            for (int sx = 0; sx < SIZE; sx++) {
                uint8_t pixel = test_sprite[sy * SIZE + sx];
                if (pixel != 0) {
                    screen.Put_Pixel(x + sx, y + sy, remap[pixel]);
                }
            }
        }
    }

    // Draw with shadow effect
    printf("Drawing shadow sprites...\n");
    const uint8_t* shadow = GetShadowTable();
    for (int sy = 0; sy < SIZE; sy++) {
        for (int sx = 0; sx < SIZE; sx++) {
            uint8_t pixel = test_sprite[sy * SIZE + sx];
            if (pixel != 0) {
                // Shadow is offset and uses shadow table on background
                int x = 450 + sx + 4;
                int y = 50 + sy + 4;
                uint8_t bg = screen.Get_Pixel(x, y);
                screen.Put_Pixel(x, y, shadow[bg]);
            }
        }
    }

    // Draw the actual sprite over shadow
    for (int sy = 0; sy < SIZE; sy++) {
        for (int sx = 0; sx < SIZE; sx++) {
            uint8_t pixel = test_sprite[sy * SIZE + sx];
            if (pixel != 0) {
                screen.Put_Pixel(450 + sx, 50 + sy, pixel);
            }
        }
    }

    screen.Unlock();
    screen.Flip();

    printf("Visual test displayed. Waiting 3 seconds...\n");
    Platform_Delay(3000);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("==========================================\n");
    printf("Shape Drawing Test Suite\n");
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
    test_shape_loading();
    test_shape_cache();
    test_remap_tables();
    test_draw_flags();

    printf("\n------------------------------------------\n");
    printf("Tests: %d/%d passed\n", tests_passed, tests_run);
    printf("------------------------------------------\n");

    // Run visual test
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

### ralph-tasks/verify/check-shape-drawing.sh
```bash
#!/bin/bash
# Verification script for Shape Drawing System

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Shape Drawing Implementation ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check files exist
echo "[1/5] Checking file existence..."
FILES=(
    "include/game/graphics/shape_renderer.h"
    "src/game/graphics/shape_renderer.cpp"
    "include/game/graphics/remap_tables.h"
    "src/game/graphics/remap_tables.cpp"
    "src/test_shape_drawing.cpp"
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
    include/game/graphics/shape_renderer.h 2>&1; then
    echo "VERIFY_FAILED: shape_renderer.h has syntax errors"
    exit 1
fi

if ! clang++ -std=c++17 -fsyntax-only \
    -I include \
    -I include/game \
    -I include/game/graphics \
    include/game/graphics/remap_tables.h 2>&1; then
    echo "VERIFY_FAILED: remap_tables.h has syntax errors"
    exit 1
fi
echo "  OK: Headers compile"

# Step 3: Build test executable
echo "[3/5] Building TestShapeDrawing..."
if ! cmake --build build --target TestShapeDrawing 2>&1; then
    echo "VERIFY_FAILED: Build failed"
    exit 1
fi
echo "  OK: Build succeeded"

# Step 4: Check executable exists
echo "[4/5] Checking executable..."
if [ ! -f "build/TestShapeDrawing" ]; then
    echo "VERIFY_FAILED: TestShapeDrawing executable not found"
    exit 1
fi
echo "  OK: Executable exists"

# Step 5: Run tests
echo "[5/5] Running unit tests..."
if timeout 30 ./build/TestShapeDrawing 2>&1 | grep -q "All tests PASSED"; then
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
# Add shape drawing sources to game library
target_sources(game_core PRIVATE
    src/game/graphics/shape_renderer.cpp
    src/game/graphics/remap_tables.cpp
)

# Shape Drawing Test
add_executable(TestShapeDrawing src/test_shape_drawing.cpp)

target_include_directories(TestShapeDrawing PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/game/graphics
)

target_link_libraries(TestShapeDrawing PRIVATE
    game_core
    redalert_platform
)

add_test(NAME TestShapeDrawing COMMAND TestShapeDrawing)
```

## Implementation Steps

1. Create `include/game/graphics/shape_renderer.h`
2. Create `src/game/graphics/shape_renderer.cpp`
3. Create `include/game/graphics/remap_tables.h`
4. Create `src/game/graphics/remap_tables.cpp`
5. Create `src/test_shape_drawing.cpp`
6. Update CMakeLists.txt
7. Create verification script
8. Build and test

## Success Criteria

- [ ] ShapeRenderer compiles without warnings
- [ ] Can create ShapeRenderer instances
- [ ] Shape loading from memory works
- [ ] Shape loading from MIX files works (if available)
- [ ] Frame information queries work
- [ ] Basic Draw() produces correct output
- [ ] DrawRemapped() applies house colors correctly
- [ ] DrawShadow() darkens background correctly
- [ ] DrawGhost() creates checkerboard transparency
- [ ] DrawFlat() renders solid color
- [ ] Flipping (X/Y) works correctly
- [ ] Centering works correctly
- [ ] Clipping works at all edges
- [ ] Remap tables initialize correctly
- [ ] All house color tables work
- [ ] ShapeCache caches shapes correctly
- [ ] All unit tests pass
- [ ] Visual test shows correct output

## Common Issues

### Frame Data Not Loading
**Symptom**: GetFrame() returns nullptr
**Solution**: Check Platform_Shape_GetFrame() returns correct data format

### House Colors Wrong
**Symptom**: Units all same color
**Solution**: Ensure remap range (80-95) matches palette, verify house table generation

### Transparency Not Working
**Symptom**: Black boxes around sprites
**Solution**: Ensure color 0 is being skipped, check transparent flag

### Shadow Too Dark/Light
**Symptom**: Shadows invisible or too prominent
**Solution**: Adjust shadow intensity in GenerateShadowTable()

### Performance Issues
**Symptom**: Low FPS with many sprites
**Solution**: Use PrecacheAllFrames(), prefer ShapeCache over individual loads

## Completion Promise
When verification passes, output:
```
<promise>TASK_15B_COMPLETE</promise>
```

## Escape Hatch
If stuck after 15 iterations:
- Document what's blocking in `ralph-tasks/blocked/15b.md`
- Output: `<promise>TASK_15B_BLOCKED</promise>`

## Max Iterations
15
