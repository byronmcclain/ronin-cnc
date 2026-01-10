# Task 18h: Performance Optimization

| Field | Value |
|-------|-------|
| **Task ID** | 18h |
| **Task Name** | Performance Optimization |
| **Phase** | 18 - Testing & Polish |
| **Depends On** | Task 18f (Performance Tests), All core systems implemented |
| **Estimated Complexity** | High (~700 lines) |

---

## Dependencies

- Task 18f (Performance Tests) - Profiling infrastructure
- Graphics system implemented
- Audio system implemented
- Asset loading system implemented
- Game loop operational

---

## Context

Performance optimization ensures the game runs smoothly:

1. **Profiling**: Identify bottlenecks via measurement
2. **Rendering Optimization**: Reduce draw calls, batch sprites
3. **Memory Optimization**: Pool allocations, reduce fragmentation
4. **CPU Optimization**: Cache-friendly code, minimize branching
5. **Asset Optimization**: Preloading, streaming, compression
6. **Frame Pacing**: Consistent frame times, no stuttering

Target: 60 FPS on Apple Silicon, 30 FPS on older Intel Macs.

---

## Technical Background

### Performance Bottlenecks (Typical)

```
1. Rendering (~40% of frame time)
   - Too many draw calls
   - Overdraw from layered sprites
   - Texture binding changes

2. Game Logic (~30% of frame time)
   - Pathfinding calculations
   - AI decision making
   - Collision detection

3. Asset I/O (~20% of frame time)
   - Loading during gameplay
   - Decompression overhead

4. Audio (~10% of frame time)
   - Mixing many channels
   - Streaming from disk
```

### Optimization Techniques

```
Rendering:
- Sprite batching (group by texture)
- Dirty rectangle rendering
- View frustum culling
- Reduce state changes

Memory:
- Object pooling for frequently created/destroyed objects
- Memory arenas for frame-local allocations
- Preallocate containers

CPU:
- Data-oriented design (structure of arrays)
- Cache-friendly iteration
- Minimize virtual calls in hot paths
- Profile-guided optimization
```

---

## Objective

Implement performance optimizations:
1. Sprite batching system
2. Object pooling for game objects
3. Memory arena for frame allocations
4. Dirty rectangle tracking
5. Asset preloading system
6. Frame time smoothing

---

## Deliverables

- [ ] `src/platform/profiler.h` - Profiling utilities
- [ ] `src/platform/profiler.cpp` - Profiler implementation
- [ ] `src/platform/memory_pool.h` - Object pooling
- [ ] `src/platform/memory_pool.cpp` - Pool implementation
- [ ] `src/platform/memory_arena.h` - Frame arena allocator
- [ ] `src/platform/memory_arena.cpp` - Arena implementation
- [ ] `src/graphics/sprite_batch.h` - Sprite batching
- [ ] `src/graphics/sprite_batch.cpp` - Batch implementation
- [ ] `src/graphics/dirty_rect.h` - Dirty rectangle tracking
- [ ] `src/graphics/dirty_rect.cpp` - Dirty rect implementation
- [ ] CMakeLists.txt updated

---

## Files to Create

### src/platform/profiler.h

```cpp
// src/platform/profiler.h
// Performance Profiling Utilities
// Task 18h - Performance Optimization

#ifndef PROFILER_H
#define PROFILER_H

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <cstdint>

//=============================================================================
// Profiler Configuration
//=============================================================================

// Enable/disable profiling at compile time
#ifndef PROFILER_ENABLED
    #ifdef NDEBUG
        #define PROFILER_ENABLED 0
    #else
        #define PROFILER_ENABLED 1
    #endif
#endif

//=============================================================================
// Profiler Macros
//=============================================================================

#if PROFILER_ENABLED
    #define PROFILE_SCOPE(name) ProfileScope _profile_scope_##__LINE__(name)
    #define PROFILE_FUNCTION() ProfileScope _profile_scope_func(__FUNCTION__)
    #define PROFILE_BEGIN(name) Profiler::instance().begin_sample(name)
    #define PROFILE_END(name) Profiler::instance().end_sample(name)
    #define PROFILE_VALUE(name, value) Profiler::instance().record_value(name, value)
    #define PROFILE_COUNTER(name) Profiler::instance().increment_counter(name)
#else
    #define PROFILE_SCOPE(name)
    #define PROFILE_FUNCTION()
    #define PROFILE_BEGIN(name)
    #define PROFILE_END(name)
    #define PROFILE_VALUE(name, value)
    #define PROFILE_COUNTER(name)
#endif

//=============================================================================
// Profile Sample
//=============================================================================

struct ProfileSample {
    std::string name;
    double start_time_ms;
    double end_time_ms;
    double duration_ms;
    int depth;
    uint32_t thread_id;
};

struct ProfileStats {
    std::string name;
    int call_count;
    double total_time_ms;
    double min_time_ms;
    double max_time_ms;
    double avg_time_ms;
    double last_time_ms;
};

//=============================================================================
// Profiler
//=============================================================================

class Profiler {
public:
    static Profiler& instance();

    // Frame management
    void begin_frame();
    void end_frame();
    int get_frame_number() const { return frame_number_; }

    // Sampling
    void begin_sample(const std::string& name);
    void end_sample(const std::string& name);

    // Value tracking
    void record_value(const std::string& name, double value);
    void increment_counter(const std::string& name);

    // Statistics
    ProfileStats get_stats(const std::string& name) const;
    std::vector<ProfileStats> get_all_stats() const;
    double get_frame_time_ms() const { return last_frame_time_ms_; }
    double get_avg_frame_time_ms() const;
    double get_fps() const;

    // Frame history
    std::vector<double> get_frame_times(int count = 60) const;
    double get_percentile_frame_time(int percentile) const;

    // Reporting
    std::string get_report() const;
    std::string get_csv_report() const;
    void print_report() const;

    // Chrome tracing format export
    std::string export_chrome_trace() const;
    bool save_chrome_trace(const std::string& path) const;

    // Control
    void reset();
    void set_enabled(bool enabled) { enabled_ = enabled; }
    bool is_enabled() const { return enabled_; }

private:
    Profiler() = default;

    struct SampleData {
        int call_count = 0;
        double total_time_ms = 0.0;
        double min_time_ms = 999999.0;
        double max_time_ms = 0.0;
        double last_time_ms = 0.0;
    };

    struct ActiveSample {
        std::string name;
        std::chrono::high_resolution_clock::time_point start_time;
        int depth;
    };

    bool enabled_ = true;
    int frame_number_ = 0;
    double last_frame_time_ms_ = 0.0;
    std::chrono::high_resolution_clock::time_point frame_start_;

    std::vector<ActiveSample> sample_stack_;
    std::map<std::string, SampleData> sample_data_;
    std::map<std::string, double> values_;
    std::map<std::string, int> counters_;

    std::vector<double> frame_times_;
    static const int MAX_FRAME_HISTORY = 300;

    std::vector<ProfileSample> chrome_samples_;
};

//=============================================================================
// RAII Profile Scope
//=============================================================================

class ProfileScope {
public:
    explicit ProfileScope(const char* name) : name_(name) {
        Profiler::instance().begin_sample(name_);
    }

    ~ProfileScope() {
        Profiler::instance().end_sample(name_);
    }

    ProfileScope(const ProfileScope&) = delete;
    ProfileScope& operator=(const ProfileScope&) = delete;

private:
    const char* name_;
};

//=============================================================================
// GPU Profiler (Placeholder for Metal)
//=============================================================================

class GPUProfiler {
public:
    static GPUProfiler& instance();

    void begin_frame();
    void end_frame();

    void begin_gpu_sample(const std::string& name);
    void end_gpu_sample(const std::string& name);

    double get_gpu_time_ms() const { return last_gpu_time_ms_; }
    std::string get_report() const;

private:
    GPUProfiler() = default;
    double last_gpu_time_ms_ = 0.0;
    // Would integrate with Metal's GPU profiling API
};

#endif // PROFILER_H
```

