//! SIMD-optimized routines
//!
//! Provides optimized versions of common operations:
//! - Palette conversion (8-bit indexed to 32-bit ARGB)
//!
//! Automatically selects the best implementation for the current CPU.

/// Palette converter with automatic SIMD dispatch
pub struct PaletteConverter {
    /// Current palette (256 ARGB entries)
    palette: [u32; 256],
}

impl PaletteConverter {
    /// Create a new palette converter
    pub fn new() -> Self {
        Self {
            palette: [0xFF000000; 256], // Default to opaque black
        }
    }

    /// Set the palette
    pub fn set_palette(&mut self, palette: &[u32; 256]) {
        self.palette = *palette;
    }

    /// Get a reference to the current palette
    pub fn get_palette(&self) -> &[u32; 256] {
        &self.palette
    }

    /// Convert indexed pixels to ARGB
    ///
    /// # Arguments
    /// - `src`: Source indexed pixels (8-bit)
    /// - `dest`: Destination ARGB pixels (32-bit)
    ///
    /// # Panics
    /// Panics if dest.len() < src.len()
    pub fn convert(&self, src: &[u8], dest: &mut [u32]) {
        convert_palette_fast(src, dest, &self.palette);
    }
}

impl Default for PaletteConverter {
    fn default() -> Self {
        Self::new()
    }
}

/// Fast palette conversion with auto-dispatch
///
/// Selects the best implementation for the current CPU:
/// - AVX2 gather on x86_64 with AVX2 (future)
/// - NEON on ARM64 (future)
/// - Scalar with loop unrolling as baseline
pub fn convert_palette_fast(src: &[u8], dest: &mut [u32], palette: &[u32; 256]) {
    assert!(
        dest.len() >= src.len(),
        "Destination buffer too small: {} < {}",
        dest.len(),
        src.len()
    );

    // Use the optimized unrolled version
    convert_palette_unrolled(src, dest, palette);
}

/// Scalar conversion with 8x loop unrolling
///
/// This is surprisingly fast as the compiler can optimize it well.
/// The unrolling helps with instruction-level parallelism.
#[inline(always)]
fn convert_palette_unrolled(src: &[u8], dest: &mut [u32], palette: &[u32; 256]) {
    let len = src.len();
    let chunks = len / 8;
    let remainder = len % 8;

    // Process 8 pixels at a time
    for i in 0..chunks {
        let base = i * 8;
        // Explicit unroll for ILP
        dest[base] = palette[src[base] as usize];
        dest[base + 1] = palette[src[base + 1] as usize];
        dest[base + 2] = palette[src[base + 2] as usize];
        dest[base + 3] = palette[src[base + 3] as usize];
        dest[base + 4] = palette[src[base + 4] as usize];
        dest[base + 5] = palette[src[base + 5] as usize];
        dest[base + 6] = palette[src[base + 6] as usize];
        dest[base + 7] = palette[src[base + 7] as usize];
    }

    // Handle remainder
    let rem_start = chunks * 8;
    for i in 0..remainder {
        dest[rem_start + i] = palette[src[rem_start + i] as usize];
    }
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_palette_converter_new() {
        let converter = PaletteConverter::new();
        // All entries should be opaque black
        assert_eq!(converter.palette[0], 0xFF000000);
        assert_eq!(converter.palette[255], 0xFF000000);
    }

    #[test]
    fn test_palette_converter_set() {
        let mut converter = PaletteConverter::new();
        let mut palette = [0u32; 256];
        for i in 0..256 {
            palette[i] = 0xFF000000 | (i as u32);
        }
        converter.set_palette(&palette);

        assert_eq!(converter.palette[0], 0xFF000000);
        assert_eq!(converter.palette[1], 0xFF000001);
        assert_eq!(converter.palette[255], 0xFF0000FF);
    }

    #[test]
    fn test_convert_palette_fast_basic() {
        let mut palette = [0u32; 256];
        for i in 0..256 {
            palette[i] = 0xFF000000 | ((i as u32) << 16);
        }

        let src = [0u8, 1, 2, 3, 255];
        let mut dest = [0u32; 5];

        convert_palette_fast(&src, &mut dest, &palette);

        assert_eq!(dest[0], 0xFF000000);
        assert_eq!(dest[1], 0xFF010000);
        assert_eq!(dest[2], 0xFF020000);
        assert_eq!(dest[3], 0xFF030000);
        assert_eq!(dest[4], 0xFFFF0000);
    }

    #[test]
    fn test_convert_palette_fast_unrolled() {
        let mut palette = [0u32; 256];
        for i in 0..256 {
            palette[i] = i as u32;
        }

        // Test with exactly 8 pixels (one chunk, no remainder)
        let src: Vec<u8> = (0..8).collect();
        let mut dest = vec![0u32; 8];
        convert_palette_fast(&src, &mut dest, &palette);
        for i in 0..8 {
            assert_eq!(dest[i], i as u32);
        }

        // Test with 10 pixels (one chunk + 2 remainder)
        let src: Vec<u8> = (0..10).collect();
        let mut dest = vec![0u32; 10];
        convert_palette_fast(&src, &mut dest, &palette);
        for i in 0..10 {
            assert_eq!(dest[i], i as u32);
        }

        // Test with 3 pixels (no chunks, only remainder)
        let src: Vec<u8> = (0..3).collect();
        let mut dest = vec![0u32; 3];
        convert_palette_fast(&src, &mut dest, &palette);
        for i in 0..3 {
            assert_eq!(dest[i], i as u32);
        }
    }

    #[test]
    fn test_convert_large_buffer() {
        let mut palette = [0u32; 256];
        for i in 0..256 {
            palette[i] = i as u32;
        }

        // Test 640x400 = 256000 pixels (typical screen size)
        let size = 640 * 400;
        let src: Vec<u8> = (0..size).map(|i| (i % 256) as u8).collect();
        let mut dest = vec![0u32; size];

        convert_palette_fast(&src, &mut dest, &palette);

        for i in 0..size {
            assert_eq!(dest[i], (i % 256) as u32);
        }
    }

    #[test]
    #[should_panic(expected = "Destination buffer too small")]
    fn test_convert_palette_fast_dest_too_small() {
        let palette = [0u32; 256];
        let src = [0u8; 10];
        let mut dest = [0u32; 5];
        convert_palette_fast(&src, &mut dest, &palette);
    }

    #[test]
    fn test_converter_struct() {
        let mut converter = PaletteConverter::new();
        let mut palette = [0u32; 256];
        for i in 0..256 {
            palette[i] = i as u32 * 2;
        }
        converter.set_palette(&palette);

        let src = [0u8, 1, 2, 127, 255];
        let mut dest = [0u32; 5];
        converter.convert(&src, &mut dest);

        assert_eq!(dest[0], 0);
        assert_eq!(dest[1], 2);
        assert_eq!(dest[2], 4);
        assert_eq!(dest[3], 254);
        assert_eq!(dest[4], 510);
    }
}
