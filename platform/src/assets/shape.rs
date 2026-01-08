//! SHP shape file loading
//!
//! SHP files contain sprite graphics with multiple frames.
//! Each frame can be compressed using LCW compression.
//!
//! ## SHP File Format
//!
//! ### Header (8 bytes)
//! ```text
//! frame_count: u16   - Number of frames
//! unknown1:    u16   - Usually 0
//! width:       u16   - Frame width
//! height:      u16   - Frame height
//! ```
//!
//! ### Frame Offsets (frame_count * 8 bytes)
//! ```text
//! For each frame:
//!   offset:    u32   - Offset to frame data from file start (0 = empty frame)
//!   format:    u8    - Frame format flags
//!   ref_frame: u8    - Reference frame for XOR delta
//!   unknown:   u16   - Usually 0
//! ```
//!
//! ### Frame Data
//! ```text
//! If format & 0x02: LCW compressed
//! If format & 0x01: XOR with reference frame
//! Raw data is 8-bit indexed pixels (row-major, top to bottom)
//! ```

use std::io::{self, Read, Seek, SeekFrom};

use crate::compression::lcw;

/// Error type for shape operations
#[derive(Debug)]
pub enum ShapeError {
    IoError(io::Error),
    InvalidHeader,
    InvalidFrame(usize),
    DecompressionFailed,
}

impl From<io::Error> for ShapeError {
    fn from(e: io::Error) -> Self {
        ShapeError::IoError(e)
    }
}

impl std::fmt::Display for ShapeError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            ShapeError::IoError(e) => write!(f, "IO error: {}", e),
            ShapeError::InvalidHeader => write!(f, "Invalid SHP header"),
            ShapeError::InvalidFrame(n) => write!(f, "Invalid frame: {}", n),
            ShapeError::DecompressionFailed => write!(f, "LCW decompression failed"),
        }
    }
}

impl std::error::Error for ShapeError {}

/// Frame format flags
pub mod frame_format {
    /// Frame data is XOR'd with reference frame
    pub const XOR_DELTA: u8 = 0x01;
    /// Frame data is LCW compressed
    pub const LCW_COMPRESSED: u8 = 0x02;
}

/// SHP frame offset entry
#[derive(Debug, Clone, Copy)]
pub struct FrameOffset {
    /// Offset to frame data (0 = empty frame)
    pub offset: u32,
    /// Format flags
    pub format: u8,
    /// Reference frame for XOR delta
    pub ref_frame: u8,
}

/// A single shape frame
#[derive(Debug, Clone)]
pub struct ShapeFrame {
    /// Frame width
    pub width: u16,
    /// Frame height
    pub height: u16,
    /// Pixel data (8-bit indexed, row-major)
    pub pixels: Vec<u8>,
}

