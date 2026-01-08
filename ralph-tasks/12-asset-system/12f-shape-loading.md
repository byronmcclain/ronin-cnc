# Task 12f: Shape/Sprite Loading

## Dependencies
- Task 12e (Palette Loading) must be complete
- Platform graphics buffer access available

## Context
Shapes (.SHP files) are Red Alert's sprite format. They contain multiple frames of animation with support for transparency, RLE compression, and various flags. This task implements the shape file parser and basic drawing routines.

## Objective
Create a ShapeData class that:
1. Parses .SHP files from MIX archives
2. Handles various frame formats (XOR, RLE, etc.)
3. Draws sprites to the back buffer
4. Supports color remapping for house colors

## Technical Background

### SHP File Format (Red Alert)

#### File Header
```
Offset  Size    Type     Description
------  ----    ----     -----------
0x0000  2       uint16   Frame count
0x0002  2       uint16   Unknown (usually 0)
0x0004  2       uint16   Image width (bounding)
0x0006  2       uint16   Image height (bounding)
0x0008  2       uint16   Largest frame size (for allocation)
0x000A  2       uint16   Flags
```

#### Frame Offset Table
Following header, array of frame offsets:
```
Offset  Size    Description
------  ----    -----------
0x000C  3*N     Array of 24-bit offsets (N = frame_count + 2)
                Last two entries point past valid data
```

Each offset is 3 bytes (little-endian, 24-bit value).

#### Frame Data
Each frame starts at its offset:
```
Offset  Size    Type     Description
------  ----    ----     -----------
0x0000  2       int16    X offset from image origin
0x0002  2       int16    Y offset from image origin
0x0004  2       uint16   Frame width
0x0006  2       uint16   Frame height
0x0008  1       uint8    Flags:
                         - Bit 0: Reserved
                         - Bit 1: RLE data
                         - Bit 2: Custom offset table
0x0009  3       uint24   Unused/padding (sometimes different)
0x000C  ...     raw      Pixel data (format depends on flags)
```

#### Pixel Data Formats

**Format 80 (LCW Compressed)**
- Uses LCW compression algorithm
- Common for larger sprites

**Format 40 (XOR with previous frame)**
- Data is XOR'd with previous frame
- Used for animation sequences

**Format 20 (RLE)**
- Simple run-length encoding
- 0 = transparent run (next byte = count)
- Non-zero = literal pixel

**Format 00 (Raw)**
- Uncompressed pixel data
- Width * Height bytes

### Known SHP Files
| Filename | Purpose |
|----------|---------|
| MOUSE.SHP | Mouse cursors |
| SIDEBAR.SHP | Sidebar graphics |
| CLOCK.SHP | Radar clock |
| PIPS.SHP | Health pips |
| Various unit/building shapes | Game entities |

## Deliverables
- [ ] `include/game/shape.h` - Shape structures
- [ ] `src/game/shape.cpp` - SHP parser and drawing
- [ ] RLE decompression support
- [ ] LCW decompression (optional, for advanced shapes)
- [ ] Color remapping during draw
- [ ] Test program with visual output
- [ ] Verification script

## Files to Create

