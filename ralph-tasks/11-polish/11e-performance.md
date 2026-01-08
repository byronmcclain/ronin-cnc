# Task 11e: Performance Optimization Module

## Dependencies
- Phase 03 (Graphics) must be complete (palette conversion exists)
- Platform crate structure in place

## Context
This task creates performance optimization utilities including:
- **SIMD palette conversion**: Fast 8-bit to 32-bit color lookup
- **Object pooling**: Reduce allocation overhead for frequently reused objects
- **Performance counters**: FPS and frame time measurement
- **Architecture detection**: Runtime CPU feature detection

The original game targets 15-20 FPS. Modern systems can easily hit 60 FPS, but inefficient code wastes battery on laptops. Optimized routines help achieve smooth performance with minimal CPU usage.

**Target Architectures:**
- x86_64 (Intel Macs): Use SSE2/AVX2 when available
- aarch64 (Apple Silicon): Use NEON intrinsics

## Objective
Create a performance module that:
1. Provides SIMD-optimized palette conversion for both x86_64 and ARM64
2. Implements a generic object pool
3. Provides frame timing utilities
4. Detects CPU features at runtime
5. Exports FFI functions for performance monitoring

## Deliverables
- [ ] `platform/src/perf/mod.rs` - Module root
- [ ] `platform/src/perf/simd.rs` - SIMD-optimized functions
- [ ] `platform/src/perf/pool.rs` - Object pooling
- [ ] `platform/src/perf/timing.rs` - Frame timing
- [ ] FFI exports: `Platform_GetFPS`, `Platform_GetFrameTime`, `Platform_FrameStart`, `Platform_FrameEnd`
- [ ] Header contains FFI exports
- [ ] Unit tests pass
- [ ] Verification passes

## Technical Background

### SIMD Palette Conversion
The core operation is:
```
for each pixel:
    output[i] = palette[input[i]]
```

This is memory-bound, but we can optimize by:
- Processing multiple pixels per iteration
- Prefetching palette entries
- Using SIMD gather operations (AVX2)

For now, we'll use a simple loop unrolling approach that the compiler can auto-vectorize, with explicit SIMD for later optimization.

### Object Pooling
Many game objects are created/destroyed frequently:
- Projectiles
- Particle effects
- Sound instances

A pool avoids allocation overhead by reusing objects.

### Frame Timing
```
frame_start()
  |
  v
  [game logic] <- measure this
  |
  v
  [render] <- and this
  |
  v
frame_end()
  |
  v
  calculate FPS, frame time
```

## Files to Create/Modify

### platform/src/perf/mod.rs (NEW)
```rust
//! Performance optimization utilities
//!
//! Provides:
//! - SIMD-optimized routines
//! - Object pooling
//! - Frame timing

pub mod pool;
pub mod simd;
pub mod timing;

pub use pool::ObjectPool;
pub use simd::{convert_palette_fast, PaletteConverter};
pub use timing::{FrameTimer, get_fps, get_frame_time_ms, frame_start, frame_end};

// Re-export FFI functions
pub use timing::{
    Platform_GetFPS,
    Platform_GetFrameTime,
    Platform_FrameStart,
    Platform_FrameEnd,
    Platform_GetFrameCount,
};
```

