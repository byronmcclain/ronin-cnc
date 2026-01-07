//! LCW Compression/Decompression
//!
//! Replaces CODE/LCWCOMP.ASM and WIN32LIB/IFF/LCWUNCMP.ASM
//!
//! LCW is Westwood's variant of LZSS compression used for game assets.
//!
//! # Format
//!
//! Command byte interpretation:
//! - 0x00: End of data
//! - 0x01-0x7F: Back-reference (relative)
//!   - count = (byte >> 4) + 3 (range 3-10 bytes since bit 7=0)
//!   - offset = ((byte & 0x0F) << 8) | next_byte (range 1-4095)
//! - 0x80-0xBF: Short literal (1-64 bytes)
//!   - count = (byte & 0x3F) + 1
//! - 0xC0-0xFF: Long literal (1-16384 bytes)
//!   - count = ((byte & 0x3F) << 8 | next_byte) + 1

use crate::error::PlatformError;

/// Maximum back-reference offset (12 bits)
const MAX_OFFSET: usize = 4095;

/// Maximum match length for back-ref (bits [6:4] + 3 = 7+3 = 10)
const MAX_BACKREF_MATCH: usize = 10;

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
            // Literal copy (0x80-0xFF)
            let count = if cmd & 0x40 != 0 {
                // Long form (0xC0-0xFF): 14-bit count
                if src_pos >= source.len() {
                    return Err(PlatformError::InvalidData);
                }
                let low = source[src_pos] as usize;
                src_pos += 1;
                (((cmd & 0x3F) as usize) << 8 | low) + 1
            } else {
                // Short form (0x80-0xBF): 6-bit count
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
            // Back-reference (0x01-0x7F)
            // Since bit 7=0, bits[6:4] can be 0-7, giving count 3-10
            let count = ((cmd >> 4) & 0x07) as usize + 3;

            if src_pos >= source.len() {
                return Err(PlatformError::InvalidData);
            }

            let offset = (((cmd & 0x0F) as usize) << 8) | source[src_pos] as usize;
            src_pos += 1;

            if offset == 0 || offset > dst_pos {
                return Err(PlatformError::InvalidData);
            }

            // Copy from back-reference (handle overlapping for RLE)
            let ref_start = dst_pos - offset;
            for i in 0..count {
                if dst_pos >= dest.len() {
                    break;
                }
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
pub fn lcw_compress(source: &[u8], dest: &mut [u8]) -> Result<usize, PlatformError> {
    let mut src_pos = 0;
    let mut dst_pos = 0;

    while src_pos < source.len() {
        // Try to find a back-reference
        let (match_len, match_offset) = find_match(source, src_pos);

        if match_len >= MIN_MATCH {
            // Calculate back-reference encoding
            // count-3 goes in bits[6:4] (0-7), offset_high in bits[3:0]
            let count_field = ((match_len - 3) & 0x07) << 4;
            let offset_high = (match_offset >> 8) & 0x0F;
            let offset_low = match_offset & 0xFF;
            let first_byte = (count_field | offset_high) as u8;

            // If first byte would be 0x00 (END marker), we can't use this back-ref
            // This happens when count=3 and offset < 256
            if first_byte == 0 {
                // Fall through to literal handling
            } else {
                // Emit back-reference
                if dst_pos + 2 > dest.len() {
                    return Err(PlatformError::BufferTooSmall);
                }

                dest[dst_pos] = first_byte;
                dest[dst_pos + 1] = offset_low as u8;
                dst_pos += 2;
                src_pos += match_len;
                continue;
            }
        }

        // Either no match or match would produce 0x00 byte - emit literal(s)
        let mut literal_count = 1;
        while src_pos + literal_count < source.len() && literal_count < 63 {
            let (next_match_len, next_match_offset) = find_match(source, src_pos + literal_count);
            if next_match_len >= MIN_MATCH {
                let count_field = ((next_match_len - 3) & 0x07) << 4;
                let offset_high = (next_match_offset >> 8) & 0x0F;
                if (count_field | offset_high) != 0 {
                    break;
                }
            }
            literal_count += 1;
        }

        // Emit literal command (short form 0x80-0xBF)
        if dst_pos + 1 + literal_count > dest.len() {
            return Err(PlatformError::BufferTooSmall);
        }
        dest[dst_pos] = 0x80 | ((literal_count - 1) as u8);
        dst_pos += 1;

        // Copy literal bytes
        for i in 0..literal_count {
            dest[dst_pos] = source[src_pos + i];
            dst_pos += 1;
        }
        src_pos += literal_count;
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
/// Returns (match_length, offset) where offset is distance back.
/// Prefers smaller offsets when lengths are equal (for better compression).
fn find_match(data: &[u8], pos: usize) -> (usize, usize) {
    if pos == 0 {
        return (0, 0);
    }

    let search_start = pos.saturating_sub(MAX_OFFSET);
    let max_match = MAX_BACKREF_MATCH.min(data.len() - pos);

    let mut best_len = 0;
    let mut best_offset = 0;

    // Search from closest to farthest to prefer smaller offsets
    for ref_pos in (search_start..pos).rev() {
        let mut len = 0;
        while len < max_match && data[ref_pos + len] == data[pos + len] {
            len += 1;
        }

        // Use >= to prefer smaller offsets (we're iterating in reverse)
        if len >= MIN_MATCH && len >= best_len {
            best_len = len;
            best_offset = pos - ref_pos;

            // If we found max match with minimum offset, stop searching
            if best_len == MAX_BACKREF_MATCH && best_offset == 1 {
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
    fn test_lcw_decompress_literals() {
        // Short literal: 0x83 = 0x80 | 3, so 4 bytes follow
        let compressed = [0x83, b'H', b'e', b'l', b'l', 0x00];
        let mut dest = [0u8; 100];

        let len = lcw_decompress(&compressed, &mut dest).unwrap();
        assert_eq!(len, 4);
        assert_eq!(&dest[..4], b"Hell");
    }

    #[test]
    fn test_lcw_decompress_single_literal() {
        // 0x80 | 0 = 0x80, so 1 byte follows
        let compressed = [0x80, b'X', 0x00];
        let mut dest = [0u8; 100];

        let len = lcw_decompress(&compressed, &mut dest).unwrap();
        assert_eq!(len, 1);
        assert_eq!(dest[0], b'X');
    }

    #[test]
    fn test_lcw_decompress_back_reference() {
        // Back-ref format: byte[0] = (count-3)<<4 | offset_high, byte[1] = offset_low
        // Since bit 7 must be 0, bits[6:4] encode count-3 (0-7), bits[3:0] encode offset_high
        //
        // For count=4, offset=4: byte[0] = (4-3)<<4 | 0 = 0x10, byte[1] = 0x04
        let compressed = [
            0x83, b'A', b'B', b'C', b'D',  // 4 literals
            0x10, 0x04,                     // back-ref: count=(1)+3=4, offset=0x004=4
            0x00,                           // end
        ];
        let mut dest = [0u8; 100];

        let len = lcw_decompress(&compressed, &mut dest).unwrap();
        assert_eq!(len, 8);
        assert_eq!(&dest[..8], b"ABCDABCD");
    }

    #[test]
    fn test_lcw_decompress_overlapping_back_reference() {
        // This tests RLE-like compression where we back-ref 1 byte repeatedly
        // Literal "A", then back-ref count=7+3=10, offset=1
        // (max count-3 is 7 since bits[6:4] with bit7=0 gives 0-7)
        // Should produce 1 + 10 = 11 A's
        let compressed = [
            0x80, b'A',     // 1 literal 'A'
            0x70, 0x01,     // back-ref: count=7+3=10, offset=1
            0x00,           // end
        ];
        let mut dest = [0u8; 100];

        let len = lcw_decompress(&compressed, &mut dest).unwrap();
        assert_eq!(len, 11);  // 1 + 10 = 11
        assert!(dest[..11].iter().all(|&b| b == b'A'));
    }

    #[test]
    fn test_lcw_decompress_empty() {
        // Just end marker
        let compressed = [0x00];
        let mut dest = [0u8; 100];

        let len = lcw_decompress(&compressed, &mut dest).unwrap();
        assert_eq!(len, 0);
    }

    #[test]
    fn test_lcw_decompress_long_literal() {
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
    fn test_lcw_compress_simple() {
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
    fn test_lcw_compress_with_repetition() {
        // Data with repetition should compress
        let source = b"AAAAAAAABBBBBBBBCCCCCCCC";
        let mut compressed = [0u8; 100];

        let comp_len = lcw_compress(source, &mut compressed).unwrap();

        // Decompress and verify
        let mut decompressed = [0u8; 100];
        let decomp_len = lcw_decompress(&compressed[..comp_len], &mut decompressed).unwrap();
        assert_eq!(&decompressed[..decomp_len], source.as_slice());
    }

    #[test]
    fn test_lcw_round_trip_random() {
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
    fn test_lcw_round_trip_all_zeros() {
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
    fn test_lcw_round_trip_all_same() {
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
    fn test_lcw_decompress_invalid_offset() {
        // Back-reference with offset larger than output so far
        // We have 1 byte of output ("A"), but try to back-ref with offset 16
        // back-ref: count=3, offset=0x010 (16)
        // byte[0] = (3-3)<<4 | (16>>8) = 0 | 0 = 0x00 -- END marker, won't work
        // byte[0] = (4-3)<<4 | (16>>8) = 0x10 | 0 = 0x10, byte[1] = 0x10
        // That gives count=4, offset=16 which is too large for 1 byte output
        let compressed = [
            0x80, b'A',     // 1 literal
            0x10, 0x10,     // back-ref: count=4, offset=16 (too large!)
            0x00,
        ];
        let mut dest = [0u8; 100];

        let result = lcw_decompress(&compressed, &mut dest);
        assert!(result.is_err());
    }

    #[test]
    fn test_lcw_compress_empty() {
        let source: [u8; 0] = [];
        let mut compressed = [0u8; 10];

        let comp_len = lcw_compress(&source, &mut compressed).unwrap();
        assert_eq!(comp_len, 1);  // Just end marker
        assert_eq!(compressed[0], 0x00);
    }

    #[test]
    fn test_lcw_max_compressed_size() {
        assert!(lcw_max_compressed_size(0) >= 1);
        assert!(lcw_max_compressed_size(100) >= 100);
        assert!(lcw_max_compressed_size(1000) >= 1000);
    }
}
