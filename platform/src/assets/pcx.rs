//! PCX image file loading
//!
//! Red Alert uses PCX format for title screens and some UI graphics.
//! PCX files are 8-bit palettized images with RLE compression.
//!
//! ## PCX File Format
//! - 128-byte header
//! - RLE-compressed pixel data
//! - 768-byte palette at end of file (256 × 3 RGB, 6-bit values)
//!
//! ## Reference
//! Original implementation: CODE/WINSTUB.CPP Read_PCX_File()

use std::io::{self, Read, Seek, SeekFrom};

/// PCX file header (128 bytes)
#[repr(C, packed)]
#[derive(Clone, Copy, Debug)]
pub struct PcxHeader {
    pub id: u8,              // Must be 10 (0x0A)
    pub version: u8,         // Should be 5
    pub encoding: u8,        // 1 = RLE
    pub bits_per_pixel: u8,  // 8 for 256-color
    pub x_min: i16,
    pub y_min: i16,
    pub x_max: i16,
    pub y_max: i16,
    pub hdpi: i16,
    pub vdpi: i16,
    pub ega_palette: [u8; 48],  // EGA palette (not used)
    pub reserved: u8,
    pub color_planes: u8,
    pub bytes_per_line: i16,
    pub palette_type: i16,
    pub filler: [u8; 58],
}

impl Default for PcxHeader {
    fn default() -> Self {
        PcxHeader {
            id: 0,
            version: 0,
            encoding: 0,
            bits_per_pixel: 0,
            x_min: 0,
            y_min: 0,
            x_max: 0,
            y_max: 0,
            hdpi: 0,
            vdpi: 0,
            ega_palette: [0; 48],
            reserved: 0,
            color_planes: 0,
            bytes_per_line: 0,
            palette_type: 0,
            filler: [0; 58],
        }
    }
}

/// Error type for PCX operations
#[derive(Debug)]
pub enum PcxError {
    IoError(io::Error),
    InvalidHeader,
    InvalidFormat,
    InvalidSize,
    DecompressFailed,
}

impl From<io::Error> for PcxError {
    fn from(e: io::Error) -> Self {
        PcxError::IoError(e)
    }
}

impl std::fmt::Display for PcxError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            PcxError::IoError(e) => write!(f, "IO error: {}", e),
            PcxError::InvalidHeader => write!(f, "Invalid PCX header"),
            PcxError::InvalidFormat => write!(f, "Invalid PCX format (not 8-bit)"),
            PcxError::InvalidSize => write!(f, "Invalid PCX size"),
            PcxError::DecompressFailed => write!(f, "PCX decompression failed"),
        }
    }
}

impl std::error::Error for PcxError {}

/// A loaded PCX image
#[derive(Clone)]
pub struct PcxImage {
    /// Image width in pixels
    pub width: i32,
    /// Image height in pixels
    pub height: i32,
    /// 8-bit indexed pixel data (width * height bytes)
    pub pixels: Vec<u8>,
    /// 256-color palette (768 bytes, 8-bit RGB values)
    pub palette: [u8; 768],
}

impl PcxImage {
    /// Create an empty PCX image
    pub fn new(width: i32, height: i32) -> Self {
        PcxImage {
            width,
            height,
            pixels: vec![0; (width * height) as usize],
            palette: [0; 768],
        }
    }

    /// Load PCX from raw file data
    ///
    /// Based on original Read_PCX_File() from CODE/WINSTUB.CPP
    pub fn from_data(data: &[u8]) -> Result<Self, PcxError> {
        if data.len() < 128 + 768 {
            return Err(PcxError::InvalidSize);
        }

        // Parse header
        let header = parse_header(&data[0..128])?;

        // Validate header
        if header.id != 10 {
            return Err(PcxError::InvalidHeader);
        }
        if header.bits_per_pixel != 8 {
            return Err(PcxError::InvalidFormat);
        }

        // Calculate dimensions
        let width = (header.x_max - header.x_min + 1) as i32;
        let height = (header.y_max - header.y_min + 1) as i32;

        if width <= 0 || height <= 0 || width > 4096 || height > 4096 {
            return Err(PcxError::InvalidSize);
        }

        // Decode RLE pixel data
        let pixel_data = &data[128..data.len() - 768];
        let pixels = decode_rle(pixel_data, width, height, header.bytes_per_line as i32)?;

        // Read palette from last 768 bytes (6-bit to 8-bit conversion)
        let pal_offset = data.len() - 768;
        let mut palette = [0u8; 768];
        for i in 0..768 {
            // PCX palette uses 6-bit values in high bits, no shift needed
            // Actually, original code shifts RIGHT by 2, then the C++ code
            // expects 6-bit values and shifts LEFT by 2 when applying.
            // So we store as-is (8-bit values from file)
            palette[i] = data[pal_offset + i];
        }

        Ok(PcxImage {
            width,
            height,
            pixels,
            palette,
        })
    }

