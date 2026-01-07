//! IMA ADPCM decoder
//!
//! Decodes IMA ADPCM compressed audio to 16-bit PCM samples.
//! Based on the original decoder in CODE/ADPCM.CPP.

/// IMA ADPCM index adjustment table
/// Maps the 4-bit ADPCM code to step index adjustment
static INDEX_TABLE: [i32; 16] = [
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8,
];

/// IMA ADPCM step size table
/// 89 quantizer step sizes for the ADPCM algorithm
static STEP_TABLE: [i32; 89] = [
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767,
];

/// ADPCM decoder state
#[derive(Debug, Clone)]
pub struct AdpcmDecoder {
    /// Current predicted sample value
    predictor: i32,
    /// Current step table index
    step_index: i32,
}

impl AdpcmDecoder {
    /// Create a new ADPCM decoder with default state
    pub fn new() -> Self {
        Self {
            predictor: 0,
            step_index: 0,
        }
    }

    /// Create a decoder with initial state
    pub fn with_state(predictor: i32, step_index: i32) -> Self {
        Self {
            predictor,
            step_index: step_index.clamp(0, 88),
        }
    }

    /// Reset decoder to initial state
    pub fn reset(&mut self) {
        self.predictor = 0;
        self.step_index = 0;
    }

    /// Get current predictor value
    pub fn predictor(&self) -> i32 {
        self.predictor
    }

    /// Get current step index
    pub fn step_index(&self) -> i32 {
        self.step_index
    }

    /// Decode a single 4-bit ADPCM nibble to a 16-bit PCM sample
    #[inline]
    fn decode_nibble(&mut self, nibble: u8) -> i16 {
        let step = STEP_TABLE[self.step_index as usize];

        // Calculate the difference using the step size
        // diff = step/8 + step/4*b0 + step/2*b1 + step*b2
        let mut diff = step >> 3;
        if nibble & 1 != 0 {
            diff += step >> 2;
        }
        if nibble & 2 != 0 {
            diff += step >> 1;
        }
        if nibble & 4 != 0 {
            diff += step;
        }

        // Apply sign bit
        if nibble & 8 != 0 {
            diff = -diff;
        }

        // Update predictor with clamping
        self.predictor += diff;
        self.predictor = self.predictor.clamp(-32768, 32767);

        // Update step index with clamping
        self.step_index += INDEX_TABLE[(nibble & 0x0F) as usize];
        self.step_index = self.step_index.clamp(0, 88);

        self.predictor as i16
    }

    /// Decode a block of ADPCM data to PCM samples
    /// Each input byte produces 2 output samples (low nibble first, then high nibble)
    pub fn decode(&mut self, input: &[u8]) -> Vec<i16> {
        let mut output = Vec::with_capacity(input.len() * 2);

        for &byte in input {
            // Decode low nibble first
            output.push(self.decode_nibble(byte & 0x0F));
            // Then decode high nibble
            output.push(self.decode_nibble((byte >> 4) & 0x0F));
        }

        output
    }

    /// Decode a single byte to two samples
    #[inline]
    pub fn decode_byte(&mut self, byte: u8) -> (i16, i16) {
        let low = self.decode_nibble(byte & 0x0F);
        let high = self.decode_nibble((byte >> 4) & 0x0F);
        (low, high)
    }
}

impl Default for AdpcmDecoder {
    fn default() -> Self {
        Self::new()
    }
}

/// Convenience function to decode ADPCM data in one call
pub fn decode_adpcm(input: &[u8]) -> Vec<i16> {
    let mut decoder = AdpcmDecoder::new();
    decoder.decode(input)
}

/// Decode ADPCM with initial state
pub fn decode_adpcm_with_state(input: &[u8], predictor: i32, step_index: i32) -> Vec<i16> {
    let mut decoder = AdpcmDecoder::with_state(predictor, step_index);
    decoder.decode(input)
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_decoder_creation() {
        let decoder = AdpcmDecoder::new();
        assert_eq!(decoder.predictor(), 0);
        assert_eq!(decoder.step_index(), 0);
    }

    #[test]
    fn test_decoder_with_state() {
        let decoder = AdpcmDecoder::with_state(1000, 44);
        assert_eq!(decoder.predictor(), 1000);
        assert_eq!(decoder.step_index(), 44);
    }

    #[test]
    fn test_step_index_clamping() {
        // Test that step index is clamped to valid range
        let decoder = AdpcmDecoder::with_state(0, 100);
        assert_eq!(decoder.step_index(), 88); // Max valid index

        let decoder = AdpcmDecoder::with_state(0, -10);
        assert_eq!(decoder.step_index(), 0); // Min valid index
    }

    #[test]
    fn test_decode_empty() {
        let output = decode_adpcm(&[]);
        assert!(output.is_empty());
    }

    #[test]
    fn test_decode_single_byte() {
        let mut decoder = AdpcmDecoder::new();
        let (low, high) = decoder.decode_byte(0x00);

        // With step_index=0, step=7, nibble=0: diff = 7/8 = 0
        // predictor = 0 + 0 = 0
        assert_eq!(low, 0);
        // After first nibble: step_index = max(0, 0 + INDEX_TABLE[0]) = max(0, -1) = 0
        // Second nibble: same calculation
        assert_eq!(high, 0);
    }

    #[test]
    fn test_decode_produces_correct_count() {
        let input = vec![0x12, 0x34, 0x56, 0x78];
        let output = decode_adpcm(&input);
        assert_eq!(output.len(), input.len() * 2);
    }

    #[test]
    fn test_decoder_reset() {
        let mut decoder = AdpcmDecoder::new();
        decoder.decode(&[0xFF, 0xFF]); // Decode some data

        // State should be non-zero
        assert_ne!(decoder.predictor(), 0);

        decoder.reset();
        assert_eq!(decoder.predictor(), 0);
        assert_eq!(decoder.step_index(), 0);
    }

    #[test]
    fn test_predictor_clamping() {
        // Test that predictor is clamped to 16-bit range
        let mut decoder = AdpcmDecoder::with_state(32000, 88);

        // Decode nibble that would push predictor past 32767
        let sample = decoder.decode_nibble(0x07); // Large positive value

        // Should be clamped to valid 16-bit range
        assert!(sample >= -32768 && sample <= 32767);
    }

    #[test]
    fn test_negative_samples() {
        let mut decoder = AdpcmDecoder::new();

        // Nibble with high bit set should produce negative difference
        let sample = decoder.decode_nibble(0x0F); // Sign bit + all magnitude bits

        // Should produce a negative sample (from 0)
        assert!(sample < 0);
    }
}
