# Task 05b: Periodic Timers

## Dependencies
- Task 05a must be complete (Basic Timing)

## Context
The original game uses Windows multimedia timers (`timeSetEvent`/`timeKillEvent`) for periodic callbacks. This task implements a thread-based periodic timer system that can call C callbacks at fixed intervals.

## Objective
Implement a periodic timer registry that spawns threads to call callbacks at specified intervals. Support creation, destruction, and safe C callback wrapping.

## Deliverables
- [ ] `TimerRegistry` with thread-based periodic timers
- [ ] `create_timer()` and `destroy_timer()` functions
- [ ] C callback FFI exports
- [ ] Verification passes

## Files to Modify

### platform/src/timer/mod.rs (MODIFY)
Add after the basic timing functions:
```rust
use std::sync::{Arc, Mutex};

// =============================================================================
// Periodic Timers
// =============================================================================

/// Timer handle
pub type TimerHandle = i32;

pub const INVALID_TIMER_HANDLE: TimerHandle = -1;

/// Global timer registry
static TIMERS: Lazy<Mutex<TimerRegistry>> = Lazy::new(|| Mutex::new(TimerRegistry::new()));

struct TimerState {
    active: Arc<std::sync::atomic::AtomicBool>,
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
        let active = Arc::new(std::sync::atomic::AtomicBool::new(true));
        let active_clone = active.clone();

        // Spawn timer thread
        thread::spawn(move || {
            let mut next_tick = Instant::now() + interval;

            while active_clone.load(std::sync::atomic::Ordering::SeqCst) {
                // Wait until next tick
                let now = Instant::now();
                if now < next_tick {
                    thread::sleep(next_tick - now);
                }
                next_tick += interval;

                // Check if still active before calling
                if !active_clone.load(std::sync::atomic::Ordering::SeqCst) {
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
                state.active.store(false, std::sync::atomic::Ordering::SeqCst);
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
```

### platform/src/ffi/mod.rs (MODIFY)
Add periodic timer FFI exports:
```rust
use std::ffi::c_void;
use crate::timer::{TimerHandle, INVALID_TIMER_HANDLE};

/// C timer callback type
pub type CTimerCallback = extern "C" fn(*mut c_void);

/// Create a periodic timer
/// Note: The callback will be called from a separate thread!
#[no_mangle]
pub extern "C" fn Platform_Timer_Create(
    interval_ms: u32,
    callback: CTimerCallback,
    userdata: *mut c_void,
) -> TimerHandle {
    timer::create_timer_c(interval_ms, callback, userdata)
}

/// Destroy a periodic timer
#[no_mangle]
pub extern "C" fn Platform_Timer_Destroy(handle: TimerHandle) {
    timer::destroy_timer(handle);
}

/// Get invalid timer handle constant
#[no_mangle]
pub extern "C" fn Platform_Timer_InvalidHandle() -> TimerHandle {
    INVALID_TIMER_HANDLE
}
```

## Verification Command
```bash
cd /Users/ooartist/Src/Ronin_CnC && \
cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1 && \
grep -q "Platform_Timer_Create" include/platform.h && \
grep -q "Platform_Timer_Destroy" include/platform.h && \
cargo test --manifest-path platform/Cargo.toml 2>&1 | grep -q "passed\|0 passed" && \
echo "VERIFY_SUCCESS"
```

## Implementation Steps
1. Add `Arc`, `Mutex` imports to timer module
2. Implement `TimerState` and `TimerRegistry` structs
3. Implement `create_timer()` with thread spawning
4. Implement `destroy_timer()` with atomic flag
5. Add `create_timer_c()` for C callback support
6. Add FFI exports for C interface
7. Build and verify header generation
8. Run verification command

## Completion Promise
When verification passes, output:
```
<promise>TASK_05B_COMPLETE</promise>
```

## Escape Hatch
If stuck after 12 iterations:
- Document what's blocking in `ralph-tasks/blocked/05b.md`
- List attempted approaches
- Output: `<promise>TASK_05B_BLOCKED</promise>`

## Max Iterations
12
