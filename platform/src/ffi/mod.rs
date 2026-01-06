//! FFI exports for C/C++ interop
//!
//! All functions in this module use C calling convention and are
//! designed to be called from the game's C++ code.

use crate::error::{catch_panic_or, get_error, set_error, clear_error};
use crate::{PLATFORM_STATE, PlatformState, PlatformResult, PLATFORM_VERSION};
use crate::graphics::{self, DisplayMode};
use std::ffi::{c_char, CStr};

/// Log level for Platform_Log
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum LogLevel {
    Debug = 0,
    Info = 1,
    Warn = 2,
    Error = 3,
}

// =============================================================================
// Platform Lifecycle
// =============================================================================

/// Initialize the platform layer
#[no_mangle]
pub extern "C" fn Platform_Init() -> PlatformResult {
    catch_panic_or(PlatformResult::InitFailed, || {
        let mut state = PLATFORM_STATE.lock().unwrap();
        if state.is_some() {
            return PlatformResult::AlreadyInitialized;
        }

        // Initialize SDL context
        if let Err(e) = graphics::init_sdl() {
            set_error(e.to_string());
            return PlatformResult::InitFailed;
        }

        *state = Some(PlatformState { initialized: true });
        PlatformResult::Success
    })
}

/// Shutdown the platform layer
#[no_mangle]
pub extern "C" fn Platform_Shutdown() -> PlatformResult {
    catch_panic_or(PlatformResult::NotInitialized, || {
        let mut state = PLATFORM_STATE.lock().unwrap();
        if state.is_none() {
            return PlatformResult::NotInitialized;
        }
        *state = None;
        PlatformResult::Success
    })
}

/// Check if platform is initialized
#[no_mangle]
pub extern "C" fn Platform_IsInitialized() -> bool {
    catch_panic_or(false, || {
        PLATFORM_STATE.lock().map(|s| s.is_some()).unwrap_or(false)
    })
}

/// Get platform version
#[no_mangle]
pub extern "C" fn Platform_GetVersion() -> i32 {
    PLATFORM_VERSION
}

// =============================================================================
// Error Handling
// =============================================================================

/// Get the last error message into a buffer
/// Returns number of bytes written, or -1 on error
/// Buffer will be null-terminated if there's space
#[no_mangle]
pub extern "C" fn Platform_GetLastError(buffer: *mut i8, buffer_size: i32) -> i32 {
    catch_panic_or(-1, || {
        if buffer.is_null() || buffer_size <= 0 {
            return -1;
        }

        let error = get_error();
        let bytes = error.as_bytes();
        let copy_len = std::cmp::min(bytes.len(), (buffer_size - 1) as usize);

        unsafe {
            std::ptr::copy_nonoverlapping(bytes.as_ptr(), buffer as *mut u8, copy_len);
            *buffer.add(copy_len) = 0; // null terminator
        }

        copy_len as i32
    })
}

/// Static buffer for error string (thread-safe)
static ERROR_STRING_BUF: std::sync::Mutex<[u8; 512]> = std::sync::Mutex::new([0; 512]);

/// Get error as static string pointer (valid until next error)
/// Returns null if no error
#[no_mangle]
pub extern "C" fn Platform_GetErrorString() -> *const c_char {
    catch_panic_or(std::ptr::null(), || {
        let error = get_error();
        if error.is_empty() {
            return std::ptr::null();
        }

        if let Ok(mut buf) = ERROR_STRING_BUF.lock() {
            let bytes = error.as_bytes();
            let len = std::cmp::min(bytes.len(), buf.len() - 1);
            buf[..len].copy_from_slice(&bytes[..len]);
            buf[len] = 0;
            buf.as_ptr() as *const c_char
        } else {
            std::ptr::null()
        }
    })
}

/// Set an error message from C string
#[no_mangle]
pub unsafe extern "C" fn Platform_SetError(msg: *const c_char) {
    if msg.is_null() {
        return;
    }

    if let Ok(s) = CStr::from_ptr(msg).to_str() {
        set_error(s);
    }
}

/// Clear the last error
#[no_mangle]
pub extern "C" fn Platform_ClearError() {
    clear_error();
}

