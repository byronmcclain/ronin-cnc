//! FFI exports for C/C++ interop
//!
//! All functions in this module use C calling convention and are
//! designed to be called from the game's C++ code.

use crate::error::{catch_panic_or, get_error, set_error, clear_error};
use crate::{PLATFORM_STATE, PlatformState, PlatformResult, PLATFORM_VERSION};
use crate::graphics::{self, DisplayMode};
use crate::input;
use std::ffi::{c_char, CStr};

// Re-export input types for cbindgen
pub use crate::input::{KeyCode, MouseButton};

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
/// Uses the new logging infrastructure from crate::logging
#[no_mangle]
pub unsafe extern "C" fn Platform_Log(level: LogLevel, msg: *const c_char) {
    if msg.is_null() {
        return;
    }

    let msg_str = match CStr::from_ptr(msg).to_str() {
        Ok(s) => s,
        Err(_) => return,
    };

    // Convert LogLevel enum to u8 for the logging module
    // LogLevel: Debug=0, Info=1, Warn=2, Error=3
    // logging module: Error=0, Warn=1, Info=2, Debug=3, Trace=4
    let log_level = match level {
        LogLevel::Debug => 3,
        LogLevel::Info => 2,
        LogLevel::Warn => 1,
        LogLevel::Error => 0,
    };

    crate::logging::log_message(log_level, msg_str);
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

// =============================================================================
// Keyboard Input
// =============================================================================

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

// =============================================================================
// Mouse Input
// =============================================================================

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
    // SDL2 show_cursor takes bool and returns previous state
    // We just want to show/hide, so ignore the return value
    unsafe {
        sdl2::sys::SDL_ShowCursor(sdl2::sys::SDL_ENABLE as i32);
    }
}

/// Hide mouse cursor
#[no_mangle]
pub extern "C" fn Platform_Mouse_Hide() {
    unsafe {
        sdl2::sys::SDL_ShowCursor(sdl2::sys::SDL_DISABLE as i32);
    }
}

// =============================================================================
// Timer FFI
// =============================================================================

use crate::timer;

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

// =============================================================================
// Periodic Timers
// =============================================================================

use crate::timer::{TimerHandle, INVALID_TIMER_HANDLE};

/// C timer callback type
pub type CTimerCallback = extern "C" fn(*mut c_void);

/// Create a periodic timer
/// Note: The callback will be called from a separate thread!
#[no_mangle]
pub extern "C" fn Platform_Timer_Create(
    interval_ms: u32,
    callback: CTimerCallback,
    userdata: *mut c_void,
) -> TimerHandle {
    timer::create_timer_c(interval_ms, callback, userdata)
}

/// Destroy a periodic timer
#[no_mangle]
pub extern "C" fn Platform_Timer_Destroy(handle: TimerHandle) {
    timer::destroy_timer(handle);
}

/// Get invalid timer handle constant
#[no_mangle]
pub extern "C" fn Platform_Timer_InvalidHandle() -> TimerHandle {
    INVALID_TIMER_HANDLE
}

// =============================================================================
// Frame Rate Control FFI
// =============================================================================

/// Start frame timing
#[no_mangle]
pub extern "C" fn Platform_Frame_Begin() {
    timer::with_frame_controller(|fc| fc.begin_frame());
}

/// End frame timing (sleeps to maintain target FPS)
#[no_mangle]
pub extern "C" fn Platform_Frame_End() {
    timer::with_frame_controller(|fc| fc.end_frame());
}

/// Set target FPS
#[no_mangle]
pub extern "C" fn Platform_Frame_SetTargetFPS(fps: u32) {
    timer::with_frame_controller(|fc| fc.set_target_fps(fps));
}

/// Get last frame time in seconds
#[no_mangle]
pub extern "C" fn Platform_Frame_GetTime() -> f64 {
    timer::with_frame_controller(|fc| fc.frame_time())
}

/// Get current FPS (rolling average)
#[no_mangle]
pub extern "C" fn Platform_Frame_GetFPS() -> f64 {
    timer::with_frame_controller(|fc| fc.fps())
}

// =============================================================================
// Memory Allocation FFI
// =============================================================================

use crate::memory;

/// Allocate memory
#[no_mangle]
pub extern "C" fn Platform_Alloc(size: usize, flags: u32) -> *mut c_void {
    memory::mem_alloc(size, flags) as *mut c_void
}

/// Free memory
#[no_mangle]
pub extern "C" fn Platform_Free(ptr: *mut c_void, size: usize) {
    memory::mem_free(ptr as *mut u8, size);
}

/// Reallocate memory
#[no_mangle]
pub extern "C" fn Platform_Realloc(ptr: *mut c_void, old_size: usize, new_size: usize) -> *mut c_void {
    memory::mem_realloc(ptr as *mut u8, old_size, new_size) as *mut c_void
}

// =============================================================================
// Memory Stats FFI
// =============================================================================

/// Get free RAM (returns large value for compatibility)
#[no_mangle]
pub extern "C" fn Platform_Ram_Free() -> usize {
    memory::get_free_ram()
}

/// Get total RAM
#[no_mangle]
pub extern "C" fn Platform_Ram_Total() -> usize {
    memory::get_total_ram()
}

/// Get current allocation total
#[no_mangle]
pub extern "C" fn Platform_Mem_GetAllocated() -> usize {
    memory::get_allocated()
}

/// Get peak allocation
#[no_mangle]
pub extern "C" fn Platform_Mem_GetPeak() -> usize {
    memory::get_peak()
}

/// Get allocation count
#[no_mangle]
pub extern "C" fn Platform_Mem_GetCount() -> usize {
    memory::get_count()
}

/// Dump leaks (debug only)
#[no_mangle]
pub extern "C" fn Platform_Mem_DumpLeaks() {
    memory::dump_leaks();
}

