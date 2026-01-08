//! MIX file archive format handling
//!
//! MIX files are the archive format used by Command & Conquer Red Alert.
//! They contain multiple files indexed by a CRC hash of the filename.
//!
//! ## MIX File Format
//!
//! ### Plain Format (original RA)
//! ```text
//! [FileHeader: 6 bytes]
//!   count: i16  - Number of files
//!   size:  i32  - Total size of data section
//!
//! [SubBlock array: count * 12 bytes]
//!   For each file:
//!     crc:    i32  - Westwood CRC of uppercase filename
//!     offset: i32  - Offset from start of data section
//!     size:   i32  - Size of file data
//!
//! [Data section]
//!   Raw file data, accessed via SubBlock offsets
//! ```
//!
//! ### Extended Format (RA with encryption/digest)
//! ```text
//! [Alternate Header: 4 bytes]
//!   first:  i16  - Always 0 for extended format
//!   second: i16  - Flags: bit 0 = has digest, bit 1 = encrypted
//!
//! [If encrypted: Blowfish key data]
//! [FileHeader + SubBlocks (possibly encrypted)]
//! [Data section]
//! [If has digest: 20-byte SHA-1 hash]
//! ```

use std::collections::HashMap;
use std::fs::File;
use std::io::{self, Read, Seek, SeekFrom};
use std::path::Path;

use crate::crypto::{decrypt_mix_key, BlowfishEngine};
use crate::util::crc::westwood_crc_filename;

/// Size of the RSA-encrypted key block in MIX files
const RSA_KEY_BLOCK_SIZE: usize = 80;

/// Error type for MIX file operations
#[derive(Debug)]
pub enum MixError {
    IoError(io::Error),
    InvalidHeader,
    InvalidFormat(String),
    FileNotFound(String),
    DecryptionFailed(String),
}

impl From<io::Error> for MixError {
    fn from(e: io::Error) -> Self {
        MixError::IoError(e)
    }
}

impl std::fmt::Display for MixError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            MixError::IoError(e) => write!(f, "IO error: {}", e),
            MixError::InvalidHeader => write!(f, "Invalid MIX header"),
            MixError::InvalidFormat(msg) => write!(f, "Invalid format: {}", msg),
            MixError::FileNotFound(name) => write!(f, "File not found: {}", name),
            MixError::DecryptionFailed(msg) => write!(f, "Decryption failed: {}", msg),
        }
    }
}

impl std::error::Error for MixError {}

/// MIX file entry (SubBlock in original code)
#[derive(Debug, Clone, Copy)]
pub struct MixEntry {
    /// Westwood CRC of filename (treated as signed for sorting)
    pub crc: i32,
    /// Offset from start of data section
    pub offset: u32,
    /// Size of file data
    pub size: u32,
}

impl MixEntry {
    /// Read a MixEntry from a byte slice
    pub fn from_bytes(data: &[u8; 12]) -> Self {
        MixEntry {
            crc: i32::from_le_bytes([data[0], data[1], data[2], data[3]]),
            offset: u32::from_le_bytes([data[4], data[5], data[6], data[7]]),
            size: u32::from_le_bytes([data[8], data[9], data[10], data[11]]),
        }
    }
}

/// MIX file header
#[derive(Debug, Clone)]
pub struct MixHeader {
    /// Number of files in the archive
    pub file_count: u16,
    /// Total size of the data section
    pub data_size: u32,
    /// Whether this is an extended format MIX
    pub is_extended: bool,
    /// Whether the MIX has a SHA digest
    pub has_digest: bool,
    /// Whether the header is encrypted
    pub is_encrypted: bool,
}

impl MixHeader {
    /// Read MIX header from a byte slice (already decrypted if needed)
    pub fn from_bytes(data: &[u8]) -> Result<Self, MixError> {
        if data.len() < 6 {
            return Err(MixError::InvalidHeader);
        }

        let file_count = u16::from_le_bytes([data[0], data[1]]);
        let data_size = u32::from_le_bytes([data[2], data[3], data[4], data[5]]);

        Ok(MixHeader {
            file_count,
            data_size,
            is_extended: false,  // Will be set by caller
            has_digest: false,   // Will be set by caller
            is_encrypted: false, // Will be set by caller
        })
    }

