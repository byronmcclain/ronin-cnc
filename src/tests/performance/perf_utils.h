// src/tests/performance/perf_utils.h
// Performance Testing Utilities
// Task 18f - Performance Tests

#ifndef PERF_UTILS_H
#define PERF_UTILS_H

#include "platform.h"
#include <cstdint>

//=============================================================================
// PerfTimer - High resolution performance timer
//=============================================================================

class PerfTimer {
public:
    PerfTimer() : start_time_(0), end_time_(0), running_(false) {}

    void Start() {
        start_time_ = Platform_Timer_GetTicks();
        running_ = true;
    }

    void Stop() {
        if (running_) {
            end_time_ = Platform_Timer_GetTicks();
            running_ = false;
        }
    }

    uint32_t ElapsedMs() const {
        if (running_) {
            return Platform_Timer_GetTicks() - start_time_;
        }
        return end_time_ - start_time_;
    }

    float ElapsedSeconds() const {
        return ElapsedMs() / 1000.0f;
    }

    void Reset() {
        start_time_ = 0;
        end_time_ = 0;
        running_ = false;
    }

private:
    uint32_t start_time_;
    uint32_t end_time_;
    bool running_;
};

//=============================================================================
// FrameRateTracker - Track frame timing statistics
//=============================================================================

class FrameRateTracker {
public:
    static const int MAX_SAMPLES = 256;

    FrameRateTracker() : sample_count_(0), current_index_(0) {
        for (int i = 0; i < MAX_SAMPLES; i++) {
            frame_times_[i] = 0;
        }
    }

    void RecordFrame(uint32_t frame_time_ms) {
        frame_times_[current_index_] = frame_time_ms;
        current_index_ = (current_index_ + 1) % MAX_SAMPLES;
        if (sample_count_ < MAX_SAMPLES) {
            sample_count_++;
        }
    }

    float GetAverageFPS() const {
        if (sample_count_ == 0) return 0.0f;
        uint32_t total = 0;
        for (int i = 0; i < sample_count_; i++) {
            total += frame_times_[i];
        }
        float avg_ms = static_cast<float>(total) / sample_count_;
        return avg_ms > 0 ? 1000.0f / avg_ms : 0.0f;
    }

    float GetMinFPS() const {
        if (sample_count_ == 0) return 0.0f;
        uint32_t max_time = 0;
        for (int i = 0; i < sample_count_; i++) {
            if (frame_times_[i] > max_time) {
                max_time = frame_times_[i];
            }
        }
        return max_time > 0 ? 1000.0f / max_time : 0.0f;
    }

    float GetMaxFPS() const {
        if (sample_count_ == 0) return 0.0f;
        uint32_t min_time = UINT32_MAX;
        for (int i = 0; i < sample_count_; i++) {
            if (frame_times_[i] > 0 && frame_times_[i] < min_time) {
                min_time = frame_times_[i];
            }
        }
        return min_time < UINT32_MAX ? 1000.0f / min_time : 0.0f;
    }

    uint32_t GetAverageFrameTimeMs() const {
        if (sample_count_ == 0) return 0;
        uint32_t total = 0;
        for (int i = 0; i < sample_count_; i++) {
            total += frame_times_[i];
        }
        return total / sample_count_;
    }

    int GetSampleCount() const { return sample_count_; }

    void Reset() {
        sample_count_ = 0;
        current_index_ = 0;
    }

private:
    uint32_t frame_times_[MAX_SAMPLES];
    int sample_count_;
    int current_index_;
};

//=============================================================================
// MemoryStats - Track memory usage (simplified cross-platform)
//=============================================================================

struct MemoryStats {
    size_t allocated_bytes;
    size_t allocation_count;
    size_t peak_bytes;

    MemoryStats() : allocated_bytes(0), allocation_count(0), peak_bytes(0) {}

    void RecordAllocation(size_t bytes) {
        allocated_bytes += bytes;
        allocation_count++;
        if (allocated_bytes > peak_bytes) {
            peak_bytes = allocated_bytes;
        }
    }

    void RecordDeallocation(size_t bytes) {
        if (bytes <= allocated_bytes) {
            allocated_bytes -= bytes;
        }
    }

    void Reset() {
        allocated_bytes = 0;
        allocation_count = 0;
        peak_bytes = 0;
    }

    float GetAllocatedMB() const {
        return allocated_bytes / (1024.0f * 1024.0f);
    }

    float GetPeakMB() const {
        return peak_bytes / (1024.0f * 1024.0f);
    }
};

//=============================================================================
// BenchmarkResult - Store benchmark results
//=============================================================================

struct BenchmarkResult {
    const char* name;
    uint32_t total_time_ms;
    uint32_t iterations;
    float ops_per_second;
    float avg_ms_per_op;

    BenchmarkResult()
        : name(nullptr), total_time_ms(0), iterations(0),
          ops_per_second(0.0f), avg_ms_per_op(0.0f) {}

    void Calculate() {
        if (total_time_ms > 0) {
            ops_per_second = (iterations * 1000.0f) / total_time_ms;
            avg_ms_per_op = static_cast<float>(total_time_ms) / iterations;
        }
    }
};

//=============================================================================
// BENCHMARK macro for easy benchmarking
//=============================================================================

#define BENCHMARK(name, iterations, code) \
    do { \
        PerfTimer _timer; \
        _timer.Start(); \
        for (int _i = 0; _i < iterations; _i++) { \
            code; \
        } \
        _timer.Stop(); \
        char _msg[256]; \
        float _ops = (iterations * 1000.0f) / _timer.ElapsedMs(); \
        snprintf(_msg, sizeof(_msg), \
                 "BENCH %s: %d iterations in %u ms (%.1f ops/sec)", \
                 name, iterations, _timer.ElapsedMs(), _ops); \
        Platform_Log(LOG_LEVEL_INFO, _msg); \
    } while(0)

#endif // PERF_UTILS_H