### src/platform/profiler.cpp

```cpp
// src/platform/profiler.cpp
// Performance Profiling Implementation
// Task 18h - Performance Optimization

#include "platform/profiler.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cstdio>

//=============================================================================
// Profiler Implementation
//=============================================================================

Profiler& Profiler::instance() {
    static Profiler instance;
    return instance;
}

void Profiler::begin_frame() {
    if (!enabled_) return;

    frame_start_ = std::chrono::high_resolution_clock::now();
    sample_stack_.clear();
    counters_.clear();
}

void Profiler::end_frame() {
    if (!enabled_) return;

    auto frame_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        frame_end - frame_start_);
    last_frame_time_ms_ = duration.count() / 1000.0;

    // Store frame time history
    frame_times_.push_back(last_frame_time_ms_);
    if (frame_times_.size() > MAX_FRAME_HISTORY) {
        frame_times_.erase(frame_times_.begin());
    }

    frame_number_++;
}

void Profiler::begin_sample(const std::string& name) {
    if (!enabled_) return;

    ActiveSample sample;
    sample.name = name;
    sample.start_time = std::chrono::high_resolution_clock::now();
    sample.depth = static_cast<int>(sample_stack_.size());
    sample_stack_.push_back(sample);
}

void Profiler::end_sample(const std::string& name) {
    if (!enabled_) return;
    if (sample_stack_.empty()) return;

    // Find matching sample (should be on top)
    if (sample_stack_.back().name != name) {
        // Mismatched begin/end - log warning
        return;
    }

    ActiveSample& active = sample_stack_.back();
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - active.start_time);
    double duration_ms = duration.count() / 1000.0;

    // Update statistics
    SampleData& data = sample_data_[name];
    data.call_count++;
    data.total_time_ms += duration_ms;
    data.last_time_ms = duration_ms;
    if (duration_ms < data.min_time_ms) data.min_time_ms = duration_ms;
    if (duration_ms > data.max_time_ms) data.max_time_ms = duration_ms;

    // Store for Chrome trace export
    ProfileSample chrome_sample;
    chrome_sample.name = name;
    auto frame_start_us = std::chrono::duration_cast<std::chrono::microseconds>(
        active.start_time.time_since_epoch()).count();
    chrome_sample.start_time_ms = frame_start_us / 1000.0;
    chrome_sample.duration_ms = duration_ms;
    chrome_sample.depth = active.depth;
    chrome_samples_.push_back(chrome_sample);

    sample_stack_.pop_back();
}

void Profiler::record_value(const std::string& name, double value) {
    if (!enabled_) return;
    values_[name] = value;
}

void Profiler::increment_counter(const std::string& name) {
    if (!enabled_) return;
    counters_[name]++;
}

ProfileStats Profiler::get_stats(const std::string& name) const {
    ProfileStats stats;
    stats.name = name;

    auto it = sample_data_.find(name);
    if (it != sample_data_.end()) {
        const SampleData& data = it->second;
        stats.call_count = data.call_count;
        stats.total_time_ms = data.total_time_ms;
        stats.min_time_ms = data.min_time_ms;
        stats.max_time_ms = data.max_time_ms;
        stats.avg_time_ms = data.call_count > 0 ?
            data.total_time_ms / data.call_count : 0.0;
        stats.last_time_ms = data.last_time_ms;
    }

    return stats;
}

std::vector<ProfileStats> Profiler::get_all_stats() const {
    std::vector<ProfileStats> result;
    for (const auto& pair : sample_data_) {
        result.push_back(get_stats(pair.first));
    }

    // Sort by total time descending
    std::sort(result.begin(), result.end(),
        [](const ProfileStats& a, const ProfileStats& b) {
            return a.total_time_ms > b.total_time_ms;
        });

    return result;
}

double Profiler::get_avg_frame_time_ms() const {
    if (frame_times_.empty()) return 0.0;

    double sum = 0.0;
    for (double t : frame_times_) {
        sum += t;
    }
    return sum / frame_times_.size();
}

double Profiler::get_fps() const {
    double avg = get_avg_frame_time_ms();
    return avg > 0.0 ? 1000.0 / avg : 0.0;
}

std::vector<double> Profiler::get_frame_times(int count) const {
    int start = static_cast<int>(frame_times_.size()) - count;
    if (start < 0) start = 0;

    return std::vector<double>(
        frame_times_.begin() + start,
        frame_times_.end());
}

double Profiler::get_percentile_frame_time(int percentile) const {
    if (frame_times_.empty()) return 0.0;

    std::vector<double> sorted = frame_times_;
    std::sort(sorted.begin(), sorted.end());

    int index = (percentile * static_cast<int>(sorted.size())) / 100;
    if (index >= static_cast<int>(sorted.size())) {
        index = static_cast<int>(sorted.size()) - 1;
    }

    return sorted[index];
}

std::string Profiler::get_report() const {
    std::ostringstream oss;

    oss << "=== Performance Report ===\n\n";
    oss << "Frame Statistics:\n";
    oss << "  Frames: " << frame_number_ << "\n";
    oss << "  Avg FPS: " << std::fixed << std::setprecision(1) << get_fps() << "\n";
    oss << "  Avg Frame: " << std::setprecision(2) << get_avg_frame_time_ms() << " ms\n";
    oss << "  Last Frame: " << last_frame_time_ms_ << " ms\n";
    oss << "  95th percentile: " << get_percentile_frame_time(95) << " ms\n";
    oss << "  99th percentile: " << get_percentile_frame_time(99) << " ms\n\n";

    oss << "Sample Statistics:\n";
    oss << std::setw(30) << std::left << "Name"
        << std::setw(10) << "Calls"
        << std::setw(12) << "Total(ms)"
        << std::setw(12) << "Avg(ms)"
        << std::setw(12) << "Min(ms)"
        << std::setw(12) << "Max(ms)"
        << "\n";
    oss << std::string(88, '-') << "\n";

    auto stats = get_all_stats();
    for (const auto& s : stats) {
        oss << std::setw(30) << std::left << s.name
            << std::setw(10) << s.call_count
            << std::setw(12) << std::setprecision(2) << s.total_time_ms
            << std::setw(12) << s.avg_time_ms
            << std::setw(12) << s.min_time_ms
            << std::setw(12) << s.max_time_ms
            << "\n";
    }

    if (!counters_.empty()) {
        oss << "\nCounters:\n";
        for (const auto& pair : counters_) {
            oss << "  " << pair.first << ": " << pair.second << "\n";
        }
    }

    if (!values_.empty()) {
        oss << "\nValues:\n";
        for (const auto& pair : values_) {
            oss << "  " << pair.first << ": " << pair.second << "\n";
        }
    }

    return oss.str();
}

std::string Profiler::get_csv_report() const {
    std::ostringstream oss;

    oss << "Name,Calls,Total(ms),Avg(ms),Min(ms),Max(ms)\n";

    auto stats = get_all_stats();
    for (const auto& s : stats) {
        oss << s.name << ","
            << s.call_count << ","
            << s.total_time_ms << ","
            << s.avg_time_ms << ","
            << s.min_time_ms << ","
            << s.max_time_ms << "\n";
    }

    return oss.str();
}

void Profiler::print_report() const {
    printf("%s", get_report().c_str());
}

std::string Profiler::export_chrome_trace() const {
    // Chrome tracing JSON format
    std::ostringstream oss;
    oss << "{\"traceEvents\":[\n";

    bool first = true;
    for (const auto& sample : chrome_samples_) {
        if (!first) oss << ",\n";
        first = false;

        oss << "{\"name\":\"" << sample.name << "\","
            << "\"cat\":\"profile\","
            << "\"ph\":\"X\","
            << "\"ts\":" << static_cast<int64_t>(sample.start_time_ms * 1000) << ","
            << "\"dur\":" << static_cast<int64_t>(sample.duration_ms * 1000) << ","
            << "\"pid\":1,"
            << "\"tid\":" << sample.depth << "}";
    }

    oss << "\n]}";
    return oss.str();
}

bool Profiler::save_chrome_trace(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << export_chrome_trace();
    return true;
}

void Profiler::reset() {
    frame_number_ = 0;
    last_frame_time_ms_ = 0.0;
    sample_stack_.clear();
    sample_data_.clear();
    values_.clear();
    counters_.clear();
    frame_times_.clear();
    chrome_samples_.clear();
}

//=============================================================================
// GPU Profiler Implementation (Placeholder)
//=============================================================================

GPUProfiler& GPUProfiler::instance() {
    static GPUProfiler instance;
    return instance;
}

void GPUProfiler::begin_frame() {
    // Would set up Metal GPU timestamp queries
}

void GPUProfiler::end_frame() {
    // Would read back GPU timestamps
    // last_gpu_time_ms_ = calculated from timestamps
}

void GPUProfiler::begin_gpu_sample(const std::string& name) {
    // Would insert Metal timestamp
    (void)name;
}

void GPUProfiler::end_gpu_sample(const std::string& name) {
    // Would insert Metal timestamp
    (void)name;
}

std::string GPUProfiler::get_report() const {
    std::ostringstream oss;
    oss << "GPU Time: " << last_gpu_time_ms_ << " ms\n";
    return oss.str();
}
```