// =============================================================================
// Memory Operations FFI
// =============================================================================

/// Copy memory (memcpy) - non-overlapping
#[no_mangle]
pub unsafe extern "C" fn Platform_MemCopy(dest: *mut c_void, src: *const c_void, count: usize) {
    if !dest.is_null() && !src.is_null() && count > 0 {
        std::ptr::copy_nonoverlapping(src as *const u8, dest as *mut u8, count);
    }
}

/// Move memory (memmove) - handles overlapping
#[no_mangle]
pub unsafe extern "C" fn Platform_MemMove(dest: *mut c_void, src: *const c_void, count: usize) {
    if !dest.is_null() && !src.is_null() && count > 0 {
        std::ptr::copy(src as *const u8, dest as *mut u8, count);
    }
}

/// Set memory (memset)
#[no_mangle]
pub unsafe extern "C" fn Platform_MemSet(dest: *mut c_void, value: i32, count: usize) {
    if !dest.is_null() && count > 0 {
        std::ptr::write_bytes(dest as *mut u8, value as u8, count);
    }
}

/// Compare memory (memcmp)
#[no_mangle]
pub unsafe extern "C" fn Platform_MemCmp(s1: *const c_void, s2: *const c_void, count: usize) -> i32 {
    if s1.is_null() || s2.is_null() || count == 0 {
        return 0;
    }

    let slice1 = std::slice::from_raw_parts(s1 as *const u8, count);
    let slice2 = std::slice::from_raw_parts(s2 as *const u8, count);

    for i in 0..count {
        let diff = slice1[i] as i32 - slice2[i] as i32;
        if diff != 0 {
            return diff;
        }
    }
    0
}

/// Zero memory
#[no_mangle]
pub unsafe extern "C" fn Platform_ZeroMemory(dest: *mut c_void, count: usize) {
    Platform_MemSet(dest, 0, count);
}

// =============================================================================
// File System FFI
// =============================================================================

use crate::files::{self, PlatformFile, PlatformDir, DirEntry, FileMode, SeekOrigin};

/// Get base path (executable directory)
#[no_mangle]
pub extern "C" fn Platform_GetBasePath() -> *const c_char {
    static mut PATH_BUF: [u8; 512] = [0; 512];

    let path = files::get_base_path();
    let bytes = path.as_bytes();
    let len = bytes.len().min(511);

    unsafe {
        PATH_BUF[..len].copy_from_slice(&bytes[..len]);
        PATH_BUF[len] = 0;
        PATH_BUF.as_ptr() as *const c_char
    }
}

/// Get preferences path (user data directory)
#[no_mangle]
pub extern "C" fn Platform_GetPrefPath() -> *const c_char {
    static mut PATH_BUF: [u8; 512] = [0; 512];

    let path = files::get_pref_path();
    let bytes = path.as_bytes();
    let len = bytes.len().min(511);

    unsafe {
        PATH_BUF[..len].copy_from_slice(&bytes[..len]);
        PATH_BUF[len] = 0;
        PATH_BUF.as_ptr() as *const c_char
    }
}

/// Initialize file system
#[no_mangle]
pub unsafe extern "C" fn Platform_Files_Init(org_name: *const c_char, app_name: *const c_char) {
    let org = if org_name.is_null() {
        ""
    } else {
        CStr::from_ptr(org_name).to_str().unwrap_or("")
    };

    let app = if app_name.is_null() {
        "RedAlert"
    } else {
        CStr::from_ptr(app_name).to_str().unwrap_or("RedAlert")
    };

    files::init(org, app);
}

/// Normalize a path (convert Windows to Unix style, modifies in place)
#[no_mangle]
pub unsafe extern "C" fn Platform_NormalizePath(path: *mut c_char) {
    if path.is_null() {
        return;
    }

    let c_str = CStr::from_ptr(path);
    if let Ok(s) = c_str.to_str() {
        let normalized = files::path::normalize(s);
        let bytes = normalized.as_bytes();
        let len = bytes.len();

        std::ptr::copy_nonoverlapping(bytes.as_ptr(), path as *mut u8, len);
        *path.add(len) = 0;
    }
}

/// Open a file
#[no_mangle]
pub unsafe extern "C" fn Platform_File_Open(
    path: *const c_char,
    mode: FileMode,
) -> *mut PlatformFile {
    if path.is_null() {
        return std::ptr::null_mut();
    }

    let path_str = match CStr::from_ptr(path).to_str() {
        Ok(s) => s,
        Err(_) => return std::ptr::null_mut(),
    };

    match PlatformFile::open(path_str, mode) {
        Ok(file) => Box::into_raw(Box::new(file)),
        Err(e) => {
            set_error(e.to_string());
            std::ptr::null_mut()
        }
    }
}

/// Close a file
#[no_mangle]
pub extern "C" fn Platform_File_Close(file: *mut PlatformFile) {
    if !file.is_null() {
        unsafe {
            drop(Box::from_raw(file));
        }
    }
}

/// Read from file
#[no_mangle]
pub unsafe extern "C" fn Platform_File_Read(
    file: *mut PlatformFile,
    buffer: *mut c_void,
    size: i32,
) -> i32 {
    if file.is_null() || buffer.is_null() || size <= 0 {
        return -1;
    }

    let file = &mut *file;
    let buf = std::slice::from_raw_parts_mut(buffer as *mut u8, size as usize);

    match file.read(buf) {
        Ok(n) => n as i32,
        Err(_) => -1,
    }
}

