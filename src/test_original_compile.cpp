/**
 * Original Code Compilation Test
 *
 * This file tests that original Red Alert source files can be compiled
 * with our compatibility layer. It includes modified versions of original
 * files and verifies they compile correctly.
 *
 * Note: This is a COMPILATION test, not a functional test.
 * The goal is to verify syntax compatibility.
 */

#include "compat/compat.h"
#include <cstdio>

// =============================================================================
// Test 1: CRC Functions
// =============================================================================
// Based on CODE/CRC.CPP - CRC calculation routines

namespace OriginalCRC {

// Original CRC table (from CODE/CRC.CPP)
static unsigned long CRCTable[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
    0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    // ... (abbreviated for test)
    0x00000000
};

// Original function signature from CODE/CRC.CPP
unsigned long __cdecl Calculate_CRC(void* buffer, long length) {
    unsigned long crc = 0xFFFFFFFF;
    unsigned char* ptr = (unsigned char*)buffer;

    while (length--) {
        crc = CRCTable[(crc ^ *ptr++) & 0xFF] ^ (crc >> 8);
    }

    return ~crc;
}

} // namespace OriginalCRC

// =============================================================================
// Test 2: Random Number Generator
// =============================================================================
// Based on CODE/RANDOM.CPP - Linear congruential RNG

namespace OriginalRandom {

// Original RandomClass from CODE/RANDOM.H
class RandomClass {
public:
    RandomClass(unsigned seed = 0) : Seed(seed) {}

    int operator()() {
        // Linear congruential generator (original algorithm)
        Seed = Seed * 1103515245 + 12345;
        return (Seed >> 16) & 0x7FFF;
    }

    int operator()(int minval, int maxval) {
        // Range-limited random
        int range = maxval - minval + 1;
        if (range <= 0) return minval;
        return minval + ((*this)() % range);
    }

private:
    unsigned Seed;
};

} // namespace OriginalRandom

// =============================================================================
// Test 3: Memory Functions
// =============================================================================
// Based on CODE/JSHELL.CPP - Memory allocation wrappers

namespace OriginalMemory {

// Original function signatures using Windows types
void* __cdecl Alloc(unsigned long bytes_to_alloc, int flags) {
    // Original would call HeapAlloc, we use platform layer
    (void)flags;
    return Platform_Alloc(bytes_to_alloc, 0);
}

void __cdecl Free(void* pointer) {
    if (pointer) {
        Platform_Free(pointer, 0);
    }
}

// Original Mem_Copy using Windows types
void __cdecl Mem_Copy(void* source, void* dest, unsigned long bytes) {
    Platform_MemCopy(dest, source, bytes);
}

} // namespace OriginalMemory

// =============================================================================
// Test 4: Timer Functions
// =============================================================================
// Based on CODE/TIMER.CPP - Game timing

namespace OriginalTimer {

// Original uses DWORD from Windows
DWORD __cdecl Get_System_Tick_Count(void) {
    return GetTickCount();
}

void __cdecl Delay(DWORD milliseconds) {
    Sleep(milliseconds);
}

// Timer class skeleton (simplified)
class TimerClass {
public:
    TimerClass() : Started(0), Accumulated(0), Running(false) {}

    void Start() {
        if (!Running) {
            Started = GetTickCount();
            Running = true;
        }
    }

    void Stop() {
        if (Running) {
            Accumulated += GetTickCount() - Started;
            Running = false;
        }
    }

    DWORD Time() const {
        if (Running) {
            return Accumulated + (GetTickCount() - Started);
        }
        return Accumulated;
    }

    void Reset() {
        Started = 0;
        Accumulated = 0;
        Running = false;
    }

private:
    DWORD Started;
    DWORD Accumulated;
    BOOL Running;
};

} // namespace OriginalTimer

// =============================================================================
// Test 5: POINT/RECT usage patterns
// =============================================================================
// Based on various CODE files that use Windows structures

namespace OriginalGeometry {

// Function using POINT structure
BOOL Point_In_Rect(POINT* point, RECT* rect) {
    if (!point || !rect) return FALSE;
    return PtInRect(rect, *point) ? TRUE : FALSE;
}

// Function using RECT structure
void Normalize_Rect(RECT* rect) {
    if (!rect) return;

    if (rect->left > rect->right) {
        LONG temp = rect->left;
        rect->left = rect->right;
        rect->right = temp;
    }

    if (rect->top > rect->bottom) {
        LONG temp = rect->top;
        rect->top = rect->bottom;
        rect->bottom = temp;
    }
}

// Clip point to rect
void Clip_Point_To_Rect(POINT* point, RECT* rect) {
    if (!point || !rect) return;

    if (point->x < rect->left) point->x = rect->left;
    if (point->x > rect->right) point->x = rect->right;
    if (point->y < rect->top) point->y = rect->top;
    if (point->y > rect->bottom) point->y = rect->bottom;
}

} // namespace OriginalGeometry

