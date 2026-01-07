# Task 09a: Basic Buffer Operations

## Dependencies
- Phase 01-08 must be complete
- Platform crate structure in place

## Context
The original WIN32LIB assembly files (CLEAR.ASM, portions of BITBLIT.ASM) implement low-level buffer operations for clearing and filling rectangular regions. These are foundational operations used by all higher-level graphics routines. This task creates the `blit` module with basic operations that don't involve source-to-destination copying.

## Objective
Create the `platform/src/blit/` module with:
1. `ClipRect` structure for clipping calculations
2. `buffer_clear()` - fill entire buffer with a value
3. `buffer_fill_rect()` - fill rectangular region with a value
4. FFI exports for C++ usage
5. Comprehensive unit tests

## Deliverables
- [ ] `platform/src/blit/mod.rs` - Module root with ClipRect
- [ ] `platform/src/blit/basic.rs` - Clear and fill operations
- [ ] FFI exports in `platform/src/ffi/mod.rs`
- [ ] Unit tests pass
- [ ] Header contains FFI exports
- [ ] Verification passes

## Technical Background

### Original Assembly (CLEAR.ASM concept)
The original assembly used REP STOSB/STOSD for fast memory fills. In Rust, we use `slice::fill()` which LLVM optimizes to similar instructions.

### Buffer Layout
- 8-bit indexed color (1 byte per pixel)
- Row-major order
- Pitch may differ from width (for alignment)
- Buffer size = pitch * height

### Clipping
All operations must handle:
- Negative coordinates (clip to 0)
- Coordinates beyond buffer bounds (clip to width/height)
- Zero or negative dimensions (no-op)

## Files to Create/Modify

