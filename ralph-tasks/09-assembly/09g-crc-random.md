# Task 09g: CRC32 and Random Number Generator

## Dependencies
- Task 09a must be complete (platform crate structure)

## Context
The original WIN32LIB/MISC/CRC.ASM and RANDOM.ASM implement CRC32 checksums and random number generation. CRC32 is used for data integrity (MIX files, save games). The random number generator uses a Linear Congruential Generator (LCG) that must match the original to ensure game replay compatibility.

## Objective
Create utility functions:
1. `crc32()` - Calculate CRC32 checksum
2. `crc32_update()` - Update existing CRC with more data
3. `random_seed()` - Seed the random number generator
4. `random_get()` - Get next random number
5. `random_range()` - Get random number in range
6. FFI exports and tests with known values

## Deliverables
- [ ] `platform/src/crc.rs` - CRC32 implementation
- [ ] `platform/src/random.rs` - LCG random number generator
- [ ] FFI exports for all functions
- [ ] Tests with known CRC32 values
- [ ] Tests verifying LCG sequence
- [ ] Header contains FFI exports
- [ ] Verification passes

## Technical Background

### CRC32 Algorithm
- Polynomial: 0xEDB88320 (reversed representation of IEEE 802.3)
- Initial value: 0xFFFFFFFF
- Final XOR: 0xFFFFFFFF
- Uses 256-entry lookup table for performance

### LCG Random
The original uses standard LCG constants:
- `seed = seed * 1103515245 + 12345`
- Output: `(seed >> 16) & 0x7FFF` (15-bit result, 0-32767)

These constants match the C standard library's `rand()` on some systems.

### Importance of Compatibility
- CRC must match for MIX file validation
- Random must match for deterministic game playback

## Files to Create/Modify

### platform/src/crc.rs (NEW)
```rust
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
///
/// # Example
/// ```
/// use redalert_platform::crc::crc32;
/// let data = b"Hello, World!";
/// let checksum = crc32(data);
/// ```
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
/// * `crc` - Previous CRC value (use 0xFFFFFFFF for first chunk)
/// * `data` - New data to incorporate
///
/// # Returns
/// Updated CRC value (not finalized - call finalize_crc when done)
///
/// # Example
/// ```ignore
/// let mut crc = CRC_INIT;
/// crc = crc32_update(crc, &chunk1);
/// crc = crc32_update(crc, &chunk2);
/// let final_crc = crc32_finalize(crc);
/// ```
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
```

### platform/src/random.rs (NEW)
```rust
//! Random number generation
//!
//! Replaces WIN32LIB/MISC/RANDOM.ASM
//!
//! Uses Linear Congruential Generator (LCG) with constants matching
//! the original game for deterministic replay compatibility.

use std::sync::atomic::{AtomicU32, Ordering};

/// LCG multiplier
const LCG_MULTIPLIER: u32 = 1103515245;

/// LCG increment
const LCG_INCREMENT: u32 = 12345;

/// Global random seed (thread-safe)
static RANDOM_SEED: AtomicU32 = AtomicU32::new(1);

/// Seed the global random number generator
///
/// # Arguments
/// * `seed` - Initial seed value (use 0 for default behavior)
///
/// # Note
/// Using the same seed will produce the same sequence of random numbers.
/// This is important for game replay determinism.
pub fn seed(value: u32) {
    RANDOM_SEED.store(value, Ordering::SeqCst);
}

/// Get the current seed value
pub fn get_seed() -> u32 {
    RANDOM_SEED.load(Ordering::SeqCst)
}

/// Get next random number (0-32767)
///
/// Returns a 15-bit random value using the LCG algorithm.
///
/// # Returns
/// Random value in range [0, 32767]
pub fn next() -> u32 {
    let seed = RANDOM_SEED.load(Ordering::SeqCst);
    let new_seed = seed.wrapping_mul(LCG_MULTIPLIER).wrapping_add(LCG_INCREMENT);
    RANDOM_SEED.store(new_seed, Ordering::SeqCst);

    // Return bits 16-30 (15 bits)
    (new_seed >> 16) & 0x7FFF
}

/// Get random number in range [min, max] (inclusive)
///
/// # Arguments
/// * `min` - Minimum value (inclusive)
/// * `max` - Maximum value (inclusive)
///
/// # Returns
/// Random value in range [min, max]
///
/// # Note
/// If min >= max, returns min
pub fn range(min: i32, max: i32) -> i32 {
    if min >= max {
        return min;
    }

    let range_size = (max - min + 1) as u32;
    min + (next() % range_size) as i32
}

