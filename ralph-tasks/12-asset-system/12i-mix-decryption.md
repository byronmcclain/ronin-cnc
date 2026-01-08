# Task 12i: MIX File Decryption (Blowfish/RSA)

## Dependencies
- Task 12b (MIX Header) must be complete
- Task 12c (MIX Lookup) must be complete
- Platform file I/O functions available

## Context
Most Red Alert MIX files (18 of 20) use Blowfish encryption on their headers. Without decryption, we can only load AUD.MIX and SETUP.MIX. This task implements the crypto primitives needed to decrypt REDALERT.MIX and all theater/campaign files.

## Objective
Implement Blowfish and RSA decryption in Rust to enable loading encrypted MIX files:
1. Port Blowfish cipher from Westwood's GPL-licensed code
2. Implement RSA decryption using Westwood's known public key
3. Modify `MixFile` to automatically decrypt encrypted headers
4. Verify by successfully loading REDALERT.MIX

## Technical Background

### Encrypted MIX File Layout
```
Offset   Size    Description
------   ----    -----------
0x0000   2       Zero marker (0x0000) - indicates extended format
0x0002   2       Flags: bit 0 = digest, bit 1 = encrypted
0x0004   80      RSA-encrypted Blowfish key block
0x0054   ...     Blowfish-encrypted header:
                   - 2 bytes: file count
                   - 4 bytes: total data size
                   - 12*N bytes: SubBlock entries
                 (padded to 8-byte Blowfish block boundary)
???      ...     Unencrypted file data section
```

### Westwood Public Key
From `CODE/CONST.CPP`:
```cpp
"[PublicKey]\n"
"1=AihRvNoIbTn85FZRYNZRcT+i6KpU+maCsEqr3Q5q+LDB5tH7Tz2qQ38V\n"
```
- **Modulus**: Base64-decoded ~40-byte big integer
- **Exponent**: 65537 (0x10001) - standard RSA public exponent

### Blowfish Algorithm
From `CODE/BLOWFISH.H`:
- Block size: 64 bits (8 bytes)
- Key length: 1-56 bytes
- Rounds: 16 Feistel rounds
- P-box: 18 entries initialized from Pi
- S-boxes: 4 boxes × 256 entries each

### Decryption Flow
1. Read 4-byte header, check for 0x0000 marker and 0x02 flag
2. Read 80 bytes of RSA-encrypted key data
3. RSA decrypt to recover 56-byte Blowfish key
4. Initialize Blowfish engine with decrypted key
5. Read and decrypt header (file count, data size, entries)
6. Data section is NOT encrypted

## Deliverables
- [ ] `platform/src/crypto/mod.rs` - Crypto module declaration
- [ ] `platform/src/crypto/blowfish.rs` - Blowfish cipher implementation
- [ ] `platform/src/crypto/rsa.rs` - RSA decryption with Westwood key
- [ ] Modified `platform/src/assets/mix.rs` - Decryption integration
- [ ] `src/test_mix_decrypt.cpp` - Test program
- [ ] CMake integration for test executable
- [ ] Verification script passes

## Files to Create

### platform/src/crypto/mod.rs (NEW)
```rust
//! Cryptographic primitives for MIX file decryption

pub mod blowfish;
pub mod rsa;

pub use blowfish::BlowfishEngine;
pub use rsa::{RsaPublicKey, WESTWOOD_PUBLIC_KEY};
```