    /// Calculate header size including entry table (from start of file header, not flags)
    pub fn header_size(&self) -> u32 {
        6 + (self.file_count as u32 * 12)
    }

    /// Calculate total bytes needed from start of file to data section
    pub fn total_header_size(&self) -> u32 {
        let mut size = self.header_size();
        if self.is_extended {
            size += 4; // 4 bytes for extended flags
            if self.is_encrypted {
                size += RSA_KEY_BLOCK_SIZE as u32; // 80 bytes for RSA key block
            }
        }
        size
    }
}

/// A single MIX archive file
pub struct MixFile {
    /// Path to the MIX file
    pub path: String,
    /// Parsed header
    pub header: MixHeader,
    /// File entries, sorted by CRC for binary search
    entries: Vec<MixEntry>,
    /// Offset to start of data section
    data_start: u64,
    /// Optional: cached file data (if loaded into memory)
    cached_data: Option<Vec<u8>>,
}

impl MixFile {
    /// Open a MIX file and read its header
    ///
    /// Automatically handles encrypted MIX files by decrypting the header
    /// using RSA to recover the Blowfish key, then Blowfish to decrypt
    /// the file index.
    pub fn open<P: AsRef<Path>>(path: P) -> Result<Self, MixError> {
        let path_str = path.as_ref().to_string_lossy().to_string();
        let mut file = File::open(&path)?;

        // Read first 4 bytes to detect format
        let mut flag_buf = [0u8; 4];
        file.read_exact(&mut flag_buf)?;

        let first = i16::from_le_bytes([flag_buf[0], flag_buf[1]]);

        let (header, entries, data_start) = if first == 0 {
            // Extended format
            let flags = i16::from_le_bytes([flag_buf[2], flag_buf[3]]);
            let has_digest = (flags & 0x01) != 0;
            let is_encrypted = (flags & 0x02) != 0;

            if is_encrypted {
                // Read encrypted MIX file
                Self::read_encrypted(&mut file, has_digest)?
            } else {
                // Extended but not encrypted - just read header normally
                let mut header_buf = [0u8; 6];
                file.read_exact(&mut header_buf)?;

                let mut header = MixHeader::from_bytes(&header_buf)?;
                header.is_extended = true;
                header.has_digest = has_digest;
                header.is_encrypted = false;

                // Read entry table
                let entries = Self::read_entries(&mut file, header.file_count as usize)?;
                let data_start = file.stream_position()?;

                (header, entries, data_start)
            }
        } else {
            // Plain format - first 4 bytes are part of header
            // Read remaining 2 bytes
            let mut extra = [0u8; 2];
            file.read_exact(&mut extra)?;

            let header_buf = [
                flag_buf[0],
                flag_buf[1],
                flag_buf[2],
                flag_buf[3],
                extra[0],
                extra[1],
            ];

            let header = MixHeader::from_bytes(&header_buf)?;

            // Read entry table
            let entries = Self::read_entries(&mut file, header.file_count as usize)?;
            let data_start = file.stream_position()?;

            (header, entries, data_start)
        };

        // Verify entries are sorted (debug only)
        #[cfg(debug_assertions)]
        {
            for i in 1..entries.len() {
                if entries[i].crc < entries[i - 1].crc {
                    log::warn!("MIX entries not sorted at index {}", i);
                }
            }
        }

        Ok(MixFile {
            path: path_str,
            header,
            entries,
            data_start,
            cached_data: None,
        })
    }

    /// Read entries from a reader
    fn read_entries<R: Read>(reader: &mut R, count: usize) -> Result<Vec<MixEntry>, MixError> {
        let mut entries = Vec::with_capacity(count);

        for _ in 0..count {
            let mut buf = [0u8; 12];
            reader.read_exact(&mut buf)?;
            entries.push(MixEntry::from_bytes(&buf));
        }

        Ok(entries)
    }

