//! Simple RSA decryption for MIX file key recovery
//!
//! This implements just enough RSA to decrypt the Blowfish key
//! from encrypted MIX files using Westwood's public key.

/// Westwood's public key for MIX file decryption
///
/// The modulus is base64-decoded from:
/// "AihRvNoIbTn85FZRYNZRcT+i6KpU+maCsEqr3Q5q+LDB5tH7Tz2qQ38V"
///
/// The exponent is the standard RSA public exponent: 65537 (0x10001)
pub const WESTWOOD_PUBLIC_KEY: RsaPublicKey = RsaPublicKey {
    // Base64: AihRvNoIbTn85FZRYNZRcT+i6KpU+maCsEqr3Q5q+LDB5tH7Tz2qQ38V
    // Decoded bytes (40 bytes):
    modulus: &[
        0x02, 0x28, 0x51, 0xBC, 0xDA, 0x08, 0x6D, 0x39, 0xFC, 0xE4, 0x56, 0x51, 0x60, 0xD6, 0x51,
        0x71, 0x3F, 0xA2, 0xE8, 0xAA, 0x54, 0xFA, 0x66, 0x82, 0xB0, 0x4A, 0xAB, 0xDD, 0x0E, 0x6A,
        0xF8, 0xB0, 0xC1, 0xE6, 0xD1, 0xFB, 0x4F, 0x3D, 0xAA, 0x43, 0x7F, 0x15,
    ],
    exponent: 65537,
};

/// RSA block sizes for Westwood's key
pub const RSA_PLAIN_BLOCK_SIZE: usize = 39; // modulus_bits - 1 / 8 = (40*8-1)/8 = 39
pub const RSA_CRYPT_BLOCK_SIZE: usize = 40; // Same as modulus size

/// RSA Public Key
pub struct RsaPublicKey {
    /// The modulus n (big-endian bytes)
    pub modulus: &'static [u8],
    /// The exponent e
    pub exponent: u32,
}

impl RsaPublicKey {
    /// Get the plaintext block size
    pub fn plain_block_size(&self) -> usize {
        self.modulus.len() - 1
    }

    /// Get the ciphertext block size
    pub fn crypt_block_size(&self) -> usize {
        self.modulus.len()
    }

    /// Decrypt a single RSA block
    ///
    /// Performs: plaintext = ciphertext^e mod n
    pub fn decrypt_block(&self, ciphertext: &[u8]) -> Vec<u8> {
        // Convert ciphertext to BigUint
        let c = BigUint::from_be_bytes(ciphertext);
        let n = BigUint::from_be_bytes(self.modulus);
        let e = BigUint::from_u32(self.exponent);

        // Compute m = c^e mod n
        let m = mod_pow(&c, &e, &n);

        // Convert back to bytes
        m.to_be_bytes_padded(self.plain_block_size())
    }
}

/// Decrypt the RSA-encrypted Blowfish key from a MIX file
///
/// # Arguments
/// * `encrypted_key_block` - 80 bytes of RSA-encrypted key data
///
/// # Returns
/// 56-byte Blowfish key (with trailing padding)
pub fn decrypt_mix_key(encrypted_key_block: &[u8]) -> Vec<u8> {
    let mut result = Vec::with_capacity(56);

    // Process 2 blocks of 40 bytes each
    // The original code reads Block_Count(56) * Crypt_Block_Size() = 2 * 40 = 80 bytes
    // And decrypts to Block_Count(56) * Plain_Block_Size() = 2 * 39 = 78 bytes

    for block_idx in 0..2 {
        let start = block_idx * RSA_CRYPT_BLOCK_SIZE;
        let end = start + RSA_CRYPT_BLOCK_SIZE;

        if end <= encrypted_key_block.len() {
            let decrypted = WESTWOOD_PUBLIC_KEY.decrypt_block(&encrypted_key_block[start..end]);
            result.extend_from_slice(&decrypted);
        }
    }

    // Truncate to 56 bytes (Blowfish key length)
    result.truncate(56);
    result
}

// =============================================================================
// Minimal BigUint implementation for RSA
// =============================================================================

/// Simple arbitrary-precision unsigned integer
///
/// Digits are stored in little-endian order (least significant first).
/// Each digit is a 32-bit value.
#[derive(Clone, Debug)]
struct BigUint {
    digits: Vec<u32>,
}