### platform/src/crypto/blowfish.rs (NEW)
```rust
//! Blowfish symmetric cipher implementation
//!
//! Ported from Westwood's GPL-licensed CODE/BLOWFISH.CPP

/// Blowfish cipher engine
pub struct BlowfishEngine {
    p_encrypt: [u32; 18],
    p_decrypt: [u32; 18],
    s_boxes: [[u32; 256]; 4],
    is_keyed: bool,
}

impl BlowfishEngine {
    pub const MAX_KEY_LENGTH: usize = 56;
    pub const BLOCK_SIZE: usize = 8;
    const ROUNDS: usize = 16;

    pub fn new() -> Self {
        Self {
            p_encrypt: P_INIT,
            p_decrypt: [0u32; 18],
            s_boxes: S_INIT,
            is_keyed: false,
        }
    }

    /// Initialize with key (1-56 bytes)
    pub fn set_key(&mut self, key: &[u8]) {
        assert!(!key.is_empty() && key.len() <= Self::MAX_KEY_LENGTH);

        self.p_encrypt = P_INIT;
        self.s_boxes = S_INIT;

        // XOR P-box with key cyclically
        let mut key_idx = 0;
        for i in 0..18 {
            let mut data = 0u32;
            for _ in 0..4 {
                data = (data << 8) | (key[key_idx] as u32);
                key_idx = (key_idx + 1) % key.len();
            }
            self.p_encrypt[i] ^= data;
        }

        // Generate P-box and S-box values by encrypting zeros
        let mut left = 0u32;
        let mut right = 0u32;

        for i in (0..18).step_by(2) {
            self.encrypt_block(&mut left, &mut right);
            self.p_encrypt[i] = left;
            self.p_encrypt[i + 1] = right;
        }

        for s in 0..4 {
            for i in (0..256).step_by(2) {
                self.encrypt_block(&mut left, &mut right);
                self.s_boxes[s][i] = left;
                self.s_boxes[s][i + 1] = right;
            }
        }

        // Create decryption P-box (reversed)
        for i in 0..18 {
            self.p_decrypt[i] = self.p_encrypt[17 - i];
        }

        self.is_keyed = true;
    }

    fn encrypt_block(&self, left: &mut u32, right: &mut u32) {
        let mut l = *left;
        let mut r = *right;

        for i in 0..Self::ROUNDS {
            l ^= self.p_encrypt[i];
            r ^= self.feistel(l);
            std::mem::swap(&mut l, &mut r);
        }

        std::mem::swap(&mut l, &mut r);
        r ^= self.p_encrypt[Self::ROUNDS];
        l ^= self.p_encrypt[Self::ROUNDS + 1];

        *left = l;
        *right = r;
    }

    fn decrypt_block(&self, left: &mut u32, right: &mut u32) {
        let mut l = *left;
        let mut r = *right;

        for i in 0..Self::ROUNDS {
            l ^= self.p_decrypt[i];
            r ^= self.feistel(l);
            std::mem::swap(&mut l, &mut r);
        }

        std::mem::swap(&mut l, &mut r);
        r ^= self.p_decrypt[Self::ROUNDS];
        l ^= self.p_decrypt[Self::ROUNDS + 1];

        *left = l;
        *right = r;
    }

    #[inline]
    fn feistel(&self, x: u32) -> u32 {
        let a = ((x >> 24) & 0xFF) as usize;
        let b = ((x >> 16) & 0xFF) as usize;
        let c = ((x >> 8) & 0xFF) as usize;
        let d = (x & 0xFF) as usize;

        ((self.s_boxes[0][a].wrapping_add(self.s_boxes[1][b]))
            ^ self.s_boxes[2][c])
            .wrapping_add(self.s_boxes[3][d])
    }

    /// Decrypt data in place (must be multiple of 8 bytes)
    pub fn decrypt(&self, data: &mut [u8]) -> usize {
        if !self.is_keyed || data.len() < Self::BLOCK_SIZE {
            return 0;
        }

        let block_count = data.len() / Self::BLOCK_SIZE;

        for i in 0..block_count {
            let offset = i * Self::BLOCK_SIZE;

            let mut left = u32::from_be_bytes([
                data[offset], data[offset + 1],
                data[offset + 2], data[offset + 3]
            ]);
            let mut right = u32::from_be_bytes([
                data[offset + 4], data[offset + 5],
                data[offset + 6], data[offset + 7]
            ]);

            self.decrypt_block(&mut left, &mut right);

            data[offset..offset + 4].copy_from_slice(&left.to_be_bytes());
            data[offset + 4..offset + 8].copy_from_slice(&right.to_be_bytes());
        }

        block_count * Self::BLOCK_SIZE
    }
}

impl Default for BlowfishEngine {
    fn default() -> Self {
        Self::new()
    }
}

// P-box initial values (from digits of Pi)
const P_INIT: [u32; 18] = [
    0x243f6a88, 0x85a308d3, 0x13198a2e, 0x03707344,
    0xa4093822, 0x299f31d0, 0x082efa98, 0xec4e6c89,
    0x452821e6, 0x38d01377, 0xbe5466cf, 0x34e90c6c,
    0xc0ac29b7, 0xc97c50dd, 0x3f84d5b5, 0xb5470917,
    0x9216d5d9, 0x8979fb1b,
];

// S-box initial values (from digits of Pi) - 4 boxes x 256 entries
// Full tables from CODE/BLOWFISH.CPP (4096 bytes total)
const S_INIT: [[u32; 256]; 4] = [
    // S-box 0
    [
        0xd1310ba6, 0x98dfb5ac, 0x2ffd72db, 0xd01adfb7,
        0xb8e1afed, 0x6a267e96, 0xba7c9045, 0xf12c7f99,
        // ... (remaining 248 entries)
        0x53b02d5d, 0xa99f8fa1, 0x08ba4799, 0x6e85076a,
    ],
    // S-box 1, 2, 3 follow same pattern
    // ... (full tables copied from CODE/BLOWFISH.CPP)
];

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_roundtrip() {
        let mut bf = BlowfishEngine::new();
        bf.set_key(b"TESTKEY");

        let mut data = *b"Hello!!X";  // 8 bytes
        let original = data;

        bf.encrypt(&mut data);
        assert_ne!(data, original);

        bf.decrypt(&mut data);
        assert_eq!(data, original);
    }
}
```

