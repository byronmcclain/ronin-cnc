# Task 06c: FFI Exports and Memory Operations

## Dependencies
- Task 06b must be complete (Debug Tracking)

## Context
The game code needs C-callable allocation functions and memory operations (memcpy, memmove, memset, memcmp). This task exposes the memory module via FFI.

## Objective
Add FFI exports for allocation functions and implement memory operations. Generate the C header with cbindgen.

## Deliverables
- [ ] FFI exports for `Platform_Alloc`, `Platform_Free`, `Platform_Realloc`
- [ ] Memory stats FFI: `Platform_Mem_GetAllocated`, `Platform_Mem_GetPeak`, etc.
- [ ] Memory operations: `Platform_MemCopy`, `Platform_MemMove`, `Platform_MemSet`, `Platform_MemCmp`
- [ ] C header updated with memory functions
- [ ] Verification passes

## Files to Modify

### platform/src/ffi/mod.rs (MODIFY)
Add memory FFI exports:
```rust
use crate::memory;

// =============================================================================
// Memory Allocation FFI
// =============================================================================

/// Allocate memory
#[no_mangle]
pub extern "C" fn Platform_Alloc(size: usize, flags: u32) -> *mut c_void {
    memory::mem_alloc(size, flags) as *mut c_void
}

/// Free memory
#[no_mangle]
pub extern "C" fn Platform_Free(ptr: *mut c_void, size: usize) {
    memory::mem_free(ptr as *mut u8, size);
}

/// Reallocate memory
#[no_mangle]
pub extern "C" fn Platform_Realloc(ptr: *mut c_void, old_size: usize, new_size: usize) -> *mut c_void {
    memory::mem_realloc(ptr as *mut u8, old_size, new_size) as *mut c_void
}

// =============================================================================
// Memory Stats FFI
// =============================================================================

/// Get free RAM (returns large value for compatibility)
#[no_mangle]
pub extern "C" fn Platform_Ram_Free() -> usize {
    memory::get_free_ram()
}

/// Get total RAM
#[no_mangle]
pub extern "C" fn Platform_Ram_Total() -> usize {
    memory::get_total_ram()
}

/// Get current allocation total
#[no_mangle]
pub extern "C" fn Platform_Mem_GetAllocated() -> usize {
    memory::get_allocated()
}

/// Get peak allocation
#[no_mangle]
pub extern "C" fn Platform_Mem_GetPeak() -> usize {
    memory::get_peak()
}

/// Get allocation count
#[no_mangle]
pub extern "C" fn Platform_Mem_GetCount() -> usize {
    memory::get_count()
}

/// Dump leaks (debug only)
#[no_mangle]
pub extern "C" fn Platform_Mem_DumpLeaks() {
    memory::dump_leaks();
}

// =============================================================================
// Memory Operations FFI
// =============================================================================

/// Copy memory (memcpy) - non-overlapping
#[no_mangle]
pub unsafe extern "C" fn Platform_MemCopy(dest: *mut c_void, src: *const c_void, count: usize) {
    if !dest.is_null() && !src.is_null() && count > 0 {
        std::ptr::copy_nonoverlapping(src as *const u8, dest as *mut u8, count);
    }
}

/// Move memory (memmove) - handles overlapping
#[no_mangle]
pub unsafe extern "C" fn Platform_MemMove(dest: *mut c_void, src: *const c_void, count: usize) {
    if !dest.is_null() && !src.is_null() && count > 0 {
        std::ptr::copy(src as *const u8, dest as *mut u8, count);
    }
}

/// Set memory (memset)
#[no_mangle]
pub unsafe extern "C" fn Platform_MemSet(dest: *mut c_void, value: i32, count: usize) {
    if !dest.is_null() && count > 0 {
        std::ptr::write_bytes(dest as *mut u8, value as u8, count);
    }
}

/// Compare memory (memcmp)
#[no_mangle]
pub unsafe extern "C" fn Platform_MemCmp(s1: *const c_void, s2: *const c_void, count: usize) -> i32 {
    if s1.is_null() || s2.is_null() || count == 0 {
        return 0;
    }

    let slice1 = std::slice::from_raw_parts(s1 as *const u8, count);
    let slice2 = std::slice::from_raw_parts(s2 as *const u8, count);

    for i in 0..count {
        let diff = slice1[i] as i32 - slice2[i] as i32;
        if diff != 0 {
            return diff;
        }
    }
    0
}

/// Zero memory
#[no_mangle]
pub unsafe extern "C" fn Platform_ZeroMemory(dest: *mut c_void, count: usize) {
    Platform_MemSet(dest, 0, count);
}
```

## Verification Command
```bash
cd /Users/ooartist/Src/Ronin_CnC && \
cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1 && \
grep -q "Platform_Alloc" include/platform.h && \
grep -q "Platform_Free" include/platform.h && \
grep -q "Platform_MemCopy" include/platform.h && \
grep -q "Platform_Mem_GetAllocated" include/platform.h && \
echo "VERIFY_SUCCESS"
```

## Implementation Steps
1. Add `use crate::memory;` to ffi/mod.rs
2. Add allocation FFI exports (Alloc, Free, Realloc)
3. Add memory stats FFI exports
4. Add memory operations FFI (MemCopy, MemMove, MemSet, MemCmp, ZeroMemory)
5. Build with cbindgen to generate header
6. Verify header contains all memory functions

## Completion Promise
When verification passes, output:
```
<promise>TASK_06C_COMPLETE</promise>
```

## Escape Hatch
If stuck after 10 iterations:
- Document what's blocking in `ralph-tasks/blocked/06c.md`
- List attempted approaches
- Output: `<promise>TASK_06C_BLOCKED</promise>`

## Max Iterations
10