### include/game/shape.h (NEW)
```cpp
#ifndef SHAPE_H
#define SHAPE_H

#include <cstdint>
#include <vector>
#include <string>

/// Flags for shape frames
enum ShapeFlags : uint8_t {
    SHAPE_NORMAL  = 0x00,
    SHAPE_HAS_RLE = 0x02,
    SHAPE_HAS_OFFSET_TABLE = 0x04,
};

/// Single frame of a shape sprite
struct ShapeFrame {
    int16_t x_offset;       // Draw offset from anchor
    int16_t y_offset;
    uint16_t width;         // Frame dimensions
    uint16_t height;
    uint8_t flags;          // Compression/format flags
    std::vector<uint8_t> pixels;  // Decompressed pixel data (width * height)

    ShapeFrame() : x_offset(0), y_offset(0), width(0), height(0), flags(0) {}

    /// Get pixel at position (returns 0 for out of bounds)
    uint8_t GetPixel(int x, int y) const;

    /// Check if pixel is transparent (index 0)
    bool IsTransparent(int x, int y) const;
};

/// Shape sprite data (multi-frame)
class ShapeData {
public:
    ShapeData();
    ~ShapeData();

    /// Load from raw SHP data
    /// @param data Raw SHP file data
    /// @param size Size in bytes
    /// @return true if parsed successfully
    bool LoadFromData(const uint8_t* data, uint32_t size);

    /// Load from MIX file
    /// @param filename SHP filename (e.g., "MOUSE.SHP")
    /// @return true if loaded successfully
    bool LoadFromMix(const char* filename);

    /// Check if loaded
    bool IsValid() const { return !frames_.empty(); }

    /// Get number of frames
    int GetFrameCount() const { return static_cast<int>(frames_.size()); }

    /// Get bounding dimensions
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }

    /// Get specific frame
    const ShapeFrame* GetFrame(int index) const;

    /// Draw frame to buffer
    /// @param buffer Destination pixel buffer
    /// @param pitch Buffer pitch (bytes per row)
    /// @param buf_width Buffer width for clipping
    /// @param buf_height Buffer height for clipping
    /// @param x Draw position X
    /// @param y Draw position Y
    /// @param frame Frame index to draw
    void Draw(uint8_t* buffer, int pitch, int buf_width, int buf_height,
              int x, int y, int frame) const;

    /// Draw with color remapping
    void DrawRemapped(uint8_t* buffer, int pitch, int buf_width, int buf_height,
                      int x, int y, int frame, const uint8_t* remap_table) const;

    /// Draw with transparency test only (for hit testing)
    /// Returns true if any pixel would be drawn at test position
    bool HitTest(int x, int y, int frame, int test_x, int test_y) const;

    /// Get filename (if loaded from MIX)
    const char* GetFilename() const { return filename_.c_str(); }

private:
    bool ParseHeader(const uint8_t* data, uint32_t size);
    bool ParseFrames(const uint8_t* data, uint32_t size);
    bool DecodeFrame(ShapeFrame& frame, const uint8_t* data, uint32_t available);
    bool DecodeRLE(ShapeFrame& frame, const uint8_t* data, uint32_t available);
    bool DecodeLCW(ShapeFrame& frame, const uint8_t* data, uint32_t available);
    bool DecodeRaw(ShapeFrame& frame, const uint8_t* data, uint32_t available);

    std::vector<ShapeFrame> frames_;
    std::string filename_;
    uint16_t width_;        // Bounding width
    uint16_t height_;       // Bounding height
    uint16_t flags_;
};

/// LCW decompression (Westwood's format)
/// @param src Compressed source data
/// @param dest Decompression destination
/// @param src_size Source data size
/// @param dest_size Destination buffer size
/// @return Number of bytes written to dest, or 0 on error
int LCW_Decompress(const uint8_t* src, uint8_t* dest, int src_size, int dest_size);

#endif // SHAPE_H
```

