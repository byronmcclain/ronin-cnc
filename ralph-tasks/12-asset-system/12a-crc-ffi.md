# Task 12a: CRC32 FFI Export

## Dependencies
- Phases 01-11 must be complete
- Platform layer CRC implementation exists in `platform/src/util/crc.rs`

## Context
MIX files identify embedded files by CRC32 hash of their uppercase filename, not by the filename itself. The platform layer already has a CRC32 implementation, but it needs to be exposed via FFI so C++ code can calculate CRCs for file lookups. This is the foundational task - all other MIX file operations depend on this.

## Objective
Export the platform layer's CRC32 function to C++ via FFI, specifically tailored for MIX file lookups which require:
1. Converting filename to uppercase
2. Calculating CRC32 of the resulting bytes
3. Returning the CRC as a 32-bit unsigned integer

## Technical Background

### Original CRC Calculation (from CODE/MIXFILE.CPP:538)
```cpp
long crc = Calculate_CRC(strupr((char *)filename), strlen(filename));
```

The original code:
1. Converts filename to uppercase in-place with `strupr()`
2. Calculates CRC on the resulting bytes
3. Uses the CRC as a lookup key (entries are sorted by CRC for binary search)

### CRC Algorithm Details
The Westwood CRC32 algorithm has specific properties:
- Initial value: 0 (not 0xFFFFFFFF like standard CRC32)
- Polynomial: May differ from standard CRC32
- No final XOR

We need to verify our CRC matches the original game's CRC values for known files.

### Known File CRCs for Verification
These can be extracted from REDALERT.MIX header or calculated from known filenames:
| Filename | Expected CRC (verify with tools) |
|----------|----------------------------------|
| RULES.INI | (to be determined) |
| MOUSE.SHP | (to be determined) |
| TEMPERAT.PAL | (to be determined) |

## Deliverables
- [ ] FFI function `Platform_CRC_Calculate` in `platform/src/ffi/mod.rs`
- [ ] Helper function for MIX-style CRC (uppercase filename)
- [ ] Unit tests verifying CRC calculation
- [ ] C++ test that calculates CRC and verifies against known values
- [ ] Verification script passes

## Files to Create/Modify

### platform/src/util/crc.rs (VERIFY/MODIFY)
First, verify the existing CRC implementation matches Westwood's algorithm:

