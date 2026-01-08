//! Graphics subsystem using SDL2

pub mod display;
pub mod palette;
pub mod surface;

pub use palette::{Palette, PaletteEntry};
pub use surface::Surface;

// Re-export display types and functions
pub use display::{
    DisplayScale, DrawableSize, FullscreenState, WindowSize,
    get_display_scale, get_drawable_size, get_fullscreen_state,
    get_window_size, is_fullscreen, is_retina_display,
    set_fullscreen, toggle_fullscreen,
};

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

use crate::error::PlatformError;
use crate::input;
use sdl2::pixels::{Color, PixelFormatEnum};
use sdl2::render::{Canvas, TextureCreator};
use sdl2::video::{Window, WindowContext};
use sdl2::Sdl;
use std::sync::Mutex;
use once_cell::sync::Lazy;

/// Display mode configuration
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct DisplayMode {
    pub width: i32,
    pub height: i32,
    pub bits_per_pixel: i32,
}

impl Default for DisplayMode {
    fn default() -> Self {
        Self {
            width: 640,
            height: 400,
            bits_per_pixel: 8,
        }
    }
}

/// Wrapper to make SDL context Send+Sync
/// SAFETY: SDL operations must only be called from the main thread
struct SendSyncSdl(Sdl);
unsafe impl Send for SendSyncSdl {}
unsafe impl Sync for SendSyncSdl {}

/// Global SDL context - must be created once
static SDL_CONTEXT: Lazy<Mutex<Option<SendSyncSdl>>> = Lazy::new(|| Mutex::new(None));

/// Wrapper to make GraphicsState Send+Sync
/// SAFETY: Graphics operations must only be called from the main thread
struct SendSyncGraphics(GraphicsState);
unsafe impl Send for SendSyncGraphics {}
unsafe impl Sync for SendSyncGraphics {}

/// Global graphics state
static GRAPHICS: Lazy<Mutex<Option<SendSyncGraphics>>> = Lazy::new(|| Mutex::new(None));

pub struct GraphicsState {
    pub canvas: Canvas<Window>,
    pub texture_creator: TextureCreator<WindowContext>,
    /// Raw pointer to screen texture (lifetime managed manually)
    screen_texture_ptr: *mut sdl2::sys::SDL_Texture,
    pub display_mode: DisplayMode,
    pub back_buffer: Vec<u8>,
    pub palette: Palette,
    pub argb_buffer: Vec<u32>,
}

impl GraphicsState {
    pub fn new(sdl: &Sdl, mode: DisplayMode) -> Result<Self, PlatformError> {
        let video = sdl.video().map_err(|e| PlatformError::Sdl(e.to_string()))?;

        // Create window at 2x scale for better visibility on modern displays
        let window = video
            .window(
                "Command & Conquer: Red Alert",
                (mode.width * 2) as u32,
                (mode.height * 2) as u32,
            )
            .position_centered()
            .resizable()
            .build()
            .map_err(|e| PlatformError::Graphics(e.to_string()))?;

        let canvas = window
            .into_canvas()
            .accelerated()
            .present_vsync()
            .build()
            .map_err(|e| PlatformError::Graphics(e.to_string()))?;

        let texture_creator = canvas.texture_creator();

        // Create streaming texture for screen buffer (ARGB8888 for compatibility)
        let screen_texture = texture_creator
            .create_texture_streaming(
                PixelFormatEnum::ARGB8888,
                mode.width as u32,
                mode.height as u32,
            )
            .map_err(|e| PlatformError::Graphics(e.to_string()))?;

        // Extract raw pointer and forget the texture to manage lifetime manually
        let screen_texture_ptr = screen_texture.raw();
        std::mem::forget(screen_texture);

        let buffer_size = (mode.width * mode.height) as usize;

        Ok(Self {
            canvas,
            texture_creator,
            screen_texture_ptr,
            display_mode: mode,
            back_buffer: vec![0; buffer_size],
            palette: Palette::new(),
            argb_buffer: vec![0; buffer_size],
        })
    }

    /// Get pointer to back buffer for direct pixel access
    pub fn get_back_buffer(&mut self) -> (*mut u8, i32, i32, i32) {
        let ptr = self.back_buffer.as_mut_ptr();
        let width = self.display_mode.width;
        let height = self.display_mode.height;
        let pitch = width; // 8-bit indexed, so pitch = width
        (ptr, width, height, pitch)
    }

    /// Clear back buffer with a color index
    pub fn clear_back_buffer(&mut self, color: u8) {
        self.back_buffer.fill(color);
    }

    /// Convert back buffer using palette and store in argb_buffer
    pub fn convert_back_buffer(&mut self) {
        self.palette.convert_buffer(&self.back_buffer, &mut self.argb_buffer);
    }

    /// Update texture with 32-bit ARGB data
    /// Called by the palette conversion step
    pub fn update_texture(&mut self, argb_data: &[u32]) -> Result<(), PlatformError> {
        let width = self.display_mode.width as usize;
        let height = self.display_mode.height as usize;

        unsafe {
            let mut pixels_ptr: *mut std::ffi::c_void = std::ptr::null_mut();
            let mut texture_pitch: i32 = 0;

            let ret = sdl2::sys::SDL_LockTexture(
                self.screen_texture_ptr,
                std::ptr::null(),
                &mut pixels_ptr,
                &mut texture_pitch,
            );

            if ret != 0 {
                return Err(PlatformError::Graphics(
                    std::ffi::CStr::from_ptr(sdl2::sys::SDL_GetError())
                        .to_string_lossy()
                        .into_owned(),
                ));
            }

            let pixels = std::slice::from_raw_parts_mut(
                pixels_ptr as *mut u8,
                height * texture_pitch as usize,
            );

            for y in 0..height {
                let src_row = &argb_data[y * width..(y + 1) * width];
                let dst_row = &mut pixels[y * texture_pitch as usize..];

                for x in 0..width {
                    let color = src_row[x];
                    let offset = x * 4;
                    // ARGB8888 in SDL memory layout (BGRA on little-endian)
                    dst_row[offset] = (color & 0xFF) as u8;           // B
                    dst_row[offset + 1] = ((color >> 8) & 0xFF) as u8;  // G
                    dst_row[offset + 2] = ((color >> 16) & 0xFF) as u8; // R
                    dst_row[offset + 3] = ((color >> 24) & 0xFF) as u8; // A
                }
            }

            sdl2::sys::SDL_UnlockTexture(self.screen_texture_ptr);
        }

        Ok(())
    }