impl ShapeFrame {
    /// Create an empty frame
    pub fn empty(width: u16, height: u16) -> Self {
        let size = width as usize * height as usize;
        ShapeFrame {
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

    /// Set pixel at (x, y)
    pub fn set_pixel(&mut self, x: u16, y: u16, color: u8) {
        if x < self.width && y < self.height {
            let idx = y as usize * self.width as usize + x as usize;
            if idx < self.pixels.len() {
                self.pixels[idx] = color;
            }
        }
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

/// A shape file containing multiple frames
pub struct ShapeFile {
    /// Frame width
    pub width: u16,
    /// Frame height
    pub height: u16,
    /// Decoded frames
    frames: Vec<ShapeFrame>,
}

impl ShapeFile {
    /// Load shape from raw SHP data
    pub fn from_data(data: &[u8]) -> Result<Self, ShapeError> {
        if data.len() < 8 {
            return Err(ShapeError::InvalidHeader);
        }

        // Read header
        let frame_count = u16::from_le_bytes([data[0], data[1]]) as usize;
        let _unknown1 = u16::from_le_bytes([data[2], data[3]]);
        let width = u16::from_le_bytes([data[4], data[5]]);
        let height = u16::from_le_bytes([data[6], data[7]]);

        if frame_count == 0 || width == 0 || height == 0 {
            return Err(ShapeError::InvalidHeader);
        }

        // Calculate expected header size
        let header_end = 8 + frame_count * 8;
        if data.len() < header_end {
            return Err(ShapeError::InvalidHeader);
        }

        // Read frame offsets
        let mut offsets = Vec::with_capacity(frame_count);
        for i in 0..frame_count {
            let base = 8 + i * 8;
            let offset = u32::from_le_bytes([
                data[base], data[base + 1], data[base + 2], data[base + 3]
            ]);
            let format = data[base + 4];
            let ref_frame = data[base + 5];

            offsets.push(FrameOffset { offset, format, ref_frame });
        }

        // Decode frames
        let frame_size = width as usize * height as usize;
        let mut frames = Vec::with_capacity(frame_count);

        for (frame_idx, frame_offset) in offsets.iter().enumerate() {
            let frame = if frame_offset.offset == 0 {
                // Empty frame
                ShapeFrame::empty(width, height)
            } else {
                // Get frame data slice
                let start = frame_offset.offset as usize;
                if start >= data.len() {
                    return Err(ShapeError::InvalidFrame(frame_idx));
                }

                let frame_data = &data[start..];
                let is_compressed = (frame_offset.format & frame_format::LCW_COMPRESSED) != 0;
                let is_xor = (frame_offset.format & frame_format::XOR_DELTA) != 0;

                let mut pixels = if is_compressed {
                    // LCW decompress
                    let mut output = vec![0u8; frame_size];
                    match lcw::lcw_decompress(frame_data, &mut output) {
                        Ok(_) => output,
                        Err(_) => return Err(ShapeError::DecompressionFailed),
                    }
                } else {
                    // Raw data
                    if frame_data.len() < frame_size {
                        return Err(ShapeError::InvalidFrame(frame_idx));
                    }
                    frame_data[..frame_size].to_vec()
                };

                // Apply XOR delta if needed
                if is_xor && frame_offset.ref_frame < frames.len() as u8 {
                    let ref_pixels = &frames[frame_offset.ref_frame as usize].pixels;
                    for (p, r) in pixels.iter_mut().zip(ref_pixels.iter()) {
                        *p ^= *r;
                    }
                }

                ShapeFrame {
                    width,
                    height,
                    pixels,
                }
            };

            frames.push(frame);
        }

        Ok(ShapeFile {
            width,
            height,
            frames,
        })
    }

    /// Load shape from a reader
    pub fn read<R: Read>(reader: &mut R) -> Result<Self, ShapeError> {
        let mut data = Vec::new();
        reader.read_to_end(&mut data)?;
        Self::from_data(&data)
    }

    /// Get number of frames
    pub fn frame_count(&self) -> usize {
        self.frames.len()
    }

    /// Get a specific frame
    pub fn frame(&self, index: usize) -> Option<&ShapeFrame> {
        self.frames.get(index)
    }

    /// Get mutable reference to a frame
    pub fn frame_mut(&mut self, index: usize) -> Option<&mut ShapeFrame> {
        self.frames.get_mut(index)
    }

    /// Iterate over all frames
    pub fn frames(&self) -> impl Iterator<Item = &ShapeFrame> {
        self.frames.iter()
    }
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_shape_frame_empty() {
        let frame = ShapeFrame::empty(32, 32);
        assert_eq!(frame.width, 32);
        assert_eq!(frame.height, 32);
        assert_eq!(frame.pixels.len(), 1024);
        assert_eq!(frame.get_pixel(0, 0), 0);
    }

    #[test]
    fn test_shape_frame_pixel_access() {
        let mut frame = ShapeFrame::empty(16, 16);
        frame.set_pixel(5, 5, 42);
        assert_eq!(frame.get_pixel(5, 5), 42);
        assert_eq!(frame.get_pixel(0, 0), 0);

        // Out of bounds
        assert_eq!(frame.get_pixel(100, 100), 0);
    }

    #[test]
    fn test_shape_invalid_header() {
        let data = [0u8; 4]; // Too short
        let result = ShapeFile::from_data(&data);
        assert!(matches!(result, Err(ShapeError::InvalidHeader)));
    }

    #[test]
    fn test_shape_zero_frames() {
        let mut data = [0u8; 8];
        // frame_count = 0
        data[0] = 0;
        data[1] = 0;
        // width = 32
        data[4] = 32;
        data[5] = 0;
        // height = 32
        data[6] = 32;
        data[7] = 0;

        let result = ShapeFile::from_data(&data);
        assert!(matches!(result, Err(ShapeError::InvalidHeader)));
    }
}
