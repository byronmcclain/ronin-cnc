//! Template (tileset) file loading
//!
//! Template files contain tileset graphics for terrain rendering.
//! Red Alert uses .TMP files for terrain tiles.
//!
//! ## Template File Format
//!
//! ### Header (Variable size based on format)
//! Different terrain sets use different header formats.
//!
//! ### Common Format
//! ```text
//! width:       u16   - Tile width (usually 24)
//! height:      u16   - Tile height (usually 24)
//! tile_count:  u16   - Number of tiles
//! reserved:    u16   - Usually 0
//! ```
//!
//! ### Icon Data
//! Each tile is width * height bytes of 8-bit indexed pixels.
//! Tiles may be compressed with LCW compression.

use std::io::{self, Read};

use crate::compression::lcw;

/// Error type for template operations
#[derive(Debug)]
pub enum TemplateError {
    IoError(io::Error),
    InvalidHeader,
    InvalidTile(usize),
    DecompressionFailed,
}

impl From<io::Error> for TemplateError {
    fn from(e: io::Error) -> Self {
        TemplateError::IoError(e)
    }
}

impl std::fmt::Display for TemplateError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            TemplateError::IoError(e) => write!(f, "IO error: {}", e),
            TemplateError::InvalidHeader => write!(f, "Invalid template header"),
            TemplateError::InvalidTile(n) => write!(f, "Invalid tile: {}", n),
            TemplateError::DecompressionFailed => write!(f, "Decompression failed"),
        }
    }
}

impl std::error::Error for TemplateError {}

/// A single template tile
#[derive(Debug, Clone)]
pub struct TemplateTile {
    /// Tile width
    pub width: u16,
    /// Tile height
    pub height: u16,
    /// Pixel data (8-bit indexed, row-major)
    pub pixels: Vec<u8>,
}

impl TemplateTile {
    /// Create an empty tile
    pub fn empty(width: u16, height: u16) -> Self {
        let size = width as usize * height as usize;
        TemplateTile {
            width,
            height,
            pixels: vec![0; size],
        }
    }

    /// Get pixel at (x, y)
    pub fn get_pixel(&self, x: u16, y: u16) -> u8 {
        if x >= self.width || y >= self.height {
            return 0;
        }
        let idx = y as usize * self.width as usize + x as usize;
        self.pixels.get(idx).copied().unwrap_or(0)
    }

    /// Get row pitch (bytes per row)
    pub fn pitch(&self) -> usize {
        self.width as usize
    }

    /// Get total size in bytes
    pub fn size(&self) -> usize {
        self.width as usize * self.height as usize
    }
}

/// A template file containing multiple tiles
pub struct TemplateFile {
    /// Tile width (usually 24 for RA)
    pub tile_width: u16,
    /// Tile height (usually 24 for RA)
    pub tile_height: u16,
    /// All tiles in the template
    tiles: Vec<TemplateTile>,
}

impl TemplateFile {
    /// Load template from raw data (simple format)
    ///
    /// This handles the basic template format where tiles are stored sequentially.
    pub fn from_data(data: &[u8], tile_width: u16, tile_height: u16) -> Result<Self, TemplateError> {
        if data.len() < 8 {
            return Err(TemplateError::InvalidHeader);
        }

        let tile_size = tile_width as usize * tile_height as usize;
        if tile_size == 0 {
            return Err(TemplateError::InvalidHeader);
        }

        // Try to detect format - check for icon header
        let header_width = u16::from_le_bytes([data[0], data[1]]);
        let header_height = u16::from_le_bytes([data[2], data[3]]);
        let tile_count = u16::from_le_bytes([data[4], data[5]]);

        let mut offset = 0;
        let actual_tile_count;

        // Check if this looks like a valid header
        if header_width == tile_width && header_height == tile_height && tile_count > 0 {
            // Use header info
            actual_tile_count = tile_count as usize;
            offset = 8; // Skip header

            // Check if we have offset table
            if data.len() >= offset + actual_tile_count * 4 {
                // Has offset table - skip it for now, read sequentially
                offset += actual_tile_count * 4;
            }
        } else {
            // No header, calculate based on data size
            actual_tile_count = data.len() / tile_size;
        }

        if actual_tile_count == 0 {
            return Err(TemplateError::InvalidHeader);
        }

        let mut tiles = Vec::with_capacity(actual_tile_count);

        for i in 0..actual_tile_count {
            let start = offset + i * tile_size;
            let end = start + tile_size;

            if end > data.len() {
                // Remaining tiles are truncated, stop here
                break;
            }

            tiles.push(TemplateTile {
                width: tile_width,
                height: tile_height,
                pixels: data[start..end].to_vec(),
            });
        }

        if tiles.is_empty() {
            return Err(TemplateError::InvalidHeader);
        }

        Ok(TemplateFile {
            tile_width,
            tile_height,
            tiles,
        })
    }

