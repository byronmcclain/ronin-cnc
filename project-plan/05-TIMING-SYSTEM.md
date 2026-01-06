# Plan 05: Timing System (Rust std::time)

## Objective
Replace Windows timing APIs (multimedia timers, QueryPerformanceCounter) with Rust's std::time, ensuring the game runs at the correct speed.

## Current State Analysis

### Windows Timing in Codebase

**Primary Files:**
- `WIN32LIB/TIMER/TIMER.CPP`
- `WIN32LIB/TIMER/TIMER.H`
- `CODE/STARTUP.CPP` (lines 351-361)

**Windows APIs Used:**
```cpp
QueryPerformanceCounter(&counter);
QueryPerformanceFrequency(&frequency);
timeSetEvent(interval, resolution, callback, data, TIME_PERIODIC);
timeKillEvent(timerID);
GetTickCount();
Sleep(milliseconds);
```

## Rust Implementation

### 5.1 Core Timing Functions

File: `platform/src/timer/mod.rs`
```rust
//! Timer subsystem using std::time

use std::time::{Duration, Instant};
use std::sync::{Arc, Mutex};
use std::thread;
use once_cell::sync::Lazy;

/// Global start time for relative timing
static START_TIME: Lazy<Instant> = Lazy::new(Instant::now);

/// Periodic timer registry
static TIMERS: Lazy<Mutex<TimerRegistry>> = Lazy::new(|| Mutex::new(TimerRegistry::new()));

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

/// Timer callback type (called from timer thread)
pub type TimerCallback = Box<dyn FnMut() + Send + 'static>;

/// Timer handle
pub type TimerHandle = i32;

pub const INVALID_TIMER_HANDLE: TimerHandle = -1;

struct PeriodicTimer {
    interval: Duration,
    callback: TimerCallback,
    active: bool,
    thread_handle: Option<thread::JoinHandle<()>>,
}

struct TimerRegistry {
    timers: Vec<Option<Arc<Mutex<TimerState>>>>,
    next_handle: TimerHandle,
}

struct TimerState {
    active: bool,
}

impl TimerRegistry {
    fn new() -> Self {
        Self {
            timers: Vec::new(),
            next_handle: 1,
        }
    }

    fn create(&mut self, interval_ms: u32, mut callback: TimerCallback) -> TimerHandle {
        let handle = self.next_handle;
        self.next_handle += 1;

        let interval = Duration::from_millis(interval_ms as u64);
        let state = Arc::new(Mutex::new(TimerState { active: true }));
        let state_clone = state.clone();

        // Spawn timer thread
        let _thread_handle = thread::spawn(move || {
            let mut next_tick = Instant::now() + interval;

            loop {
                // Check if still active
                if let Ok(s) = state_clone.lock() {
                    if !s.active {
                        break;
                    }
                } else {
                    break;
                }

                // Wait until next tick
                let now = Instant::now();
                if now < next_tick {
                    thread::sleep(next_tick - now);
                }
                next_tick += interval;

                // Call callback
                callback();
            }
        });

        // Store state
        let idx = handle as usize;
        if idx >= self.timers.len() {
            self.timers.resize(idx + 1, None);
        }
        self.timers[idx] = Some(state);

        handle
    }

    fn destroy(&mut self, handle: TimerHandle) {
        let idx = handle as usize;
        if idx < self.timers.len() {
            if let Some(state) = &self.timers[idx] {
                if let Ok(mut s) = state.lock() {
                    s.active = false;
                }
            }
            self.timers[idx] = None;
        }
    }
}

/// Create a periodic timer
pub fn create_timer(interval_ms: u32, callback: TimerCallback) -> TimerHandle {
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

// =============================================================================
// Frame Rate Control
// =============================================================================

/// Frame rate controller for game loop
pub struct FrameRateController {
    frame_start: Instant,
    target_fps: u32,
    frame_time: f64,
    fps: f64,
    fps_samples: [f64; 60],
    fps_sample_index: usize,
}

impl FrameRateController {
    pub fn new(target_fps: u32) -> Self {
        Self {
            frame_start: Instant::now(),
            target_fps,
            frame_time: 0.0,
            fps: 0.0,
            fps_samples: [0.0; 60],
            fps_sample_index: 0,
        }
    }

    /// Call at start of frame
    pub fn begin_frame(&mut self) {
        self.frame_start = Instant::now();
    }

    /// Call at end of frame - handles sleeping and FPS calculation
    pub fn end_frame(&mut self) {
        let frame_end = Instant::now();
        let elapsed = (frame_end - self.frame_start).as_secs_f64();

        // Calculate target frame time
        let target_time = 1.0 / self.target_fps as f64;

        // Sleep if we're running too fast
        let sleep_time = target_time - elapsed;
        if sleep_time > 0.001 {
            thread::sleep(Duration::from_secs_f64(sleep_time));
        }

        // Recalculate actual frame time
        let actual_frame_end = Instant::now();
        self.frame_time = (actual_frame_end - self.frame_start).as_secs_f64();

        // Update FPS counter (rolling average)
        self.fps_samples[self.fps_sample_index] = 1.0 / self.frame_time;
        self.fps_sample_index = (self.fps_sample_index + 1) % 60;

        let sum: f64 = self.fps_samples.iter().sum();
        self.fps = sum / 60.0;
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

// =============================================================================
// Game Timer Classes (for compatibility)
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
```

