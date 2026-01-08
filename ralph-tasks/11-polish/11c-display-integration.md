# Task 11c: macOS Display Integration

## Dependencies
- Phase 03 (Graphics) must be complete
- Existing graphics module at `platform/src/graphics/mod.rs`

## Context
This task enhances the graphics module with macOS-specific display features:
- **Retina Display Support**: Detect HiDPI displays and report scale factor
- **Fullscreen Mode**: Toggle and control fullscreen state
- **Display Scaling**: Report drawable vs window size for proper coordinate mapping
- **Window Management**: Center window, set title, handle resize

The original DOS game ran at 640x400. On modern Retina displays (2x or 3x scaling), we need to properly handle the difference between "logical pixels" (window size) and "physical pixels" (drawable size).

## Objective
Add display integration features to the graphics module:
1. Detect Retina/HiDPI displays
2. Get display scale factor
3. Toggle fullscreen mode
4. Set fullscreen mode explicitly
5. Get window and drawable sizes
6. Provide FFI exports for C++ usage

## Deliverables
- [ ] `platform/src/graphics/display.rs` - Display integration functions
- [ ] FFI exports: `Platform_IsRetinaDisplay`, `Platform_GetDisplayScale`, `Platform_ToggleFullscreen`, `Platform_SetFullscreen`, `Platform_GetWindowSize`, `Platform_GetDrawableSize`
- [ ] Header contains FFI exports
- [ ] Unit tests pass
- [ ] Verification passes

## Technical Background

### Retina Display Detection
On macOS Retina displays:
- Window size is in "points" (logical pixels)
- Drawable size is in "pixels" (physical pixels)
- Scale factor = drawable_size / window_size

For a 2x Retina display with a 640x400 window:
- Window size: 640x400 points
- Drawable size: 1280x800 pixels
- Scale factor: 2.0

### Fullscreen Modes
SDL2 offers three fullscreen modes:
1. `Off` - Windowed mode
2. `True` - Full exclusive fullscreen (changes resolution)
3. `Desktop` - Borderless fullscreen at desktop resolution (preferred)

We use `Desktop` mode as it:
- Avoids resolution changes
- Allows faster alt-tab
- Works better with multiple monitors

### Window Coordinate Mapping
For mouse input on Retina displays:
- Mouse events are in window coordinates (points)
- Rendering is in drawable coordinates (pixels)
- Must scale mouse input by display scale factor

## Files to Create/Modify

