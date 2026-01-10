# Task 18f: Performance Tests

| Field | Value |
|-------|-------|
| **Task ID** | 18f |
| **Task Name** | Performance Tests |
| **Phase** | 18 - Testing & Polish |
| **Depends On** | Tasks 18a-18e |
| **Estimated Complexity** | Medium (~450 lines) |

---

## Dependencies

- Task 18a (Test Framework) - Test infrastructure
- All game systems functional
- Graphics, audio, game logic implemented

---

## Context

Performance tests ensure the game runs at acceptable frame rates:

1. **Frame Rate Tests**: Verify 60fps target achieved
2. **Stress Tests**: Test with maximum units/buildings
3. **Memory Tests**: Monitor memory usage
4. **Asset Loading**: Measure load times
5. **Bottleneck Detection**: Identify slow operations

---

## Objective

Create performance tests that:
1. Measure baseline frame rates
2. Stress test with many objects
3. Profile critical code paths
4. Detect performance regressions
5. Generate performance reports

---

## Deliverables

- [ ] `src/tests/performance/test_framerate.cpp` - Frame rate tests
- [ ] `src/tests/performance/test_stress.cpp` - Stress tests
- [ ] `src/tests/performance/test_memory.cpp` - Memory tests
- [ ] `src/tests/performance/test_loading.cpp` - Load time tests
- [ ] `src/tests/performance/perf_utils.cpp` - Profiling utilities
- [ ] `src/tests/performance_tests_main.cpp` - Main entry point
- [ ] CMakeLists.txt updated

---

## Files to Create

### src/tests/performance/perf_utils.h

```cpp
// src/tests/performance/perf_utils.h
// Performance measurement utilities
// Task 18f - Performance Tests

#ifndef PERF_UTILS_H
#define PERF_UTILS_H

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

//=============================================================================
// Performance Timer
//=============================================================================

class PerfTimer {
public:
    void Start();
    void Stop();
    void Reset();

    double GetElapsedMs() const;
    double GetElapsedUs() const;

private:
    std::chrono::high_resolution_clock::time_point start_;
    std::chrono::high_resolution_clock::time_point end_;
    bool running_ = false;
};

//=============================================================================
// Frame Rate Tracker
//=============================================================================

class FrameRateTracker {
public:
    explicit FrameRateTracker(int sample_count = 60);

    void RecordFrame();
    double GetAverageFPS() const;
    double GetMinFPS() const;
    double GetMaxFPS() const;
    double Get1PercentileFPS() const;

private:
    std::vector<double> frame_times_;
    int sample_count_;
    std::chrono::high_resolution_clock::time_point last_frame_;
    bool first_frame_ = true;
};

//=============================================================================
// Memory Tracker
//=============================================================================

struct MemoryStats {
    size_t current_usage;
    size_t peak_usage;
    size_t allocation_count;
};

MemoryStats GetMemoryStats();

//=============================================================================
// Performance Report
//=============================================================================

struct PerfResult {
    std::string test_name;
    double min_fps;
    double avg_fps;
    double max_fps;
    double percentile_1;
    size_t memory_usage;
    bool passed;
};

void PrintPerfReport(const std::vector<PerfResult>& results);

//=============================================================================
// Assertions
//=============================================================================

#define PERF_ASSERT_FPS_GE(tracker, min_fps) \
    do { \
        double avg = tracker.GetAverageFPS(); \
        if (avg < min_fps) { \
            std::ostringstream ss; \
            ss << "FPS " << avg << " below minimum " << min_fps; \
            TEST_FAIL(ss.str()); \
        } \
    } while(0)

#define PERF_ASSERT_1PERCENT_GE(tracker, min_fps) \
    do { \
        double p1 = tracker.Get1PercentileFPS(); \
        if (p1 < min_fps) { \
            std::ostringstream ss; \
            ss << "1% low FPS " << p1 << " below minimum " << min_fps; \
            TEST_FAIL(ss.str()); \
        } \
    } while(0)

#endif // PERF_UTILS_H
```

### src/tests/performance/perf_utils.cpp

