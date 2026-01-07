//! Bitmap blitting and buffer operations
//!
//! Replaces WIN32LIB assembly: BITBLIT.ASM, CLEAR.ASM
//!
//! This module provides portable Rust implementations of the low-level
//! graphics operations originally written in x86 assembly.

pub mod basic;
pub mod copy;
pub mod remap;
pub mod scale;
pub mod shadow;

#[cfg(feature = "simd")]
pub mod simd;

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
