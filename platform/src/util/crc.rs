//! CRC32 calculation
//!
//! Replaces WIN32LIB/MISC/CRC.ASM
//!
//! This module provides two CRC algorithms:
//! 1. Standard IEEE CRC32 (for network sync, file validation)
//! 2. Westwood CRC (rotate-and-add, for MIX file lookups)

use once_cell::sync::Lazy;

/// CRC32 polynomial (reversed bit order)
const CRC32_POLYNOMIAL: u32 = 0xEDB88320;

/// Pre-computed CRC32 lookup table
static CRC_TABLE: Lazy<[u32; 256]> = Lazy::new(|| {
    let mut table = [0u32; 256];

    for i in 0..256 {
        let mut crc = i as u32;
        for _ in 0..8 {
            if crc & 1 != 0 {
                crc = (crc >> 1) ^ CRC32_POLYNOMIAL;
            } else {
                crc >>= 1;
            }
        }
        table[i] = crc;
    }

    table
});

/// Calculate CRC32 of a byte slice
///
/// # Arguments
/// * `data` - Byte slice to checksum
///
/// # Returns
/// 32-bit CRC value
pub fn crc32(data: &[u8]) -> u32 {
    let mut crc = 0xFFFFFFFF_u32;

    for &byte in data {
        let index = ((crc ^ byte as u32) & 0xFF) as usize;
        crc = CRC_TABLE[index] ^ (crc >> 8);
    }

    crc ^ 0xFFFFFFFF
}

/// Update existing CRC with more data
///
/// Use this for streaming CRC calculation when data arrives in chunks.
///
/// # Arguments
/// * `crc` - Previous CRC value (use CRC_INIT for first chunk)
/// * `data` - New data to incorporate
///
/// # Returns
/// Updated CRC value (not finalized - call crc32_finalize when done)
pub fn crc32_update(crc: u32, data: &[u8]) -> u32 {
    let mut crc = crc;

    for &byte in data {
        let index = ((crc ^ byte as u32) & 0xFF) as usize;
        crc = CRC_TABLE[index] ^ (crc >> 8);
    }

    crc
}

/// Initial CRC value for streaming calculation
pub const CRC_INIT: u32 = 0xFFFFFFFF;

/// Finalize CRC (XOR with 0xFFFFFFFF)
#[inline]
pub fn crc32_finalize(crc: u32) -> u32 {
    crc ^ 0xFFFFFFFF
}

/// Streaming CRC calculator
pub struct Crc32 {
    state: u32,
}

impl Crc32 {
    /// Create new CRC calculator
    pub fn new() -> Self {
        Self { state: CRC_INIT }
    }

    /// Update with more data
    pub fn update(&mut self, data: &[u8]) {
        self.state = crc32_update(self.state, data);
    }

    /// Get current CRC value
    pub fn finalize(&self) -> u32 {
        crc32_finalize(self.state)
    }

    /// Reset to initial state
    pub fn reset(&mut self) {
        self.state = CRC_INIT;
    }
}

impl Default for Crc32 {
    fn default() -> Self {
        Self::new()
    }
}

// =============================================================================
// Westwood CRC (Rotate-and-Add)
// =============================================================================
//
// This is Westwood Studios' custom hash algorithm used for MIX file lookups.
// It is NOT a true CRC - it's a faster rotate-and-add checksum.
//
// Algorithm (from CODE/CRC.H and CODE/CRC.CPP):
// 1. Process data in 4-byte chunks
// 2. For each chunk: CRC = rotate_left(CRC, 1) + chunk
// 3. Handle remaining bytes with partial accumulation
//
// The filename is converted to uppercase before hashing.
// The result is a signed 32-bit integer used as a key in binary search.