    /// Read an encrypted MIX file header
    ///
    /// Layout:
    /// - 80 bytes: RSA-encrypted Blowfish key block
    /// - Blowfish-encrypted data (header + entries, padded to 8 bytes)
    fn read_encrypted<R: Read + Seek>(
        reader: &mut R,
        has_digest: bool,
    ) -> Result<(MixHeader, Vec<MixEntry>, u64), MixError> {
        // Read RSA-encrypted key block (80 bytes)
        let mut key_block = [0u8; RSA_KEY_BLOCK_SIZE];
        reader.read_exact(&mut key_block)?;

        // Decrypt to get Blowfish key
        let bf_key = decrypt_mix_key(&key_block);

        // Initialize Blowfish engine
        let mut bf = BlowfishEngine::new();
        bf.set_key(&bf_key);

        // First, decrypt just the header (8 bytes to get file_count and data_size)
        // The header is 6 bytes but Blowfish operates on 8-byte blocks
        let mut header_block = [0u8; 8];
        reader.read_exact(&mut header_block)?;
        bf.decrypt(&mut header_block);

        let file_count = u16::from_le_bytes([header_block[0], header_block[1]]);
        let data_size = u32::from_le_bytes([
            header_block[2],
            header_block[3],
            header_block[4],
            header_block[5],
        ]);

        // Calculate how many more bytes we need for entries
        // Each entry is 12 bytes, we already decrypted first 8 bytes
        // Total encrypted = 6 (header) + file_count * 12, rounded up to 8
        let total_index_size = 6 + (file_count as usize * 12);
        let encrypted_size = (total_index_size + 7) & !7; // Round up to 8
        let remaining_encrypted = encrypted_size - 8; // We already read 8 bytes

        // Read and decrypt remaining data
        let mut remaining_data = vec![0u8; remaining_encrypted];
        reader.read_exact(&mut remaining_data)?;

        // Decrypt in 8-byte blocks
        for chunk in remaining_data.chunks_exact_mut(8) {
            let block: &mut [u8; 8] = chunk.try_into().unwrap();
            bf.decrypt(block);
        }

        // Combine decrypted data: first 8 bytes + remaining
        // But we only need bytes starting from offset 6 (after header)
        // header_block[6..8] contains first 2 bytes of first entry
        // remaining_data contains the rest

        let mut entries = Vec::with_capacity(file_count as usize);

        // Build entry data from decrypted blocks
        // First entry starts at byte 6, we have bytes 6-7 in header_block
        let mut entry_data = Vec::with_capacity(file_count as usize * 12);
        entry_data.extend_from_slice(&header_block[6..8]);
        entry_data.extend_from_slice(&remaining_data);

        // Parse entries
        for i in 0..file_count as usize {
            let start = i * 12;
            if start + 12 > entry_data.len() {
                return Err(MixError::DecryptionFailed(format!(
                    "Not enough data for entry {}",
                    i
                )));
            }
            let entry_bytes: [u8; 12] = entry_data[start..start + 12]
                .try_into()
                .map_err(|_| MixError::DecryptionFailed("Entry conversion failed".to_string()))?;
            entries.push(MixEntry::from_bytes(&entry_bytes));
        }

        // Calculate data start position
        // 4 (extended flags) + 80 (RSA block) + encrypted_size
        let data_start = reader.stream_position()?;

        let header = MixHeader {
            file_count,
            data_size,
            is_extended: true,
            has_digest,
            is_encrypted: true,
        };

        Ok((header, entries, data_start))
    }

    /// Cache the entire data section in memory
    pub fn cache(&mut self) -> Result<(), MixError> {
        if self.cached_data.is_some() {
            return Ok(());
        }

        let mut file = File::open(&self.path)?;
        file.seek(SeekFrom::Start(self.data_start))?;

        let mut data = vec![0u8; self.header.data_size as usize];
        file.read_exact(&mut data)?;

        self.cached_data = Some(data);
        Ok(())
    }

    /// Release cached data
    pub fn uncache(&mut self) {
        self.cached_data = None;
    }

