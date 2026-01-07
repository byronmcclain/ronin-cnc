# Task 09d: Buffer Scaling

## Dependencies
- Task 09a must be complete (basic buffer operations, ClipRect)
- `platform/src/blit/mod.rs` structure in place

## Context
The original WIN32LIB/DRAWBUFF/SCALE.ASM implements sprite scaling using fixed-point arithmetic. This is used for zoom effects, distance-based sprite sizing, and UI scaling. The algorithm uses nearest-neighbor interpolation which matches the original game's look.

## Objective
Create scaling functions:
1. `buffer_scale()` - Scale source to destination (opaque)
2. `buffer_scale_trans()` - Scale with transparency (skip color 0)
3. Fixed-point arithmetic for sub-pixel accuracy
4. Proper clipping at destination bounds
5. FFI exports and comprehensive tests

## Deliverables
- [ ] `platform/src/blit/scale.rs` - Scaling operations
- [ ] FFI exports for scale functions
- [ ] Unit tests covering various scale factors and edge cases
- [ ] Header contains FFI exports
- [ ] Verification passes

## Technical Background

### Fixed-Point Arithmetic
Scaling uses 16.16 fixed-point numbers for sub-pixel accuracy:
- High 16 bits: integer part (source coordinate)
- Low 16 bits: fractional part (interpolation, discarded for nearest-neighbor)

Example: To scale a 32-pixel source to 64-pixel destination:
- x_ratio = (32 << 16) / 64 = 0x8000 (0.5 in fixed-point)
- Each dest pixel advances source by 0.5 pixels

### Nearest Neighbor
Simply take the pixel at floor(source_coord). No blending or interpolation.
This matches the original game's aesthetic.

### Clipping Complexity
Scaling with clipping is complex because:
1. Dest clip affects how many pixels we draw
2. Source starting position must be calculated from the clipped dest position
3. The accumulator must be initialized correctly for the first visible pixel

### Scale Direction
- scale > 1.0: Magnification (dest larger than source)
- scale < 1.0: Minification (dest smaller than source)
- scale = 1.0: 1:1 copy (but scale functions still work)

## Files to Create/Modify

### platform/src/blit/scale.rs (NEW)
```rust
//! Buffer scaling operations
//!
//! Replaces WIN32LIB/DRAWBUFF/SCALE.ASM
//!
//! Uses nearest-neighbor interpolation with 16.16 fixed-point arithmetic
//! for sub-pixel accuracy.

use super::ClipRect;

/// Fixed-point shift amount (16.16 format)
const FP_SHIFT: u32 = 16;
const FP_ONE: u32 = 1 << FP_SHIFT;

/// Scale source to destination using nearest-neighbor interpolation
///
/// # Arguments
/// * `dest` - Destination buffer
/// * `dest_pitch` - Bytes per row in destination
/// * `dest_width` - Destination buffer width
/// * `dest_height` - Destination buffer height
/// * `src` - Source buffer
/// * `src_pitch` - Bytes per row in source
/// * `src_width` - Source image width
/// * `src_height` - Source image height
/// * `dest_x` - X position in destination
/// * `dest_y` - Y position in destination
/// * `scale_width` - Desired width after scaling
/// * `scale_height` - Desired height after scaling
///
/// # Example
/// ```ignore
/// // Scale 32x32 sprite to 64x64
/// buffer_scale(
///     &mut dest, 640, 640, 400,
///     &src, 32, 32, 32,
///     100, 100,  // dest position
///     64, 64,    // scaled size
/// );
/// ```
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
    // Validate inputs
    if scale_width <= 0 || scale_height <= 0 {
        return;
    }
    if src_width <= 0 || src_height <= 0 {
        return;
    }
    if dest_width <= 0 || dest_height <= 0 {
        return;
    }

    // Calculate fixed-point scaling ratios
    // x_ratio = (src_width << 16) / scale_width
    let x_ratio = ((src_width as u32) << FP_SHIFT) / scale_width as u32;
    let y_ratio = ((src_height as u32) << FP_SHIFT) / scale_height as u32;

    // Calculate clipping
    let clip_result = clip_scaled_rect(
        dest_x, dest_y,
        scale_width, scale_height,
        dest_width, dest_height,
        x_ratio, y_ratio,
    );

    let Some((clip_dest_x, clip_dest_y, clip_width, clip_height, start_x_fp, start_y_fp)) = clip_result else {
        return;  // Completely clipped
    };

    // Scale each row
    let mut y_accum = start_y_fp;

    for dy in 0..clip_height {
        let dest_row_y = clip_dest_y + dy;
        if dest_row_y < 0 || dest_row_y >= dest_height {
            y_accum += y_ratio;
            continue;
        }

        let src_y = (y_accum >> FP_SHIFT) as i32;
        if src_y < 0 || src_y >= src_height {
            y_accum += y_ratio;
            continue;
        }

        let dest_row_offset = (dest_row_y as usize) * dest_pitch;
        let src_row_offset = (src_y as usize) * src_pitch;

        let mut x_accum = start_x_fp;

        for dx in 0..clip_width {
            let dest_col_x = clip_dest_x + dx;
            if dest_col_x >= 0 && dest_col_x < dest_width {
                let src_x = (x_accum >> FP_SHIFT) as i32;
                if src_x >= 0 && src_x < src_width {
                    let dest_idx = dest_row_offset + dest_col_x as usize;
                    let src_idx = src_row_offset + src_x as usize;

                    if dest_idx < dest.len() && src_idx < src.len() {
                        dest[dest_idx] = src[src_idx];
                    }
                }
            }
            x_accum += x_ratio;
        }

        y_accum += y_ratio;
    }
}