### platform/src/blit/mod.rs (NEW)
```rust
//! Bitmap blitting and buffer operations
//!
//! Replaces WIN32LIB assembly: BITBLIT.ASM, CLEAR.ASM
//!
//! This module provides portable Rust implementations of the low-level
//! graphics operations originally written in x86 assembly.

pub mod basic;

/// Clipping rectangle for blit operations
///
/// Used to define source regions and handle clipping to destination bounds.
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct ClipRect {
    pub x: i32,
    pub y: i32,
    pub width: i32,
    pub height: i32,
}

impl ClipRect {
    /// Create a new clipping rectangle
    #[inline]
    pub const fn new(x: i32, y: i32, width: i32, height: i32) -> Self {
        Self { x, y, width, height }
    }

    /// Create from origin with dimensions
    #[inline]
    pub const fn from_dimensions(width: i32, height: i32) -> Self {
        Self { x: 0, y: 0, width, height }
    }

    /// Check if rectangle has zero or negative area
    #[inline]
    pub const fn is_empty(&self) -> bool {
        self.width <= 0 || self.height <= 0
    }

    /// Get the right edge (x + width)
    #[inline]
    pub const fn right(&self) -> i32 {
        self.x + self.width
    }

    /// Get the bottom edge (y + height)
    #[inline]
    pub const fn bottom(&self) -> i32 {
        self.y + self.height
    }

    /// Clip this rectangle to fit within destination bounds.
    ///
    /// Returns `Some((clipped_src, dest_x, dest_y))` if any visible region remains,
    /// or `None` if the rectangle is completely outside the destination.
    ///
    /// # Arguments
    /// * `dest_x` - X position in destination where this rect will be placed
    /// * `dest_y` - Y position in destination where this rect will be placed
    /// * `dest_width` - Width of destination buffer
    /// * `dest_height` - Height of destination buffer
    ///
    /// # Returns
    /// Tuple of (adjusted source rect, adjusted dest_x, adjusted dest_y)
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

        // Clip left edge: if dest_x < 0, advance source and reduce width
        if dst_x < 0 {
            let clip = -dst_x;
            src_x += clip;
            width -= clip;
            dst_x = 0;
        }

        // Clip top edge: if dest_y < 0, advance source and reduce height
        if dst_y < 0 {
            let clip = -dst_y;
            src_y += clip;
            height -= clip;
            dst_y = 0;
        }

        // Clip right edge: if extends past dest_width, reduce width
        if dst_x + width > dest_width {
            width = dest_width - dst_x;
        }

        // Clip bottom edge: if extends past dest_height, reduce height
        if dst_y + height > dest_height {
            height = dest_height - dst_y;
        }

        // Check if anything remains
        if width <= 0 || height <= 0 {
            None
        } else {
            Some((ClipRect::new(src_x, src_y, width, height), dst_x, dst_y))
        }
    }

    /// Intersect this rectangle with another, returning the overlapping region
    pub fn intersect(&self, other: &ClipRect) -> Option<ClipRect> {
        let x1 = self.x.max(other.x);
        let y1 = self.y.max(other.y);
        let x2 = self.right().min(other.right());
        let y2 = self.bottom().min(other.bottom());

        if x2 > x1 && y2 > y1 {
            Some(ClipRect::new(x1, y1, x2 - x1, y2 - y1))
        } else {
            None
        }
    }
}

impl Default for ClipRect {
    fn default() -> Self {
        Self::new(0, 0, 0, 0)
    }
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_clip_rect_creation() {
        let rect = ClipRect::new(10, 20, 100, 50);
        assert_eq!(rect.x, 10);
        assert_eq!(rect.y, 20);
        assert_eq!(rect.width, 100);
        assert_eq!(rect.height, 50);
        assert_eq!(rect.right(), 110);
        assert_eq!(rect.bottom(), 70);
    }

    #[test]
    fn test_clip_rect_empty() {
        assert!(ClipRect::new(0, 0, 0, 0).is_empty());
        assert!(ClipRect::new(0, 0, -1, 10).is_empty());
        assert!(ClipRect::new(0, 0, 10, -1).is_empty());
        assert!(!ClipRect::new(0, 0, 1, 1).is_empty());
    }

    #[test]
    fn test_clip_to_bounds_no_clipping() {
        let src = ClipRect::new(0, 0, 50, 50);
        let result = src.clip_to_bounds(10, 10, 100, 100);

        assert!(result.is_some());
        let (clipped, dx, dy) = result.unwrap();
        assert_eq!(clipped.x, 0);
        assert_eq!(clipped.y, 0);
        assert_eq!(clipped.width, 50);
        assert_eq!(clipped.height, 50);
        assert_eq!(dx, 10);
        assert_eq!(dy, 10);
    }

    #[test]
    fn test_clip_to_bounds_left_edge() {
        let src = ClipRect::new(0, 0, 50, 50);
        let result = src.clip_to_bounds(-10, 0, 100, 100);

        assert!(result.is_some());
        let (clipped, dx, dy) = result.unwrap();
        assert_eq!(clipped.x, 10);  // Source advanced by 10
        assert_eq!(clipped.width, 40);  // Width reduced by 10
        assert_eq!(dx, 0);  // Dest clipped to 0
        assert_eq!(dy, 0);
    }

    #[test]
    fn test_clip_to_bounds_right_edge() {
        let src = ClipRect::new(0, 0, 50, 50);
        let result = src.clip_to_bounds(80, 0, 100, 100);

        assert!(result.is_some());
        let (clipped, dx, _) = result.unwrap();
        assert_eq!(clipped.width, 20);  // Only 20 pixels fit
        assert_eq!(dx, 80);
    }

    #[test]
    fn test_clip_to_bounds_completely_outside() {
        let src = ClipRect::new(0, 0, 50, 50);

        // Completely to the left
        assert!(src.clip_to_bounds(-100, 0, 50, 50).is_none());

        // Completely above
        assert!(src.clip_to_bounds(0, -100, 50, 50).is_none());

        // Completely to the right
        assert!(src.clip_to_bounds(100, 0, 50, 50).is_none());

        // Completely below
        assert!(src.clip_to_bounds(0, 100, 50, 50).is_none());
    }

    #[test]
    fn test_intersect() {
        let a = ClipRect::new(0, 0, 100, 100);
        let b = ClipRect::new(50, 50, 100, 100);

        let result = a.intersect(&b);
        assert!(result.is_some());
        let i = result.unwrap();
        assert_eq!(i.x, 50);
        assert_eq!(i.y, 50);
        assert_eq!(i.width, 50);
        assert_eq!(i.height, 50);
    }

    #[test]
    fn test_intersect_no_overlap() {
        let a = ClipRect::new(0, 0, 50, 50);
        let b = ClipRect::new(100, 100, 50, 50);

        assert!(a.intersect(&b).is_none());
    }
}
```

### platform/src/blit/basic.rs (NEW)
```rust
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
```

### platform/src/lib.rs (MODIFY)
Add after other module declarations:
```rust
pub mod blit;
```