```rust
//! CRC32 calculation for MIX file lookups
//!
//! Westwood uses a specific CRC32 variant for identifying files in MIX archives.
//! The CRC is calculated on the uppercase ASCII filename (no path, no extension dot handling).

/// Calculate CRC32 hash of data using Westwood's algorithm
///
/// This implementation must match the original game's CRC calculation
/// for MIX file lookups to work correctly.
///
/// Key differences from standard CRC32:
/// - Initial value is 0 (not 0xFFFFFFFF)
/// - Uses specific polynomial (verify against original)
/// - No final XOR
pub fn calculate_crc32(data: &[u8]) -> u32 {
    // Westwood CRC32 lookup table
    // This table is pre-computed from the polynomial
    static CRC_TABLE: [u32; 256] = [
        0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
        0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
        0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
        0x09B64C2B, 0x7EB17CBD, 0xE7B82D09, 0x90BF1D9F,
        0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
        0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
        0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
        0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
        0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
        0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
        0x35B5A8FA, 0x42B2986C, 0xDBBBBBD6, 0xACBCCF40,
        0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
        0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
        0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
        0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
        0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
        0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
        0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
        0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
        0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
        0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
        0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
        0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
        0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
        0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
        0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
        0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
        0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7A9B,
        0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
        0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
        0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
        0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
        0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
        0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
        0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
        0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
        0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
        0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
        0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
        0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
        0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
        0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
        0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
        0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
        0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
        0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
        0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
        0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
        0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
        0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
        0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
        0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
        0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
        0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
        0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
        0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
        0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
        0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
        0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
        0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
        0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
        0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
        0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
        0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D,
    ];

    let mut crc: u32 = 0;
    for &byte in data {
        let index = ((crc ^ byte as u32) & 0xFF) as usize;
        crc = (crc >> 8) ^ CRC_TABLE[index];
    }
    crc
}

/// Calculate CRC32 for a MIX file lookup (uppercase ASCII)
///
/// Converts the filename to uppercase ASCII and calculates CRC32.
/// This matches the original game's behavior in MIXFILE.CPP.
pub fn calculate_mix_crc(filename: &str) -> u32 {
    // Convert to uppercase ASCII bytes
    let upper: Vec<u8> = filename.bytes().map(|b| b.to_ascii_uppercase()).collect();
    calculate_crc32(&upper)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_crc_empty() {
        assert_eq!(calculate_crc32(&[]), 0);
    }

    #[test]
    fn test_crc_basic() {
        // Simple test - CRC of single byte
        let crc = calculate_crc32(&[0x41]); // 'A'
        assert_ne!(crc, 0); // Should produce non-zero result
    }

    #[test]
    fn test_mix_crc_case_insensitive() {
        // MIX CRC should be case-insensitive
        let crc1 = calculate_mix_crc("RULES.INI");
        let crc2 = calculate_mix_crc("rules.ini");
        let crc3 = calculate_mix_crc("Rules.Ini");
        assert_eq!(crc1, crc2);
        assert_eq!(crc2, crc3);
    }

    #[test]
    fn test_mix_crc_different_files() {
        // Different filenames should produce different CRCs
        let crc1 = calculate_mix_crc("RULES.INI");
        let crc2 = calculate_mix_crc("MOUSE.SHP");
        assert_ne!(crc1, crc2);
    }
}
```

### platform/src/ffi/mod.rs (MODIFY)
Add CRC FFI exports:

```rust
// =============================================================================
// CRC32 FFI (for MIX file lookups)
// =============================================================================

use crate::util::crc;

/// Calculate CRC32 of raw bytes
///
/// # Safety
/// - `data` must point to valid memory of at least `len` bytes
/// - Returns 0 if data is null or len is 0
#[no_mangle]
pub unsafe extern "C" fn Platform_CRC_Calculate(
    data: *const u8,
    len: i32,
) -> u32 {
    if data.is_null() || len <= 0 {
        return 0;
    }
    let slice = std::slice::from_raw_parts(data, len as usize);
    crc::calculate_crc32(slice)
}

/// Calculate CRC32 for MIX file lookup (uppercase ASCII)
///
/// Converts the filename to uppercase and calculates CRC32.
/// This is the function to use for looking up files in MIX archives.
///
/// # Safety
/// - `filename` must be a valid null-terminated C string
/// - Returns 0 if filename is null
#[no_mangle]
pub unsafe extern "C" fn Platform_CRC_ForMixLookup(
    filename: *const std::os::raw::c_char,
) -> u32 {
    if filename.is_null() {
        return 0;
    }
    let c_str = std::ffi::CStr::from_ptr(filename);
    match c_str.to_str() {
        Ok(s) => crc::calculate_mix_crc(s),
        Err(_) => 0, // Invalid UTF-8
    }
}
```

### platform/src/lib.rs (VERIFY)
Ensure `util` module is declared and exports `crc`:

```rust
pub mod util;
```

### platform/src/util/mod.rs (CREATE if needed)
```rust
pub mod crc;
```

### src/test_crc.cpp (NEW)
Create a C++ test program to verify CRC calculation:

