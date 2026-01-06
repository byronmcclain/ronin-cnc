# Plan 03: Graphics System (Rust + sdl2-rs)

## Objective
Replace DirectDraw with Rust's sdl2 crate, supporting the original 8-bit palettized graphics modes (320x200, 640x400, 640x480) with proper scaling to modern displays.

## Current State Analysis

### DirectDraw Usage in Codebase

**Primary Files:**
- `WIN32LIB/MISC/DDRAW.CPP` (1001 lines) - Core DirectDraw wrapper
- `WIN32LIB/DRAWBUFF/GBUFFER.CPP` - Graphics buffer class
- `WIN32LIB/INCLUDE/GBUFFER.H` - Buffer definitions
- `CODE/STARTUP.CPP` - Display initialization

**Key DirectDraw Objects:**
```cpp
LPDIRECTDRAW DirectDrawObject;
LPDIRECTDRAW2 DirectDraw2Interface;
LPDIRECTDRAWSURFACE PaletteSurface;
LPDIRECTDRAWPALETTE PalettePtr;
PALETTEENTRY PaletteEntries[256];
```

**Display Modes Used:**
- 320x200x8 (ModeX-style, 2x scaled to 640x400)
- 640x400x8 (primary hi-res mode)
- 640x480x8 (alternate hi-res)

## Rust Implementation Strategy

### Architecture

```
┌────────────────────────────────────────────────────────────────┐
│                        Game Code (C++)                          │
│              (uses GraphicBufferClass, etc.)                    │
└────────────────────────────────────────────────────────────────┘
                               │
                               ▼  extern "C" FFI
┌────────────────────────────────────────────────────────────────┐
│                    platform.h (cbindgen)                        │
│              Platform_Graphics_*, Platform_Surface_*            │
└────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌────────────────────────────────────────────────────────────────┐
│                  Rust Graphics Module                           │
│                 platform/src/graphics/                          │
├────────────────────────────────────────────────────────────────┤
│   Surface   │   Palette   │   Window   │   Texture Streaming   │
└────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌────────────────────────────────────────────────────────────────┐
│                      sdl2 crate (0.36)                          │
├────────────────────────────────────────────────────────────────┤
│  VideoSubsystem  │  Canvas  │  TextureCreator  │  EventPump    │
└────────────────────────────────────────────────────────────────┘
```

### Rendering Pipeline

```
1. Game writes to 8-bit buffer (index values 0-255)
                    │
                    ▼
2. On flip: Rust converts 8-bit → 32-bit RGBA using palette LUT
                    │
                    ▼
3. Upload to SDL_Texture (GPU) via texture.update()
                    │
                    ▼
4. canvas.copy() with scaling to window size
                    │
                    ▼
5. canvas.present() (display with VSync)
```

## Rust Implementation

### 3.1 Graphics State

