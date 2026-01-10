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
};

#endif // PROFILER_H
