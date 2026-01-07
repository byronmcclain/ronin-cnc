# Task 09c: Palette Remapping

## Dependencies
- Task 09a must be complete (basic buffer operations)
- `platform/src/blit/mod.rs` structure in place

## Context
The original WIN32LIB/DRAWBUFF/REMAP.ASM implements color remapping through 256-byte lookup tables. This is used for team colors (converting neutral units to faction colors), lighting effects, and other color transformations. The remapping operates on 8-bit indexed pixels.

## Objective
Create palette remapping functions:
1. `buffer_remap()` - Apply remap table to entire region
2. `buffer_remap_trans()` - Remap with transparency (skip color 0)
3. `buffer_remap_copy()` - Remap from source to destination
4. FFI exports and comprehensive tests

## Deliverables
- [ ] `platform/src/blit/remap.rs` - Remap operations
- [ ] FFI exports for all remap functions
- [ ] Unit tests including identity and various transformations
- [ ] Header contains FFI exports
- [ ] Verification passes

## Technical Background

### How Remapping Works
A remap table is a 256-byte array where `table[i]` gives the output color for input color `i`. Common uses:
- **Identity**: `table[i] = i` (no change)
- **Team colors**: `table[red] = blue` for blue team
- **Shadow**: `table[i] = darker_version_of_i`
- **Fade**: `table[i] = faded_version_of_i`

### Original Assembly
REMAP.ASM used XLAT instruction or MOV lookups. Modern compilers optimize array lookups well, so Rust code is equally efficient.

### Use Cases in Red Alert
1. **House colors**: GDI yellow, Nod red, etc.
2. **Shadow effects**: Darken ground under units
3. **Stealth**: Make units semi-transparent
4. **Damage**: Tint damaged buildings

## Files to Create/Modify

### platform/src/blit/remap.rs (NEW)
```rust
//! Palette remapping operations
//!
//! Replaces WIN32LIB/DRAWBUFF/REMAP.ASM
//!
//! Remapping transforms pixel colors through a 256-byte lookup table.
//! This is used for team colors, shadows, and other effects.

use super::ClipRect;

/// Type alias for remap table (256 entries for 8-bit indexed color)
pub type RemapTable = [u8; 256];

/// Apply remap table to a rectangular region in-place
///
/// Each pixel value is replaced by `remap_table[pixel]`.
///
/// # Arguments
/// * `buffer` - Buffer to modify
/// * `pitch` - Bytes per row
/// * `x` - X coordinate of region
/// * `y` - Y coordinate of region
/// * `width` - Width of region
/// * `height` - Height of region
/// * `remap_table` - 256-byte lookup table
///
/// # Example
/// ```ignore
/// let mut remap = [0u8; 256];
/// for i in 0..256 { remap[i] = i as u8; }  // Identity
/// remap[1] = 2;  // Change color 1 to color 2
/// buffer_remap(&mut buffer, 640, 0, 0, 640, 400, &remap);
/// ```
pub fn buffer_remap(
    buffer: &mut [u8],
    pitch: usize,
    x: i32,
    y: i32,
    width: i32,
    height: i32,
    remap_table: &RemapTable,
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
                *pixel = remap_table[*pixel as usize];
            }
        }
    }
}

/// Apply remap table with transparency (skip color 0)
///
/// Like `buffer_remap` but pixels with value 0 are not modified.
/// This preserves transparent areas.
pub fn buffer_remap_trans(
    buffer: &mut [u8],
    pitch: usize,
    x: i32,
    y: i32,
    width: i32,
    height: i32,
    remap_table: &RemapTable,
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
                if *pixel != 0 {
                    *pixel = remap_table[*pixel as usize];
                }
            }
        }
    }
}

/// Remap from source to destination
///
/// Reads from source, applies remap, writes to destination.
/// Source and destination can be different buffers.
///
/// # Arguments
/// * `dest` - Destination buffer
/// * `dest_pitch` - Bytes per row in destination
/// * `dest_x`, `dest_y` - Position in destination
/// * `src` - Source buffer
/// * `src_pitch` - Bytes per row in source
/// * `src_x`, `src_y` - Position in source
/// * `width`, `height` - Region size
/// * `remap_table` - 256-byte lookup table
pub fn buffer_remap_copy(
    dest: &mut [u8],
    dest_pitch: usize,
    dest_x: i32,
    dest_y: i32,
    src: &[u8],
    src_pitch: usize,
    src_x: i32,
    src_y: i32,
    width: i32,
    height: i32,
    remap_table: &RemapTable,
) {
    if width <= 0 || height <= 0 {
        return;
    }

    let dest_x = dest_x.max(0) as usize;
    let dest_y = dest_y.max(0) as usize;
    let src_x = src_x.max(0) as usize;
    let src_y = src_y.max(0) as usize;
    let width = width as usize;
    let height = height as usize;

    for row in 0..height {
        let dest_row_start = (dest_y + row) * dest_pitch + dest_x;
        let src_row_start = (src_y + row) * src_pitch + src_x;

        for col in 0..width {
            let src_idx = src_row_start + col;
            let dest_idx = dest_row_start + col;

            if src_idx < src.len() && dest_idx < dest.len() {
                dest[dest_idx] = remap_table[src[src_idx] as usize];
            }
        }
    }
}