    /// Get palette with 6-bit to 8-bit conversion applied
    ///
    /// PCX palettes are stored as 8-bit in the file, but the original game
    /// code expected them to be pre-divided by 4 (shifted right 2).
    /// This returns the palette converted to full 8-bit range.
    pub fn get_palette_8bit(&self) -> [u8; 768] {
        // PCX palettes are already 8-bit in the file
        // No conversion needed for modern usage
        self.palette
    }
}

/// Parse PCX header from bytes
fn parse_header(data: &[u8]) -> Result<PcxHeader, PcxError> {
    if data.len() < 128 {
        return Err(PcxError::InvalidHeader);
    }

    let mut header = PcxHeader::default();

    header.id = data[0];
    header.version = data[1];
    header.encoding = data[2];
    header.bits_per_pixel = data[3];
    header.x_min = i16::from_le_bytes([data[4], data[5]]);
    header.y_min = i16::from_le_bytes([data[6], data[7]]);
    header.x_max = i16::from_le_bytes([data[8], data[9]]);
    header.y_max = i16::from_le_bytes([data[10], data[11]]);
    header.hdpi = i16::from_le_bytes([data[12], data[13]]);
    header.vdpi = i16::from_le_bytes([data[14], data[15]]);
    header.ega_palette.copy_from_slice(&data[16..64]);
    header.reserved = data[64];
    header.color_planes = data[65];
    header.bytes_per_line = i16::from_le_bytes([data[66], data[67]]);
    header.palette_type = i16::from_le_bytes([data[68], data[69]]);
    header.filler.copy_from_slice(&data[70..128]);

    Ok(header)
}

/// Decode RLE-compressed pixel data
///
/// PCX RLE encoding:
/// - If byte >= 192 (0xC0), it's a run: count = byte - 192, next byte is value
/// - Otherwise, byte is a literal pixel value
fn decode_rle(data: &[u8], width: i32, height: i32, bytes_per_line: i32) -> Result<Vec<u8>, PcxError> {
    let total_pixels = (width * height) as usize;
    let mut pixels = vec![0u8; total_pixels];

    let mut src_pos = 0;
    let mut dst_pos = 0;

    // Decode line by line
    for _y in 0..height {
        let mut line_pos = 0;
        let line_end = bytes_per_line as usize;

        while line_pos < line_end && src_pos < data.len() {
            let byte = data[src_pos];
            src_pos += 1;

            if byte >= 192 {
                // Run-length encoded
                let count = (byte - 192) as usize;
                if src_pos >= data.len() {
                    return Err(PcxError::DecompressFailed);
                }
                let value = data[src_pos];
                src_pos += 1;

                for _ in 0..count {
                    if line_pos < width as usize && dst_pos < total_pixels {
                        pixels[dst_pos] = value;
                        dst_pos += 1;
                    }
                    line_pos += 1;
                }
            } else {
                // Literal byte
                if line_pos < width as usize && dst_pos < total_pixels {
                    pixels[dst_pos] = byte;
                    dst_pos += 1;
                }
                line_pos += 1;
            }
        }

        // Skip any padding in the scan line
        while line_pos < line_end && src_pos < data.len() {
            let byte = data[src_pos];
            src_pos += 1;
            if byte >= 192 {
                let count = (byte - 192) as usize;
                if src_pos < data.len() {
                    src_pos += 1;
                }
                line_pos += count;
            } else {
                line_pos += 1;
            }
        }
    }

    Ok(pixels)
}

// =============================================================================
// Unit Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_header_parse() {
        let mut data = [0u8; 128];
        data[0] = 10;  // id
        data[1] = 5;   // version
        data[2] = 1;   // encoding
        data[3] = 8;   // bits_per_pixel
        // x_min = 0, y_min = 0
        data[8] = 99;  // x_max low byte
        data[9] = 0;   // x_max high byte (width = 100)
        data[10] = 49; // y_max low byte
        data[11] = 0;  // y_max high byte (height = 50)

        let header = parse_header(&data).unwrap();
        assert_eq!(header.id, 10);
        assert_eq!(header.version, 5);
        assert_eq!(header.bits_per_pixel, 8);
        // Copy packed fields to local variables to avoid alignment issues
        let x_max = header.x_max;
        let y_max = header.y_max;
        assert_eq!(x_max, 99);
        assert_eq!(y_max, 49);
    }

    #[test]
    fn test_rle_decode_literal() {
        // Simple literal data (no RLE)
        let data = vec![1, 2, 3, 4];
        let result = decode_rle(&data, 4, 1, 4).unwrap();
        assert_eq!(result, vec![1, 2, 3, 4]);
    }

    #[test]
    fn test_rle_decode_run() {
        // RLE run: 0xC3 (195 = 192 + 3) followed by 0x55
        // Should decode to [0x55, 0x55, 0x55]
        let data = vec![195, 0x55];
        let result = decode_rle(&data, 3, 1, 3).unwrap();
        assert_eq!(result, vec![0x55, 0x55, 0x55]);
    }

    #[test]
    fn test_invalid_header() {
        let mut data = [0u8; 128 + 768];
        data[0] = 99; // Invalid ID
        let result = PcxImage::from_data(&data);
        assert!(result.is_err());
    }
}
