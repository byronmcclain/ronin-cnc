# Plan 06: Memory Management

## Objective
Replace Windows memory allocation APIs with standard C/C++ allocation, maintaining compatibility with the game's memory allocation wrappers.

## Current State Analysis

### Windows Memory APIs in Codebase

**Primary Files:**
- `WIN32LIB/MEM/MSVC/MEM.CPP`
- `CODE/ALLOC.CPP`
- `WIN32LIB/MEM/MEM.H`

**Windows APIs Used:**
```cpp
// Virtual Memory
VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
VirtualFree(ptr, 0, MEM_RELEASE);
VirtualLock(ptr, size);  // Lock in physical memory
VirtualUnlock(ptr, size);

// Heap
HeapCreate(0, initial_size, max_size);
HeapAlloc(heap, 0, size);
HeapFree(heap, 0, ptr);
HeapDestroy(heap);

// Global (16-bit legacy)
GlobalAlloc(GMEM_FIXED, size);
GlobalFree(ptr);
```

**Game Memory Flags:**
```cpp
#define MEM_NORMAL  0x0000  // Normal allocation
#define MEM_TEMP    0x0001  // Temporary (short-lived)
#define MEM_LOCK    0x0002  // Lock in memory (no swap)
#define MEM_CLEAR   0x0004  // Zero-initialize
```

**Memory Functions:**
```cpp
void* Alloc(size_t size, MemoryFlagType flags);
void Free(void* ptr);
void* Resize_Alloc(void* ptr, size_t new_size);
size_t Ram_Free();       // Get available memory
size_t Total_Ram_Free(); // Get total free RAM
```

## Implementation Strategy

Since we're running on modern macOS with virtual memory, we can simplify significantly:

1. **Use standard allocation**: `malloc()`/`free()` or `new`/`delete`
2. **Ignore MEM_LOCK**: Modern OSes handle this automatically
3. **Honor MEM_CLEAR**: Use `calloc()` or `memset()`
4. **Track allocations**: For debugging memory leaks

### 6.1 Core Memory Implementation

File: `src/platform/memory.cpp`
```cpp
#include "platform/platform.h"
#include <cstdlib>
#include <cstring>
#include <atomic>
#include <mutex>
#include <unordered_map>

namespace {

//=============================================================================
// Memory Statistics
//=============================================================================

std::atomic<size_t> g_total_allocated{0};
std::atomic<size_t> g_allocation_count{0};
std::atomic<size_t> g_peak_allocated{0};

#ifdef DEBUG
// Track allocations for leak detection
std::mutex g_alloc_mutex;
std::unordered_map<void*, size_t> g_allocations;
#endif

void Track_Alloc(void* ptr, size_t size) {
    g_total_allocated += size;
    g_allocation_count++;

    size_t current = g_total_allocated.load();
    size_t peak = g_peak_allocated.load();
    while (current > peak && !g_peak_allocated.compare_exchange_weak(peak, current)) {
        peak = g_peak_allocated.load();
    }

#ifdef DEBUG
    std::lock_guard<std::mutex> lock(g_alloc_mutex);
    g_allocations[ptr] = size;
#endif
}

void Track_Free(void* ptr) {
#ifdef DEBUG
    std::lock_guard<std::mutex> lock(g_alloc_mutex);
    auto it = g_allocations.find(ptr);
    if (it != g_allocations.end()) {
        g_total_allocated -= it->second;
        g_allocation_count--;
        g_allocations.erase(it);
    }
#endif
}

} // anonymous namespace

//=============================================================================
// Memory Flag Types
//=============================================================================

typedef unsigned int MemoryFlagType;
#define MEM_NORMAL  0x0000
#define MEM_TEMP    0x0001
#define MEM_LOCK    0x0002
#define MEM_CLEAR   0x0004

//=============================================================================
// Allocation Functions
//=============================================================================

void* Alloc(size_t size, MemoryFlagType flags) {
    if (size == 0) return nullptr;

    void* ptr;

    if (flags & MEM_CLEAR) {
        ptr = calloc(1, size);
    } else {
        ptr = malloc(size);
    }

    if (ptr) {
        Track_Alloc(ptr, size);
    }

    return ptr;
}

void Free(void* ptr) {
    if (ptr) {
        Track_Free(ptr);
        free(ptr);
    }
}

void* Resize_Alloc(void* ptr, size_t new_size) {
    if (ptr == nullptr) {
        return Alloc(new_size, MEM_NORMAL);
    }

    if (new_size == 0) {
        Free(ptr);
        return nullptr;
    }

#ifdef DEBUG
    // Update tracking
    {
        std::lock_guard<std::mutex> lock(g_alloc_mutex);
        auto it = g_allocations.find(ptr);
        if (it != g_allocations.end()) {
            g_total_allocated -= it->second;
            g_allocations.erase(it);
        }
    }
#endif

    void* new_ptr = realloc(ptr, new_size);

    if (new_ptr) {
        Track_Alloc(new_ptr, new_size);
    }

    return new_ptr;
}

//=============================================================================
// Memory Information
//=============================================================================

size_t Ram_Free() {
    // On modern systems, this isn't really meaningful
    // Return a large value to indicate plenty of memory
    return 256 * 1024 * 1024;  // 256 MB
}

size_t Total_Ram_Free() {
    // Could use sysctl on macOS to get actual free memory
    // For game purposes, just return a reasonable value
    return 512 * 1024 * 1024;  // 512 MB
}

size_t Mem_Get_Allocated() {
    return g_total_allocated.load();
}

size_t Mem_Get_Peak() {
    return g_peak_allocated.load();
}

size_t Mem_Get_Count() {
    return g_allocation_count.load();
}

//=============================================================================
// Debug Functions
//=============================================================================

#ifdef DEBUG
void Mem_Dump_Leaks() {
    std::lock_guard<std::mutex> lock(g_alloc_mutex);

    if (g_allocations.empty()) {
        Platform_Log(LOG_INFO, "No memory leaks detected");
        return;
    }

    Platform_Log(LOG_WARN, "Memory leaks detected: %zu allocations, %zu bytes",
                 g_allocations.size(), g_total_allocated.load());

    for (const auto& pair : g_allocations) {
        Platform_Log(LOG_WARN, "  Leak: %p (%zu bytes)", pair.first, pair.second);
    }
}
#endif
```