/// Remap copy with transparency
///
/// Like `buffer_remap_copy` but skips source pixels with value 0.
pub fn buffer_remap_copy_trans(
    dest: &mut [u8],
    dest_pitch: usize,
    dest_x: i32,
    dest_y: i32,
    src: &[u8],
    src_pitch: usize,
    src_x: i32,
    src_y: i32,
    width: i32,
    height: i32,
    remap_table: &RemapTable,
) {
    if width <= 0 || height <= 0 {
        return;
    }

    let dest_x = dest_x.max(0) as usize;
    let dest_y = dest_y.max(0) as usize;
    let src_x = src_x.max(0) as usize;
    let src_y = src_y.max(0) as usize;
    let width = width as usize;
    let height = height as usize;

    for row in 0..height {
        let dest_row_start = (dest_y + row) * dest_pitch + dest_x;
        let src_row_start = (src_y + row) * src_pitch + src_x;

        for col in 0..width {
            let src_idx = src_row_start + col;
            let dest_idx = dest_row_start + col;

            if src_idx < src.len() && dest_idx < dest.len() {
                let src_pixel = src[src_idx];
                if src_pixel != 0 {
                    dest[dest_idx] = remap_table[src_pixel as usize];
                }
            }
        }
    }
}

/// Create an identity remap table (no transformation)
pub fn create_identity_table() -> RemapTable {
    let mut table = [0u8; 256];
    for i in 0..256 {
        table[i] = i as u8;
    }
    table
}

/// Create a grayscale remap table from RGB palette
///
/// Converts palette entries to grayscale using standard luminance formula.
/// The palette is an array of 256 RGB triplets.
pub fn create_grayscale_table(palette: &[[u8; 3]; 256]) -> RemapTable {
    let mut table = [0u8; 256];

    // First, calculate grayscale value for each palette entry
    let mut gray_values: [(u8, usize); 256] = [(0, 0); 256];
    for i in 0..256 {
        let r = palette[i][0] as u32;
        let g = palette[i][1] as u32;
        let b = palette[i][2] as u32;
        // Standard luminance: 0.299*R + 0.587*G + 0.114*B
        let gray = ((r * 77 + g * 150 + b * 29) / 256) as u8;
        gray_values[i] = (gray, i);
    }

    // For each input color, find the closest palette entry with that gray level
    for i in 0..256 {
        let target_gray = gray_values[i].0;
        let mut best_idx = i;
        let mut best_diff = u32::MAX;

        for j in 0..256 {
            // Find the palette entry closest to this gray level
            let pr = palette[j][0] as i32;
            let pg = palette[j][1] as i32;
            let pb = palette[j][2] as i32;

            // Prefer entries that are already grayish
            let gray_diff = (pr - pg).abs() + (pg - pb).abs() + (pr - pb).abs();
            let luminance = ((pr as u32 * 77 + pg as u32 * 150 + pb as u32 * 29) / 256) as i32;
            let lum_diff = (luminance - target_gray as i32).unsigned_abs();

            let diff = lum_diff * 10 + gray_diff as u32;
            if diff < best_diff {
                best_diff = diff;
                best_idx = j;
            }
        }

        table[i] = best_idx as u8;
    }

    table
}

