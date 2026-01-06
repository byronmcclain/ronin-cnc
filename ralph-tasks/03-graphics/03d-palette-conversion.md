# Task 03d: Palette Conversion

## Dependencies
- Task 03c must be complete (Texture streaming)
- Back buffer and texture infrastructure in place

## Context
Red Alert uses 8-bit palettized graphics with 256 color palettes. We need to maintain a 256-entry palette and provide a fast lookup table for converting 8-bit indices to 32-bit ARGB. The game frequently changes palettes for effects (fading, highlighting, cycling).

## Objective
Implement palette management with set/get operations, an optimized ARGB lookup table, and the conversion routine that transforms the 8-bit back buffer to 32-bit for texture upload.

## Deliverables
- [ ] `platform/src/graphics/palette.rs` - Palette management
- [ ] PaletteEntry struct for RGB values
- [ ] 256-entry ARGB lookup table
- [ ] Convert 8-bit buffer to 32-bit using LUT
- [ ] Fade/restore palette effects
- [ ] FFI exports for palette operations

## Files to Create/Modify

### platform/src/graphics/palette.rs (NEW)
```rust
//! Palette management for 8-bit indexed graphics

/// RGB palette entry (matches game format)
#[repr(C)]
#[derive(Clone, Copy, Default, Debug, PartialEq)]
pub struct PaletteEntry {
    pub r: u8,
    pub g: u8,
    pub b: u8,
}

impl PaletteEntry {
    pub fn new(r: u8, g: u8, b: u8) -> Self {
        Self { r, g, b }
    }

    /// Convert to ARGB8888 format (alpha = 255)
    pub fn to_argb(&self) -> u32 {
        0xFF000000 | ((self.r as u32) << 16) | ((self.g as u32) << 8) | (self.b as u32)
    }
}

/// 256-color palette with ARGB lookup table
pub struct Palette {
    /// The 256 palette entries
    entries: [PaletteEntry; 256],
    /// Pre-computed ARGB values for fast lookup
    argb_lut: [u32; 256],
    /// Flag indicating LUT needs rebuild
    dirty: bool,
}

impl Default for Palette {
    fn default() -> Self {
        Self::new()
    }
}

impl Palette {
    /// Create a new palette initialized to grayscale
    pub fn new() -> Self {
        let mut palette = Self {
            entries: [PaletteEntry::default(); 256],
            argb_lut: [0; 256],
            dirty: true,
        };

        // Initialize with grayscale palette
        for i in 0..256 {
            palette.entries[i] = PaletteEntry::new(i as u8, i as u8, i as u8);
        }
        palette.rebuild_lut();

        palette
    }

    /// Set a single palette entry
    pub fn set(&mut self, index: u8, entry: PaletteEntry) {
        self.entries[index as usize] = entry;
        self.dirty = true;
    }

    /// Get a single palette entry
    pub fn get(&self, index: u8) -> PaletteEntry {
        self.entries[index as usize]
    }

    /// Set multiple palette entries starting at index
    pub fn set_range(&mut self, start: usize, entries: &[PaletteEntry]) {
        let end = (start + entries.len()).min(256);
        let count = end - start;
        self.entries[start..end].copy_from_slice(&entries[..count]);
        self.dirty = true;
    }

    /// Get multiple palette entries
    pub fn get_range(&self, start: usize, count: usize) -> &[PaletteEntry] {
        let end = (start + count).min(256);
        &self.entries[start..end]
    }

    /// Rebuild the ARGB lookup table
    pub fn rebuild_lut(&mut self) {
        for i in 0..256 {
            self.argb_lut[i] = self.entries[i].to_argb();
        }
        self.dirty = false;
    }

    /// Ensure LUT is up to date
    pub fn ensure_lut(&mut self) {
        if self.dirty {
            self.rebuild_lut();
        }
    }

    /// Get reference to ARGB lookup table
    pub fn get_lut(&self) -> &[u32; 256] {
        &self.argb_lut
    }

    /// Convert 8-bit indexed buffer to 32-bit ARGB
    pub fn convert_buffer(&mut self, src: &[u8], dst: &mut [u32]) {
        self.ensure_lut();
        let lut = &self.argb_lut;

        for (i, &index) in src.iter().enumerate() {
            if i < dst.len() {
                dst[i] = lut[index as usize];
            }
        }
    }

    /// Fade palette towards black (factor 0.0 = black, 1.0 = full color)
    pub fn fade(&mut self, factor: f32) {
        let factor = factor.clamp(0.0, 1.0);
        for i in 0..256 {
            let entry = &self.entries[i];
            self.argb_lut[i] = PaletteEntry::new(
                (entry.r as f32 * factor) as u8,
                (entry.g as f32 * factor) as u8,
                (entry.b as f32 * factor) as u8,
            ).to_argb();
        }
    }

    /// Restore LUT from entries (after fade)
    pub fn restore(&mut self) {
        self.rebuild_lut();
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_palette_entry_argb() {
        let entry = PaletteEntry::new(255, 128, 64);
        let argb = entry.to_argb();
        assert_eq!(argb, 0xFF_FF_80_40);
    }

    #[test]
    fn test_palette_grayscale() {
        let palette = Palette::new();
        assert_eq!(palette.get(0), PaletteEntry::new(0, 0, 0));
        assert_eq!(palette.get(255), PaletteEntry::new(255, 255, 255));
    }

    #[test]
    fn test_palette_convert() {
        let mut palette = Palette::new();
        palette.set(10, PaletteEntry::new(255, 0, 0)); // Red

        let src = [0u8, 10, 255];
        let mut dst = [0u32; 3];
        palette.convert_buffer(&src, &mut dst);

        assert_eq!(dst[0], 0xFF_00_00_00); // Black
        assert_eq!(dst[1], 0xFF_FF_00_00); // Red
        assert_eq!(dst[2], 0xFF_FF_FF_FF); // White
    }

    #[test]
    fn test_palette_fade() {
        let mut palette = Palette::new();
        palette.set(100, PaletteEntry::new(200, 100, 50));
        palette.ensure_lut();

        palette.fade(0.5);
        // After 50% fade: (100, 50, 25)
        assert_eq!(palette.get_lut()[100], 0xFF_64_32_19);

        palette.restore();
        assert_eq!(palette.get_lut()[100], 0xFF_C8_64_32);
    }
}
```

