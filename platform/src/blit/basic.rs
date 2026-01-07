//! Basic buffer operations: clear and fill
//!
//! Replaces WIN32LIB/DRAWBUFF/CLEAR.ASM

use super::ClipRect;

/// Fill entire buffer with a single value
///
/// This is the fastest way to clear a buffer. Uses `slice::fill()` which
/// LLVM optimizes to REP STOSB or equivalent on all platforms.
///
/// # Arguments
/// * `buffer` - Mutable byte slice to fill
/// * `value` - Value to fill with (typically 0 for black, or a palette index)
///
/// # Example
/// ```
/// use redalert_platform::blit::basic::buffer_clear;
/// let mut buffer = vec![0u8; 640 * 400];
/// buffer_clear(&mut buffer, 0);  // Clear to black
/// ```
#[inline]
pub fn buffer_clear(buffer: &mut [u8], value: u8) {
    buffer.fill(value);
}

/// Fill a rectangular region with a single value
///
/// Handles clipping to buffer bounds automatically. If the rectangle
/// extends outside the buffer, only the visible portion is filled.
///
/// # Arguments
/// * `buffer` - Mutable byte slice (row-major, pitch bytes per row)
/// * `pitch` - Bytes per row (may be > width for alignment)
/// * `buf_width` - Buffer width in pixels
/// * `buf_height` - Buffer height in pixels
/// * `x` - X coordinate of rectangle (can be negative)
/// * `y` - Y coordinate of rectangle (can be negative)
/// * `width` - Width of rectangle
/// * `height` - Height of rectangle
/// * `value` - Fill value
///
/// # Safety
/// Buffer must be at least `pitch * buf_height` bytes.
pub fn buffer_fill_rect(
    buffer: &mut [u8],
    pitch: usize,
    buf_width: i32,
    buf_height: i32,
    x: i32,
    y: i32,
    width: i32,
    height: i32,
    value: u8,
) {
    // Early exit for invalid dimensions
    if width <= 0 || height <= 0 || buf_width <= 0 || buf_height <= 0 {
        return;
    }

    // Clip rectangle to buffer bounds
    let rect = ClipRect::new(0, 0, width, height);
    let Some((clipped, clip_x, clip_y)) = rect.clip_to_bounds(x, y, buf_width, buf_height) else {
        return;  // Completely outside buffer
    };

    let fill_width = clipped.width as usize;
    let fill_height = clipped.height as usize;
    let start_x = clip_x as usize;
    let start_y = clip_y as usize;

    // Fill each row
    for row in 0..fill_height {
        let row_start = (start_y + row) * pitch + start_x;
        let row_end = row_start + fill_width;

        // Bounds check (defensive, should always pass after clipping)
        if row_end <= buffer.len() {
            buffer[row_start..row_end].fill(value);
        }
    }
}

/// Fill rectangle with clipping against a custom clip region
///
/// Like `buffer_fill_rect` but clips against an additional ClipRect,
/// useful when the game has a defined viewport.
pub fn buffer_fill_rect_clipped(
    buffer: &mut [u8],
    pitch: usize,
    clip: &ClipRect,
    x: i32,
    y: i32,
    width: i32,
    height: i32,
    value: u8,
) {
    // Early exit
    if width <= 0 || height <= 0 || clip.is_empty() {
        return;
    }

    // First clip against the clip region
    let rect = ClipRect::new(x, y, width, height);
    let Some(visible) = rect.intersect(clip) else {
        return;
    };

    // Now fill the visible portion
    let fill_width = visible.width as usize;
    let fill_height = visible.height as usize;
    let start_x = visible.x as usize;
    let start_y = visible.y as usize;

    for row in 0..fill_height {
        let row_start = (start_y + row) * pitch + start_x;
        let row_end = row_start + fill_width;

        if row_end <= buffer.len() {
            buffer[row_start..row_end].fill(value);
        }
    }
}

/// Draw a horizontal line (1 pixel thick)
pub fn buffer_hline(
    buffer: &mut [u8],
    pitch: usize,
    buf_width: i32,
    buf_height: i32,
    x: i32,
    y: i32,
    length: i32,
    color: u8,
) {
    buffer_fill_rect(buffer, pitch, buf_width, buf_height, x, y, length, 1, color);
}