impl BigUint {
    /// Create a BigUint from a u32
    fn from_u32(value: u32) -> Self {
        if value == 0 {
            Self { digits: vec![0] }
        } else {
            Self {
                digits: vec![value],
            }
        }
    }

    /// Create a BigUint from big-endian bytes
    fn from_be_bytes(bytes: &[u8]) -> Self {
        if bytes.is_empty() {
            return Self { digits: vec![0] };
        }

        // Convert to 32-bit digits in little-endian order
        let mut digits = Vec::new();

        // Process bytes in groups of 4, from right to left
        let mut i = bytes.len();
        while i > 0 {
            let end = i;
            let start = if i >= 4 { i - 4 } else { 0 };

            let mut digit = 0u32;
            for j in start..end {
                digit = (digit << 8) | (bytes[j] as u32);
            }
            digits.push(digit);
            i = start;
        }

        // Trim leading zeros
        while digits.len() > 1 && *digits.last().unwrap() == 0 {
            digits.pop();
        }

        Self { digits }
    }

    /// Convert to big-endian bytes with specified minimum length
    fn to_be_bytes_padded(&self, min_len: usize) -> Vec<u8> {
        let mut bytes = Vec::new();

        // Convert digits to bytes (big-endian output)
        for digit in self.digits.iter().rev() {
            bytes.extend_from_slice(&digit.to_be_bytes());
        }

        // Trim leading zeros
        while bytes.len() > 1 && bytes[0] == 0 {
            bytes.remove(0);
        }

        // Pad with leading zeros if needed
        while bytes.len() < min_len {
            bytes.insert(0, 0);
        }

        bytes
    }

    /// Check if zero
    fn is_zero(&self) -> bool {
        self.digits.len() == 1 && self.digits[0] == 0
    }

    /// Check if odd
    fn is_odd(&self) -> bool {
        self.digits[0] & 1 == 1
    }

    /// Right shift by 1 bit (divide by 2)
    fn shr1(&mut self) {
        let mut carry = 0u32;
        for digit in self.digits.iter_mut().rev() {
            let new_carry = *digit & 1;
            *digit = (*digit >> 1) | (carry << 31);
            carry = new_carry;
        }

        // Trim leading zeros
        while self.digits.len() > 1 && *self.digits.last().unwrap() == 0 {
            self.digits.pop();
        }
    }

    /// Multiply by another BigUint
    fn mul(&self, other: &BigUint) -> BigUint {
        let mut result = vec![0u64; self.digits.len() + other.digits.len()];

        for (i, &a) in self.digits.iter().enumerate() {
            for (j, &b) in other.digits.iter().enumerate() {
                let prod = (a as u64) * (b as u64);
                result[i + j] += prod;
            }
        }

        // Propagate carries
        let mut carry = 0u64;
        for digit in result.iter_mut() {
            *digit += carry;
            carry = *digit >> 32;
            *digit &= 0xFFFFFFFF;
        }

        // Convert to u32 and trim
        let mut digits: Vec<u32> = result.into_iter().map(|d| d as u32).collect();
        while digits.len() > 1 && *digits.last().unwrap() == 0 {
            digits.pop();
        }

        BigUint { digits }
    }

    /// Compare with another BigUint
    fn cmp(&self, other: &BigUint) -> std::cmp::Ordering {
        use std::cmp::Ordering;

        if self.digits.len() != other.digits.len() {
            return self.digits.len().cmp(&other.digits.len());
        }

        for (a, b) in self.digits.iter().rev().zip(other.digits.iter().rev()) {
            if a != b {
                return a.cmp(b);
            }
        }

        Ordering::Equal
    }

    /// Subtract another BigUint (assumes self >= other)
    fn sub(&self, other: &BigUint) -> BigUint {
        let mut result = self.digits.clone();
        let mut borrow = 0i64;

        for i in 0..result.len() {
            let a = result[i] as i64;
            let b = if i < other.digits.len() {
                other.digits[i] as i64
            } else {
                0
            };
            let diff = a - b - borrow;

            if diff < 0 {
                result[i] = (diff + (1i64 << 32)) as u32;
                borrow = 1;
            } else {
                result[i] = diff as u32;
                borrow = 0;
            }
        }

        // Trim leading zeros
        while result.len() > 1 && *result.last().unwrap() == 0 {
            result.pop();
        }

        BigUint { digits: result }
    }