    /// Render texture to screen
    pub fn present_texture(&mut self) -> Result<(), PlatformError> {
        self.canvas.clear();

        unsafe {
            let renderer = self.canvas.raw();
            let ret = sdl2::sys::SDL_RenderCopy(
                renderer,
                self.screen_texture_ptr,
                std::ptr::null(),
                std::ptr::null(),
            );
            if ret != 0 {
                return Err(PlatformError::Graphics(
                    std::ffi::CStr::from_ptr(sdl2::sys::SDL_GetError())
                        .to_string_lossy()
                        .into_owned(),
                ));
            }
        }

        self.canvas.present();
        Ok(())
    }

    /// Clear screen with a color (for testing)
    pub fn clear(&mut self, r: u8, g: u8, b: u8) {
        self.canvas.set_draw_color(Color::RGB(r, g, b));
        self.canvas.clear();
    }

    /// Present the canvas
    pub fn present(&mut self) {
        self.canvas.present();
    }

    /// Complete flip operation:
    /// 1. Convert 8-bit back buffer to 32-bit ARGB using palette
    /// 2. Upload to streaming texture
    /// 3. Render texture to canvas
    /// 4. Present (with VSync)
    pub fn flip(&mut self) -> Result<(), PlatformError> {
        // Step 1: Convert back buffer using palette LUT
        self.palette.ensure_lut();
        self.palette.convert_buffer(&self.back_buffer, &mut self.argb_buffer);

        // Step 2: Update texture with converted pixels
        self.update_texture(&self.argb_buffer.clone())?;

        // Step 3 & 4: Render texture to canvas and present
        self.present_texture()
    }

    /// Draw directly to back buffer - vertical color bars for testing
    pub fn draw_test_pattern(&mut self) {
        let width = self.display_mode.width as usize;
        let height = self.display_mode.height as usize;

        for y in 0..height {
            for x in 0..width {
                // Vertical color bars
                let color = ((x * 256) / width) as u8;
                self.back_buffer[y * width + x] = color;
            }
        }
    }

    /// Draw horizontal gradient - for palette testing
    pub fn draw_gradient(&mut self) {
        let width = self.display_mode.width as usize;
        let height = self.display_mode.height as usize;

        for y in 0..height {
            for x in 0..width {
                let color = ((y * 256) / height) as u8;
                self.back_buffer[y * width + x] = color;
            }
        }
    }
}

impl Drop for GraphicsState {
    fn drop(&mut self) {
        // Clean up the manually managed texture
        if !self.screen_texture_ptr.is_null() {
            unsafe {
                sdl2::sys::SDL_DestroyTexture(self.screen_texture_ptr);
            }
        }
    }
}

/// Initialize SDL context
pub fn init_sdl() -> Result<(), PlatformError> {
    let sdl = sdl2::init().map_err(|e| PlatformError::Sdl(e))?;

    if let Ok(mut guard) = SDL_CONTEXT.lock() {
        *guard = Some(SendSyncSdl(sdl));
    }

    Ok(())
}

/// Get SDL context reference
pub fn with_sdl<F, R>(f: F) -> Option<R>
where
    F: FnOnce(&Sdl) -> R,
{
    SDL_CONTEXT.lock().ok().and_then(|guard| guard.as_ref().map(|s| f(&s.0)))
}

/// Initialize graphics subsystem
pub fn init() -> Result<(), PlatformError> {
    let mode = DisplayMode::default();

    let state = with_sdl(|sdl| GraphicsState::new(sdl, mode))
        .ok_or(PlatformError::NotInitialized)??;

    if let Ok(mut guard) = GRAPHICS.lock() {
        *guard = Some(SendSyncGraphics(state));
    }

    eprintln!("[INFO] Graphics initialized: {}x{}x{}",
              mode.width, mode.height, mode.bits_per_pixel);
    Ok(())
}

/// Shutdown graphics subsystem
pub fn shutdown() {
    if let Ok(mut guard) = GRAPHICS.lock() {
        *guard = None;
    }
    eprintln!("[INFO] Graphics shutdown");
}

/// Access graphics state with closure
pub fn with_graphics<F, R>(f: F) -> Option<R>
where
    F: FnOnce(&mut GraphicsState) -> R,
{
    GRAPHICS.lock().ok().and_then(|mut guard| guard.as_mut().map(|s| f(&mut s.0)))
}

/// Check if graphics is initialized
pub fn is_initialized() -> bool {
    GRAPHICS.lock().ok().map(|g| g.is_some()).unwrap_or(false)
}

/// Poll events and return true if quit was requested
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

/// Get current tick count in milliseconds
pub fn get_ticks() -> u32 {
    with_sdl(|sdl| {
        sdl.timer().map(|t| t.ticks()).unwrap_or(0)
    }).unwrap_or(0)
}

/// Delay for specified milliseconds
pub fn delay(ms: u32) {
    std::thread::sleep(std::time::Duration::from_millis(ms as u64));
}