/// Write to file
#[no_mangle]
pub unsafe extern "C" fn Platform_File_Write(
    file: *mut PlatformFile,
    buffer: *const c_void,
    size: i32,
) -> i32 {
    if file.is_null() || buffer.is_null() || size <= 0 {
        return -1;
    }

    let file = &mut *file;
    let buf = std::slice::from_raw_parts(buffer as *const u8, size as usize);

    match file.write(buf) {
        Ok(n) => n as i32,
        Err(_) => -1,
    }
}

/// Seek in file
#[no_mangle]
pub unsafe extern "C" fn Platform_File_Seek(
    file: *mut PlatformFile,
    offset: i64,
    origin: SeekOrigin,
) -> i32 {
    if file.is_null() {
        return -1;
    }

    let file = &mut *file;
    match file.seek(offset, origin) {
        Ok(_) => 0,
        Err(_) => -1,
    }
}

/// Get file position
#[no_mangle]
pub unsafe extern "C" fn Platform_File_Tell(file: *mut PlatformFile) -> i64 {
    if file.is_null() {
        return -1;
    }

    let file = &mut *file;
    file.tell().map(|p| p as i64).unwrap_or(-1)
}

/// Get file size
#[no_mangle]
pub unsafe extern "C" fn Platform_File_Size(file: *const PlatformFile) -> i64 {
    if file.is_null() {
        return -1;
    }

    (*file).size()
}

/// Check if at end of file
#[no_mangle]
pub unsafe extern "C" fn Platform_File_Eof(file: *mut PlatformFile) -> bool {
    if file.is_null() {
        return true;
    }

    (*file).eof()
}

/// Flush file buffers
#[no_mangle]
pub unsafe extern "C" fn Platform_File_Flush(file: *mut PlatformFile) -> bool {
    if file.is_null() {
        return false;
    }

    (*file).flush().is_ok()
}

/// Check if file exists
#[no_mangle]
pub unsafe extern "C" fn Platform_File_Exists(path: *const c_char) -> bool {
    if path.is_null() {
        return false;
    }

    if let Ok(s) = CStr::from_ptr(path).to_str() {
        files::exists(s)
    } else {
        false
    }
}

/// Get file size without opening
#[no_mangle]
pub unsafe extern "C" fn Platform_File_GetSize(path: *const c_char) -> i64 {
    if path.is_null() {
        return -1;
    }

    if let Ok(s) = CStr::from_ptr(path).to_str() {
        files::get_file_size(s).unwrap_or(-1)
    } else {
        -1
    }
}

/// Delete a file
#[no_mangle]
pub unsafe extern "C" fn Platform_File_Delete(path: *const c_char) -> bool {
    if path.is_null() {
        return false;
    }

    if let Ok(s) = CStr::from_ptr(path).to_str() {
        files::delete_file(s).is_ok()
    } else {
        false
    }
}

/// Copy a file
#[no_mangle]
pub unsafe extern "C" fn Platform_File_Copy(src: *const c_char, dst: *const c_char) -> i64 {
    if src.is_null() || dst.is_null() {
        return -1;
    }

    let src_str = match CStr::from_ptr(src).to_str() {
        Ok(s) => s,
        Err(_) => return -1,
    };
    let dst_str = match CStr::from_ptr(dst).to_str() {
        Ok(s) => s,
        Err(_) => return -1,
    };

    files::copy_file(src_str, dst_str).map(|n| n as i64).unwrap_or(-1)
}

/// Open a directory for iteration
#[no_mangle]
pub unsafe extern "C" fn Platform_Dir_Open(path: *const c_char) -> *mut PlatformDir {
    if path.is_null() {
        return std::ptr::null_mut();
    }

    let path_str = match CStr::from_ptr(path).to_str() {
        Ok(s) => s,
        Err(_) => return std::ptr::null_mut(),
    };

    match PlatformDir::open(path_str) {
        Ok(dir) => Box::into_raw(Box::new(dir)),
        Err(_) => std::ptr::null_mut(),
    }
}

/// Read next directory entry
#[no_mangle]
pub unsafe extern "C" fn Platform_Dir_Read(
    dir: *mut PlatformDir,
    entry: *mut DirEntry,
) -> bool {
    if dir.is_null() || entry.is_null() {
        return false;
    }

    if let Some(e) = (*dir).read() {
        *entry = e;
        true
    } else {
        false
    }
}

/// Close directory
#[no_mangle]
pub extern "C" fn Platform_Dir_Close(dir: *mut PlatformDir) {
    if !dir.is_null() {
        unsafe {
            drop(Box::from_raw(dir));
        }
    }
}

/// Check if path is a directory
#[no_mangle]
pub unsafe extern "C" fn Platform_IsDirectory(path: *const c_char) -> bool {
    if path.is_null() {
        return false;
    }

    if let Ok(s) = CStr::from_ptr(path).to_str() {
        files::is_directory(s)
    } else {
        false
    }
}

/// Create a directory
#[no_mangle]
pub unsafe extern "C" fn Platform_CreateDirectory(path: *const c_char) -> bool {
    if path.is_null() {
        return false;
    }

    if let Ok(s) = CStr::from_ptr(path).to_str() {
        files::create_directory(s).is_ok()
    } else {
        false
    }
}

/// Delete a directory
#[no_mangle]
pub unsafe extern "C" fn Platform_DeleteDirectory(path: *const c_char) -> bool {
    if path.is_null() {
        return false;
    }

    if let Ok(s) = CStr::from_ptr(path).to_str() {
        files::delete_directory(s).is_ok()
    } else {
        false
    }
}

// =============================================================================
// Audio FFI
// =============================================================================

use crate::audio::{self, AudioConfig};

/// Initialize audio subsystem
#[no_mangle]
pub extern "C" fn Platform_Audio_Init(config: *const AudioConfig) -> i32 {
    let config = if config.is_null() {
        AudioConfig::default()
    } else {
        unsafe { *config }
    };

    match audio::init(&config) {
        Ok(()) => 0,
        Err(e) => {
            set_error(e.to_string());
            -1
        }
    }
}

