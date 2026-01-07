//! Buffer-to-buffer copy operations
//!
//! Replaces WIN32LIB/DRAWBUFF/BITBLIT.ASM

use super::ClipRect;

/// Copy rectangular region from source to destination (opaque)
///
/// All pixels are copied regardless of value. This is the fastest blit
/// operation and should be used when transparency is not needed.
///
/// # Arguments
/// * `dest` - Destination buffer
/// * `dest_pitch` - Bytes per row in destination
/// * `dest_width` - Destination buffer width
/// * `dest_height` - Destination buffer height
/// * `src` - Source buffer
/// * `src_pitch` - Bytes per row in source
/// * `src_width` - Source buffer width (for bounds checking)
/// * `src_height` - Source buffer height (for bounds checking)
/// * `dest_x` - X position in destination
/// * `dest_y` - Y position in destination
/// * `src_rect` - Rectangle in source to copy
pub fn buffer_to_buffer(
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
    src_rect: ClipRect,
) {
    // Validate inputs
    if src_rect.is_empty() {
        return;
    }

    // Clip source rect to source bounds first
    let src_bounds = ClipRect::new(0, 0, src_width, src_height);
    let Some(src_clipped) = src_rect.intersect(&src_bounds) else {
        return;
    };

    // Adjust dest position if source was clipped at top/left
    let adjusted_dest_x = dest_x + (src_clipped.x - src_rect.x);
    let adjusted_dest_y = dest_y + (src_clipped.y - src_rect.y);

    // Now clip to destination bounds
    let Some((final_src, final_dx, final_dy)) = src_clipped.clip_to_bounds(
        adjusted_dest_x,
        adjusted_dest_y,
        dest_width,
        dest_height,
    ) else {
        return;
    };

    let copy_width = final_src.width as usize;
    let copy_height = final_src.height as usize;
    let src_start_x = final_src.x as usize;
    let src_start_y = final_src.y as usize;
    let dest_start_x = final_dx as usize;
    let dest_start_y = final_dy as usize;

    // Copy row by row
    for row in 0..copy_height {
        let src_offset = (src_start_y + row) * src_pitch + src_start_x;
        let dest_offset = (dest_start_y + row) * dest_pitch + dest_start_x;

        // Bounds check (should pass after clipping, but be defensive)
        if src_offset + copy_width <= src.len() && dest_offset + copy_width <= dest.len() {
            dest[dest_offset..dest_offset + copy_width]
                .copy_from_slice(&src[src_offset..src_offset + copy_width]);
        }
    }
}

/// Copy with transparency (skip color 0)
///
/// Pixels with value 0 in the source are not copied, allowing the destination
/// to show through. This is the standard transparency mode for sprites.
///
/// # Arguments
/// Same as `buffer_to_buffer`
pub fn buffer_to_buffer_trans(
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
    src_rect: ClipRect,
) {
    buffer_to_buffer_trans_key(
        dest, dest_pitch, dest_width, dest_height,
        src, src_pitch, src_width, src_height,
        dest_x, dest_y, src_rect,
        0,  // Color 0 is transparent
    );
}

/// Copy with custom transparent color key
///
/// Like `buffer_to_buffer_trans` but allows specifying which color value
/// should be treated as transparent.
///
/// # Arguments
/// * All arguments from `buffer_to_buffer`
/// * `transparent_color` - Pixel value to skip during copy
pub fn buffer_to_buffer_trans_key(
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
    src_rect: ClipRect,
    transparent_color: u8,
) {
    // Validate inputs
    if src_rect.is_empty() {
        return;
    }

    // Clip source rect to source bounds first
    let src_bounds = ClipRect::new(0, 0, src_width, src_height);
    let Some(src_clipped) = src_rect.intersect(&src_bounds) else {
        return;
    };

    // Adjust dest position if source was clipped at top/left
    let adjusted_dest_x = dest_x + (src_clipped.x - src_rect.x);
    let adjusted_dest_y = dest_y + (src_clipped.y - src_rect.y);

    // Now clip to destination bounds
    let Some((final_src, final_dx, final_dy)) = src_clipped.clip_to_bounds(
        adjusted_dest_x,
        adjusted_dest_y,
        dest_width,
        dest_height,
    ) else {
        return;
    };

    let copy_width = final_src.width as usize;
    let copy_height = final_src.height as usize;
    let src_start_x = final_src.x as usize;
    let src_start_y = final_src.y as usize;
    let dest_start_x = final_dx as usize;
    let dest_start_y = final_dy as usize;

    // Copy pixel by pixel, skipping transparent
    for row in 0..copy_height {
        let src_row_start = (src_start_y + row) * src_pitch + src_start_x;
        let dest_row_start = (dest_start_y + row) * dest_pitch + dest_start_x;

        for col in 0..copy_width {
            let src_idx = src_row_start + col;
            let dest_idx = dest_row_start + col;

            // Bounds check
            if src_idx < src.len() && dest_idx < dest.len() {
                let pixel = src[src_idx];
                if pixel != transparent_color {
                    dest[dest_idx] = pixel;
                }
            }
        }
    }
}