/// Create a color shift table (add offset to color indices)
///
/// Useful for animation effects or palette rotation.
pub fn create_shift_table(offset: i32) -> RemapTable {
    let mut table = [0u8; 256];
    for i in 0..256 {
        table[i] = ((i as i32 + offset).rem_euclid(256)) as u8;
    }
    table
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_identity_table() {
        let table = create_identity_table();
        for i in 0..256 {
            assert_eq!(table[i], i as u8);
        }
    }

    #[test]
    fn test_buffer_remap_identity() {
        let mut buffer = vec![0u8; 100];
        for i in 0..100 {
            buffer[i] = (i % 256) as u8;
        }
        let original = buffer.clone();
        let table = create_identity_table();

        buffer_remap(&mut buffer, 10, 0, 0, 10, 10, &table);

        assert_eq!(buffer, original);
    }

    #[test]
    fn test_buffer_remap_swap() {
        let mut buffer = vec![1u8; 100];  // All 1s
        let mut table = create_identity_table();
        table[1] = 42;  // Remap 1 -> 42

        buffer_remap(&mut buffer, 10, 0, 0, 10, 10, &table);

        assert!(buffer.iter().all(|&b| b == 42));
    }

    #[test]
    fn test_buffer_remap_partial() {
        let mut buffer = vec![1u8; 100];
        let mut table = create_identity_table();
        table[1] = 42;

        // Only remap center 4x4
        buffer_remap(&mut buffer, 10, 3, 3, 4, 4, &table);

        for y in 0..10 {
            for x in 0..10 {
                let expected = if x >= 3 && x < 7 && y >= 3 && y < 7 { 42 } else { 1 };
                assert_eq!(buffer[y * 10 + x], expected, "at ({}, {})", x, y);
            }
        }
    }

    #[test]
    fn test_buffer_remap_trans() {
        let mut buffer = vec![0u8; 100];
        // Pattern: 0, 1, 2, 0, 1, 2, ...
        for i in 0..100 {
            buffer[i] = (i % 3) as u8;
        }

        let mut table = create_identity_table();
        table[1] = 10;  // 1 -> 10
        table[2] = 20;  // 2 -> 20
        // table[0] = 0, but won't be applied due to transparency

        buffer_remap_trans(&mut buffer, 10, 0, 0, 10, 10, &table);

        for i in 0..100 {
            let original = (i % 3) as u8;
            let expected = match original {
                0 => 0,   // Transparent, unchanged
                1 => 10,
                2 => 20,
                _ => unreachable!(),
            };
            assert_eq!(buffer[i], expected, "at index {}", i);
        }
    }

    #[test]
    fn test_buffer_remap_copy() {
        let src = vec![1u8; 25];  // 5x5 all 1s
        let mut dest = vec![0u8; 100];  // 10x10

        let mut table = create_identity_table();
        table[1] = 99;

        buffer_remap_copy(
            &mut dest, 10, 2, 2,
            &src, 5, 0, 0,
            5, 5,
            &table,
        );

        for y in 0..10 {
            for x in 0..10 {
                let expected = if x >= 2 && x < 7 && y >= 2 && y < 7 { 99 } else { 0 };
                assert_eq!(dest[y * 10 + x], expected, "at ({}, {})", x, y);
            }
        }
    }

    #[test]
    fn test_buffer_remap_copy_trans() {
        // Source with 0 (transparent) and 1
        let mut src = vec![0u8; 25];
        for i in 0..25 {
            src[i] = (i % 2) as u8;  // Alternating 0 and 1
        }

        let mut dest = vec![0xFFu8; 100];  // Filled with 0xFF

        let mut table = create_identity_table();
        table[1] = 42;

        buffer_remap_copy_trans(
            &mut dest, 10, 0, 0,
            &src, 5, 0, 0,
            5, 5,
            &table,
        );

        for y in 0..5 {
            for x in 0..5 {
                let src_pixel = src[y * 5 + x];
                let expected = if src_pixel == 0 { 0xFF } else { 42 };
                assert_eq!(dest[y * 10 + x], expected, "at ({}, {})", x, y);
            }
        }
    }

    #[test]
    fn test_shift_table() {
        let table = create_shift_table(10);
        assert_eq!(table[0], 10);
        assert_eq!(table[245], 255);
        assert_eq!(table[246], 0);
        assert_eq!(table[255], 9);

        let table_neg = create_shift_table(-5);
        assert_eq!(table_neg[0], 251);
        assert_eq!(table_neg[5], 0);
        assert_eq!(table_neg[255], 250);
    }

    #[test]
    fn test_remap_zero_dimensions() {
        let mut buffer = vec![0u8; 100];
        let table = create_identity_table();

        // Should not crash
        buffer_remap(&mut buffer, 10, 0, 0, 0, 0, &table);
        buffer_remap(&mut buffer, 10, 0, 0, -1, 10, &table);
    }
}
```

### platform/src/blit/mod.rs (MODIFY)
Add after existing module declarations:
```rust
pub mod remap;
```

### platform/src/ffi/mod.rs (MODIFY)
Add remap FFI exports:
```rust
// =============================================================================
// Palette Remap FFI (replaces REMAP.ASM)
// =============================================================================

