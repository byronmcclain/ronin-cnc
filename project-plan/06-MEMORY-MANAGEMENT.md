# Plan 06: Memory Management (Rust)

## Objective
Replace Windows memory allocation APIs with Rust's standard allocation, providing C-compatible allocation functions for the game code while leveraging Rust's memory safety guarantees.

## Current State Analysis

### Windows Memory APIs in Codebase

**Primary Files:**
- `WIN32LIB/MEM/MSVC/MEM.CPP`
- `CODE/ALLOC.CPP`
- `WIN32LIB/MEM/MEM.H`

**Windows APIs Used:**
```cpp
VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
VirtualFree(ptr, 0, MEM_RELEASE);
HeapCreate(), HeapAlloc(), HeapFree(), HeapDestroy();
GlobalAlloc(GMEM_FIXED, size);
GlobalFree(ptr);
```

## Rust Implementation Strategy

Rust's memory model is fundamentally different from C/C++:
- Rust manages memory through ownership and borrowing
- For FFI, we provide C-compatible allocation functions
- Use Rust's global allocator for actual allocation
- Track allocations for debugging (in debug builds)

### 6.1 Core Memory Module

File: `platform/src/memory/mod.rs`
```rust
//! Memory management module
//!
//! Provides C-compatible allocation functions backed by Rust's allocator.

use std::alloc::{Layout, alloc, dealloc, realloc};
use std::sync::atomic::{AtomicUsize, Ordering};

#[cfg(debug_assertions)]
use std::collections::HashMap;
#[cfg(debug_assertions)]
use std::sync::Mutex;
#[cfg(debug_assertions)]
use once_cell::sync::Lazy;

// =============================================================================
// Statistics
// =============================================================================

static TOTAL_ALLOCATED: AtomicUsize = AtomicUsize::new(0);
static ALLOCATION_COUNT: AtomicUsize = AtomicUsize::new(0);
static PEAK_ALLOCATED: AtomicUsize = AtomicUsize::new(0);

#[cfg(debug_assertions)]
static ALLOCATIONS: Lazy<Mutex<HashMap<usize, AllocationInfo>>> =
    Lazy::new(|| Mutex::new(HashMap::new()));

#[cfg(debug_assertions)]
struct AllocationInfo {
    size: usize,
    #[allow(dead_code)]
    file: &'static str,
    #[allow(dead_code)]
    line: u32,
}

/// Memory allocation flags (matching original game)
#[repr(C)]
pub struct MemFlags(u32);

impl MemFlags {
    pub const NORMAL: u32 = 0x0000;
    pub const TEMP: u32 = 0x0001;
    pub const LOCK: u32 = 0x0002;  // Ignored on modern systems
    pub const CLEAR: u32 = 0x0004;
}

// =============================================================================
// Allocation Functions
// =============================================================================

/// Allocate memory with flags
///
/// # Safety
/// Returns a valid pointer or null. Caller must free with `mem_free`.
pub fn mem_alloc(size: usize, flags: u32) -> *mut u8 {
    if size == 0 {
        return std::ptr::null_mut();
    }

    // Use proper alignment for the size
    let layout = match Layout::from_size_align(size, std::mem::align_of::<usize>()) {
        Ok(l) => l,
        Err(_) => return std::ptr::null_mut(),
    };

    unsafe {
        let ptr = alloc(layout);
        if ptr.is_null() {
            return ptr;
        }

        // Zero-initialize if requested
        if flags & MemFlags::CLEAR != 0 {
            std::ptr::write_bytes(ptr, 0, size);
        }

        // Track allocation
        track_alloc(ptr as usize, size);

        ptr
    }
}

/// Free allocated memory
///
/// # Safety
/// `ptr` must have been allocated by `mem_alloc` with the same size,
/// or be null (in which case this is a no-op).
pub fn mem_free(ptr: *mut u8, size: usize) {
    if ptr.is_null() {
        return;
    }

    track_free(ptr as usize);

    let layout = match Layout::from_size_align(size, std::mem::align_of::<usize>()) {
        Ok(l) => l,
        Err(_) => return,
    };

    unsafe {
        dealloc(ptr, layout);
    }
}

/// Reallocate memory
///
/// # Safety
/// `ptr` must have been allocated by `mem_alloc`, or be null.
pub fn mem_realloc(ptr: *mut u8, old_size: usize, new_size: usize) -> *mut u8 {
    if ptr.is_null() {
        return mem_alloc(new_size, MemFlags::NORMAL);
    }

    if new_size == 0 {
        mem_free(ptr, old_size);
        return std::ptr::null_mut();
    }

    track_free(ptr as usize);

    let old_layout = match Layout::from_size_align(old_size, std::mem::align_of::<usize>()) {
        Ok(l) => l,
        Err(_) => return std::ptr::null_mut(),
    };

    unsafe {
        let new_ptr = realloc(ptr, old_layout, new_size);
        if !new_ptr.is_null() {
            track_alloc(new_ptr as usize, new_size);
        }
        new_ptr
    }
}

// =============================================================================
// Tracking
// =============================================================================

fn track_alloc(addr: usize, size: usize) {
    let prev = TOTAL_ALLOCATED.fetch_add(size, Ordering::Relaxed);
    ALLOCATION_COUNT.fetch_add(1, Ordering::Relaxed);

    // Update peak
    let current = prev + size;
    let mut peak = PEAK_ALLOCATED.load(Ordering::Relaxed);
    while current > peak {
        match PEAK_ALLOCATED.compare_exchange_weak(
            peak, current, Ordering::Relaxed, Ordering::Relaxed
        ) {
            Ok(_) => break,
            Err(p) => peak = p,
        }
    }

    #[cfg(debug_assertions)]
    if let Ok(mut map) = ALLOCATIONS.lock() {
        map.insert(addr, AllocationInfo {
            size,
            file: "",
            line: 0,
        });
    }
}

fn track_free(addr: usize) {
    #[cfg(debug_assertions)]
    if let Ok(mut map) = ALLOCATIONS.lock() {
        if let Some(info) = map.remove(&addr) {
            TOTAL_ALLOCATED.fetch_sub(info.size, Ordering::Relaxed);
            ALLOCATION_COUNT.fetch_sub(1, Ordering::Relaxed);
        }
    }

    #[cfg(not(debug_assertions))]
    {
        let _ = addr; // Suppress warning
    }
}

// =============================================================================
// Statistics Access
// =============================================================================

/// Get total currently allocated bytes
pub fn get_allocated() -> usize {
    TOTAL_ALLOCATED.load(Ordering::Relaxed)
}

/// Get peak allocation (high water mark)
pub fn get_peak() -> usize {
    PEAK_ALLOCATED.load(Ordering::Relaxed)
}

/// Get current allocation count
pub fn get_count() -> usize {
    ALLOCATION_COUNT.load(Ordering::Relaxed)
}

/// Get estimated free RAM (returns large value for compatibility)
pub fn get_free_ram() -> usize {
    256 * 1024 * 1024 // 256 MB
}

/// Get total RAM (returns reasonable estimate)
pub fn get_total_ram() -> usize {
    512 * 1024 * 1024 // 512 MB
}

// =============================================================================
// Debug Functions
// =============================================================================

/// Dump memory leaks (debug builds only)
#[cfg(debug_assertions)]
pub fn dump_leaks() {
    if let Ok(map) = ALLOCATIONS.lock() {
        if map.is_empty() {
            log::info!("No memory leaks detected");
            return;
        }

        log::warn!("Memory leaks detected: {} allocations, {} bytes",
                   map.len(), TOTAL_ALLOCATED.load(Ordering::Relaxed));

        for (addr, info) in map.iter() {
            log::warn!("  Leak: 0x{:x} ({} bytes)", addr, info.size);
        }
    }
}

#[cfg(not(debug_assertions))]
pub fn dump_leaks() {
    // No-op in release builds
}
```