// =============================================================================
// Logging
// =============================================================================

/// Log a message at the specified level
#[no_mangle]
pub unsafe extern "C" fn Platform_Log(level: LogLevel, msg: *const c_char) {
    if msg.is_null() {
        return;
    }

    let msg_str = match CStr::from_ptr(msg).to_str() {
        Ok(s) => s,
        Err(_) => return,
    };

    match level {
        LogLevel::Debug => eprintln!("[DEBUG] {}", msg_str),
        LogLevel::Info => eprintln!("[INFO] {}", msg_str),
        LogLevel::Warn => eprintln!("[WARN] {}", msg_str),
        LogLevel::Error => eprintln!("[ERROR] {}", msg_str),
    }
}

/// Log a formatted debug message
#[no_mangle]
pub unsafe extern "C" fn Platform_LogDebug(msg: *const c_char) {
    Platform_Log(LogLevel::Debug, msg);
}

/// Log a formatted info message
#[no_mangle]
pub unsafe extern "C" fn Platform_LogInfo(msg: *const c_char) {
    Platform_Log(LogLevel::Info, msg);
}

/// Log a formatted warning message
#[no_mangle]
pub unsafe extern "C" fn Platform_LogWarn(msg: *const c_char) {
    Platform_Log(LogLevel::Warn, msg);
}

/// Log a formatted error message
#[no_mangle]
pub unsafe extern "C" fn Platform_LogError(msg: *const c_char) {
    Platform_Log(LogLevel::Error, msg);
}

// =============================================================================
// Graphics
// =============================================================================

/// Initialize graphics subsystem
#[no_mangle]
pub extern "C" fn Platform_Graphics_Init() -> i32 {
    catch_panic_or(-1, || {
        match graphics::init() {
            Ok(()) => 0,
            Err(e) => {
                set_error(e.to_string());
                -1
            }
        }
    })
}

/// Shutdown graphics subsystem
#[no_mangle]
pub extern "C" fn Platform_Graphics_Shutdown() {
    graphics::shutdown();
}

/// Check if graphics is initialized
#[no_mangle]
pub extern "C" fn Platform_Graphics_IsInitialized() -> bool {
    graphics::is_initialized()
}

/// Clear screen with color (for testing)
#[no_mangle]
pub extern "C" fn Platform_Graphics_Clear(r: u8, g: u8, b: u8) {
    graphics::with_graphics(|state| {
        state.clear(r, g, b);
    });
}

/// Present the frame
#[no_mangle]
pub extern "C" fn Platform_Graphics_Present() {
    graphics::with_graphics(|state| {
        state.present();
    });
}

/// Get display mode
#[no_mangle]
pub extern "C" fn Platform_Graphics_GetMode(mode: *mut DisplayMode) {
    if mode.is_null() {
        return;
    }

    graphics::with_graphics(|state| {
        unsafe {
            *mode = state.display_mode;
        }
    });
}

/// Get back buffer for direct pixel access
/// Returns pointer to 8-bit indexed pixel buffer
#[no_mangle]
pub extern "C" fn Platform_Graphics_GetBackBuffer(
    pixels: *mut *mut u8,
    width: *mut i32,
    height: *mut i32,
    pitch: *mut i32,
) -> i32 {
    if pixels.is_null() {
        return -1;
    }

    let result = graphics::with_graphics(|state| {
        let (ptr, w, h, p) = state.get_back_buffer();
        unsafe {
            *pixels = ptr;
            if !width.is_null() { *width = w; }
            if !height.is_null() { *height = h; }
            if !pitch.is_null() { *pitch = p; }
        }
    });

    if result.is_some() { 0 } else { -1 }
}

/// Clear back buffer with color index
#[no_mangle]
pub extern "C" fn Platform_Graphics_ClearBackBuffer(color: u8) {
    graphics::with_graphics(|state| {
        state.clear_back_buffer(color);
    });
}

// =============================================================================
// Palette Management
// =============================================================================

use crate::graphics::PaletteEntry;