```cpp
// Test CRC32 calculation for MIX file lookups
#include <cstdio>
#include <cstdint>
#include <cstring>
#include "platform.h"

// Known CRC values from a working MIX file
// These should be extracted from an actual REDALERT.MIX file
struct KnownCRC {
    const char* filename;
    uint32_t expected_crc;
};

// Initially we'll test consistency, not exact values
// Exact values will be determined by reading a real MIX file
int main(int argc, char* argv[]) {
    printf("=== CRC32 Test for MIX Lookups ===\n\n");

    // Test 1: Null safety
    printf("Test 1: Null safety...\n");
    uint32_t null_crc = Platform_CRC_Calculate(nullptr, 0);
    if (null_crc != 0) {
        printf("FAIL: CRC of null should be 0, got %u\n", null_crc);
        return 1;
    }
    printf("  PASS: Null returns 0\n");

    // Test 2: Empty data
    printf("Test 2: Empty data...\n");
    uint8_t empty[1] = {0};
    uint32_t empty_crc = Platform_CRC_Calculate(empty, 0);
    if (empty_crc != 0) {
        printf("FAIL: CRC of empty should be 0, got %u\n", empty_crc);
        return 1;
    }
    printf("  PASS: Empty returns 0\n");

    // Test 3: Case insensitivity for MIX lookup
    printf("Test 3: Case insensitivity...\n");
    uint32_t crc_upper = Platform_CRC_ForMixLookup("RULES.INI");
    uint32_t crc_lower = Platform_CRC_ForMixLookup("rules.ini");
    uint32_t crc_mixed = Platform_CRC_ForMixLookup("Rules.Ini");

    if (crc_upper != crc_lower || crc_lower != crc_mixed) {
        printf("FAIL: CRCs should be case-insensitive\n");
        printf("  RULES.INI = %08X\n", crc_upper);
        printf("  rules.ini = %08X\n", crc_lower);
        printf("  Rules.Ini = %08X\n", crc_mixed);
        return 1;
    }
    printf("  PASS: All cases produce CRC %08X\n", crc_upper);

    // Test 4: Different filenames produce different CRCs
    printf("Test 4: Different files produce different CRCs...\n");
    uint32_t crc_rules = Platform_CRC_ForMixLookup("RULES.INI");
    uint32_t crc_mouse = Platform_CRC_ForMixLookup("MOUSE.SHP");
    uint32_t crc_palette = Platform_CRC_ForMixLookup("TEMPERAT.PAL");

    if (crc_rules == crc_mouse || crc_mouse == crc_palette || crc_rules == crc_palette) {
        printf("FAIL: Different files should have different CRCs\n");
        return 1;
    }
    printf("  PASS: Different CRCs for different files\n");
    printf("    RULES.INI    = %08X\n", crc_rules);
    printf("    MOUSE.SHP    = %08X\n", crc_mouse);
    printf("    TEMPERAT.PAL = %08X\n", crc_palette);

    // Test 5: Verify raw CRC calculation matches expected
    printf("Test 5: Raw CRC calculation...\n");
    const char* test_str = "TEST";
    uint32_t raw_crc = Platform_CRC_Calculate(
        reinterpret_cast<const uint8_t*>(test_str),
        static_cast<int>(strlen(test_str))
    );
    printf("  CRC of 'TEST' = %08X\n", raw_crc);

    printf("\n=== All CRC tests passed ===\n");
    return 0;
}
```

### CMakeLists.txt (MODIFY)
Add the test executable:

```cmake
# CRC test
add_executable(TestCRC src/test_crc.cpp)
target_include_directories(TestCRC PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(TestCRC PRIVATE redalert_platform)
```

## Verification Command
```bash
ralph-tasks/verify/check-crc-ffi.sh
```

## Verification Script

