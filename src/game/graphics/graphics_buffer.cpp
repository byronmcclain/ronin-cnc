/**
 * GraphicsBuffer Implementation
 *
 * Wraps platform layer graphics for game rendering.
 */

#include "game/graphics/graphics_buffer.h"
#include "platform.h"
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
        for (int row = 0; row < height_; row++) {
            Platform_MemSet(pixels_ + row * pitch_, color, width_);
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