/// Apply remap table to buffer region
///
/// # Safety
/// - `buffer` must point to valid memory of at least `pitch * (y + height)` bytes
/// - `remap_table` must point to exactly 256 bytes
#[no_mangle]
pub unsafe extern "C" fn Platform_Buffer_Remap(
    buffer: *mut u8,
    pitch: i32,
    x: i32,
    y: i32,
    width: i32,
    height: i32,
    remap_table: *const u8,
) {
    if buffer.is_null() || remap_table.is_null() {
        return;
    }
    if pitch <= 0 || width <= 0 || height <= 0 {
        return;
    }

    let buf_size = (pitch * (y + height)) as usize;
    let buffer_slice = std::slice::from_raw_parts_mut(buffer, buf_size);
    let table: &[u8; 256] = &*(remap_table as *const [u8; 256]);

    blit::remap::buffer_remap(
        buffer_slice,
        pitch as usize,
        x, y,
        width, height,
        table,
    );
}

/// Apply remap table with transparency (skip color 0)
///
/// # Safety
/// Same as Platform_Buffer_Remap
#[no_mangle]
pub unsafe extern "C" fn Platform_Buffer_RemapTrans(
    buffer: *mut u8,
    pitch: i32,
    x: i32,
    y: i32,
    width: i32,
    height: i32,
    remap_table: *const u8,
) {
    if buffer.is_null() || remap_table.is_null() {
        return;
    }
    if pitch <= 0 || width <= 0 || height <= 0 {
        return;
    }

    let buf_size = (pitch * (y + height)) as usize;
    let buffer_slice = std::slice::from_raw_parts_mut(buffer, buf_size);
    let table: &[u8; 256] = &*(remap_table as *const [u8; 256]);

    blit::remap::buffer_remap_trans(
        buffer_slice,
        pitch as usize,
        x, y,
        width, height,
        table,
    );
}

/// Remap from source to destination
///
/// # Safety
/// - `dest` must point to valid memory
/// - `src` must point to valid memory
/// - `remap_table` must point to exactly 256 bytes
#[no_mangle]
pub unsafe extern "C" fn Platform_Buffer_RemapCopy(
    dest: *mut u8,
    dest_pitch: i32,
    dest_x: i32,
    dest_y: i32,
    src: *const u8,
    src_pitch: i32,
    src_x: i32,
    src_y: i32,
    width: i32,
    height: i32,
    remap_table: *const u8,
) {
    if dest.is_null() || src.is_null() || remap_table.is_null() {
        return;
    }
    if dest_pitch <= 0 || src_pitch <= 0 || width <= 0 || height <= 0 {
        return;
    }

    let dest_size = (dest_pitch * (dest_y + height)) as usize;
    let src_size = (src_pitch * (src_y + height)) as usize;
    let dest_slice = std::slice::from_raw_parts_mut(dest, dest_size);
    let src_slice = std::slice::from_raw_parts(src, src_size);
    let table: &[u8; 256] = &*(remap_table as *const [u8; 256]);

    blit::remap::buffer_remap_copy(
        dest_slice, dest_pitch as usize, dest_x, dest_y,
        src_slice, src_pitch as usize, src_x, src_y,
        width, height,
        table,
    );
}
```

## Verification Command
```bash
ralph-tasks/verify/check-palette-remap.sh
```

## Implementation Steps
1. Create `platform/src/blit/remap.rs` with all remap functions
2. Add `pub mod remap;` to `platform/src/blit/mod.rs`
3. Add FFI exports to `platform/src/ffi/mod.rs`
4. Run `cargo test --manifest-path platform/Cargo.toml -- remap` to verify unit tests
5. Run `cargo build --manifest-path platform/Cargo.toml --features cbindgen-run`
6. Verify header contains remap FFI exports
7. Run verification script

## Success Criteria
- All unit tests pass
- Identity remap leaves buffer unchanged
- Non-identity remaps transform colors correctly
- Transparent variant skips color 0
- Copy variants write to separate destination
- FFI exports appear in platform.h

## Completion Promise
When verification passes, output:
```
<promise>TASK_09C_COMPLETE</promise>
```

## Escape Hatch
If stuck after 10 iterations:
- Document what's blocking in `ralph-tasks/blocked/09c.md`
- List attempted approaches
- Output: `<promise>TASK_09C_BLOCKED</promise>`

## Max Iterations
10