### ralph-tasks/verify/check-crc-ffi.sh (NEW)
```bash
#!/bin/bash
# Verification script for CRC32 FFI export

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking CRC32 FFI Export ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check that crc module exists
echo "[1/6] Checking crc module exists..."
if [ ! -f "platform/src/util/crc.rs" ]; then
    echo "VERIFY_FAILED: platform/src/util/crc.rs not found"
    exit 1
fi
echo "  OK: crc.rs exists"

# Step 2: Run Rust unit tests for CRC
echo "[2/6] Running Rust CRC tests..."
if ! cargo test --manifest-path platform/Cargo.toml -- crc --nocapture 2>&1; then
    echo "VERIFY_FAILED: Rust CRC tests failed"
    exit 1
fi
echo "  OK: Rust tests pass"

# Step 3: Build platform library
echo "[3/6] Building platform library..."
if ! cargo build --manifest-path platform/Cargo.toml --release 2>&1; then
    echo "VERIFY_FAILED: Platform build failed"
    exit 1
fi
echo "  OK: Platform builds"

# Step 4: Check that FFI exports are in header
echo "[4/6] Checking FFI exports in platform.h..."
if ! grep -q "Platform_CRC_Calculate" include/platform.h; then
    echo "VERIFY_FAILED: Platform_CRC_Calculate not found in platform.h"
    exit 1
fi
if ! grep -q "Platform_CRC_ForMixLookup" include/platform.h; then
    echo "VERIFY_FAILED: Platform_CRC_ForMixLookup not found in platform.h"
    exit 1
fi
echo "  OK: FFI exports present in header"

# Step 5: Build C++ test
echo "[5/6] Building C++ test..."
if ! cmake --build build --target TestCRC 2>&1; then
    echo "VERIFY_FAILED: TestCRC build failed"
    exit 1
fi
echo "  OK: TestCRC builds"

# Step 6: Run C++ test
echo "[6/6] Running C++ test..."
if ! ./build/TestCRC; then
    echo "VERIFY_FAILED: TestCRC failed"
    exit 1
fi
echo "  OK: TestCRC passes"

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Verify/update `platform/src/util/crc.rs` with CRC32 implementation
2. Ensure `platform/src/util/mod.rs` exports the crc module
3. Add `pub mod util;` to `platform/src/lib.rs` if not present
4. Add FFI exports to `platform/src/ffi/mod.rs`
5. Run `cargo test --manifest-path platform/Cargo.toml -- crc`
6. Run `cargo build --manifest-path platform/Cargo.toml --release`
7. Verify header contains exports: `grep Platform_CRC include/platform.h`
8. Create `src/test_crc.cpp`
9. Update `CMakeLists.txt` to build TestCRC
10. Run `cmake --build build --target TestCRC`
11. Run `./build/TestCRC`
12. Run verification script

## Success Criteria
- `Platform_CRC_Calculate` function exported and callable from C++
- `Platform_CRC_ForMixLookup` function handles uppercase conversion
- CRC calculation is case-insensitive for filenames
- Different filenames produce different CRCs
- All unit tests pass
- Verification script exits with 0

## Common Issues

### "cannot find module util"
- Add `pub mod util;` to `platform/src/lib.rs`
- Create `platform/src/util/mod.rs` with `pub mod crc;`

### "unresolved import crate::util::crc"
- Verify the module path in lib.rs and util/mod.rs

### CRC values don't match original game
- The CRC algorithm must exactly match Westwood's implementation
- Verify polynomial and initial value
- May need to extract known CRCs from a real MIX file for comparison

### Header doesn't contain new exports
- Run `cargo build --manifest-path platform/Cargo.toml --features cbindgen-run`
- Check that cbindgen.toml includes the new functions

## Notes on Westwood CRC Algorithm
The original Westwood CRC32 may differ from standard CRC-32/ISO-HDLC. Key points:
1. Initial value appears to be 0 (not 0xFFFFFFFF)
2. Uses standard polynomial 0xEDB88320 (reflected)
3. No final XOR

If CRCs don't match when testing with real MIX files, we may need to:
1. Use a hex editor to examine a real MIX file's header
2. Extract some CRC values from the SubBlock entries
3. Compute what CRC algorithm produces those values

## Completion Promise
When verification passes, output:
```
<promise>TASK_12A_COMPLETE</promise>
```

## Escape Hatch
If stuck after 15 iterations:
- Document what's blocking in `ralph-tasks/blocked/12a.md`
- Specific focus: if CRC values don't match original game, document the discrepancy
- List attempted approaches
- Output: `<promise>TASK_12A_BLOCKED</promise>`

## Max Iterations
15