    /// Check if data is cached
    pub fn is_cached(&self) -> bool {
        self.cached_data.is_some()
    }

    /// Get number of files in the archive
    pub fn file_count(&self) -> usize {
        self.entries.len()
    }

    /// Look up a file by CRC hash
    pub fn find_by_crc(&self, crc: i32) -> Option<&MixEntry> {
        // Binary search using signed comparison (matching original code)
        match self.entries.binary_search_by(|e| e.crc.cmp(&crc)) {
            Ok(idx) => Some(&self.entries[idx]),
            Err(_) => None,
        }
    }

    /// Look up a file by name
    pub fn find(&self, filename: &str) -> Option<&MixEntry> {
        let crc = westwood_crc_filename(filename) as i32;
        self.find_by_crc(crc)
    }

    /// Check if file exists in archive
    pub fn contains(&self, filename: &str) -> bool {
        self.find(filename).is_some()
    }

    /// Read file data from the archive
    pub fn read(&self, filename: &str) -> Result<Vec<u8>, MixError> {
        let entry = self
            .find(filename)
            .ok_or_else(|| MixError::FileNotFound(filename.to_string()))?;

        self.read_entry(entry)
    }

    /// Read file data by entry
    pub fn read_entry(&self, entry: &MixEntry) -> Result<Vec<u8>, MixError> {
        // If cached, read from cache
        if let Some(ref data) = self.cached_data {
            let start = entry.offset as usize;
            let end = start + entry.size as usize;

            if end > data.len() {
                return Err(MixError::InvalidFormat(format!(
                    "Entry offset {} + size {} exceeds data size {}",
                    entry.offset,
                    entry.size,
                    data.len()
                )));
            }

            return Ok(data[start..end].to_vec());
        }

        // Otherwise read from file
        let mut file = File::open(&self.path)?;
        file.seek(SeekFrom::Start(self.data_start + entry.offset as u64))?;

        let mut data = vec![0u8; entry.size as usize];
        file.read_exact(&mut data)?;

        Ok(data)
    }

    /// Read file data by CRC
    pub fn read_by_crc(&self, crc: i32) -> Result<Vec<u8>, MixError> {
        let entry = self
            .find_by_crc(crc)
            .ok_or_else(|| MixError::FileNotFound(format!("CRC 0x{:08X}", crc)))?;

        self.read_entry(entry)
    }

    /// Get a pointer to file data if cached
    ///
    /// Returns None if not cached or file not found
    pub fn get_ptr(&self, filename: &str) -> Option<&[u8]> {
        let entry = self.find(filename)?;

        self.cached_data.as_ref().and_then(|data| {
            let start = entry.offset as usize;
            let end = start + entry.size as usize;

            if end <= data.len() {
                Some(&data[start..end])
            } else {
                None
            }
        })
    }

    /// Iterate over all entries
    pub fn entries(&self) -> impl Iterator<Item = &MixEntry> {
        self.entries.iter()
    }
}

/// Manager for multiple MIX files
///
/// Searches through registered MIX files in order when looking up assets.
pub struct MixManager {
    /// Registered MIX files
    mixes: Vec<MixFile>,
    /// Optional: CRC to filename mapping for debugging
    name_map: HashMap<i32, String>,
}

impl MixManager {
    /// Create a new empty manager
    pub fn new() -> Self {
        MixManager {
            mixes: Vec::new(),
            name_map: HashMap::new(),
        }
    }

    /// Register a MIX file
    pub fn register<P: AsRef<Path>>(&mut self, path: P) -> Result<(), MixError> {
        let mix = MixFile::open(path)?;
        self.mixes.push(mix);
        Ok(())
    }

    /// Register and cache a MIX file
    pub fn register_and_cache<P: AsRef<Path>>(&mut self, path: P) -> Result<(), MixError> {
        let mut mix = MixFile::open(path)?;
        mix.cache()?;
        self.mixes.push(mix);
        Ok(())
    }