/// Shutdown audio subsystem
#[no_mangle]
pub extern "C" fn Platform_Audio_Shutdown() {
    audio::shutdown();
}

/// Check if audio is initialized
#[no_mangle]
pub extern "C" fn Platform_Audio_IsInitialized() -> bool {
    audio::is_initialized()
}

/// Set master volume (0.0 to 1.0)
#[no_mangle]
pub extern "C" fn Platform_Audio_SetMasterVolume(volume: f32) {
    audio::set_master_volume(volume);
}

/// Get master volume
#[no_mangle]
pub extern "C" fn Platform_Audio_GetMasterVolume() -> f32 {
    audio::get_master_volume()
}

// =============================================================================
// Sound FFI
// =============================================================================

use crate::audio::{SoundHandle, PlayHandle, INVALID_SOUND_HANDLE, INVALID_PLAY_HANDLE};

/// Create a sound from raw PCM data in memory
#[no_mangle]
pub extern "C" fn Platform_Sound_CreateFromMemory(
    data: *const c_void,
    size: i32,
    sample_rate: i32,
    channels: i32,
    bits_per_sample: i32,
) -> SoundHandle {
    if data.is_null() || size <= 0 {
        return INVALID_SOUND_HANDLE;
    }

    let slice = unsafe { std::slice::from_raw_parts(data as *const u8, size as usize) };

    audio::create_sound(
        slice,
        sample_rate as u32,
        channels as u16,
        bits_per_sample as u16,
    )
}

/// Destroy a sound
#[no_mangle]
pub extern "C" fn Platform_Sound_Destroy(handle: SoundHandle) {
    audio::destroy_sound(handle);
}

/// Get number of loaded sounds
#[no_mangle]
pub extern "C" fn Platform_Sound_GetCount() -> i32 {
    audio::get_sound_count() as i32
}

/// Play a sound
#[no_mangle]
pub extern "C" fn Platform_Sound_Play(
    handle: SoundHandle,
    volume: f32,
    pan: f32,
    looping: bool,
) -> PlayHandle {
    audio::play(handle, volume, pan, looping)
}

/// Stop a playing sound
#[no_mangle]
pub extern "C" fn Platform_Sound_Stop(handle: PlayHandle) {
    audio::stop(handle);
}

/// Stop all playing sounds
#[no_mangle]
pub extern "C" fn Platform_Sound_StopAll() {
    audio::stop_all();
}

/// Check if a sound is playing
#[no_mangle]
pub extern "C" fn Platform_Sound_IsPlaying(handle: PlayHandle) -> bool {
    audio::is_playing(handle)
}

/// Set volume of a playing sound
#[no_mangle]
pub extern "C" fn Platform_Sound_SetVolume(handle: PlayHandle, volume: f32) {
    audio::set_volume(handle, volume);
}

/// Pause a playing sound
#[no_mangle]
pub extern "C" fn Platform_Sound_Pause(handle: PlayHandle) {
    audio::pause(handle);
}

/// Resume a paused sound
#[no_mangle]
pub extern "C" fn Platform_Sound_Resume(handle: PlayHandle) {
    audio::resume(handle);
}

/// Get number of currently playing sounds
#[no_mangle]
pub extern "C" fn Platform_Sound_GetPlayingCount() -> i32 {
    audio::get_playing_count() as i32
}

// =============================================================================
// ADPCM FFI
// =============================================================================

use crate::audio::adpcm;

/// Create a sound from ADPCM compressed data
#[no_mangle]
pub extern "C" fn Platform_Sound_CreateFromADPCM(
    data: *const c_void,
    size: i32,
    sample_rate: i32,
    channels: i32,
) -> SoundHandle {
    if data.is_null() || size <= 0 {
        return INVALID_SOUND_HANDLE;
    }

    let slice = unsafe { std::slice::from_raw_parts(data as *const u8, size as usize) };

    // Decode ADPCM to PCM
    let samples = adpcm::decode_adpcm(slice);

    if samples.is_empty() {
        return INVALID_SOUND_HANDLE;
    }

    // Convert i16 samples to bytes for create_sound
    let bytes: Vec<u8> = samples.iter()
        .flat_map(|&s| s.to_le_bytes())
        .collect();

    audio::create_sound(
        &bytes,
        sample_rate as u32,
        channels as u16,
        16, // ADPCM decodes to 16-bit
    )
}

/// Create a sound from ADPCM data with initial decoder state
#[no_mangle]
pub extern "C" fn Platform_Sound_CreateFromADPCMWithState(
    data: *const c_void,
    size: i32,
    sample_rate: i32,
    channels: i32,
    predictor: i32,
    step_index: i32,
) -> SoundHandle {
    if data.is_null() || size <= 0 {
        return INVALID_SOUND_HANDLE;
    }

    let slice = unsafe { std::slice::from_raw_parts(data as *const u8, size as usize) };

    // Decode ADPCM with initial state
    let samples = adpcm::decode_adpcm_with_state(slice, predictor, step_index);

    if samples.is_empty() {
        return INVALID_SOUND_HANDLE;
    }

    // Convert i16 samples to bytes
    let bytes: Vec<u8> = samples.iter()
        .flat_map(|&s| s.to_le_bytes())
        .collect();

    audio::create_sound(
        &bytes,
        sample_rate as u32,
        channels as u16,
        16,
    )
}

// =============================================================================
// Buffer Operations FFI (replaces assembly)
// =============================================================================

use crate::blit::{self, ClipRect};

// Re-export ClipRect for cbindgen
pub use crate::blit::ClipRect as PlatformClipRect;

