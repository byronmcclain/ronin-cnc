// src/tests/performance/test_memory.cpp
// Memory Performance Tests
// Task 18f - Performance Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "perf_utils.h"
#include <cstring>
#include <cstdlib>

//=============================================================================
// Memory Allocation Tests
//=============================================================================

TEST_CASE(Perf_Memory_SmallAllocations, "Performance") {
    const int ALLOC_COUNT = 10000;
    const int ALLOC_SIZE = 64;

    void* ptrs[ALLOC_COUNT];

    PerfTimer timer;
    timer.Start();

    // Allocate many small blocks
    for (int i = 0; i < ALLOC_COUNT; i++) {
        ptrs[i] = malloc(ALLOC_SIZE);
    }

    uint32_t alloc_time = timer.ElapsedMs();

    // Free all
    for (int i = 0; i < ALLOC_COUNT; i++) {
        free(ptrs[i]);
    }

    timer.Stop();

    char msg[128];
    snprintf(msg, sizeof(msg),
             "Small allocs: %d x %d bytes in %u ms (free in %u ms)",
             ALLOC_COUNT, ALLOC_SIZE, alloc_time, timer.ElapsedMs() - alloc_time);
    Platform_Log(LOG_LEVEL_INFO, msg);

    // Should be fast
    TEST_ASSERT_LT(timer.ElapsedMs(), 500u);
}

TEST_CASE(Perf_Memory_LargeAllocations, "Performance") {
    const int ALLOC_COUNT = 100;
    const int ALLOC_SIZE = 1024 * 1024;  // 1 MB

    void* ptrs[ALLOC_COUNT];

    PerfTimer timer;
    timer.Start();

    // Allocate large blocks
    for (int i = 0; i < ALLOC_COUNT; i++) {
        ptrs[i] = malloc(ALLOC_SIZE);
        if (ptrs[i]) {
            // Touch the memory to ensure it's allocated
            memset(ptrs[i], 0, ALLOC_SIZE);
        }
    }

    uint32_t alloc_time = timer.ElapsedMs();

    // Free all
    for (int i = 0; i < ALLOC_COUNT; i++) {
        free(ptrs[i]);
    }

    timer.Stop();

    char msg[128];
    snprintf(msg, sizeof(msg),
             "Large allocs: %d x %d MB in %u ms",
             ALLOC_COUNT, ALLOC_SIZE / (1024 * 1024), alloc_time);
    Platform_Log(LOG_LEVEL_INFO, msg);

    // Should complete in reasonable time
    TEST_ASSERT_LT(timer.ElapsedMs(), 2000u);
}

TEST_CASE(Perf_Memory_MixedAllocations, "Performance") {
    const int ITERATIONS = 1000;
    void* ptrs[100];
    int ptr_count = 0;

    PerfTimer timer;
    timer.Start();

    for (int i = 0; i < ITERATIONS; i++) {
        // Randomly allocate or free
        if ((i % 3) != 0 && ptr_count < 100) {
            // Allocate various sizes
            int size = 64 + (i % 10) * 256;
            ptrs[ptr_count++] = malloc(size);
        } else if (ptr_count > 0) {
            // Free
            free(ptrs[--ptr_count]);
        }
    }

    // Clean up remaining
    while (ptr_count > 0) {
        free(ptrs[--ptr_count]);
    }

    timer.Stop();

    char msg[128];
    snprintf(msg, sizeof(msg),
             "Mixed allocs: %d operations in %u ms",
             ITERATIONS, timer.ElapsedMs());
    Platform_Log(LOG_LEVEL_INFO, msg);

    TEST_ASSERT_LT(timer.ElapsedMs(), 500u);
}

TEST_CASE(Perf_Memory_MemsetSpeed, "Performance") {
    const int BUFFER_SIZE = 1024 * 1024;  // 1 MB
    const int ITERATIONS = 100;

    uint8_t* buffer = static_cast<uint8_t*>(malloc(BUFFER_SIZE));
    TEST_ASSERT(buffer != nullptr);

    PerfTimer timer;
    timer.Start();

    for (int i = 0; i < ITERATIONS; i++) {
        memset(buffer, static_cast<uint8_t>(i), BUFFER_SIZE);
    }

    timer.Stop();
    free(buffer);

    float mb_per_sec = (BUFFER_SIZE * ITERATIONS / (1024.0f * 1024.0f)) /
                       (timer.ElapsedMs() / 1000.0f);

    char msg[128];
    snprintf(msg, sizeof(msg),
             "Memset: %d MB in %u ms (%.0f MB/sec)",
             ITERATIONS, timer.ElapsedMs(), mb_per_sec);
    Platform_Log(LOG_LEVEL_INFO, msg);

    // Should be fast (at least 500 MB/sec)
    TEST_ASSERT_GT(mb_per_sec, 500.0f);
}