    /// Load template from data with LCW decompression
    pub fn from_compressed_data(
        data: &[u8],
        tile_width: u16,
        tile_height: u16,
        tile_count: usize,
    ) -> Result<Self, TemplateError> {
        let tile_size = tile_width as usize * tile_height as usize;
        let total_size = tile_size * tile_count;

        // Decompress
        let mut output = vec![0u8; total_size];
        match lcw::lcw_decompress(data, &mut output) {
            Ok(actual_size) => {
                if actual_size < tile_size {
                    return Err(TemplateError::DecompressionFailed);
                }
                output.truncate(actual_size);
            }
            Err(_) => return Err(TemplateError::DecompressionFailed),
        }

        // Parse into tiles
        let actual_count = output.len() / tile_size;
        let mut tiles = Vec::with_capacity(actual_count);

        for i in 0..actual_count {
            let start = i * tile_size;
            let end = start + tile_size;

            if end > output.len() {
                break;
            }

            tiles.push(TemplateTile {
                width: tile_width,
                height: tile_height,
                pixels: output[start..end].to_vec(),
            });
        }

        Ok(TemplateFile {
            tile_width,
            tile_height,
            tiles,
        })
    }

    /// Load template from a reader
    pub fn read<R: Read>(
        reader: &mut R,
        tile_width: u16,
        tile_height: u16,
    ) -> Result<Self, TemplateError> {
        let mut data = Vec::new();
        reader.read_to_end(&mut data)?;
        Self::from_data(&data, tile_width, tile_height)
    }

    /// Get number of tiles
    pub fn tile_count(&self) -> usize {
        self.tiles.len()
    }

    /// Get a specific tile
    pub fn tile(&self, index: usize) -> Option<&TemplateTile> {
        self.tiles.get(index)
    }

    /// Iterate over all tiles
    pub fn tiles(&self) -> impl Iterator<Item = &TemplateTile> {
        self.tiles.iter()
    }

    /// Get tile dimensions
    pub fn tile_dimensions(&self) -> (u16, u16) {
        (self.tile_width, self.tile_height)
    }
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_template_tile_empty() {
        let tile = TemplateTile::empty(24, 24);
        assert_eq!(tile.width, 24);
        assert_eq!(tile.height, 24);
        assert_eq!(tile.pixels.len(), 576);
        assert_eq!(tile.get_pixel(0, 0), 0);
    }

    #[test]
    fn test_template_tile_pixel_access() {
        let mut tile = TemplateTile::empty(8, 8);
        tile.pixels[10] = 42;
        assert_eq!(tile.get_pixel(2, 1), 42); // x=2, y=1 -> 1*8+2 = 10
    }

    #[test]
    fn test_template_from_data() {
        // Create 2 tiles of 4x4
        let mut data = vec![0u8; 32];
        // Fill first tile with 1s
        for i in 0..16 {
            data[i] = 1;
        }
        // Fill second tile with 2s
        for i in 16..32 {
            data[i] = 2;
        }

        let template = TemplateFile::from_data(&data, 4, 4).unwrap();
        assert_eq!(template.tile_count(), 2);
        assert_eq!(template.tile(0).unwrap().get_pixel(0, 0), 1);
        assert_eq!(template.tile(1).unwrap().get_pixel(0, 0), 2);
    }

    #[test]
    fn test_template_invalid() {
        let data = [0u8; 4]; // Too short
        let result = TemplateFile::from_data(&data, 24, 24);
        assert!(matches!(result, Err(TemplateError::InvalidHeader)));
    }
}
