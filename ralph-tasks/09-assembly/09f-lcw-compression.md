# Task 09f: LCW Compression/Decompression

## Dependencies
- Task 09a must be complete (platform crate structure)
- Understanding of LCW/LZSS compression format

## Context
LCW (Lempel-Ziv-Welch variant) is Westwood's custom compression format used throughout Red Alert for graphics, animations, and game data. The original implementation is in CODE/LCWCOMP.ASM and WIN32LIB/IFF/LCWUNCMP.ASM. **This is a critical task** - without LCW decompression, the game cannot load MIX files or display most graphics.

## Objective
Implement LCW compression and decompression:
1. `lcw_decompress()` - Decompress LCW-encoded data
2. `lcw_compress()` - Compress data using LCW algorithm
3. Round-trip testing (compress then decompress equals original)
4. FFI exports and comprehensive tests

## Deliverables
- [ ] `platform/src/lcw.rs` - Complete LCW implementation
- [ ] FFI exports for compress/decompress
- [ ] Unit tests with known test vectors
- [ ] Round-trip compression tests
- [ ] Header contains FFI exports
- [ ] Verification passes

## Technical Background

### LCW Format Overview
LCW is an LZSS variant with three command types encoded in the first byte:

**Command Format:**
```
Byte 0 bits: [7] [6] [5-0]

If bit 7 = 1: LITERAL COPY
  - If bit 6 = 0: count = bits[5:0] + 1 (1-64 literals)
  - If bit 6 = 1: count = ((bits[5:0] << 8) | byte1) + 1 (1-16384 literals)
  - Copy 'count' bytes directly from source

If bit 7 = 0 and byte != 0: BACK REFERENCE (relative)
  - count = (bits[7:4] >> 4) + 3 (3-18 bytes)
  - offset = ((bits[3:0] << 8) | byte1) (1-4095 back)
  - Copy 'count' bytes from (dest - offset)

If byte = 0: END OF DATA
  - Stop decompression
```

### Important Details
1. **Overlapping copies**: Back-references can overlap (used for RLE-like patterns)
2. **Relative offsets**: Back-references are relative to current output position
3. **End marker**: Single 0x00 byte marks end of compressed data
4. **No header**: LCW data has no header; size must be tracked externally

### Westwood-Specific Variations
The exact format may have variations. Test with actual game data if available.

## Files to Create/Modify

### platform/src/lcw.rs (NEW)
```rust
//! LCW Compression/Decompression
//!
//! Replaces CODE/LCWCOMP.ASM and WIN32LIB/IFF/LCWUNCMP.ASM
//!
//! LCW is Westwood's variant of LZSS compression used for game assets.

use crate::error::PlatformError;

/// Maximum back-reference offset (12 bits)
const MAX_OFFSET: usize = 4095;

/// Maximum match length for short form (4 bits + 3)
const MAX_SHORT_MATCH: usize = 18;

/// Minimum match length for back-reference
const MIN_MATCH: usize = 3;

/// Decompress LCW-encoded data
///
/// # Arguments
/// * `source` - LCW-compressed input data
/// * `dest` - Output buffer (must be large enough for decompressed data)
///
/// # Returns
/// Number of bytes written to dest, or error
///
/// # Format
/// LCW uses variable-length commands:
/// - 0x80-0xBF: Short literal (1-64 bytes)
/// - 0xC0-0xFF: Long literal (1-16384 bytes)
/// - 0x01-0x7F: Back-reference (3-18 bytes, 1-4095 offset)
/// - 0x00: End of data
///
/// # Example
/// ```ignore
/// let compressed = [0x83, b'H', b'e', b'l', b'l', 0x00];  // "Hell" + end
/// let mut dest = [0u8; 100];
/// let len = lcw_decompress(&compressed, &mut dest)?;
/// assert_eq!(&dest[..len], b"Hell");
/// ```
pub fn lcw_decompress(source: &[u8], dest: &mut [u8]) -> Result<usize, PlatformError> {
    let mut src_pos = 0;
    let mut dst_pos = 0;

    while src_pos < source.len() {
        let cmd = source[src_pos];
        src_pos += 1;

        if cmd == 0 {
            // End of data
            break;
        } else if cmd & 0x80 != 0 {
            // Literal copy
            let count = if cmd & 0x40 != 0 {
                // Long form: 14-bit count
                if src_pos >= source.len() {
                    return Err(PlatformError::InvalidData);
                }
                let low = source[src_pos] as usize;
                src_pos += 1;
                (((cmd & 0x3F) as usize) << 8 | low) + 1
            } else {
                // Short form: 6-bit count
                (cmd & 0x3F) as usize + 1
            };

            // Copy literal bytes
            for _ in 0..count {
                if src_pos >= source.len() || dst_pos >= dest.len() {
                    break;
                }
                dest[dst_pos] = source[src_pos];
                dst_pos += 1;
                src_pos += 1;
            }
        } else {
            // Back-reference
            let count = ((cmd >> 4) & 0x0F) as usize + 3;  // 3-18 bytes

            if src_pos >= source.len() {
                return Err(PlatformError::InvalidData);
            }

            let offset = (((cmd & 0x0F) as usize) << 8) | source[src_pos] as usize;
            src_pos += 1;

            if offset == 0 || offset > dst_pos {
                // Invalid offset
                return Err(PlatformError::InvalidData);
            }

            // Copy from back-reference (handle overlapping)
            let ref_start = dst_pos - offset;
            for i in 0..count {
                if dst_pos >= dest.len() {
                    break;
                }
                // Must read byte-by-byte to handle overlap correctly
                dest[dst_pos] = dest[ref_start + i];
                dst_pos += 1;
            }
        }
    }

    Ok(dst_pos)
}

