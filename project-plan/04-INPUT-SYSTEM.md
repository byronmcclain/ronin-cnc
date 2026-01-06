# Plan 04: Input System (Rust + sdl2-rs)

## Objective
Replace Windows input handling (keyboard, mouse) with Rust's sdl2 crate event-based input, maintaining compatibility with the existing game input code via FFI.

## Current State Analysis

### Windows Input in Codebase

**Primary Files:**
- `WIN32LIB/KEYBOARD/KEYBOARD.CPP` (534 lines)
- `WIN32LIB/KEYBOARD/KEYBOARD.H`
- `WIN32LIB/KEYBOARD/MOUSE.CPP`
- `CODE/KEYBOARD.CPP`
- `CODE/KEY.CPP`

**Key Classes:**
```cpp
class WWKeyboardClass {
    unsigned short Buffer[256];  // Ring buffer for key events
    int Head, Tail;
    int DownModifiers;           // Shift, Ctrl, Alt state
};
```

## Rust Implementation

### 4.1 Input State Structure

File: `platform/src/input/mod.rs`
```rust
//! Input subsystem using sdl2 events

pub mod keyboard;
pub mod mouse;

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
    // 0-9 = 0x30-0x39
    // A-Z = 0x41-0x5A
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
#[derive(Clone, Copy, Debug)]
pub enum MouseButton {
    Left = 0,
    Right = 1,
    Middle = 2,
}

/// Key event for queue
#[derive(Clone, Copy)]
pub struct KeyEvent {
    pub key: KeyCode,
    pub released: bool,
}

/// Global input state
static INPUT: Lazy<Mutex<InputState>> = Lazy::new(|| Mutex::new(InputState::new()));

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
    pub mouse_clicked: [bool; 3],
    pub mouse_double_clicked: [bool; 3],
    pub mouse_wheel: i32,
    pub last_click_time: [u32; 3],
    pub last_click_x: [i32; 3],
    pub last_click_y: [i32; 3],

    // Modifiers
    pub shift_down: bool,
    pub ctrl_down: bool,
    pub alt_down: bool,
    pub caps_lock: bool,
    pub num_lock: bool,

    // Quit flag
    pub should_quit: bool,
}

const DOUBLE_CLICK_TIME: u32 = 500;  // ms
const DOUBLE_CLICK_DISTANCE: i32 = 4; // pixels

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
            mouse_clicked: [false; 3],
            mouse_double_clicked: [false; 3],
            mouse_wheel: 0,
            last_click_time: [0; 3],
            last_click_x: [0; 3],
            last_click_y: [0; 3],
            shift_down: false,
            ctrl_down: false,
            alt_down: false,
            caps_lock: false,
            num_lock: false,
            should_quit: false,
        }
    }

    /// Called at start of each frame
    pub fn begin_frame(&mut self) {
        self.keys_previous = self.keys_current;
        self.mouse_buttons_prev = self.mouse_buttons;
        self.mouse_clicked = [false; 3];
        self.mouse_double_clicked = [false; 3];
        self.mouse_wheel = 0;
    }

    /// Handle SDL event
    pub fn handle_event(&mut self, event: &Event, ticks: u32) {
        match event {
            Event::Quit { .. } => {
                self.should_quit = true;
            }

            Event::KeyDown { keycode: Some(key), repeat, .. } => {
                if !repeat {
                    if let Some(code) = sdl_to_keycode(*key) {
                        let idx = code as usize;
                        if idx < 256 {
                            self.keys_current[idx] = true;
                            self.key_queue.push_back(KeyEvent { key: code, released: false });
                        }

                        // Track modifiers
                        match code {
                            KeyCode::Shift => self.shift_down = true,
                            KeyCode::Control => self.ctrl_down = true,
                            KeyCode::Alt => self.alt_down = true,
                            _ => {}
                        }
                    }
                }
            }

            Event::KeyUp { keycode: Some(key), .. } => {
                if let Some(code) = sdl_to_keycode(*key) {
                    let idx = code as usize;
                    if idx < 256 {
                        self.keys_current[idx] = false;
                        self.key_queue.push_back(KeyEvent { key: code, released: true });
                    }

                    // Track modifiers
                    match code {
                        KeyCode::Shift => self.shift_down = false,
                        KeyCode::Control => self.ctrl_down = false,
                        KeyCode::Alt => self.alt_down = false,
                        _ => {}
                    }
                }
            }

            Event::MouseMotion { x, y, .. } => {
                self.mouse_x = *x;
                self.mouse_y = *y;
            }

            Event::MouseButtonDown { mouse_btn, x, y, .. } => {
                let btn = sdl_to_mousebutton(*mouse_btn) as usize;
                self.mouse_buttons[btn] = true;
                self.mouse_clicked[btn] = true;

                // Double-click detection
                let dx = (x - self.last_click_x[btn]).abs();
                let dy = (y - self.last_click_y[btn]).abs();
                let dt = ticks.saturating_sub(self.last_click_time[btn]);

                if dt < DOUBLE_CLICK_TIME && dx < DOUBLE_CLICK_DISTANCE && dy < DOUBLE_CLICK_DISTANCE {
                    self.mouse_double_clicked[btn] = true;
                }

                self.last_click_time[btn] = ticks;
                self.last_click_x[btn] = *x;
                self.last_click_y[btn] = *y;
            }

            Event::MouseButtonUp { mouse_btn, .. } => {
                let btn = sdl_to_mousebutton(*mouse_btn) as usize;
                self.mouse_buttons[btn] = false;
            }

            Event::MouseWheel { y, .. } => {
                self.mouse_wheel = *y;
            }

            _ => {}
        }
    }
}

/// Convert SDL keycode to our KeyCode
fn sdl_to_keycode(key: Keycode) -> Option<KeyCode> {
    match key {
        Keycode::Backspace => Some(KeyCode::Backspace),
        Keycode::Tab => Some(KeyCode::Tab),
        Keycode::Return => Some(KeyCode::Return),
        Keycode::Escape => Some(KeyCode::Escape),
        Keycode::Space => Some(KeyCode::Space),
        Keycode::Up => Some(KeyCode::Up),
        Keycode::Down => Some(KeyCode::Down),
        Keycode::Left => Some(KeyCode::Left),
        Keycode::Right => Some(KeyCode::Right),
        Keycode::LShift | Keycode::RShift => Some(KeyCode::Shift),
        Keycode::LCtrl | Keycode::RCtrl => Some(KeyCode::Control),
        Keycode::LAlt | Keycode::RAlt => Some(KeyCode::Alt),
        Keycode::PageUp => Some(KeyCode::PageUp),
        Keycode::PageDown => Some(KeyCode::PageDown),
        Keycode::Home => Some(KeyCode::Home),
        Keycode::End => Some(KeyCode::End),
        Keycode::Insert => Some(KeyCode::Insert),
        Keycode::Delete => Some(KeyCode::Delete),
        Keycode::F1 => Some(KeyCode::F1),
        Keycode::F2 => Some(KeyCode::F2),
        Keycode::F3 => Some(KeyCode::F3),
        Keycode::F4 => Some(KeyCode::F4),
        Keycode::F5 => Some(KeyCode::F5),
        Keycode::F6 => Some(KeyCode::F6),
        Keycode::F7 => Some(KeyCode::F7),
        Keycode::F8 => Some(KeyCode::F8),
        Keycode::F9 => Some(KeyCode::F9),
        Keycode::F10 => Some(KeyCode::F10),
        Keycode::F11 => Some(KeyCode::F11),
        Keycode::F12 => Some(KeyCode::F12),
        _ => {
            // Handle alphanumeric keys
            let code = key as i32;
            if code >= 'a' as i32 && code <= 'z' as i32 {
                // Convert to uppercase VK code (0x41-0x5A)
                return Some(unsafe { std::mem::transmute((code - 'a' as i32 + 0x41) as u8) });
            }
            if code >= '0' as i32 && code <= '9' as i32 {
                // VK_0 to VK_9 (0x30-0x39)
                return Some(unsafe { std::mem::transmute((code - '0' as i32 + 0x30) as u8) });
            }
            None
        }
    }
}

/// Convert SDL mouse button to our MouseButton
fn sdl_to_mousebutton(btn: sdl2::mouse::MouseButton) -> MouseButton {
    match btn {
        sdl2::mouse::MouseButton::Left => MouseButton::Left,
        sdl2::mouse::MouseButton::Right => MouseButton::Right,
        sdl2::mouse::MouseButton::Middle => MouseButton::Middle,
        _ => MouseButton::Left,
    }
}

/// Access input state
pub fn with_input<F, R>(f: F) -> Option<R>
where
    F: FnOnce(&mut InputState) -> R,
{
    INPUT.lock().ok().map(|mut guard| f(&mut guard))
}

/// Check if quit requested
pub fn should_quit() -> bool {
    INPUT.lock().map(|s| s.should_quit).unwrap_or(false)
}

/// Pump events (called from main event loop)
pub fn pump_events() {
    // This is a no-op - actual pumping happens in graphics module
}
```