/// Scale with transparency (skip color 0)
///
/// Like `buffer_scale` but pixels with value 0 in the source are not copied.
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
    buffer_scale_trans_key(
        dest, dest_pitch, dest_width, dest_height,
        src, src_pitch, src_width, src_height,
        dest_x, dest_y, scale_width, scale_height,
        0,  // Color 0 is transparent
    );
}

/// Scale with custom transparent color key
///
/// Like `buffer_scale_trans` but allows specifying which color is transparent.
pub fn buffer_scale_trans_key(
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
    transparent_color: u8,
) {
    // Validate inputs
    if scale_width <= 0 || scale_height <= 0 {
        return;
    }
    if src_width <= 0 || src_height <= 0 {
        return;
    }
    if dest_width <= 0 || dest_height <= 0 {
        return;
    }

    let x_ratio = ((src_width as u32) << FP_SHIFT) / scale_width as u32;
    let y_ratio = ((src_height as u32) << FP_SHIFT) / scale_height as u32;

    let clip_result = clip_scaled_rect(
        dest_x, dest_y,
        scale_width, scale_height,
        dest_width, dest_height,
        x_ratio, y_ratio,
    );

    let Some((clip_dest_x, clip_dest_y, clip_width, clip_height, start_x_fp, start_y_fp)) = clip_result else {
        return;
    };

    let mut y_accum = start_y_fp;

    for dy in 0..clip_height {
        let dest_row_y = clip_dest_y + dy;
        if dest_row_y < 0 || dest_row_y >= dest_height {
            y_accum += y_ratio;
            continue;
        }

        let src_y = (y_accum >> FP_SHIFT) as i32;
        if src_y < 0 || src_y >= src_height {
            y_accum += y_ratio;
            continue;
        }

        let dest_row_offset = (dest_row_y as usize) * dest_pitch;
        let src_row_offset = (src_y as usize) * src_pitch;

        let mut x_accum = start_x_fp;

        for dx in 0..clip_width {
            let dest_col_x = clip_dest_x + dx;
            if dest_col_x >= 0 && dest_col_x < dest_width {
                let src_x = (x_accum >> FP_SHIFT) as i32;
                if src_x >= 0 && src_x < src_width {
                    let dest_idx = dest_row_offset + dest_col_x as usize;
                    let src_idx = src_row_offset + src_x as usize;

                    if dest_idx < dest.len() && src_idx < src.len() {
                        let pixel = src[src_idx];
                        if pixel != transparent_color {
                            dest[dest_idx] = pixel;
                        }
                    }
                }
            }
            x_accum += x_ratio;
        }

        y_accum += y_ratio;
    }
}

