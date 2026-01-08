# Task 11d: Application Lifecycle Management

## Dependencies
- Task 11b (Logging) should be complete for proper logging
- Input module must be in place

## Context
macOS applications have a well-defined lifecycle with events for:
- **Entering background**: User switched away, hide audio, pause game
- **Entering foreground**: User returned, resume game
- **Terminating**: App is closing, save state, cleanup
- **Low memory**: System needs memory, release caches

This task creates an application lifecycle module that:
- Tracks application state (active, background, terminating)
- Hooks into SDL2 application events
- Provides callbacks/notifications for state changes
- Exposes state to C++ code via FFI

Proper lifecycle handling ensures:
- Game pauses when user switches to another app
- Audio stops when backgrounded (macOS users expect this)
- Save games are flushed on termination
- Resources are released gracefully

## Objective
Create an application lifecycle module that:
1. Defines application states (Active, Background, Terminating)
2. Tracks current state atomically
3. Processes SDL2 application events
4. Provides FFI exports for C++ code
5. Integrates with the input event loop

## Deliverables
- [ ] `platform/src/app/mod.rs` - Module root
- [ ] `platform/src/app/lifecycle.rs` - Lifecycle state machine
- [ ] FFI exports: `Platform_GetAppState`, `Platform_IsAppActive`, `Platform_IsAppBackground`, `Platform_RequestQuit`, `Platform_ShouldQuit`
- [ ] Header contains FFI exports
- [ ] Unit tests pass
- [ ] Verification passes

## Technical Background

### Application States
```
                    ┌─────────────────┐
                    │    Starting     │
                    └────────┬────────┘
                             │ Platform_Init()
                    ┌────────▼────────┐
        ┌──────────►│     Active      │◄──────────┐
        │           └────────┬────────┘           │
        │                    │                    │
        │ AppDidEnter        │ AppWillEnter       │
        │ Foreground         │ Background         │
        │                    │                    │
        │           ┌────────▼────────┐           │
        └───────────│   Background    │───────────┘
                    └────────┬────────┘
                             │ AppTerminating
                    ┌────────▼────────┐
                    │   Terminating   │
                    └────────┬────────┘
                             │ Platform_Shutdown()
                    ┌────────▼────────┐
                    │    Shutdown     │
                    └─────────────────┘
```

### SDL2 Application Events
- `AppWillEnterBackground` - About to go to background
- `AppDidEnterBackground` - Now in background
- `AppWillEnterForeground` - About to become active
- `AppDidEnterForeground` - Now active
- `AppTerminating` - App is being terminated
- `AppLowMemory` - Low memory warning (iOS/mobile, rare on macOS)

### Quit Handling
The game should check `Platform_ShouldQuit()` each frame:
- User closes window → triggers quit request
- `Cmd+Q` → triggers quit request
- Signal handler → triggers quit request
- Game checks flag and exits gracefully

## Files to Create/Modify

### platform/src/app/mod.rs (NEW)
```rust
//! Application management for Red Alert
//!
//! Provides:
//! - Application lifecycle state tracking
//! - Background/foreground transitions
//! - Quit request handling

pub mod lifecycle;

pub use lifecycle::{
    AppState,
    get_app_state,
    set_app_state,
    is_app_active,
    is_app_background,
    request_quit,
    should_quit,
    clear_quit_request,
    handle_app_event,
};

// Re-export FFI functions for cbindgen
pub use lifecycle::{
    Platform_GetAppState,
    Platform_IsAppActive,
    Platform_IsAppBackground,
    Platform_RequestQuit,
    Platform_ShouldQuit,
    Platform_ClearQuitRequest,
};
```