/// Clear buffer with a single value
///
/// # Safety
/// - `buffer` must point to valid memory of at least `size` bytes
#[no_mangle]
pub unsafe extern "C" fn Platform_Buffer_Clear(
    buffer: *mut u8,
    size: i32,
    value: u8,
) {
    if buffer.is_null() || size <= 0 {
        return;
    }
    let slice = std::slice::from_raw_parts_mut(buffer, size as usize);
    blit::basic::buffer_clear(slice, value);
}

/// Fill rectangular region with a value
///
/// # Safety
/// - `buffer` must point to valid memory of at least `pitch * buf_height` bytes
#[no_mangle]
pub unsafe extern "C" fn Platform_Buffer_FillRect(
    buffer: *mut u8,
    pitch: i32,
    buf_width: i32,
    buf_height: i32,
    x: i32,
    y: i32,
    width: i32,
    height: i32,
    value: u8,
) {
    if buffer.is_null() || pitch <= 0 || buf_height <= 0 {
        return;
    }
    let buf_size = (pitch * buf_height) as usize;
    let slice = std::slice::from_raw_parts_mut(buffer, buf_size);
    blit::basic::buffer_fill_rect(
        slice,
        pitch as usize,
        buf_width,
        buf_height,
        x, y,
        width, height,
        value,
    );
}

/// Draw horizontal line
///
/// # Safety
/// - `buffer` must point to valid memory of at least `pitch * buf_height` bytes
#[no_mangle]
pub unsafe extern "C" fn Platform_Buffer_HLine(
    buffer: *mut u8,
    pitch: i32,
    buf_width: i32,
    buf_height: i32,
    x: i32,
    y: i32,
    length: i32,
    color: u8,
) {
    if buffer.is_null() || pitch <= 0 || buf_height <= 0 {
        return;
    }
    let buf_size = (pitch * buf_height) as usize;
    let slice = std::slice::from_raw_parts_mut(buffer, buf_size);
    blit::basic::buffer_hline(
        slice,
        pitch as usize,
        buf_width,
        buf_height,
        x, y,
        length,
        color,
    );
}

/// Draw vertical line
///
/// # Safety
/// - `buffer` must point to valid memory of at least `pitch * buf_height` bytes
#[no_mangle]
pub unsafe extern "C" fn Platform_Buffer_VLine(
    buffer: *mut u8,
    pitch: i32,
    buf_width: i32,
    buf_height: i32,
    x: i32,
    y: i32,
    length: i32,
    color: u8,
) {
    if buffer.is_null() || pitch <= 0 || buf_height <= 0 {
        return;
    }
    let buf_size = (pitch * buf_height) as usize;
    let slice = std::slice::from_raw_parts_mut(buffer, buf_size);
    blit::basic::buffer_vline(
        slice,
        pitch as usize,
        buf_width,
        buf_height,
        x, y,
        length,
        color,
    );
}

// =============================================================================
// Buffer Blit FFI (replaces BITBLIT.ASM)
// =============================================================================

/// Blit from source buffer to destination (opaque)
///
/// # Safety
/// - `dest` must point to valid memory of at least `dest_pitch * dest_height` bytes
/// - `src` must point to valid memory of at least `src_pitch * src_height` bytes
#[no_mangle]
pub unsafe extern "C" fn Platform_Buffer_ToBuffer(
    dest: *mut u8,
    dest_pitch: i32,
    dest_width: i32,
    dest_height: i32,
    src: *const u8,
    src_pitch: i32,
    src_width: i32,
    src_height: i32,
    dest_x: i32,
    dest_y: i32,
    src_x: i32,
    src_y: i32,
    width: i32,
    height: i32,
) {
    if dest.is_null() || src.is_null() {
        return;
    }
    if dest_pitch <= 0 || src_pitch <= 0 || dest_height <= 0 || src_height <= 0 {
        return;
    }

    let dest_size = (dest_pitch * dest_height) as usize;
    let src_size = (src_pitch * src_height) as usize;
    let dest_slice = std::slice::from_raw_parts_mut(dest, dest_size);
    let src_slice = std::slice::from_raw_parts(src, src_size);

    let src_rect = blit::ClipRect::new(src_x, src_y, width, height);
    blit::copy::buffer_to_buffer(
        dest_slice, dest_pitch as usize, dest_width, dest_height,
        src_slice, src_pitch as usize, src_width, src_height,
        dest_x, dest_y, src_rect,
    );
}

/// Blit with transparency (color 0 = transparent)
///
/// # Safety
/// Same as Platform_Buffer_ToBuffer
#[no_mangle]
pub unsafe extern "C" fn Platform_Buffer_ToBufferTrans(
    dest: *mut u8,
    dest_pitch: i32,
    dest_width: i32,
    dest_height: i32,
    src: *const u8,
    src_pitch: i32,
    src_width: i32,
    src_height: i32,
    dest_x: i32,
    dest_y: i32,
    src_x: i32,
    src_y: i32,
    width: i32,
    height: i32,
) {
    if dest.is_null() || src.is_null() {
        return;
    }
    if dest_pitch <= 0 || src_pitch <= 0 || dest_height <= 0 || src_height <= 0 {
        return;
    }

    let dest_size = (dest_pitch * dest_height) as usize;
    let src_size = (src_pitch * src_height) as usize;
    let dest_slice = std::slice::from_raw_parts_mut(dest, dest_size);
    let src_slice = std::slice::from_raw_parts(src, src_size);

    let src_rect = blit::ClipRect::new(src_x, src_y, width, height);
    blit::copy::buffer_to_buffer_trans(
        dest_slice, dest_pitch as usize, dest_width, dest_height,
        src_slice, src_pitch as usize, src_width, src_height,
        dest_x, dest_y, src_rect,
    );
}