/// Calculate Westwood CRC of a byte slice.
///
/// This is the rotate-and-add algorithm used by MIX files for filename hashing.
/// The algorithm processes data in 32-bit chunks with left rotation.
///
/// # Arguments
/// * `data` - Byte slice to hash
///
/// # Returns
/// 32-bit hash value (can be interpreted as signed for binary search)
pub fn westwood_crc(data: &[u8]) -> u32 {
    let mut crc: u32 = 0;
    let mut staging: u32 = 0;
    let mut index: usize = 0;
    let mut bytes_remaining = data.len();
    let mut data_ptr = 0;

    // Process leading bytes until we're aligned to 4-byte boundary
    // Also handles case where total data < 4 bytes
    while bytes_remaining > 0 && index < 4 {
        staging |= (data[data_ptr] as u32) << (index * 8);
        index += 1;
        data_ptr += 1;
        bytes_remaining -= 1;

        if index == 4 {
            crc = crc.rotate_left(1).wrapping_add(staging);
            staging = 0;
            index = 0;
        }
    }

    // Process 4-byte chunks
    while bytes_remaining >= 4 {
        let chunk = u32::from_le_bytes([
            data[data_ptr],
            data[data_ptr + 1],
            data[data_ptr + 2],
            data[data_ptr + 3],
        ]);
        crc = crc.rotate_left(1).wrapping_add(chunk);
        data_ptr += 4;
        bytes_remaining -= 4;
    }

    // Handle remaining bytes (< 4)
    while bytes_remaining > 0 {
        staging |= (data[data_ptr] as u32) << (index * 8);
        index += 1;
        data_ptr += 1;
        bytes_remaining -= 1;
    }

    // If there are any remaining bytes in staging buffer, incorporate them
    if index > 0 {
        crc = crc.rotate_left(1).wrapping_add(staging);
    }

    crc
}

/// Calculate Westwood CRC of an uppercase ASCII string.
///
/// This is the typical usage for MIX file lookups - the filename
/// is converted to uppercase before hashing.
///
/// # Arguments
/// * `name` - Filename string (will be uppercased)
///
/// # Returns
/// 32-bit hash value for MIX file lookup
pub fn westwood_crc_filename(name: &str) -> u32 {
    let upper = name.to_ascii_uppercase();
    westwood_crc(upper.as_bytes())
}

/// Streaming Westwood CRC calculator
///
/// Maintains state for incremental hash computation.
pub struct WestwoodCrc {
    crc: u32,
    staging: u32,
    index: usize,
}

impl WestwoodCrc {
    /// Create new Westwood CRC calculator
    pub fn new() -> Self {
        Self {
            crc: 0,
            staging: 0,
            index: 0,
        }
    }

    /// Update with more data
    pub fn update(&mut self, data: &[u8]) {
        for &byte in data {
            self.staging |= (byte as u32) << (self.index * 8);
            self.index += 1;

            if self.index == 4 {
                self.crc = self.crc.rotate_left(1).wrapping_add(self.staging);
                self.staging = 0;
                self.index = 0;
            }
        }
    }

    /// Get current hash value (finalizes any pending bytes)
    pub fn finalize(&self) -> u32 {
        if self.index > 0 {
            self.crc.rotate_left(1).wrapping_add(self.staging)
        } else {
            self.crc
        }
    }

    /// Reset to initial state
    pub fn reset(&mut self) {
        self.crc = 0;
        self.staging = 0;
        self.index = 0;
    }
}

impl Default for WestwoodCrc {
    fn default() -> Self {
        Self::new()
    }
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_crc32_empty() {
        // CRC32 of empty data
        let crc = crc32(&[]);
        assert_eq!(crc, 0x00000000);
    }

    #[test]
    fn test_crc32_known_values() {
        // Known test vectors
        // "123456789" should produce 0xCBF43926
        let data = b"123456789";
        let crc = crc32(data);
        assert_eq!(crc, 0xCBF43926, "CRC32(\"123456789\") should be 0xCBF43926");

        // Single byte
        let crc_a = crc32(b"a");
        assert_eq!(crc_a, 0xE8B7BE43);

        // "test"
        let crc_test = crc32(b"test");
        assert_eq!(crc_test, 0xD87F7E0C);
    }

    #[test]
    fn test_crc32_streaming() {
        let data = b"Hello, World!";

        // Calculate in one shot
        let crc_one_shot = crc32(data);

        // Calculate in chunks
        let mut crc_streaming = CRC_INIT;
        crc_streaming = crc32_update(crc_streaming, b"Hello, ");
        crc_streaming = crc32_update(crc_streaming, b"World!");
        let crc_final = crc32_finalize(crc_streaming);

        assert_eq!(crc_one_shot, crc_final);
    }

    #[test]
    fn test_crc32_struct() {
        let mut crc = Crc32::new();
        crc.update(b"Hello, ");
        crc.update(b"World!");
        let result = crc.finalize();

        assert_eq!(result, crc32(b"Hello, World!"));
    }

