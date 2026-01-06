# Task 05c: Frame Rate Control

## Dependencies
- Task 05b must be complete (Periodic Timers)

## Context
The game needs frame rate limiting to run at the correct speed, plus utility timer classes for game logic (elapsed timers, countdown timers). This task completes the timing system.

## Objective
Implement `FrameRateController` for game loop timing, `Timer` for elapsed time tracking, and `CountdownTimer` for timeout logic. Export frame timing FFI functions.

## Deliverables
- [ ] `FrameRateController` struct with begin/end frame methods
- [ ] `Timer` struct for elapsed time measurement
- [ ] `CountdownTimer` struct for timeout logic
- [ ] Frame timing FFI exports
- [ ] Verification passes

## Files to Modify

### platform/src/timer/mod.rs (MODIFY)
Add after periodic timer code:
```rust
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
```

### platform/src/ffi/mod.rs (MODIFY)
Add frame rate FFI exports:
```rust
// =============================================================================
// Frame Rate Control FFI
// =============================================================================

/// Start frame timing
#[no_mangle]
pub extern "C" fn Platform_Frame_Begin() {
    timer::with_frame_controller(|fc| fc.begin_frame());
}

/// End frame timing (sleeps to maintain target FPS)
#[no_mangle]
pub extern "C" fn Platform_Frame_End() {
    timer::with_frame_controller(|fc| fc.end_frame());
}

/// Set target FPS
#[no_mangle]
pub extern "C" fn Platform_Frame_SetTargetFPS(fps: u32) {
    timer::with_frame_controller(|fc| fc.set_target_fps(fps));
}

/// Get last frame time in seconds
#[no_mangle]
pub extern "C" fn Platform_Frame_GetTime() -> f64 {
    timer::with_frame_controller(|fc| fc.frame_time())
}

/// Get current FPS (rolling average)
#[no_mangle]
pub extern "C" fn Platform_Frame_GetFPS() -> f64 {
    timer::with_frame_controller(|fc| fc.fps())
}
```

## Verification Command
```bash
cd /Users/ooartist/Src/Ronin_CnC && \
cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1 && \
grep -q "Platform_Frame_Begin" include/platform.h && \
grep -q "Platform_Frame_End" include/platform.h && \
grep -q "Platform_Frame_GetFPS" include/platform.h && \
echo "VERIFY_SUCCESS"
```

## Implementation Steps
1. Add `FrameRateController` struct with rolling FPS average
2. Add global `FRAME_CONTROLLER` static
3. Implement `with_frame_controller()` accessor
4. Add `Timer` struct for elapsed time
5. Add `CountdownTimer` struct for timeouts
6. Add frame rate FFI exports
7. Build and verify header generation
8. Run verification command

## Completion Promise
When verification passes, output:
```
<promise>TASK_05C_COMPLETE</promise>
```

## Escape Hatch
If stuck after 10 iterations:
- Document what's blocking in `ralph-tasks/blocked/05c.md`
- List attempted approaches
- Output: `<promise>TASK_05C_BLOCKED</promise>`

## Max Iterations
10