### platform/src/perf/simd.rs (NEW)
```rust
//! SIMD-optimized routines
//!
//! Provides optimized versions of common operations:
//! - Palette conversion (8-bit indexed to 32-bit ARGB)
//!
//! Automatically selects the best implementation for the current CPU.

/// Palette converter with automatic SIMD dispatch
pub struct PaletteConverter {
    /// Current palette (256 ARGB entries)
    palette: [u32; 256],
}

impl PaletteConverter {
    /// Create a new palette converter
    pub fn new() -> Self {
        Self {
            palette: [0xFF000000; 256], // Default to opaque black
        }
    }

    /// Set the palette
    pub fn set_palette(&mut self, palette: &[u32; 256]) {
        self.palette = *palette;
    }

    /// Convert indexed pixels to ARGB
    ///
    /// # Arguments
    /// - `src`: Source indexed pixels (8-bit)
    /// - `dest`: Destination ARGB pixels (32-bit)
    ///
    /// # Panics
    /// Panics if dest.len() < src.len()
    pub fn convert(&self, src: &[u8], dest: &mut [u32]) {
        convert_palette_fast(src, dest, &self.palette);
    }
}

impl Default for PaletteConverter {
    fn default() -> Self {
        Self::new()
    }
}

/// Fast palette conversion with auto-dispatch
///
/// Selects the best implementation for the current CPU:
/// - AVX2 gather on x86_64 with AVX2 (future)
/// - NEON on ARM64 (future)
/// - Scalar with loop unrolling as baseline
pub fn convert_palette_fast(src: &[u8], dest: &mut [u32], palette: &[u32; 256]) {
    assert!(
        dest.len() >= src.len(),
        "Destination buffer too small: {} < {}",
        dest.len(),
        src.len()
    );

    // Use the optimized unrolled version
    convert_palette_unrolled(src, dest, palette);
}

/// Scalar conversion with 8x loop unrolling
///
/// This is surprisingly fast as the compiler can optimize it well.
/// The unrolling helps with instruction-level parallelism.
#[inline(always)]
fn convert_palette_unrolled(src: &[u8], dest: &mut [u32], palette: &[u32; 256]) {
    let len = src.len();
    let chunks = len / 8;
    let remainder = len % 8;

    // Process 8 pixels at a time
    for i in 0..chunks {
        let base = i * 8;
        // Explicit unroll for ILP
        dest[base] = palette[src[base] as usize];
        dest[base + 1] = palette[src[base + 1] as usize];
        dest[base + 2] = palette[src[base + 2] as usize];
        dest[base + 3] = palette[src[base + 3] as usize];
        dest[base + 4] = palette[src[base + 4] as usize];
        dest[base + 5] = palette[src[base + 5] as usize];
        dest[base + 6] = palette[src[base + 6] as usize];
        dest[base + 7] = palette[src[base + 7] as usize];
    }

    // Handle remainder
    let rem_start = chunks * 8;
    for i in 0..remainder {
        dest[rem_start + i] = palette[src[rem_start + i] as usize];
    }
}

/// x86_64 SSE2 optimized version (future enhancement)
#[cfg(target_arch = "x86_64")]
#[allow(dead_code)]
fn convert_palette_sse2(_src: &[u8], _dest: &mut [u32], _palette: &[u32; 256]) {
    // SSE2 doesn't have efficient gather, so unrolled scalar is often faster
    // Could use prefetch hints here
    unimplemented!("SSE2 version not yet implemented")
}

/// ARM NEON optimized version (future enhancement)
#[cfg(target_arch = "aarch64")]
#[allow(dead_code)]
fn convert_palette_neon(_src: &[u8], _dest: &mut [u32], _palette: &[u32; 256]) {
    // NEON has vld1q_u8 for loading, vtbl for table lookup
    // But table lookup is limited to 16 entries, not 256
    // Would need to use scalar with NEON loads/stores
    unimplemented!("NEON version not yet implemented")
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    fn create_test_palette() -> [u32; 256] {
        let mut palette = [0u32; 256];
        for i in 0..256 {
            // Create a gradient palette
            palette[i] = 0xFF000000 | (i as u32) << 16 | (i as u32) << 8 | (i as u32);
        }
        palette
    }

    #[test]
    fn test_convert_palette_basic() {
        let palette = create_test_palette();
        let src = [0u8, 128, 255];
        let mut dest = [0u32; 3];

        convert_palette_fast(&src, &mut dest, &palette);

        assert_eq!(dest[0], palette[0]);
        assert_eq!(dest[1], palette[128]);
        assert_eq!(dest[2], palette[255]);
    }

    #[test]
    fn test_convert_palette_large() {
        let palette = create_test_palette();
        let src: Vec<u8> = (0..640 * 400).map(|i| (i % 256) as u8).collect();
        let mut dest = vec![0u32; 640 * 400];

        convert_palette_fast(&src, &mut dest, &palette);

        // Verify all conversions
        for i in 0..src.len() {
            assert_eq!(dest[i], palette[src[i] as usize]);
        }
    }

    #[test]
    fn test_convert_palette_unaligned() {
        // Test with sizes that aren't multiples of 8
        let palette = create_test_palette();

        for len in [1, 5, 7, 9, 15, 17, 100, 103] {
            let src: Vec<u8> = (0..len).map(|i| i as u8).collect();
            let mut dest = vec![0u32; len];

            convert_palette_fast(&src, &mut dest, &palette);

            for i in 0..len {
                assert_eq!(dest[i], palette[src[i] as usize], "Mismatch at index {} for len {}", i, len);
            }
        }
    }

    #[test]
    fn test_palette_converter_struct() {
        let mut converter = PaletteConverter::new();

        let palette = create_test_palette();
        converter.set_palette(&palette);

        let src = [10u8, 20, 30];
        let mut dest = [0u32; 3];

        converter.convert(&src, &mut dest);

        assert_eq!(dest[0], palette[10]);
        assert_eq!(dest[1], palette[20]);
        assert_eq!(dest[2], palette[30]);
    }

    #[test]
    #[should_panic(expected = "Destination buffer too small")]
    fn test_convert_palette_dest_too_small() {
        let palette = create_test_palette();
        let src = [0u8; 10];
        let mut dest = [0u32; 5]; // Too small!

        convert_palette_fast(&src, &mut dest, &palette);
    }
}
```