/// Calculate clipping for scaled rectangle
///
/// Returns: (clip_dest_x, clip_dest_y, clip_width, clip_height, start_x_fp, start_y_fp)
/// - clip_dest_x/y: Where to start drawing in destination
/// - clip_width/height: How many pixels to draw
/// - start_x/y_fp: Fixed-point accumulator starting values
fn clip_scaled_rect(
    dest_x: i32,
    dest_y: i32,
    scale_width: i32,
    scale_height: i32,
    dest_width: i32,
    dest_height: i32,
    x_ratio: u32,
    y_ratio: u32,
) -> Option<(i32, i32, i32, i32, u32, u32)> {
    let mut clip_dest_x = dest_x;
    let mut clip_dest_y = dest_y;
    let mut clip_width = scale_width;
    let mut clip_height = scale_height;
    let mut start_x_fp: u32 = 0;
    let mut start_y_fp: u32 = 0;

    // Clip left edge
    if clip_dest_x < 0 {
        let clip_amount = -clip_dest_x;
        start_x_fp = (clip_amount as u32) * x_ratio;
        clip_width -= clip_amount;
        clip_dest_x = 0;
    }

    // Clip top edge
    if clip_dest_y < 0 {
        let clip_amount = -clip_dest_y;
        start_y_fp = (clip_amount as u32) * y_ratio;
        clip_height -= clip_amount;
        clip_dest_y = 0;
    }

    // Clip right edge
    if clip_dest_x + clip_width > dest_width {
        clip_width = dest_width - clip_dest_x;
    }

    // Clip bottom edge
    if clip_dest_y + clip_height > dest_height {
        clip_height = dest_height - clip_dest_y;
    }

    // Check if anything remains
    if clip_width <= 0 || clip_height <= 0 {
        return None;
    }

    Some((clip_dest_x, clip_dest_y, clip_width, clip_height, start_x_fp, start_y_fp))
}

