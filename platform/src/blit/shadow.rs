//! Shadow drawing operations
//!
//! Replaces WIN32LIB/DRAWBUFF/SHADOW.ASM
//!
//! Shadows are applied by darkening existing pixels through a lookup table.

use super::remap::RemapTable;

/// Default shadow intensity (50% darkness)
pub const DEFAULT_SHADOW_INTENSITY: f32 = 0.5;

/// Apply shadow to a rectangular region
///
/// Each pixel in the region is darkened by looking up its current value
/// in the shadow table and replacing it with the result.
///
/// # Arguments
/// * `buffer` - Destination buffer to darken
/// * `pitch` - Bytes per row
/// * `x` - X coordinate of shadow region
/// * `y` - Y coordinate of shadow region
/// * `width` - Width of shadow region
/// * `height` - Height of shadow region
/// * `shadow_table` - 256-byte lookup table mapping colors to darker versions
pub fn buffer_shadow(
    buffer: &mut [u8],
    pitch: usize,
    x: i32,
    y: i32,
    width: i32,
    height: i32,
    shadow_table: &RemapTable,
) {
    if width <= 0 || height <= 0 {
        return;
    }

    let x = x.max(0) as usize;
    let y = y.max(0) as usize;
    let width = width as usize;
    let height = height as usize;

    for row in 0..height {
        let row_start = (y + row) * pitch + x;
        let row_end = row_start + width;

        if row_end <= buffer.len() {
            for pixel in &mut buffer[row_start..row_end] {
                *pixel = shadow_table[*pixel as usize];
            }
        }
    }
}

/// Apply shadow using a mask sprite
///
/// The mask determines WHERE to apply shadow, not what color.
/// Where mask is non-zero, the destination pixel is darkened.
/// Where mask is zero, the destination is unchanged.
///
/// # Arguments
/// * `buffer` - Destination buffer to darken
/// * `buffer_pitch` - Bytes per row in destination
/// * `mask` - Mask sprite (non-zero = apply shadow)
/// * `mask_pitch` - Bytes per row in mask
/// * `x` - X position in destination
/// * `y` - Y position in destination
/// * `width` - Width of mask/shadow region
/// * `height` - Height of mask/shadow region
/// * `shadow_table` - 256-byte lookup table
pub fn buffer_shadow_mask(
    buffer: &mut [u8],
    buffer_pitch: usize,
    mask: &[u8],
    mask_pitch: usize,
    x: i32,
    y: i32,
    width: i32,
    height: i32,
    shadow_table: &RemapTable,
) {
    if width <= 0 || height <= 0 {
        return;
    }

    // Handle negative coordinates (simple clip)
    let (x, y, mask_x, mask_y, width, height) = clip_shadow(
        x, y, 0, 0, width, height
    );

    let x = x as usize;
    let y = y as usize;
    let mask_x = mask_x as usize;
    let mask_y = mask_y as usize;
    let width = width as usize;
    let height = height as usize;

    for row in 0..height {
        let buf_row_start = (y + row) * buffer_pitch + x;
        let mask_row_start = (mask_y + row) * mask_pitch + mask_x;

        for col in 0..width {
            let mask_idx = mask_row_start + col;
            let buf_idx = buf_row_start + col;

            if mask_idx < mask.len() && buf_idx < buffer.len() {
                // Only apply shadow where mask is non-zero
                if mask[mask_idx] != 0 {
                    buffer[buf_idx] = shadow_table[buffer[buf_idx] as usize];
                }
            }
        }
    }
}

/// Apply shadow with clipping to buffer bounds
pub fn buffer_shadow_clipped(
    buffer: &mut [u8],
    buffer_pitch: usize,
    buffer_width: i32,
    buffer_height: i32,
    mask: &[u8],
    mask_pitch: usize,
    x: i32,
    y: i32,
    width: i32,
    height: i32,
    shadow_table: &RemapTable,
) {
    if width <= 0 || height <= 0 {
        return;
    }

    // Clip to buffer bounds
    let mut clip_x = x;
    let mut clip_y = y;
    let mut mask_x = 0i32;
    let mut mask_y = 0i32;
    let mut clip_width = width;
    let mut clip_height = height;

    // Clip left
    if clip_x < 0 {
        mask_x = -clip_x;
        clip_width += clip_x;
        clip_x = 0;
    }

    // Clip top
    if clip_y < 0 {
        mask_y = -clip_y;
        clip_height += clip_y;
        clip_y = 0;
    }

    // Clip right
    if clip_x + clip_width > buffer_width {
        clip_width = buffer_width - clip_x;
    }

    // Clip bottom
    if clip_y + clip_height > buffer_height {
        clip_height = buffer_height - clip_y;
    }

    if clip_width <= 0 || clip_height <= 0 {
        return;
    }

    let clip_x = clip_x as usize;
    let clip_y = clip_y as usize;
    let mask_x = mask_x as usize;
    let mask_y = mask_y as usize;
    let clip_width = clip_width as usize;
    let clip_height = clip_height as usize;

    for row in 0..clip_height {
        let buf_row_start = (clip_y + row) * buffer_pitch + clip_x;
        let mask_row_start = (mask_y + row) * mask_pitch + mask_x;

        for col in 0..clip_width {
            let mask_idx = mask_row_start + col;
            let buf_idx = buf_row_start + col;

            if mask_idx < mask.len() && buf_idx < buffer.len() {
                if mask[mask_idx] != 0 {
                    buffer[buf_idx] = shadow_table[buffer[buf_idx] as usize];
                }
            }
        }
    }
}