### platform/src/perf/pool.rs (NEW)
```rust
//! Object pooling for performance
//!
//! Reduces allocation overhead by reusing objects.

use std::collections::VecDeque;
use std::sync::Mutex;

/// Simple thread-safe object pool
///
/// # Example
/// ```
/// use redalert_platform::perf::ObjectPool;
///
/// let pool: ObjectPool<Vec<u8>> = ObjectPool::new(
///     || Vec::with_capacity(1024), // Factory function
///     8 // Initial pool size
/// );
///
/// // Acquire an object
/// let mut buffer = pool.acquire();
/// buffer.extend_from_slice(b"data");
///
/// // Clear and return to pool
/// buffer.clear();
/// pool.release(buffer);
/// ```
pub struct ObjectPool<T> {
    available: Mutex<VecDeque<T>>,
    factory: fn() -> T,
}

impl<T> ObjectPool<T> {
    /// Create a new object pool
    ///
    /// # Arguments
    /// - `factory`: Function to create new objects
    /// - `initial_size`: Number of objects to pre-allocate
    pub fn new(factory: fn() -> T, initial_size: usize) -> Self {
        let mut available = VecDeque::with_capacity(initial_size);
        for _ in 0..initial_size {
            available.push_back(factory());
        }
        Self {
            available: Mutex::new(available),
            factory,
        }
    }

    /// Acquire an object from the pool
    ///
    /// Returns a pooled object if available, or creates a new one.
    pub fn acquire(&self) -> T {
        let mut pool = self.available.lock().unwrap();
        pool.pop_front().unwrap_or_else(|| (self.factory)())
    }

    /// Release an object back to the pool
    ///
    /// The caller should reset the object to a clean state before releasing.
    pub fn release(&self, item: T) {
        let mut pool = self.available.lock().unwrap();
        pool.push_back(item);
    }

    /// Get the number of available objects in the pool
    pub fn available_count(&self) -> usize {
        self.available.lock().unwrap().len()
    }

    /// Clear all pooled objects
    pub fn clear(&self) {
        self.available.lock().unwrap().clear();
    }
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_pool_basic() {
        let pool: ObjectPool<Vec<u8>> = ObjectPool::new(Vec::new, 3);

        // Should have 3 pre-allocated
        assert_eq!(pool.available_count(), 3);

        // Acquire all 3
        let _a = pool.acquire();
        let _b = pool.acquire();
        let _c = pool.acquire();
        assert_eq!(pool.available_count(), 0);

        // Acquire one more (creates new)
        let _d = pool.acquire();
        assert_eq!(pool.available_count(), 0);
    }

    #[test]
    fn test_pool_release() {
        let pool: ObjectPool<Vec<u8>> = ObjectPool::new(Vec::new, 1);

        let item = pool.acquire();
        assert_eq!(pool.available_count(), 0);

        pool.release(item);
        assert_eq!(pool.available_count(), 1);
    }

    #[test]
    fn test_pool_factory() {
        let pool: ObjectPool<Vec<u8>> = ObjectPool::new(
            || Vec::with_capacity(100),
            2
        );

        let item = pool.acquire();
        // Factory creates with capacity 100
        assert!(item.capacity() >= 100);
    }

