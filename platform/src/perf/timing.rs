//! Frame timing utilities
//!
//! Provides FPS measurement and frame time tracking.

use std::sync::atomic::{AtomicU32, AtomicU64, Ordering};
use std::time::{Duration, Instant};

// =============================================================================
// Global Frame Timer State
// =============================================================================

/// Current frame number
static FRAME_COUNT: AtomicU64 = AtomicU64::new(0);

/// Current FPS (multiplied by 100 for precision without floats)
static FPS_X100: AtomicU32 = AtomicU32::new(6000); // 60.00 FPS default

/// Last frame time in microseconds
static FRAME_TIME_US: AtomicU32 = AtomicU32::new(16667); // ~60 FPS

/// Frame start time storage (using AtomicU64 for cross-thread safety)
static FRAME_START_NS: AtomicU64 = AtomicU64::new(0);

// =============================================================================
// Frame Timer Struct (for local use)
// =============================================================================

/// Frame timing helper
pub struct FrameTimer {
    start_time: Instant,
    frame_count: u64,
    last_fps_update: Instant,
    fps_frame_count: u64,
    current_fps: f32,
    frame_times: [f32; 60], // Rolling window
    frame_time_idx: usize,
}

impl FrameTimer {
    /// Create a new frame timer
    pub fn new() -> Self {
        let now = Instant::now();
        Self {
            start_time: now,
            frame_count: 0,
            last_fps_update: now,
            fps_frame_count: 0,
            current_fps: 60.0,
            frame_times: [16.667; 60],
            frame_time_idx: 0,
        }
    }

    /// Mark the start of a frame
    pub fn begin_frame(&mut self) {
        self.start_time = Instant::now();
    }

    /// Mark the end of a frame
    ///
    /// Returns the frame time in milliseconds
    pub fn end_frame(&mut self) -> f32 {
        let elapsed = self.start_time.elapsed();
        let frame_time_ms = elapsed.as_secs_f32() * 1000.0;

        // Update rolling average
        self.frame_times[self.frame_time_idx] = frame_time_ms;
        self.frame_time_idx = (self.frame_time_idx + 1) % 60;

        self.frame_count += 1;
        self.fps_frame_count += 1;

        // Update FPS every second
        let fps_elapsed = self.last_fps_update.elapsed();
        if fps_elapsed >= Duration::from_secs(1) {
            self.current_fps = self.fps_frame_count as f32 / fps_elapsed.as_secs_f32();
            self.last_fps_update = Instant::now();
            self.fps_frame_count = 0;
        }

        frame_time_ms
    }

    /// Get current FPS
    pub fn fps(&self) -> f32 {
        self.current_fps
    }

    /// Get average frame time in milliseconds
    pub fn average_frame_time(&self) -> f32 {
        let sum: f32 = self.frame_times.iter().sum();
        sum / 60.0
    }

    /// Get total frame count
    pub fn total_frames(&self) -> u64 {
        self.frame_count
    }
}

impl Default for FrameTimer {
    fn default() -> Self {
        Self::new()
    }
}

// =============================================================================
// Global Timing Functions
// =============================================================================

/// Get current FPS (global state)
pub fn get_fps() -> f32 {
    FPS_X100.load(Ordering::Relaxed) as f32 / 100.0
}

/// Get last frame time in milliseconds (global state)
pub fn get_frame_time_ms() -> f32 {
    FRAME_TIME_US.load(Ordering::Relaxed) as f32 / 1000.0
}

/// Get total frame count
pub fn get_frame_count() -> u64 {
    FRAME_COUNT.load(Ordering::Relaxed)
}

/// Mark frame start (global state)
pub fn frame_start() {
    let _now = Instant::now();
    // Store as nanoseconds since UNIX epoch
    // We just need consistency within a frame
    FRAME_START_NS.store(
        std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .unwrap_or_default()
            .as_nanos() as u64,
        Ordering::Release
    );
}