### src/platform/memory_pool.h

```cpp
// src/platform/memory_pool.h
// Object Pool Memory Management
// Task 18h - Performance Optimization

#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <vector>
#include <cstdint>
#include <cassert>

//=============================================================================
// Fixed-Size Object Pool
//=============================================================================

template<typename T, size_t BlockSize = 64>
class ObjectPool {
public:
    ObjectPool() {
        allocate_block();
    }

    ~ObjectPool() {
        for (char* block : blocks_) {
            delete[] block;
        }
    }

    // Allocate an object (default constructed)
    T* allocate() {
        if (free_list_.empty()) {
            allocate_block();
        }

        T* obj = free_list_.back();
        free_list_.pop_back();
        new (obj) T();  // Placement new
        allocated_count_++;
        return obj;
    }

    // Allocate with constructor arguments
    template<typename... Args>
    T* allocate(Args&&... args) {
        if (free_list_.empty()) {
            allocate_block();
        }

        T* obj = free_list_.back();
        free_list_.pop_back();
        new (obj) T(std::forward<Args>(args)...);
        allocated_count_++;
        return obj;
    }

    // Return object to pool
    void deallocate(T* obj) {
        if (obj == nullptr) return;

        obj->~T();  // Call destructor
        free_list_.push_back(obj);
        allocated_count_--;
    }

    // Statistics
    size_t get_allocated_count() const { return allocated_count_; }
    size_t get_free_count() const { return free_list_.size(); }
    size_t get_total_capacity() const { return blocks_.size() * BlockSize; }

    // Clear all (call destructors on allocated objects)
    void clear() {
        // Note: This doesn't actually track which are allocated
        // In production, would need a used list
        free_list_.clear();
        for (char* block : blocks_) {
            T* objects = reinterpret_cast<T*>(block);
            for (size_t i = 0; i < BlockSize; i++) {
                free_list_.push_back(&objects[i]);
            }
        }
        allocated_count_ = 0;
    }

private:
    void allocate_block() {
        char* block = new char[sizeof(T) * BlockSize];
        blocks_.push_back(block);

        T* objects = reinterpret_cast<T*>(block);
        for (size_t i = 0; i < BlockSize; i++) {
            free_list_.push_back(&objects[i]);
        }
    }

    std::vector<char*> blocks_;
    std::vector<T*> free_list_;
    size_t allocated_count_ = 0;
};

//=============================================================================
// Pool Handle (for safer pool access)
//=============================================================================

template<typename T>
class PoolHandle {
public:
    PoolHandle() : pool_(nullptr), obj_(nullptr) {}
    PoolHandle(ObjectPool<T>* pool, T* obj) : pool_(pool), obj_(obj) {}

    ~PoolHandle() {
        release();
    }

    // Move semantics
    PoolHandle(PoolHandle&& other) noexcept
        : pool_(other.pool_), obj_(other.obj_) {
        other.pool_ = nullptr;
        other.obj_ = nullptr;
    }

    PoolHandle& operator=(PoolHandle&& other) noexcept {
        if (this != &other) {
            release();
            pool_ = other.pool_;
            obj_ = other.obj_;
            other.pool_ = nullptr;
            other.obj_ = nullptr;
        }
        return *this;
    }

    // No copying
    PoolHandle(const PoolHandle&) = delete;
    PoolHandle& operator=(const PoolHandle&) = delete;

    T* get() const { return obj_; }
    T* operator->() const { return obj_; }
    T& operator*() const { return *obj_; }
    explicit operator bool() const { return obj_ != nullptr; }

    void release() {
        if (pool_ && obj_) {
            pool_->deallocate(obj_);
        }
        pool_ = nullptr;
        obj_ = nullptr;
    }

private:
    ObjectPool<T>* pool_;
    T* obj_;
};

//=============================================================================
// Type-Specific Pool Manager
//=============================================================================

// Example: Global pools for common game objects
// Would be instantiated in game code

/*
class GamePools {
public:
    static GamePools& instance();

    ObjectPool<Bullet, 256> bullets;
    ObjectPool<Particle, 1024> particles;
    ObjectPool<Sound, 64> sounds;

private:
    GamePools() = default;
};
*/

#endif // MEMORY_POOL_H
```