### platform/src/app/lifecycle.rs (NEW)
```rust
//! Application lifecycle state machine
//!
//! Tracks application state and handles transitions:
//! - Active: Normal operation, game running
//! - Background: User switched away, game paused
//! - Terminating: App is shutting down

use sdl2::event::Event;
use std::sync::atomic::{AtomicBool, AtomicU8, Ordering};

// =============================================================================
// Application State
// =============================================================================

/// Application lifecycle state
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum AppState {
    /// App is active and in foreground
    Active = 0,
    /// App is in background (user switched away)
    Background = 1,
    /// App is terminating
    Terminating = 2,
}

impl AppState {
    fn to_u8(self) -> u8 {
        match self {
            AppState::Active => 0,
            AppState::Background => 1,
            AppState::Terminating => 2,
        }
    }

    fn from_u8(v: u8) -> Self {
        match v {
            0 => AppState::Active,
            1 => AppState::Background,
            2 => AppState::Terminating,
            _ => AppState::Active,
        }
    }
}

impl Default for AppState {
    fn default() -> Self {
        AppState::Active
    }
}

// =============================================================================
// Global State
// =============================================================================

/// Current application state (atomic for lock-free access)
static APP_STATE: AtomicU8 = AtomicU8::new(0); // Default: Active

/// Quit request flag
static QUIT_REQUESTED: AtomicBool = AtomicBool::new(false);

// =============================================================================
// State Access Functions
// =============================================================================

/// Get the current application state
pub fn get_app_state() -> AppState {
    AppState::from_u8(APP_STATE.load(Ordering::SeqCst))
}

/// Set the application state
///
/// This is typically called by the event handler, not directly by game code.
pub fn set_app_state(state: AppState) {
    let old = AppState::from_u8(APP_STATE.swap(state.to_u8(), Ordering::SeqCst));
    if old != state {
        log::info!("App state changed: {:?} -> {:?}", old, state);
    }
}

/// Check if app is active (foreground)
pub fn is_app_active() -> bool {
    get_app_state() == AppState::Active
}

/// Check if app is in background
pub fn is_app_background() -> bool {
    get_app_state() == AppState::Background
}

/// Check if app is terminating
pub fn is_app_terminating() -> bool {
    get_app_state() == AppState::Terminating
}

// =============================================================================
// Quit Handling
// =============================================================================

/// Request application quit
///
/// Sets a flag that should be checked by the main loop.
/// The game should save state and exit gracefully.
pub fn request_quit() {
    log::info!("Quit requested");
    QUIT_REQUESTED.store(true, Ordering::SeqCst);
}

/// Check if quit has been requested
pub fn should_quit() -> bool {
    QUIT_REQUESTED.load(Ordering::SeqCst)
}

/// Clear the quit request
///
/// Called if quit was cancelled (e.g., user said "No" to save prompt).
pub fn clear_quit_request() {
    QUIT_REQUESTED.store(false, Ordering::SeqCst);
}

// =============================================================================
// Event Handling
// =============================================================================

/// Handle SDL2 application events
///
/// Call this for each event in your event loop.
/// Returns the new AppState if it changed, None otherwise.
///
/// # Example
/// ```ignore
/// for event in event_pump.poll_iter() {
///     if let Some(new_state) = handle_app_event(&event) {
///         match new_state {
///             AppState::Background => pause_game(),
///             AppState::Active => resume_game(),
///             AppState::Terminating => save_and_exit(),
///         }
///     }
///     // ... handle other events
/// }
/// ```
pub fn handle_app_event(event: &Event) -> Option<AppState> {
    match event {
        // Going to background
        Event::AppWillEnterBackground { .. } => {
            log::debug!("App will enter background");
            None // State changes on DidEnter
        }

        Event::AppDidEnterBackground { .. } => {
            log::info!("App entered background");
            set_app_state(AppState::Background);
            Some(AppState::Background)
        }

        // Coming to foreground
        Event::AppWillEnterForeground { .. } => {
            log::debug!("App will enter foreground");
            None // State changes on DidEnter
        }

        Event::AppDidEnterForeground { .. } => {
            log::info!("App entered foreground");
            set_app_state(AppState::Active);
            Some(AppState::Active)
        }

        // Terminating
        Event::AppTerminating { .. } => {
            log::info!("App terminating");
            set_app_state(AppState::Terminating);
            request_quit();
            Some(AppState::Terminating)
        }

        // Low memory (less relevant on macOS, but handle it)
        Event::AppLowMemory { .. } => {
            log::warn!("Low memory warning received");
            // Could trigger cache cleanup here
            None
        }

        // Quit events
        Event::Quit { .. } => {
            log::info!("Quit event received");
            request_quit();
            None
        }

        _ => None,
    }
}

// =============================================================================
// FFI Exports
// =============================================================================

use std::os::raw::c_int;

/// Get the current application state
///
/// # Returns
/// - 0: Active (foreground)
/// - 1: Background
/// - 2: Terminating
#[no_mangle]
pub extern "C" fn Platform_GetAppState() -> c_int {
    get_app_state() as c_int
}

/// Check if application is active (foreground)
///
/// # Returns
/// - 1 if active
/// - 0 if not active (background or terminating)
#[no_mangle]
pub extern "C" fn Platform_IsAppActive() -> c_int {
    if is_app_active() { 1 } else { 0 }
}

/// Check if application is in background
///
/// # Returns
/// - 1 if in background
/// - 0 if not in background
#[no_mangle]
pub extern "C" fn Platform_IsAppBackground() -> c_int {
    if is_app_background() { 1 } else { 0 }
}

/// Request application quit
///
/// Sets a flag that should be checked each frame with Platform_ShouldQuit().
/// The game should save state and exit gracefully.
#[no_mangle]
pub extern "C" fn Platform_RequestQuit() {
    request_quit();
}