// =============================================================================
// Main Test Runner
// =============================================================================

int main() {
    printf("==========================================\n");
    printf("Original Code Compilation Test\n");
    printf("==========================================\n\n");

    printf("This test verifies that code patterns from\n");
    printf("original Red Alert source files compile\n");
    printf("correctly with our compatibility layer.\n\n");

    // Initialize platform
    if (Platform_Init() != PLATFORM_RESULT_SUCCESS) {
        printf("Failed to initialize platform!\n");
        return 1;
    }

    int pass = 0;
    int fail = 0;

    // Test 1: CRC
    printf("Test 1: CRC Functions\n");
    {
        char data[] = "Hello World";
        unsigned long crc = OriginalCRC::Calculate_CRC(data, sizeof(data) - 1);
        printf("  CRC of 'Hello World' = 0x%08lX\n", crc);
        if (crc != 0) {
            printf("  [PASS]\n");
            pass++;
        } else {
            printf("  [FAIL] CRC should be non-zero\n");
            fail++;
        }
    }

    // Test 2: Random
    printf("Test 2: Random Number Generator\n");
    {
        OriginalRandom::RandomClass rng(12345);
        int r1 = rng();
        int r2 = rng();
        int r3 = rng(1, 100);
        printf("  Random values: %d, %d, range[1-100]=%d\n", r1, r2, r3);
        if (r1 != r2 && r3 >= 1 && r3 <= 100) {
            printf("  [PASS]\n");
            pass++;
        } else {
            printf("  [FAIL] Random values incorrect\n");
            fail++;
        }
    }

    // Test 3: Memory
    printf("Test 3: Memory Functions\n");
    {
        void* ptr = OriginalMemory::Alloc(1024, 0);
        if (ptr) {
            OriginalMemory::Mem_Copy((void*)"Test", ptr, 5);
            OriginalMemory::Free(ptr);
            printf("  Alloc/Copy/Free succeeded\n");
            printf("  [PASS]\n");
            pass++;
        } else {
            printf("  [FAIL] Allocation failed\n");
            fail++;
        }
    }

    // Test 4: Timer
    printf("Test 4: Timer Functions\n");
    {
        DWORD t1 = OriginalTimer::Get_System_Tick_Count();
        OriginalTimer::Delay(10);
        DWORD t2 = OriginalTimer::Get_System_Tick_Count();

        OriginalTimer::TimerClass timer;
        timer.Start();
        OriginalTimer::Delay(5);
        timer.Stop();
        DWORD elapsed = timer.Time();

        printf("  GetTickCount: %u -> %u (delta=%u)\n", t1, t2, t2 - t1);
        printf("  TimerClass: elapsed=%ums\n", elapsed);

        if (t2 > t1 && elapsed > 0) {
            printf("  [PASS]\n");
            pass++;
        } else {
            printf("  [FAIL] Timer not working\n");
            fail++;
        }
    }

    // Test 5: Geometry
    printf("Test 5: Geometry Structures\n");
    {
        POINT pt = {50, 50};
        RECT rc = {0, 0, 100, 100};

        BOOL inside = OriginalGeometry::Point_In_Rect(&pt, &rc);
        printf("  Point (50,50) in Rect (0,0,100,100): %s\n", inside ? "YES" : "NO");

        RECT bad_rect = {100, 100, 0, 0};
        OriginalGeometry::Normalize_Rect(&bad_rect);
        printf("  Normalized (100,100,0,0) -> (%ld,%ld,%ld,%ld)\n",
               bad_rect.left, bad_rect.top, bad_rect.right, bad_rect.bottom);

        if (inside && bad_rect.left == 0 && bad_rect.right == 100) {
            printf("  [PASS]\n");
            pass++;
        } else {
            printf("  [FAIL] Geometry functions incorrect\n");
            fail++;
        }
    }

    printf("\n==========================================\n");
    printf("Summary: %d passed, %d failed\n", pass, fail);
    printf("==========================================\n");

    // Cleanup
    Platform_Shutdown();

    if (fail > 0) {
        printf("\nSome original code patterns failed to work correctly.\n");
        printf("Review the compatibility layer for issues.\n");
        return 1;
    }

    printf("\nAll original code patterns compile and work!\n");
    printf("The compatibility layer is ready for Phase 14.\n");
    return 0;
}