```cpp
// src/tests/performance/perf_utils.cpp
// Performance utilities implementation
// Task 18f - Performance Tests

#include "perf_utils.h"
#include <algorithm>
#include <numeric>
#include <iostream>
#include <iomanip>

//=============================================================================
// PerfTimer
//=============================================================================

void PerfTimer::Start() {
    start_ = std::chrono::high_resolution_clock::now();
    running_ = true;
}

void PerfTimer::Stop() {
    end_ = std::chrono::high_resolution_clock::now();
    running_ = false;
}

void PerfTimer::Reset() {
    running_ = false;
}

double PerfTimer::GetElapsedMs() const {
    auto end = running_ ? std::chrono::high_resolution_clock::now() : end_;
    return std::chrono::duration<double, std::milli>(end - start_).count();
}

double PerfTimer::GetElapsedUs() const {
    auto end = running_ ? std::chrono::high_resolution_clock::now() : end_;
    return std::chrono::duration<double, std::micro>(end - start_).count();
}

//=============================================================================
// FrameRateTracker
//=============================================================================

FrameRateTracker::FrameRateTracker(int sample_count)
    : sample_count_(sample_count) {
    frame_times_.reserve(sample_count);
}

void FrameRateTracker::RecordFrame() {
    auto now = std::chrono::high_resolution_clock::now();

    if (!first_frame_) {
        double frame_ms = std::chrono::duration<double, std::milli>(
            now - last_frame_).count();

        if (frame_times_.size() >= static_cast<size_t>(sample_count_)) {
            frame_times_.erase(frame_times_.begin());
        }
        frame_times_.push_back(frame_ms);
    }

    last_frame_ = now;
    first_frame_ = false;
}

double FrameRateTracker::GetAverageFPS() const {
    if (frame_times_.empty()) return 0.0;

    double avg_ms = std::accumulate(frame_times_.begin(), frame_times_.end(), 0.0)
                    / frame_times_.size();
    return avg_ms > 0 ? 1000.0 / avg_ms : 0.0;
}

double FrameRateTracker::GetMinFPS() const {
    if (frame_times_.empty()) return 0.0;

    double max_ms = *std::max_element(frame_times_.begin(), frame_times_.end());
    return max_ms > 0 ? 1000.0 / max_ms : 0.0;
}

double FrameRateTracker::GetMaxFPS() const {
    if (frame_times_.empty()) return 0.0;

    double min_ms = *std::min_element(frame_times_.begin(), frame_times_.end());
    return min_ms > 0 ? 1000.0 / min_ms : 0.0;
}

double FrameRateTracker::Get1PercentileFPS() const {
    if (frame_times_.empty()) return 0.0;

    std::vector<double> sorted = frame_times_;
    std::sort(sorted.begin(), sorted.end(), std::greater<double>());

    size_t idx = sorted.size() / 100;  // 1%
    if (idx >= sorted.size()) idx = sorted.size() - 1;

    return sorted[idx] > 0 ? 1000.0 / sorted[idx] : 0.0;
}

//=============================================================================
// Memory Tracking
//=============================================================================

MemoryStats GetMemoryStats() {
    MemoryStats stats = {};

    // Platform-specific implementation
    // On macOS, could use mach_task_info or similar

    return stats;
}

//=============================================================================
// Report
//=============================================================================

void PrintPerfReport(const std::vector<PerfResult>& results) {
    std::cout << "\n=== Performance Report ===\n\n";
    std::cout << std::setw(30) << "Test"
              << std::setw(10) << "Min FPS"
              << std::setw(10) << "Avg FPS"
              << std::setw(10) << "Max FPS"
              << std::setw(10) << "1% Low"
              << std::setw(10) << "Memory"
              << std::setw(10) << "Status"
              << "\n";
    std::cout << std::string(90, '-') << "\n";

    for (const auto& r : results) {
        std::cout << std::setw(30) << r.test_name
                  << std::setw(10) << std::fixed << std::setprecision(1) << r.min_fps
                  << std::setw(10) << r.avg_fps
                  << std::setw(10) << r.max_fps
                  << std::setw(10) << r.percentile_1
                  << std::setw(10) << (r.memory_usage / 1024) << "KB"
                  << std::setw(10) << (r.passed ? "PASS" : "FAIL")
                  << "\n";
    }
}
```

### src/tests/performance/test_framerate.cpp