    #[test]
    fn test_pool_clear() {
        let pool: ObjectPool<i32> = ObjectPool::new(|| 0, 5);
        assert_eq!(pool.available_count(), 5);

        pool.clear();
        assert_eq!(pool.available_count(), 0);
    }
}
```

### platform/src/perf/timing.rs (NEW)
```rust
//! Frame timing and FPS measurement
//!
//! Provides utilities for measuring frame rate and frame times.

use std::sync::atomic::{AtomicU64, Ordering};
use std::time::Instant;
use once_cell::sync::Lazy;
use std::sync::Mutex;

/// Frame timing state
struct TimingState {
    /// Frame start time
    frame_start: Option<Instant>,
    /// Last frame duration in microseconds
    last_frame_us: u64,
    /// Rolling average of frame times (microseconds)
    avg_frame_us: f64,
    /// Frame count since start
    frame_count: u64,
    /// Time of last FPS calculation
    fps_calc_time: Instant,
    /// Frames since last FPS calculation
    fps_frame_count: u64,
    /// Current FPS
    current_fps: f64,
}

impl Default for TimingState {
    fn default() -> Self {
        Self {
            frame_start: None,
            last_frame_us: 0,
            avg_frame_us: 16666.0, // ~60 FPS default
            frame_count: 0,
            fps_calc_time: Instant::now(),
            fps_frame_count: 0,
            current_fps: 60.0,
        }
    }
}

static TIMING: Lazy<Mutex<TimingState>> = Lazy::new(|| Mutex::new(TimingState::default()));
static FRAME_COUNT: AtomicU64 = AtomicU64::new(0);

/// High-precision frame timer
pub struct FrameTimer {
    start: Instant,
}

impl FrameTimer {
    /// Start a new frame timer
    pub fn start() -> Self {
        Self {
            start: Instant::now(),
        }
    }

    /// Get elapsed time in seconds
    pub fn elapsed_secs(&self) -> f64 {
        self.start.elapsed().as_secs_f64()
    }

    /// Get elapsed time in milliseconds
    pub fn elapsed_ms(&self) -> f64 {
        self.start.elapsed().as_secs_f64() * 1000.0
    }

    /// Get elapsed time in microseconds
    pub fn elapsed_us(&self) -> u64 {
        self.start.elapsed().as_micros() as u64
    }
}

// =============================================================================
// Global Timing Functions
// =============================================================================

/// Mark the start of a frame
///
/// Call at the beginning of each game loop iteration.
pub fn frame_start() {
    if let Ok(mut state) = TIMING.lock() {
        state.frame_start = Some(Instant::now());
    }
}

/// Mark the end of a frame
///
/// Call at the end of each game loop iteration.
/// Updates FPS and frame time statistics.
pub fn frame_end() {
    let now = Instant::now();

    if let Ok(mut state) = TIMING.lock() {
        if let Some(start) = state.frame_start.take() {
            let frame_us = start.elapsed().as_micros() as u64;
            state.last_frame_us = frame_us;

            // Exponential moving average (smoothing factor 0.1)
            state.avg_frame_us = state.avg_frame_us * 0.9 + frame_us as f64 * 0.1;

            state.frame_count += 1;
            state.fps_frame_count += 1;

            // Update FPS every second
            let fps_elapsed = now.duration_since(state.fps_calc_time);
            if fps_elapsed.as_secs_f64() >= 1.0 {
                state.current_fps = state.fps_frame_count as f64 / fps_elapsed.as_secs_f64();
                state.fps_frame_count = 0;
                state.fps_calc_time = now;
            }
        }
    }

    FRAME_COUNT.fetch_add(1, Ordering::Relaxed);
}

/// Get the current FPS
///
/// Returns the frames per second, updated approximately once per second.
pub fn get_fps() -> f64 {
    TIMING.lock().map(|s| s.current_fps).unwrap_or(0.0)
}

/// Get the last frame time in milliseconds
pub fn get_frame_time_ms() -> f64 {
    TIMING.lock().map(|s| s.last_frame_us as f64 / 1000.0).unwrap_or(0.0)
}

/// Get the average frame time in milliseconds
pub fn get_avg_frame_time_ms() -> f64 {
    TIMING.lock().map(|s| s.avg_frame_us / 1000.0).unwrap_or(0.0)
}