### 6.2 Large Allocation Support

For very large allocations (video memory, etc.), use mmap:

```cpp
#include <sys/mman.h>

void* Alloc_Large(size_t size) {
    void* ptr = mmap(nullptr, size,
                     PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANON,
                     -1, 0);

    if (ptr == MAP_FAILED) {
        return nullptr;
    }

    Track_Alloc(ptr, size);
    return ptr;
}

void Free_Large(void* ptr, size_t size) {
    if (ptr) {
        Track_Free(ptr);
        munmap(ptr, size);
    }
}
```

### 6.3 Windows Compatibility Wrappers

File: `include/compat/memory_wrapper.h`
```cpp
#ifndef MEMORY_WRAPPER_H
#define MEMORY_WRAPPER_H

#include <cstdlib>
#include <cstring>

//=============================================================================
// VirtualAlloc / VirtualFree
//=============================================================================

#define MEM_COMMIT      0x00001000
#define MEM_RESERVE     0x00002000
#define MEM_RELEASE     0x00008000
#define PAGE_READWRITE  0x04

inline void* VirtualAlloc(void* address, size_t size,
                          unsigned int alloc_type, unsigned int protect) {
    (void)address;  // Ignore requested address
    (void)alloc_type;
    (void)protect;
    return calloc(1, size);
}

inline bool VirtualFree(void* ptr, size_t size, unsigned int free_type) {
    (void)size;
    (void)free_type;
    free(ptr);
    return true;
}

inline bool VirtualLock(void* ptr, size_t size) {
    // No-op on modern systems
    (void)ptr;
    (void)size;
    return true;
}

inline bool VirtualUnlock(void* ptr, size_t size) {
    (void)ptr;
    (void)size;
    return true;
}

//=============================================================================
// Heap Functions
//=============================================================================

typedef void* HANDLE;

inline HANDLE HeapCreate(unsigned int options, size_t initial, size_t max) {
    (void)options;
    (void)initial;
    (void)max;
    return (HANDLE)1;  // Dummy handle, we use global heap
}

inline void* HeapAlloc(HANDLE heap, unsigned int flags, size_t size) {
    (void)heap;
    (void)flags;
    return malloc(size);
}

inline bool HeapFree(HANDLE heap, unsigned int flags, void* ptr) {
    (void)heap;
    (void)flags;
    free(ptr);
    return true;
}

inline bool HeapDestroy(HANDLE heap) {
    (void)heap;
    return true;
}

//=============================================================================
// Global Memory (16-bit legacy)
//=============================================================================

#define GMEM_FIXED      0x0000
#define GMEM_MOVEABLE   0x0002
#define GMEM_ZEROINIT   0x0040

typedef void* HGLOBAL;

inline HGLOBAL GlobalAlloc(unsigned int flags, size_t size) {
    if (flags & GMEM_ZEROINIT) {
        return calloc(1, size);
    }
    return malloc(size);
}

inline HGLOBAL GlobalFree(HGLOBAL ptr) {
    free(ptr);
    return nullptr;
}

inline void* GlobalLock(HGLOBAL ptr) {
    return ptr;  // Already a pointer in our implementation
}

inline bool GlobalUnlock(HGLOBAL ptr) {
    (void)ptr;
    return true;
}

//=============================================================================
// Memory Copy/Move/Set
//=============================================================================

// These are already standard C, but ensure they're available
#ifndef memcpy
#define memcpy std::memcpy
#endif

#ifndef memmove
#define memmove std::memmove
#endif

#ifndef memset
#define memset std::memset
#endif

#ifndef memcmp
#define memcmp std::memcmp
#endif

//=============================================================================
// ZeroMemory / CopyMemory
//=============================================================================

inline void ZeroMemory(void* ptr, size_t size) {
    memset(ptr, 0, size);
}

inline void CopyMemory(void* dest, const void* src, size_t size) {
    memcpy(dest, src, size);
}

inline void MoveMemory(void* dest, const void* src, size_t size) {
    memmove(dest, src, size);
}

inline void FillMemory(void* dest, size_t size, unsigned char value) {
    memset(dest, value, size);
}

#endif // MEMORY_WRAPPER_H
```

