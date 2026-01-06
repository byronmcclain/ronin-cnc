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