/// Copy from source to destination with separate source coordinates
///
/// Convenience function when source rect is specified as x, y, width, height
/// instead of ClipRect.
#[inline]
pub fn blit(
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
    src_x: i32,
    src_y: i32,
    width: i32,
    height: i32,
    transparent: bool,
) {
    let src_rect = ClipRect::new(src_x, src_y, width, height);

    if transparent {
        buffer_to_buffer_trans(
            dest, dest_pitch, dest_width, dest_height,
            src, src_pitch, src_width, src_height,
            dest_x, dest_y, src_rect,
        );
    } else {
        buffer_to_buffer(
            dest, dest_pitch, dest_width, dest_height,
            src, src_pitch, src_width, src_height,
            dest_x, dest_y, src_rect,
        );
    }
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    fn create_test_source() -> Vec<u8> {
        // 10x10 source with a pattern
        let mut src = vec![0u8; 100];
        for y in 0..10 {
            for x in 0..10 {
                // Checkerboard pattern: 0 and 1
                src[y * 10 + x] = ((x + y) % 2) as u8;
            }
        }
        src
    }

    fn create_gradient_source() -> Vec<u8> {
        // 10x10 source with gradient (no zeros except at 0,0)
        let mut src = vec![0u8; 100];
        for y in 0..10 {
            for x in 0..10 {
                src[y * 10 + x] = ((y * 10 + x) % 255 + 1) as u8;
            }
        }
        src[0] = 0;  // Just one transparent pixel
        src
    }

    #[test]
    fn test_buffer_to_buffer_basic() {
        let src = vec![0xFFu8; 25];  // 5x5 filled
        let mut dest = vec![0u8; 100];  // 10x10 empty

        let src_rect = ClipRect::new(0, 0, 5, 5);
        buffer_to_buffer(
            &mut dest, 10, 10, 10,
            &src, 5, 5, 5,
            2, 2, src_rect,
        );

        // Check that 5x5 region at (2,2) is filled
        for y in 0..10 {
            for x in 0..10 {
                let expected = if x >= 2 && x < 7 && y >= 2 && y < 7 { 0xFF } else { 0 };
                assert_eq!(dest[y * 10 + x], expected, "at ({}, {})", x, y);
            }
        }
    }

    #[test]
    fn test_buffer_to_buffer_partial_source() {
        let src = vec![0xFFu8; 100];  // 10x10 filled
        let mut dest = vec![0u8; 100];

        // Copy only a 3x3 region from middle of source
        let src_rect = ClipRect::new(3, 3, 3, 3);
        buffer_to_buffer(
            &mut dest, 10, 10, 10,
            &src, 10, 10, 10,
            0, 0, src_rect,
        );

        // Check that only 3x3 at origin is filled
        for y in 0..10 {
            for x in 0..10 {
                let expected = if x < 3 && y < 3 { 0xFF } else { 0 };
                assert_eq!(dest[y * 10 + x], expected, "at ({}, {})", x, y);
            }
        }
    }

    #[test]
    fn test_buffer_to_buffer_clip_left() {
        let src = vec![0xFFu8; 25];  // 5x5
        let mut dest = vec![0u8; 100];  // 10x10

        let src_rect = ClipRect::new(0, 0, 5, 5);
        buffer_to_buffer(
            &mut dest, 10, 10, 10,
            &src, 5, 5, 5,
            -2, 0, src_rect,  // 2 pixels off left edge
        );

        // Should copy 3 pixels wide (5 - 2 clipped)
        for y in 0..10 {
            for x in 0..10 {
                let expected = if x < 3 && y < 5 { 0xFF } else { 0 };
                assert_eq!(dest[y * 10 + x], expected, "at ({}, {})", x, y);
            }
        }
    }

    #[test]
    fn test_buffer_to_buffer_clip_all_edges() {
        let src = vec![0xFFu8; 100];  // 10x10
        let mut dest = vec![0u8; 25];  // 5x5

        let src_rect = ClipRect::new(0, 0, 10, 10);
        buffer_to_buffer(
            &mut dest, 5, 5, 5,
            &src, 10, 10, 10,
            -2, -2, src_rect,  // Extends 2 pixels off all edges
        );

        // Entire dest should be filled (clipped to 5x5)
        assert!(dest.iter().all(|&b| b == 0xFF));
    }

    #[test]
    fn test_buffer_to_buffer_completely_outside() {
        let src = vec![0xFFu8; 25];
        let mut dest = vec![0u8; 100];

        let src_rect = ClipRect::new(0, 0, 5, 5);
        buffer_to_buffer(
            &mut dest, 10, 10, 10,
            &src, 5, 5, 5,
            20, 20, src_rect,  // Completely outside
        );

        // Dest should be unchanged
        assert!(dest.iter().all(|&b| b == 0));
    }

    #[test]
    fn test_buffer_to_buffer_trans_basic() {
        let src = create_test_source();  // Checkerboard of 0 and 1
        let mut dest = vec![0xFFu8; 100];  // 10x10 filled with 0xFF

        let src_rect = ClipRect::new(0, 0, 10, 10);
        buffer_to_buffer_trans(
            &mut dest, 10, 10, 10,
            &src, 10, 10, 10,
            0, 0, src_rect,
        );

        // Where source is 0 (transparent), dest should remain 0xFF
        // Where source is 1, dest should be 1
        for y in 0..10 {
            for x in 0..10 {
                let src_pixel = src[y * 10 + x];
                let expected = if src_pixel == 0 { 0xFF } else { src_pixel };
                assert_eq!(dest[y * 10 + x], expected, "at ({}, {})", x, y);
            }
        }
    }

    #[test]
    fn test_buffer_to_buffer_trans_key() {
        // Source with values 1, 2, 3 where 2 is transparent
        let mut src = vec![0u8; 9];
        src[0] = 1; src[1] = 2; src[2] = 3;
        src[3] = 2; src[4] = 1; src[5] = 2;
        src[6] = 3; src[7] = 2; src[8] = 1;

        let mut dest = vec![0xFFu8; 9];

        let src_rect = ClipRect::new(0, 0, 3, 3);
        buffer_to_buffer_trans_key(
            &mut dest, 3, 3, 3,
            &src, 3, 3, 3,
            0, 0, src_rect,
            2,  // 2 is transparent
        );

        let expected = [1, 0xFF, 3, 0xFF, 1, 0xFF, 3, 0xFF, 1];
        assert_eq!(dest, expected);
    }

    #[test]
    fn test_buffer_to_buffer_trans_with_clipping() {
        let src = create_gradient_source();  // Has one transparent pixel at 0,0
        let mut dest = vec![0xFFu8; 25];  // 5x5

        let src_rect = ClipRect::new(0, 0, 10, 10);
        buffer_to_buffer_trans(
            &mut dest, 5, 5, 5,
            &src, 10, 10, 10,
            -2, -2, src_rect,
        );

        // Check some pixels - complex due to clipping and transparency
        // At dest (0,0) we're reading src (2,2) which should have value 23
        assert_eq!(dest[0], 23);  // y*10+x+1 = 2*10+2+1 = 23
    }

    #[test]
    fn test_blit_convenience() {
        let src = vec![0xFFu8; 25];
        let mut dest = vec![0u8; 100];

        blit(
            &mut dest, 10, 10, 10,
            &src, 5, 5, 5,
            2, 2,
            0, 0, 5, 5,
            false,  // opaque
        );

        // Verify same result as direct call
        let mut dest2 = vec![0u8; 100];
        let src_rect = ClipRect::new(0, 0, 5, 5);
        buffer_to_buffer(
            &mut dest2, 10, 10, 10,
            &src, 5, 5, 5,
            2, 2, src_rect,
        );

        assert_eq!(dest, dest2);
    }

    #[test]
    fn test_empty_source_rect() {
        let src = vec![0xFFu8; 100];
        let mut dest = vec![0u8; 100];

        let src_rect = ClipRect::new(0, 0, 0, 0);  // Empty
        buffer_to_buffer(
            &mut dest, 10, 10, 10,
            &src, 10, 10, 10,
            0, 0, src_rect,
        );

        // Dest should be unchanged
        assert!(dest.iter().all(|&b| b == 0));
    }

    #[test]
    fn test_source_rect_outside_source() {
        let src = vec![0xFFu8; 25];  // 5x5
        let mut dest = vec![0u8; 100];

        // Try to copy from outside source bounds
        let src_rect = ClipRect::new(10, 10, 5, 5);
        buffer_to_buffer(
            &mut dest, 10, 10, 10,
            &src, 5, 5, 5,
            0, 0, src_rect,
        );

        // Dest should be unchanged
        assert!(dest.iter().all(|&b| b == 0));
    }
}