/// Simple clipping helper
fn clip_shadow(
    x: i32, y: i32,
    mask_x: i32, mask_y: i32,
    width: i32, height: i32,
) -> (i32, i32, i32, i32, i32, i32) {
    let mut out_x = x;
    let mut out_y = y;
    let mut out_mask_x = mask_x;
    let mut out_mask_y = mask_y;
    let mut out_width = width;
    let mut out_height = height;

    if out_x < 0 {
        out_mask_x -= out_x;
        out_width += out_x;
        out_x = 0;
    }

    if out_y < 0 {
        out_mask_y -= out_y;
        out_height += out_y;
        out_y = 0;
    }

    (out_x, out_y, out_mask_x, out_mask_y, out_width.max(0), out_height.max(0))
}

/// Generate a shadow lookup table from a palette
///
/// Creates a table that maps each color to a darker version.
///
/// # Arguments
/// * `palette` - Array of 256 RGB triplets
/// * `intensity` - Shadow darkness (0.0 = black, 1.0 = no change)
///
/// # Returns
/// 256-byte lookup table
pub fn generate_shadow_table(palette: &[[u8; 3]; 256], intensity: f32) -> RemapTable {
    let mut table = [0u8; 256];
    let intensity = intensity.clamp(0.0, 1.0);

    for i in 0..256 {
        // Calculate target darkened color
        let target_r = ((palette[i][0] as f32) * intensity) as u8;
        let target_g = ((palette[i][1] as f32) * intensity) as u8;
        let target_b = ((palette[i][2] as f32) * intensity) as u8;

        // Find closest palette entry to target
        let mut best_idx = 0usize;
        let mut best_dist = u32::MAX;

        for j in 0..256 {
            let dr = (palette[j][0] as i32 - target_r as i32).abs() as u32;
            let dg = (palette[j][1] as i32 - target_g as i32).abs() as u32;
            let db = (palette[j][2] as i32 - target_b as i32).abs() as u32;

            // Weighted distance (human eye is more sensitive to green)
            let dist = dr * dr + dg * dg * 2 + db * db;

            if dist < best_dist {
                best_dist = dist;
                best_idx = j;
            }
        }

        table[i] = best_idx as u8;
    }

    table
}

/// Generate shadow table with fixed 50% darkness
pub fn generate_shadow_table_50(palette: &[[u8; 3]; 256]) -> RemapTable {
    generate_shadow_table(palette, DEFAULT_SHADOW_INTENSITY)
}

