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

    if size > 0 {
        track_free(ptr as usize, size);

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
            track_free(ptr as usize, old_size);
            track_alloc(new_ptr as usize, new_size);
        }
        new_ptr
    }
}

// =============================================================================
// Basic Tracking
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

    #[cfg(not(debug_assertions))]
    let _ = addr;
}

fn track_free(addr: usize, size: usize) {
    TOTAL_ALLOCATED.fetch_sub(size, Ordering::Relaxed);
    ALLOCATION_COUNT.fetch_sub(1, Ordering::Relaxed);

    #[cfg(debug_assertions)]
    if let Ok(mut map) = ALLOCATIONS.lock() {
        map.remove(&addr);
    }

    #[cfg(not(debug_assertions))]
    {
        let _ = addr;
        let _ = size;
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
// Leak Detection
// =============================================================================

/// Dump memory leaks (debug builds only)
#[cfg(debug_assertions)]
pub fn dump_leaks() {
    if let Ok(map) = ALLOCATIONS.lock() {
        if map.is_empty() {
            eprintln!("[INFO] No memory leaks detected");
            return;
        }

        eprintln!("[WARN] Memory leaks detected: {} allocations, {} bytes",
                  map.len(), TOTAL_ALLOCATED.load(Ordering::Relaxed));

        for (addr, info) in map.iter() {
            eprintln!("  Leak: 0x{:x} ({} bytes)", addr, info.size);
        }
    }
}

#[cfg(not(debug_assertions))]
pub fn dump_leaks() {
    // No-op in release builds
}