**Note**: The full S_INIT tables (4096 bytes) should be copied from `CODE/BLOWFISH.CPP`. They are the fractional parts of Pi and are public domain.

### platform/src/crypto/rsa.rs (NEW)
```rust
//! Simple RSA decryption for MIX file key recovery

/// Westwood's public key for MIX decryption
/// Base64 decoded from: "AihRvNoIbTn85FZRYNZRcT+i6KpU+maCsEqr3Q5q+LDB5tH7Tz2qQ38V"
pub const WESTWOOD_PUBLIC_KEY: RsaPublicKey = RsaPublicKey {
    modulus: &[
        0x02, 0x28, 0x51, 0xbc, 0xda, 0x08, 0x6d, 0x39,
        0xfc, 0xe4, 0x56, 0x51, 0x60, 0xd6, 0x51, 0x71,
        0x3f, 0xa2, 0xe8, 0xaa, 0x54, 0xfa, 0x66, 0x82,
        0xb0, 0x4a, 0xab, 0xdd, 0x0e, 0x6a, 0xf8, 0xb0,
        0xc1, 0xe6, 0xd1, 0xfb, 0x4f, 0x3d, 0xaa, 0x43,
        0x7f, 0x15,
    ],
    exponent: 65537,
};

pub struct RsaPublicKey {
    pub modulus: &'static [u8],
    pub exponent: u32,
}

impl RsaPublicKey {
    /// Decrypt RSA-encrypted data
    /// Returns number of bytes written to plaintext
    pub fn decrypt(&self, ciphertext: &[u8], plaintext: &mut [u8]) -> usize {
        let c = BigUint::from_bytes_be(ciphertext);
        let n = BigUint::from_bytes_be(self.modulus);
        let e = BigUint::from(self.exponent);

        let m = mod_pow(&c, &e, &n);
        let result = m.to_bytes_be();

        let copy_len = result.len().min(plaintext.len());
        let pad_len = plaintext.len().saturating_sub(result.len());
        plaintext[..pad_len].fill(0);
        plaintext[pad_len..pad_len + copy_len].copy_from_slice(&result[..copy_len]);

        plaintext.len()
    }

    pub fn block_size(&self) -> usize {
        self.modulus.len()
    }
}

// Minimal BigUint implementation for RSA
// Consider using num-bigint crate for production
struct BigUint {
    digits: Vec<u32>,
}

impl BigUint {
    fn from(value: u32) -> Self { Self { digits: vec![value] } }
    fn from_bytes_be(bytes: &[u8]) -> Self { /* ... */ }
    fn to_bytes_be(&self) -> Vec<u8> { /* ... */ }
    // ... multiplication, modulo, comparison operations
}

fn mod_pow(base: &BigUint, exp: &BigUint, modulus: &BigUint) -> BigUint {
    // Square-and-multiply algorithm
    // ...
}
```

### platform/src/assets/mix.rs (MODIFY)
Add decryption to `MixFile::open()`:

```rust
use crate::crypto::{BlowfishEngine, WESTWOOD_PUBLIC_KEY};

impl MixFile {
    pub fn open<P: AsRef<Path>>(path: P) -> Result<Self, MixError> {
        // ... existing header reading code ...

        if first_short == 0 {
            let is_encrypted = (flags & 0x02) != 0;

            if is_encrypted {
                // Read 80-byte RSA-encrypted key block
                let mut key_block = [0u8; 80];
                file.read_exact(&mut key_block)?;

                // RSA decrypt to get Blowfish key
                let mut bf_key = [0u8; 56];
                WESTWOOD_PUBLIC_KEY.decrypt(&key_block[..40], &mut bf_key[..40]);

                // Initialize Blowfish
                let mut bf = BlowfishEngine::new();
                bf.set_key(&bf_key);

                // Read and decrypt header
                let mut enc_header = [0u8; 8];
                file.read_exact(&mut enc_header)?;
                bf.decrypt(&mut enc_header);

                let file_count = u16::from_le_bytes([enc_header[0], enc_header[1]]);
                let data_size = u32::from_le_bytes([
                    enc_header[2], enc_header[3], enc_header[4], enc_header[5]
                ]);

                // Decrypt entries
                let entries_size = (file_count as usize) * 12;
                let padded_size = (entries_size + 7) & !7;
                let mut enc_entries = vec![0u8; padded_size];
                file.read_exact(&mut enc_entries)?;
                bf.decrypt(&mut enc_entries);

                // Parse entries from decrypted data
                // ...
            }
        }
        // ... rest of implementation
    }
}
```