/// Check if quit has been requested
///
/// The main game loop should check this each frame:
/// ```c
/// while (!Platform_ShouldQuit()) {
///     // game loop
/// }
/// ```
///
/// # Returns
/// - 1 if quit requested
/// - 0 if not
#[no_mangle]
pub extern "C" fn Platform_ShouldQuit() -> c_int {
    if should_quit() { 1 } else { 0 }
}

/// Clear quit request
///
/// Call if quit was cancelled (e.g., user declined to save).
#[no_mangle]
pub extern "C" fn Platform_ClearQuitRequest() {
    clear_quit_request();
}

/// Process application events for lifecycle changes
///
/// This should be called by the input processing code for each SDL event.
/// It handles app lifecycle events and updates state accordingly.
///
/// # Returns
/// - The new AppState value if state changed
/// - -1 if no state change
#[no_mangle]
pub extern "C" fn Platform_HandleAppEvent() -> c_int {
    // Note: This is a simplified FFI. The actual event handling happens
    // in the input module which calls handle_app_event() internally.
    // This function just returns current state for polling.
    get_app_state() as c_int
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    fn reset_state() {
        APP_STATE.store(AppState::Active.to_u8(), Ordering::SeqCst);
        QUIT_REQUESTED.store(false, Ordering::SeqCst);
    }

    #[test]
    fn test_initial_state_is_active() {
        reset_state();
        assert_eq!(get_app_state(), AppState::Active);
        assert!(is_app_active());
        assert!(!is_app_background());
        assert!(!is_app_terminating());
    }

    #[test]
    fn test_set_app_state() {
        reset_state();

        set_app_state(AppState::Background);
        assert_eq!(get_app_state(), AppState::Background);
        assert!(is_app_background());
        assert!(!is_app_active());

        set_app_state(AppState::Terminating);
        assert_eq!(get_app_state(), AppState::Terminating);
        assert!(is_app_terminating());

        set_app_state(AppState::Active);
        assert_eq!(get_app_state(), AppState::Active);
        assert!(is_app_active());
    }

    #[test]
    fn test_quit_request() {
        reset_state();

        assert!(!should_quit());

        request_quit();
        assert!(should_quit());

        clear_quit_request();
        assert!(!should_quit());
    }

    #[test]
    fn test_app_state_conversion() {
        assert_eq!(AppState::Active.to_u8(), 0);
        assert_eq!(AppState::Background.to_u8(), 1);
        assert_eq!(AppState::Terminating.to_u8(), 2);

        assert_eq!(AppState::from_u8(0), AppState::Active);
        assert_eq!(AppState::from_u8(1), AppState::Background);
        assert_eq!(AppState::from_u8(2), AppState::Terminating);
        assert_eq!(AppState::from_u8(99), AppState::Active); // Invalid defaults to Active
    }

    #[test]
    fn test_ffi_get_app_state() {
        reset_state();
        assert_eq!(Platform_GetAppState(), 0);

        set_app_state(AppState::Background);
        assert_eq!(Platform_GetAppState(), 1);

        reset_state();
    }

    #[test]
    fn test_ffi_is_app_active() {
        reset_state();
        assert_eq!(Platform_IsAppActive(), 1);

        set_app_state(AppState::Background);
        assert_eq!(Platform_IsAppActive(), 0);

        reset_state();
    }

    #[test]
    fn test_ffi_quit_request() {
        reset_state();

        assert_eq!(Platform_ShouldQuit(), 0);

        Platform_RequestQuit();
        assert_eq!(Platform_ShouldQuit(), 1);

        Platform_ClearQuitRequest();
        assert_eq!(Platform_ShouldQuit(), 0);
    }

    #[test]
    fn test_handle_quit_event() {
        reset_state();

        let event = Event::Quit { timestamp: 0 };
        let result = handle_app_event(&event);

        assert!(result.is_none()); // Quit doesn't change app state directly
        assert!(should_quit()); // But it requests quit

        reset_state();
    }

    #[test]
    fn test_handle_background_event() {
        reset_state();

        let event = Event::AppDidEnterBackground { timestamp: 0 };
        let result = handle_app_event(&event);

        assert_eq!(result, Some(AppState::Background));
        assert_eq!(get_app_state(), AppState::Background);

        reset_state();
    }

    #[test]
    fn test_handle_foreground_event() {
        reset_state();
        set_app_state(AppState::Background);

        let event = Event::AppDidEnterForeground { timestamp: 0 };
        let result = handle_app_event(&event);

        assert_eq!(result, Some(AppState::Active));
        assert_eq!(get_app_state(), AppState::Active);

        reset_state();
    }

    #[test]
    fn test_handle_terminating_event() {
        reset_state();

        let event = Event::AppTerminating { timestamp: 0 };
        let result = handle_app_event(&event);

        assert_eq!(result, Some(AppState::Terminating));
        assert_eq!(get_app_state(), AppState::Terminating);
        assert!(should_quit()); // Terminating also requests quit

        reset_state();
    }

    #[test]
    fn test_handle_unrelated_event() {
        reset_state();

        // A keyboard event shouldn't affect app state
        let event = Event::KeyDown {
            timestamp: 0,
            window_id: 0,
            keycode: None,
            scancode: None,
            keymod: sdl2::keyboard::Mod::empty(),
            repeat: false,
        };
        let result = handle_app_event(&event);

        assert!(result.is_none());
        assert_eq!(get_app_state(), AppState::Active);
    }
}
```

### platform/src/lib.rs (MODIFY)
Add the app module:
```rust
pub mod app;
```

Add re-exports:
```rust
// Re-export app lifecycle FFI functions
pub use app::{
    Platform_GetAppState,
    Platform_IsAppActive,
    Platform_IsAppBackground,
    Platform_RequestQuit,
    Platform_ShouldQuit,
    Platform_ClearQuitRequest,
};
```

## Verification Command
```bash
ralph-tasks/verify/check-app-lifecycle.sh
```

## Verification Script
Create `ralph-tasks/verify/check-app-lifecycle.sh`:
```bash
#!/bin/bash
# Verification script for application lifecycle

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Application Lifecycle ==="