/// Calculate the scale factor needed to fit source into destination size
pub fn calc_scale_to_fit(
    src_width: i32,
    src_height: i32,
    max_width: i32,
    max_height: i32,
) -> (i32, i32) {
    if src_width <= 0 || src_height <= 0 {
        return (0, 0);
    }

    // Calculate scale factor to fit
    let x_scale = (max_width * 1000) / src_width;
    let y_scale = (max_height * 1000) / src_height;
    let scale = x_scale.min(y_scale);

    let scaled_width = (src_width * scale) / 1000;
    let scaled_height = (src_height * scale) / 1000;

    (scaled_width, scaled_height)
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_scale_1_to_1() {
        // 4x4 source scaled to 4x4 (1:1)
        let src = vec![
            1, 2, 3, 4,
            5, 6, 7, 8,
            9, 10, 11, 12,
            13, 14, 15, 16,
        ];
        let mut dest = vec![0u8; 64];  // 8x8

        buffer_scale(
            &mut dest, 8, 8, 8,
            &src, 4, 4, 4,
            2, 2,  // dest position
            4, 4,  // scale to 4x4 (1:1)
        );

        // Check that pixels were copied correctly
        for y in 0..4 {
            for x in 0..4 {
                let expected = src[y * 4 + x];
                let actual = dest[(y + 2) * 8 + (x + 2)];
                assert_eq!(actual, expected, "at ({}, {})", x, y);
            }
        }
    }

    #[test]
    fn test_scale_2x() {
        // 2x2 source scaled to 4x4 (2x magnification)
        let src = vec![1, 2, 3, 4];  // 2x2: [1,2] [3,4]
        let mut dest = vec![0u8; 64];  // 8x8

        buffer_scale(
            &mut dest, 8, 8, 8,
            &src, 2, 2, 2,
            0, 0,
            4, 4,  // Scale to 4x4
        );

        // Expected: each source pixel becomes 2x2 block
        // [1,1,2,2]
        // [1,1,2,2]
        // [3,3,4,4]
        // [3,3,4,4]
        let expected_rows = [
            [1, 1, 2, 2],
            [1, 1, 2, 2],
            [3, 3, 4, 4],
            [3, 3, 4, 4],
        ];

        for y in 0..4 {
            for x in 0..4 {
                assert_eq!(
                    dest[y * 8 + x],
                    expected_rows[y][x],
                    "at ({}, {})", x, y
                );
            }
        }
    }

    #[test]
    fn test_scale_0_5x() {
        // 4x4 source scaled to 2x2 (0.5x minification)
        let src = vec![
            1, 2, 3, 4,
            5, 6, 7, 8,
            9, 10, 11, 12,
            13, 14, 15, 16,
        ];
        let mut dest = vec![0u8; 16];  // 4x4

        buffer_scale(
            &mut dest, 4, 4, 4,
            &src, 4, 4, 4,
            1, 1,
            2, 2,  // Scale to 2x2
        );

        // Nearest neighbor samples every other pixel
        // Source (0,0), (2,0), (0,2), (2,2) -> 1, 3, 9, 11
        assert_eq!(dest[1 * 4 + 1], 1);   // (1,1) <- src(0,0)
        assert_eq!(dest[1 * 4 + 2], 3);   // (2,1) <- src(2,0)
        assert_eq!(dest[2 * 4 + 1], 9);   // (1,2) <- src(0,2)
        assert_eq!(dest[2 * 4 + 2], 11);  // (2,2) <- src(2,2)
    }

    #[test]
    fn test_scale_clip_left() {
        let src = vec![1u8; 16];  // 4x4 filled with 1
        let mut dest = vec![0u8; 16];  // 4x4

        buffer_scale(
            &mut dest, 4, 4, 4,
            &src, 4, 4, 4,
            -2, 0,  // 2 pixels off left edge
            4, 4,   // Full 4x4 scale
        );

        // Only right 2 columns should be filled
        for y in 0..4 {
            for x in 0..4 {
                let expected = if x < 2 { 1 } else { 0 };
                assert_eq!(dest[y * 4 + x], expected, "at ({}, {})", x, y);
            }
        }
    }

    #[test]
    fn test_scale_clip_right() {
        let src = vec![1u8; 16];
        let mut dest = vec![0u8; 16];

        buffer_scale(
            &mut dest, 4, 4, 4,
            &src, 4, 4, 4,
            2, 0,   // Starts at 2, extends to 6 (2 pixels clipped)
            4, 4,
        );

        // Only columns 2-3 should be filled
        for y in 0..4 {
            for x in 0..4 {
                let expected = if x >= 2 { 1 } else { 0 };
                assert_eq!(dest[y * 4 + x], expected, "at ({}, {})", x, y);
            }
        }
    }

    #[test]
    fn test_scale_completely_outside() {
        let src = vec![1u8; 16];
        let mut dest = vec![0u8; 16];

        buffer_scale(
            &mut dest, 4, 4, 4,
            &src, 4, 4, 4,
            10, 10,  // Completely outside
            4, 4,
        );

        // Dest should be unchanged
        assert!(dest.iter().all(|&b| b == 0));
    }

    #[test]
    fn test_scale_trans() {
        // Source with transparent (0) and opaque (1) pixels
        let src = vec![
            0, 1, 0, 1,
            1, 0, 1, 0,
            0, 1, 0, 1,
            1, 0, 1, 0,
        ];
        let mut dest = vec![0xFFu8; 64];  // 8x8 filled with 0xFF

        buffer_scale_trans(
            &mut dest, 8, 8, 8,
            &src, 4, 4, 4,
            0, 0,
            4, 4,  // 1:1 scale
        );

        // Where source is 0, dest should remain 0xFF
        // Where source is 1, dest should be 1
        for y in 0..4 {
            for x in 0..4 {
                let src_pixel = src[y * 4 + x];
                let expected = if src_pixel == 0 { 0xFF } else { 1 };
                assert_eq!(dest[y * 8 + x], expected, "at ({}, {})", x, y);
            }
        }
    }

    #[test]
    fn test_scale_trans_with_magnification() {
        // 2x2 source with transparency
        let src = vec![0, 1, 1, 0];  // [0,1] [1,0]
        let mut dest = vec![0xFFu8; 16];  // 4x4

        buffer_scale_trans(
            &mut dest, 4, 4, 4,
            &src, 2, 2, 2,
            0, 0,
            4, 4,  // 2x magnification
        );

        // Each pixel becomes 2x2
        // [0xFF,0xFF, 1, 1]
        // [0xFF,0xFF, 1, 1]
        // [  1,   1,0xFF,0xFF]
        // [  1,   1,0xFF,0xFF]
        let expected = [
            0xFF, 0xFF, 1, 1,
            0xFF, 0xFF, 1, 1,
            1, 1, 0xFF, 0xFF,
            1, 1, 0xFF, 0xFF,
        ];

        assert_eq!(dest, expected);
    }

    #[test]
    fn test_scale_zero_dimensions() {
        let src = vec![1u8; 16];
        let mut dest = vec![0u8; 16];

        // Zero scale dimensions - should be no-op
        buffer_scale(
            &mut dest, 4, 4, 4,
            &src, 4, 4, 4,
            0, 0,
            0, 0,
        );
        assert!(dest.iter().all(|&b| b == 0));

        buffer_scale(
            &mut dest, 4, 4, 4,
            &src, 4, 4, 4,
            0, 0,
            -4, 4,
        );
        assert!(dest.iter().all(|&b| b == 0));
    }

    #[test]
    fn test_calc_scale_to_fit() {
        // 100x50 into 200x200 -> scale is 2x, result is 200x100
        let (w, h) = calc_scale_to_fit(100, 50, 200, 200);
        assert_eq!(w, 200);
        assert_eq!(h, 100);

        // 50x100 into 200x200 -> scale is 2x, result is 100x200
        let (w, h) = calc_scale_to_fit(50, 100, 200, 200);
        assert_eq!(w, 100);
        assert_eq!(h, 200);

        // 100x100 into 50x50 -> scale is 0.5x, result is 50x50
        let (w, h) = calc_scale_to_fit(100, 100, 50, 50);
        assert_eq!(w, 50);
        assert_eq!(h, 50);
    }

    #[test]
    fn test_non_uniform_scale() {
        // 2x2 source scaled to 4x2 (stretch horizontal only)
        let src = vec![1, 2, 3, 4];
        let mut dest = vec![0u8; 16];  // 4x4

        buffer_scale(
            &mut dest, 4, 4, 4,
            &src, 2, 2, 2,
            0, 0,
            4, 2,  // 4 wide, 2 tall
        );

        // Row 0: 1,1,2,2
        // Row 1: 3,3,4,4
        assert_eq!(&dest[0..4], &[1, 1, 2, 2]);
        assert_eq!(&dest[4..8], &[3, 3, 4, 4]);
    }
}
```

### platform/src/blit/mod.rs (MODIFY)
Add after existing module declarations:
```rust
pub mod scale;
```

### platform/src/ffi/mod.rs (MODIFY)
Add scale FFI exports:
```rust
// =============================================================================
// Buffer Scale FFI (replaces SCALE.ASM)
// =============================================================================