/// Set palette entries
#[no_mangle]
pub extern "C" fn Platform_Graphics_SetPalette(
    entries: *const PaletteEntry,
    start: i32,
    count: i32,
) {
    if entries.is_null() || start < 0 || count <= 0 || start + count > 256 {
        return;
    }

    graphics::with_graphics(|state| {
        unsafe {
            let slice = std::slice::from_raw_parts(entries, count as usize);
            state.palette.set_range(start as usize, slice);
        }
    });
}

/// Get palette entries
#[no_mangle]
pub extern "C" fn Platform_Graphics_GetPalette(
    entries: *mut PaletteEntry,
    start: i32,
    count: i32,
) {
    if entries.is_null() || start < 0 || count <= 0 || start + count > 256 {
        return;
    }

    graphics::with_graphics(|state| {
        unsafe {
            let dest = std::slice::from_raw_parts_mut(entries, count as usize);
            let src = state.palette.get_range(start as usize, count as usize);
            dest[..src.len()].copy_from_slice(src);
        }
    });
}

/// Set single palette entry
#[no_mangle]
pub extern "C" fn Platform_Graphics_SetPaletteEntry(index: u8, r: u8, g: u8, b: u8) {
    graphics::with_graphics(|state| {
        state.palette.set(index, PaletteEntry::new(r, g, b));
    });
}

/// Fade palette (0.0 = black, 1.0 = full)
#[no_mangle]
pub extern "C" fn Platform_Graphics_FadePalette(factor: f32) {
    graphics::with_graphics(|state| {
        state.palette.fade(factor);
    });
}

/// Restore palette from fade
#[no_mangle]
pub extern "C" fn Platform_Graphics_RestorePalette() {
    graphics::with_graphics(|state| {
        state.palette.restore();
    });
}

// =============================================================================
// Render Pipeline
// =============================================================================

/// Flip back buffer to screen (full render pipeline)
#[no_mangle]
pub extern "C" fn Platform_Graphics_Flip() -> i32 {
    let result = graphics::with_graphics(|state| {
        state.flip()
    });

    match result {
        Some(Ok(())) => 0,
        Some(Err(e)) => {
            set_error(e.to_string());
            -1
        }
        None => {
            set_error("Graphics not initialized");
            -1
        }
    }
}

/// Wait for vertical sync (no-op, handled by SDL)
#[no_mangle]
pub extern "C" fn Platform_Graphics_WaitVSync() {
    // VSync is handled by SDL_RENDERER_PRESENTVSYNC flag
}

/// Draw test pattern to back buffer (for testing)
#[no_mangle]
pub extern "C" fn Platform_Graphics_DrawTestPattern() {
    graphics::with_graphics(|state| {
        state.draw_test_pattern();
    });
}

/// Get free video memory (returns large value for compatibility)
#[no_mangle]
pub extern "C" fn Platform_Graphics_GetFreeVideoMemory() -> u32 {
    256 * 1024 * 1024 // 256 MB
}

/// Check if hardware accelerated
#[no_mangle]
pub extern "C" fn Platform_Graphics_IsHardwareAccelerated() -> bool {
    true // SDL2 renderer is always accelerated on modern systems
}

// =============================================================================
// Event Handling
// =============================================================================

/// Poll events and return true if quit was requested (window close or Escape key)
#[no_mangle]
pub extern "C" fn Platform_PollEvents() -> bool {
    graphics::poll_events()
}

/// Get tick count in milliseconds since SDL init
#[no_mangle]
pub extern "C" fn Platform_GetTicks() -> u32 {
    graphics::get_ticks()
}

/// Delay for specified milliseconds
#[no_mangle]
pub extern "C" fn Platform_Delay(ms: u32) {
    graphics::delay(ms);
}

// =============================================================================
// Surface Management
// =============================================================================

use crate::graphics::Surface;
use std::ffi::c_void;

/// Opaque surface handle for C
#[repr(C)]
pub struct PlatformSurface {
    _private: [u8; 0],
}

/// Internal surface wrapper (not exposed to C)
struct SurfaceWrapper(Surface);

/// Convert raw pointer to internal surface
unsafe fn surface_ref(ptr: *const PlatformSurface) -> &'static Surface {
    &(*(ptr as *const SurfaceWrapper)).0
}

