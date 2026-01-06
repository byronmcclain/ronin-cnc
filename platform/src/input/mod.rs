//! Input subsystem using SDL2 events

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

/// Double-click detection constants
const DOUBLE_CLICK_TIME: u32 = 500;  // ms
const DOUBLE_CLICK_DISTANCE: i32 = 4; // pixels

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
            mouse_clicked: [false; 3],
            mouse_double_clicked: [false; 3],
            mouse_wheel: 0,
            last_click_time: [0; 3],
            last_click_x: [0; 3],
            last_click_y: [0; 3],
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
        self.mouse_clicked = [false; 3];
        self.mouse_double_clicked = [false; 3];
        self.mouse_wheel = 0;
    }

    /// Handle keyboard down event
    pub fn handle_key_down(&mut self, key: Keycode, repeat: bool) {
        if repeat {
            return; // Ignore key repeats
        }

        if let Some(code) = sdl_to_keycode(key) {
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

    /// Handle keyboard up event
    pub fn handle_key_up(&mut self, key: Keycode) {
        if let Some(code) = sdl_to_keycode(key) {
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

    /// Check if key is currently pressed
    pub fn is_key_pressed(&self, key: KeyCode) -> bool {
        let idx = key as usize;
        if idx < 256 { self.keys_current[idx] } else { false }
    }

    /// Check if key was just pressed this frame
    pub fn was_key_pressed(&self, key: KeyCode) -> bool {
        let idx = key as usize;
        if idx < 256 {
            self.keys_current[idx] && !self.keys_previous[idx]
        } else {
            false
        }
    }

    /// Check if key was just released this frame
    pub fn was_key_released(&self, key: KeyCode) -> bool {
        let idx = key as usize;
        if idx < 256 {
            !self.keys_current[idx] && self.keys_previous[idx]
        } else {
            false
        }
    }

    /// Get next key event from queue
    pub fn pop_key_event(&mut self) -> Option<KeyEvent> {
        self.key_queue.pop_front()
    }

    /// Clear key queue
    pub fn clear_key_queue(&mut self) {
        self.key_queue.clear();
    }

    // Mouse handling methods

    /// Handle mouse motion event
    pub fn handle_mouse_motion(&mut self, x: i32, y: i32) {
        self.mouse_x = x;
        self.mouse_y = y;
    }

    /// Handle mouse button down event
    pub fn handle_mouse_down(&mut self, button: MouseButton, x: i32, y: i32, ticks: u32) {
        let btn = button as usize;
        if btn >= 3 { return; }

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
        self.last_click_x[btn] = x;
        self.last_click_y[btn] = y;
    }

    /// Handle mouse button up event
    pub fn handle_mouse_up(&mut self, button: MouseButton) {
        let btn = button as usize;
        if btn >= 3 { return; }
        self.mouse_buttons[btn] = false;
    }

    /// Handle mouse wheel event
    pub fn handle_mouse_wheel(&mut self, y: i32) {
        self.mouse_wheel = y;
    }

    /// Check if mouse button is pressed
    pub fn is_mouse_pressed(&self, button: MouseButton) -> bool {
        let btn = button as usize;
        if btn < 3 { self.mouse_buttons[btn] } else { false }
    }

    /// Check if mouse button was clicked this frame
    pub fn was_mouse_clicked(&self, button: MouseButton) -> bool {
        let btn = button as usize;
        if btn < 3 { self.mouse_clicked[btn] } else { false }
    }

    /// Check if mouse button was double-clicked this frame
    pub fn was_mouse_double_clicked(&self, button: MouseButton) -> bool {
        let btn = button as usize;
        if btn < 3 { self.mouse_double_clicked[btn] } else { false }
    }
}

impl Default for InputState {
    fn default() -> Self {
        Self::new()
    }
}

/// Convert SDL keycode to our KeyCode
pub fn sdl_to_keycode(key: Keycode) -> Option<KeyCode> {
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
        Keycode::Pause => Some(KeyCode::Pause),
        Keycode::CapsLock => Some(KeyCode::CapsLock),
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
        Keycode::NumLockClear => Some(KeyCode::NumLock),
        Keycode::ScrollLock => Some(KeyCode::ScrollLock),
        _ => {
            // Handle alphanumeric keys
            let code = key as i32;
            if code >= 'a' as i32 && code <= 'z' as i32 {
                // Convert to uppercase VK code (0x41-0x5A)
                let vk = (code - 'a' as i32 + 0x41) as i32;
                return Some(unsafe { std::mem::transmute(vk) });
            }
            if code >= '0' as i32 && code <= '9' as i32 {
                // VK_0 to VK_9 (0x30-0x39)
                let vk = (code - '0' as i32 + 0x30) as i32;
                return Some(unsafe { std::mem::transmute(vk) });
            }
            None
        }
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

// Public accessor functions for keyboard
pub fn is_key_pressed(key: KeyCode) -> bool {
    with_input(|s| s.is_key_pressed(key)).unwrap_or(false)
}

pub fn was_key_pressed(key: KeyCode) -> bool {
    with_input(|s| s.was_key_pressed(key)).unwrap_or(false)
}

pub fn was_key_released(key: KeyCode) -> bool {
    with_input(|s| s.was_key_released(key)).unwrap_or(false)
}

pub fn shift_down() -> bool {
    with_input(|s| s.shift_down).unwrap_or(false)
}

pub fn ctrl_down() -> bool {
    with_input(|s| s.ctrl_down).unwrap_or(false)
}

pub fn alt_down() -> bool {
    with_input(|s| s.alt_down).unwrap_or(false)
}

/// Convert SDL mouse button to our MouseButton
pub fn sdl_to_mousebutton(btn: sdl2::mouse::MouseButton) -> MouseButton {
    match btn {
        sdl2::mouse::MouseButton::Left => MouseButton::Left,
        sdl2::mouse::MouseButton::Right => MouseButton::Right,
        sdl2::mouse::MouseButton::Middle => MouseButton::Middle,
        _ => MouseButton::Left,
    }
}

// Public accessor functions for mouse
pub fn get_mouse_position() -> (i32, i32) {
    with_input(|s| (s.mouse_x, s.mouse_y)).unwrap_or((0, 0))
}

pub fn is_mouse_pressed(button: MouseButton) -> bool {
    with_input(|s| s.is_mouse_pressed(button)).unwrap_or(false)
}

pub fn was_mouse_clicked(button: MouseButton) -> bool {
    with_input(|s| s.was_mouse_clicked(button)).unwrap_or(false)
}

pub fn was_mouse_double_clicked(button: MouseButton) -> bool {
    with_input(|s| s.was_mouse_double_clicked(button)).unwrap_or(false)
}

pub fn get_mouse_wheel() -> i32 {
    with_input(|s| s.mouse_wheel).unwrap_or(0)
}