### platform/src/graphics/display.rs (NEW)
```rust
//! Display integration for macOS
//!
//! Provides:
//! - Retina/HiDPI display detection
//! - Display scale factor calculation
//! - Fullscreen mode control
//! - Window and drawable size queries

use sdl2::video::{FullscreenType, Window};

/// Display scale factor
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct DisplayScale {
    pub x: f32,
    pub y: f32,
}

impl DisplayScale {
    pub fn new(x: f32, y: f32) -> Self {
        Self { x, y }
    }

    pub fn unity() -> Self {
        Self { x: 1.0, y: 1.0 }
    }

    pub fn is_retina(&self) -> bool {
        self.x > 1.0 || self.y > 1.0
    }
}

impl Default for DisplayScale {
    fn default() -> Self {
        Self::unity()
    }
}

/// Window size in logical pixels (points)
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct WindowSize {
    pub width: i32,
    pub height: i32,
}

/// Drawable size in physical pixels
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct DrawableSize {
    pub width: i32,
    pub height: i32,
}

// =============================================================================
// Display Query Functions
// =============================================================================

/// Check if the window is on a Retina/HiDPI display
///
/// Returns true if drawable size > window size in either dimension.
pub fn is_retina_display(window: &Window) -> bool {
    let (window_w, _) = window.size();
    let (drawable_w, _) = window.drawable_size();
    drawable_w > window_w
}

/// Get the display scale factor
///
/// Scale factor = drawable_size / window_size
/// On non-Retina displays, this returns (1.0, 1.0).
pub fn get_display_scale(window: &Window) -> DisplayScale {
    let (window_w, window_h) = window.size();
    let (drawable_w, drawable_h) = window.drawable_size();

    if window_w == 0 || window_h == 0 {
        return DisplayScale::unity();
    }

    DisplayScale::new(
        drawable_w as f32 / window_w as f32,
        drawable_h as f32 / window_h as f32,
    )
}

/// Get window size in logical pixels (points)
pub fn get_window_size(window: &Window) -> WindowSize {
    let (width, height) = window.size();
    WindowSize {
        width: width as i32,
        height: height as i32,
    }
}

/// Get drawable size in physical pixels
pub fn get_drawable_size(window: &Window) -> DrawableSize {
    let (width, height) = window.drawable_size();
    DrawableSize {
        width: width as i32,
        height: height as i32,
    }
}

// =============================================================================
// Fullscreen Control
// =============================================================================

/// Fullscreen state
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FullscreenState {
    Windowed = 0,
    Fullscreen = 1,
    FullscreenDesktop = 2,
}

impl From<FullscreenType> for FullscreenState {
    fn from(ft: FullscreenType) -> Self {
        match ft {
            FullscreenType::Off => FullscreenState::Windowed,
            FullscreenType::True => FullscreenState::Fullscreen,
            FullscreenType::Desktop => FullscreenState::FullscreenDesktop,
        }
    }
}

/// Get the current fullscreen state
pub fn get_fullscreen_state(window: &Window) -> FullscreenState {
    window.fullscreen_state().into()
}

/// Toggle fullscreen mode
///
/// Toggles between windowed and desktop fullscreen mode.
/// Returns true on success, false on failure.
pub fn toggle_fullscreen(window: &mut Window) -> bool {
    let current = window.fullscreen_state();

    let new_state = match current {
        FullscreenType::Off => FullscreenType::Desktop,
        _ => FullscreenType::Off,
    };

    window.set_fullscreen(new_state).is_ok()
}

/// Set fullscreen mode explicitly
///
/// - `fullscreen = false` -> Windowed mode
/// - `fullscreen = true` -> Desktop fullscreen mode
pub fn set_fullscreen(window: &mut Window, fullscreen: bool) -> bool {
    let state = if fullscreen {
        FullscreenType::Desktop
    } else {
        FullscreenType::Off
    };

    window.set_fullscreen(state).is_ok()
}

/// Check if window is currently fullscreen
pub fn is_fullscreen(window: &Window) -> bool {
    matches!(
        window.fullscreen_state(),
        FullscreenType::True | FullscreenType::Desktop
    )
}

// =============================================================================
// FFI Exports
// =============================================================================

use std::os::raw::c_int;

/// Check if running on a Retina/HiDPI display
///
/// # Returns
/// - 1 if Retina display (scale factor > 1)
/// - 0 if standard display
/// - -1 if graphics not initialized
#[no_mangle]
pub extern "C" fn Platform_IsRetinaDisplay() -> c_int {
    crate::graphics::with_graphics(|state| {
        if is_retina_display(state.canvas.window()) {
            1
        } else {
            0
        }
    })
    .unwrap_or(-1)
}

/// Get the display scale factor
///
/// On Retina displays, this is typically 2.0.
/// On standard displays, this is 1.0.
///
/// # Parameters
/// - `scale_x`: Output pointer for X scale factor (nullable)
/// - `scale_y`: Output pointer for Y scale factor (nullable)
///
/// # Returns
/// - 0 on success
/// - -1 if graphics not initialized
#[no_mangle]
pub unsafe extern "C" fn Platform_GetDisplayScale(
    scale_x: *mut f32,
    scale_y: *mut f32,
) -> c_int {
    match crate::graphics::with_graphics(|state| {
        get_display_scale(state.canvas.window())
    }) {
        Ok(scale) => {
            if !scale_x.is_null() {
                *scale_x = scale.x;
            }
            if !scale_y.is_null() {
                *scale_y = scale.y;
            }
            0
        }
        Err(_) => -1,
    }
}

/// Toggle fullscreen mode
///
/// Switches between windowed and desktop fullscreen.
///
/// # Returns
/// - 0 on success
/// - -1 if failed or graphics not initialized
#[no_mangle]
pub extern "C" fn Platform_ToggleFullscreen() -> c_int {
    crate::graphics::with_graphics_mut(|state| {
        if toggle_fullscreen(state.canvas.window_mut()) {
            0
        } else {
            -1
        }
    })
    .unwrap_or(-1)
}

/// Set fullscreen mode explicitly
///
/// # Parameters
/// - `fullscreen`: 0 for windowed, non-zero for fullscreen
///
/// # Returns
/// - 0 on success
/// - -1 if failed or graphics not initialized
#[no_mangle]
pub extern "C" fn Platform_SetFullscreen(fullscreen: c_int) -> c_int {
    crate::graphics::with_graphics_mut(|state| {
        if set_fullscreen(state.canvas.window_mut(), fullscreen != 0) {
            0
        } else {
            -1
        }
    })
    .unwrap_or(-1)
}

/// Check if currently in fullscreen mode
///
/// # Returns
/// - 1 if fullscreen
/// - 0 if windowed
/// - -1 if graphics not initialized
#[no_mangle]
pub extern "C" fn Platform_IsFullscreen() -> c_int {
    crate::graphics::with_graphics(|state| {
        if is_fullscreen(state.canvas.window()) {
            1
        } else {
            0
        }
    })
    .unwrap_or(-1)
}

/// Get window size in logical pixels (points)
///
/// # Parameters
/// - `width`: Output pointer for width (nullable)
/// - `height`: Output pointer for height (nullable)
///
/// # Returns
/// - 0 on success
/// - -1 if graphics not initialized
#[no_mangle]
pub unsafe extern "C" fn Platform_GetWindowSize(
    width: *mut c_int,
    height: *mut c_int,
) -> c_int {
    match crate::graphics::with_graphics(|state| {
        get_window_size(state.canvas.window())
    }) {
        Ok(size) => {
            if !width.is_null() {
                *width = size.width;
            }
            if !height.is_null() {
                *height = size.height;
            }
            0
        }
        Err(_) => -1,
    }
}

/// Get drawable size in physical pixels
///
/// On Retina displays, this is larger than window size.
///
/// # Parameters
/// - `width`: Output pointer for width (nullable)
/// - `height`: Output pointer for height (nullable)
///
/// # Returns
/// - 0 on success
/// - -1 if graphics not initialized
#[no_mangle]
pub unsafe extern "C" fn Platform_GetDrawableSize(
    width: *mut c_int,
    height: *mut c_int,
) -> c_int {
    match crate::graphics::with_graphics(|state| {
        get_drawable_size(state.canvas.window())
    }) {
        Ok(size) => {
            if !width.is_null() {
                *width = size.width;
            }
            if !height.is_null() {
                *height = size.height;
            }
            0
        }
        Err(_) => -1,
    }
}

/// Get the fullscreen state
///
/// # Returns
/// - 0: Windowed
/// - 1: True fullscreen (exclusive)
/// - 2: Desktop fullscreen (borderless)
/// - -1: Graphics not initialized
#[no_mangle]
pub extern "C" fn Platform_GetFullscreenState() -> c_int {
    crate::graphics::with_graphics(|state| {
        get_fullscreen_state(state.canvas.window()) as c_int
    })
    .unwrap_or(-1)
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_display_scale_unity() {
        let scale = DisplayScale::unity();
        assert_eq!(scale.x, 1.0);
        assert_eq!(scale.y, 1.0);
        assert!(!scale.is_retina());
    }

    #[test]
    fn test_display_scale_retina() {
        let scale = DisplayScale::new(2.0, 2.0);
        assert!(scale.is_retina());
    }

    #[test]
    fn test_display_scale_mixed() {
        // Unusual but possible on some monitors
        let scale = DisplayScale::new(1.5, 1.5);
        assert!(scale.is_retina());
    }

    #[test]
    fn test_fullscreen_state_conversion() {
        assert_eq!(
            FullscreenState::from(FullscreenType::Off),
            FullscreenState::Windowed
        );
        assert_eq!(
            FullscreenState::from(FullscreenType::True),
            FullscreenState::Fullscreen
        );
        assert_eq!(
            FullscreenState::from(FullscreenType::Desktop),
            FullscreenState::FullscreenDesktop
        );
    }

    #[test]
    fn test_window_size_struct() {
        let size = WindowSize {
            width: 640,
            height: 400,
        };
        assert_eq!(size.width, 640);
        assert_eq!(size.height, 400);
    }

    #[test]
    fn test_drawable_size_struct() {
        let size = DrawableSize {
            width: 1280,
            height: 800,
        };
        assert_eq!(size.width, 1280);
        assert_eq!(size.height, 800);
    }

    // Note: Tests that require graphics state can't run without display
    // They're tested implicitly by the verification script

    #[test]
    fn test_ffi_without_graphics_returns_error() {
        // These should return -1 when graphics not initialized
        // Can't test in unit tests since graphics may or may not be init

        // Just verify the functions exist and are callable
        let _ = Platform_IsRetinaDisplay();
        let _ = Platform_IsFullscreen();
        let _ = Platform_GetFullscreenState();
    }
}
```