### src/game/shape.cpp (NEW)
```cpp
#include "game/shape.h"
#include "game/mix_manager.h"
#include <cstring>
#include <cstdio>
#include <algorithm>

// === ShapeFrame ===

uint8_t ShapeFrame::GetPixel(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return 0;
    }
    return pixels[y * width + x];
}

bool ShapeFrame::IsTransparent(int x, int y) const {
    return GetPixel(x, y) == 0;
}

// === ShapeData ===

ShapeData::ShapeData()
    : width_(0)
    , height_(0)
    , flags_(0)
{
}

ShapeData::~ShapeData() {
}

bool ShapeData::LoadFromData(const uint8_t* data, uint32_t size) {
    frames_.clear();
    width_ = 0;
    height_ = 0;

    if (!data || size < 12) {
        fprintf(stderr, "Shape: Data too small\n");
        return false;
    }

    return ParseHeader(data, size) && ParseFrames(data, size);
}

bool ShapeData::LoadFromMix(const char* filename) {
    if (!filename) return false;

    filename_ = filename;

    uint32_t size;
    uint8_t* data = MixManager::Instance().LoadFile(filename, &size);

    if (!data) {
        fprintf(stderr, "Shape: Failed to load '%s' from MIX\n", filename);
        return false;
    }

    bool result = LoadFromData(data, size);
    delete[] data;

    if (result) {
        printf("Shape: Loaded '%s' (%d frames, %dx%d)\n",
               filename, GetFrameCount(), width_, height_);
    }

    return result;
}

bool ShapeData::ParseHeader(const uint8_t* data, uint32_t size) {
    // Read header
    uint16_t frame_count = data[0] | (data[1] << 8);
    // uint16_t unknown = data[2] | (data[3] << 8);
    width_ = data[4] | (data[5] << 8);
    height_ = data[6] | (data[7] << 8);
    // uint16_t largest = data[8] | (data[9] << 8);
    flags_ = data[10] | (data[11] << 8);

    if (frame_count == 0 || frame_count > 1000) {
        fprintf(stderr, "Shape: Invalid frame count %u\n", frame_count);
        return false;
    }

    if (width_ == 0 || height_ == 0 || width_ > 2048 || height_ > 2048) {
        fprintf(stderr, "Shape: Invalid dimensions %ux%u\n", width_, height_);
        return false;
    }

    frames_.resize(frame_count);
    return true;
}

bool ShapeData::ParseFrames(const uint8_t* data, uint32_t size) {
    int frame_count = static_cast<int>(frames_.size());

    // Frame offset table starts at byte 12
    // Each offset is 3 bytes (24-bit)
    uint32_t offset_table_size = (frame_count + 2) * 3;
    if (12 + offset_table_size > size) {
        fprintf(stderr, "Shape: Not enough data for offset table\n");
        return false;
    }

    // Read offsets
    std::vector<uint32_t> offsets(frame_count + 2);
    for (int i = 0; i < frame_count + 2; i++) {
        const uint8_t* p = data + 12 + i * 3;
        offsets[i] = p[0] | (p[1] << 8) | (p[2] << 16);
    }

    // Parse each frame
    for (int i = 0; i < frame_count; i++) {
        uint32_t offset = offsets[i];
        uint32_t next_offset = offsets[i + 1];

        if (offset >= size || offset < 12 + offset_table_size) {
            fprintf(stderr, "Shape: Invalid frame %d offset %u\n", i, offset);
            return false;
        }

        uint32_t available = (next_offset > offset) ? (next_offset - offset) : (size - offset);

        if (!DecodeFrame(frames_[i], data + offset, available)) {
            fprintf(stderr, "Shape: Failed to decode frame %d\n", i);
            return false;
        }
    }

    return true;
}

bool ShapeData::DecodeFrame(ShapeFrame& frame, const uint8_t* data, uint32_t available) {
    if (available < 12) {
        return false;
    }

    // Read frame header
    frame.x_offset = static_cast<int16_t>(data[0] | (data[1] << 8));
    frame.y_offset = static_cast<int16_t>(data[2] | (data[3] << 8));
    frame.width = data[4] | (data[5] << 8);
    frame.height = data[6] | (data[7] << 8);
    frame.flags = data[8];

    if (frame.width == 0 || frame.height == 0) {
        // Empty frame
        frame.pixels.clear();
        return true;
    }

    if (frame.width > 2048 || frame.height > 2048) {
        fprintf(stderr, "Shape: Frame too large %ux%u\n", frame.width, frame.height);
        return false;
    }

    // Allocate pixel buffer
    frame.pixels.resize(frame.width * frame.height, 0);

    // Skip to pixel data (past header)
    const uint8_t* pixel_data = data + 12;  // Typical header size
    uint32_t pixel_available = available - 12;

    // Decode based on flags
    if (frame.flags & SHAPE_HAS_RLE) {
        return DecodeRLE(frame, pixel_data, pixel_available);
    } else {
        // Check for LCW by looking at first byte
        if (pixel_available > 0 && pixel_data[0] == 0x80) {
            return DecodeLCW(frame, pixel_data, pixel_available);
        } else {
            return DecodeRaw(frame, pixel_data, pixel_available);
        }
    }
}

bool ShapeData::DecodeRLE(ShapeFrame& frame, const uint8_t* data, uint32_t available) {
    // Simple RLE format:
    // If byte is 0, next byte is count of transparent pixels
    // Otherwise, byte is a literal pixel value

    uint32_t src_pos = 0;
    uint32_t dest_pos = 0;
    uint32_t total_pixels = frame.width * frame.height;

    while (dest_pos < total_pixels && src_pos < available) {
        uint8_t value = data[src_pos++];

        if (value == 0) {
            // Transparent run
            if (src_pos >= available) break;
            uint8_t count = data[src_pos++];
            dest_pos += count;
        } else {
            // Literal pixel
            if (dest_pos < total_pixels) {
                frame.pixels[dest_pos++] = value;
            }
        }
    }

    return true;
}

bool ShapeData::DecodeLCW(ShapeFrame& frame, const uint8_t* data, uint32_t available) {
    int written = LCW_Decompress(data, frame.pixels.data(),
                                  static_cast<int>(available),
                                  static_cast<int>(frame.pixels.size()));
    return written > 0;
}

bool ShapeData::DecodeRaw(ShapeFrame& frame, const uint8_t* data, uint32_t available) {
    uint32_t needed = frame.width * frame.height;
    if (available < needed) {
        // Fill what we can
        memcpy(frame.pixels.data(), data, available);
        return true;  // Partial is OK for some shapes
    }
    memcpy(frame.pixels.data(), data, needed);
    return true;
}

const ShapeFrame* ShapeData::GetFrame(int index) const {
    if (index < 0 || index >= static_cast<int>(frames_.size())) {
        return nullptr;
    }
    return &frames_[index];
}

void ShapeData::Draw(uint8_t* buffer, int pitch, int buf_width, int buf_height,
                     int x, int y, int frame_idx) const {
    const ShapeFrame* frame = GetFrame(frame_idx);
    if (!frame || frame->pixels.empty()) return;

    int draw_x = x + frame->x_offset;
    int draw_y = y + frame->y_offset;

    for (int fy = 0; fy < frame->height; fy++) {
        int screen_y = draw_y + fy;
        if (screen_y < 0 || screen_y >= buf_height) continue;

        for (int fx = 0; fx < frame->width; fx++) {
            int screen_x = draw_x + fx;
            if (screen_x < 0 || screen_x >= buf_width) continue;

            uint8_t pixel = frame->pixels[fy * frame->width + fx];
            if (pixel != 0) {  // 0 = transparent
                buffer[screen_y * pitch + screen_x] = pixel;
            }
        }
    }
}

void ShapeData::DrawRemapped(uint8_t* buffer, int pitch, int buf_width, int buf_height,
                             int x, int y, int frame_idx, const uint8_t* remap_table) const {
    const ShapeFrame* frame = GetFrame(frame_idx);
    if (!frame || frame->pixels.empty()) return;

    int draw_x = x + frame->x_offset;
    int draw_y = y + frame->y_offset;

    for (int fy = 0; fy < frame->height; fy++) {
        int screen_y = draw_y + fy;
        if (screen_y < 0 || screen_y >= buf_height) continue;

        for (int fx = 0; fx < frame->width; fx++) {
            int screen_x = draw_x + fx;
            if (screen_x < 0 || screen_x >= buf_width) continue;

            uint8_t pixel = frame->pixels[fy * frame->width + fx];
            if (pixel != 0) {
                pixel = remap_table[pixel];  // Apply remap
                buffer[screen_y * pitch + screen_x] = pixel;
            }
        }
    }
}

bool ShapeData::HitTest(int x, int y, int frame_idx, int test_x, int test_y) const {
    const ShapeFrame* frame = GetFrame(frame_idx);
    if (!frame) return false;

    int local_x = test_x - (x + frame->x_offset);
    int local_y = test_y - (y + frame->y_offset);

    return !frame->IsTransparent(local_x, local_y);
}

// === LCW Decompression ===
// Westwood's LCW (Lempel-Ziv-Westwood) compression

int LCW_Decompress(const uint8_t* src, uint8_t* dest, int src_size, int dest_size) {
    if (!src || !dest || src_size <= 0 || dest_size <= 0) {
        return 0;
    }

    int src_pos = 0;
    int dest_pos = 0;

    while (src_pos < src_size && dest_pos < dest_size) {
        uint8_t cmd = src[src_pos++];

        if (cmd == 0x80) {
            // End marker
            break;
        }
        else if ((cmd & 0x80) == 0) {
            // 0CCCCCCC: Copy C+1 bytes from source
            int count = (cmd & 0x7F) + 1;
            while (count-- > 0 && src_pos < src_size && dest_pos < dest_size) {
                dest[dest_pos++] = src[src_pos++];
            }
        }
        else if ((cmd & 0xC0) == 0x80) {
            // 10CCCCCC PPPPPPPP: Copy C+1 bytes from dest at relative offset -P
            if (src_pos >= src_size) break;
            int count = (cmd & 0x3F) + 1;
            int offset = src[src_pos++];
            int ref_pos = dest_pos - offset;
            if (ref_pos < 0) ref_pos = 0;
            while (count-- > 0 && dest_pos < dest_size) {
                dest[dest_pos++] = dest[ref_pos++];
            }
        }
        else if ((cmd & 0xE0) == 0xC0) {
            // 110CCCCC PPPPPPPP PPPPPPPP: Large copy from dest
            if (src_pos + 1 >= src_size) break;
            int count = (cmd & 0x1F) + 1;
            int offset = src[src_pos] | (src[src_pos + 1] << 8);
            src_pos += 2;
            int ref_pos = dest_pos - offset;
            if (ref_pos < 0) ref_pos = 0;
            while (count-- > 0 && dest_pos < dest_size) {
                dest[dest_pos++] = dest[ref_pos++];
            }
        }
        else if ((cmd & 0xFC) == 0xFC) {
            // 111111CC: Fill C+1 bytes with next byte
            if (src_pos >= src_size) break;
            int count = (cmd & 0x03) + 1;
            uint8_t fill = src[src_pos++];
            while (count-- > 0 && dest_pos < dest_size) {
                dest[dest_pos++] = fill;
            }
        }
        else if ((cmd & 0xFE) == 0xFE) {
            // 1111111C CCCCCCCC: Large fill
            if (src_pos + 1 >= src_size) break;
            int count = ((cmd & 0x01) << 8) | src[src_pos++];
            uint8_t fill = src[src_pos++];
            while (count-- > 0 && dest_pos < dest_size) {
                dest[dest_pos++] = fill;
            }
        }
        else {
            // Unknown command - treat as literal
            dest[dest_pos++] = cmd;
        }
    }

    return dest_pos;
}
```