    #[test]
    fn test_crc32_reset() {
        let mut crc = Crc32::new();
        crc.update(b"garbage");
        crc.reset();
        crc.update(b"test");

        assert_eq!(crc.finalize(), crc32(b"test"));
    }

    #[test]
    fn test_crc32_binary_data() {
        // Test with binary data including nulls
        let data = [0x00, 0xFF, 0x55, 0xAA, 0x00, 0x01, 0x02, 0x03];
        let crc = crc32(&data);

        // Verify it's consistent (not checking specific value)
        assert_eq!(crc, crc32(&data));
    }

    // =========================================================================
    // Westwood CRC Tests
    // =========================================================================

    #[test]
    fn test_westwood_crc_empty() {
        let crc = westwood_crc(&[]);
        assert_eq!(crc, 0);
    }

    #[test]
    fn test_westwood_crc_single_byte() {
        // Single byte 'A' (0x41) should produce 0x41 rotated left 1 = 0x82
        let crc = westwood_crc(b"A");
        assert_eq!(crc, 0x82);
    }

    #[test]
    fn test_westwood_crc_four_bytes() {
        // "TEST" = 0x54455354 (little endian: T=0x54, E=0x45, S=0x53, T=0x54)
        // As u32 LE: 0x54534554
        // rotate_left(0, 1) + 0x54534554 = 0x54534554
        let crc = westwood_crc(b"TEST");
        let expected = u32::from_le_bytes([b'T', b'E', b'S', b'T']);
        assert_eq!(crc, expected);
    }

    #[test]
    fn test_westwood_crc_eight_bytes() {
        // Two 4-byte chunks
        let crc = westwood_crc(b"TESTDATA");

        // First chunk: "TEST" = 0x54534554
        // After: crc = rotate_left(0, 1) + 0x54534554 = 0x54534554
        // Second chunk: "DATA" = 0x41544144
        // After: crc = rotate_left(0x54534554, 1) + 0x41544144
        //            = 0xA8A68AA8 + 0x41544144

        let chunk1 = u32::from_le_bytes([b'T', b'E', b'S', b'T']);
        let chunk2 = u32::from_le_bytes([b'D', b'A', b'T', b'A']);
        let expected = chunk1.rotate_left(1).wrapping_add(chunk2);

        assert_eq!(crc, expected);
    }

    #[test]
    fn test_westwood_crc_filename() {
        // Test case-insensitive filename hashing
        let crc_lower = westwood_crc_filename("test.shp");
        let crc_upper = westwood_crc_filename("TEST.SHP");
        let crc_mixed = westwood_crc_filename("TeSt.ShP");

        assert_eq!(crc_lower, crc_upper);
        assert_eq!(crc_lower, crc_mixed);
    }

    #[test]
    fn test_westwood_crc_streaming() {
        let data = b"TESTDATA";

        // One-shot
        let crc_one = westwood_crc(data);

        // Streaming
        let mut crc_stream = WestwoodCrc::new();
        crc_stream.update(b"TEST");
        crc_stream.update(b"DATA");

        assert_eq!(crc_one, crc_stream.finalize());
    }

    #[test]
    fn test_westwood_crc_streaming_unaligned() {
        let data = b"ABCDEFGHIJ";

        // One-shot
        let crc_one = westwood_crc(data);

        // Streaming with unaligned chunks
        let mut crc_stream = WestwoodCrc::new();
        crc_stream.update(b"ABC");
        crc_stream.update(b"DEFG");
        crc_stream.update(b"HI");
        crc_stream.update(b"J");

        assert_eq!(crc_one, crc_stream.finalize());
    }

    #[test]
    fn test_westwood_crc_known_filenames() {
        // These are well-known Red Alert asset filenames
        // The actual CRC values should match the original game's lookup table

        // Verify consistency - same input always produces same output
        let crc1 = westwood_crc_filename("CONQUER.MIX");
        let crc2 = westwood_crc_filename("CONQUER.MIX");
        assert_eq!(crc1, crc2);

        // Different files should have different hashes (usually)
        let crc_a = westwood_crc_filename("PALETTE.PAL");
        let crc_b = westwood_crc_filename("SHADOW.PAL");
        assert_ne!(crc_a, crc_b);
    }

    #[test]
    fn test_westwood_crc_reset() {
        let mut crc = WestwoodCrc::new();
        crc.update(b"garbage");
        crc.reset();
        crc.update(b"TEST");

        assert_eq!(crc.finalize(), westwood_crc(b"TEST"));
    }
}
