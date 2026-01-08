//! PAL palette file loading
//!
//! Red Alert uses 256-color indexed palettes stored in PAL files.
//! Each palette entry is 3 bytes (R, G, B) with 6-bit color values (0-63).
//!
//! ## PAL File Format
//! - 768 bytes total (256 entries × 3 bytes)
//! - Each byte is 0-63 (must be shifted left 2 bits for 8-bit color)

use std::io::{self, Read};

/// RGB palette entry
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
}

/// Error type for palette operations
#[derive(Debug)]
pub enum PaletteError {
    IoError(io::Error),
    InvalidSize(usize),
}

impl From<io::Error> for PaletteError {
    fn from(e: io::Error) -> Self {
        PaletteError::IoError(e)
    }
}

impl std::fmt::Display for PaletteError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            PaletteError::IoError(e) => write!(f, "IO error: {}", e),
            PaletteError::InvalidSize(size) => {
                write!(f, "Invalid palette size: {} (expected 768)", size)
            }
        }
    }
}

impl std::error::Error for PaletteError {}

/// A 256-color palette loaded from PAL file
#[derive(Clone)]
pub struct Palette {
    /// 256 RGB entries
    pub entries: [PaletteEntry; 256],
}

impl Palette {
    /// Create a new black palette
    pub fn new() -> Self {
        Palette {
            entries: [PaletteEntry::new(0, 0, 0); 256],
        }
    }

    /// Load palette from raw PAL data
    ///
    /// PAL files use 6-bit color values (0-63), which are shifted to 8-bit.
    pub fn from_pal_data(data: &[u8]) -> Result<Self, PaletteError> {
        if data.len() != 768 {
            return Err(PaletteError::InvalidSize(data.len()));
        }

        let mut palette = Palette::new();

        for i in 0..256 {
            let offset = i * 3;
            // PAL uses 6-bit values, shift to 8-bit
            palette.entries[i] = PaletteEntry::new(
                data[offset] << 2,
                data[offset + 1] << 2,
                data[offset + 2] << 2,
            );
        }

        Ok(palette)
    }

    /// Load palette from a reader
    pub fn read<R: Read>(reader: &mut R) -> Result<Self, PaletteError> {
        let mut data = [0u8; 768];
        reader.read_exact(&mut data)?;
        Self::from_pal_data(&data)
    }

    /// Load palette from raw 8-bit RGB data (no shift needed)
    pub fn from_rgb_data(data: &[u8]) -> Result<Self, PaletteError> {
        if data.len() != 768 {
            return Err(PaletteError::InvalidSize(data.len()));
        }

        let mut palette = Palette::new();

        for i in 0..256 {
            let offset = i * 3;
            palette.entries[i] = PaletteEntry::new(
                data[offset],
                data[offset + 1],
                data[offset + 2],
            );
        }

        Ok(palette)
    }

    /// Get a palette entry
    pub fn get(&self, index: u8) -> PaletteEntry {
        self.entries[index as usize]
    }

    /// Set a palette entry
    pub fn set(&mut self, index: u8, entry: PaletteEntry) {
        self.entries[index as usize] = entry;
    }

    /// Get slice of entries
    pub fn as_slice(&self) -> &[PaletteEntry] {
        &self.entries
    }

    /// Get mutable slice of entries
    pub fn as_mut_slice(&mut self) -> &mut [PaletteEntry] {
        &mut self.entries
    }

    /// Convert to raw RGB bytes (8-bit per component)
    pub fn to_rgb_bytes(&self) -> [u8; 768] {
        let mut data = [0u8; 768];
        for (i, entry) in self.entries.iter().enumerate() {
            data[i * 3] = entry.r;
            data[i * 3 + 1] = entry.g;
            data[i * 3 + 2] = entry.b;
        }
        data
    }

    /// Generate a grayscale ramp palette
    pub fn grayscale() -> Self {
        let mut palette = Palette::new();
        for i in 0..256 {
            let v = i as u8;
            palette.entries[i] = PaletteEntry::new(v, v, v);
        }
        palette
    }

    /// Create a copy with faded colors
    pub fn faded(&self, factor: f32) -> Self {
        let mut palette = self.clone();
        for entry in &mut palette.entries {
            entry.r = (entry.r as f32 * factor) as u8;
            entry.g = (entry.g as f32 * factor) as u8;
            entry.b = (entry.b as f32 * factor) as u8;
        }
        palette
    }
}

impl Default for Palette {
    fn default() -> Self {
        Self::new()
    }
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_palette_new() {
        let palette = Palette::new();
        assert_eq!(palette.entries.len(), 256);
        assert_eq!(palette.get(0).r, 0);
        assert_eq!(palette.get(0).g, 0);
        assert_eq!(palette.get(0).b, 0);
    }

    #[test]
    fn test_palette_from_pal_data() {
        // Create test PAL data with 6-bit values
        let mut data = [0u8; 768];
        data[0] = 63;  // R max (63 << 2 = 252)
        data[1] = 0;   // G
        data[2] = 32;  // B half (32 << 2 = 128)

        let palette = Palette::from_pal_data(&data).unwrap();
        assert_eq!(palette.get(0).r, 252);
        assert_eq!(palette.get(0).g, 0);
        assert_eq!(palette.get(0).b, 128);
    }

    #[test]
    fn test_palette_invalid_size() {
        let data = [0u8; 100];
        let result = Palette::from_pal_data(&data);
        assert!(matches!(result, Err(PaletteError::InvalidSize(100))));
    }

    #[test]
    fn test_palette_grayscale() {
        let palette = Palette::grayscale();
        assert_eq!(palette.get(0).r, 0);
        assert_eq!(palette.get(128).r, 128);
        assert_eq!(palette.get(255).r, 255);
    }

    #[test]
    fn test_palette_faded() {
        let mut palette = Palette::new();
        palette.set(0, PaletteEntry::new(100, 100, 100));

        let faded = palette.faded(0.5);
        assert_eq!(faded.get(0).r, 50);
        assert_eq!(faded.get(0).g, 50);
        assert_eq!(faded.get(0).b, 50);
    }
}