/// Get the total frame count since program start
pub fn get_frame_count() -> u64 {
    FRAME_COUNT.load(Ordering::Relaxed)
}

// =============================================================================
// FFI Exports
// =============================================================================

use std::os::raw::c_int;

/// Get the current FPS
///
/// # Returns
/// Frames per second as an integer (0-999)
#[no_mangle]
pub extern "C" fn Platform_GetFPS() -> c_int {
    get_fps().round() as c_int
}

/// Get the last frame time in milliseconds
///
/// # Returns
/// Frame time in milliseconds (0-9999)
#[no_mangle]
pub extern "C" fn Platform_GetFrameTime() -> c_int {
    get_frame_time_ms().round() as c_int
}

/// Get the last frame time in microseconds
///
/// Higher precision than Platform_GetFrameTime.
///
/// # Returns
/// Frame time in microseconds
#[no_mangle]
pub extern "C" fn Platform_GetFrameTimeUs() -> c_int {
    TIMING.lock().map(|s| s.last_frame_us as c_int).unwrap_or(0)
}

/// Mark the start of a frame
///
/// Call at the beginning of each game loop iteration.
#[no_mangle]
pub extern "C" fn Platform_FrameStart() {
    frame_start();
}

/// Mark the end of a frame
///
/// Call at the end of each game loop iteration.
#[no_mangle]
pub extern "C" fn Platform_FrameEnd() {
    frame_end();
}

/// Get the total frame count
///
/// # Returns
/// Number of frames since program start (wraps at u32 max)
#[no_mangle]
pub extern "C" fn Platform_GetFrameCount() -> u32 {
    get_frame_count() as u32
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;
    use std::thread;
    use std::time::Duration;

    fn reset_timing() {
        if let Ok(mut state) = TIMING.lock() {
            *state = TimingState::default();
        }
        FRAME_COUNT.store(0, Ordering::Relaxed);
    }

    #[test]
    fn test_frame_timer() {
        let timer = FrameTimer::start();
        thread::sleep(Duration::from_millis(10));

        let elapsed = timer.elapsed_ms();
        assert!(elapsed >= 9.0 && elapsed < 50.0, "Elapsed: {}", elapsed);
    }

    #[test]
    fn test_frame_count() {
        reset_timing();
        assert_eq!(get_frame_count(), 0);

        frame_start();
        frame_end();
        assert_eq!(get_frame_count(), 1);

        frame_start();
        frame_end();
        assert_eq!(get_frame_count(), 2);
    }

    #[test]
    fn test_frame_time_measurement() {
        reset_timing();

        frame_start();
        thread::sleep(Duration::from_millis(5));
        frame_end();

        let frame_time = get_frame_time_ms();
        // Should be approximately 5ms (with some tolerance for scheduling)
        assert!(frame_time >= 4.0 && frame_time < 50.0, "Frame time: {}", frame_time);
    }

    #[test]
    fn test_ffi_functions() {
        reset_timing();

        Platform_FrameStart();
        thread::sleep(Duration::from_millis(1));
        Platform_FrameEnd();

        // FPS won't be accurate after just one frame, but should not crash
        let fps = Platform_GetFPS();
        assert!(fps >= 0);

        let frame_time = Platform_GetFrameTime();
        assert!(frame_time >= 0);

        let frame_count = Platform_GetFrameCount();
        assert!(frame_count >= 1);
    }

    #[test]
    fn test_without_frame_start() {
        reset_timing();

        // Calling frame_end without frame_start should not crash
        frame_end();
        frame_end();
        frame_end();

        // Stats should be at defaults
        assert!(get_fps() >= 0.0);
    }
}
```

### platform/src/lib.rs (MODIFY)
Add the perf module:
```rust
pub mod perf;
```

Add re-exports:
```rust
// Re-export perf FFI functions
pub use perf::{
    Platform_GetFPS,
    Platform_GetFrameTime,
    Platform_GetFrameTimeUs,
    Platform_FrameStart,
    Platform_FrameEnd,
    Platform_GetFrameCount,
};
```

## Verification Command
```bash
ralph-tasks/verify/check-performance.sh
```

## Verification Script
Create `ralph-tasks/verify/check-performance.sh`:
```bash
#!/bin/bash
# Verification script for performance optimization module

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Performance Module ==="