### src/test_shape.cpp (NEW)
```cpp
// Test shape loading and display
#include <cstdio>
#include "game/shape.h"
#include "game/palette.h"
#include "game/mix_manager.h"
#include "platform.h"

void draw_shape_grid(ShapeData& shape) {
    uint8_t* pixels;
    int32_t width, height, pitch;

    Platform_Graphics_LockBackBuffer();
    Platform_Graphics_GetBackBuffer(&pixels, &width, &height, &pitch);

    // Clear background
    memset(pixels, 0, pitch * height);

    // Draw frames in a grid
    int frames_per_row = 8;
    int cell_w = width / frames_per_row;
    int cell_h = 50;  // Assume max frame height ~40

    for (int i = 0; i < shape.GetFrameCount() && i < 64; i++) {
        int gx = (i % frames_per_row) * cell_w + cell_w / 2;
        int gy = (i / frames_per_row) * cell_h + cell_h / 2;

        shape.Draw(pixels, pitch, width, height, gx, gy, i);
    }

    Platform_Graphics_UnlockBackBuffer();
    Platform_Graphics_Flip();
}

int main(int argc, char* argv[]) {
    const char* shape_name = "MOUSE.SHP";
    if (argc > 1) {
        shape_name = argv[1];
    }

    printf("=== Shape Loading Test ===\n\n");

    if (Platform_Init() != 0) {
        fprintf(stderr, "Platform init failed\n");
        return 1;
    }

    if (Platform_Graphics_Init() != 0) {
        fprintf(stderr, "Graphics init failed\n");
        Platform_Shutdown();
        return 1;
    }

    MixManager& mgr = MixManager::Instance();
    mgr.Initialize("gamedata");
    mgr.Mount("REDALERT.MIX");

    // Load a palette first
    PaletteManager::Instance().LoadPalette("TEMPERAT.PAL");

    // Test 1: Load shape
    printf("Test 1: Load shape '%s'...\n", shape_name);
    ShapeData shape;
    if (!shape.LoadFromMix(shape_name)) {
        fprintf(stderr, "  FAIL: Could not load shape\n");
        mgr.Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return 1;
    }
    printf("  PASS: Loaded %d frames\n", shape.GetFrameCount());

    // Test 2: Frame info
    printf("\nTest 2: Frame information...\n");
    printf("  Bounding: %dx%d\n", shape.GetWidth(), shape.GetHeight());
    for (int i = 0; i < std::min(5, shape.GetFrameCount()); i++) {
        const ShapeFrame* frame = shape.GetFrame(i);
        if (frame) {
            printf("  Frame %d: %dx%d at offset (%d,%d), flags=0x%02X\n",
                   i, frame->width, frame->height,
                   frame->x_offset, frame->y_offset, frame->flags);
        }
    }

    // Test 3: Draw frames
    printf("\nTest 3: Draw shape grid...\n");
    draw_shape_grid(shape);
    printf("  PASS: Grid drawn\n");

    // Animate through frames
    printf("\nTest 4: Frame animation...\n");
    for (int i = 0; i < shape.GetFrameCount() && i < 30; i++) {
        uint8_t* pixels;
        int32_t width, height, pitch;

        Platform_Graphics_LockBackBuffer();
        Platform_Graphics_GetBackBuffer(&pixels, &width, &height, &pitch);
        memset(pixels, 0, pitch * height);

        // Draw centered
        shape.Draw(pixels, pitch, width, height, width/2, height/2, i);

        Platform_Graphics_UnlockBackBuffer();
        Platform_Graphics_Flip();

        Platform_Timer_Delay(100);
    }
    printf("  PASS: Animation complete\n");

    // Show final grid
    draw_shape_grid(shape);
    Platform_Timer_Delay(2000);

    mgr.Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();

    printf("\n=== Shape Test Complete ===\n");
    return 0;
}
```

