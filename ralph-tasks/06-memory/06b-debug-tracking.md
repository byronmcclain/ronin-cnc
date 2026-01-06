# Task 06b: Debug Tracking and Leak Detection

## Dependencies
- Task 06a must be complete (Core Allocation)

## Context
Debug builds need detailed allocation tracking to detect memory leaks. This task adds per-allocation tracking in debug builds and a leak dump function.

## Objective
Add debug-only allocation tracking using a HashMap to store allocation info. Implement `dump_leaks()` to report unfreed allocations at shutdown.

## Deliverables
- [ ] Debug-only allocation HashMap tracking
- [ ] `AllocationInfo` struct with size, file, line
- [ ] `dump_leaks()` function
- [ ] Verification passes

## Files to Modify

### platform/src/memory/mod.rs (MODIFY)
Add debug tracking after the imports:
```rust
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
```

Update `track_alloc()` to record allocations in debug builds:
```rust
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

fn track_free(addr: usize, size: usize) {
    TOTAL_ALLOCATED.fetch_sub(size, Ordering::Relaxed);
    ALLOCATION_COUNT.fetch_sub(1, Ordering::Relaxed);

    #[cfg(debug_assertions)]
    if let Ok(mut map) = ALLOCATIONS.lock() {
        map.remove(&addr);
    }

    #[cfg(not(debug_assertions))]
    {
        let _ = addr; // Suppress warning
        let _ = size;
    }
}
```

Add leak detection function:
```rust
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
```

Update `mem_alloc()` and `mem_free()` to pass address to tracking:
```rust
// In mem_alloc(), change:
track_alloc(ptr as usize, size);

// In mem_free(), change:
track_free(ptr as usize, size);

// In mem_realloc(), change:
track_free(ptr as usize, old_size);
track_alloc(new_ptr as usize, new_size);
```

## Verification Command
```bash
cd /Users/ooartist/Src/Ronin_CnC && \
cargo build --manifest-path platform/Cargo.toml 2>&1 && \
grep -q "dump_leaks" platform/src/memory/mod.rs && \
grep -q "AllocationInfo" platform/src/memory/mod.rs && \
echo "VERIFY_SUCCESS"
```

## Implementation Steps
1. Add debug-only imports (HashMap, Mutex, Lazy)
2. Add `ALLOCATIONS` static HashMap
3. Add `AllocationInfo` struct
4. Update `track_alloc()` to record address in HashMap
5. Update `track_free()` to remove from HashMap
6. Implement `dump_leaks()` function
7. Update allocation functions to pass addresses
8. Build and verify

## Completion Promise
When verification passes, output:
```
<promise>TASK_06B_COMPLETE</promise>
```

## Escape Hatch
If stuck after 10 iterations:
- Document what's blocking in `ralph-tasks/blocked/06b.md`
- List attempted approaches
- Output: `<promise>TASK_06B_BLOCKED</promise>`

## Max Iterations
10