cd "$ROOT_DIR"

# Step 1: Check module files exist
echo "Step 1: Checking module files exist..."
for file in mod.rs simd.rs pool.rs timing.rs; do
    if [ ! -f "platform/src/perf/$file" ]; then
        echo "VERIFY_FAILED: platform/src/perf/$file not found"
        exit 1
    fi
done
echo "  All perf module files exist"

# Step 2: Check lib.rs has perf module
echo "Step 2: Checking lib.rs includes perf module..."
if ! grep -q "pub mod perf" platform/src/lib.rs; then
    echo "VERIFY_FAILED: perf module not declared in lib.rs"
    exit 1
fi
echo "  Perf module declared in lib.rs"

# Step 3: Build the platform crate
echo "Step 3: Building platform crate..."
if ! cargo build --manifest-path platform/Cargo.toml --release 2>&1; then
    echo "VERIFY_FAILED: cargo build failed"
    exit 1
fi
echo "  Build successful"

# Step 4: Run perf tests
echo "Step 4: Running perf tests..."
if ! cargo test --manifest-path platform/Cargo.toml --release -- perf --test-threads=1 2>&1; then
    echo "VERIFY_FAILED: perf tests failed"
    exit 1
fi
echo "  Tests passed"

# Step 5: Run SIMD tests specifically
echo "Step 5: Running SIMD tests..."
if ! cargo test --manifest-path platform/Cargo.toml --release -- simd --test-threads=1 2>&1; then
    echo "VERIFY_FAILED: SIMD tests failed"
    exit 1
fi
echo "  SIMD tests passed"

# Step 6: Generate and check header
echo "Step 6: Checking FFI exports in header..."
if ! cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1; then
    echo "VERIFY_FAILED: cbindgen header generation failed"
    exit 1
fi

# Check for perf FFI exports
for func in Platform_GetFPS Platform_GetFrameTime Platform_FrameStart Platform_FrameEnd Platform_GetFrameCount; do
    if ! grep -q "$func" include/platform.h; then
        echo "VERIFY_FAILED: $func not found in platform.h"
        exit 1
    fi
    echo "  Found $func in header"
done

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Create directory `platform/src/perf/`
2. Create `platform/src/perf/mod.rs`
3. Create `platform/src/perf/simd.rs`
4. Create `platform/src/perf/pool.rs`
5. Create `platform/src/perf/timing.rs`
6. Add `pub mod perf;` to `platform/src/lib.rs`
7. Add FFI re-exports to `platform/src/lib.rs`
8. Run `cargo build --manifest-path platform/Cargo.toml`
9. Run `cargo test --manifest-path platform/Cargo.toml -- perf --test-threads=1`
10. Run verification script

## Success Criteria
- All performance utilities compile
- SIMD palette conversion produces correct results
- Object pool works correctly
- Frame timing measures accurately
- FFI functions appear in platform.h
- All unit tests pass
- Verification script exits with 0

## Common Issues

### "target_feature is not available"
For explicit SIMD intrinsics, use `#[target_feature(enable = "...")]` attributes. The current implementation uses loop unrolling which works everywhere.

### "pool deadlock"
The pool uses a Mutex. Don't hold acquired items while calling pool methods from the same thread.

### "frame time is 0"
Ensure `frame_start()` is called before `frame_end()` each frame.

## Performance Benchmarking
```rust
// To benchmark palette conversion:
use std::time::Instant;

let palette = [0u32; 256];
let src = vec![0u8; 640 * 400];
let mut dest = vec![0u32; 640 * 400];

let start = Instant::now();
for _ in 0..1000 {
    convert_palette_fast(&src, &mut dest, &palette);
}
let elapsed = start.elapsed();
println!("1000 frames: {:?}", elapsed);
println!("Per frame: {:?}", elapsed / 1000);
```

## Completion Promise
When verification passes, output:
```
<promise>TASK_11E_COMPLETE</promise>
```

## Escape Hatch
If stuck after 12 iterations:
- Document what's blocking in `ralph-tasks/blocked/11e.md`
- List attempted approaches
- Output: `<promise>TASK_11E_BLOCKED</promise>`

## Max Iterations
12