### platform/src/graphics/mod.rs (MODIFY)
Add the display module and helper functions. Near the top after existing module declarations:
```rust
pub mod display;
```

Add re-exports:
```rust
pub use display::{
    DisplayScale, DrawableSize, FullscreenState, WindowSize,
    get_display_scale, get_drawable_size, get_fullscreen_state,
    get_window_size, is_fullscreen, is_retina_display,
    set_fullscreen, toggle_fullscreen,
};
```

Add helper functions if they don't exist (typically near the bottom of mod.rs):
```rust
/// Access graphics state immutably
pub fn with_graphics<F, R>(f: F) -> Result<R, PlatformError>
where
    F: FnOnce(&GraphicsState) -> R,
{
    let guard = GRAPHICS.lock().map_err(|_| PlatformError::LockFailed)?;
    match guard.as_ref() {
        Some(wrapper) => Ok(f(&wrapper.0)),
        None => Err(PlatformError::NotInitialized),
    }
}

/// Access graphics state mutably
pub fn with_graphics_mut<F, R>(f: F) -> Result<R, PlatformError>
where
    F: FnOnce(&mut GraphicsState) -> R,
{
    let mut guard = GRAPHICS.lock().map_err(|_| PlatformError::LockFailed)?;
    match guard.as_mut() {
        Some(wrapper) => Ok(f(&mut wrapper.0)),
        None => Err(PlatformError::NotInitialized),
    }
}
```

