//! Timer subsystem using std::time

use std::time::{Duration, Instant};
use std::thread;
use std::sync::{Arc, Mutex};
use std::sync::atomic::{AtomicBool, Ordering};
use once_cell::sync::Lazy;

/// Global start time for relative timing
static START_TIME: Lazy<Instant> = Lazy::new(Instant::now);

// =============================================================================
// Basic Time Functions
// =============================================================================

/// Get milliseconds since platform initialization
pub fn get_ticks() -> u32 {
    START_TIME.elapsed().as_millis() as u32
}

/// Get high-precision time in seconds
pub fn get_time() -> f64 {
    START_TIME.elapsed().as_secs_f64()
}

/// Sleep for specified milliseconds
pub fn delay(ms: u32) {
    thread::sleep(Duration::from_millis(ms as u64));
}

/// Get high-resolution performance counter (nanoseconds since start)
pub fn get_performance_counter() -> u64 {
    START_TIME.elapsed().as_nanos() as u64
}

/// Get performance counter frequency (1 GHz = nanosecond resolution)
pub fn get_performance_frequency() -> u64 {
    1_000_000_000
}

// =============================================================================
// Periodic Timers
// =============================================================================

/// Timer handle
pub type TimerHandle = i32;

pub const INVALID_TIMER_HANDLE: TimerHandle = -1;

/// Global timer registry
static TIMERS: Lazy<Mutex<TimerRegistry>> = Lazy::new(|| Mutex::new(TimerRegistry::new()));

struct TimerState {
    active: Arc<AtomicBool>,
}

struct TimerRegistry {
    timers: Vec<Option<TimerState>>,
    next_handle: TimerHandle,
}

impl TimerRegistry {
    fn new() -> Self {
        Self {
            timers: Vec::new(),
            next_handle: 1,
        }
    }

    fn create<F>(&mut self, interval_ms: u32, mut callback: F) -> TimerHandle
    where
        F: FnMut() + Send + 'static,
    {
        let handle = self.next_handle;
        self.next_handle += 1;

        let interval = Duration::from_millis(interval_ms as u64);
        let active = Arc::new(AtomicBool::new(true));
        let active_clone = active.clone();

        // Spawn timer thread
        thread::spawn(move || {
            let mut next_tick = Instant::now() + interval;

            while active_clone.load(Ordering::SeqCst) {
                // Wait until next tick
                let now = Instant::now();
                if now < next_tick {
                    thread::sleep(next_tick - now);
                }
                next_tick += interval;

                // Check if still active before calling
                if !active_clone.load(Ordering::SeqCst) {
                    break;
                }

                // Call callback
                callback();
            }
        });

        // Store state
        let idx = handle as usize;
        if idx >= self.timers.len() {
            self.timers.resize_with(idx + 1, || None);
        }
        self.timers[idx] = Some(TimerState { active });

        handle
    }

    fn destroy(&mut self, handle: TimerHandle) {
        let idx = handle as usize;
        if idx < self.timers.len() {
            if let Some(state) = self.timers[idx].take() {
                state.active.store(false, Ordering::SeqCst);
            }
        }
    }
}

/// Create a periodic timer with a Rust closure
pub fn create_timer<F>(interval_ms: u32, callback: F) -> TimerHandle
where
    F: FnMut() + Send + 'static,
{
    if let Ok(mut registry) = TIMERS.lock() {
        registry.create(interval_ms, callback)
    } else {
        INVALID_TIMER_HANDLE
    }
}

/// Destroy a periodic timer
pub fn destroy_timer(handle: TimerHandle) {
    if let Ok(mut registry) = TIMERS.lock() {
        registry.destroy(handle);
    }
}

/// Create a periodic timer with a C callback
///
/// # Safety
/// The callback and userdata must remain valid for the lifetime of the timer.
pub fn create_timer_c(
    interval_ms: u32,
    callback: extern "C" fn(*mut std::ffi::c_void),
    userdata: *mut std::ffi::c_void,
) -> TimerHandle {
    // Convert raw pointer to usize for Send safety
    let userdata_addr = userdata as usize;

    create_timer(interval_ms, move || {
        callback(userdata_addr as *mut std::ffi::c_void);
    })
}

// =============================================================================
// Frame Rate Control
// =============================================================================

/// Global frame rate controller
static FRAME_CONTROLLER: Lazy<Mutex<FrameRateController>> =
    Lazy::new(|| Mutex::new(FrameRateController::new(60)));

/// Frame rate controller for game loop
pub struct FrameRateController {
    frame_start: Instant,
    target_fps: u32,
    frame_time: f64,
    fps: f64,
    fps_samples: [f64; 60],
    fps_sample_index: usize,
    fps_sample_count: usize,
}