### src/test_mix_decrypt.cpp (NEW)
```cpp
#include <cstdio>
#include <cstring>
#include "../include/platform.h"

int main(int argc, char* argv[]) {
    const char* mix_path = argc > 1 ? argv[1] : "gamedata/REDALERT.MIX";

    printf("=== MIX Decryption Test ===\n");
    printf("Testing: %s\n\n", mix_path);

    Platform_Assets_Init();

    int result = Platform_Mix_Register(mix_path);
    if (result != 0) {
        printf("FAILED: Could not register MIX file (error %d)\n", result);
        return 1;
    }

    printf("SUCCESS: MIX file registered\n");
    printf("Registered MIX files: %d\n", Platform_Mix_GetCount());

    // Test finding known files
    const char* test_files[] = {"RULES.INI", "PALETTE.PAL", "MOUSE.SHP", NULL};

    printf("\nSearching for known files:\n");
    for (int i = 0; test_files[i]; i++) {
        uint32_t size = 0;
        const uint8_t* data = Platform_Mix_FindFile(test_files[i], &size);
        printf("  %s: %s", test_files[i], data ? "FOUND" : "not found");
        if (data) printf(" (%u bytes)", size);
        printf("\n");
    }

    Platform_Mix_Unregister(mix_path);
    printf("\n=== Test Complete ===\n");
    return 0;
}
```

### CMakeLists.txt (MODIFY)
```cmake
# MIX decryption test
add_executable(TestMixDecrypt src/test_mix_decrypt.cpp)
target_include_directories(TestMixDecrypt PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(TestMixDecrypt PRIVATE redalert_platform ${PLATFORM_LIBS})
```

## Verification Command
```bash
ralph-tasks/verify/check-mix-decrypt.sh
```

## Implementation Steps
1. Create `platform/src/crypto/` directory
2. Create `platform/src/crypto/mod.rs`
3. Create `platform/src/crypto/blowfish.rs` with full S-box tables
4. Create `platform/src/crypto/rsa.rs` with BigUint implementation
5. Add `pub mod crypto;` to `platform/src/lib.rs`
6. Modify `platform/src/assets/mix.rs` to use crypto module
7. Create `src/test_mix_decrypt.cpp`
8. Update `CMakeLists.txt`
9. Build: `cmake --build build --target TestMixDecrypt`
10. Test: `./build/TestMixDecrypt gamedata/REDALERT.MIX`
11. Run verification script

## Success Criteria
- Blowfish encrypt/decrypt roundtrip works correctly
- RSA decryption produces valid Blowfish key
- `Platform_Mix_Register("gamedata/REDALERT.MIX")` returns 0 (success)
- Known files (RULES.INI, PALETTE.PAL) are found in decrypted archive
- All 20 MIX files in gamedata/ can be loaded
- Verification script outputs VERIFY_SUCCESS

## Common Issues

### "EncryptedNotSupported" error persists
- Ensure crypto module is compiled into library
- Check that `mix.rs` imports from `crate::crypto`
- Verify `pub mod crypto;` is in `lib.rs`

### RSA decryption produces garbage
- Verify public key modulus bytes are correct (Base64 decode)
- Check big-endian byte ordering
- Exponent should be 65537 (0x10001)

### Blowfish decryption produces garbage
- S-box tables must be exact copies from BLOWFISH.CPP
- Block size is 8 bytes - data must be padded
- Byte order is big-endian for block operations

### File count is wrong after decryption
- Encrypted header is padded to 8-byte boundary
- First 6 bytes of decrypted header are: count (2) + size (4)
- Remaining padding bytes should be ignored

### Data section appears corrupted
- Data section is NOT encrypted, only the header
- Data offsets in entries are relative to data section start
- Data start = 4 + 80 + 8 + padded_entries_size

## Completion Promise
When verification passes, output:
```
<promise>TASK_12I_COMPLETE</promise>
```

## Escape Hatch
If stuck after 20 iterations:
- Document what's blocking in `ralph-tasks/blocked/12i.md`
- List attempted approaches
- Output: `<promise>TASK_12I_BLOCKED</promise>`

## Max Iterations
20

## Reference Files
- `CODE/BLOWFISH.H` / `CODE/BLOWFISH.CPP` - Original Blowfish implementation
- `CODE/PK.H` / `CODE/PK.CPP` - Original RSA implementation
- `CODE/PKSTRAW.H` / `CODE/PKSTRAW.CPP` - Key decryption stream
- `CODE/CONST.CPP` - Public key definition (line 814)
- `CODE/MIXFILE.CPP` - MIX file loading with decryption (line 168-256)