/// Convert raw pointer to mutable internal surface
unsafe fn surface_mut(ptr: *mut PlatformSurface) -> &'static mut Surface {
    &mut (*(ptr as *mut SurfaceWrapper)).0
}

/// Create a new surface
#[no_mangle]
pub extern "C" fn Platform_Surface_Create(width: i32, height: i32, bpp: i32) -> *mut PlatformSurface {
    if width <= 0 || height <= 0 || bpp <= 0 {
        return std::ptr::null_mut();
    }
    let surface = Surface::new(width, height, bpp);
    Box::into_raw(Box::new(SurfaceWrapper(surface))) as *mut PlatformSurface
}

/// Destroy a surface
#[no_mangle]
pub extern "C" fn Platform_Surface_Destroy(surface: *mut PlatformSurface) {
    if !surface.is_null() {
        unsafe {
            drop(Box::from_raw(surface as *mut SurfaceWrapper));
        }
    }
}

/// Get surface dimensions
#[no_mangle]
pub extern "C" fn Platform_Surface_GetSize(
    surface: *const PlatformSurface,
    width: *mut i32,
    height: *mut i32,
) {
    if surface.is_null() {
        return;
    }
    unsafe {
        let (w, h) = surface_ref(surface).get_size();
        if !width.is_null() {
            *width = w;
        }
        if !height.is_null() {
            *height = h;
        }
    }
}

/// Lock surface for direct pixel access
#[no_mangle]
pub extern "C" fn Platform_Surface_Lock(
    surface: *mut PlatformSurface,
    pixels: *mut *mut c_void,
    pitch: *mut i32,
) -> i32 {
    if surface.is_null() || pixels.is_null() || pitch.is_null() {
        return -1;
    }

    unsafe {
        match surface_mut(surface).lock() {
            Ok((ptr, p)) => {
                *pixels = ptr as *mut c_void;
                *pitch = p;
                0
            }
            Err(e) => {
                set_error(e.to_string());
                -1
            }
        }
    }
}

/// Unlock surface
#[no_mangle]
pub extern "C" fn Platform_Surface_Unlock(surface: *mut PlatformSurface) {
    if !surface.is_null() {
        unsafe {
            surface_mut(surface).unlock();
        }
    }
}

/// Blit surface to another
#[no_mangle]
pub extern "C" fn Platform_Surface_Blit(
    dest: *mut PlatformSurface,
    dx: i32,
    dy: i32,
    src: *const PlatformSurface,
    sx: i32,
    sy: i32,
    sw: i32,
    sh: i32,
) -> i32 {
    if dest.is_null() || src.is_null() {
        return -1;
    }

    unsafe {
        match surface_mut(dest).blit(dx, dy, surface_ref(src), sx, sy, sw, sh) {
            Ok(()) => 0,
            Err(e) => {
                set_error(e.to_string());
                -1
            }
        }
    }
}

/// Blit with transparency (skip color 0)
#[no_mangle]
pub extern "C" fn Platform_Surface_BlitTransparent(
    dest: *mut PlatformSurface,
    dx: i32,
    dy: i32,
    src: *const PlatformSurface,
    sx: i32,
    sy: i32,
    sw: i32,
    sh: i32,
) -> i32 {
    if dest.is_null() || src.is_null() {
        return -1;
    }

    unsafe {
        match surface_mut(dest).blit_transparent(dx, dy, surface_ref(src), sx, sy, sw, sh) {
            Ok(()) => 0,
            Err(e) => {
                set_error(e.to_string());
                -1
            }
        }
    }
}

/// Fill rectangle
#[no_mangle]
pub extern "C" fn Platform_Surface_Fill(
    surface: *mut PlatformSurface,
    x: i32,
    y: i32,
    w: i32,
    h: i32,
    color: u8,
) {
    if !surface.is_null() {
        unsafe {
            surface_mut(surface).fill(x, y, w, h, color);
        }
    }
}

/// Clear entire surface
#[no_mangle]
pub extern "C" fn Platform_Surface_Clear(surface: *mut PlatformSurface, color: u8) {
    if !surface.is_null() {
        unsafe {
            surface_mut(surface).clear(color);
        }
    }
}
