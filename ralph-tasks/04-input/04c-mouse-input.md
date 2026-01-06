# Task 04c: Mouse Input

## Dependencies
- Task 04b must be complete (Keyboard input)
- Input module and event handling in place

## Context
This task implements mouse handling: position tracking, button state, click detection, and double-click detection. The game uses mouse for unit selection, commands, and menu interaction. Double-click detection requires timing and position thresholds.

## Objective
Implement complete mouse input handling with position, buttons, single-click, double-click, and wheel support.

## Deliverables
- [ ] Mouse position tracking
- [ ] Mouse button state (current/previous)
- [ ] Single-click detection
- [ ] Double-click detection with timing/distance thresholds
- [ ] Mouse wheel delta
- [ ] FFI exports for mouse queries

## Files to Modify

### Update platform/src/input/mod.rs
Add mouse handling to InputState:
```rust
const DOUBLE_CLICK_TIME: u32 = 500;  // ms
const DOUBLE_CLICK_DISTANCE: i32 = 4; // pixels

impl InputState {
    // Update the struct to add these fields (if not already present):
    // pub mouse_clicked: [bool; 3],
    // pub mouse_double_clicked: [bool; 3],
    // pub mouse_wheel: i32,
    // pub last_click_time: [u32; 3],
    // pub last_click_x: [i32; 3],
    // pub last_click_y: [i32; 3],

    /// Called at start of each frame - update to clear mouse click states
    pub fn begin_frame(&mut self) {
        self.keys_previous = self.keys_current;
        self.mouse_buttons_prev = self.mouse_buttons;
        self.mouse_clicked = [false; 3];
        self.mouse_double_clicked = [false; 3];
        self.mouse_wheel = 0;
    }

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

/// Convert SDL mouse button to our MouseButton
pub fn sdl_to_mousebutton(btn: sdl2::mouse::MouseButton) -> MouseButton {
    match btn {
        sdl2::mouse::MouseButton::Left => MouseButton::Left,
        sdl2::mouse::MouseButton::Right => MouseButton::Right,
        sdl2::mouse::MouseButton::Middle => MouseButton::Middle,
        _ => MouseButton::Left,
    }
}

// Public accessor functions
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
```

### Update InputState::new() to initialize new fields
```rust
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
}
```

### Update platform/src/graphics/mod.rs poll_events()
Add mouse event handling:
```rust
pub fn poll_events() -> bool {
    let ticks = get_ticks();

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
                sdl2::event::Event::MouseMotion { x, y, .. } => {
                    input::with_input(|state| state.handle_mouse_motion(x, y));
                }
                sdl2::event::Event::MouseButtonDown { mouse_btn, x, y, .. } => {
                    let btn = input::sdl_to_mousebutton(mouse_btn);
                    input::with_input(|state| state.handle_mouse_down(btn, x, y, ticks));
                }
                sdl2::event::Event::MouseButtonUp { mouse_btn, .. } => {
                    let btn = input::sdl_to_mousebutton(mouse_btn);
                    input::with_input(|state| state.handle_mouse_up(btn));
                }
                sdl2::event::Event::MouseWheel { y, .. } => {
                    input::with_input(|state| state.handle_mouse_wheel(y));
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
use crate::input::MouseButton;

/// Get mouse position
#[no_mangle]
pub extern "C" fn Platform_Mouse_GetPosition(x: *mut i32, y: *mut i32) {
    let (mx, my) = input::get_mouse_position();
    if !x.is_null() {
        unsafe { *x = mx; }
    }
    if !y.is_null() {
        unsafe { *y = my; }
    }
}

/// Get mouse X position
#[no_mangle]
pub extern "C" fn Platform_Mouse_GetX() -> i32 {
    input::get_mouse_position().0
}

/// Get mouse Y position
#[no_mangle]
pub extern "C" fn Platform_Mouse_GetY() -> i32 {
    input::get_mouse_position().1
}

/// Check if mouse button is pressed
#[no_mangle]
pub extern "C" fn Platform_Mouse_IsPressed(button: MouseButton) -> bool {
    input::is_mouse_pressed(button)
}

/// Check if mouse button was clicked this frame
#[no_mangle]
pub extern "C" fn Platform_Mouse_WasClicked(button: MouseButton) -> bool {
    input::was_mouse_clicked(button)
}

/// Check if mouse button was double-clicked this frame
#[no_mangle]
pub extern "C" fn Platform_Mouse_WasDoubleClicked(button: MouseButton) -> bool {
    input::was_mouse_double_clicked(button)
}

/// Get mouse wheel delta
#[no_mangle]
pub extern "C" fn Platform_Mouse_GetWheelDelta() -> i32 {
    input::get_mouse_wheel()
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
```

## Verification Command
```bash
cd /Users/ooartist/Src/Ronin_CnC && \
cd platform && cargo build --features cbindgen-run 2>&1 && \
grep -q "Platform_Mouse_GetPosition" ../include/platform.h && \
grep -q "Platform_Mouse_IsPressed" ../include/platform.h && \
grep -q "Platform_Mouse_WasDoubleClicked" ../include/platform.h && \
echo "VERIFY_SUCCESS"
```

## Implementation Steps
1. Add mouse fields to InputState struct
2. Add mouse handling methods
3. Update begin_frame() to clear click states
4. Update graphics::poll_events() with mouse events
5. Add mouse FFI exports
6. Build with cbindgen and verify header

## Common Issues

### Mouse position wrong scale
- Position is in window coordinates, may need scaling to game coordinates
- The window is 2x scale, so divide by 2 for game coords

### Double-click not detecting
- Check DOUBLE_CLICK_TIME threshold (500ms default)
- Check DOUBLE_CLICK_DISTANCE threshold (4 pixels default)
- Ensure ticks are being passed correctly

### Mouse buttons stuck
- Ensure MouseButtonUp events are processed
- Check button index bounds (0-2)

## Completion Promise
When verification passes, output:
<promise>TASK_04C_COMPLETE</promise>

## Escape Hatch
If stuck after 12 iterations:
- Document blocking issue in `ralph-tasks/blocked/04c.md`
- Output: <promise>TASK_04C_BLOCKED</promise>

## Max Iterations
12