```cpp
// src/tests/performance/test_framerate.cpp
// Frame Rate Performance Tests
// Task 18f - Performance Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "perf_utils.h"
#include "platform.h"

//=============================================================================
// Baseline Frame Rate Tests
//=============================================================================

TEST_WITH_FIXTURE(GraphicsFixture, Perf_FrameRate_EmptyScene, "Performance") {
    if (!fixture.IsInitialized()) TEST_SKIP("Graphics not initialized");

    FrameRateTracker tracker(300);

    // Run 300 frames (5 seconds)
    for (int i = 0; i < 300; i++) {
        uint8_t* buffer = fixture.GetBackBuffer();
        int pitch = fixture.GetPitch();
        int height = fixture.GetHeight();

        // Minimal render
        memset(buffer, 0, pitch * height);

        fixture.RenderFrame();
        tracker.RecordFrame();
    }

    double avg_fps = tracker.GetAverageFPS();
    Platform_Log("Empty scene: %.1f FPS (min %.1f, 1%% %.1f)",
                 avg_fps, tracker.GetMinFPS(), tracker.Get1PercentileFPS());

    PERF_ASSERT_FPS_GE(tracker, 55.0);  // Should easily hit 60
}

TEST_WITH_FIXTURE(GraphicsFixture, Perf_FrameRate_FullRedraw, "Performance") {
    if (!fixture.IsInitialized()) TEST_SKIP("Graphics not initialized");

    FrameRateTracker tracker(300);
    int width = fixture.GetWidth();
    int height = fixture.GetHeight();

    for (int i = 0; i < 300; i++) {
        uint8_t* buffer = fixture.GetBackBuffer();
        int pitch = fixture.GetPitch();

        // Full screen fill with pattern
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                buffer[y * pitch + x] = (x + y + i) & 0xFF;
            }
        }

        fixture.RenderFrame();
        tracker.RecordFrame();
    }

    Platform_Log("Full redraw: %.1f FPS", tracker.GetAverageFPS());
    PERF_ASSERT_FPS_GE(tracker, 50.0);
}

TEST_WITH_FIXTURE(GraphicsFixture, Perf_FrameRate_WithFlip, "Performance") {
    if (!fixture.IsInitialized()) TEST_SKIP("Graphics not initialized");

    FrameRateTracker tracker(300);

    for (int i = 0; i < 300; i++) {
        Platform_PumpEvents();

        uint8_t* buffer = fixture.GetBackBuffer();
        memset(buffer, i & 0xFF, fixture.GetPitch() * fixture.GetHeight());

        fixture.RenderFrame();
        tracker.RecordFrame();

        // No artificial delay - test raw flip performance
    }

    Platform_Log("With flip: %.1f FPS", tracker.GetAverageFPS());
    // VSync might limit to 60
    PERF_ASSERT_FPS_GE(tracker, 55.0);
}
```

### src/tests/performance/test_stress.cpp

