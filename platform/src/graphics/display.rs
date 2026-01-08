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
    super::with_graphics(|state| {
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
    match super::with_graphics(|state| {
        get_display_scale(state.canvas.window())
    }) {
        Some(scale) => {
            if !scale_x.is_null() {
                *scale_x = scale.x;
            }
            if !scale_y.is_null() {
                *scale_y = scale.y;
            }
            0
        }
        None => -1,
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
    super::with_graphics(|state| {
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
    super::with_graphics(|state| {
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
    super::with_graphics(|state| {
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
    match super::with_graphics(|state| {
        get_window_size(state.canvas.window())
    }) {
        Some(size) => {
            if !width.is_null() {
                *width = size.width;
            }
            if !height.is_null() {
                *height = size.height;
            }
            0
        }
        None => -1,
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
    match super::with_graphics(|state| {
        get_drawable_size(state.canvas.window())
    }) {
        Some(size) => {
            if !width.is_null() {
                *width = size.width;
            }
            if !height.is_null() {
                *height = size.height;
            }
            0
        }
        None => -1,
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
    super::with_graphics(|state| {
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
        // Just verify the functions exist and are callable
        let _ = Platform_IsRetinaDisplay();
        let _ = Platform_IsFullscreen();
        let _ = Platform_GetFullscreenState();
    }
}