### src/platform/memory_pool.cpp

```cpp
// src/platform/memory_pool.cpp
// Object Pool Implementation
// Task 18h - Performance Optimization

#include "platform/memory_pool.h"

// Template implementations are in the header
// This file exists for any non-template utilities

//=============================================================================
// Pool Statistics Helper
//=============================================================================

struct PoolStats {
    size_t allocated;
    size_t free;
    size_t total;
    size_t memory_bytes;
};

// Would collect stats from multiple pools if needed
```

### src/platform/memory_arena.h

```cpp
// src/platform/memory_arena.h
// Frame Arena Allocator
// Task 18h - Performance Optimization

#ifndef MEMORY_ARENA_H
#define MEMORY_ARENA_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <cassert>

//=============================================================================
// Linear Arena Allocator
//=============================================================================

class MemoryArena {
public:
    explicit MemoryArena(size_t initial_size = 1024 * 1024)  // 1MB default
        : block_size_(initial_size) {
        allocate_block(initial_size);
    }

    ~MemoryArena() {
        for (char* block : blocks_) {
            delete[] block;
        }
    }

    // Allocate raw memory (aligned)
    void* allocate(size_t size, size_t alignment = 8) {
        // Align current position
        size_t aligned_pos = (current_pos_ + alignment - 1) & ~(alignment - 1);

        if (aligned_pos + size > current_block_size_) {
            // Need new block
            size_t new_block_size = block_size_;
            if (size > new_block_size) {
                new_block_size = size + alignment;
            }
            allocate_block(new_block_size);
            aligned_pos = 0;
        }

        void* ptr = current_block_ + aligned_pos;
        current_pos_ = aligned_pos + size;
        total_allocated_ += size;

        return ptr;
    }

    // Allocate and construct
    template<typename T, typename... Args>
    T* create(Args&&... args) {
        void* mem = allocate(sizeof(T), alignof(T));
        return new (mem) T(std::forward<Args>(args)...);
    }

    // Allocate array
    template<typename T>
    T* create_array(size_t count) {
        void* mem = allocate(sizeof(T) * count, alignof(T));
        T* arr = static_cast<T*>(mem);
        for (size_t i = 0; i < count; i++) {
            new (&arr[i]) T();
        }
        return arr;
    }

    // Reset arena (reuse memory)
    void reset() {
        // Keep first block, release others
        while (blocks_.size() > 1) {
            delete[] blocks_.back();
            blocks_.pop_back();
        }

        if (!blocks_.empty()) {
            current_block_ = blocks_[0];
            current_block_size_ = block_size_;
        }
        current_pos_ = 0;
        total_allocated_ = 0;
    }

    // Statistics
    size_t get_total_allocated() const { return total_allocated_; }
    size_t get_total_capacity() const {
        size_t total = 0;
        for (size_t i = 0; i < blocks_.size(); i++) {
            total += (i == 0) ? block_size_ : block_size_;  // Simplified
        }
        return total;
    }
    size_t get_block_count() const { return blocks_.size(); }

    // Scoped marker for temporary allocations
    class Marker {
    public:
        Marker(MemoryArena& arena)
            : arena_(arena)
            , block_index_(arena.blocks_.size() - 1)
            , position_(arena.current_pos_) {}

        ~Marker() {
            // Restore arena state
            while (arena_.blocks_.size() > block_index_ + 1) {
                delete[] arena_.blocks_.back();
                arena_.blocks_.pop_back();
            }
            arena_.current_block_ = arena_.blocks_[block_index_];
            arena_.current_pos_ = position_;
        }

    private:
        MemoryArena& arena_;
        size_t block_index_;
        size_t position_;
    };

    Marker create_marker() { return Marker(*this); }

private:
    void allocate_block(size_t size) {
        char* block = new char[size];
        blocks_.push_back(block);
        current_block_ = block;
        current_block_size_ = size;
        current_pos_ = 0;
    }

    std::vector<char*> blocks_;
    char* current_block_ = nullptr;
    size_t current_block_size_ = 0;
    size_t current_pos_ = 0;
    size_t block_size_;
    size_t total_allocated_ = 0;
};

//=============================================================================
// Frame Allocator (reset each frame)
//=============================================================================

class FrameAllocator {
public:
    static FrameAllocator& instance() {
        static FrameAllocator instance;
        return instance;
    }

    void begin_frame() {
        arena_.reset();
    }

    void end_frame() {
        // Could add stats collection here
    }

    void* allocate(size_t size, size_t alignment = 8) {
        return arena_.allocate(size, alignment);
    }

    template<typename T, typename... Args>
    T* create(Args&&... args) {
        return arena_.create<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    T* create_array(size_t count) {
        return arena_.create_array<T>(count);
    }

    size_t get_frame_allocation() const {
        return arena_.get_total_allocated();
    }

private:
    FrameAllocator() : arena_(2 * 1024 * 1024) {}  // 2MB frame arena
    MemoryArena arena_;
};

//=============================================================================
// Convenience Macros
//=============================================================================

#define FRAME_ALLOC(size) FrameAllocator::instance().allocate(size)
#define FRAME_NEW(Type, ...) FrameAllocator::instance().create<Type>(__VA_ARGS__)
#define FRAME_ARRAY(Type, count) FrameAllocator::instance().create_array<Type>(count)

#endif // MEMORY_ARENA_H
```

