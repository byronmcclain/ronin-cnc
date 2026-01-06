# Task 03e: Full Render Pipeline

## Dependencies
- Task 03d must be complete (Palette conversion)
- All graphics infrastructure in place (window, surfaces, textures, palette)

## Context
This final task ties together all graphics components into a complete render pipeline. The flip() operation converts the 8-bit back buffer to 32-bit using the palette, uploads to the GPU texture, and presents to the screen. We also add a comprehensive C++ test that validates the entire graphics system.

## Objective
Implement the complete flip/present pipeline and create integration tests that verify the entire graphics system works end-to-end.

## Deliverables
- [ ] Implement flip() that does full 8-bit→32-bit→texture→present
- [ ] VSync waiting (handled by SDL)
- [ ] Update C++ test to display test pattern
- [ ] All graphics FFI functions working
- [ ] Window displays rendered content

## Files to Create/Modify

### Update platform/src/graphics/mod.rs
Add the complete flip implementation:
```rust
impl GraphicsState {
    // ... existing methods ...

    /// Complete flip operation:
    /// 1. Convert 8-bit back buffer to 32-bit ARGB using palette
    /// 2. Upload to streaming texture
    /// 3. Render texture to canvas
    /// 4. Present (with VSync)
    pub fn flip(&mut self) -> Result<(), PlatformError> {
        let width = self.display_mode.width as usize;
        let height = self.display_mode.height as usize;

        // Step 1: Convert back buffer using palette LUT
        self.palette.ensure_lut();
        let lut = self.palette.get_lut();

        // Step 2: Update texture with converted pixels
        {
            let back_buffer = &self.back_buffer;
            self.screen_texture
                .with_lock(None, |pixels: &mut [u8], pitch: usize| {
                    for y in 0..height {
                        let src_row = &back_buffer[y * width..(y + 1) * width];
                        let dst_row = &mut pixels[y * pitch..];

                        for x in 0..width {
                            let argb = lut[src_row[x] as usize];
                            let offset = x * 4;
                            // SDL ARGB8888 memory layout (little-endian: BGRA)
                            dst_row[offset] = (argb & 0xFF) as u8;           // B
                            dst_row[offset + 1] = ((argb >> 8) & 0xFF) as u8;  // G
                            dst_row[offset + 2] = ((argb >> 16) & 0xFF) as u8; // R
                            dst_row[offset + 3] = ((argb >> 24) & 0xFF) as u8; // A
                        }
                    }
                })
                .map_err(|e| PlatformError::Graphics(e))?;
        }

        // Step 3: Clear canvas and render texture (scaled to window)
        self.canvas.set_draw_color(sdl2::pixels::Color::RGB(0, 0, 0));
        self.canvas.clear();
        self.canvas
            .copy(&self.screen_texture, None, None)
            .map_err(|e| PlatformError::Graphics(e))?;

        // Step 4: Present (VSync handled by SDL_RENDERER_PRESENTVSYNC)
        self.canvas.present();

        Ok(())
    }

    /// Draw directly to back buffer - for testing
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
```

### Add FFI exports to platform/src/ffi/mod.rs
```rust
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
```

