// include/game/audio/event_rate_limiter.h
// Rate limiting for audio events
// Task 17f - Game Audio Events

#ifndef EVENT_RATE_LIMITER_H
#define EVENT_RATE_LIMITER_H

#include <cstdint>
#include <unordered_map>

//=============================================================================
// Rate Limiter Key Types
//=============================================================================

/// Key for position-based rate limiting (cell coordinates)
struct PositionKey {
    int16_t cell_x;
    int16_t cell_y;

    bool operator==(const PositionKey& other) const {
        return cell_x == other.cell_x && cell_y == other.cell_y;
    }
};

/// Hash function for PositionKey
struct PositionKeyHash {
    size_t operator()(const PositionKey& key) const {
        return (static_cast<size_t>(key.cell_x) << 16) |
               static_cast<size_t>(static_cast<uint16_t>(key.cell_y));
    }
};

/// Key for object-based rate limiting
using ObjectKey = uint32_t;  // Object ID or pointer cast

//=============================================================================
// Event Rate Limiter
//=============================================================================

/// Rate limiter for audio events to prevent spam
class EventRateLimiter {
public:
    EventRateLimiter();

    //=========================================================================
    // Configuration
    //=========================================================================

    /// Set global rate limit for an event type
    /// @param event_type Event type identifier
    /// @param cooldown_ms Minimum time between events in milliseconds
    void SetGlobalCooldown(int event_type, uint32_t cooldown_ms);

    /// Set per-position rate limit for an event type
    /// @param event_type Event type identifier
    /// @param cooldown_ms Minimum time between events at same position
    void SetPositionCooldown(int event_type, uint32_t cooldown_ms);

    /// Set per-object rate limit for an event type
    /// @param event_type Event type identifier
    /// @param cooldown_ms Minimum time between events for same object
    void SetObjectCooldown(int event_type, uint32_t cooldown_ms);

    //=========================================================================
    // Rate Limit Checking
    //=========================================================================

    /// Check if global event can fire
    /// @param event_type Event type identifier
    /// @return true if event can fire (not rate limited)
    bool CanFireGlobal(int event_type);

    /// Check if position-based event can fire
    /// @param event_type Event type identifier
    /// @param cell_x Cell X coordinate
    /// @param cell_y Cell Y coordinate
    /// @return true if event can fire at this position
    bool CanFireAtPosition(int event_type, int cell_x, int cell_y);

    /// Check if object-based event can fire
    /// @param event_type Event type identifier
    /// @param object_id Object identifier
    /// @return true if event can fire for this object
    bool CanFireForObject(int event_type, ObjectKey object_id);

    /// Combined check: global AND position
    bool CanFireGlobalAndPosition(int event_type, int cell_x, int cell_y);

    /// Combined check: global AND object
    bool CanFireGlobalAndObject(int event_type, ObjectKey object_id);

    //=========================================================================
    // Maintenance
    //=========================================================================

    /// Clear old entries to prevent memory growth
    /// Call periodically (e.g., every few seconds)
    void Cleanup();

    /// Reset all rate limiters
    void Reset();

    /// Get number of tracked entries (for debugging)
    size_t GetTrackedCount() const;

private:
    struct CooldownConfig {
        uint32_t global_cooldown_ms = 0;
        uint32_t position_cooldown_ms = 0;
        uint32_t object_cooldown_ms = 0;
    };

    // Cooldown configurations per event type
    std::unordered_map<int, CooldownConfig> configs_;

    // Last fire times
    std::unordered_map<int, uint32_t> global_times_;
    std::unordered_map<int, std::unordered_map<PositionKey, uint32_t, PositionKeyHash>> position_times_;
    std::unordered_map<int, std::unordered_map<ObjectKey, uint32_t>> object_times_;

    uint32_t GetCurrentTime() const;
    bool CheckCooldown(uint32_t last_time, uint32_t cooldown_ms) const;
};

//=============================================================================
// Global Rate Limiter Instance
//=============================================================================

/// Get the global event rate limiter
EventRateLimiter& GetEventRateLimiter();

#endif // EVENT_RATE_LIMITER_H