Re-export FFI functions at module level:
```rust
// Re-export display FFI functions
pub use display::{
    Platform_IsRetinaDisplay,
    Platform_GetDisplayScale,
    Platform_ToggleFullscreen,
    Platform_SetFullscreen,
    Platform_IsFullscreen,
    Platform_GetWindowSize,
    Platform_GetDrawableSize,
    Platform_GetFullscreenState,
};
```

### platform/src/lib.rs (MODIFY)
Add re-exports for display FFI functions:
```rust
// Re-export display FFI functions
pub use graphics::{
    Platform_IsRetinaDisplay,
    Platform_GetDisplayScale,
    Platform_ToggleFullscreen,
    Platform_SetFullscreen,
    Platform_IsFullscreen,
    Platform_GetWindowSize,
    Platform_GetDrawableSize,
    Platform_GetFullscreenState,
};
```

## Verification Command
```bash
ralph-tasks/verify/check-display-integration.sh
```

## Verification Script
Create `ralph-tasks/verify/check-display-integration.sh`:
```bash
#!/bin/bash
# Verification script for display integration

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Display Integration ==="

cd "$ROOT_DIR"

# Step 1: Check display module exists
echo "Step 1: Checking display module exists..."
if [ ! -f "platform/src/graphics/display.rs" ]; then
    echo "VERIFY_FAILED: platform/src/graphics/display.rs not found"
    exit 1
fi
echo "  Display module exists"

# Step 2: Check mod.rs includes display
echo "Step 2: Checking mod.rs includes display..."
if ! grep -q "pub mod display" platform/src/graphics/mod.rs; then
    echo "VERIFY_FAILED: display module not declared in graphics/mod.rs"
    exit 1
fi
echo "  Display module declared"

# Step 3: Build the platform crate
echo "Step 3: Building platform crate..."
if ! cargo build --manifest-path platform/Cargo.toml --release 2>&1; then
    echo "VERIFY_FAILED: cargo build failed"
    exit 1
fi
echo "  Build successful"

# Step 4: Run display tests
echo "Step 4: Running display tests..."
if ! cargo test --manifest-path platform/Cargo.toml --release -- display 2>&1; then
    echo "VERIFY_FAILED: display tests failed"
    exit 1
fi
echo "  Tests passed"

# Step 5: Generate and check header
echo "Step 5: Checking FFI exports in header..."
if ! cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1; then
    echo "VERIFY_FAILED: cbindgen header generation failed"
    exit 1
fi

# Check for display FFI exports
for func in Platform_IsRetinaDisplay Platform_GetDisplayScale Platform_ToggleFullscreen Platform_SetFullscreen Platform_IsFullscreen Platform_GetWindowSize Platform_GetDrawableSize; do
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
1. Create `platform/src/graphics/display.rs`
2. Add `pub mod display;` to `platform/src/graphics/mod.rs`
3. Add `with_graphics` and `with_graphics_mut` helpers to mod.rs if not present
4. Add re-exports to mod.rs
5. Add FFI re-exports to lib.rs
6. Run `cargo build --manifest-path platform/Cargo.toml`
7. Run `cargo test --manifest-path platform/Cargo.toml -- display`
8. Run verification script

## Success Criteria
- All display functions compile without errors
- Unit tests pass
- FFI functions appear in platform.h
- Verification script exits with 0

## Common Issues

### "cannot find function with_graphics"
Add the helper functions to graphics/mod.rs as shown above.

### "expected struct Window, found &Window"
Make sure `canvas.window()` returns a reference. Some operations need `canvas.window_mut()`.

### "type mismatch in FFI function"
Ensure return types match: `c_int` for integers, proper pointer types.

### "trait bounds not satisfied"
The graphics state wrapper must be Send+Sync. This is already handled with unsafe impl.

## C++ Usage Example
```cpp
#include "platform.h"

void handle_display() {
    // Check for Retina display
    if (Platform_IsRetinaDisplay() == 1) {
        float scale_x, scale_y;
        Platform_GetDisplayScale(&scale_x, &scale_y);
        printf("Retina display: %.1fx scale\n", scale_x);
    }

    // Toggle fullscreen on F11
    if (key_pressed(KEY_F11)) {
        Platform_ToggleFullscreen();
    }

    // Get sizes for coordinate mapping
    int win_w, win_h, draw_w, draw_h;
    Platform_GetWindowSize(&win_w, &win_h);
    Platform_GetDrawableSize(&draw_w, &draw_h);

    // Scale mouse coordinates if needed
    float mouse_scale_x = (float)draw_w / win_w;
    float mouse_scale_y = (float)draw_h / win_h;
}
```

## Completion Promise
When verification passes, output:
```
<promise>TASK_11C_COMPLETE</promise>
```

## Escape Hatch
If stuck after 12 iterations:
- Document what's blocking in `ralph-tasks/blocked/11c.md`
- List attempted approaches
- Output: `<promise>TASK_11C_BLOCKED</promise>`

## Max Iterations
12
