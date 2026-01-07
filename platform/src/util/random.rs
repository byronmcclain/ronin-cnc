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
/// * `value` - Initial seed value
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
    pub fn get_seed(&self) -> u32 {
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
        // Same seed should produce same sequence (use instance to avoid test races)
        let mut rng1 = Random::new(42);
        let seq1: Vec<u32> = (0..10).map(|_| rng1.next()).collect();

        let mut rng2 = Random::new(42);
        let seq2: Vec<u32> = (0..10).map(|_| rng2.next()).collect();

        assert_eq!(seq1, seq2);
    }

    #[test]
    fn test_different_seeds_different_sequences() {
        let mut rng1 = Random::new(1);
        let seq1: Vec<u32> = (0..5).map(|_| rng1.next()).collect();

        let mut rng2 = Random::new(2);
        let seq2: Vec<u32> = (0..5).map(|_| rng2.next()).collect();

        assert_ne!(seq1, seq2);
    }

    #[test]
    fn test_range_bounds() {
        let mut rng = Random::new(12345);

        // Test that range stays in bounds
        for _ in 0..1000 {
            let val = rng.range(10, 20);
            assert!(val >= 10 && val <= 20, "value {} out of range [10, 20]", val);
        }
    }

    #[test]
    fn test_range_single_value() {
        let mut rng = Random::new(12345);
        // When min == max, always return min
        assert_eq!(rng.range(42, 42), 42);
        assert_eq!(rng.range(42, 42), 42);
    }

    #[test]
    fn test_range_inverted() {
        let mut rng = Random::new(12345);
        // When min > max, return min
        assert_eq!(rng.range(50, 10), 50);
    }

    #[test]
    fn test_next_max_zero() {
        let mut rng = Random::new(12345);
        assert_eq!(rng.next_max(0), 0);
    }

    #[test]
    fn test_output_range() {
        let mut rng = Random::new(12345);

        // All values should be in [0, 32767]
        for _ in 0..1000 {
            let val = rng.next();
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