/// Blit with custom transparent color key
///
/// # Safety
/// Same as Platform_Buffer_ToBuffer
#[no_mangle]
pub unsafe extern "C" fn Platform_Buffer_ToBufferTransKey(
    dest: *mut u8,
    dest_pitch: i32,
    dest_width: i32,
    dest_height: i32,
    src: *const u8,
    src_pitch: i32,
    src_width: i32,
    src_height: i32,
    dest_x: i32,
    dest_y: i32,
    src_x: i32,
    src_y: i32,
    width: i32,
    height: i32,
    transparent_color: u8,
) {
    if dest.is_null() || src.is_null() {
        return;
    }
    if dest_pitch <= 0 || src_pitch <= 0 || dest_height <= 0 || src_height <= 0 {
        return;
    }

    let dest_size = (dest_pitch * dest_height) as usize;
    let src_size = (src_pitch * src_height) as usize;
    let dest_slice = std::slice::from_raw_parts_mut(dest, dest_size);
    let src_slice = std::slice::from_raw_parts(src, src_size);

    let src_rect = blit::ClipRect::new(src_x, src_y, width, height);
    blit::copy::buffer_to_buffer_trans_key(
        dest_slice, dest_pitch as usize, dest_width, dest_height,
        src_slice, src_pitch as usize, src_width, src_height,
        dest_x, dest_y, src_rect,
        transparent_color,
    );
}

// =============================================================================
// Palette Remap FFI (replaces REMAP.ASM)
// =============================================================================

/// Apply remap table to buffer region
///
/// # Safety
/// - `buffer` must point to valid memory of at least `pitch * (y + height)` bytes
/// - `remap_table` must point to exactly 256 bytes
#[no_mangle]
pub unsafe extern "C" fn Platform_Buffer_Remap(
    buffer: *mut u8,
    pitch: i32,
    x: i32,
    y: i32,
    width: i32,
    height: i32,
    remap_table: *const u8,
) {
    if buffer.is_null() || remap_table.is_null() {
        return;
    }
    if pitch <= 0 || width <= 0 || height <= 0 {
        return;
    }

    let buf_size = (pitch * (y + height)) as usize;
    let buffer_slice = std::slice::from_raw_parts_mut(buffer, buf_size);
    let table: &[u8; 256] = &*(remap_table as *const [u8; 256]);

    blit::remap::buffer_remap(
        buffer_slice,
        pitch as usize,
        x, y,
        width, height,
        table,
    );
}

/// Apply remap table with transparency (skip color 0)
///
/// # Safety
/// Same as Platform_Buffer_Remap
#[no_mangle]
pub unsafe extern "C" fn Platform_Buffer_RemapTrans(
    buffer: *mut u8,
    pitch: i32,
    x: i32,
    y: i32,
    width: i32,
    height: i32,
    remap_table: *const u8,
) {
    if buffer.is_null() || remap_table.is_null() {
        return;
    }
    if pitch <= 0 || width <= 0 || height <= 0 {
        return;
    }

    let buf_size = (pitch * (y + height)) as usize;
    let buffer_slice = std::slice::from_raw_parts_mut(buffer, buf_size);
    let table: &[u8; 256] = &*(remap_table as *const [u8; 256]);

    blit::remap::buffer_remap_trans(
        buffer_slice,
        pitch as usize,
        x, y,
        width, height,
        table,
    );
}

/// Remap from source to destination
///
/// # Safety
/// - `dest` must point to valid memory
/// - `src` must point to valid memory
/// - `remap_table` must point to exactly 256 bytes
#[no_mangle]
pub unsafe extern "C" fn Platform_Buffer_RemapCopy(
    dest: *mut u8,
    dest_pitch: i32,
    dest_x: i32,
    dest_y: i32,
    src: *const u8,
    src_pitch: i32,
    src_x: i32,
    src_y: i32,
    width: i32,
    height: i32,
    remap_table: *const u8,
) {
    if dest.is_null() || src.is_null() || remap_table.is_null() {
        return;
    }
    if dest_pitch <= 0 || src_pitch <= 0 || width <= 0 || height <= 0 {
        return;
    }

    let dest_size = (dest_pitch * (dest_y + height)) as usize;
    let src_size = (src_pitch * (src_y + height)) as usize;
    let dest_slice = std::slice::from_raw_parts_mut(dest, dest_size);
    let src_slice = std::slice::from_raw_parts(src, src_size);
    let table: &[u8; 256] = &*(remap_table as *const [u8; 256]);

    blit::remap::buffer_remap_copy(
        dest_slice, dest_pitch as usize, dest_x, dest_y,
        src_slice, src_pitch as usize, src_x, src_y,
        width, height,
        table,
    );
}

// =============================================================================
// Buffer Scale FFI (replaces SCALE.ASM)
// =============================================================================

/// Scale buffer using nearest-neighbor interpolation
///
/// # Safety
/// - `dest` must point to valid memory of at least `dest_pitch * dest_height` bytes
/// - `src` must point to valid memory of at least `src_pitch * src_height` bytes
#[no_mangle]
pub unsafe extern "C" fn Platform_Buffer_Scale(
    dest: *mut u8,
    dest_pitch: i32,
    dest_width: i32,
    dest_height: i32,
    src: *const u8,
    src_pitch: i32,
    src_width: i32,
    src_height: i32,
    dest_x: i32,
    dest_y: i32,
    scale_width: i32,
    scale_height: i32,
) {
    if dest.is_null() || src.is_null() {
        return;
    }
    if dest_pitch <= 0 || src_pitch <= 0 {
        return;
    }
    if dest_height <= 0 || src_height <= 0 {
        return;
    }

    let dest_size = (dest_pitch * dest_height) as usize;
    let src_size = (src_pitch * src_height) as usize;
    let dest_slice = std::slice::from_raw_parts_mut(dest, dest_size);
    let src_slice = std::slice::from_raw_parts(src, src_size);

    blit::scale::buffer_scale(
        dest_slice, dest_pitch as usize, dest_width, dest_height,
        src_slice, src_pitch as usize, src_width, src_height,
        dest_x, dest_y, scale_width, scale_height,
    );
}