    /// Compute self mod modulus
    fn modulo(&self, modulus: &BigUint) -> BigUint {
        if self.cmp(modulus) == std::cmp::Ordering::Less {
            return self.clone();
        }

        // Simple repeated subtraction for small numbers
        // For larger numbers, we'd need proper division
        let mut result = self.clone();

        // Find the highest bit position
        let self_bits = result.bit_length();
        let mod_bits = modulus.bit_length();

        if self_bits < mod_bits {
            return result;
        }

        // Align modulus with result
        let shift = self_bits - mod_bits;
        let mut shifted_mod = modulus.clone();
        for _ in 0..shift {
            shifted_mod = shifted_mod.shl1();
        }

        for _ in 0..=shift {
            if result.cmp(&shifted_mod) != std::cmp::Ordering::Less {
                result = result.sub(&shifted_mod);
            }
            shifted_mod.shr1();
        }

        result
    }

    /// Left shift by 1 bit (multiply by 2)
    fn shl1(&self) -> BigUint {
        let mut result = vec![0u32; self.digits.len() + 1];
        let mut carry = 0u32;

        for (i, &digit) in self.digits.iter().enumerate() {
            let shifted = ((digit as u64) << 1) | (carry as u64);
            result[i] = shifted as u32;
            carry = (shifted >> 32) as u32;
        }

        if carry != 0 {
            result[self.digits.len()] = carry;
        }

        // Trim leading zeros
        while result.len() > 1 && *result.last().unwrap() == 0 {
            result.pop();
        }

        BigUint { digits: result }
    }

    /// Get bit length
    fn bit_length(&self) -> usize {
        if self.is_zero() {
            return 0;
        }

        let top_digit = *self.digits.last().unwrap();
        let top_bits = 32 - top_digit.leading_zeros() as usize;

        (self.digits.len() - 1) * 32 + top_bits
    }
}

/// Modular exponentiation: base^exp mod modulus
///
/// Uses square-and-multiply algorithm
fn mod_pow(base: &BigUint, exp: &BigUint, modulus: &BigUint) -> BigUint {
    if modulus.is_zero() {
        panic!("Modulus cannot be zero");
    }

    let mut result = BigUint::from_u32(1);
    let mut base = base.modulo(modulus);
    let mut exp = exp.clone();

    while !exp.is_zero() {
        if exp.is_odd() {
            result = result.mul(&base).modulo(modulus);
        }
        base = base.mul(&base).modulo(modulus);
        exp.shr1();
    }

    result
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_biguint_from_u32() {
        let n = BigUint::from_u32(0x12345678);
        assert_eq!(n.digits, vec![0x12345678]);
    }

    #[test]
    fn test_biguint_from_bytes() {
        let n = BigUint::from_be_bytes(&[0x01, 0x02, 0x03, 0x04]);
        assert_eq!(n.digits, vec![0x01020304]);
    }

    #[test]
    fn test_biguint_mul() {
        let a = BigUint::from_u32(12345);
        let b = BigUint::from_u32(67890);
        let c = a.mul(&b);
        // 12345 * 67890 = 838102050
        assert_eq!(c.digits, vec![838102050]);
    }

    #[test]
    fn test_biguint_modulo() {
        let a = BigUint::from_u32(100);
        let m = BigUint::from_u32(7);
        let r = a.modulo(&m);
        // 100 % 7 = 2
        assert_eq!(r.digits, vec![2]);
    }

    #[test]
    fn test_mod_pow() {
        // 3^5 mod 7 = 243 mod 7 = 5
        let base = BigUint::from_u32(3);
        let exp = BigUint::from_u32(5);
        let modulus = BigUint::from_u32(7);
        let result = mod_pow(&base, &exp, &modulus);
        assert_eq!(result.digits, vec![5]);
    }

    #[test]
    fn test_mod_pow_larger() {
        // 2^10 mod 1000 = 1024 mod 1000 = 24
        let base = BigUint::from_u32(2);
        let exp = BigUint::from_u32(10);
        let modulus = BigUint::from_u32(1000);
        let result = mod_pow(&base, &exp, &modulus);
        assert_eq!(result.digits, vec![24]);
    }

    #[test]
    fn test_westwood_key_sizes() {
        assert_eq!(WESTWOOD_PUBLIC_KEY.modulus.len(), 40);
        assert_eq!(WESTWOOD_PUBLIC_KEY.plain_block_size(), 39);
        assert_eq!(WESTWOOD_PUBLIC_KEY.crypt_block_size(), 40);
    }
}