### src/platform/memory_arena.cpp

```cpp
// src/platform/memory_arena.cpp
// Frame Arena Implementation
// Task 18h - Performance Optimization

#include "platform/memory_arena.h"

// Template implementations are in the header
// This file exists for any non-template utilities and static instance
```

### src/graphics/sprite_batch.h

```cpp
// src/graphics/sprite_batch.h
// Sprite Batching for Efficient Rendering
// Task 18h - Performance Optimization

#ifndef SPRITE_BATCH_H
#define SPRITE_BATCH_H

#include <vector>
#include <cstdint>

//=============================================================================
// Sprite Vertex
//=============================================================================

struct SpriteVertex {
    float x, y;           // Position
    float u, v;           // Texture coordinates
    uint32_t color;       // RGBA packed color
};

//=============================================================================
// Sprite Instance
//=============================================================================

struct SpriteInstance {
    float x, y;           // Position
    float width, height;  // Size
    float u0, v0, u1, v1; // Texture rect (normalized)
    uint32_t color;       // Tint color
    float rotation;       // Rotation in radians
    uint32_t texture_id;  // Texture to use
    int layer;            // Sort layer (higher = on top)
};

//=============================================================================
// Sprite Batch
//=============================================================================

class SpriteBatch {
public:
    SpriteBatch(size_t max_sprites = 10000);
    ~SpriteBatch();

    // Begin/end batch
    void begin();
    void end();

    // Add sprite to batch
    void draw(const SpriteInstance& sprite);

    // Convenience methods
    void draw(uint32_t texture_id, float x, float y, float w, float h);
    void draw(uint32_t texture_id, float x, float y, float w, float h,
              float u0, float v0, float u1, float v1);
    void draw(uint32_t texture_id, float x, float y, float w, float h,
              float u0, float v0, float u1, float v1,
              uint32_t color, float rotation);

    // Flush current batch to GPU
    void flush();

    // Statistics
    int get_draw_calls() const { return draw_calls_; }
    int get_sprite_count() const { return sprite_count_; }
    int get_batch_count() const { return batch_count_; }

    // Settings
    void set_sort_enabled(bool enabled) { sort_enabled_ = enabled; }

private:
    struct Batch {
        uint32_t texture_id;
        int start_index;
        int count;
    };

    void sort_sprites();
    void build_batches();
    void render_batch(const Batch& batch);

    std::vector<SpriteInstance> sprites_;
    std::vector<SpriteVertex> vertices_;
    std::vector<uint16_t> indices_;
    std::vector<Batch> batches_;

    size_t max_sprites_;
    bool in_batch_ = false;
    bool sort_enabled_ = true;

    // Stats
    int draw_calls_ = 0;
    int sprite_count_ = 0;
    int batch_count_ = 0;

    // GPU resources (would be Metal buffers)
    void* vertex_buffer_ = nullptr;
    void* index_buffer_ = nullptr;
};

//=============================================================================
// Batch Renderer (Higher-level interface)
//=============================================================================

class BatchRenderer {
public:
    static BatchRenderer& instance();

    void begin_frame();
    void end_frame();

    // Layer management
    void set_layer(int layer);
    int get_layer() const { return current_layer_; }

    // Draw calls
    void draw_sprite(uint32_t texture_id, float x, float y, float w, float h);
    void draw_sprite_rotated(uint32_t texture_id, float x, float y, float w, float h,
                             float rotation);
    void draw_sprite_tinted(uint32_t texture_id, float x, float y, float w, float h,
                            uint32_t color);

    // Frame statistics
    int get_total_sprites() const;
    int get_total_draw_calls() const;
    int get_batches_rendered() const;

private:
    BatchRenderer();

    SpriteBatch batch_;
    int current_layer_ = 0;
};

#endif // SPRITE_BATCH_H
```

### src/graphics/sprite_batch.cpp

