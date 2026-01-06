# Task 03c: Texture Streaming

## Dependencies
- Task 03b must be complete (Surface management)
- SDL2 window and canvas in place

## Context
SDL2 uses textures for GPU-accelerated rendering. We need a streaming texture that can be updated every frame with the converted pixel data from our 8-bit back buffer. This is the bridge between our software rendering and the GPU display.

## Objective
Create texture management for uploading the screen buffer to the GPU each frame.

## Deliverables
- [ ] Add TextureCreator and Texture to GraphicsState
- [ ] Create streaming texture at display resolution
- [ ] Implement texture update method
- [ ] Back buffer storage for 8-bit pixels
- [ ] FFI exports for back buffer access

## Files to Create/Modify

### Update platform/src/graphics/mod.rs
Add texture and back buffer to GraphicsState:
```rust
use sdl2::pixels::PixelFormatEnum;
use sdl2::render::{Canvas, Texture, TextureCreator};
use sdl2::video::{Window, WindowContext};

pub struct GraphicsState {
    pub canvas: Canvas<Window>,
    pub texture_creator: TextureCreator<WindowContext>,
    pub screen_texture: Texture,
    pub display_mode: DisplayMode,
    pub back_buffer: Vec<u8>,
}

impl GraphicsState {
    pub fn new(sdl: &Sdl, mode: DisplayMode) -> Result<Self, PlatformError> {
        let video = sdl.video().map_err(|e| PlatformError::Sdl(e.to_string()))?;

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

        let buffer_size = (mode.width * mode.height) as usize;

        Ok(Self {
            canvas,
            texture_creator,
            screen_texture,
            display_mode: mode,
            back_buffer: vec![0; buffer_size],
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

    /// Update texture with 32-bit ARGB data
    /// Called by the palette conversion step
    pub fn update_texture(&mut self, argb_data: &[u32]) -> Result<(), PlatformError> {
        let width = self.display_mode.width as usize;
        let height = self.display_mode.height as usize;

        self.screen_texture
            .with_lock(None, |pixels: &mut [u8], pitch: usize| {
                for y in 0..height {
                    let src_row = &argb_data[y * width..(y + 1) * width];
                    let dst_row = &mut pixels[y * pitch..];

                    for x in 0..width {
                        let color = src_row[x];
                        let offset = x * 4;
                        // ARGB8888 in SDL memory layout (may be BGRA on little-endian)
                        dst_row[offset] = (color & 0xFF) as u8;           // B
                        dst_row[offset + 1] = ((color >> 8) & 0xFF) as u8;  // G
                        dst_row[offset + 2] = ((color >> 16) & 0xFF) as u8; // R
                        dst_row[offset + 3] = ((color >> 24) & 0xFF) as u8; // A
                    }
                }
            })
            .map_err(|e| PlatformError::Graphics(e))?;

        Ok(())
    }

    /// Render texture to screen
    pub fn present_texture(&mut self) -> Result<(), PlatformError> {
        self.canvas.clear();
        self.canvas
            .copy(&self.screen_texture, None, None)
            .map_err(|e| PlatformError::Graphics(e))?;
        self.canvas.present();
        Ok(())
    }
}
```

### Add FFI exports to platform/src/ffi/mod.rs
```rust
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

/// Get display mode dimensions
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
```

## Verification Command
```bash
cd /path/to/repo && \
cd platform && cargo build --features cbindgen-run 2>&1 && \
grep -q "Platform_Graphics_GetBackBuffer" ../include/platform.h && \
grep -q "Platform_Graphics_ClearBackBuffer" ../include/platform.h && \
echo "VERIFY_SUCCESS"
```

## Implementation Steps
1. Update GraphicsState with texture_creator, screen_texture, back_buffer
2. Implement get_back_buffer(), clear_back_buffer(), update_texture()
3. Add FFI exports for back buffer access
4. Run `cargo build --features cbindgen-run`
5. Verify header contains back buffer exports
6. If any step fails, analyze error and fix

## Expected Header Additions
```c
int32_t Platform_Graphics_GetBackBuffer(uint8_t **pixels, int32_t *width, int32_t *height, int32_t *pitch);
void Platform_Graphics_ClearBackBuffer(uint8_t color);
void Platform_Graphics_GetMode(struct DisplayMode *mode);
```

## Common Issues

### "texture_creator does not live long enough"
- TextureCreator must be stored in GraphicsState, not created temporarily
- Texture lifetime is tied to TextureCreator

### "cannot move out of borrowed content"
- Use `canvas.texture_creator()` before moving canvas into struct
- Store TextureCreator separately

### Texture update is slow
- Use streaming texture (create_texture_streaming)
- Avoid creating new textures each frame

## Completion Promise
When verification passes (header contains exports), output:
<promise>TASK_03C_COMPLETE</promise>

## Escape Hatch
If stuck after 12 iterations:
- Document blocking issue in `ralph-tasks/blocked/03c.md`
- Output: <promise>TASK_03C_BLOCKED</promise>

## Max Iterations
12
