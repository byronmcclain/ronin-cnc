/**
 * ShapeRenderer Implementation
 */

#include "game/graphics/shape_renderer.h"
#include "game/graphics/graphics_buffer.h"
#include "platform.h"
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

    // Set frame dimensions from shape max size
    // The platform layer returns raw pixel data
    cached.x_offset = 0;
    cached.y_offset = 0;
    cached.width = static_cast<int16_t>(width_);
    cached.height = static_cast<int16_t>(height_);

    // Adjust height if we got fewer bytes than expected
    int expected = width_ * height_;
    if (bytes < expected) {
        cached.height = static_cast<int16_t>(bytes / width_);
        if (cached.height <= 0) cached.height = 1;
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

    return DrawInternal(buffer, x, y, *f, nullptr, SHAPE_PREDATOR, static_cast<uint8_t>(phase));
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