/// Create a simple shadow table for grayscale palette
///
/// For testing without a real palette. Maps each value to half.
pub fn create_simple_shadow_table() -> RemapTable {
    let mut table = [0u8; 256];
    for i in 0..256 {
        table[i] = (i / 2) as u8;
    }
    table
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    fn create_test_palette() -> [[u8; 3]; 256] {
        let mut palette = [[0u8; 3]; 256];
        // Create a grayscale ramp
        for i in 0..256 {
            palette[i] = [i as u8, i as u8, i as u8];
        }
        palette
    }

    #[test]
    fn test_simple_shadow_table() {
        let table = create_simple_shadow_table();
        assert_eq!(table[0], 0);
        assert_eq!(table[100], 50);
        assert_eq!(table[200], 100);
        assert_eq!(table[255], 127);
    }

    #[test]
    fn test_buffer_shadow_basic() {
        let mut buffer = vec![100u8; 100];  // 10x10 filled with 100
        let table = create_simple_shadow_table();

        buffer_shadow(&mut buffer, 10, 2, 2, 4, 4, &table);

        // Center 4x4 should be halved to 50
        for y in 0..10 {
            for x in 0..10 {
                let expected = if x >= 2 && x < 6 && y >= 2 && y < 6 { 50 } else { 100 };
                assert_eq!(buffer[y * 10 + x], expected, "at ({}, {})", x, y);
            }
        }
    }

    #[test]
    fn test_buffer_shadow_mask_basic() {
        // 10x10 buffer filled with 200
        let mut buffer = vec![200u8; 100];

        // 4x4 mask with checkerboard pattern
        let mask = vec![
            1, 0, 1, 0,
            0, 1, 0, 1,
            1, 0, 1, 0,
            0, 1, 0, 1,
        ];

        let table = create_simple_shadow_table();

        buffer_shadow_mask(
            &mut buffer, 10,
            &mask, 4,
            2, 2, 4, 4,
            &table,
        );

        // Check the affected region
        for row in 0..4 {
            for col in 0..4 {
                let buf_y = 2 + row;
                let buf_x = 2 + col;
                let mask_pixel = mask[row * 4 + col];
                let expected = if mask_pixel != 0 { 100 } else { 200 };  // 200/2 = 100
                assert_eq!(buffer[buf_y * 10 + buf_x], expected, "at ({}, {})", buf_x, buf_y);
            }
        }
    }

    #[test]
    fn test_buffer_shadow_mask_all_transparent() {
        let mut buffer = vec![200u8; 100];
        let original = buffer.clone();
        let mask = vec![0u8; 16];  // All transparent
        let table = create_simple_shadow_table();

        buffer_shadow_mask(
            &mut buffer, 10,
            &mask, 4,
            0, 0, 4, 4,
            &table,
        );

        // Buffer should be unchanged
        assert_eq!(buffer, original);
    }

    #[test]
    fn test_buffer_shadow_mask_all_opaque() {
        let mut buffer = vec![200u8; 100];
        let mask = vec![1u8; 16];  // All opaque
        let table = create_simple_shadow_table();

        buffer_shadow_mask(
            &mut buffer, 10,
            &mask, 4,
            0, 0, 4, 4,
            &table,
        );

        // Check that 4x4 region is darkened
        for y in 0..10 {
            for x in 0..10 {
                let expected = if x < 4 && y < 4 { 100 } else { 200 };
                assert_eq!(buffer[y * 10 + x], expected, "at ({}, {})", x, y);
            }
        }
    }

    #[test]
    fn test_generate_shadow_table() {
        let palette = create_test_palette();  // Grayscale 0-255

        // 50% shadow
        let table_50 = generate_shadow_table(&palette, 0.5);

        // For grayscale palette, color 200 at 50% should map to ~100
        // (exact match depends on palette search)
        assert!(table_50[200] >= 90 && table_50[200] <= 110);
        assert!(table_50[100] >= 40 && table_50[100] <= 60);

        // 0% shadow = everything maps to black (index 0)
        let table_0 = generate_shadow_table(&palette, 0.0);
        assert_eq!(table_0[100], 0);
        assert_eq!(table_0[255], 0);

        // 100% shadow = identity (no change)
        let table_100 = generate_shadow_table(&palette, 1.0);
        for i in 0..256 {
            // Should map to itself or very close
            let diff = (table_100[i] as i32 - i as i32).abs();
            assert!(diff <= 1, "index {} mapped to {}", i, table_100[i]);
        }
    }

    #[test]
    fn test_buffer_shadow_clipped() {
        let mut buffer = vec![200u8; 100];  // 10x10
        let mask = vec![1u8; 100];  // 10x10 all opaque
        let table = create_simple_shadow_table();

        // Shadow extends off right and bottom
        buffer_shadow_clipped(
            &mut buffer, 10, 10, 10,
            &mask, 10,
            7, 7, 10, 10,  // Only 3x3 should be visible
            &table,
        );

        // Only (7,7) to (9,9) should be darkened
        for y in 0..10 {
            for x in 0..10 {
                let expected = if x >= 7 && y >= 7 { 100 } else { 200 };
                assert_eq!(buffer[y * 10 + x], expected, "at ({}, {})", x, y);
            }
        }
    }

    #[test]
    fn test_buffer_shadow_negative_coords() {
        let mut buffer = vec![200u8; 100];
        let mask = vec![1u8; 16];  // 4x4 all opaque
        let table = create_simple_shadow_table();

        // Shadow starts at (-2, -2), so only 2x2 should be visible at (0,0)
        buffer_shadow_mask(
            &mut buffer, 10,
            &mask, 4,
            -2, -2, 4, 4,
            &table,
        );

        // Only (0,0) to (1,1) should be darkened
        for y in 0..10 {
            for x in 0..10 {
                let expected = if x < 2 && y < 2 { 100 } else { 200 };
                assert_eq!(buffer[y * 10 + x], expected, "at ({}, {})", x, y);
            }
        }
    }

    #[test]
    fn test_shadow_zero_dimensions() {
        let mut buffer = vec![200u8; 100];
        let original = buffer.clone();
        let table = create_simple_shadow_table();

        buffer_shadow(&mut buffer, 10, 0, 0, 0, 0, &table);
        assert_eq!(buffer, original);

        buffer_shadow(&mut buffer, 10, 0, 0, -5, 5, &table);
        assert_eq!(buffer, original);
    }
}
