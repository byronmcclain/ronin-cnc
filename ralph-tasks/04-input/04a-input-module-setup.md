# Task 04a: Input Module Setup

## Dependencies
- Phase 03 must be complete (Graphics System)
- Event polling already exists in graphics module

## Context
This task sets up the input module structure and integrates it with the existing SDL event loop. We need to refactor the current poll_events() in graphics to feed events into the new input system while maintaining the quit detection functionality.

## Objective
Create the input module foundation with InputState structure and integrate event handling with the existing graphics event pump.

## Deliverables
- [ ] Create `platform/src/input/mod.rs` with InputState structure
- [ ] Define KeyCode and MouseButton enums (Windows VK_* compatible)
- [ ] Create global INPUT state with thread-safe access
- [ ] Integrate input event handling into graphics::poll_events()
- [ ] Export basic input FFI (init, shutdown, update)

## Files to Create/Modify

### platform/src/input/mod.rs (NEW)
```rust
//! Input subsystem using SDL2 events

use sdl2::event::Event;
use sdl2::keyboard::Keycode;
use std::collections::VecDeque;
use std::sync::Mutex;
use once_cell::sync::Lazy;

/// Key codes (matching Windows VK_* values for compatibility)
#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum KeyCode {
    None = 0,
    Backspace = 0x08,
    Tab = 0x09,
    Return = 0x0D,
    Shift = 0x10,
    Control = 0x11,
    Alt = 0x12,
    Pause = 0x13,
    CapsLock = 0x14,
    Escape = 0x1B,
    Space = 0x20,
    PageUp = 0x21,
    PageDown = 0x22,
    End = 0x23,
    Home = 0x24,
    Left = 0x25,
    Up = 0x26,
    Right = 0x27,
    Down = 0x28,
    Insert = 0x2D,
    Delete = 0x2E,
    // 0-9 = 0x30-0x39 (handled dynamically)
    // A-Z = 0x41-0x5A (handled dynamically)
    F1 = 0x70,
    F2 = 0x71,
    F3 = 0x72,
    F4 = 0x73,
    F5 = 0x74,
    F6 = 0x75,
    F7 = 0x76,
    F8 = 0x77,
    F9 = 0x78,
    F10 = 0x79,
    F11 = 0x7A,
    F12 = 0x7B,
    NumLock = 0x90,
    ScrollLock = 0x91,
}

/// Mouse button
#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum MouseButton {
    Left = 0,
    Right = 1,
    Middle = 2,
}

/// Key event for queue
#[derive(Clone, Copy, Debug)]
pub struct KeyEvent {
    pub key: KeyCode,
    pub released: bool,
}

/// Global input state
static INPUT: Lazy<Mutex<InputState>> = Lazy::new(|| Mutex::new(InputState::new()));

/// Input state structure
pub struct InputState {
    // Keyboard
    pub keys_current: [bool; 256],
    pub keys_previous: [bool; 256],
    pub key_queue: VecDeque<KeyEvent>,

    // Mouse
    pub mouse_x: i32,
    pub mouse_y: i32,
    pub mouse_buttons: [bool; 3],
    pub mouse_buttons_prev: [bool; 3],

    // Modifiers
    pub shift_down: bool,
    pub ctrl_down: bool,
    pub alt_down: bool,

    // Quit flag
    pub should_quit: bool,
}

impl InputState {
    pub fn new() -> Self {
        Self {
            keys_current: [false; 256],
            keys_previous: [false; 256],
            key_queue: VecDeque::with_capacity(256),
            mouse_x: 0,
            mouse_y: 0,
            mouse_buttons: [false; 3],
            mouse_buttons_prev: [false; 3],
            shift_down: false,
            ctrl_down: false,
            alt_down: false,
            should_quit: false,
        }
    }

    /// Called at start of each frame to update previous state
    pub fn begin_frame(&mut self) {
        self.keys_previous = self.keys_current;
        self.mouse_buttons_prev = self.mouse_buttons;
    }
}

impl Default for InputState {
    fn default() -> Self {
        Self::new()
    }
}

/// Access input state with closure
pub fn with_input<F, R>(f: F) -> Option<R>
where
    F: FnOnce(&mut InputState) -> R,
{
    INPUT.lock().ok().map(|mut guard| f(&mut guard))
}

/// Check if quit was requested
pub fn should_quit() -> bool {
    INPUT.lock().map(|s| s.should_quit).unwrap_or(false)
}

/// Set quit flag
pub fn request_quit() {
    if let Ok(mut state) = INPUT.lock() {
        state.should_quit = true;
    }
}

/// Begin frame - call at start of each frame
pub fn begin_frame() {
    with_input(|state| state.begin_frame());
}
```

### Update platform/src/lib.rs
Add after `pub mod graphics;`:
```rust
pub mod input;
```

### Update platform/src/ffi/mod.rs
Add input FFI functions after Event Handling section:
```rust
use crate::input;

// =============================================================================
// Input System
// =============================================================================

/// Initialize input subsystem
#[no_mangle]
pub extern "C" fn Platform_Input_Init() -> i32 {
    // Input is auto-initialized via Lazy static
    0
}

/// Shutdown input subsystem
#[no_mangle]
pub extern "C" fn Platform_Input_Shutdown() {
    // Nothing to clean up
}

/// Update input state (call at start of each frame)
#[no_mangle]
pub extern "C" fn Platform_Input_Update() {
    input::begin_frame();
}

/// Check if quit was requested
#[no_mangle]
pub extern "C" fn Platform_Input_ShouldQuit() -> bool {
    input::should_quit()
}
```

## Verification Command
```bash
cd /Users/ooartist/Src/Ronin_CnC && \
cd platform && cargo build --features cbindgen-run 2>&1 && \
grep -q "KeyCode" ../include/platform.h && \
grep -q "MouseButton" ../include/platform.h && \
grep -q "Platform_Input_Init" ../include/platform.h && \
echo "VERIFY_SUCCESS"
```

## Implementation Steps
1. Create `platform/src/input/mod.rs` with InputState and enums
2. Add `pub mod input;` to lib.rs
3. Add input FFI exports to ffi/mod.rs
4. Run `cargo build --features cbindgen-run`
5. Verify header contains KeyCode, MouseButton, and input functions
6. If any step fails, analyze error and fix

## Expected Header Additions
```c
typedef enum KeyCode {
    KEY_CODE_NONE = 0,
    KEY_CODE_BACKSPACE = 8,
    KEY_CODE_TAB = 9,
    // ... more keys
} KeyCode;

typedef enum MouseButton {
    MOUSE_BUTTON_LEFT = 0,
    MOUSE_BUTTON_RIGHT = 1,
    MOUSE_BUTTON_MIDDLE = 2,
} MouseButton;

int32_t Platform_Input_Init(void);
void Platform_Input_Shutdown(void);
void Platform_Input_Update(void);
bool Platform_Input_ShouldQuit(void);
```

## Common Issues

### "cannot find input module"
- Ensure `pub mod input;` is added to lib.rs

### KeyCode enum values don't match Windows VK_*
- Double-check hex values against Windows virtual key codes
- VK_ESCAPE = 0x1B, VK_SPACE = 0x20, etc.

## Completion Promise
When verification passes, output:
<promise>TASK_04A_COMPLETE</promise>

## Escape Hatch
If stuck after 10 iterations:
- Document blocking issue in `ralph-tasks/blocked/04a.md`
- Output: <promise>TASK_04A_BLOCKED</promise>

## Max Iterations
10