```cpp
// src/tests/performance/test_stress.cpp
// Stress Performance Tests
// Task 18f - Performance Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "perf_utils.h"
#include "platform.h"
#include <vector>
#include <cstdlib>

//=============================================================================
// Stress Tests
//=============================================================================

TEST_WITH_FIXTURE(GraphicsFixture, Perf_Stress_ManySprites, "Stress") {
    if (!fixture.IsInitialized()) TEST_SKIP("Graphics not initialized");

    struct Sprite {
        int x, y;
        int vx, vy;
        uint8_t color;
    };

    const int SPRITE_COUNT = 500;
    const int SPRITE_SIZE = 8;

    std::vector<Sprite> sprites(SPRITE_COUNT);
    int width = fixture.GetWidth();
    int height = fixture.GetHeight();

    // Initialize sprites
    for (auto& s : sprites) {
        s.x = rand() % (width - SPRITE_SIZE);
        s.y = rand() % (height - SPRITE_SIZE);
        s.vx = (rand() % 5) - 2;
        s.vy = (rand() % 5) - 2;
        s.color = rand() & 0xFF;
    }

    FrameRateTracker tracker(300);

    for (int frame = 0; frame < 300; frame++) {
        uint8_t* buffer = fixture.GetBackBuffer();
        int pitch = fixture.GetPitch();

        // Clear
        memset(buffer, 0, pitch * height);

        // Update and draw sprites
        for (auto& s : sprites) {
            // Move
            s.x += s.vx;
            s.y += s.vy;

            // Bounce
            if (s.x < 0 || s.x >= width - SPRITE_SIZE) s.vx = -s.vx;
            if (s.y < 0 || s.y >= height - SPRITE_SIZE) s.vy = -s.vy;

            s.x = std::max(0, std::min(s.x, width - SPRITE_SIZE));
            s.y = std::max(0, std::min(s.y, height - SPRITE_SIZE));

            // Draw
            for (int dy = 0; dy < SPRITE_SIZE; dy++) {
                for (int dx = 0; dx < SPRITE_SIZE; dx++) {
                    buffer[(s.y + dy) * pitch + (s.x + dx)] = s.color;
                }
            }
        }

        fixture.RenderFrame();
        tracker.RecordFrame();
    }

    Platform_Log("%d sprites: %.1f FPS", SPRITE_COUNT, tracker.GetAverageFPS());
    PERF_ASSERT_FPS_GE(tracker, 30.0);  // Playable
}

TEST_WITH_FIXTURE(GraphicsFixture, Perf_Stress_MaxUnits, "Stress") {
    if (!fixture.IsInitialized()) TEST_SKIP("Graphics not initialized");

    // Simulate game with max unit count (50)
    const int MAX_UNITS = 50;

    struct Unit {
        int x, y;
        int target_x, target_y;
    };

    std::vector<Unit> units(MAX_UNITS);
    int width = fixture.GetWidth();
    int height = fixture.GetHeight();

    for (auto& u : units) {
        u.x = rand() % width;
        u.y = rand() % height;
        u.target_x = rand() % width;
        u.target_y = rand() % height;
    }

    FrameRateTracker tracker(300);

    for (int frame = 0; frame < 300; frame++) {
        uint8_t* buffer = fixture.GetBackBuffer();
        int pitch = fixture.GetPitch();

        memset(buffer, 32, pitch * height);

        // Update units
        for (auto& u : units) {
            // Simple movement toward target
            if (u.x < u.target_x) u.x++;
            else if (u.x > u.target_x) u.x--;
            if (u.y < u.target_y) u.y++;
            else if (u.y > u.target_y) u.y--;

            // New target if reached
            if (u.x == u.target_x && u.y == u.target_y) {
                u.target_x = rand() % width;
                u.target_y = rand() % height;
            }

            // Draw unit (small box)
            for (int dy = 0; dy < 10; dy++) {
                for (int dx = 0; dx < 10; dx++) {
                    int px = u.x + dx;
                    int py = u.y + dy;
                    if (px >= 0 && px < width && py >= 0 && py < height) {
                        buffer[py * pitch + px] = 200;
                    }
                }
            }
        }

        fixture.RenderFrame();
        tracker.RecordFrame();
    }

    Platform_Log("Max units (%d): %.1f FPS", MAX_UNITS, tracker.GetAverageFPS());
    PERF_ASSERT_FPS_GE(tracker, 55.0);  // Should maintain 60fps with 50 units
}
```

### src/tests/performance/test_memory.cpp

```cpp
// src/tests/performance/test_memory.cpp
// Memory Performance Tests
// Task 18f - Performance Tests

#include "test/test_framework.h"
#include "perf_utils.h"
#include "platform.h"
#include <vector>

//=============================================================================
// Memory Tests
//=============================================================================

TEST_CASE(Perf_Memory_AllocationSpeed, "Memory") {
    PerfTimer timer;
    const int ITERATIONS = 10000;

    timer.Start();
    for (int i = 0; i < ITERATIONS; i++) {
        void* ptr = malloc(1024);
        free(ptr);
    }
    timer.Stop();

    double us_per_alloc = timer.GetElapsedUs() / ITERATIONS;
    Platform_Log("Allocation: %.2f us per malloc/free", us_per_alloc);

    TEST_ASSERT_LT(us_per_alloc, 10.0);  // Should be fast
}

TEST_CASE(Perf_Memory_LargeAllocation, "Memory") {
    PerfTimer timer;

    timer.Start();
    void* ptr = malloc(64 * 1024 * 1024);  // 64MB
    timer.Stop();

    TEST_ASSERT_NOT_NULL(ptr);
    Platform_Log("64MB allocation: %.2f ms", timer.GetElapsedMs());

    free(ptr);
}

TEST_CASE(Perf_Memory_VectorGrowth, "Memory") {
    PerfTimer timer;
    std::vector<int> vec;

    timer.Start();
    for (int i = 0; i < 1000000; i++) {
        vec.push_back(i);
    }
    timer.Stop();

    Platform_Log("Vector 1M push: %.2f ms", timer.GetElapsedMs());
    TEST_ASSERT_EQ(vec.size(), 1000000u);
}
```

