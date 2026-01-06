# Task 05a: Basic Timing Functions

## Dependencies
- Task 04d must be complete (Input System)
- Phase 02 Platform Abstraction complete

## Context
The timing system provides millisecond and high-precision timing for game logic, frame rate control, and periodic callbacks. This task implements the core timing functions using Rust's `std::time`.

## Objective
Create the timer module with basic timing functions: `get_ticks()`, `delay()`, `get_time()`, and high-precision performance counter functions. Export these via FFI.

## Deliverables
- [ ] `platform/src/timer/mod.rs` - Core timing implementation
- [ ] FFI exports added to `platform/src/ffi/mod.rs`
- [ ] C header updated with timing functions
- [ ] Verification passes

## Files to Create/Modify

### platform/src/timer/mod.rs (NEW)
```rust
//! Timer subsystem using std::time

use std::time::{Duration, Instant};
use std::thread;
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
```

### platform/src/lib.rs (MODIFY)
Add after `pub mod input;`:
```rust
pub mod timer;
```

### platform/src/ffi/mod.rs (MODIFY)
Add timer FFI exports:
```rust
use crate::timer;

// =============================================================================
// Timer FFI
// =============================================================================

/// Get milliseconds since platform init
#[no_mangle]
pub extern "C" fn Platform_Timer_GetTicks() -> u32 {
    timer::get_ticks()
}

/// Sleep for milliseconds
#[no_mangle]
pub extern "C" fn Platform_Timer_Delay(milliseconds: u32) {
    timer::delay(milliseconds);
}

/// Get high-resolution performance counter
#[no_mangle]
pub extern "C" fn Platform_Timer_GetPerformanceCounter() -> u64 {
    timer::get_performance_counter()
}

/// Get performance counter frequency
#[no_mangle]
pub extern "C" fn Platform_Timer_GetPerformanceFrequency() -> u64 {
    timer::get_performance_frequency()
}

/// Get time in seconds (floating point)
#[no_mangle]
pub extern "C" fn Platform_Timer_GetTime() -> f64 {
    timer::get_time()
}
```

## Verification Command
```bash
cd /Users/ooartist/Src/Ronin_CnC && \
cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1 && \
grep -q "Platform_Timer_GetTicks" include/platform.h && \
grep -q "Platform_Timer_GetPerformanceCounter" include/platform.h && \
echo "VERIFY_SUCCESS"
```

## Implementation Steps
1. Create `platform/src/timer/mod.rs` with basic timing functions
2. Add `pub mod timer;` to `platform/src/lib.rs`
3. Add timer FFI exports to `platform/src/ffi/mod.rs`
4. Build with cbindgen to generate header
5. Verify header contains timer functions
6. Run verification command

## Completion Promise
When verification passes, output:
```
<promise>TASK_05A_COMPLETE</promise>
```

## Escape Hatch
If stuck after 10 iterations:
- Document what's blocking in `ralph-tasks/blocked/05a.md`
- List attempted approaches
- Output: `<promise>TASK_05A_BLOCKED</promise>`

## Max Iterations
10