/// Compress data using LCW algorithm
///
/// # Arguments
/// * `source` - Uncompressed input data
/// * `dest` - Output buffer for compressed data
///
/// # Returns
/// Number of bytes written to dest, or error if dest is too small
///
/// # Note
/// This implementation prioritizes correctness over compression ratio.
/// A production implementation would use more sophisticated matching.
pub fn lcw_compress(source: &[u8], dest: &mut [u8]) -> Result<usize, PlatformError> {
    let mut src_pos = 0;
    let mut dst_pos = 0;

    while src_pos < source.len() {
        // Try to find a back-reference
        let (match_len, match_offset) = find_match(source, src_pos);

        if match_len >= MIN_MATCH {
            // Emit back-reference
            if dst_pos + 2 > dest.len() {
                return Err(PlatformError::BufferTooSmall);
            }

            let count_field = ((match_len - 3) & 0x0F) << 4;
            let offset_high = (match_offset >> 8) & 0x0F;
            let offset_low = match_offset & 0xFF;

            dest[dst_pos] = (count_field | offset_high) as u8;
            dest[dst_pos + 1] = offset_low as u8;
            dst_pos += 2;
            src_pos += match_len;
        } else {
            // Emit literal(s)
            // Find how many literals to emit
            let mut literal_count = 1;
            while src_pos + literal_count < source.len() && literal_count < 63 {
                // Check if next position has a good match
                let (next_match_len, _) = find_match(source, src_pos + literal_count);
                if next_match_len >= MIN_MATCH {
                    break;
                }
                literal_count += 1;
            }

            // Emit literal command
            if literal_count <= 64 {
                // Short form
                if dst_pos + 1 + literal_count > dest.len() {
                    return Err(PlatformError::BufferTooSmall);
                }
                dest[dst_pos] = 0x80 | ((literal_count - 1) as u8);
                dst_pos += 1;
            } else {
                // Long form (shouldn't happen with our limit of 63)
                if dst_pos + 2 + literal_count > dest.len() {
                    return Err(PlatformError::BufferTooSmall);
                }
                let count_minus_1 = literal_count - 1;
                dest[dst_pos] = 0xC0 | ((count_minus_1 >> 8) as u8);
                dest[dst_pos + 1] = (count_minus_1 & 0xFF) as u8;
                dst_pos += 2;
            }

            // Copy literal bytes
            for i in 0..literal_count {
                dest[dst_pos] = source[src_pos + i];
                dst_pos += 1;
            }
            src_pos += literal_count;
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

/// Find best match at current position
///
/// Returns (match_length, offset) where offset is distance back
fn find_match(data: &[u8], pos: usize) -> (usize, usize) {
    if pos == 0 {
        return (0, 0);
    }

    let search_start = pos.saturating_sub(MAX_OFFSET);
    let max_match = MAX_SHORT_MATCH.min(data.len() - pos);

    let mut best_len = 0;
    let mut best_offset = 0;

    for ref_pos in search_start..pos {
        let mut len = 0;
        while len < max_match && data[ref_pos + len] == data[pos + len] {
            len += 1;
        }

        if len >= MIN_MATCH && len > best_len {
            best_len = len;
            best_offset = pos - ref_pos;

            // If we found max match, stop searching
            if best_len == MAX_SHORT_MATCH {
                break;
            }
        }
    }

    (best_len, best_offset)
}

/// Calculate maximum compressed size (worst case: all literals)
///
/// Worst case: 1 command byte per 64 literal bytes, plus end marker
pub fn lcw_max_compressed_size(uncompressed_size: usize) -> usize {
    // Each literal group: 1 cmd byte + up to 64 data bytes
    // So worst case is about (size * 65 / 64) + 1
    (uncompressed_size * 65 / 64) + 2
}

/// Decompress LCW data, allocating output buffer
///
/// # Arguments
/// * `source` - LCW-compressed data
/// * `expected_size` - Expected decompressed size (hint for allocation)
///
/// # Returns
/// Decompressed data as Vec<u8>
pub fn lcw_decompress_alloc(source: &[u8], expected_size: usize) -> Result<Vec<u8>, PlatformError> {
    let mut dest = vec![0u8; expected_size];
    let actual_size = lcw_decompress(source, &mut dest)?;
    dest.truncate(actual_size);
    Ok(dest)
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_decompress_literals() {
        // Short literal: 0x83 = 0x80 | 3, so 4 bytes follow
        let compressed = [0x83, b'H', b'e', b'l', b'l', 0x00];
        let mut dest = [0u8; 100];

        let len = lcw_decompress(&compressed, &mut dest).unwrap();
        assert_eq!(len, 4);
        assert_eq!(&dest[..4], b"Hell");
    }

    #[test]
    fn test_decompress_single_literal() {
        // 0x80 | 0 = 0x80, so 1 byte follows
        let compressed = [0x80, b'X', 0x00];
        let mut dest = [0u8; 100];

        let len = lcw_decompress(&compressed, &mut dest).unwrap();
        assert_eq!(len, 1);
        assert_eq!(dest[0], b'X');
    }

    #[test]
    fn test_decompress_back_reference() {
        // First: 3 literals "ABC"
        // Then: back-ref to copy from start
        // cmd = 0x03 means count=3 (from high nibble 0) + 3 = 3, offset high = 3
        // But we need to construct this carefully
        //
        // Let's do: literals "ABCD" then back-ref 3 bytes from offset 4
        // That copies "ABC" again
        let compressed = [
            0x83, b'A', b'B', b'C', b'D',  // 4 literals
            0x03, 0x04,                     // back-ref: count=(0)+3=3, offset=0x004=4
            0x00,                           // end
        ];
        let mut dest = [0u8; 100];

        let len = lcw_decompress(&compressed, &mut dest).unwrap();
        assert_eq!(len, 7);
        assert_eq!(&dest[..7], b"ABCDABC");
    }

    #[test]
    fn test_decompress_overlapping_back_reference() {
        // This tests RLE-like compression where we back-ref 1 byte repeatedly
        // Literal "A", then back-ref count=5, offset=1
        // Should produce "AAAAAA" (1 + 5 = 6 A's... wait, count is 3+ so 8 bytes)
        // Let's recalculate: count field 0x50 >> 4 = 5, +3 = 8 bytes
        let compressed = [
            0x80, b'A',     // 1 literal 'A'
            0x50, 0x01,     // back-ref: count=5+3=8, offset=1
            0x00,           // end
        ];
        let mut dest = [0u8; 100];

        let len = lcw_decompress(&compressed, &mut dest).unwrap();
        assert_eq!(len, 9);  // 1 + 8 = 9
        assert!(dest[..9].iter().all(|&b| b == b'A'));
    }

    #[test]
    fn test_decompress_empty() {
        // Just end marker
        let compressed = [0x00];
        let mut dest = [0u8; 100];

        let len = lcw_decompress(&compressed, &mut dest).unwrap();
        assert_eq!(len, 0);
    }

    #[test]
    fn test_decompress_long_literal() {
        // Long literal form: 0xC0 | high byte, low byte, then data
        // 0xC0 | 0x00 = 0xC0, next byte 0x40 = count of 0x0040 + 1 = 65 bytes
        let mut compressed = vec![0xC0, 0x40];  // 65 bytes
        compressed.extend(vec![0x55u8; 65]);
        compressed.push(0x00);  // end

        let mut dest = [0u8; 200];
        let len = lcw_decompress(&compressed, &mut dest).unwrap();
        assert_eq!(len, 65);
        assert!(dest[..65].iter().all(|&b| b == 0x55));
    }

    #[test]
    fn test_compress_simple() {
        let source = b"Hello, World!";
        let mut compressed = [0u8; 100];

        let comp_len = lcw_compress(source, &mut compressed).unwrap();
        assert!(comp_len > 0);
        assert_eq!(compressed[comp_len - 1], 0x00);  // End marker

        // Decompress and verify
        let mut decompressed = [0u8; 100];
        let decomp_len = lcw_decompress(&compressed[..comp_len], &mut decompressed).unwrap();
        assert_eq!(decomp_len, source.len());
        assert_eq!(&decompressed[..decomp_len], source.as_slice());
    }

    #[test]
    fn test_compress_with_repetition() {
        // Data with repetition should compress
        let source = b"AAAAAAAABBBBBBBBCCCCCCCC";
        let mut compressed = [0u8; 100];

        let comp_len = lcw_compress(source, &mut compressed).unwrap();

        // Decompress and verify
        let mut decompressed = [0u8; 100];
        let decomp_len = lcw_decompress(&compressed[..comp_len], &mut decompressed).unwrap();
        assert_eq!(&decompressed[..decomp_len], source.as_slice());

        // With repetition, compressed should be smaller (or at least work)
        // Note: our simple compressor may not be optimal
    }

    #[test]
    fn test_round_trip_random() {
        // Test with pseudo-random data
        let mut source = [0u8; 256];
        for i in 0..256 {
            source[i] = ((i * 73 + 41) % 256) as u8;
        }

        let mut compressed = [0u8; 512];
        let comp_len = lcw_compress(&source, &mut compressed).unwrap();

        let mut decompressed = [0u8; 256];
        let decomp_len = lcw_decompress(&compressed[..comp_len], &mut decompressed).unwrap();

        assert_eq!(decomp_len, 256);
        assert_eq!(&decompressed, &source);
    }

    #[test]
    fn test_round_trip_all_zeros() {
        // All zeros should compress well
        let source = [0u8; 1000];
        let mut compressed = [0u8; 2000];

        let comp_len = lcw_compress(&source, &mut compressed).unwrap();

        let mut decompressed = [0u8; 1000];
        let decomp_len = lcw_decompress(&compressed[..comp_len], &mut decompressed).unwrap();

        assert_eq!(decomp_len, 1000);
        assert!(decompressed.iter().all(|&b| b == 0));
    }

    #[test]
    fn test_round_trip_all_same() {
        // All same value should compress well
        let source = [0x42u8; 500];
        let mut compressed = [0u8; 1000];

        let comp_len = lcw_compress(&source, &mut compressed).unwrap();

        let mut decompressed = [0u8; 500];
        let decomp_len = lcw_decompress(&compressed[..comp_len], &mut decompressed).unwrap();

        assert_eq!(decomp_len, 500);
        assert!(decompressed.iter().all(|&b| b == 0x42));
    }

    #[test]
    fn test_decompress_invalid_offset() {
        // Back-reference with offset larger than output so far
        let compressed = [
            0x80, b'A',     // 1 literal
            0x00, 0x10,     // back-ref with offset 16 (too large!)
            0x00,
        ];
        let mut dest = [0u8; 100];

        let result = lcw_decompress(&compressed, &mut dest);
        assert!(result.is_err());
    }

    #[test]
    fn test_compress_empty() {
        let source: [u8; 0] = [];
        let mut compressed = [0u8; 10];

        let comp_len = lcw_compress(&source, &mut compressed).unwrap();
        assert_eq!(comp_len, 1);  // Just end marker
        assert_eq!(compressed[0], 0x00);
    }

    #[test]
    fn test_max_compressed_size() {
        assert!(lcw_max_compressed_size(0) >= 1);
        assert!(lcw_max_compressed_size(100) >= 100);
        assert!(lcw_max_compressed_size(1000) >= 1000);
    }
}
```

### platform/src/lib.rs (MODIFY)
Add after other module declarations:
```rust
pub mod lcw;
```

### platform/src/ffi/mod.rs (MODIFY)
Add LCW FFI exports:
```rust
use crate::lcw;

// =============================================================================
// LCW Compression FFI (replaces LCWCOMP.ASM, LCWUNCMP.ASM)
// =============================================================================

/// Decompress LCW-encoded data
///
/// # Arguments
/// * `source` - Pointer to compressed data
/// * `source_size` - Size of compressed data
/// * `dest` - Pointer to output buffer
/// * `dest_size` - Size of output buffer
///
/// # Returns
/// Number of bytes written, or -1 on error
///
/// # Safety
/// - `source` must point to valid memory of at least `source_size` bytes
/// - `dest` must point to valid memory of at least `dest_size` bytes
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
    if source_size <= 0 || dest_size <= 0 {
        return -1;
    }

    let src_slice = std::slice::from_raw_parts(source, source_size as usize);
    let dst_slice = std::slice::from_raw_parts_mut(dest, dest_size as usize);

    match lcw::lcw_decompress(src_slice, dst_slice) {
        Ok(size) => size as i32,
        Err(_) => -1,
    }
}

/// Compress data using LCW algorithm
///
/// # Arguments
/// * `source` - Pointer to uncompressed data
/// * `source_size` - Size of uncompressed data
/// * `dest` - Pointer to output buffer
/// * `dest_size` - Size of output buffer
///
/// # Returns
/// Number of bytes written, or -1 on error
///
/// # Safety
/// - `source` must point to valid memory of at least `source_size` bytes
/// - `dest` must point to valid memory of at least `dest_size` bytes
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
    if source_size <= 0 || dest_size <= 0 {
        return -1;
    }

    let src_slice = std::slice::from_raw_parts(source, source_size as usize);
    let dst_slice = std::slice::from_raw_parts_mut(dest, dest_size as usize);

    match lcw::lcw_compress(src_slice, dst_slice) {
        Ok(size) => size as i32,
        Err(_) => -1,
    }
}

/// Calculate maximum compressed size for given input size
///
/// Use this to allocate output buffer for compression.
#[no_mangle]
pub extern "C" fn Platform_LCW_MaxCompressedSize(uncompressed_size: i32) -> i32 {
    if uncompressed_size <= 0 {
        return 1;  // Just end marker
    }
    lcw::lcw_max_compressed_size(uncompressed_size as usize) as i32
}
```

## Verification Command
```bash
ralph-tasks/verify/check-lcw-compression.sh
```

## Implementation Steps
1. Create `platform/src/lcw.rs` with LCW implementation
2. Add `pub mod lcw;` to `platform/src/lib.rs`
3. Add FFI exports to `platform/src/ffi/mod.rs`
4. Run `cargo test --manifest-path platform/Cargo.toml -- lcw` to verify unit tests
5. Run `cargo build --manifest-path platform/Cargo.toml --features cbindgen-run`
6. Verify header contains LCW FFI exports
7. Run verification script

## Success Criteria
- All unit tests pass
- Decompression handles all command types
- Round-trip compression works for all test patterns
- Invalid data produces errors (not crashes)
- FFI exports appear in platform.h

## Common Issues

### Decompression produces wrong output
- Check command type detection (bit 7, bit 6)
- Verify count calculations (+1, +3)
- Ensure overlapping back-references copy byte-by-byte

### Compression doesn't round-trip
- Check literal count encoding
- Verify end marker is written
- Ensure back-reference offset is calculated correctly

### Invalid offset not detected
- Offset 0 is invalid (would be self-reference)
- Offset > current position is invalid

## Completion Promise
When verification passes, output:
```
<promise>TASK_09F_COMPLETE</promise>
```

## Escape Hatch
If stuck after 20 iterations:
- Document what's blocking in `ralph-tasks/blocked/09f.md`
- List attempted approaches
- Note: This is a critical task; may need to reference original ASM
- Output: `<promise>TASK_09F_BLOCKED</promise>`

## Max Iterations
20
