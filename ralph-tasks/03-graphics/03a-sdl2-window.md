# Task 03a: SDL2 Window Creation

## Dependencies
- Task 02c must be complete (Phase 02 Platform Abstraction)
- Platform FFI infrastructure in place

## Context
This task sets up SDL2 integration for the graphics subsystem. We need to add the sdl2 crate dependency, create the graphics module structure, and implement basic window creation with hardware-accelerated canvas.

## Objective
Create the graphics module foundation with SDL2 window creation and basic rendering verification.

## Deliverables
- [ ] Add `sdl2` crate dependency to Cargo.toml
- [ ] Create `platform/src/graphics/mod.rs` with GraphicsState
- [ ] Add SDL context management to platform initialization
- [ ] Create window with hardware-accelerated canvas
- [ ] Display solid color to verify rendering works
- [ ] Export basic FFI functions for graphics init/shutdown

## Files to Create/Modify

### Update platform/Cargo.toml
```toml
[dependencies]
once_cell = "1.19"
thiserror = "1.0"
log = "0.4"
sdl2 = { version = "0.36", features = ["bundled"] }
```

### platform/src/graphics/mod.rs (NEW)
```rust
//! Graphics subsystem using SDL2

use crate::error::PlatformError;
use sdl2::pixels::Color;
use sdl2::render::Canvas;
use sdl2::video::Window;
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

/// Global SDL context - must be created once
static SDL_CONTEXT: Lazy<Mutex<Option<Sdl>>> = Lazy::new(|| Mutex::new(None));

/// Global graphics state
static GRAPHICS: Lazy<Mutex<Option<GraphicsState>>> = Lazy::new(|| Mutex::new(None));

pub struct GraphicsState {
    pub canvas: Canvas<Window>,
    pub display_mode: DisplayMode,
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

        Ok(Self {
            canvas,
            display_mode: mode,
        })
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
}

/// Initialize SDL context
pub fn init_sdl() -> Result<(), PlatformError> {
    let sdl = sdl2::init().map_err(|e| PlatformError::Sdl(e.to_string()))?;

    if let Ok(mut guard) = SDL_CONTEXT.lock() {
        *guard = Some(sdl);
    }

    Ok(())
}

/// Get SDL context reference
pub fn with_sdl<F, R>(f: F) -> Option<R>
where
    F: FnOnce(&Sdl) -> R,
{
    SDL_CONTEXT.lock().ok().and_then(|guard| guard.as_ref().map(f))
}

/// Initialize graphics subsystem
pub fn init() -> Result<(), PlatformError> {
    let mode = DisplayMode::default();

    let state = with_sdl(|sdl| GraphicsState::new(sdl, mode))
        .ok_or_else(|| PlatformError::NotInitialized)??;

    if let Ok(mut guard) = GRAPHICS.lock() {
        *guard = Some(state);
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
    GRAPHICS.lock().ok().and_then(|mut guard| guard.as_mut().map(f))
}

/// Check if graphics is initialized
pub fn is_initialized() -> bool {
    GRAPHICS.lock().ok().map(|g| g.is_some()).unwrap_or(false)
}
```

### Update platform/src/lib.rs
Add after `pub mod ffi;`:
```rust
pub mod graphics;
```

### Update platform/src/ffi/mod.rs
Add graphics FFI functions:
```rust
use crate::graphics;

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
```

### Update Platform_Init in ffi/mod.rs
Modify Platform_Init to also initialize SDL and graphics:
```rust
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
```

## Verification Command
```bash
cd /path/to/repo && \
cd platform && cargo build --features cbindgen-run 2>&1 && \
grep -q "Platform_Graphics_Init" ../include/platform.h && \
grep -q "DisplayMode" ../include/platform.h && \
echo "VERIFY_SUCCESS"
```

## Implementation Steps
1. Update `platform/Cargo.toml` with sdl2 dependency
2. Create `platform/src/graphics/mod.rs`
3. Add `pub mod graphics;` to lib.rs
4. Add graphics FFI functions to ffi/mod.rs
5. Update Platform_Init to initialize SDL
6. Run `cargo build --features cbindgen-run`
7. Verify header contains graphics exports
8. If any step fails, analyze error and fix

## Expected Header Additions
```c
typedef struct DisplayMode {
    int32_t width;
    int32_t height;
    int32_t bits_per_pixel;
} DisplayMode;

int32_t Platform_Graphics_Init(void);
void Platform_Graphics_Shutdown(void);
bool Platform_Graphics_IsInitialized(void);
void Platform_Graphics_Clear(uint8_t r, uint8_t g, uint8_t b);
void Platform_Graphics_Present(void);
```

## Common Issues

### "SDL2 not found" or linking errors
- Ensure `features = ["bundled"]` is in Cargo.toml
- The bundled feature compiles SDL2 from source

### "video subsystem already initialized"
- SDL context should only be created once
- Use the global SDL_CONTEXT pattern

### Window doesn't appear
- Ensure Platform_Init is called before Platform_Graphics_Init
- Check for error messages in Platform_GetErrorString

## Completion Promise
When verification passes (header generated with graphics exports), output:
<promise>TASK_03A_COMPLETE</promise>

## Escape Hatch
If stuck after 15 iterations:
- Document blocking issue in `ralph-tasks/blocked/03a.md`
- Output: <promise>TASK_03A_BLOCKED</promise>

## Max Iterations
15
