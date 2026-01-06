# Plan 09: Assembly Rewrites (Rust Implementation)

## Objective
Convert x86 assembly routines to portable Rust code that runs on both Intel and Apple Silicon Macs, with optional SIMD optimizations using `std::arch` or the `packed_simd` crate.

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
| File | Purpose | Lines | Rust Equivalent |
|------|---------|-------|-----------------|
| WIN32LIB/DRAWBUFF/BITBLIT.ASM | Bitmap blitting | ~500 | `blit.rs` |
| WIN32LIB/DRAWBUFF/CLEAR.ASM | Buffer clearing | ~100 | `slice::fill` |
| WIN32LIB/DRAWBUFF/SCALE.ASM | Scaling sprites | ~400 | `scale.rs` |
| WIN32LIB/DRAWBUFF/REMAP.ASM | Palette remapping | ~300 | `remap.rs` |
| WIN32LIB/DRAWBUFF/SHADOW.ASM | Shadow drawing | ~200 | `shadow.rs` |
| WIN32LIB/SHAPE/*.ASM | Shape rendering (14 files) | ~2000 | `shape.rs` |

#### Priority 2: Audio (Medium Priority)
| File | Purpose | Lines | Rust Equivalent |
|------|---------|-------|-----------------|
| WIN32LIB/AUDIO/AUDUNCMP.ASM | Audio decompression | ~300 | In audio.rs |
| WIN32LIB/AUDIO/SOSCODEC.ASM | SOS codec | ~400 | In audio.rs |

#### Priority 3: Compression (Required)
| File | Purpose | Lines | Rust Equivalent |
|------|---------|-------|-----------------|
| CODE/LCWCOMP.ASM | LCW compression | ~500 | `lcw.rs` |
| WIN32LIB/IFF/LCWUNCMP.ASM | LCW decompression | ~300 | `lcw.rs` |

#### Priority 4: Utilities (Low Priority)
| File | Purpose | Lines | Rust Equivalent |
|------|---------|-------|-----------------|
| WIN32LIB/MISC/CRC.ASM | CRC calculation | ~100 | `crc32` crate or manual |
| WIN32LIB/MISC/RANDOM.ASM | Random numbers | ~50 | `rand` crate or LCG |
| CODE/CPUID.ASM | CPU detection | ~200 | Not needed |
| WIN32LIB/MEM/MEM_COPY.ASM | Memory copy | ~100 | `slice::copy_from_slice` |

## Implementation Strategy

### Approach

1. **Implement in safe Rust first** - Use standard library where possible
2. **Optimize with SIMD later** - Use `std::arch` for platform-specific intrinsics
3. **Expose via FFI** - Make callable from C++ game code
4. **Benchmark thoroughly** - Compare against reference implementation

## Rust Module Structure

```
platform/src/
├── lib.rs
├── blit/
│   ├── mod.rs
│   ├── basic.rs      # Basic blitting
│   ├── transparent.rs # Transparency handling
│   └── simd.rs       # SIMD-optimized versions
├── scale.rs          # Scaling routines
├── remap.rs          # Palette remapping
├── shadow.rs         # Shadow effects
├── shape.rs          # Shape/sprite rendering
├── lcw.rs            # LCW compression
└── crc.rs            # CRC32 calculation
```

### 9.1 Graphics Blitting

File: `platform/src/blit/mod.rs`
```rust
//! Bitmap blitting routines (replaces BITBLIT.ASM)

pub mod basic;
pub mod transparent;

#[cfg(any(target_arch = "x86_64", target_arch = "aarch64"))]
pub mod simd;

use crate::ffi::catch_panic;

/// Clipping rectangle
#[derive(Debug, Clone, Copy)]
pub struct ClipRect {
    pub x: i32,
    pub y: i32,
    pub width: i32,
    pub height: i32,
}

impl ClipRect {
    pub fn new(x: i32, y: i32, width: i32, height: i32) -> Self {
        Self { x, y, width, height }
    }

    /// Clip source rectangle to destination bounds
    pub fn clip_to_bounds(
        &self,
        dest_x: i32,
        dest_y: i32,
        dest_width: i32,
        dest_height: i32,
    ) -> Option<(ClipRect, i32, i32)> {
        let mut src_x = self.x;
        let mut src_y = self.y;
        let mut width = self.width;
        let mut height = self.height;
        let mut dst_x = dest_x;
        let mut dst_y = dest_y;

        // Clip left edge
        if dst_x < 0 {
            width += dst_x;
            src_x -= dst_x;
            dst_x = 0;
        }

        // Clip top edge
        if dst_y < 0 {
            height += dst_y;
            src_y -= dst_y;
            dst_y = 0;
        }

        // Clip right edge
        if dst_x + width > dest_width {
            width = dest_width - dst_x;
        }

        // Clip bottom edge
        if dst_y + height > dest_height {
            height = dest_height - dst_y;
        }

        if width <= 0 || height <= 0 {
            None
        } else {
            Some((ClipRect::new(src_x, src_y, width, height), dst_x, dst_y))
        }
    }
}
```

File: `platform/src/blit/basic.rs`
```rust
//! Basic blitting operations

use super::ClipRect;

/// Copy rectangular region from source to destination
pub fn buffer_to_buffer(
    dest: &mut [u8],
    dest_pitch: usize,
    dest_width: i32,
    dest_height: i32,
    src: &[u8],
    src_pitch: usize,
    dest_x: i32,
    dest_y: i32,
    src_rect: ClipRect,
) {
    let Some((clipped, dst_x, dst_y)) = src_rect.clip_to_bounds(
        dest_x, dest_y, dest_width, dest_height
    ) else {
        return;
    };

    let dst_x = dst_x as usize;
    let dst_y = dst_y as usize;
    let src_x = clipped.x as usize;
    let src_y = clipped.y as usize;
    let width = clipped.width as usize;
    let height = clipped.height as usize;

    for row in 0..height {
        let dst_offset = (dst_y + row) * dest_pitch + dst_x;
        let src_offset = (src_y + row) * src_pitch + src_x;

        if dst_offset + width <= dest.len() && src_offset + width <= src.len() {
            dest[dst_offset..dst_offset + width]
                .copy_from_slice(&src[src_offset..src_offset + width]);
        }
    }
}

/// Fill buffer with a single value
#[inline]
pub fn buffer_clear(buffer: &mut [u8], value: u8) {
    buffer.fill(value);
}

/// Fill rectangular region with a value
pub fn buffer_fill_rect(
    buffer: &mut [u8],
    pitch: usize,
    x: usize,
    y: usize,
    width: usize,
    height: usize,
    value: u8,
) {
    for row in 0..height {
        let offset = (y + row) * pitch + x;
        if offset + width <= buffer.len() {
            buffer[offset..offset + width].fill(value);
        }
    }
}
```

File: `platform/src/blit/transparent.rs`
```rust
//! Transparent blitting (skip color 0)

/// Blit with transparency (color 0 = transparent)
pub fn buffer_to_buffer_trans(
    dest: &mut [u8],
    dest_pitch: usize,
    src: &[u8],
    src_pitch: usize,
    dest_x: usize,
    dest_y: usize,
    width: usize,
    height: usize,
) {
    for row in 0..height {
        let dst_offset = (dest_y + row) * dest_pitch + dest_x;
        let src_offset = row * src_pitch;

        for col in 0..width {
            let src_idx = src_offset + col;
            let dst_idx = dst_offset + col;

            if src_idx < src.len() && dst_idx < dest.len() {
                let pixel = src[src_idx];
                if pixel != 0 {
                    dest[dst_idx] = pixel;
                }
            }
        }
    }
}

/// Blit with transparency and color key
pub fn buffer_to_buffer_trans_key(
    dest: &mut [u8],
    dest_pitch: usize,
    src: &[u8],
    src_pitch: usize,
    dest_x: usize,
    dest_y: usize,
    width: usize,
    height: usize,
    transparent_color: u8,
) {
    for row in 0..height {
        let dst_offset = (dest_y + row) * dest_pitch + dest_x;
        let src_offset = row * src_pitch;

        for col in 0..width {
            let src_idx = src_offset + col;
            let dst_idx = dst_offset + col;

            if src_idx < src.len() && dst_idx < dest.len() {
                let pixel = src[src_idx];
                if pixel != transparent_color {
                    dest[dst_idx] = pixel;
                }
            }
        }
    }
}
```

### 9.2 Palette Remapping

File: `platform/src/remap.rs`
```rust
//! Palette remapping (replaces REMAP.ASM)

/// Remap colors through a lookup table
pub fn buffer_remap(
    buffer: &mut [u8],
    pitch: usize,
    width: usize,
    height: usize,
    remap_table: &[u8; 256],
) {
    for row in 0..height {
        let offset = row * pitch;
        for col in 0..width {
            let idx = offset + col;
            if idx < buffer.len() {
                buffer[idx] = remap_table[buffer[idx] as usize];
            }
        }
    }
}

/// Remap colors with transparency (skip color 0)
pub fn buffer_remap_trans(
    buffer: &mut [u8],
    pitch: usize,
    width: usize,
    height: usize,
    remap_table: &[u8; 256],
) {
    for row in 0..height {
        let offset = row * pitch;
        for col in 0..width {
            let idx = offset + col;
            if idx < buffer.len() {
                let pixel = buffer[idx];
                if pixel != 0 {
                    buffer[idx] = remap_table[pixel as usize];
                }
            }
        }
    }
}

/// Apply remap to source, write to destination
pub fn buffer_remap_copy(
    dest: &mut [u8],
    src: &[u8],
    remap_table: &[u8; 256],
) {
    for (d, s) in dest.iter_mut().zip(src.iter()) {
        *d = remap_table[*s as usize];
    }
}

// =============================================================================
// FFI Exports
// =============================================================================

/// Remap buffer colors through lookup table
///
/// # Safety
/// - `buffer` must point to valid memory of at least `pitch * height` bytes
/// - `remap_table` must point to exactly 256 bytes
#[no_mangle]
pub unsafe extern "C" fn Platform_Buffer_Remap(
    buffer: *mut u8,
    pitch: i32,
    width: i32,
    height: i32,
    remap_table: *const u8,
) -> i32 {
    crate::ffi::catch_panic(|| {
        if buffer.is_null() || remap_table.is_null() {
            return Err(crate::PlatformError::NullPointer);
        }

        let buf_len = (pitch * height) as usize;
        let buffer_slice = std::slice::from_raw_parts_mut(buffer, buf_len);
        let table: &[u8; 256] = &*(remap_table as *const [u8; 256]);

        buffer_remap(
            buffer_slice,
            pitch as usize,
            width as usize,
            height as usize,
            table,
        );

        Ok(())
    })
}
```

### 9.3 Scaling

File: `platform/src/scale.rs`
```rust
//! Scaling routines (replaces SCALE.ASM)

use crate::blit::ClipRect;

/// Scale source to destination using nearest neighbor
pub fn buffer_scale(
    dest: &mut [u8],
    dest_pitch: usize,
    dest_width: i32,
    dest_height: i32,
    src: &[u8],
    src_pitch: usize,
    src_width: i32,
    src_height: i32,
    dest_x: i32,
    dest_y: i32,
    scale_width: i32,
    scale_height: i32,
) {
    if scale_width <= 0 || scale_height <= 0 || src_width <= 0 || src_height <= 0 {
        return;
    }

    // Fixed-point scaling factors (16.16)
    let x_ratio = ((src_width as u32) << 16) / scale_width as u32;
    let y_ratio = ((src_height as u32) << 16) / scale_height as u32;

    let mut y_accum: u32 = 0;

    for y in 0..scale_height {
        let dst_y = dest_y + y;

        if dst_y < 0 || dst_y >= dest_height {
            y_accum += y_ratio;
            continue;
        }

        let src_y = (y_accum >> 16) as usize;
        let dst_row_offset = (dst_y as usize) * dest_pitch;
        let src_row_offset = src_y * src_pitch;

        let mut x_accum: u32 = 0;

        for x in 0..scale_width {
            let dst_x = dest_x + x;

            if dst_x >= 0 && dst_x < dest_width {
                let src_x = (x_accum >> 16) as usize;
                let dst_idx = dst_row_offset + dst_x as usize;
                let src_idx = src_row_offset + src_x;

                if dst_idx < dest.len() && src_idx < src.len() {
                    dest[dst_idx] = src[src_idx];
                }
            }
            x_accum += x_ratio;
        }
        y_accum += y_ratio;
    }
}

/// Scale with transparency (skip color 0)
pub fn buffer_scale_trans(
    dest: &mut [u8],
    dest_pitch: usize,
    dest_width: i32,
    dest_height: i32,
    src: &[u8],
    src_pitch: usize,
    src_width: i32,
    src_height: i32,
    dest_x: i32,
    dest_y: i32,
    scale_width: i32,
    scale_height: i32,
) {
    if scale_width <= 0 || scale_height <= 0 || src_width <= 0 || src_height <= 0 {
        return;
    }

    let x_ratio = ((src_width as u32) << 16) / scale_width as u32;
    let y_ratio = ((src_height as u32) << 16) / scale_height as u32;

    let mut y_accum: u32 = 0;

    for y in 0..scale_height {
        let dst_y = dest_y + y;

        if dst_y < 0 || dst_y >= dest_height {
            y_accum += y_ratio;
            continue;
        }

        let src_y = (y_accum >> 16) as usize;
        let dst_row_offset = (dst_y as usize) * dest_pitch;
        let src_row_offset = src_y * src_pitch;

        let mut x_accum: u32 = 0;

        for x in 0..scale_width {
            let dst_x = dest_x + x;

            if dst_x >= 0 && dst_x < dest_width {
                let src_x = (x_accum >> 16) as usize;
                let dst_idx = dst_row_offset + dst_x as usize;
                let src_idx = src_row_offset + src_x;

                if dst_idx < dest.len() && src_idx < src.len() {
                    let pixel = src[src_idx];
                    if pixel != 0 {
                        dest[dst_idx] = pixel;
                    }
                }
            }
            x_accum += x_ratio;
        }
        y_accum += y_ratio;
    }
}

// =============================================================================
// FFI Exports
// =============================================================================

#[no_mangle]
pub unsafe extern "C" fn Platform_Buffer_Scale(
    dest: *mut u8,
    dest_pitch: i32,
    dest_width: i32,
    dest_height: i32,
    src: *const u8,
    src_pitch: i32,
    src_width: i32,
    src_height: i32,
    dest_x: i32,
    dest_y: i32,
    scale_width: i32,
    scale_height: i32,
    transparent: i32,
) -> i32 {
    crate::ffi::catch_panic(|| {
        if dest.is_null() || src.is_null() {
            return Err(crate::PlatformError::NullPointer);
        }

        let dest_len = (dest_pitch * dest_height) as usize;
        let src_len = (src_pitch * src_height) as usize;
        let dest_slice = std::slice::from_raw_parts_mut(dest, dest_len);
        let src_slice = std::slice::from_raw_parts(src, src_len);

        if transparent != 0 {
            buffer_scale_trans(
                dest_slice, dest_pitch as usize, dest_width, dest_height,
                src_slice, src_pitch as usize, src_width, src_height,
                dest_x, dest_y, scale_width, scale_height,
            );
        } else {
            buffer_scale(
                dest_slice, dest_pitch as usize, dest_width, dest_height,
                src_slice, src_pitch as usize, src_width, src_height,
                dest_x, dest_y, scale_width, scale_height,
            );
        }

        Ok(())
    })
}
```

### 9.4 Shadow Drawing

File: `platform/src/shadow.rs`
```rust
//! Shadow drawing (replaces SHADOW.ASM)

/// Draw shadow by darkening existing pixels
pub fn buffer_shadow(
    buffer: &mut [u8],
    pitch: usize,
    x: usize,
    y: usize,
    width: usize,
    height: usize,
    shadow_table: &[u8; 256],
) {
    for row in 0..height {
        let offset = (y + row) * pitch + x;
        for col in 0..width {
            let idx = offset + col;
            if idx < buffer.len() {
                buffer[idx] = shadow_table[buffer[idx] as usize];
            }
        }
    }
}

/// Draw shadow with mask (only darken where mask is non-zero)
pub fn buffer_shadow_mask(
    buffer: &mut [u8],
    buffer_pitch: usize,
    mask: &[u8],
    mask_pitch: usize,
    x: usize,
    y: usize,
    width: usize,
    height: usize,
    shadow_table: &[u8; 256],
) {
    for row in 0..height {
        let buf_offset = (y + row) * buffer_pitch + x;
        let mask_offset = row * mask_pitch;

        for col in 0..width {
            let mask_idx = mask_offset + col;
            let buf_idx = buf_offset + col;

            if mask_idx < mask.len() && buf_idx < buffer.len() {
                if mask[mask_idx] != 0 {
                    buffer[buf_idx] = shadow_table[buffer[buf_idx] as usize];
                }
            }
        }
    }
}

/// Generate standard shadow lookup table (darken by 50%)
pub fn generate_shadow_table(palette: &[[u8; 3]; 256]) -> [u8; 256] {
    let mut table = [0u8; 256];

    for i in 0..256 {
        // Find the closest darker color
        let target_r = palette[i][0] / 2;
        let target_g = palette[i][1] / 2;
        let target_b = palette[i][2] / 2;

        let mut best_idx = 0;
        let mut best_dist = u32::MAX;

        for j in 0..256 {
            let dr = (palette[j][0] as i32 - target_r as i32).abs() as u32;
            let dg = (palette[j][1] as i32 - target_g as i32).abs() as u32;
            let db = (palette[j][2] as i32 - target_b as i32).abs() as u32;
            let dist = dr * dr + dg * dg + db * db;

            if dist < best_dist {
                best_dist = dist;
                best_idx = j;
            }
        }

        table[i] = best_idx as u8;
    }

    table
}
```

### 9.5 LCW Compression/Decompression

File: `platform/src/lcw.rs`
```rust
//! LCW compression/decompression (replaces LCWCOMP.ASM, LCWUNCMP.ASM)
//!
//! LCW is a variant of LZSS compression used by Westwood Studios.

use crate::PlatformError;

/// Decompress LCW-compressed data
pub fn lcw_decompress(source: &[u8], dest: &mut [u8]) -> Result<usize, PlatformError> {
    let mut src_pos = 0;
    let mut dst_pos = 0;

    while src_pos < source.len() && dst_pos < dest.len() {
        let cmd = source[src_pos];
        src_pos += 1;

        if cmd & 0x80 != 0 {
            // Copy from source (literal bytes)
            let mut count = (cmd & 0x3F) as usize;

            if cmd & 0x40 != 0 {
                // Extended count
                if src_pos >= source.len() {
                    return Err(PlatformError::InvalidData);
                }
                count = (count << 8) | source[src_pos] as usize;
                src_pos += 1;
            }
            count += 1;

            // Copy literal bytes
            for _ in 0..count {
                if src_pos >= source.len() || dst_pos >= dest.len() {
                    break;
                }
                dest[dst_pos] = source[src_pos];
                dst_pos += 1;
                src_pos += 1;
            }
        } else if cmd == 0 {
            // End of data
            break;
        } else {
            // Copy from destination (back-reference)
            let count = ((cmd >> 4) + 3) as usize;

            if src_pos >= source.len() {
                return Err(PlatformError::InvalidData);
            }

            let offset = (((cmd & 0x0F) as usize) << 8) | source[src_pos] as usize;
            src_pos += 1;

            if offset > dst_pos {
                return Err(PlatformError::InvalidData);
            }

            let ref_pos = dst_pos - offset;

            // Copy from back-reference (handle overlapping)
            for i in 0..count {
                if dst_pos >= dest.len() {
                    break;
                }
                dest[dst_pos] = dest[ref_pos + i];
                dst_pos += 1;
            }
        }
    }

    Ok(dst_pos)
}

/// Compress data using LCW algorithm
///
/// Returns the compressed size, or error if destination is too small
pub fn lcw_compress(source: &[u8], dest: &mut [u8]) -> Result<usize, PlatformError> {
    // Simple implementation: mostly literal with basic back-references
    // A production implementation would use better matching

    let mut src_pos = 0;
    let mut dst_pos = 0;

    while src_pos < source.len() {
        // Try to find a back-reference
        let mut best_length = 0;
        let mut best_offset = 0;

        let search_start = src_pos.saturating_sub(4095);

        for ref_pos in search_start..src_pos {
            let mut length = 0;
            while src_pos + length < source.len()
                && length < 18  // Max match length for short form
                && source[ref_pos + length] == source[src_pos + length]
            {
                length += 1;
            }

            if length >= 3 && length > best_length {
                best_length = length;
                best_offset = src_pos - ref_pos;
            }
        }

        if best_length >= 3 {
            // Emit back-reference
            if dst_pos + 2 > dest.len() {
                return Err(PlatformError::BufferTooSmall);
            }

            let cmd = (((best_length - 3) & 0x0F) << 4) | ((best_offset >> 8) & 0x0F);
            dest[dst_pos] = cmd as u8;
            dest[dst_pos + 1] = (best_offset & 0xFF) as u8;
            dst_pos += 2;
            src_pos += best_length;
        } else {
            // Emit literal
            // Find how many literals to emit
            let mut literal_count = 1;
            while src_pos + literal_count < source.len() && literal_count < 63 {
                // Check if next position would benefit from back-reference
                // (simplified: just emit up to 63 literals)
                literal_count += 1;
            }

            if dst_pos + 1 + literal_count > dest.len() {
                return Err(PlatformError::BufferTooSmall);
            }

            dest[dst_pos] = 0x80 | ((literal_count - 1) as u8);
            dst_pos += 1;

            for _ in 0..literal_count {
                dest[dst_pos] = source[src_pos];
                dst_pos += 1;
                src_pos += 1;
            }
        }
    }

    // End marker
    if dst_pos >= dest.len() {
        return Err(PlatformError::BufferTooSmall);
    }
    dest[dst_pos] = 0;
    dst_pos += 1;

    Ok(dst_pos)
}

// =============================================================================
// FFI Exports
// =============================================================================

#[no_mangle]
pub unsafe extern "C" fn Platform_LCW_Uncompress(
    source: *const u8,
    source_size: i32,
    dest: *mut u8,
    dest_size: i32,
) -> i32 {
    if source.is_null() || dest.is_null() {
        return -1;
    }

    let src_slice = std::slice::from_raw_parts(source, source_size as usize);
    let dst_slice = std::slice::from_raw_parts_mut(dest, dest_size as usize);

    match lcw_decompress(src_slice, dst_slice) {
        Ok(size) => size as i32,
        Err(_) => -1,
    }
}

#[no_mangle]
pub unsafe extern "C" fn Platform_LCW_Compress(
    source: *const u8,
    source_size: i32,
    dest: *mut u8,
    dest_size: i32,
) -> i32 {
    if source.is_null() || dest.is_null() {
        return -1;
    }

    let src_slice = std::slice::from_raw_parts(source, source_size as usize);
    let dst_slice = std::slice::from_raw_parts_mut(dest, dest_size as usize);

    match lcw_compress(src_slice, dst_slice) {
        Ok(size) => size as i32,
        Err(_) => -1,
    }
}
```

### 9.6 CRC and Random

File: `platform/src/crc.rs`
```rust
//! CRC32 calculation (replaces CRC.ASM)

use once_cell::sync::Lazy;

/// Pre-computed CRC32 table
static CRC_TABLE: Lazy<[u32; 256]> = Lazy::new(|| {
    let mut table = [0u32; 256];

    for i in 0..256 {
        let mut crc = i as u32;
        for _ in 0..8 {
            if crc & 1 != 0 {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
        table[i] = crc;
    }

    table
});

/// Calculate CRC32 of a byte slice
pub fn crc32(data: &[u8]) -> u32 {
    let mut crc = 0xFFFFFFFF_u32;

    for &byte in data {
        let index = ((crc ^ byte as u32) & 0xFF) as usize;
        crc = CRC_TABLE[index] ^ (crc >> 8);
    }

    crc ^ 0xFFFFFFFF
}

/// Update existing CRC with more data
pub fn crc32_update(crc: u32, data: &[u8]) -> u32 {
    let mut crc = crc ^ 0xFFFFFFFF;

    for &byte in data {
        let index = ((crc ^ byte as u32) & 0xFF) as usize;
        crc = CRC_TABLE[index] ^ (crc >> 8);
    }

    crc ^ 0xFFFFFFFF
}

// =============================================================================
// FFI Exports
// =============================================================================

#[no_mangle]
pub unsafe extern "C" fn Platform_CRC32(data: *const u8, size: i32) -> u32 {
    if data.is_null() || size <= 0 {
        return 0;
    }

    let slice = std::slice::from_raw_parts(data, size as usize);
    crc32(slice)
}
```

File: `platform/src/random.rs`
```rust
//! Random number generation (replaces RANDOM.ASM)
//!
//! Uses Linear Congruential Generator for compatibility with original game.

use std::sync::atomic::{AtomicU32, Ordering};

/// Global random seed (thread-safe)
static RANDOM_SEED: AtomicU32 = AtomicU32::new(12345);

/// Seed the random number generator
pub fn seed(value: u32) {
    RANDOM_SEED.store(value, Ordering::SeqCst);
}

/// Get next random number (0-32767)
pub fn next() -> u32 {
    // LCG: same constants as original for compatibility
    let seed = RANDOM_SEED.load(Ordering::SeqCst);
    let new_seed = seed.wrapping_mul(1103515245).wrapping_add(12345);
    RANDOM_SEED.store(new_seed, Ordering::SeqCst);
    (new_seed >> 16) & 0x7FFF
}

/// Get random number in range [min, max]
pub fn range(min: i32, max: i32) -> i32 {
    if min >= max {
        return min;
    }
    min + (next() as i32 % (max - min + 1))
}

// =============================================================================
// FFI Exports
// =============================================================================

#[no_mangle]
pub extern "C" fn Platform_Random_Seed(seed_value: u32) {
    seed(seed_value);
}

#[no_mangle]
pub extern "C" fn Platform_Random_Get() -> u32 {
    next()
}

#[no_mangle]
pub extern "C" fn Platform_Random_Range(min: i32, max: i32) -> i32 {
    range(min, max)
}
```

### 9.7 SIMD Optimizations (Optional)

File: `platform/src/blit/simd.rs`
```rust
//! SIMD-optimized blitting routines
//!
//! Uses std::arch for platform-specific SIMD intrinsics.
//! Falls back to scalar code on unsupported platforms.

#[cfg(target_arch = "x86_64")]
use std::arch::x86_64::*;

#[cfg(target_arch = "aarch64")]
use std::arch::aarch64::*;

/// Clear buffer with SIMD (x86_64 SSE2)
#[cfg(target_arch = "x86_64")]
#[target_feature(enable = "sse2")]
pub unsafe fn buffer_clear_simd(buffer: &mut [u8], value: u8) {
    let fill = _mm_set1_epi8(value as i8);
    let mut ptr = buffer.as_mut_ptr();
    let mut remaining = buffer.len();

    // Align to 16 bytes
    while (ptr as usize & 15) != 0 && remaining > 0 {
        *ptr = value;
        ptr = ptr.add(1);
        remaining -= 1;
    }

    // Process 16 bytes at a time
    while remaining >= 16 {
        _mm_store_si128(ptr as *mut __m128i, fill);
        ptr = ptr.add(16);
        remaining -= 16;
    }

    // Handle remainder
    while remaining > 0 {
        *ptr = value;
        ptr = ptr.add(1);
        remaining -= 1;
    }
}

/// Clear buffer with SIMD (ARM NEON)
#[cfg(target_arch = "aarch64")]
#[target_feature(enable = "neon")]
pub unsafe fn buffer_clear_simd(buffer: &mut [u8], value: u8) {
    let fill = vdupq_n_u8(value);
    let mut ptr = buffer.as_mut_ptr();
    let mut remaining = buffer.len();

    // Align to 16 bytes
    while (ptr as usize & 15) != 0 && remaining > 0 {
        *ptr = value;
        ptr = ptr.add(1);
        remaining -= 1;
    }

    // Process 16 bytes at a time
    while remaining >= 16 {
        vst1q_u8(ptr, fill);
        ptr = ptr.add(16);
        remaining -= 16;
    }

    // Handle remainder
    while remaining > 0 {
        *ptr = value;
        ptr = ptr.add(1);
        remaining -= 1;
    }
}

/// Remap buffer with SIMD lookup (x86_64 SSSE3)
#[cfg(target_arch = "x86_64")]
#[target_feature(enable = "ssse3")]
pub unsafe fn buffer_remap_simd(
    buffer: &mut [u8],
    remap_table: &[u8; 256],
) {
    // PSHUFB-based lookup for 16 values at a time
    // Split table into 16 chunks of 16 bytes each for low nibble lookup
    let mut tables_lo: [__m128i; 16] = std::mem::zeroed();
    let mut tables_hi: [__m128i; 16] = std::mem::zeroed();

    for i in 0..16 {
        let mut lo_bytes = [0u8; 16];
        let mut hi_bytes = [0u8; 16];
        for j in 0..16 {
            lo_bytes[j] = remap_table[i * 16 + j];
            hi_bytes[j] = remap_table[i * 16 + j];
        }
        tables_lo[i] = _mm_loadu_si128(lo_bytes.as_ptr() as *const __m128i);
        tables_hi[i] = _mm_loadu_si128(hi_bytes.as_ptr() as *const __m128i);
    }

    // For simplicity, fall back to scalar for now
    // Full SIMD lookup table is complex
    for byte in buffer.iter_mut() {
        *byte = remap_table[*byte as usize];
    }
}

/// Dispatch to SIMD or scalar based on CPU support
pub fn buffer_clear_fast(buffer: &mut [u8], value: u8) {
    #[cfg(target_arch = "x86_64")]
    {
        if is_x86_feature_detected!("sse2") {
            unsafe { buffer_clear_simd(buffer, value); }
            return;
        }
    }

    #[cfg(target_arch = "aarch64")]
    {
        // NEON is always available on aarch64
        unsafe { buffer_clear_simd(buffer, value); }
        return;
    }

    // Scalar fallback
    buffer.fill(value);
}
```

## Tasks Breakdown

### Phase 1: Inventory (1 day)
- [ ] List all ASM files
- [ ] Identify which have Rust/C++ equivalents
- [ ] Determine which are actually used

### Phase 2: Core Graphics (3 days)
- [ ] Implement blit routines in Rust
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
- [ ] Verify compatibility with original

### Phase 6: Integration & Testing (2 days)
- [ ] Create FFI exports for all functions
- [ ] Generate platform.h with cbindgen
- [ ] Verify game renders correctly
- [ ] Performance benchmarking

### Phase 7: SIMD Optimization (Optional, 2 days)
- [ ] Profile to identify bottlenecks
- [ ] Implement SSE2/NEON versions
- [ ] Benchmark improvements

## Acceptance Criteria

- [ ] No assembly files in build
- [ ] Game renders correctly
- [ ] Shapes/sprites display correctly
- [ ] Compression/decompression works
- [ ] Performance acceptable (>30 FPS)
- [ ] Works on both Intel and Apple Silicon

## Estimated Duration
**8-10 days** (plus 2 optional for SIMD)

## Dependencies
- Plans 01-08 completed
- Game running with stubs

## Next Plan
Once assembly is replaced, proceed to **Plan 10: Networking**