/// Scale buffer using nearest-neighbor interpolation
///
/// # Safety
/// - `dest` must point to valid memory of at least `dest_pitch * dest_height` bytes
/// - `src` must point to valid memory of at least `src_pitch * src_height` bytes
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
) {
    if dest.is_null() || src.is_null() {
        return;
    }
    if dest_pitch <= 0 || src_pitch <= 0 {
        return;
    }
    if dest_height <= 0 || src_height <= 0 {
        return;
    }

    let dest_size = (dest_pitch * dest_height) as usize;
    let src_size = (src_pitch * src_height) as usize;
    let dest_slice = std::slice::from_raw_parts_mut(dest, dest_size);
    let src_slice = std::slice::from_raw_parts(src, src_size);

    blit::scale::buffer_scale(
        dest_slice, dest_pitch as usize, dest_width, dest_height,
        src_slice, src_pitch as usize, src_width, src_height,
        dest_x, dest_y, scale_width, scale_height,
    );
}

/// Scale buffer with transparency (skip color 0)
///
/// # Safety
/// Same as Platform_Buffer_Scale
#[no_mangle]
pub unsafe extern "C" fn Platform_Buffer_ScaleTrans(
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
) {
    if dest.is_null() || src.is_null() {
        return;
    }
    if dest_pitch <= 0 || src_pitch <= 0 {
        return;
    }
    if dest_height <= 0 || src_height <= 0 {
        return;
    }

    let dest_size = (dest_pitch * dest_height) as usize;
    let src_size = (src_pitch * src_height) as usize;
    let dest_slice = std::slice::from_raw_parts_mut(dest, dest_size);
    let src_slice = std::slice::from_raw_parts(src, src_size);

    blit::scale::buffer_scale_trans(
        dest_slice, dest_pitch as usize, dest_width, dest_height,
        src_slice, src_pitch as usize, src_width, src_height,
        dest_x, dest_y, scale_width, scale_height,
    );
}