### 5.2 FFI Timer Exports

File: `platform/src/ffi/timer.rs`
```rust
//! Timer FFI exports

use crate::timer::{self, TimerHandle, INVALID_TIMER_HANDLE};
use std::ffi::c_void;

// =============================================================================
// Basic Time Functions
// =============================================================================

/// Get milliseconds since platform init
#[no_mangle]
pub extern "C" fn Platform_GetTicks() -> u32 {
    timer::get_ticks()
}

/// Sleep for milliseconds
#[no_mangle]
pub extern "C" fn Platform_Delay(milliseconds: u32) {
    timer::delay(milliseconds);
}

/// Get high-resolution performance counter
#[no_mangle]
pub extern "C" fn Platform_GetPerformanceCounter() -> u64 {
    timer::get_performance_counter()
}

/// Get performance counter frequency
#[no_mangle]
pub extern "C" fn Platform_GetPerformanceFrequency() -> u64 {
    timer::get_performance_frequency()
}

/// Get time in seconds (floating point)
#[no_mangle]
pub extern "C" fn Platform_GetTime() -> f64 {
    timer::get_time()
}

// =============================================================================
// Periodic Timers
// =============================================================================

/// Timer callback type for C
pub type CTimerCallback = extern "C" fn(*mut c_void);

/// Create a periodic timer
/// Note: The callback will be called from a separate thread!
#[no_mangle]
pub extern "C" fn Platform_Timer_Create(
    interval_ms: u32,
    callback: CTimerCallback,
    userdata: *mut c_void,
) -> TimerHandle {
    // Wrap C callback
    let userdata = userdata as usize; // Convert to usize for Send
    let rust_callback = Box::new(move || {
        callback(userdata as *mut c_void);
    });

    timer::create_timer(interval_ms, rust_callback)
}

/// Destroy a periodic timer
#[no_mangle]
pub extern "C" fn Platform_Timer_Destroy(handle: TimerHandle) {
    timer::destroy_timer(handle);
}

/// Set timer interval (may require recreating timer)
#[no_mangle]
pub extern "C" fn Platform_Timer_SetInterval(_handle: TimerHandle, _interval_ms: u32) {
    // Not easily supported - would need to recreate timer
    // Original code rarely changes intervals
}

// =============================================================================
// Frame Rate Control
// =============================================================================

/// Start frame timing
#[no_mangle]
pub extern "C" fn Platform_Frame_Start() {
    // Called from game loop
}

/// End frame timing with target FPS
#[no_mangle]
pub extern "C" fn Platform_Frame_End(_target_fps: i32) {
    // Called from game loop
}

/// Get last frame time in seconds
#[no_mangle]
pub extern "C" fn Platform_GetFrameTime() -> f64 {
    0.016 // Default to 60 FPS for now
}

/// Get current FPS
#[no_mangle]
pub extern "C" fn Platform_GetFPS() -> f64 {
    60.0 // Default for now
}
```

## Tasks Breakdown

### Phase 1: Basic Timing (1 day)
- [ ] Implement `get_ticks()`, `delay()`
- [ ] Implement high-precision counter
- [ ] Implement `get_time()`
- [ ] Test timing accuracy

### Phase 2: Periodic Timers (1 day)
- [ ] Implement timer thread spawning
- [ ] Implement timer creation/destruction
- [ ] Handle callback safely
- [ ] Test with periodic callback

### Phase 3: FFI Exports (1 day)
- [ ] Create FFI wrapper functions
- [ ] Handle C callback conversion
- [ ] Verify cbindgen output
- [ ] Test from C++

### Phase 4: Frame Rate Control (1 day)
- [ ] Implement `FrameRateController`
- [ ] Implement game timer classes
- [ ] Verify game runs at correct speed

## Acceptance Criteria

- [ ] `get_ticks()` accurate to ±1ms
- [ ] High-precision timer accurate to microseconds
- [ ] Periodic timers fire at correct rate
- [ ] Callbacks work from timer thread
- [ ] Game logic runs at 15 ticks/second
- [ ] No timer-related crashes

## Estimated Duration
**3-4 days**

## Dependencies
- Plan 02 (Platform Abstraction) completed
- std::time (stable Rust)

## Next Plan
Once timing is working, proceed to **Plan 06: Memory Management**