TEST_CASE(Perf_Memory_MemcpySpeed, "Performance") {
    const int BUFFER_SIZE = 1024 * 1024;  // 1 MB
    const int ITERATIONS = 100;

    uint8_t* src = static_cast<uint8_t*>(malloc(BUFFER_SIZE));
    uint8_t* dst = static_cast<uint8_t*>(malloc(BUFFER_SIZE));
    TEST_ASSERT(src != nullptr);
    TEST_ASSERT(dst != nullptr);

    // Initialize source
    memset(src, 0xAA, BUFFER_SIZE);

    PerfTimer timer;
    timer.Start();

    for (int i = 0; i < ITERATIONS; i++) {
        memcpy(dst, src, BUFFER_SIZE);
    }

    timer.Stop();

    free(src);
    free(dst);

    float mb_per_sec = (BUFFER_SIZE * ITERATIONS / (1024.0f * 1024.0f)) /
                       (timer.ElapsedMs() / 1000.0f);

    char msg[128];
    snprintf(msg, sizeof(msg),
             "Memcpy: %d MB in %u ms (%.0f MB/sec)",
             ITERATIONS, timer.ElapsedMs(), mb_per_sec);
    Platform_Log(LOG_LEVEL_INFO, msg);

    // Should be fast (at least 500 MB/sec)
    TEST_ASSERT_GT(mb_per_sec, 500.0f);
}

//=============================================================================
// Buffer Operations
//=============================================================================

TEST_WITH_FIXTURE(GraphicsFixture, Perf_Memory_BackBufferClear, "Performance") {
    if (!fixture.IsInitialized()) {
        TEST_SKIP("Graphics not initialized");
    }

    const int ITERATIONS = 100;
    int width = fixture.GetWidth();
    int height = fixture.GetHeight();

    PerfTimer timer;
    timer.Start();

    for (int i = 0; i < ITERATIONS; i++) {
        uint8_t* buffer = fixture.GetBackBuffer();
        int pitch = fixture.GetPitch();

        for (int y = 0; y < height; y++) {
            memset(buffer + y * pitch, 0, width);
        }
    }

    timer.Stop();

    int total_bytes = width * height * ITERATIONS;
    float mb_per_sec = (total_bytes / (1024.0f * 1024.0f)) /
                       (timer.ElapsedMs() / 1000.0f);

    char msg[128];
    snprintf(msg, sizeof(msg),
             "Buffer clear: %d x %d x %d in %u ms (%.0f MB/sec)",
             width, height, ITERATIONS, timer.ElapsedMs(), mb_per_sec);
    Platform_Log(LOG_LEVEL_INFO, msg);

    // Should be fast
    TEST_ASSERT_GT(mb_per_sec, 200.0f);
}

TEST_WITH_FIXTURE(GraphicsFixture, Perf_Memory_BackBufferWrite, "Performance") {
    if (!fixture.IsInitialized()) {
        TEST_SKIP("Graphics not initialized");
    }

    const int ITERATIONS = 50;
    int width = fixture.GetWidth();
    int height = fixture.GetHeight();

    PerfTimer timer;
    timer.Start();

    for (int iter = 0; iter < ITERATIONS; iter++) {
        uint8_t* buffer = fixture.GetBackBuffer();
        int pitch = fixture.GetPitch();

        // Write pattern
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                buffer[y * pitch + x] = static_cast<uint8_t>((x + y + iter) & 0xFF);
            }
        }
    }

    timer.Stop();

    int total_bytes = width * height * ITERATIONS;
    float mb_per_sec = (total_bytes / (1024.0f * 1024.0f)) /
                       (timer.ElapsedMs() / 1000.0f);

    char msg[128];
    snprintf(msg, sizeof(msg),
             "Buffer write: %d x %d x %d in %u ms (%.0f MB/sec)",
             width, height, ITERATIONS, timer.ElapsedMs(), mb_per_sec);
    Platform_Log(LOG_LEVEL_INFO, msg);

    // Allow for slower per-pixel writes
    TEST_ASSERT_GT(mb_per_sec, 50.0f);
}

//=============================================================================
// Memory Pool Simulation
//=============================================================================

TEST_CASE(Perf_Memory_PoolAllocation, "Performance") {
    // Simulate a memory pool for game objects
    const int POOL_SIZE = 1000;
    const int OBJECT_SIZE = 128;
    const int ITERATIONS = 10000;

    // Allocate pool
    uint8_t* pool = static_cast<uint8_t*>(malloc(POOL_SIZE * OBJECT_SIZE));
    bool* used = static_cast<bool*>(calloc(POOL_SIZE, sizeof(bool)));
    TEST_ASSERT(pool != nullptr);
    TEST_ASSERT(used != nullptr);

    int next_free = 0;

    PerfTimer timer;
    timer.Start();

    for (int i = 0; i < ITERATIONS; i++) {
        // Allocate
        if (!used[next_free]) {
            used[next_free] = true;
            // Touch the memory
            uint8_t* obj = pool + next_free * OBJECT_SIZE;
            memset(obj, static_cast<uint8_t>(i), OBJECT_SIZE);
        }

        // Find next free (simple linear search)
        int start = next_free;
        do {
            next_free = (next_free + 1) % POOL_SIZE;
        } while (used[next_free] && next_free != start);

        // Occasionally free some
        if ((i % 5) == 0 && i > 0) {
            int to_free = (i * 7) % POOL_SIZE;
            used[to_free] = false;
        }
    }

    timer.Stop();

    free(pool);
    free(used);

    char msg[128];
    snprintf(msg, sizeof(msg),
             "Pool alloc: %d operations in %u ms",
             ITERATIONS, timer.ElapsedMs());
    Platform_Log(LOG_LEVEL_INFO, msg);

    // Should be fast
    TEST_ASSERT_LT(timer.ElapsedMs(), 500u);
}