/// Get random number in range [0, max) (exclusive)
///
/// # Arguments
/// * `max` - Upper bound (exclusive)
///
/// # Returns
/// Random value in range [0, max)
pub fn next_max(max: u32) -> u32 {
    if max == 0 {
        return 0;
    }
    next() % max
}

/// Random number generator instance (for isolated state)
///
/// Use this when you need a separate random stream that doesn't
/// affect the global state.
#[derive(Clone, Debug)]
pub struct Random {
    seed: u32,
}

impl Random {
    /// Create new RNG with given seed
    pub fn new(seed: u32) -> Self {
        Self { seed }
    }

    /// Create RNG seeded from current time
    pub fn from_time() -> Self {
        use std::time::{SystemTime, UNIX_EPOCH};
        let seed = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .map(|d| d.as_millis() as u32)
            .unwrap_or(12345);
        Self { seed }
    }

    /// Get next random value (0-32767)
    pub fn next(&mut self) -> u32 {
        self.seed = self.seed.wrapping_mul(LCG_MULTIPLIER).wrapping_add(LCG_INCREMENT);
        (self.seed >> 16) & 0x7FFF
    }

    /// Get random value in range [min, max]
    pub fn range(&mut self, min: i32, max: i32) -> i32 {
        if min >= max {
            return min;
        }
        let range_size = (max - min + 1) as u32;
        min + (self.next() % range_size) as i32
    }

    /// Get random value in range [0, max)
    pub fn next_max(&mut self, max: u32) -> u32 {
        if max == 0 {
            return 0;
        }
        self.next() % max
    }

    /// Get current seed
    pub fn seed(&self) -> u32 {
        self.seed
    }

    /// Set new seed
    pub fn set_seed(&mut self, seed: u32) {
        self.seed = seed;
    }
}

impl Default for Random {
    fn default() -> Self {
        Self::new(1)
    }
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_seed_and_get() {
        seed(12345);
        assert_eq!(get_seed(), 12345);
    }

    #[test]
    fn test_deterministic_sequence() {
        // Same seed should produce same sequence
        seed(42);
        let seq1: Vec<u32> = (0..10).map(|_| next()).collect();

        seed(42);
        let seq2: Vec<u32> = (0..10).map(|_| next()).collect();

        assert_eq!(seq1, seq2);
    }

    #[test]
    fn test_different_seeds_different_sequences() {
        seed(1);
        let seq1: Vec<u32> = (0..5).map(|_| next()).collect();

        seed(2);
        let seq2: Vec<u32> = (0..5).map(|_| next()).collect();

        assert_ne!(seq1, seq2);
    }

    #[test]
    fn test_range_bounds() {
        seed(12345);

        // Test that range stays in bounds
        for _ in 0..1000 {
            let val = range(10, 20);
            assert!(val >= 10 && val <= 20, "value {} out of range [10, 20]", val);
        }
    }

    #[test]
    fn test_range_single_value() {
        seed(12345);
        // When min == max, always return min
        assert_eq!(range(42, 42), 42);
        assert_eq!(range(42, 42), 42);
    }

    #[test]
    fn test_range_inverted() {
        seed(12345);
        // When min > max, return min
        assert_eq!(range(50, 10), 50);
    }

    #[test]
    fn test_next_max_zero() {
        seed(12345);
        assert_eq!(next_max(0), 0);
    }

    #[test]
    fn test_output_range() {
        seed(12345);

        // All values should be in [0, 32767]
        for _ in 0..1000 {
            let val = next();
            assert!(val <= 32767, "value {} exceeds 32767", val);
        }
    }

    #[test]
    fn test_random_instance() {
        let mut rng1 = Random::new(42);
        let mut rng2 = Random::new(42);

        // Same seed, same sequence
        for _ in 0..10 {
            assert_eq!(rng1.next(), rng2.next());
        }
    }

    #[test]
    fn test_random_instance_independent() {
        // Instance doesn't affect global
        seed(1);
        let global_first = next();

        let mut rng = Random::new(99);
        let _ = rng.next();  // Shouldn't affect global

        seed(1);
        assert_eq!(next(), global_first);
    }

    #[test]
    fn test_known_sequence() {
        // Verify a known sequence (first 5 values with seed 1)
        // These values depend on the exact LCG formula
        let mut rng = Random::new(1);
        let expected = [16838, 5758, 10113, 17515, 31051];

        for &exp in &expected {
            assert_eq!(rng.next(), exp);
        }
    }
}
```

### platform/src/lib.rs (MODIFY)
Add after other module declarations:
```rust
pub mod crc;
pub mod random;
```

### platform/src/ffi/mod.rs (MODIFY)
Add CRC and Random FFI exports:
```rust
use crate::{crc, random};

