# Task 06a: Core Allocation Functions

## Dependencies
- Task 05d must be complete (Timing System)
- Phase 01 Build System complete

## Context
The memory management system provides C-compatible allocation functions backed by Rust's standard allocator. This replaces Windows VirtualAlloc/HeapAlloc APIs with cross-platform equivalents.

## Objective
Create the memory module with core allocation functions: `mem_alloc()`, `mem_free()`, and `mem_realloc()`. Include basic allocation tracking for statistics.

## Deliverables
- [ ] `platform/src/memory/mod.rs` - Core memory module
- [ ] `mem_alloc()`, `mem_free()`, `mem_realloc()` functions
- [ ] Basic allocation tracking (count, total bytes)
- [ ] Verification passes

## Files to Create/Modify

### platform/src/memory/mod.rs (NEW)
```rust
//! Memory management module
//!
//! Provides C-compatible allocation functions backed by Rust's allocator.

use std::alloc::{Layout, alloc, dealloc, realloc};
use std::sync::atomic::{AtomicUsize, Ordering};

// =============================================================================
// Statistics (basic tracking)
// =============================================================================

static TOTAL_ALLOCATED: AtomicUsize = AtomicUsize::new(0);
static ALLOCATION_COUNT: AtomicUsize = AtomicUsize::new(0);
static PEAK_ALLOCATED: AtomicUsize = AtomicUsize::new(0);

/// Memory allocation flags (matching original game)
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct MemFlags;

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
        track_alloc(size);

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

    if size > 0 {
        track_free(size);

        let layout = match Layout::from_size_align(size, std::mem::align_of::<usize>()) {
            Ok(l) => l,
            Err(_) => return,
        };

        unsafe {
            dealloc(ptr, layout);
        }
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

    if old_size == 0 {
        return mem_alloc(new_size, MemFlags::NORMAL);
    }

    let old_layout = match Layout::from_size_align(old_size, std::mem::align_of::<usize>()) {
        Ok(l) => l,
        Err(_) => return std::ptr::null_mut(),
    };

    unsafe {
        let new_ptr = realloc(ptr, old_layout, new_size);
        if !new_ptr.is_null() {
            // Update tracking
            track_free(old_size);
            track_alloc(new_size);
        }
        new_ptr
    }
}

// =============================================================================
// Basic Tracking
// =============================================================================

fn track_alloc(size: usize) {
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
}

fn track_free(size: usize) {
    TOTAL_ALLOCATED.fetch_sub(size, Ordering::Relaxed);
    ALLOCATION_COUNT.fetch_sub(1, Ordering::Relaxed);
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
```

### platform/src/lib.rs (MODIFY)
Add after `pub mod timer;`:
```rust
pub mod memory;
```

## Verification Command
```bash
cd /Users/ooartist/Src/Ronin_CnC && \
cargo build --manifest-path platform/Cargo.toml 2>&1 && \
grep -q "pub fn mem_alloc" platform/src/memory/mod.rs && \
grep -q "pub fn mem_free" platform/src/memory/mod.rs && \
grep -q "pub fn mem_realloc" platform/src/memory/mod.rs && \
echo "VERIFY_SUCCESS"
```

## Implementation Steps
1. Create `platform/src/memory/mod.rs` with allocation functions
2. Add `pub mod memory;` to `platform/src/lib.rs`
3. Implement `mem_alloc()` with Layout-based allocation
4. Implement `mem_free()` with proper deallocation
5. Implement `mem_realloc()` for resizing
6. Add basic tracking with atomic counters
7. Build and verify

## Completion Promise
When verification passes, output:
```
<promise>TASK_06A_COMPLETE</promise>
```

## Escape Hatch
If stuck after 10 iterations:
- Document what's blocking in `ralph-tasks/blocked/06a.md`
- List attempted approaches
- Output: `<promise>TASK_06A_BLOCKED</promise>`

## Max Iterations
10