File: `platform/src/graphics/mod.rs`
```rust
//! Graphics subsystem using sdl2-rs

pub mod surface;
pub mod palette;

use crate::error::PlatformError;
use crate::PlatformResult;
use sdl2::pixels::PixelFormatEnum;
use sdl2::render::{Canvas, Texture, TextureCreator};
use sdl2::video::{Window, WindowContext};
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

/// RGB palette entry
#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct PaletteEntry {
    pub r: u8,
    pub g: u8,
    pub b: u8,
}

/// Global graphics state
static GRAPHICS: Lazy<Mutex<Option<GraphicsState>>> = Lazy::new(|| Mutex::new(None));

pub struct GraphicsState {
    pub canvas: Canvas<Window>,
    pub texture_creator: TextureCreator<WindowContext>,
    pub screen_texture: Texture,
    pub display_mode: DisplayMode,
    pub palette: [PaletteEntry; 256],
    pub palette_rgba: [u32; 256],
    pub back_buffer: Vec<u8>,
    pub front_buffer: Vec<u8>,
}

impl GraphicsState {
    pub fn new(sdl: &sdl2::Sdl, mode: DisplayMode) -> PlatformResult<Self> {
        let video = sdl.video().map_err(PlatformError::Sdl)?;

        // Create window at 2x scale for better visibility
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

        // Create streaming texture for screen buffer
        let screen_texture = texture_creator
            .create_texture_streaming(
                PixelFormatEnum::ARGB8888,
                mode.width as u32,
                mode.height as u32,
            )
            .map_err(|e| PlatformError::Graphics(e.to_string()))?;

        let buffer_size = (mode.width * mode.height) as usize;

        // Initialize grayscale palette
        let mut palette = [PaletteEntry::default(); 256];
        let mut palette_rgba = [0u32; 256];
        for i in 0..256 {
            palette[i] = PaletteEntry {
                r: i as u8,
                g: i as u8,
                b: i as u8,
            };
            palette_rgba[i] = 0xFF000000 | ((i as u32) << 16) | ((i as u32) << 8) | (i as u32);
        }

        Ok(Self {
            canvas,
            texture_creator,
            screen_texture,
            display_mode: mode,
            palette,
            palette_rgba,
            back_buffer: vec![0; buffer_size],
            front_buffer: vec![0; buffer_size],
        })
    }

    /// Rebuild RGBA palette lookup table from palette entries
    pub fn rebuild_palette_lut(&mut self) {
        for i in 0..256 {
            let p = &self.palette[i];
            self.palette_rgba[i] =
                0xFF000000 | ((p.r as u32) << 16) | ((p.g as u32) << 8) | (p.b as u32);
        }
    }

    /// Convert 8-bit back buffer to texture and present
    pub fn flip(&mut self) -> PlatformResult<()> {
        let width = self.display_mode.width as usize;
        let height = self.display_mode.height as usize;
        let palette_rgba = &self.palette_rgba;
        let back_buffer = &self.back_buffer;

        // Update texture with converted pixels
        self.screen_texture
            .with_lock(None, |pixels: &mut [u8], pitch: usize| {
                for y in 0..height {
                    let src_row = &back_buffer[y * width..(y + 1) * width];
                    let dst_row = &mut pixels[y * pitch..(y + 1) * pitch];

                    for x in 0..width {
                        let color = palette_rgba[src_row[x] as usize];
                        let offset = x * 4;
                        // ARGB8888 format
                        dst_row[offset] = (color >> 24) as u8;     // A
                        dst_row[offset + 1] = (color >> 16) as u8; // R
                        dst_row[offset + 2] = (color >> 8) as u8;  // G
                        dst_row[offset + 3] = color as u8;         // B
                    }
                }
            })
            .map_err(|e| PlatformError::Graphics(e))?;

        // Clear and render
        self.canvas.clear();
        self.canvas
            .copy(&self.screen_texture, None, None)
            .map_err(|e| PlatformError::Graphics(e))?;
        self.canvas.present();

        // Swap buffers
        std::mem::swap(&mut self.back_buffer, &mut self.front_buffer);

        Ok(())
    }
}

/// Initialize graphics subsystem
pub fn init(sdl: &sdl2::Sdl) -> PlatformResult<()> {
    let mode = DisplayMode::default();
    let state = GraphicsState::new(sdl, mode)?;

    if let Ok(mut guard) = GRAPHICS.lock() {
        *guard = Some(state);
    }

    log::info!("Graphics initialized: {}x{}x{}",
               mode.width, mode.height, mode.bits_per_pixel);
    Ok(())
}

/// Shutdown graphics subsystem
pub fn shutdown() {
    if let Ok(mut guard) = GRAPHICS.lock() {
        *guard = None;
    }
    log::info!("Graphics shutdown");
}

/// Access graphics state with closure
pub fn with_graphics<F, R>(f: F) -> Option<R>
where
    F: FnOnce(&mut GraphicsState) -> R,
{
    GRAPHICS.lock().ok().and_then(|mut guard| guard.as_mut().map(f))
}
```

### 3.2 Surface Implementation