### src/tests/performance/test_loading.cpp

```cpp
// src/tests/performance/test_loading.cpp
// Asset Loading Performance Tests
// Task 18f - Performance Tests

#include "test/test_framework.h"
#include "perf_utils.h"
#include "platform.h"
#include "game/mix_manager.h"

//=============================================================================
// Loading Tests
//=============================================================================

TEST_CASE(Perf_Loading_MixFile, "Loading") {
    Platform_Init();

    MixManager& mix = MixManager::Instance();
    mix.Initialize();

    PerfTimer timer;
    timer.Start();
    bool loaded = mix.LoadMix("REDALERT.MIX");
    timer.Stop();

    if (loaded) {
        Platform_Log("MIX load time: %.2f ms", timer.GetElapsedMs());
        TEST_ASSERT_LT(timer.GetElapsedMs(), 1000.0);  // Under 1 second
    } else {
        TEST_SKIP("Game data not found");
    }

    mix.Shutdown();
    Platform_Shutdown();
}

TEST_CASE(Perf_Loading_AssetLookup, "Loading") {
    Platform_Init();

    MixManager& mix = MixManager::Instance();
    mix.Initialize();

    if (!mix.LoadMix("REDALERT.MIX")) {
        mix.Shutdown();
        Platform_Shutdown();
        TEST_SKIP("Game data not found");
    }

    PerfTimer timer;
    const int LOOKUPS = 1000;

    timer.Start();
    for (int i = 0; i < LOOKUPS; i++) {
        mix.FindFile("MOUSE.SHP", nullptr);
    }
    timer.Stop();

    double us_per_lookup = timer.GetElapsedUs() / LOOKUPS;
    Platform_Log("Asset lookup: %.2f us per lookup", us_per_lookup);
    TEST_ASSERT_LT(us_per_lookup, 100.0);  // Should be fast

    mix.Shutdown();
    Platform_Shutdown();
}
```

### src/tests/performance_tests_main.cpp

```cpp
// src/tests/performance_tests_main.cpp
// Performance Tests Main Entry Point
// Task 18f - Performance Tests

#include "test/test_framework.h"

TEST_MAIN()
```

---

## CMakeLists.txt Additions

```cmake
# Task 18f: Performance Tests

set(PERF_TEST_SOURCES
    ${CMAKE_SOURCE_DIR}/src/tests/performance/perf_utils.cpp
    ${CMAKE_SOURCE_DIR}/src/tests/performance/test_framerate.cpp
    ${CMAKE_SOURCE_DIR}/src/tests/performance/test_stress.cpp
    ${CMAKE_SOURCE_DIR}/src/tests/performance/test_memory.cpp
    ${CMAKE_SOURCE_DIR}/src/tests/performance/test_loading.cpp
    ${CMAKE_SOURCE_DIR}/src/tests/performance_tests_main.cpp
)

add_executable(PerformanceTests ${PERF_TEST_SOURCES})
target_link_libraries(PerformanceTests PRIVATE test_framework redalert_platform redalert_game)
target_include_directories(PerformanceTests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/tests/performance
)
add_test(NAME PerformanceTests COMMAND PerformanceTests)
```

---

## Verification Script

```bash
#!/bin/bash
set -e
echo "=== Task 18f Verification: Performance Tests ==="
ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"

cd "$BUILD_DIR" && cmake --build . --target PerformanceTests
"$BUILD_DIR/PerformanceTests" --verbose || exit 1

echo "=== Task 18f Verification PASSED ==="
```

---

## Completion Promise

When verification passes, output:
<promise>TASK_18F_COMPLETE</promise>

## Escape Hatch

If stuck after 15 iterations, output:
<promise>TASK_18F_BLOCKED</promise>

## Max Iterations
15