### CMakeLists.txt (MODIFY)
```cmake
# Add shape to game_assets library
target_sources(game_assets PRIVATE
    src/game/shape.cpp
)

# Shape test
add_executable(TestShape src/test_shape.cpp)
target_link_libraries(TestShape PRIVATE game_assets)
```

## Verification Command
```bash
ralph-tasks/verify/check-shape.sh
```

## Verification Script

### ralph-tasks/verify/check-shape.sh (NEW)
```bash
#!/bin/bash
# Verification script for shape loading

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Shape Loading ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check source files
echo "[1/4] Checking source files..."
if [ ! -f "include/game/shape.h" ]; then
    echo "VERIFY_FAILED: shape.h not found"
    exit 1
fi
if [ ! -f "src/game/shape.cpp" ]; then
    echo "VERIFY_FAILED: shape.cpp not found"
    exit 1
fi
echo "  OK: Source files exist"

# Step 2: Build
echo "[2/4] Building..."
if ! cmake --build build --target TestShape 2>&1; then
    echo "VERIFY_FAILED: Build failed"
    exit 1
fi
echo "  OK: Build successful"

# Step 3: Check for MIX file
echo "[3/4] Checking for MIX file..."
if [ ! -f "gamedata/REDALERT.MIX" ]; then
    echo "VERIFY_WARNING: REDALERT.MIX not found"
    echo "VERIFY_SUCCESS (build only)"
    exit 0
fi
echo "  OK: REDALERT.MIX exists"

# Step 4: Run test
echo "[4/4] Running shape test..."
if ! timeout 30 ./build/TestShape MOUSE.SHP 2>&1; then
    echo "VERIFY_FAILED: Shape test failed"
    exit 1
fi
echo "  OK: Shape test passed"

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Create `include/game/shape.h`
2. Create `src/game/shape.cpp`
3. Update game_assets library in CMakeLists.txt
4. Create `src/test_shape.cpp`
5. Build: `cmake --build build --target TestShape`
6. Test: `./build/TestShape MOUSE.SHP`
7. Verify mouse cursors display correctly
8. Run verification script

## Success Criteria
- Shape loads from MIX file
- Frame count matches expected (MOUSE.SHP has ~140 frames)
- Frames draw with correct transparency (index 0)
- Offsets position frames correctly
- LCW decompression produces valid pixels
- No visual corruption

## Common Issues

### Frames appear blank
- Check offset calculation (24-bit offsets)
- Verify frame header parsing

### Wrong colors
- Ensure palette is loaded before drawing
- Check pixel index values are in range 0-255

### Corruption/garbage
- LCW decompression may have bugs
- Check bounds in all decoders

### Missing frames
- Some shapes use XOR encoding with previous frame
- Not all formats are implemented initially

## Completion Promise
When verification passes, output:
```
<promise>TASK_12F_COMPLETE</promise>
```

## Escape Hatch
If stuck after 20 iterations (this is complex):
- Document in `ralph-tasks/blocked/12f.md`
- Output: `<promise>TASK_12F_BLOCKED</promise>`

## Max Iterations
20