File: `platform/src/graphics/surface.rs`
```rust
//! Software surface for off-screen rendering

use crate::error::PlatformError;
use crate::PlatformResult;

/// Software surface (8-bit indexed)
pub struct Surface {
    pub pixels: Vec<u8>,
    pub width: i32,
    pub height: i32,
    pub pitch: i32,
    pub bpp: i32,
    locked: bool,
}

impl Surface {
    /// Create new surface
    pub fn new(width: i32, height: i32, bpp: i32) -> Self {
        let pitch = width * (bpp / 8);
        let size = (pitch * height) as usize;

        Self {
            pixels: vec![0; size],
            width,
            height,
            pitch,
            bpp,
            locked: false,
        }
    }

    /// Create surface from existing memory
    pub fn from_memory(data: &[u8], width: i32, height: i32, pitch: i32, bpp: i32) -> Self {
        Self {
            pixels: data.to_vec(),
            width,
            height,
            pitch,
            bpp,
            locked: false,
        }
    }

    /// Lock surface for direct pixel access
    pub fn lock(&mut self) -> PlatformResult<(*mut u8, i32)> {
        if self.locked {
            return Err(PlatformError::Graphics("Surface already locked".into()));
        }
        self.locked = true;
        Ok((self.pixels.as_mut_ptr(), self.pitch))
    }

    /// Unlock surface
    pub fn unlock(&mut self) {
        self.locked = false;
    }

    /// Blit from another surface
    pub fn blit(
        &mut self,
        dx: i32,
        dy: i32,
        src: &Surface,
        sx: i32,
        sy: i32,
        sw: i32,
        sh: i32,
    ) -> PlatformResult<()> {
        // Clip to destination bounds
        let (dx, dy, sx, sy, sw, sh) = clip_blit(
            dx, dy, sx, sy, sw, sh,
            self.width, self.height,
            src.width, src.height,
        );

        if sw <= 0 || sh <= 0 {
            return Ok(());
        }

        // Copy pixels
        for row in 0..sh {
            let src_offset = ((sy + row) * src.pitch + sx) as usize;
            let dst_offset = ((dy + row) * self.pitch + dx) as usize;
            let width = sw as usize;

            self.pixels[dst_offset..dst_offset + width]
                .copy_from_slice(&src.pixels[src_offset..src_offset + width]);
        }

        Ok(())
    }

    /// Fill rectangle with color
    pub fn fill(&mut self, x: i32, y: i32, w: i32, h: i32, color: u8) {
        let x = x.max(0);
        let y = y.max(0);
        let w = w.min(self.width - x);
        let h = h.min(self.height - y);

        if w <= 0 || h <= 0 {
            return;
        }

        for row in 0..h {
            let offset = ((y + row) * self.pitch + x) as usize;
            let width = w as usize;
            self.pixels[offset..offset + width].fill(color);
        }
    }

    /// Clear entire surface
    pub fn clear(&mut self, color: u8) {
        self.pixels.fill(color);
    }
}

/// Clip blit coordinates to bounds
fn clip_blit(
    mut dx: i32, mut dy: i32,
    mut sx: i32, mut sy: i32,
    mut sw: i32, mut sh: i32,
    dw: i32, dh: i32,
    _sw_max: i32, _sh_max: i32,
) -> (i32, i32, i32, i32, i32, i32) {
    // Clip left edge
    if dx < 0 {
        sx -= dx;
        sw += dx;
        dx = 0;
    }
    // Clip top edge
    if dy < 0 {
        sy -= dy;
        sh += dy;
        dy = 0;
    }
    // Clip right edge
    if dx + sw > dw {
        sw = dw - dx;
    }
    // Clip bottom edge
    if dy + sh > dh {
        sh = dh - dy;
    }

    (dx, dy, sx, sy, sw, sh)
}
```

### 3.3 FFI Graphics Exports