### Update platform/src/graphics/mod.rs
Add at top:
```rust
pub mod palette;
pub use palette::{Palette, PaletteEntry};
```

Add palette and conversion buffer to GraphicsState:
```rust
pub struct GraphicsState {
    pub canvas: Canvas<Window>,
    pub texture_creator: TextureCreator<WindowContext>,
    pub screen_texture: Texture,
    pub display_mode: DisplayMode,
    pub palette: Palette,
    pub back_buffer: Vec<u8>,
    pub argb_buffer: Vec<u32>,  // Converted 32-bit buffer
}

impl GraphicsState {
    pub fn new(sdl: &Sdl, mode: DisplayMode) -> Result<Self, PlatformError> {
        // ... existing window/canvas/texture creation ...

        let buffer_size = (mode.width * mode.height) as usize;

        Ok(Self {
            canvas,
            texture_creator,
            screen_texture,
            display_mode: mode,
            palette: Palette::new(),
            back_buffer: vec![0; buffer_size],
            argb_buffer: vec![0; buffer_size],
        })
    }

    /// Convert back buffer using palette and store in argb_buffer
    pub fn convert_back_buffer(&mut self) {
        self.palette.convert_buffer(&self.back_buffer, &mut self.argb_buffer);
    }
}
```

### Add FFI exports to platform/src/ffi/mod.rs
```rust
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
```

## Verification Command
```bash
cd /path/to/repo && \
cd platform && cargo test 2>&1 && \
cargo build --features cbindgen-run 2>&1 && \
grep -q "PaletteEntry" ../include/platform.h && \
grep -q "Platform_Graphics_SetPalette" ../include/platform.h && \
grep -q "Platform_Graphics_FadePalette" ../include/platform.h && \
echo "VERIFY_SUCCESS"
```

## Implementation Steps
1. Create `platform/src/graphics/palette.rs`
2. Update `platform/src/graphics/mod.rs` to include palette and argb_buffer
3. Add palette FFI exports to `platform/src/ffi/mod.rs`
4. Run `cargo test` to verify palette tests pass
5. Run `cargo build --features cbindgen-run`
6. Verify header contains palette exports
7. If any step fails, analyze error and fix

## Expected Header Additions
```c
typedef struct PaletteEntry {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} PaletteEntry;

void Platform_Graphics_SetPalette(const struct PaletteEntry *entries, int32_t start, int32_t count);
void Platform_Graphics_GetPalette(struct PaletteEntry *entries, int32_t start, int32_t count);
void Platform_Graphics_SetPaletteEntry(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void Platform_Graphics_FadePalette(float factor);
void Platform_Graphics_RestorePalette(void);
```

## Common Issues

### Palette colors look wrong
- Check byte order: ARGB vs BGRA vs RGBA
- Original game uses 6-bit VGA (0-63), may need `<< 2` scaling

### LUT not updated after palette change
- Call `palette.ensure_lut()` before conversion
- Check `dirty` flag is set on modifications

### Conversion is slow
- Ensure LUT is pre-built, not computed per-pixel
- Use direct array indexing, not function calls

## Completion Promise
When verification passes (tests pass and header contains exports), output:
<promise>TASK_03D_COMPLETE</promise>

## Escape Hatch
If stuck after 12 iterations:
- Document blocking issue in `ralph-tasks/blocked/03d.md`
- Output: <promise>TASK_03D_BLOCKED</promise>

## Max Iterations
12
