//! CRC32 calculation
//!
//! Replaces WIN32LIB/MISC/CRC.ASM
//!
//! Uses the standard CRC32 polynomial 0xEDB88320 (IEEE 802.3)

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
}