cd "$ROOT_DIR"

# Step 1: Check module files exist
echo "Step 1: Checking module files exist..."
if [ ! -f "platform/src/app/mod.rs" ]; then
    echo "VERIFY_FAILED: platform/src/app/mod.rs not found"
    exit 1
fi
if [ ! -f "platform/src/app/lifecycle.rs" ]; then
    echo "VERIFY_FAILED: platform/src/app/lifecycle.rs not found"
    exit 1
fi
echo "  App module files exist"

# Step 2: Check lib.rs has app module
echo "Step 2: Checking lib.rs includes app module..."
if ! grep -q "pub mod app" platform/src/lib.rs; then
    echo "VERIFY_FAILED: app module not declared in lib.rs"
    exit 1
fi
echo "  App module declared in lib.rs"

# Step 3: Build the platform crate
echo "Step 3: Building platform crate..."
if ! cargo build --manifest-path platform/Cargo.toml --release 2>&1; then
    echo "VERIFY_FAILED: cargo build failed"
    exit 1
fi
echo "  Build successful"

# Step 4: Run lifecycle tests
echo "Step 4: Running lifecycle tests..."
if ! cargo test --manifest-path platform/Cargo.toml --release -- lifecycle --test-threads=1 2>&1; then
    echo "VERIFY_FAILED: lifecycle tests failed"
    exit 1
fi
echo "  Tests passed"

# Step 5: Generate and check header
echo "Step 5: Checking FFI exports in header..."
if ! cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1; then
    echo "VERIFY_FAILED: cbindgen header generation failed"
    exit 1
fi

# Check for lifecycle FFI exports
for func in Platform_GetAppState Platform_IsAppActive Platform_IsAppBackground Platform_RequestQuit Platform_ShouldQuit Platform_ClearQuitRequest; do
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
1. Create directory `platform/src/app/`
2. Create `platform/src/app/mod.rs`
3. Create `platform/src/app/lifecycle.rs`
4. Add `pub mod app;` to `platform/src/lib.rs`
5. Add FFI re-exports to `platform/src/lib.rs`
6. Run `cargo build --manifest-path platform/Cargo.toml`
7. Run `cargo test --manifest-path platform/Cargo.toml -- lifecycle --test-threads=1`
8. Run verification script

## Success Criteria
- App state tracking works correctly
- SDL2 events update state appropriately
- Quit request mechanism works
- FFI functions appear in platform.h
- All unit tests pass
- Verification script exits with 0

## Common Issues

### "cannot find Event in sdl2"
Ensure `sdl2` is in Cargo.toml dependencies (it already should be from Phase 03).

### Tests fail with global state issues
Run tests with `--test-threads=1` to avoid race conditions.

### State not updating
Make sure `handle_app_event` is called for every SDL event in the event loop.

## C++ Usage Example
```cpp
#include "platform.h"

void main_loop() {
    while (!Platform_ShouldQuit()) {
        // Process events (this calls handle_app_event internally)
        Platform_ProcessEvents();

        // Check if we should pause
        if (Platform_IsAppBackground()) {
            // Reduce CPU usage when backgrounded
            sleep_ms(100);
            continue;
        }

        // Normal game loop
        update_game();
        render_game();
    }

    // Graceful shutdown
    save_game_state();
    Platform_Shutdown();
}
```

## Completion Promise
When verification passes, output:
```
<promise>TASK_11D_COMPLETE</promise>
```

## Escape Hatch
If stuck after 10 iterations:
- Document what's blocking in `ralph-tasks/blocked/11d.md`
- List attempted approaches
- Output: `<promise>TASK_11D_BLOCKED</promise>`

## Max Iterations
10