impl FrameRateController {
    pub fn new(target_fps: u32) -> Self {
        Self {
            frame_start: Instant::now(),
            target_fps,
            frame_time: 1.0 / target_fps as f64,
            fps: target_fps as f64,
            fps_samples: [0.0; 60],
            fps_sample_index: 0,
            fps_sample_count: 0,
        }
    }

    /// Call at start of frame
    pub fn begin_frame(&mut self) {
        self.frame_start = Instant::now();
    }

    /// Call at end of frame - handles sleeping and FPS calculation
    pub fn end_frame(&mut self) {
        let elapsed = self.frame_start.elapsed().as_secs_f64();

        // Calculate target frame time
        let target_time = 1.0 / self.target_fps as f64;

        // Sleep if we're running too fast
        let sleep_time = target_time - elapsed;
        if sleep_time > 0.001 {
            thread::sleep(Duration::from_secs_f64(sleep_time));
        }

        // Recalculate actual frame time
        self.frame_time = self.frame_start.elapsed().as_secs_f64();

        // Update FPS counter (rolling average)
        if self.frame_time > 0.0 {
            self.fps_samples[self.fps_sample_index] = 1.0 / self.frame_time;
            self.fps_sample_index = (self.fps_sample_index + 1) % 60;
            if self.fps_sample_count < 60 {
                self.fps_sample_count += 1;
            }

            let sum: f64 = self.fps_samples[..self.fps_sample_count].iter().sum();
            self.fps = sum / self.fps_sample_count as f64;
        }
    }

    /// Set target FPS
    pub fn set_target_fps(&mut self, fps: u32) {
        self.target_fps = fps;
    }

    /// Get frame time in seconds
    pub fn frame_time(&self) -> f64 {
        self.frame_time
    }

    /// Get frames per second (rolling average)
    pub fn fps(&self) -> f64 {
        self.fps
    }
}

/// Access global frame controller
pub fn with_frame_controller<F, R>(f: F) -> R
where
    F: FnOnce(&mut FrameRateController) -> R,
{
    let mut guard = FRAME_CONTROLLER.lock().unwrap();
    f(&mut guard)
}

// =============================================================================
// Game Timer Classes
// =============================================================================

/// Simple elapsed timer
pub struct Timer {
    start_time: Option<Instant>,
    accumulated: Duration,
}

impl Timer {
    pub fn new() -> Self {
        Self {
            start_time: None,
            accumulated: Duration::ZERO,
        }
    }

    pub fn start(&mut self) {
        if self.start_time.is_none() {
            self.start_time = Some(Instant::now());
        }
    }

    pub fn stop(&mut self) {
        if let Some(start) = self.start_time.take() {
            self.accumulated += start.elapsed();
        }
    }

    pub fn reset(&mut self) {
        self.accumulated = Duration::ZERO;
        if self.start_time.is_some() {
            self.start_time = Some(Instant::now());
        }
    }

    pub fn elapsed_ms(&self) -> u32 {
        let running = self.start_time.map(|s| s.elapsed()).unwrap_or(Duration::ZERO);
        (self.accumulated + running).as_millis() as u32
    }

    pub fn is_running(&self) -> bool {
        self.start_time.is_some()
    }
}

impl Default for Timer {
    fn default() -> Self {
        Self::new()
    }
}

/// Countdown timer
pub struct CountdownTimer {
    start_time: Option<Instant>,
    duration: Duration,
}

impl CountdownTimer {
    pub fn new() -> Self {
        Self {
            start_time: None,
            duration: Duration::ZERO,
        }
    }

    pub fn set(&mut self, ms: u32) {
        self.start_time = Some(Instant::now());
        self.duration = Duration::from_millis(ms as u64);
    }

    pub fn clear(&mut self) {
        self.start_time = None;
        self.duration = Duration::ZERO;
    }

    pub fn remaining_ms(&self) -> u32 {
        if let Some(start) = self.start_time {
            let elapsed = start.elapsed();
            if elapsed >= self.duration {
                0
            } else {
                (self.duration - elapsed).as_millis() as u32
            }
        } else {
            0
        }
    }

    pub fn expired(&self) -> bool {
        if let Some(start) = self.start_time {
            start.elapsed() >= self.duration
        } else {
            true
        }
    }

    pub fn is_active(&self) -> bool {
        self.start_time.is_some() && !self.expired()
    }
}

impl Default for CountdownTimer {
    fn default() -> Self {
        Self::new()
    }
}