/// Scale buffer with transparency (skip color 0)
///
/// # Safety
/// Same as Platform_Buffer_Scale
#[no_mangle]
pub unsafe extern "C" fn Platform_Buffer_ScaleTrans(
    dest: *mut u8,
    dest_pitch: i32,
    dest_width: i32,
    dest_height: i32,
    src: *const u8,
    src_pitch: i32,
    src_width: i32,
    src_height: i32,
    dest_x: i32,
    dest_y: i32,
    scale_width: i32,
    scale_height: i32,
) {
    if dest.is_null() || src.is_null() {
        return;
    }
    if dest_pitch <= 0 || src_pitch <= 0 {
        return;
    }
    if dest_height <= 0 || src_height <= 0 {
        return;
    }

    let dest_size = (dest_pitch * dest_height) as usize;
    let src_size = (src_pitch * src_height) as usize;
    let dest_slice = std::slice::from_raw_parts_mut(dest, dest_size);
    let src_slice = std::slice::from_raw_parts(src, src_size);

    blit::scale::buffer_scale_trans(
        dest_slice, dest_pitch as usize, dest_width, dest_height,
        src_slice, src_pitch as usize, src_width, src_height,
        dest_x, dest_y, scale_width, scale_height,
    );
}

/// Scale buffer with custom transparent color
///
/// # Safety
/// Same as Platform_Buffer_Scale
#[no_mangle]
pub unsafe extern "C" fn Platform_Buffer_ScaleTransKey(
    dest: *mut u8,
    dest_pitch: i32,
    dest_width: i32,
    dest_height: i32,
    src: *const u8,
    src_pitch: i32,
    src_width: i32,
    src_height: i32,
    dest_x: i32,
    dest_y: i32,
    scale_width: i32,
    scale_height: i32,
    transparent_color: u8,
) {
    if dest.is_null() || src.is_null() {
        return;
    }
    if dest_pitch <= 0 || src_pitch <= 0 {
        return;
    }
    if dest_height <= 0 || src_height <= 0 {
        return;
    }

    let dest_size = (dest_pitch * dest_height) as usize;
    let src_size = (src_pitch * src_height) as usize;
    let dest_slice = std::slice::from_raw_parts_mut(dest, dest_size);
    let src_slice = std::slice::from_raw_parts(src, src_size);

    blit::scale::buffer_scale_trans_key(
        dest_slice, dest_pitch as usize, dest_width, dest_height,
        src_slice, src_pitch as usize, src_width, src_height,
        dest_x, dest_y, scale_width, scale_height,
        transparent_color,
    );
}

// =============================================================================
// Shadow FFI (replaces SHADOW.ASM)
// =============================================================================

/// Apply shadow to rectangular region
///
/// # Safety
/// - `buffer` must point to valid memory
/// - `shadow_table` must point to exactly 256 bytes
#[no_mangle]
pub unsafe extern "C" fn Platform_Buffer_Shadow(
    buffer: *mut u8,
    pitch: i32,
    x: i32,
    y: i32,
    width: i32,
    height: i32,
    shadow_table: *const u8,
) {
    if buffer.is_null() || shadow_table.is_null() {
        return;
    }
    if pitch <= 0 || width <= 0 || height <= 0 {
        return;
    }

    let buf_size = (pitch * (y + height)) as usize;
    let buffer_slice = std::slice::from_raw_parts_mut(buffer, buf_size);
    let table: &[u8; 256] = &*(shadow_table as *const [u8; 256]);

    blit::shadow::buffer_shadow(
        buffer_slice,
        pitch as usize,
        x, y, width, height,
        table,
    );
}

/// Apply shadow using mask sprite
///
/// # Safety
/// - `buffer` must point to valid memory
/// - `mask` must point to valid memory of at least `mask_pitch * height` bytes
/// - `shadow_table` must point to exactly 256 bytes
#[no_mangle]
pub unsafe extern "C" fn Platform_Buffer_ShadowMask(
    buffer: *mut u8,
    buffer_pitch: i32,
    buffer_width: i32,
    buffer_height: i32,
    mask: *const u8,
    mask_pitch: i32,
    x: i32,
    y: i32,
    width: i32,
    height: i32,
    shadow_table: *const u8,
) {
    if buffer.is_null() || mask.is_null() || shadow_table.is_null() {
        return;
    }
    if buffer_pitch <= 0 || mask_pitch <= 0 {
        return;
    }

    let buf_size = (buffer_pitch * buffer_height) as usize;
    let mask_size = (mask_pitch * height) as usize;
    let buffer_slice = std::slice::from_raw_parts_mut(buffer, buf_size);
    let mask_slice = std::slice::from_raw_parts(mask, mask_size);
    let table: &[u8; 256] = &*(shadow_table as *const [u8; 256]);

    blit::shadow::buffer_shadow_clipped(
        buffer_slice, buffer_pitch as usize, buffer_width, buffer_height,
        mask_slice, mask_pitch as usize,
        x, y, width, height,
        table,
    );
}

/// Generate shadow lookup table from palette
///
/// # Safety
/// - `palette` must point to 768 bytes (256 RGB triplets)
/// - `shadow_table` must point to 256 bytes for output
#[no_mangle]
pub unsafe extern "C" fn Platform_GenerateShadowTable(
    palette: *const u8,
    shadow_table: *mut u8,
    intensity: f32,
) {
    if palette.is_null() || shadow_table.is_null() {
        return;
    }

    let palette_array: &[[u8; 3]; 256] = &*(palette as *const [[u8; 3]; 256]);
    let table = blit::shadow::generate_shadow_table(palette_array, intensity);

    let output = std::slice::from_raw_parts_mut(shadow_table, 256);
    output.copy_from_slice(&table);
}