/// Scale buffer with custom transparent color
///
/// # Safety
/// Same as Platform_Buffer_Scale
#[no_mangle]
pub unsafe extern "C" fn Platform_Buffer_ScaleTransKey(
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
    transparent_color: u8,
) {
    if dest.is_null() || src.is_null() {
        return;
    }
    if dest_pitch <= 0 || src_pitch <= 0 {
        return;
    }
    if dest_height <= 0 || src_height <= 0 {
        return;
    }

    let dest_size = (dest_pitch * dest_height) as usize;
    let src_size = (src_pitch * src_height) as usize;
    let dest_slice = std::slice::from_raw_parts_mut(dest, dest_size);
    let src_slice = std::slice::from_raw_parts(src, src_size);

    blit::scale::buffer_scale_trans_key(
        dest_slice, dest_pitch as usize, dest_width, dest_height,
        src_slice, src_pitch as usize, src_width, src_height,
        dest_x, dest_y, scale_width, scale_height,
        transparent_color,
    );
}
```

## Verification Command
```bash
ralph-tasks/verify/check-buffer-scale.sh
```

## Implementation Steps
1. Create `platform/src/blit/scale.rs` with all scale functions
2. Add `pub mod scale;` to `platform/src/blit/mod.rs`
3. Add FFI exports to `platform/src/ffi/mod.rs`
4. Run `cargo test --manifest-path platform/Cargo.toml -- scale` to verify unit tests
5. Run `cargo build --manifest-path platform/Cargo.toml --features cbindgen-run`
6. Verify header contains scale FFI exports
7. Run verification script

## Success Criteria
- All unit tests pass
- 1:1 scale produces identical copy
- 2x scale produces correct magnification
- 0.5x scale produces correct minification
- Clipping works for all edges
- Transparency variants preserve background
- Fixed-point math doesn't overflow or produce artifacts
- FFI exports appear in platform.h

## Common Issues

### Scaling produces shifted/offset image
- Check fixed-point accumulator initialization after clipping
- Verify x_ratio and y_ratio calculation

### Off-by-one in scaled dimensions
- Integer division truncates; ensure consistent behavior
- Test with exact power-of-2 scales first

### Clipping breaks at certain positions
- Trace through clip_scaled_rect with specific values
- Verify start_x_fp and start_y_fp account for clipped pixels

## Completion Promise
When verification passes, output:
```
<promise>TASK_09D_COMPLETE</promise>
```

## Escape Hatch
If stuck after 15 iterations:
- Document what's blocking in `ralph-tasks/blocked/09d.md`
- List attempted approaches
- Output: `<promise>TASK_09D_BLOCKED</promise>`

## Max Iterations
15
