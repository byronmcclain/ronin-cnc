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
}

void GPUProfiler::begin_gpu_sample(const std::string& name) {
    (void)name;
}

void GPUProfiler::end_gpu_sample(const std::string& name) {
    (void)name;
}

std::string GPUProfiler::get_report() const {
    std::ostringstream oss;
    oss << "GPU Time: " << last_gpu_time_ms_ << " ms\n";
    return oss.str();
}