// =============================================================================
// LCW Compression FFI (replaces LCWCOMP.ASM, LCWUNCMP.ASM)
// =============================================================================

use crate::compression::lcw;

/// Decompress LCW-encoded data
///
/// # Arguments
/// * `source` - Pointer to compressed data
/// * `source_size` - Size of compressed data
/// * `dest` - Pointer to output buffer
/// * `dest_size` - Size of output buffer
///
/// # Returns
/// Number of bytes written, or -1 on error
///
/// # Safety
/// - `source` must point to valid memory of at least `source_size` bytes
/// - `dest` must point to valid memory of at least `dest_size` bytes
#[no_mangle]
pub unsafe extern "C" fn Platform_LCW_Decompress(
    source: *const u8,
    source_size: i32,
    dest: *mut u8,
    dest_size: i32,
) -> i32 {
    if source.is_null() || dest.is_null() {
        return -1;
    }
    if source_size <= 0 || dest_size <= 0 {
        return -1;
    }

    let src_slice = std::slice::from_raw_parts(source, source_size as usize);
    let dst_slice = std::slice::from_raw_parts_mut(dest, dest_size as usize);

    match lcw::lcw_decompress(src_slice, dst_slice) {
        Ok(size) => size as i32,
        Err(_) => -1,
    }
}

/// Compress data using LCW algorithm
///
/// # Arguments
/// * `source` - Pointer to uncompressed data
/// * `source_size` - Size of uncompressed data
/// * `dest` - Pointer to output buffer
/// * `dest_size` - Size of output buffer
///
/// # Returns
/// Number of bytes written, or -1 on error
///
/// # Safety
/// - `source` must point to valid memory of at least `source_size` bytes
/// - `dest` must point to valid memory of at least `dest_size` bytes
#[no_mangle]
pub unsafe extern "C" fn Platform_LCW_Compress(
    source: *const u8,
    source_size: i32,
    dest: *mut u8,
    dest_size: i32,
) -> i32 {
    if source.is_null() || dest.is_null() {
        return -1;
    }
    if source_size <= 0 || dest_size <= 0 {
        return -1;
    }

    let src_slice = std::slice::from_raw_parts(source, source_size as usize);
    let dst_slice = std::slice::from_raw_parts_mut(dest, dest_size as usize);

    match lcw::lcw_compress(src_slice, dst_slice) {
        Ok(size) => size as i32,
        Err(_) => -1,
    }
}

/// Calculate maximum compressed size for given input size
///
/// Use this to allocate output buffer for compression.
#[no_mangle]
pub extern "C" fn Platform_LCW_MaxCompressedSize(uncompressed_size: i32) -> i32 {
    if uncompressed_size <= 0 {
        return 1;  // Just end marker
    }
    lcw::lcw_max_compressed_size(uncompressed_size as usize) as i32
}

// =============================================================================
// CRC32 FFI (replaces CRC.ASM)
// =============================================================================

use crate::util::{crc, random};

/// Calculate CRC32 of data
///
/// # Safety
/// - `data` must point to valid memory of at least `size` bytes
#[no_mangle]
pub unsafe extern "C" fn Platform_CRC32(data: *const u8, size: i32) -> u32 {
    if data.is_null() || size <= 0 {
        return 0;
    }

    let slice = std::slice::from_raw_parts(data, size as usize);
    crc::crc32(slice)
}

/// Update CRC32 with more data (for streaming)
///
/// # Arguments
/// * `crc` - Previous CRC value (use 0xFFFFFFFF for first call)
/// * `data` - Pointer to data
/// * `size` - Size of data
///
/// # Returns
/// Updated CRC (call Platform_CRC32_Finalize when done)
///
/// # Safety
/// - `data` must point to valid memory of at least `size` bytes
#[no_mangle]
pub unsafe extern "C" fn Platform_CRC32_Update(crc_val: u32, data: *const u8, size: i32) -> u32 {
    if data.is_null() || size <= 0 {
        return crc_val;
    }

    let slice = std::slice::from_raw_parts(data, size as usize);
    crc::crc32_update(crc_val, slice)
}

/// Finalize CRC32 calculation
///
/// XORs with 0xFFFFFFFF to produce final value.
#[no_mangle]
pub extern "C" fn Platform_CRC32_Finalize(crc_val: u32) -> u32 {
    crc::crc32_finalize(crc_val)
}

/// Get initial CRC32 value for streaming
#[no_mangle]
pub extern "C" fn Platform_CRC32_Init() -> u32 {
    crc::CRC_INIT
}

// =============================================================================
// Random FFI (replaces RANDOM.ASM)
// =============================================================================

/// Seed the global random number generator
#[no_mangle]
pub extern "C" fn Platform_Random_Seed(seed: u32) {
    random::seed(seed);
}

/// Get the current random seed
#[no_mangle]
pub extern "C" fn Platform_Random_GetSeed() -> u32 {
    random::get_seed()
}

/// Get next random number (0-32767)
#[no_mangle]
pub extern "C" fn Platform_Random_Get() -> u32 {
    random::next()
}

/// Get random number in range [min, max]
#[no_mangle]
pub extern "C" fn Platform_Random_Range(min: i32, max: i32) -> i32 {
    random::range(min, max)
}

/// Get random number in range [0, max)
#[no_mangle]
pub extern "C" fn Platform_Random_Max(max: u32) -> u32 {
    random::next_max(max)
}
