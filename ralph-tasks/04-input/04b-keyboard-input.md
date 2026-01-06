# Task 04b: Keyboard Input

## Dependencies
- Task 04a must be complete (Input module setup)
- InputState and KeyCode enum in place

## Context
This task implements full keyboard handling: SDL keycode translation, key state tracking, event queue for buffered input, and modifier key detection. The game needs both immediate state queries ("is key down?") and buffered input ("get next key press").

## Objective
Implement complete keyboard input handling with SDL-to-VK translation, state tracking, event queue, and modifier support.

## Deliverables
- [ ] SDL keycode to KeyCode translation function
- [ ] Key state tracking (current/previous frame)
- [ ] Key event queue with push/pop
- [ ] Modifier key tracking (Shift, Ctrl, Alt)
- [ ] FFI exports for keyboard queries

## Files to Modify

### Update platform/src/input/mod.rs
Add keyboard handling functions:
```rust
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
                let vk = (code - 'a' as i32 + 0x41) as u8;
                return Some(unsafe { std::mem::transmute(vk) });
            }
            if code >= '0' as i32 && code <= '9' as i32 {
                // VK_0 to VK_9 (0x30-0x39)
                let vk = (code - '0' as i32 + 0x30) as u8;
                return Some(unsafe { std::mem::transmute(vk) });
            }
            None
        }
    }
}

impl InputState {
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
}

// Public accessor functions
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
```

### Update platform/src/graphics/mod.rs
Modify poll_events() to feed keyboard events to input system:
```rust
use crate::input;

/// Poll events and return true if quit was requested
pub fn poll_events() -> bool {
    with_sdl(|sdl| {
        let mut event_pump = match sdl.event_pump() {
            Ok(pump) => pump,
            Err(_) => return false,
        };

        for event in event_pump.poll_iter() {
            match event {
                sdl2::event::Event::Quit { .. } => {
                    input::request_quit();
                    return true;
                }
                sdl2::event::Event::KeyDown { keycode: Some(key), repeat, .. } => {
                    input::with_input(|state| state.handle_key_down(key, repeat));
                    if key == sdl2::keyboard::Keycode::Escape {
                        input::request_quit();
                        return true;
                    }
                }
                sdl2::event::Event::KeyUp { keycode: Some(key), .. } => {
                    input::with_input(|state| state.handle_key_up(key));
                }
                _ => {}
            }
        }
        input::should_quit()
    }).unwrap_or(false)
}
```

### Add FFI exports to platform/src/ffi/mod.rs
```rust
use crate::input::KeyCode;

/// Check if key is currently pressed
#[no_mangle]
pub extern "C" fn Platform_Key_IsPressed(key: KeyCode) -> bool {
    input::is_key_pressed(key)
}

/// Check if key was just pressed this frame
#[no_mangle]
pub extern "C" fn Platform_Key_WasPressed(key: KeyCode) -> bool {
    input::was_key_pressed(key)
}

/// Check if key was just released this frame
#[no_mangle]
pub extern "C" fn Platform_Key_WasReleased(key: KeyCode) -> bool {
    input::was_key_released(key)
}

/// Get next key event from queue (returns false if empty)
#[no_mangle]
pub extern "C" fn Platform_Key_GetNext(key: *mut KeyCode, released: *mut bool) -> bool {
    if key.is_null() || released.is_null() {
        return false;
    }

    input::with_input(|state| {
        if let Some(event) = state.pop_key_event() {
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

/// Clear key event queue
#[no_mangle]
pub extern "C" fn Platform_Key_Clear() {
    input::with_input(|state| state.clear_key_queue());
}

/// Check if Shift is held
#[no_mangle]
pub extern "C" fn Platform_Key_ShiftDown() -> bool {
    input::shift_down()
}

/// Check if Ctrl is held
#[no_mangle]
pub extern "C" fn Platform_Key_CtrlDown() -> bool {
    input::ctrl_down()
}

/// Check if Alt is held
#[no_mangle]
pub extern "C" fn Platform_Key_AltDown() -> bool {
    input::alt_down()
}
```

## Verification Command
```bash
cd /Users/ooartist/Src/Ronin_CnC && \
cd platform && cargo test 2>&1 && \
cargo build --features cbindgen-run 2>&1 && \
grep -q "Platform_Key_IsPressed" ../include/platform.h && \
grep -q "Platform_Key_GetNext" ../include/platform.h && \
grep -q "Platform_Key_ShiftDown" ../include/platform.h && \
echo "VERIFY_SUCCESS"
```

## Implementation Steps
1. Add sdl_to_keycode() translation function
2. Add keyboard handling methods to InputState
3. Update graphics::poll_events() to feed keyboard events
4. Add keyboard FFI exports
5. Run tests and build with cbindgen
6. Verify header contains keyboard functions

## Common Issues

### Keys not being detected
- Ensure poll_events() is being called each frame
- Check that sdl_to_keycode() handles the specific key

### Modifier keys stuck
- Ensure KeyUp events are also processed
- Check both left and right variants (LShift/RShift)

### Key queue grows unbounded
- Call Platform_Key_Clear() or process all events each frame
- Queue has initial capacity of 256

## Completion Promise
When verification passes, output:
<promise>TASK_04B_COMPLETE</promise>

## Escape Hatch
If stuck after 12 iterations:
- Document blocking issue in `ralph-tasks/blocked/04b.md`
- Output: <promise>TASK_04B_BLOCKED</promise>

## Max Iterations
12
