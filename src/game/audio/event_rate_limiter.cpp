// src/game/audio/event_rate_limiter.cpp
// Rate limiting for audio events
// Task 17f - Game Audio Events

#include "game/audio/event_rate_limiter.h"
#include "platform.h"

//=============================================================================
// EventRateLimiter Implementation
//=============================================================================

EventRateLimiter::EventRateLimiter() {
}

void EventRateLimiter::SetGlobalCooldown(int event_type, uint32_t cooldown_ms) {
    configs_[event_type].global_cooldown_ms = cooldown_ms;
}

void EventRateLimiter::SetPositionCooldown(int event_type, uint32_t cooldown_ms) {
    configs_[event_type].position_cooldown_ms = cooldown_ms;
}

void EventRateLimiter::SetObjectCooldown(int event_type, uint32_t cooldown_ms) {
    configs_[event_type].object_cooldown_ms = cooldown_ms;
}

uint32_t EventRateLimiter::GetCurrentTime() const {
    return Platform_Timer_GetTicks();
}

bool EventRateLimiter::CheckCooldown(uint32_t last_time, uint32_t cooldown_ms) const {
    if (cooldown_ms == 0) {
        return true;  // No cooldown configured
    }

    uint32_t now = GetCurrentTime();
    uint32_t elapsed = now - last_time;

    // Handle wraparound
    if (elapsed > 0x80000000) {
        return true;  // Wrapped around, allow event
    }

    return elapsed >= cooldown_ms;
}

bool EventRateLimiter::CanFireGlobal(int event_type) {
    auto config_it = configs_.find(event_type);
    if (config_it == configs_.end() || config_it->second.global_cooldown_ms == 0) {
        return true;  // No config, allow
    }

    auto time_it = global_times_.find(event_type);
    if (time_it == global_times_.end()) {
        // First time, allow and record
        global_times_[event_type] = GetCurrentTime();
        return true;
    }

    if (CheckCooldown(time_it->second, config_it->second.global_cooldown_ms)) {
        time_it->second = GetCurrentTime();
        return true;
    }

    return false;  // Rate limited
}

bool EventRateLimiter::CanFireAtPosition(int event_type, int cell_x, int cell_y) {
    auto config_it = configs_.find(event_type);
    if (config_it == configs_.end() || config_it->second.position_cooldown_ms == 0) {
        return true;  // No config, allow
    }

    PositionKey key = {static_cast<int16_t>(cell_x), static_cast<int16_t>(cell_y)};

    auto& position_map = position_times_[event_type];
    auto time_it = position_map.find(key);

    if (time_it == position_map.end()) {
        // First time at this position, allow and record
        position_map[key] = GetCurrentTime();
        return true;
    }

    if (CheckCooldown(time_it->second, config_it->second.position_cooldown_ms)) {
        time_it->second = GetCurrentTime();
        return true;
    }

    return false;  // Rate limited
}

bool EventRateLimiter::CanFireForObject(int event_type, ObjectKey object_id) {
    auto config_it = configs_.find(event_type);
    if (config_it == configs_.end() || config_it->second.object_cooldown_ms == 0) {
        return true;  // No config, allow
    }

    auto& object_map = object_times_[event_type];
    auto time_it = object_map.find(object_id);

    if (time_it == object_map.end()) {
        // First time for this object, allow and record
        object_map[object_id] = GetCurrentTime();
        return true;
    }

    if (CheckCooldown(time_it->second, config_it->second.object_cooldown_ms)) {
        time_it->second = GetCurrentTime();
        return true;
    }

    return false;  // Rate limited
}

bool EventRateLimiter::CanFireGlobalAndPosition(int event_type, int cell_x, int cell_y) {
    // Check global first
    auto config_it = configs_.find(event_type);
    if (config_it != configs_.end() && config_it->second.global_cooldown_ms > 0) {
        auto time_it = global_times_.find(event_type);
        if (time_it != global_times_.end()) {
            if (!CheckCooldown(time_it->second, config_it->second.global_cooldown_ms)) {
                return false;  // Global rate limited
            }
        }
    }

    // Check position
    if (!CanFireAtPosition(event_type, cell_x, cell_y)) {
        return false;  // Position rate limited
    }

    // Update global time
    global_times_[event_type] = GetCurrentTime();
    return true;
}

bool EventRateLimiter::CanFireGlobalAndObject(int event_type, ObjectKey object_id) {
    // Check global first
    auto config_it = configs_.find(event_type);
    if (config_it != configs_.end() && config_it->second.global_cooldown_ms > 0) {
        auto time_it = global_times_.find(event_type);
        if (time_it != global_times_.end()) {
            if (!CheckCooldown(time_it->second, config_it->second.global_cooldown_ms)) {
                return false;  // Global rate limited
            }
        }
    }

    // Check object
    if (!CanFireForObject(event_type, object_id)) {
        return false;  // Object rate limited
    }

    // Update global time
    global_times_[event_type] = GetCurrentTime();
    return true;
}

void EventRateLimiter::Cleanup() {
    uint32_t now = GetCurrentTime();
    const uint32_t CLEANUP_THRESHOLD = 60000;  // 60 seconds

    // Clean position maps
    for (auto& pair : position_times_) {
        auto& map = pair.second;
        for (auto it = map.begin(); it != map.end(); ) {
            uint32_t elapsed = now - it->second;
            if (elapsed > CLEANUP_THRESHOLD) {
                it = map.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Clean object maps
    for (auto& pair : object_times_) {
        auto& map = pair.second;
        for (auto it = map.begin(); it != map.end(); ) {
            uint32_t elapsed = now - it->second;
            if (elapsed > CLEANUP_THRESHOLD) {
                it = map.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void EventRateLimiter::Reset() {
    global_times_.clear();
    position_times_.clear();
    object_times_.clear();
}

size_t EventRateLimiter::GetTrackedCount() const {
    size_t count = global_times_.size();

    for (const auto& pair : position_times_) {
        count += pair.second.size();
    }

    for (const auto& pair : object_times_) {
        count += pair.second.size();
    }

    return count;
}

//=============================================================================
// Global Instance
//=============================================================================

EventRateLimiter& GetEventRateLimiter() {
    static EventRateLimiter instance;
    return instance;
}