// =============================================================================
// CRC32 FFI (replaces CRC.ASM)
// =============================================================================

/// Calculate CRC32 of data
///
/// # Safety
/// - `data` must point to valid memory of at least `size` bytes
#[no_mangle]
pub unsafe extern "C" fn Platform_CRC32(data: *const u8, size: i32) -> u32 {
    if data.is_null() || size <= 0 {
        return 0;
    }

    let slice = std::slice::from_raw_parts(data, size as usize);
    crc::crc32(slice)
}

/// Update CRC32 with more data (for streaming)
///
/// # Arguments
/// * `crc` - Previous CRC value (use 0xFFFFFFFF for first call)
/// * `data` - Pointer to data
/// * `size` - Size of data
///
/// # Returns
/// Updated CRC (call Platform_CRC32_Finalize when done)
///
/// # Safety
/// - `data` must point to valid memory of at least `size` bytes
#[no_mangle]
pub unsafe extern "C" fn Platform_CRC32_Update(crc: u32, data: *const u8, size: i32) -> u32 {
    if data.is_null() || size <= 0 {
        return crc;
    }

    let slice = std::slice::from_raw_parts(data, size as usize);
    crc::crc32_update(crc, slice)
}

/// Finalize CRC32 calculation
///
/// XORs with 0xFFFFFFFF to produce final value.
#[no_mangle]
pub extern "C" fn Platform_CRC32_Finalize(crc: u32) -> u32 {
    crc::crc32_finalize(crc)
}

/// Get initial CRC32 value for streaming
#[no_mangle]
pub extern "C" fn Platform_CRC32_Init() -> u32 {
    crc::CRC_INIT
}

// =============================================================================
// Random FFI (replaces RANDOM.ASM)
// =============================================================================

/// Seed the global random number generator
#[no_mangle]
pub extern "C" fn Platform_Random_Seed(seed: u32) {
    random::seed(seed);
}

/// Get the current random seed
#[no_mangle]
pub extern "C" fn Platform_Random_GetSeed() -> u32 {
    random::get_seed()
}

/// Get next random number (0-32767)
#[no_mangle]
pub extern "C" fn Platform_Random_Get() -> u32 {
    random::next()
}

/// Get random number in range [min, max]
#[no_mangle]
pub extern "C" fn Platform_Random_Range(min: i32, max: i32) -> i32 {
    random::range(min, max)
}

/// Get random number in range [0, max)
#[no_mangle]
pub extern "C" fn Platform_Random_Max(max: u32) -> u32 {
    random::next_max(max)
}
```

## Verification Command
```bash
ralph-tasks/verify/check-crc-random.sh
```

## Implementation Steps
1. Create `platform/src/crc.rs` with CRC32 implementation
2. Create `platform/src/random.rs` with LCG implementation
3. Add modules to `platform/src/lib.rs`
4. Add FFI exports to `platform/src/ffi/mod.rs`
5. Run `cargo test --manifest-path platform/Cargo.toml -- crc` to verify CRC tests
6. Run `cargo test --manifest-path platform/Cargo.toml -- random` to verify random tests
7. Run `cargo build --manifest-path platform/Cargo.toml --features cbindgen-run`
8. Verify header contains FFI exports
9. Run verification script

## Success Criteria
- CRC32 matches known test vectors (e.g., "123456789" = 0xCBF43926)
- Streaming CRC produces same result as one-shot
- Random sequence is deterministic for same seed
- Random range respects bounds
- FFI exports appear in platform.h

## Common Issues

### CRC doesn't match known values
- Check polynomial (0xEDB88320 for standard CRC32)
- Check initial value (0xFFFFFFFF)
- Check final XOR (0xFFFFFFFF)

### Random sequence doesn't match expected
- Verify LCG constants (1103515245, 12345)
- Check bit shift for output (bits 16-30)
- Ensure wrapping arithmetic is used

### Thread safety issues with global random
- AtomicU32 with SeqCst ordering should be sufficient
- For more concurrent use, consider Random instance

## Completion Promise
When verification passes, output:
```
<promise>TASK_09G_COMPLETE</promise>
```

## Escape Hatch
If stuck after 10 iterations:
- Document what's blocking in `ralph-tasks/blocked/09g.md`
- List attempted approaches
- Output: `<promise>TASK_09G_BLOCKED</promise>`

## Max Iterations
10