    /// Unregister a MIX file by path
    pub fn unregister(&mut self, path: &str) -> bool {
        if let Some(idx) = self.mixes.iter().position(|m| m.path == path) {
            self.mixes.remove(idx);
            true
        } else {
            false
        }
    }

    /// Find a file across all registered MIX files
    pub fn find(&self, filename: &str) -> Option<(&MixFile, &MixEntry)> {
        let crc = westwood_crc_filename(filename) as i32;

        for mix in &self.mixes {
            if let Some(entry) = mix.find_by_crc(crc) {
                return Some((mix, entry));
            }
        }

        None
    }

    /// Check if file exists in any registered MIX
    pub fn exists(&self, filename: &str) -> bool {
        self.find(filename).is_some()
    }

    /// Read file data from the first MIX that contains it
    pub fn read(&self, filename: &str) -> Result<Vec<u8>, MixError> {
        match self.find(filename) {
            Some((mix, entry)) => mix.read_entry(entry),
            None => Err(MixError::FileNotFound(filename.to_string())),
        }
    }

    /// Get pointer to file data if cached
    pub fn get_ptr(&self, filename: &str) -> Option<&[u8]> {
        let crc = westwood_crc_filename(filename) as i32;

        for mix in &self.mixes {
            if mix.find_by_crc(crc).is_some() {
                if let Some(ptr) = mix.get_ptr(filename) {
                    return Some(ptr);
                }
            }
        }

        None
    }

    /// Get number of registered MIX files
    pub fn mix_count(&self) -> usize {
        self.mixes.len()
    }

    /// Cache a specific MIX file
    pub fn cache(&mut self, path: &str) -> Result<bool, MixError> {
        for mix in &mut self.mixes {
            if mix.path == path {
                mix.cache()?;
                return Ok(true);
            }
        }
        Ok(false)
    }

    /// Uncache a specific MIX file
    pub fn uncache(&mut self, path: &str) -> bool {
        for mix in &mut self.mixes {
            if mix.path == path {
                mix.uncache();
                return true;
            }
        }
        false
    }

    /// Register a known filename for CRC debugging
    pub fn register_name(&mut self, filename: &str) {
        let crc = westwood_crc_filename(filename) as i32;
        self.name_map.insert(crc, filename.to_uppercase());
    }

    /// Look up a filename by CRC (if registered)
    pub fn lookup_name(&self, crc: i32) -> Option<&str> {
        self.name_map.get(&crc).map(|s| s.as_str())
    }
}

impl Default for MixManager {
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
    fn test_mix_entry_from_bytes() {
        let data = [
            0x12, 0x34, 0x56, 0x78, // crc = 0x78563412
            0xAA, 0xBB, 0xCC, 0xDD, // offset = 0xDDCCBBAA
            0x10, 0x20, 0x00, 0x00, // size = 0x00002010
        ];

        let entry = MixEntry::from_bytes(&data);
        assert_eq!(entry.crc, 0x78563412_u32 as i32);
        assert_eq!(entry.offset, 0xDDCCBBAA);
        assert_eq!(entry.size, 0x00002010);
    }

    #[test]
    fn test_westwood_crc_filename_lookup() {
        // Verify case insensitivity
        let crc1 = westwood_crc_filename("PALETTE.PAL") as i32;
        let crc2 = westwood_crc_filename("palette.pal") as i32;
        assert_eq!(crc1, crc2);
    }

    #[test]
    fn test_mix_manager_default() {
        let manager = MixManager::default();
        assert_eq!(manager.mix_count(), 0);
        assert!(!manager.exists("test.dat"));
    }

    #[test]
    fn test_mix_header_from_bytes() {
        // file_count = 10, data_size = 0x1000
        let data = [
            0x0A, 0x00, // file_count = 10
            0x00, 0x10, 0x00, 0x00, // data_size = 0x1000
        ];
        let header = MixHeader::from_bytes(&data).unwrap();
        assert_eq!(header.file_count, 10);
        assert_eq!(header.data_size, 0x1000);
        assert_eq!(header.header_size(), 6 + 10 * 12); // 126 bytes
    }
}