### Update src/main.cpp - Add comprehensive graphics test
```cpp
#include <cstdio>
#include <cstring>
#include <cassert>
#include "platform.h"

// ... existing test macros and functions ...

int test_graphics() {
    TEST("Graphics system");

    // Initialize graphics (requires Platform_Init first)
    int result = Platform_Graphics_Init();
    if (result != 0) {
        char err[256];
        Platform_GetLastError(reinterpret_cast<int8_t*>(err), sizeof(err));
        printf("SKIP (init failed: %s)\n", err);
        return 0; // Don't fail test if graphics unavailable (CI environment)
    }

    if (!Platform_Graphics_IsInitialized()) {
        Platform_Graphics_Shutdown();
        FAIL("Graphics should be initialized");
    }

    // Check display mode
    DisplayMode mode;
    Platform_Graphics_GetMode(&mode);
    if (mode.width != 640 || mode.height != 400) {
        printf("(mode: %dx%d) ", mode.width, mode.height);
    }

    // Get back buffer
    uint8_t* pixels = nullptr;
    int32_t width, height, pitch;
    result = Platform_Graphics_GetBackBuffer(&pixels, &width, &height, &pitch);
    if (result != 0 || pixels == nullptr) {
        Platform_Graphics_Shutdown();
        FAIL("Failed to get back buffer");
    }

    // Draw test pattern (vertical color bars using grayscale palette)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            pixels[y * pitch + x] = (x * 256 / width) & 0xFF;
        }
    }

    // Flip to display
    result = Platform_Graphics_Flip();
    if (result != 0) {
        char err[256];
        Platform_GetLastError(reinterpret_cast<int8_t*>(err), sizeof(err));
        Platform_Graphics_Shutdown();
        printf("FAIL: Flip failed: %s\n", err);
        return 1;
    }

    // Test palette modification
    PaletteEntry red = {255, 0, 0};
    Platform_Graphics_SetPaletteEntry(128, red.r, red.g, red.b);

    // Draw horizontal line at color 128 (should be red now)
    for (int x = 0; x < width; x++) {
        pixels[200 * pitch + x] = 128;
    }

    // Flip again
    result = Platform_Graphics_Flip();
    if (result != 0) {
        Platform_Graphics_Shutdown();
        FAIL("Second flip failed");
    }

    // Cleanup
    Platform_Graphics_Shutdown();

    if (Platform_Graphics_IsInitialized()) {
        FAIL("Graphics should be shut down");
    }

    printf("(buffer: %dx%d, flip OK) ", width, height);
    PASS();
    return 0;
}

int main(int argc, char* argv[]) {
    bool test_mode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--test-init") == 0 ||
            strcmp(argv[i], "--test") == 0) {
            test_mode = true;
        }
    }

    printf("=== Platform Layer Compatibility Test ===\n\n");
    printf("Platform version: %d\n\n", Platform_GetVersion());

    int failures = 0;

    // Core tests
    failures += test_version();
    failures += test_init_shutdown();
    failures += test_error_handling();
    failures += test_logging();

    // Graphics test (requires Platform_Init for SDL context)
    {
        // Re-init for graphics test
        Platform_Init();
        failures += test_graphics();
        Platform_Shutdown();
    }

    // Final init/shutdown for normal operation test
    printf("\n  Final init/shutdown cycle... ");
    if (Platform_Init() == PLATFORM_RESULT_SUCCESS) {
        Platform_Shutdown();
        printf("PASS\n");
    } else {
        printf("FAIL\n");
        failures++;
    }

    printf("\n=== Results ===\n");
    if (failures == 0) {
        printf("All tests passed!\n");
        printf("Test passed: init/shutdown cycle complete\n");
        return 0;
    } else {
        printf("%d test(s) failed\n", failures);
        return 1;
    }
}
```

## Verification Command
```bash
cd /path/to/repo && \
cd platform && cargo build --features cbindgen-run 2>&1 && \
grep -q "Platform_Graphics_Flip" ../include/platform.h && \
grep -q "Platform_Graphics_DrawTestPattern" ../include/platform.h && \
cd .. && cmake --build build 2>&1 && \
./build/RedAlert --test 2>&1 | grep -q "All tests passed" && \
echo "VERIFY_SUCCESS"
```

## Implementation Steps
1. Add flip() implementation to GraphicsState
2. Add draw_test_pattern() for testing
3. Add flip and helper FFI exports
4. Update src/main.cpp with comprehensive graphics test
5. Run `cargo build --features cbindgen-run`
6. Run `cmake --build build`
7. Run `./build/RedAlert --test`
8. Verify window appears briefly with test pattern
9. Verify "All tests passed!" in output
10. If any step fails, analyze error and fix

## Expected Header Additions
```c
int32_t Platform_Graphics_Flip(void);
void Platform_Graphics_WaitVSync(void);
void Platform_Graphics_DrawTestPattern(void);
uint32_t Platform_Graphics_GetFreeVideoMemory(void);
bool Platform_Graphics_IsHardwareAccelerated(void);
```

## Common Issues

### Window appears but is black
- Check palette is initialized (grayscale by default)
- Ensure flip() is called after drawing
- Verify back buffer has non-zero data

### Colors are wrong/inverted
- Check byte order: ARGB constant vs BGRA memory
- SDL ARGB8888 on little-endian is actually BGRA in memory

### Test hangs or crashes
- Ensure graphics shutdown before platform shutdown
- Check for null pointer in with_graphics closure

### Window doesn't appear in CI
- Graphics test should gracefully skip if init fails
- Use SKIP instead of FAIL for headless environments

### Texture upload is slow
- Use streaming texture, not static
- Avoid recreating texture each frame

## Completion Promise
When verification passes (all tests pass including graphics), output:
<promise>TASK_03E_COMPLETE</promise>

## Escape Hatch
If stuck after 15 iterations:
- Document blocking issue in `ralph-tasks/blocked/03e.md`
- Output: <promise>TASK_03E_BLOCKED</promise>

## Max Iterations
15

## Phase 03 Complete
When this task completes, the graphics system is fully operational:
- ✓ SDL2 window with hardware acceleration
- ✓ Software surfaces for off-screen rendering
- ✓ Streaming texture for GPU upload
- ✓ 256-color palette with fast LUT conversion
- ✓ Complete flip pipeline (8-bit → 32-bit → texture → present)

Proceed to **Phase 04: Input System**.