File: `platform/src/ffi/graphics.rs`
```rust
//! Graphics FFI exports

use crate::error::{catch_panic, catch_panic_option};
use crate::graphics::{self, DisplayMode, GraphicsState, PaletteEntry, Surface};
use std::ffi::c_void;

/// Opaque surface handle for C
pub struct PlatformSurface(Surface);

// =============================================================================
// Initialization
// =============================================================================

/// Initialize graphics subsystem
#[no_mangle]
pub extern "C" fn Platform_Graphics_Init() -> i32 {
    catch_panic(|| {
        // Graphics init is done as part of Platform_Init
        Ok(())
    })
}

/// Shutdown graphics subsystem
#[no_mangle]
pub extern "C" fn Platform_Graphics_Shutdown() {
    graphics::shutdown();
}

/// Set display mode
#[no_mangle]
pub extern "C" fn Platform_Graphics_SetMode(mode: *const DisplayMode) -> i32 {
    catch_panic(|| {
        if mode.is_null() {
            return Err(crate::error::PlatformError::InvalidParameter(
                "mode is null".into(),
            ));
        }

        let mode = unsafe { *mode };

        graphics::with_graphics(|state| {
            // Would need to recreate texture at new size
            state.display_mode = mode;
            let buffer_size = (mode.width * mode.height) as usize;
            state.back_buffer.resize(buffer_size, 0);
            state.front_buffer.resize(buffer_size, 0);
        });

        Ok(())
    })
}

/// Get current display mode
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

// =============================================================================
// Surface Operations
// =============================================================================

/// Create a new surface
#[no_mangle]
pub extern "C" fn Platform_Surface_Create(width: i32, height: i32, bpp: i32) -> *mut PlatformSurface {
    catch_panic_option(|| {
        let surface = Surface::new(width, height, bpp);
        Some(Box::into_raw(Box::new(PlatformSurface(surface))))
    })
    .unwrap_or(std::ptr::null_mut())
}

/// Destroy a surface
#[no_mangle]
pub extern "C" fn Platform_Surface_Destroy(surface: *mut PlatformSurface) {
    if !surface.is_null() {
        unsafe {
            drop(Box::from_raw(surface));
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
    catch_panic(|| {
        if surface.is_null() || pixels.is_null() || pitch.is_null() {
            return Err(crate::error::PlatformError::InvalidParameter(
                "null pointer".into(),
            ));
        }

        unsafe {
            let surface = &mut (*surface).0;
            let (ptr, p) = surface.lock()?;
            *pixels = ptr as *mut c_void;
            *pitch = p;
        }

        Ok(())
    })
}

/// Unlock surface
#[no_mangle]
pub extern "C" fn Platform_Surface_Unlock(surface: *mut PlatformSurface) {
    if !surface.is_null() {
        unsafe {
            (*surface).0.unlock();
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
    catch_panic(|| {
        if dest.is_null() || src.is_null() {
            return Err(crate::error::PlatformError::InvalidParameter(
                "null pointer".into(),
            ));
        }

        unsafe {
            let dest = &mut (*dest).0;
            let src = &(*src).0;
            dest.blit(dx, dy, src, sx, sy, sw, sh)?;
        }

        Ok(())
    })
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
            (*surface).0.fill(x, y, w, h, color);
        }
    }
}

/// Clear entire surface
#[no_mangle]
pub extern "C" fn Platform_Surface_Clear(surface: *mut PlatformSurface, color: u8) {
    if !surface.is_null() {
        unsafe {
            (*surface).0.clear(color);
        }
    }
}

// =============================================================================
// Back Buffer Access
// =============================================================================

/// Get pointer to back buffer
#[no_mangle]
pub extern "C" fn Platform_Graphics_GetBackBuffer() -> *mut PlatformSurface {
    // Return a wrapper around the internal back buffer
    // This is a special case - we don't own this memory
    std::ptr::null_mut() // TODO: Implement proper back buffer access
}

// =============================================================================
// Palette
// =============================================================================

/// Set palette entries
#[no_mangle]
pub extern "C" fn Platform_Graphics_SetPalette(
    palette: *const PaletteEntry,
    start: i32,
    count: i32,
) {
    if palette.is_null() || start < 0 || count <= 0 || start + count > 256 {
        return;
    }

    graphics::with_graphics(|state| {
        unsafe {
            let entries = std::slice::from_raw_parts(palette, count as usize);
            state.palette[start as usize..(start + count) as usize].copy_from_slice(entries);
            state.rebuild_palette_lut();
        }
    });
}

/// Get palette entries
#[no_mangle]
pub extern "C" fn Platform_Graphics_GetPalette(
    palette: *mut PaletteEntry,
    start: i32,
    count: i32,
) {
    if palette.is_null() || start < 0 || count <= 0 || start + count > 256 {
        return;
    }

    graphics::with_graphics(|state| {
        unsafe {
            let dest = std::slice::from_raw_parts_mut(palette, count as usize);
            dest.copy_from_slice(&state.palette[start as usize..(start + count) as usize]);
        }
    });
}

// =============================================================================
// Page Flipping
// =============================================================================

/// Flip buffers and present
#[no_mangle]
pub extern "C" fn Platform_Graphics_Flip() {
    graphics::with_graphics(|state| {
        if let Err(e) = state.flip() {
            log::error!("Flip failed: {}", e);
        }
    });
}

/// Wait for vertical sync (no-op with SDL VSync enabled)
#[no_mangle]
pub extern "C" fn Platform_Graphics_WaitVSync() {
    // VSync is handled by SDL_RENDERER_PRESENTVSYNC
}

// =============================================================================
// Video Memory Info
// =============================================================================

/// Get free video memory (returns large value for compatibility)
#[no_mangle]
pub extern "C" fn Platform_Graphics_GetFreeVideoMemory() -> u32 {
    256 * 1024 * 1024 // 256 MB - plenty for this game
}

/// Check if hardware accelerated
#[no_mangle]
pub extern "C" fn Platform_Graphics_IsHardwareAccelerated() -> bool {
    true // SDL2 renderer is always accelerated on modern systems
}
```