/// Mark frame end (global state)
///
/// Updates FPS and frame time counters.
pub fn frame_end() {
    let end_ns = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap_or_default()
        .as_nanos() as u64;

    let start_ns = FRAME_START_NS.load(Ordering::Acquire);

    if start_ns > 0 && end_ns > start_ns {
        let frame_time_us = ((end_ns - start_ns) / 1000) as u32;
        FRAME_TIME_US.store(frame_time_us, Ordering::Relaxed);

        // Calculate FPS from frame time
        if frame_time_us > 0 {
            let fps_x100 = (100_000_000 / frame_time_us) as u32;
            FPS_X100.store(fps_x100, Ordering::Relaxed);
        }
    }

    FRAME_COUNT.fetch_add(1, Ordering::Relaxed);
}

// =============================================================================
// FFI Exports
// =============================================================================

use std::os::raw::c_int;

/// Get current FPS
///
/// # Returns
/// Current frames per second as integer (truncated)
#[no_mangle]
pub extern "C" fn Platform_GetFPS() -> c_int {
    get_fps() as c_int
}

/// Get last frame time in milliseconds
///
/// # Returns
/// Frame time in milliseconds (truncated to integer)
#[no_mangle]
pub extern "C" fn Platform_GetFrameTime() -> c_int {
    get_frame_time_ms() as c_int
}

/// Get total frame count
///
/// # Returns
/// Total number of frames rendered
#[no_mangle]
pub extern "C" fn Platform_GetFrameCount() -> u64 {
    get_frame_count()
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
/// This updates the FPS and frame time counters.
#[no_mangle]
pub extern "C" fn Platform_FrameEnd() {
    frame_end();
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;
    use std::thread;
    use std::time::Duration;

    #[test]
    fn test_frame_timer_new() {
        let timer = FrameTimer::new();
        assert!(timer.fps() > 0.0);
        assert_eq!(timer.total_frames(), 0);
    }

    #[test]
    fn test_frame_timer_basic() {
        let mut timer = FrameTimer::new();

        for _ in 0..10 {
            timer.begin_frame();
            thread::sleep(Duration::from_millis(10));
            let frame_time = timer.end_frame();

            // Frame time should be approximately 10ms (with some overhead)
            assert!(frame_time >= 9.0 && frame_time < 50.0,
                   "Frame time {} not in expected range", frame_time);
        }

        assert_eq!(timer.total_frames(), 10);
    }

    #[test]
    fn test_frame_timer_average() {
        let mut timer = FrameTimer::new();

        // Fill the rolling window
        for _ in 0..60 {
            timer.begin_frame();
            thread::sleep(Duration::from_millis(5));
            timer.end_frame();
        }

        let avg = timer.average_frame_time();
        // Should be around 5ms (with overhead)
        assert!(avg >= 4.0 && avg < 20.0, "Average {} not in expected range", avg);
    }

    #[test]
    fn test_global_frame_functions() {
        // Reset state
        FRAME_COUNT.store(0, Ordering::SeqCst);

        frame_start();
        thread::sleep(Duration::from_millis(10));
        frame_end();

        assert_eq!(get_frame_count(), 1);

        // Frame time should be around 10ms
        let ft = get_frame_time_ms();
        assert!(ft >= 9.0 && ft < 50.0, "Frame time {} not in expected range", ft);
    }

    #[test]
    fn test_ffi_functions() {
        // Reset state
        FRAME_COUNT.store(0, Ordering::SeqCst);
        FPS_X100.store(6000, Ordering::SeqCst);
        FRAME_TIME_US.store(16667, Ordering::SeqCst);

        assert_eq!(Platform_GetFPS(), 60);
        assert!(Platform_GetFrameTime() >= 16 && Platform_GetFrameTime() <= 17);
        assert_eq!(Platform_GetFrameCount(), 0);

        Platform_FrameStart();
        thread::sleep(Duration::from_millis(10));
        Platform_FrameEnd();

        assert_eq!(Platform_GetFrameCount(), 1);
    }
}