```cpp
// src/graphics/sprite_batch.cpp
// Sprite Batching Implementation
// Task 18h - Performance Optimization

#include "graphics/sprite_batch.h"
#include "platform/profiler.h"
#include <algorithm>
#include <cstring>

//=============================================================================
// SpriteBatch Implementation
//=============================================================================

SpriteBatch::SpriteBatch(size_t max_sprites)
    : max_sprites_(max_sprites) {
    sprites_.reserve(max_sprites);
    vertices_.reserve(max_sprites * 4);  // 4 vertices per sprite
    indices_.reserve(max_sprites * 6);   // 6 indices per sprite (2 triangles)
    batches_.reserve(100);  // Rough estimate

    // Pre-generate indices (they're the same pattern for all sprites)
    for (size_t i = 0; i < max_sprites; i++) {
        uint16_t base = static_cast<uint16_t>(i * 4);
        indices_.push_back(base + 0);
        indices_.push_back(base + 1);
        indices_.push_back(base + 2);
        indices_.push_back(base + 2);
        indices_.push_back(base + 3);
        indices_.push_back(base + 0);
    }

    // Would create GPU buffers here
}

SpriteBatch::~SpriteBatch() {
    // Would release GPU buffers here
}

void SpriteBatch::begin() {
    sprites_.clear();
    batches_.clear();
    in_batch_ = true;
    draw_calls_ = 0;
    sprite_count_ = 0;
    batch_count_ = 0;
}

void SpriteBatch::end() {
    if (!in_batch_) return;

    PROFILE_SCOPE("SpriteBatch::end");

    if (sort_enabled_) {
        sort_sprites();
    }
    build_batches();
    flush();

    in_batch_ = false;
}

void SpriteBatch::draw(const SpriteInstance& sprite) {
    if (!in_batch_) return;
    if (sprites_.size() >= max_sprites_) {
        // Batch full, flush and continue
        end();
        begin();
    }

    sprites_.push_back(sprite);
    sprite_count_++;
}

void SpriteBatch::draw(uint32_t texture_id, float x, float y, float w, float h) {
    draw(texture_id, x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f);
}

void SpriteBatch::draw(uint32_t texture_id, float x, float y, float w, float h,
                       float u0, float v0, float u1, float v1) {
    draw(texture_id, x, y, w, h, u0, v0, u1, v1, 0xFFFFFFFF, 0.0f);
}

void SpriteBatch::draw(uint32_t texture_id, float x, float y, float w, float h,
                       float u0, float v0, float u1, float v1,
                       uint32_t color, float rotation) {
    SpriteInstance sprite;
    sprite.x = x;
    sprite.y = y;
    sprite.width = w;
    sprite.height = h;
    sprite.u0 = u0;
    sprite.v0 = v0;
    sprite.u1 = u1;
    sprite.v1 = v1;
    sprite.color = color;
    sprite.rotation = rotation;
    sprite.texture_id = texture_id;
    sprite.layer = 0;

    draw(sprite);
}

void SpriteBatch::sort_sprites() {
    PROFILE_SCOPE("SpriteBatch::sort");

    // Sort by texture to minimize state changes, then by layer
    std::sort(sprites_.begin(), sprites_.end(),
        [](const SpriteInstance& a, const SpriteInstance& b) {
            if (a.layer != b.layer) return a.layer < b.layer;
            return a.texture_id < b.texture_id;
        });
}

void SpriteBatch::build_batches() {
    PROFILE_SCOPE("SpriteBatch::build_batches");

    if (sprites_.empty()) return;

    vertices_.clear();
    batches_.clear();

    uint32_t current_texture = sprites_[0].texture_id;
    int batch_start = 0;
    int count = 0;

    for (size_t i = 0; i < sprites_.size(); i++) {
        const SpriteInstance& s = sprites_[i];

        // Check for texture change
        if (s.texture_id != current_texture) {
            // End current batch
            Batch batch;
            batch.texture_id = current_texture;
            batch.start_index = batch_start * 6;
            batch.count = count * 6;
            batches_.push_back(batch);

            // Start new batch
            current_texture = s.texture_id;
            batch_start = static_cast<int>(i);
            count = 0;
        }

        // Generate vertices for this sprite
        float x0 = s.x;
        float y0 = s.y;
        float x1 = s.x + s.width;
        float y1 = s.y + s.height;

        // Handle rotation
        if (s.rotation != 0.0f) {
            float cx = s.x + s.width * 0.5f;
            float cy = s.y + s.height * 0.5f;
            float cos_r = std::cos(s.rotation);
            float sin_r = std::sin(s.rotation);

            // Rotate corners around center
            auto rotate = [cx, cy, cos_r, sin_r](float& px, float& py) {
                float dx = px - cx;
                float dy = py - cy;
                px = cx + dx * cos_r - dy * sin_r;
                py = cy + dx * sin_r + dy * cos_r;
            };

            float corners[4][2] = {
                {x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}
            };
            for (int j = 0; j < 4; j++) {
                rotate(corners[j][0], corners[j][1]);
            }

            vertices_.push_back({corners[0][0], corners[0][1], s.u0, s.v0, s.color});
            vertices_.push_back({corners[1][0], corners[1][1], s.u1, s.v0, s.color});
            vertices_.push_back({corners[2][0], corners[2][1], s.u1, s.v1, s.color});
            vertices_.push_back({corners[3][0], corners[3][1], s.u0, s.v1, s.color});
        } else {
            // No rotation - fast path
            vertices_.push_back({x0, y0, s.u0, s.v0, s.color});
            vertices_.push_back({x1, y0, s.u1, s.v0, s.color});
            vertices_.push_back({x1, y1, s.u1, s.v1, s.color});
            vertices_.push_back({x0, y1, s.u0, s.v1, s.color});
        }

        count++;
    }

    // Final batch
    if (count > 0) {
        Batch batch;
        batch.texture_id = current_texture;
        batch.start_index = batch_start * 6;
        batch.count = count * 6;
        batches_.push_back(batch);
    }

    batch_count_ = static_cast<int>(batches_.size());
}

void SpriteBatch::flush() {
    PROFILE_SCOPE("SpriteBatch::flush");

    if (vertices_.empty()) return;

    // Would upload vertices to GPU here
    // glBufferSubData or Metal buffer upload

    for (const Batch& batch : batches_) {
        render_batch(batch);
    }
}

void SpriteBatch::render_batch(const Batch& batch) {
    // Would bind texture and issue draw call here
    // glDrawElements or Metal drawIndexedPrimitives

    draw_calls_++;
    PROFILE_COUNTER("DrawCalls");

    (void)batch;  // Suppress unused warning in stub
}

//=============================================================================
// BatchRenderer Implementation
//=============================================================================

BatchRenderer& BatchRenderer::instance() {
    static BatchRenderer instance;
    return instance;
}

BatchRenderer::BatchRenderer() : batch_(10000) {}

void BatchRenderer::begin_frame() {
    batch_.begin();
    current_layer_ = 0;
}

void BatchRenderer::end_frame() {
    batch_.end();
}

void BatchRenderer::set_layer(int layer) {
    current_layer_ = layer;
}

void BatchRenderer::draw_sprite(uint32_t texture_id, float x, float y, float w, float h) {
    SpriteInstance sprite;
    sprite.texture_id = texture_id;
    sprite.x = x;
    sprite.y = y;
    sprite.width = w;
    sprite.height = h;
    sprite.u0 = 0.0f;
    sprite.v0 = 0.0f;
    sprite.u1 = 1.0f;
    sprite.v1 = 1.0f;
    sprite.color = 0xFFFFFFFF;
    sprite.rotation = 0.0f;
    sprite.layer = current_layer_;
    batch_.draw(sprite);
}

void BatchRenderer::draw_sprite_rotated(uint32_t texture_id, float x, float y,
                                        float w, float h, float rotation) {
    SpriteInstance sprite;
    sprite.texture_id = texture_id;
    sprite.x = x;
    sprite.y = y;
    sprite.width = w;
    sprite.height = h;
    sprite.u0 = 0.0f;
    sprite.v0 = 0.0f;
    sprite.u1 = 1.0f;
    sprite.v1 = 1.0f;
    sprite.color = 0xFFFFFFFF;
    sprite.rotation = rotation;
    sprite.layer = current_layer_;
    batch_.draw(sprite);
}

void BatchRenderer::draw_sprite_tinted(uint32_t texture_id, float x, float y,
                                       float w, float h, uint32_t color) {
    SpriteInstance sprite;
    sprite.texture_id = texture_id;
    sprite.x = x;
    sprite.y = y;
    sprite.width = w;
    sprite.height = h;
    sprite.u0 = 0.0f;
    sprite.v0 = 0.0f;
    sprite.u1 = 1.0f;
    sprite.v1 = 1.0f;
    sprite.color = color;
    sprite.rotation = 0.0f;
    sprite.layer = current_layer_;
    batch_.draw(sprite);
}

int BatchRenderer::get_total_sprites() const {
    return batch_.get_sprite_count();
}

int BatchRenderer::get_total_draw_calls() const {
    return batch_.get_draw_calls();
}

int BatchRenderer::get_batches_rendered() const {
    return batch_.get_batch_count();
}
```