### 6.2 FFI Memory Exports

File: `platform/src/ffi/memory.rs`
```rust
//! Memory FFI exports

use crate::memory;
use std::ffi::c_void;

// =============================================================================
// Allocation Functions
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
// Memory Stats
// =============================================================================

/// Get free RAM (returns large value for compatibility)
#[no_mangle]
pub extern "C" fn Platform_Ram_Free() -> usize {
    memory::get_free_ram()
}

/// Get total RAM
#[no_mangle]
pub extern "C" fn Platform_Total_Ram_Free() -> usize {
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
// Memory Operations
// =============================================================================

/// Copy memory (memcpy)
#[no_mangle]
pub unsafe extern "C" fn Platform_MemCopy(dest: *mut c_void, src: *const c_void, count: usize) {
    if !dest.is_null() && !src.is_null() && count > 0 {
        std::ptr::copy_nonoverlapping(src as *const u8, dest as *mut u8, count);
    }
}

/// Move memory (memmove)
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

### 6.3 Compatibility Header

File: `include/compat/memory_wrapper.h` (used by game code)
```c
/* Memory Compatibility Layer */
#ifndef MEMORY_WRAPPER_H
#define MEMORY_WRAPPER_H

#include "platform.h"
#include <stddef.h>

/* Memory flags */
#define MEM_NORMAL  0x0000
#define MEM_TEMP    0x0001
#define MEM_LOCK    0x0002
#define MEM_CLEAR   0x0004