/// Draw a vertical line (1 pixel thick)
pub fn buffer_vline(
    buffer: &mut [u8],
    pitch: usize,
    buf_width: i32,
    buf_height: i32,
    x: i32,
    y: i32,
    length: i32,
    color: u8,
) {
    buffer_fill_rect(buffer, pitch, buf_width, buf_height, x, y, 1, length, color);
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_buffer_clear() {
        let mut buffer = vec![0xFFu8; 100];
        buffer_clear(&mut buffer, 0);
        assert!(buffer.iter().all(|&b| b == 0));

        buffer_clear(&mut buffer, 42);
        assert!(buffer.iter().all(|&b| b == 42));
    }

    #[test]
    fn test_buffer_fill_rect_basic() {
        // 10x10 buffer with pitch=10
        let mut buffer = vec![0u8; 100];
        buffer_fill_rect(&mut buffer, 10, 10, 10, 2, 2, 4, 4, 0xFF);

        // Check filled region
        for y in 0..10 {
            for x in 0..10 {
                let expected = if x >= 2 && x < 6 && y >= 2 && y < 6 {
                    0xFF
                } else {
                    0
                };
                assert_eq!(buffer[y * 10 + x], expected, "at ({}, {})", x, y);
            }
        }
    }

    #[test]
    fn test_buffer_fill_rect_clip_left() {
        let mut buffer = vec![0u8; 100];
        buffer_fill_rect(&mut buffer, 10, 10, 10, -2, 0, 5, 5, 0xFF);

        // Should fill from x=0 to x=2 (3 pixels), because 2 are clipped
        for y in 0..5 {
            for x in 0..10 {
                let expected = if x < 3 && y < 5 { 0xFF } else { 0 };
                assert_eq!(buffer[y * 10 + x], expected, "at ({}, {})", x, y);
            }
        }
    }

    #[test]
    fn test_buffer_fill_rect_clip_right() {
        let mut buffer = vec![0u8; 100];
        buffer_fill_rect(&mut buffer, 10, 10, 10, 7, 0, 5, 5, 0xFF);

        // Should fill from x=7 to x=9 (3 pixels)
        for y in 0..5 {
            for x in 0..10 {
                let expected = if x >= 7 && x < 10 && y < 5 { 0xFF } else { 0 };
                assert_eq!(buffer[y * 10 + x], expected, "at ({}, {})", x, y);
            }
        }
    }

    #[test]
    fn test_buffer_fill_rect_completely_outside() {
        let mut buffer = vec![0u8; 100];

        // Completely outside - should not modify buffer
        buffer_fill_rect(&mut buffer, 10, 10, 10, -10, -10, 5, 5, 0xFF);
        assert!(buffer.iter().all(|&b| b == 0));

        buffer_fill_rect(&mut buffer, 10, 10, 10, 100, 100, 5, 5, 0xFF);
        assert!(buffer.iter().all(|&b| b == 0));
    }

    #[test]
    fn test_buffer_fill_rect_zero_dimensions() {
        let mut buffer = vec![0u8; 100];

        buffer_fill_rect(&mut buffer, 10, 10, 10, 0, 0, 0, 5, 0xFF);
        assert!(buffer.iter().all(|&b| b == 0));

        buffer_fill_rect(&mut buffer, 10, 10, 10, 0, 0, 5, 0, 0xFF);
        assert!(buffer.iter().all(|&b| b == 0));
    }

    #[test]
    fn test_buffer_fill_rect_negative_dimensions() {
        let mut buffer = vec![0u8; 100];
        buffer_fill_rect(&mut buffer, 10, 10, 10, 0, 0, -5, 5, 0xFF);
        assert!(buffer.iter().all(|&b| b == 0));
    }

    #[test]
    fn test_buffer_hline() {
        let mut buffer = vec![0u8; 100];
        buffer_hline(&mut buffer, 10, 10, 10, 2, 5, 6, 0xFF);

        // Check only row 5, columns 2-7 are filled
        for y in 0..10 {
            for x in 0..10 {
                let expected = if y == 5 && x >= 2 && x < 8 { 0xFF } else { 0 };
                assert_eq!(buffer[y * 10 + x], expected, "at ({}, {})", x, y);
            }
        }
    }

    #[test]
    fn test_buffer_vline() {
        let mut buffer = vec![0u8; 100];
        buffer_vline(&mut buffer, 10, 10, 10, 5, 2, 6, 0xFF);

        // Check only column 5, rows 2-7 are filled
        for y in 0..10 {
            for x in 0..10 {
                let expected = if x == 5 && y >= 2 && y < 8 { 0xFF } else { 0 };
                assert_eq!(buffer[y * 10 + x], expected, "at ({}, {})", x, y);
            }
        }
    }

    #[test]
    fn test_buffer_fill_rect_with_pitch() {
        // Buffer with pitch > width (padding for alignment)
        // Width = 8, but pitch = 16 (8 bytes padding per row)
        let mut buffer = vec![0u8; 16 * 10];  // 10 rows, pitch 16
        buffer_fill_rect(&mut buffer, 16, 8, 10, 2, 2, 4, 4, 0xFF);

        for y in 0..10 {
            for x in 0..8 {
                let expected = if x >= 2 && x < 6 && y >= 2 && y < 6 {
                    0xFF
                } else {
                    0
                };
                assert_eq!(buffer[y * 16 + x], expected, "at ({}, {})", x, y);
            }
            // Padding bytes should be unchanged
            for x in 8..16 {
                assert_eq!(buffer[y * 16 + x], 0, "padding at ({}, {})", x, y);
            }
        }
    }
}