### 4.2 FFI Input Exports

File: `platform/src/ffi/input.rs`
```rust
//! Input FFI exports

use crate::input::{self, KeyCode, MouseButton};

// =============================================================================
// Keyboard
// =============================================================================

/// Initialize input subsystem
#[no_mangle]
pub extern "C" fn Platform_Input_Init() -> i32 {
    // Input is initialized as part of platform init
    0
}

/// Shutdown input subsystem
#[no_mangle]
pub extern "C" fn Platform_Input_Shutdown() {
    // Nothing to do
}

/// Update input state (called each frame)
#[no_mangle]
pub extern "C" fn Platform_Input_Update() {
    input::with_input(|state| {
        state.begin_frame();
    });
}

/// Check if key is currently pressed
#[no_mangle]
pub extern "C" fn Platform_Key_IsPressed(key: KeyCode) -> bool {
    input::with_input(|state| {
        let idx = key as usize;
        if idx < 256 { state.keys_current[idx] } else { false }
    }).unwrap_or(false)
}

/// Check if key was just pressed this frame
#[no_mangle]
pub extern "C" fn Platform_Key_WasPressed(key: KeyCode) -> bool {
    input::with_input(|state| {
        let idx = key as usize;
        if idx < 256 {
            state.keys_current[idx] && !state.keys_previous[idx]
        } else {
            false
        }
    }).unwrap_or(false)
}

/// Check if key was just released this frame
#[no_mangle]
pub extern "C" fn Platform_Key_WasReleased(key: KeyCode) -> bool {
    input::with_input(|state| {
        let idx = key as usize;
        if idx < 256 {
            !state.keys_current[idx] && state.keys_previous[idx]
        } else {
            false
        }
    }).unwrap_or(false)
}

/// Get next key event from queue
#[no_mangle]
pub extern "C" fn Platform_Key_GetNext(key: *mut KeyCode, released: *mut bool) -> bool {
    if key.is_null() || released.is_null() {
        return false;
    }

    input::with_input(|state| {
        if let Some(event) = state.key_queue.pop_front() {
            unsafe {
                *key = event.key;
                *released = event.released;
            }
            true
        } else {
            false
        }
    }).unwrap_or(false)
}

/// Clear key queue
#[no_mangle]
pub extern "C" fn Platform_Key_Clear() {
    input::with_input(|state| {
        state.key_queue.clear();
    });
}

/// Check if Shift is held
#[no_mangle]
pub extern "C" fn Platform_Key_ShiftDown() -> bool {
    input::with_input(|state| state.shift_down).unwrap_or(false)
}

/// Check if Ctrl is held
#[no_mangle]
pub extern "C" fn Platform_Key_CtrlDown() -> bool {
    input::with_input(|state| state.ctrl_down).unwrap_or(false)
}

/// Check if Alt is held
#[no_mangle]
pub extern "C" fn Platform_Key_AltDown() -> bool {
    input::with_input(|state| state.alt_down).unwrap_or(false)
}

/// Check Caps Lock state
#[no_mangle]
pub extern "C" fn Platform_Key_CapsLock() -> bool {
    input::with_input(|state| state.caps_lock).unwrap_or(false)
}

/// Check Num Lock state
#[no_mangle]
pub extern "C" fn Platform_Key_NumLock() -> bool {
    input::with_input(|state| state.num_lock).unwrap_or(false)
}

// =============================================================================
// Mouse
// =============================================================================

/// Get mouse position
#[no_mangle]
pub extern "C" fn Platform_Mouse_GetPosition(x: *mut i32, y: *mut i32) {
    input::with_input(|state| {
        if !x.is_null() {
            unsafe { *x = state.mouse_x; }
        }
        if !y.is_null() {
            unsafe { *y = state.mouse_y; }
        }
    });
}

/// Set mouse position
#[no_mangle]
pub extern "C" fn Platform_Mouse_SetPosition(x: i32, y: i32) {
    // Would need access to SDL window to warp mouse
    input::with_input(|state| {
        state.mouse_x = x;
        state.mouse_y = y;
    });
}

/// Check if mouse button is pressed
#[no_mangle]
pub extern "C" fn Platform_Mouse_IsPressed(button: MouseButton) -> bool {
    input::with_input(|state| {
        state.mouse_buttons[button as usize]
    }).unwrap_or(false)
}

/// Check if mouse button was clicked this frame
#[no_mangle]
pub extern "C" fn Platform_Mouse_WasClicked(button: MouseButton) -> bool {
    input::with_input(|state| {
        state.mouse_clicked[button as usize]
    }).unwrap_or(false)
}

/// Check if mouse button was double-clicked this frame
#[no_mangle]
pub extern "C" fn Platform_Mouse_WasDoubleClicked(button: MouseButton) -> bool {
    input::with_input(|state| {
        state.mouse_double_clicked[button as usize]
    }).unwrap_or(false)
}

/// Get mouse wheel delta
#[no_mangle]
pub extern "C" fn Platform_Mouse_GetWheelDelta() -> i32 {
    input::with_input(|state| state.mouse_wheel).unwrap_or(0)
}

/// Show mouse cursor
#[no_mangle]
pub extern "C" fn Platform_Mouse_Show() {
    sdl2::mouse::show_cursor(true);
}

/// Hide mouse cursor
#[no_mangle]
pub extern "C" fn Platform_Mouse_Hide() {
    sdl2::mouse::show_cursor(false);
}

/// Confine mouse to window
#[no_mangle]
pub extern "C" fn Platform_Mouse_Confine(_confine: bool) {
    // Would need access to SDL window
}
```

## Tasks Breakdown

### Phase 1: Basic Keyboard (2 days)
- [ ] Implement complete key code mapping
- [ ] Implement key state tracking
- [ ] Implement key event queue
- [ ] Test with key echo

### Phase 2: Mouse Input (1 day)
- [ ] Implement mouse position tracking
- [ ] Implement button state tracking
- [ ] Implement double-click detection
- [ ] Test with cursor display

### Phase 3: FFI Exports (1 day)
- [ ] Create all FFI wrapper functions
- [ ] Verify cbindgen output
- [ ] Test calling from C++

### Phase 4: Integration (1 day)
- [ ] Hook into main event loop
- [ ] Test modifier keys
- [ ] Verify game menu navigation

## Acceptance Criteria

- [ ] All keyboard keys mapped correctly
- [ ] Mouse position accurate
- [ ] Double-click detection works
- [ ] Modifier keys (Shift/Ctrl/Alt) work
- [ ] Game menus navigable
- [ ] No panics cross FFI boundary

## Estimated Duration
**4-5 days**

## Dependencies
- Plan 03 (Graphics) for SDL event pump
- sdl2 crate

## Next Plan
Once input is working, proceed to **Plan 05: Timing System**