## Tasks Breakdown

### Phase 1: Basic Window (2 days)
- [ ] Implement `GraphicsState::new()` with SDL2 window
- [ ] Create canvas with hardware acceleration
- [ ] Display solid color to verify rendering
- [ ] Handle window close event

### Phase 2: 8-bit Rendering (3 days)
- [ ] Implement back buffer management
- [ ] Implement palette-to-RGBA conversion
- [ ] Implement `flip()` with texture streaming
- [ ] Test with gradient pattern

### Phase 3: Surface Operations (2 days)
- [ ] Implement `Surface` struct
- [ ] Implement Lock/Unlock with proper error handling
- [ ] Implement Blit with clipping
- [ ] Implement Fill/Clear

### Phase 4: FFI Exports (2 days)
- [ ] Create all FFI wrapper functions
- [ ] Ensure cbindgen generates correct headers
- [ ] Test calling from C++
- [ ] Verify panic safety

### Phase 5: Integration (2 days)
- [ ] Hook up to game's display initialization
- [ ] Debug palette loading
- [ ] Verify main menu renders
- [ ] Test resolution switching

## Testing Strategy

### Test 1: Color Bars
Display 256 vertical color bars using current palette.

### Test 2: Palette Animation
Cycle palette entries to verify palette updates work.

### Test 3: Blit Test
Draw checkerboard pattern using blit operations.

### Test 4: Game Integration
Load and display the main menu background.

## Acceptance Criteria

- [ ] Window opens at correct resolution
- [ ] 8-bit palettized graphics render correctly
- [ ] Palette changes reflected immediately
- [ ] Page flipping works (no tearing with VSync)
- [ ] Basic blit operations work
- [ ] All FFI functions exported correctly
- [ ] No panics cross FFI boundary

## Estimated Duration
**7-9 days**

## Dependencies
- Plan 01 (Build System) completed
- Plan 02 (Platform Abstraction) headers generated
- sdl2 crate with bundled feature

## Next Plan
Once graphics are working, proceed to **Plan 04: Input System**