### src/graphics/dirty_rect.h

```cpp
// src/graphics/dirty_rect.h
// Dirty Rectangle Tracking
// Task 18h - Performance Optimization

#ifndef DIRTY_RECT_H
#define DIRTY_RECT_H

#include <vector>
#include <cstdint>

//=============================================================================
// Rectangle Structure
//=============================================================================

struct Rect {
    int x, y;
    int width, height;

    int right() const { return x + width; }
    int bottom() const { return y + height; }

    bool intersects(const Rect& other) const {
        return !(x >= other.right() || right() <= other.x ||
                 y >= other.bottom() || bottom() <= other.y);
    }

    bool contains(int px, int py) const {
        return px >= x && px < right() && py >= y && py < bottom();
    }

    Rect merged_with(const Rect& other) const {
        int nx = (x < other.x) ? x : other.x;
        int ny = (y < other.y) ? y : other.y;
        int nr = (right() > other.right()) ? right() : other.right();
        int nb = (bottom() > other.bottom()) ? bottom() : other.bottom();
        return {nx, ny, nr - nx, nb - ny};
    }

    int area() const { return width * height; }
};

//=============================================================================
// Dirty Rectangle Tracker
//=============================================================================

class DirtyRectTracker {
public:
    DirtyRectTracker(int screen_width, int screen_height);

    // Mark region as dirty
    void mark_dirty(int x, int y, int width, int height);
    void mark_dirty(const Rect& rect);

    // Mark entire screen dirty
    void mark_all_dirty();

    // Clear dirty regions (call after rendering)
    void clear();

    // Get optimized dirty rectangles for rendering
    const std::vector<Rect>& get_dirty_rects() const { return merged_rects_; }

    // Check if anything is dirty
    bool has_dirty_rects() const { return !dirty_rects_.empty(); }

    // Check if using full redraw (optimization for many dirty rects)
    bool is_full_redraw() const { return full_redraw_; }

    // Optimization settings
    void set_merge_threshold(float threshold) { merge_threshold_ = threshold; }
    void set_max_rects(int max) { max_rects_ = max; }

    // Statistics
    int get_dirty_rect_count() const { return static_cast<int>(dirty_rects_.size()); }
    int get_merged_rect_count() const { return static_cast<int>(merged_rects_.size()); }
    int get_dirty_pixel_count() const;

private:
    void merge_rects();
    void optimize_rects();

    int screen_width_;
    int screen_height_;
    std::vector<Rect> dirty_rects_;
    std::vector<Rect> merged_rects_;
    bool full_redraw_ = false;

    float merge_threshold_ = 0.5f;  // Merge if combined area < 50% larger
    int max_rects_ = 20;            // Switch to full redraw above this
};

//=============================================================================
// Double-Buffered Dirty Tracking
//=============================================================================

class DoubleBufferedDirtyTracker {
public:
    DoubleBufferedDirtyTracker(int width, int height);

    // Mark dirty in current frame
    void mark_dirty(const Rect& rect);
    void mark_dirty(int x, int y, int w, int h);

    // Swap buffers at frame boundary
    void swap_buffers();

    // Get rects that need redrawing (union of both buffers for double-buffering)
    std::vector<Rect> get_redraw_rects() const;

    // Clear both buffers (e.g., on resize)
    void clear_all();

private:
    DirtyRectTracker front_;  // Current frame
    DirtyRectTracker back_;   // Previous frame
};

#endif // DIRTY_RECT_H
```

### src/graphics/dirty_rect.cpp

