# Plan 09: Assembly Rewrites

## Objective
Convert x86 assembly routines to portable C/C++ code that runs on both Intel and Apple Silicon Macs, with optional SIMD optimizations.

## Current State Analysis

### Assembly Files in Codebase

**Total Count:**
- CODE/: 12 .ASM files
- WIN32LIB/: 100+ .ASM files
- IPX/: 20+ .ASM files (networking - will be removed)
- WWFLAT32/: Similar to WIN32LIB

**Assembly Syntax:**
- TASM/MASM syntax
- 32-bit flat memory model
- Uses `PROC`/`ENDP`, `PUBLIC`, `EXTRN`
- Some inline assembly in .CPP files

### Categories by Priority

#### Priority 1: Graphics (Performance Critical)
| File | Purpose | Lines | C++ Exists? |
|------|---------|-------|-------------|
| WIN32LIB/DRAWBUFF/BITBLIT.ASM | Bitmap blitting | ~500 | Partial |
| WIN32LIB/DRAWBUFF/CLEAR.ASM | Buffer clearing | ~100 | Yes |
| WIN32LIB/DRAWBUFF/SCALE.ASM | Scaling sprites | ~400 | Partial |
| WIN32LIB/DRAWBUFF/REMAP.ASM | Palette remapping | ~300 | No |
| WIN32LIB/DRAWBUFF/SHADOW.ASM | Shadow drawing | ~200 | No |
| WIN32LIB/SHAPE/*.ASM | Shape rendering (14 files) | ~2000 | Some |

#### Priority 2: Audio (Medium Priority)
| File | Purpose | Lines | C++ Exists? |
|------|---------|-------|-------------|
| WIN32LIB/AUDIO/AUDUNCMP.ASM | Audio decompression | ~300 | Yes (ADPCM.CPP) |
| WIN32LIB/AUDIO/SOSCODEC.ASM | SOS codec | ~400 | Partial |

#### Priority 3: Compression (Required)
| File | Purpose | Lines | C++ Exists? |
|------|---------|-------|-------------|
| CODE/LCWCOMP.ASM | LCW compression | ~500 | Unknown |
| WIN32LIB/IFF/LCWUNCMP.ASM | LCW decompression | ~300 | Unknown |

#### Priority 4: Utilities (Low Priority)
| File | Purpose | Lines | C++ Exists? |
|------|---------|-------|-------------|
| WIN32LIB/MISC/CRC.ASM | CRC calculation | ~100 | Easy to write |
| WIN32LIB/MISC/RANDOM.ASM | Random numbers | ~50 | Easy to write |
| CODE/CPUID.ASM | CPU detection | ~200 | Not needed |
| WIN32LIB/MEM/MEM_COPY.ASM | Memory copy | ~100 | Use memcpy |

## Implementation Strategy

### Approach

1. **Find C++ equivalents first** - Many ASM files have C++ fallbacks
2. **Rewrite in portable C++** - No inline assembly
3. **Add SIMD later** - Use intrinsics for optimization (optional)
4. **Test thoroughly** - Compare output with reference implementation

### 9.1 Graphics Blitting

File: `src/platform/graphics_blit.cpp`
```cpp
#include <cstdint>
#include <cstring>
#include <algorithm>

//=============================================================================
// Basic Blit (replaces BITBLIT.ASM)
//=============================================================================

// Copy rectangular region
void Buffer_To_Buffer(
    uint8_t* dest, int dest_width, int dest_height, int dest_pitch,
    const uint8_t* src, int src_width, int src_height, int src_pitch,
    int dest_x, int dest_y, int src_x, int src_y,
    int width, int height)
{
    // Clip to destination bounds
    if (dest_x < 0) {
        width += dest_x;
        src_x -= dest_x;
        dest_x = 0;
    }
    if (dest_y < 0) {
        height += dest_y;
        src_y -= dest_y;
        dest_y = 0;
    }
    if (dest_x + width > dest_width) {
        width = dest_width - dest_x;
    }
    if (dest_y + height > dest_height) {
        height = dest_height - dest_y;
    }

    if (width <= 0 || height <= 0) return;

    // Copy rows
    uint8_t* dst_row = dest + dest_y * dest_pitch + dest_x;
    const uint8_t* src_row = src + src_y * src_pitch + src_x;

    for (int y = 0; y < height; y++) {
        memcpy(dst_row, src_row, width);
        dst_row += dest_pitch;
        src_row += src_pitch;
    }
}

// Transparent blit (skip color 0)
void Buffer_To_Buffer_Trans(
    uint8_t* dest, int dest_pitch,
    const uint8_t* src, int src_pitch,
    int dest_x, int dest_y,
    int width, int height)
{
    uint8_t* dst_row = dest + dest_y * dest_pitch + dest_x;
    const uint8_t* src_row = src;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint8_t pixel = src_row[x];
            if (pixel != 0) {
                dst_row[x] = pixel;
            }
        }
        dst_row += dest_pitch;
        src_row += src_pitch;
    }
}
```

### 9.2 Palette Remapping

```cpp
//=============================================================================
// Palette Remap (replaces REMAP.ASM)
//=============================================================================

// Remap colors through a lookup table
void Buffer_Remap(
    uint8_t* buffer, int pitch, int width, int height,
    const uint8_t* remap_table)
{
    for (int y = 0; y < height; y++) {
        uint8_t* row = buffer + y * pitch;
        for (int x = 0; x < width; x++) {
            row[x] = remap_table[row[x]];
        }
    }
}

// Remap with transparency
void Buffer_Remap_Trans(
    uint8_t* buffer, int pitch, int width, int height,
    const uint8_t* remap_table)
{
    for (int y = 0; y < height; y++) {
        uint8_t* row = buffer + y * pitch;
        for (int x = 0; x < width; x++) {
            uint8_t pixel = row[x];
            if (pixel != 0) {
                row[x] = remap_table[pixel];
            }
        }
    }
}
```

### 9.3 Scaling

```cpp
//=============================================================================
// Scaling (replaces SCALE.ASM)
//=============================================================================

// Scale source to destination using nearest neighbor
void Buffer_Scale(
    uint8_t* dest, int dest_width, int dest_height, int dest_pitch,
    const uint8_t* src, int src_width, int src_height, int src_pitch,
    int dest_x, int dest_y, int scale_width, int scale_height)
{
    if (scale_width <= 0 || scale_height <= 0) return;

    // Fixed-point scaling factors (16.16)
    int x_ratio = (src_width << 16) / scale_width;
    int y_ratio = (src_height << 16) / scale_height;

    int y_accum = 0;
    for (int y = 0; y < scale_height; y++) {
        if (dest_y + y < 0 || dest_y + y >= dest_height) {
            y_accum += y_ratio;
            continue;
        }

        uint8_t* dst_row = dest + (dest_y + y) * dest_pitch + dest_x;
        const uint8_t* src_row = src + (y_accum >> 16) * src_pitch;

        int x_accum = 0;
        for (int x = 0; x < scale_width; x++) {
            if (dest_x + x >= 0 && dest_x + x < dest_width) {
                dst_row[x] = src_row[x_accum >> 16];
            }
            x_accum += x_ratio;
        }
        y_accum += y_ratio;
    }
}

// Scale with transparency
void Buffer_Scale_Trans(
    uint8_t* dest, int dest_width, int dest_height, int dest_pitch,
    const uint8_t* src, int src_width, int src_height, int src_pitch,
    int dest_x, int dest_y, int scale_width, int scale_height)
{
    if (scale_width <= 0 || scale_height <= 0) return;

    int x_ratio = (src_width << 16) / scale_width;
    int y_ratio = (src_height << 16) / scale_height;

    int y_accum = 0;
    for (int y = 0; y < scale_height; y++) {
        if (dest_y + y < 0 || dest_y + y >= dest_height) {
            y_accum += y_ratio;
            continue;
        }

        uint8_t* dst_row = dest + (dest_y + y) * dest_pitch + dest_x;
        const uint8_t* src_row = src + (y_accum >> 16) * src_pitch;

        int x_accum = 0;
        for (int x = 0; x < scale_width; x++) {
            if (dest_x + x >= 0 && dest_x + x < dest_width) {
                uint8_t pixel = src_row[x_accum >> 16];
                if (pixel != 0) {
                    dst_row[x] = pixel;
                }
            }
            x_accum += x_ratio;
        }
        y_accum += y_ratio;
    }
}
```

### 9.4 Shadow Drawing

```cpp
//=============================================================================
// Shadow (replaces SHADOW.ASM)
//=============================================================================

// Draw shadow (darken existing pixels)
void Buffer_Shadow(
    uint8_t* buffer, int pitch, int x, int y, int width, int height,
    const uint8_t* shadow_table)  // 256-byte table mapping colors to darker versions
{
    for (int row = 0; row < height; row++) {
        uint8_t* ptr = buffer + (y + row) * pitch + x;
        for (int col = 0; col < width; col++) {
            ptr[col] = shadow_table[ptr[col]];
        }
    }
}

// Draw shadow with mask
void Buffer_Shadow_Mask(
    uint8_t* buffer, int buffer_pitch,
    const uint8_t* mask, int mask_pitch,
    int x, int y, int width, int height,
    const uint8_t* shadow_table)
{
    for (int row = 0; row < height; row++) {
        uint8_t* dst = buffer + (y + row) * buffer_pitch + x;
        const uint8_t* msk = mask + row * mask_pitch;

        for (int col = 0; col < width; col++) {
            if (msk[col] != 0) {
                dst[col] = shadow_table[dst[col]];
            }
        }
    }
}
```

### 9.5 LCW Compression/Decompression

```cpp
//=============================================================================
// LCW Decompression (replaces LCWUNCMP.ASM)
//=============================================================================

// LCW is a variant of LZSS compression
int LCW_Uncomp(const uint8_t* source, uint8_t* dest, int dest_size) {
    const uint8_t* src = source;
    uint8_t* dst = dest;
    uint8_t* dst_end = dest + dest_size;

    while (dst < dst_end) {
        uint8_t cmd = *src++;

        if (cmd & 0x80) {
            // Copy from source
            int count = cmd & 0x3F;
            if (cmd & 0x40) {
                // Long count
                count = (count << 8) | *src++;
            }
            count++;

            while (count-- > 0 && dst < dst_end) {
                *dst++ = *src++;
            }
        } else {
            // Copy from destination (back-reference)
            int count = (cmd >> 4) + 3;
            int offset = ((cmd & 0x0F) << 8) | *src++;

            const uint8_t* ref = dst - offset;
            while (count-- > 0 && dst < dst_end) {
                *dst++ = *ref++;
            }
        }
    }

    return (int)(dst - dest);
}

// LCW Compression
int LCW_Comp(const uint8_t* source, uint8_t* dest, int source_size) {
    // Compression is more complex - implement if needed
    // For now, store uncompressed
    memcpy(dest, source, source_size);
    return source_size;
}
```

### 9.6 CRC and Random

```cpp
//=============================================================================
// CRC32 (replaces CRC.ASM)
//=============================================================================

static uint32_t crc_table[256];
static bool crc_table_initialized = false;

void CRC_Init() {
    if (crc_table_initialized) return;

    for (int i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
        crc_table[i] = crc;
    }
    crc_table_initialized = true;
}

uint32_t CRC_Memory(const void* data, int size) {
    CRC_Init();

    uint32_t crc = 0xFFFFFFFF;
    const uint8_t* bytes = (const uint8_t*)data;

    for (int i = 0; i < size; i++) {
        crc = crc_table[(crc ^ bytes[i]) & 0xFF] ^ (crc >> 8);
    }

    return crc ^ 0xFFFFFFFF;
}

//=============================================================================
// Random Number Generator (replaces RANDOM.ASM)
//=============================================================================

static uint32_t random_seed = 12345;

void Random_Seed(uint32_t seed) {
    random_seed = seed;
}

uint32_t Random_Get() {
    // Linear congruential generator
    random_seed = random_seed * 1103515245 + 12345;
    return (random_seed >> 16) & 0x7FFF;
}

int Random_Range(int min, int max) {
    if (min >= max) return min;
    return min + (Random_Get() % (max - min + 1));
}
```

### 9.7 Shape Drawing (Most Complex)

```cpp
//=============================================================================
// Shape Drawing (replaces SHAPE/*.ASM)
//=============================================================================

// Shape data format (simplified)
struct ShapeHeader {
    uint16_t width;
    uint16_t height;
    uint16_t flags;
    // Followed by pixel data
};

void Draw_Shape(
    uint8_t* buffer, int buffer_pitch, int buffer_width, int buffer_height,
    const uint8_t* shape_data, int x, int y,
    const uint8_t* remap_table,     // Optional color remapping
    const uint8_t* shadow_table,    // Optional shadow
    int flags)                       // Drawing flags
{
    const ShapeHeader* header = (const ShapeHeader*)shape_data;
    const uint8_t* pixels = shape_data + sizeof(ShapeHeader);

    int width = header->width;
    int height = header->height;

    // Clip to buffer
    int clip_left = std::max(0, -x);
    int clip_top = std::max(0, -y);
    int clip_right = std::max(0, (x + width) - buffer_width);
    int clip_bottom = std::max(0, (y + height) - buffer_height);

    int draw_width = width - clip_left - clip_right;
    int draw_height = height - clip_top - clip_bottom;

    if (draw_width <= 0 || draw_height <= 0) return;

    for (int row = 0; row < draw_height; row++) {
        int src_y = clip_top + row;
        int dst_y = y + clip_top + row;

        uint8_t* dst = buffer + dst_y * buffer_pitch + x + clip_left;
        const uint8_t* src = pixels + src_y * width + clip_left;

        for (int col = 0; col < draw_width; col++) {
            uint8_t pixel = src[col];

            // Skip transparent pixels
            if (pixel == 0) continue;

            // Apply remapping if specified
            if (remap_table) {
                pixel = remap_table[pixel];
            }

            dst[col] = pixel;
        }
    }
}
```

## SIMD Optimization (Optional Future Work)

For performance-critical paths on x86_64, use SSE/AVX:

```cpp
#ifdef __SSE2__
#include <emmintrin.h>

void Buffer_Clear_SSE2(uint8_t* buffer, int size, uint8_t value) {
    __m128i fill = _mm_set1_epi8(value);

    // Align to 16 bytes
    while (((uintptr_t)buffer & 15) && size > 0) {
        *buffer++ = value;
        size--;
    }

    // Process 16 bytes at a time
    while (size >= 16) {
        _mm_store_si128((__m128i*)buffer, fill);
        buffer += 16;
        size -= 16;
    }

    // Handle remainder
    while (size > 0) {
        *buffer++ = value;
        size--;
    }
}
#endif
```

## Tasks Breakdown

### Phase 1: Inventory (1 day)
- [ ] List all ASM files
- [ ] Identify which have C++ equivalents
- [ ] Determine which are actually used

### Phase 2: Core Graphics (3 days)
- [ ] Implement blit routines
- [ ] Implement remap routines
- [ ] Implement scale routines
- [ ] Test against reference images

### Phase 3: Shape Drawing (2 days)
- [ ] Understand shape data format
- [ ] Implement basic shape drawing
- [ ] Add clipping support
- [ ] Test with game sprites

### Phase 4: Compression (1 day)
- [ ] Implement LCW decompression
- [ ] Test with game data
- [ ] Implement compression if needed

### Phase 5: Utilities (1 day)
- [ ] CRC32 implementation
- [ ] Random number generator
- [ ] Memory operations

### Phase 6: Integration & Testing (2 days)
- [ ] Replace ASM function declarations
- [ ] Link C++ implementations
- [ ] Verify game renders correctly
- [ ] Performance testing

## Acceptance Criteria

- [ ] No assembly files in build
- [ ] Game renders correctly
- [ ] Shapes/sprites display correctly
- [ ] Compression/decompression works
- [ ] Performance acceptable (>30 FPS)

## Estimated Duration
**8-10 days**

## Dependencies
- Plans 01-08 completed
- Game running with stubs

## Next Plan
Once assembly is replaced, proceed to **Plan 10: Networking**