### 6.4 Fast Memory Operations

For performance-critical code that originally used assembly:

File: `src/platform/memcpy_fast.cpp`
```cpp
#include <cstring>
#include <cstdint>

// These replace assembly versions in WIN32LIB/MEM/MEM_COPY.ASM

extern "C" {

void Mem_Copy(void* dest, const void* src, size_t count) {
    memcpy(dest, src, count);
}

void Mem_Set(void* dest, int value, size_t count) {
    memset(dest, value, count);
}

// Word-aligned copy (for 16-bit values)
void Mem_Copy_Word(void* dest, const void* src, size_t word_count) {
    memcpy(dest, src, word_count * 2);
}

// DWord-aligned copy (for 32-bit values)
void Mem_Copy_DWord(void* dest, const void* src, size_t dword_count) {
    memcpy(dest, src, dword_count * 4);
}

} // extern "C"
```

## Tasks Breakdown

### Phase 1: Core Allocation (1 day)
- [ ] Implement `Alloc()` and `Free()`
- [ ] Implement `Resize_Alloc()`
- [ ] Add allocation tracking
- [ ] Test basic allocation

### Phase 2: Compatibility Wrappers (1 day)
- [ ] Create `memory_wrapper.h`
- [ ] Map `VirtualAlloc`/`VirtualFree`
- [ ] Map `HeapAlloc`/`HeapFree`
- [ ] Map `GlobalAlloc`/`GlobalFree`

### Phase 3: Memory Utilities (0.5 days)
- [ ] Implement `Ram_Free()` (return reasonable value)
- [ ] Implement `ZeroMemory`/`CopyMemory`
- [ ] Implement fast memory copy

### Phase 4: Debug Support (0.5 days)
- [ ] Implement leak detection
- [ ] Add memory statistics
- [ ] Test leak reporting

## Testing Strategy

### Test 1: Basic Allocation
Allocate and free various sizes.

### Test 2: Stress Test
Many allocations/deallocations in loop.

### Test 3: Leak Detection
Intentionally leak, verify detection.

### Test 4: Game Integration
Run game, verify no memory issues.

## Acceptance Criteria

- [ ] `Alloc()`/`Free()` work correctly
- [ ] Windows compatibility macros compile
- [ ] No memory leaks in basic operations
- [ ] Game runs without memory errors
- [ ] Memory statistics available for debugging

## Estimated Duration
**2-3 days**

## Dependencies
- Plan 01 (Build System) completed
- Standard C library available

## Next Plan
Once memory management is working, proceed to **Plan 07: File System**