```cpp
// src/graphics/dirty_rect.cpp
// Dirty Rectangle Implementation
// Task 18h - Performance Optimization

#include "graphics/dirty_rect.h"
#include <algorithm>

//=============================================================================
// DirtyRectTracker Implementation
//=============================================================================

DirtyRectTracker::DirtyRectTracker(int screen_width, int screen_height)
    : screen_width_(screen_width)
    , screen_height_(screen_height) {
    dirty_rects_.reserve(100);
    merged_rects_.reserve(max_rects_);
}

void DirtyRectTracker::mark_dirty(int x, int y, int width, int height) {
    mark_dirty(Rect{x, y, width, height});
}

void DirtyRectTracker::mark_dirty(const Rect& rect) {
    // Clamp to screen bounds
    Rect clamped;
    clamped.x = (rect.x < 0) ? 0 : rect.x;
    clamped.y = (rect.y < 0) ? 0 : rect.y;
    clamped.width = rect.width;
    clamped.height = rect.height;

    if (clamped.right() > screen_width_) {
        clamped.width = screen_width_ - clamped.x;
    }
    if (clamped.bottom() > screen_height_) {
        clamped.height = screen_height_ - clamped.y;
    }

    if (clamped.width <= 0 || clamped.height <= 0) {
        return;  // Off screen or empty
    }

    dirty_rects_.push_back(clamped);

    // Check if we should switch to full redraw
    if (static_cast<int>(dirty_rects_.size()) > max_rects_ * 2) {
        full_redraw_ = true;
    }
}

void DirtyRectTracker::mark_all_dirty() {
    full_redraw_ = true;
    dirty_rects_.clear();
    dirty_rects_.push_back({0, 0, screen_width_, screen_height_});
}

void DirtyRectTracker::clear() {
    dirty_rects_.clear();
    merged_rects_.clear();
    full_redraw_ = false;
}

void DirtyRectTracker::merge_rects() {
    merged_rects_.clear();

    if (dirty_rects_.empty()) return;

    if (full_redraw_) {
        merged_rects_.push_back({0, 0, screen_width_, screen_height_});
        return;
    }

    // Simple greedy merge algorithm
    std::vector<bool> merged(dirty_rects_.size(), false);

    for (size_t i = 0; i < dirty_rects_.size(); i++) {
        if (merged[i]) continue;

        Rect current = dirty_rects_[i];

        // Try to merge with subsequent rects
        bool did_merge;
        do {
            did_merge = false;
            for (size_t j = i + 1; j < dirty_rects_.size(); j++) {
                if (merged[j]) continue;

                const Rect& other = dirty_rects_[j];

                // Check if merging would be beneficial
                Rect combined = current.merged_with(other);
                int original_area = current.area() + other.area();
                int combined_area = combined.area();

                // Merge if overlap or combined isn't much larger
                if (current.intersects(other) ||
                    combined_area < original_area * (1.0f + merge_threshold_)) {
                    current = combined;
                    merged[j] = true;
                    did_merge = true;
                }
            }
        } while (did_merge);

        merged_rects_.push_back(current);
    }

    // If still too many rects, fall back to full redraw
    if (static_cast<int>(merged_rects_.size()) > max_rects_) {
        full_redraw_ = true;
        merged_rects_.clear();
        merged_rects_.push_back({0, 0, screen_width_, screen_height_});
    }
}

void DirtyRectTracker::optimize_rects() {
    merge_rects();
}

int DirtyRectTracker::get_dirty_pixel_count() const {
    int total = 0;
    for (const Rect& r : merged_rects_) {
        total += r.area();
    }
    return total;
}

//=============================================================================
// DoubleBufferedDirtyTracker Implementation
//=============================================================================

DoubleBufferedDirtyTracker::DoubleBufferedDirtyTracker(int width, int height)
    : front_(width, height)
    , back_(width, height) {
}

void DoubleBufferedDirtyTracker::mark_dirty(const Rect& rect) {
    front_.mark_dirty(rect);
}

void DoubleBufferedDirtyTracker::mark_dirty(int x, int y, int w, int h) {
    front_.mark_dirty(x, y, w, h);
}

void DoubleBufferedDirtyTracker::swap_buffers() {
    // Swap front and back
    std::swap(front_, back_);
    front_.clear();
}

std::vector<Rect> DoubleBufferedDirtyTracker::get_redraw_rects() const {
    // For double-buffered rendering, need union of both frames
    // because back buffer wasn't updated with previous frame's changes
    std::vector<Rect> result;

    const auto& front_rects = front_.get_dirty_rects();
    const auto& back_rects = back_.get_dirty_rects();

    result.reserve(front_rects.size() + back_rects.size());
    result.insert(result.end(), front_rects.begin(), front_rects.end());
    result.insert(result.end(), back_rects.begin(), back_rects.end());

    return result;
}

void DoubleBufferedDirtyTracker::clear_all() {
    front_.clear();
    back_.clear();
}
```

---

## CMakeLists.txt Additions

```cmake
# Task 18h: Performance Optimization

set(OPTIMIZATION_SOURCES
    ${CMAKE_SOURCE_DIR}/src/platform/profiler.cpp
    ${CMAKE_SOURCE_DIR}/src/platform/memory_pool.cpp
    ${CMAKE_SOURCE_DIR}/src/platform/memory_arena.cpp
    ${CMAKE_SOURCE_DIR}/src/graphics/sprite_batch.cpp
    ${CMAKE_SOURCE_DIR}/src/graphics/dirty_rect.cpp
)

# Add to main library
target_sources(redalert_platform PRIVATE
    ${CMAKE_SOURCE_DIR}/src/platform/profiler.cpp
    ${CMAKE_SOURCE_DIR}/src/platform/memory_pool.cpp
    ${CMAKE_SOURCE_DIR}/src/platform/memory_arena.cpp
)

target_sources(redalert_graphics PRIVATE
    ${CMAKE_SOURCE_DIR}/src/graphics/sprite_batch.cpp
    ${CMAKE_SOURCE_DIR}/src/graphics/dirty_rect.cpp
)
```

---

## Verification Script

```bash
#!/bin/bash
set -e
echo "=== Task 18h Verification: Performance Optimization ==="
ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"

# Check files exist
for file in \
    "src/platform/profiler.h" \
    "src/platform/profiler.cpp" \
    "src/platform/memory_pool.h" \
    "src/platform/memory_arena.h" \
    "src/graphics/sprite_batch.h" \
    "src/graphics/sprite_batch.cpp" \
    "src/graphics/dirty_rect.h" \
    "src/graphics/dirty_rect.cpp"
do
    test -f "$ROOT_DIR/$file" || { echo "FAIL: $file missing"; exit 1; }
done

# Build
cd "$BUILD_DIR" && cmake --build . --target redalert_platform --target redalert_graphics

# Run performance tests to validate
"$BUILD_DIR/PerformanceTests" || exit 1

echo "=== Task 18h Verification PASSED ==="
```

---

## Success Criteria

1. Profiler captures frame times and sample durations
2. Object pool allocates/deallocates without heap calls
3. Frame arena resets cleanly between frames
4. Sprite batch reduces draw calls through grouping
5. Dirty rect tracker merges overlapping regions
6. Performance tests show target FPS achieved

---

## Completion Promise

When verification passes, output:
<promise>TASK_18H_COMPLETE</promise>

## Escape Hatch

If stuck after 15 iterations, output:
<promise>TASK_18H_BLOCKED</promise>

## Max Iterations
15