### platform/src/ffi/mod.rs (MODIFY)
Add buffer operations FFI exports:
```rust
// =============================================================================
// Buffer Operations FFI (replaces assembly)
// =============================================================================

use crate::blit::{self, ClipRect};

/// Clear buffer with a single value
///
/// # Safety
/// - `buffer` must point to valid memory of at least `size` bytes
#[no_mangle]
pub unsafe extern "C" fn Platform_Buffer_Clear(
    buffer: *mut u8,
    size: i32,
    value: u8,
) {
    if buffer.is_null() || size <= 0 {
        return;
    }
    let slice = std::slice::from_raw_parts_mut(buffer, size as usize);
    blit::basic::buffer_clear(slice, value);
}

/// Fill rectangular region with a value
///
/// # Safety
/// - `buffer` must point to valid memory of at least `pitch * buf_height` bytes
#[no_mangle]
pub unsafe extern "C" fn Platform_Buffer_FillRect(
    buffer: *mut u8,
    pitch: i32,
    buf_width: i32,
    buf_height: i32,
    x: i32,
    y: i32,
    width: i32,
    height: i32,
    value: u8,
) {
    if buffer.is_null() || pitch <= 0 || buf_height <= 0 {
        return;
    }
    let buf_size = (pitch * buf_height) as usize;
    let slice = std::slice::from_raw_parts_mut(buffer, buf_size);
    blit::basic::buffer_fill_rect(
        slice,
        pitch as usize,
        buf_width,
        buf_height,
        x, y,
        width, height,
        value,
    );
}

/// Draw horizontal line
///
/// # Safety
/// - `buffer` must point to valid memory of at least `pitch * buf_height` bytes
#[no_mangle]
pub unsafe extern "C" fn Platform_Buffer_HLine(
    buffer: *mut u8,
    pitch: i32,
    buf_width: i32,
    buf_height: i32,
    x: i32,
    y: i32,
    length: i32,
    color: u8,
) {
    if buffer.is_null() || pitch <= 0 || buf_height <= 0 {
        return;
    }
    let buf_size = (pitch * buf_height) as usize;
    let slice = std::slice::from_raw_parts_mut(buffer, buf_size);
    blit::basic::buffer_hline(
        slice,
        pitch as usize,
        buf_width,
        buf_height,
        x, y,
        length,
        color,
    );
}

/// Draw vertical line
///
/// # Safety
/// - `buffer` must point to valid memory of at least `pitch * buf_height` bytes
#[no_mangle]
pub unsafe extern "C" fn Platform_Buffer_VLine(
    buffer: *mut u8,
    pitch: i32,
    buf_width: i32,
    buf_height: i32,
    x: i32,
    y: i32,
    length: i32,
    color: u8,
) {
    if buffer.is_null() || pitch <= 0 || buf_height <= 0 {
        return;
    }
    let buf_size = (pitch * buf_height) as usize;
    let slice = std::slice::from_raw_parts_mut(buffer, buf_size);
    blit::basic::buffer_vline(
        slice,
        pitch as usize,
        buf_width,
        buf_height,
        x, y,
        length,
        color,
    );
}
```

## Verification Command
```bash
ralph-tasks/verify/check-basic-buffer.sh
```

## Implementation Steps
1. Create `platform/src/blit/mod.rs` with ClipRect
2. Create `platform/src/blit/basic.rs` with clear/fill operations
3. Add `pub mod blit;` to `platform/src/lib.rs`
4. Add FFI exports to `platform/src/ffi/mod.rs`
5. Run `cargo test --manifest-path platform/Cargo.toml -- blit` to verify unit tests
6. Run `cargo build --manifest-path platform/Cargo.toml --features cbindgen-run`
7. Verify header contains FFI exports
8. Run verification script

## Success Criteria
- All unit tests pass (ClipRect, buffer_clear, buffer_fill_rect)
- FFI exports compile and appear in platform.h
- Clipping works correctly for all edge cases
- No memory safety issues

## Common Issues

### "cannot find module blit"
- Ensure `pub mod blit;` is in lib.rs
- Check file is named correctly: `platform/src/blit/mod.rs`

### "cannot find basic in blit"
- Ensure `pub mod basic;` is in `blit/mod.rs`

### Clipping produces wrong results
- Trace through clip_to_bounds with specific coordinates
- Verify signs in clipping calculations
- Check that clipped dimensions are correctly applied

### Buffer overflow panic
- Verify pitch * height calculation
- Check bounds in fill loop

## Completion Promise
When verification passes, output:
```
<promise>TASK_09A_COMPLETE</promise>
```

## Escape Hatch
If stuck after 10 iterations:
- Document what's blocking in `ralph-tasks/blocked/09a.md`
- List attempted approaches
- Output: `<promise>TASK_09A_BLOCKED</promise>`

## Max Iterations
10