/* Windows API compatibility */
#define MEM_COMMIT      0x00001000
#define MEM_RESERVE     0x00002000
#define MEM_RELEASE     0x00008000
#define PAGE_READWRITE  0x04

/* Allocation functions */
#define Alloc(size, flags) Platform_Alloc(size, flags)
#define Free(ptr) Platform_Free(ptr, 0)

/* Windows compatibility */
static inline void* VirtualAlloc(void* addr, size_t size, unsigned int type, unsigned int protect) {
    (void)addr; (void)type; (void)protect;
    return Platform_Alloc(size, MEM_CLEAR);
}

static inline int VirtualFree(void* ptr, size_t size, unsigned int type) {
    (void)size; (void)type;
    Platform_Free(ptr, 0);
    return 1;
}

static inline int VirtualLock(void* ptr, size_t size) {
    (void)ptr; (void)size;
    return 1;
}

static inline int VirtualUnlock(void* ptr, size_t size) {
    (void)ptr; (void)size;
    return 1;
}

/* Heap functions (use global allocator) */
typedef void* HANDLE;

static inline HANDLE HeapCreate(unsigned int opts, size_t init, size_t max) {
    (void)opts; (void)init; (void)max;
    return (HANDLE)1;
}

static inline void* HeapAlloc(HANDLE heap, unsigned int flags, size_t size) {
    (void)heap;
    return Platform_Alloc(size, (flags & 0x08) ? MEM_CLEAR : MEM_NORMAL);
}

static inline int HeapFree(HANDLE heap, unsigned int flags, void* ptr) {
    (void)heap; (void)flags;
    Platform_Free(ptr, 0);
    return 1;
}

static inline int HeapDestroy(HANDLE heap) {
    (void)heap;
    return 1;
}

/* Memory macros */
#define ZeroMemory(ptr, size) Platform_ZeroMemory(ptr, size)
#define CopyMemory(d, s, n) Platform_MemCopy(d, s, n)
#define MoveMemory(d, s, n) Platform_MemMove(d, s, n)
#define FillMemory(d, n, v) Platform_MemSet(d, v, n)

#endif /* MEMORY_WRAPPER_H */
```

## Tasks Breakdown

### Phase 1: Core Allocation (1 day)
- [ ] Implement `mem_alloc()` and `mem_free()`
- [ ] Implement `mem_realloc()`
- [ ] Add allocation tracking
- [ ] Test basic allocation

### Phase 2: Statistics (0.5 days)
- [ ] Implement allocation counters
- [ ] Implement peak tracking
- [ ] Implement leak detection (debug)

### Phase 3: FFI Exports (0.5 days)
- [ ] Create FFI wrapper functions
- [ ] Implement memory operations
- [ ] Verify cbindgen output

### Phase 4: Integration (1 day)
- [ ] Create compatibility header
- [ ] Test with game code
- [ ] Verify no memory issues

## Acceptance Criteria

- [ ] Allocation/free works correctly
- [ ] Statistics tracking accurate
- [ ] Leak detection works (debug)
- [ ] Game runs without crashes
- [ ] Compatible with original API

## Estimated Duration
**2-3 days**

## Dependencies
- Plan 01 (Build System) completed
- Rust standard library

## Next Plan
Once memory management is working, proceed to **Plan 07: File System**
