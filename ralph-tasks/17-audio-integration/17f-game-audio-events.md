# Task 17f: Game Audio Events

| Field | Value |
|-------|-------|
| **Task ID** | 17f |
| **Task Name** | Game Audio Events |
| **Phase** | 17 - Audio Integration |
| **Depends On** | Tasks 17a-17e |
| **Estimated Complexity** | Medium (~450 lines) |

---

## Dependencies

- Task 17a (AUD File Parser) - AudFile class
- Task 17b (Sound Manager) - SoundManager, SoundEffect, positional audio
- Task 17c (Music System) - MusicPlayer for game state music
- Task 17d (Voice System) - VoiceManager, EvaVoice, UnitVoice
- Task 17e (Audio Integration) - AudioSystem unified interface
- Platform timer API (for rate limiting)

---

## Context

Game Audio Events is the final audio task that connects the audio subsystems to actual game events. This module:

1. **Event Handlers**: Functions called when game events occur (damage, selection, movement, etc.)
2. **Rate Limiting**: Prevents audio spam from rapid repeated events
3. **Context-Aware Audio**: Different sounds based on event context (damage amount, unit type, etc.)
4. **Game State Transitions**: Music changes based on game state
5. **Combat Audio**: Weapon sounds, explosions, impacts
6. **EVA Notifications**: Important game events trigger EVA announcements

This is the glue layer that makes the game feel alive with appropriate audio feedback.

---

## Technical Background

### Rate Limiting Strategy

Different events need different rate limiting:
- **High-frequency events** (damage ticks): Aggressive limiting (100-200ms cooldown)
- **Unit acknowledgments**: Medium limiting (500ms per unit, 200ms global)
- **EVA announcements**: Heavy limiting (5-30 seconds depending on importance)
- **UI sounds**: Light limiting (50ms to prevent double-clicks)

### Event Categories

| Category | Examples | Rate Limit |
|----------|----------|------------|
| Combat | Explosions, weapons | Per-position (cell-based) |
| Unit Response | "Reporting", "Moving" | Per-unit + global |
| EVA Critical | "Base under attack" | 30 seconds |
| EVA Normal | "Unit ready" | 2 seconds |
| UI | Click, build complete | 50ms |
| Ambient | Movement sounds | Per-unit |

### Original Game Audio Behavior

The original Red Alert uses these audio patterns:
- Unit selection plays random acknowledgment from a set
- Repeated commands to same unit play different responses
- EVA warnings have long cooldowns to prevent annoyance
- Explosions at same location are culled
- Music changes based on combat proximity

---

## Objective

Create a Game Audio Events system that:
1. Provides event handler functions for all major game events
2. Implements intelligent rate limiting per-event-type
3. Plays context-appropriate sounds (damage amount, unit type, etc.)
4. Manages EVA announcements with priority queue
5. Handles game state music transitions
6. Provides debug/logging for audio event flow

---

## Deliverables

- [ ] `include/game/audio/audio_events.h` - Event handler declarations
- [ ] `include/game/audio/event_rate_limiter.h` - Rate limiting system
- [ ] `src/game/audio/audio_events.cpp` - Event handler implementations
- [ ] `src/game/audio/event_rate_limiter.cpp` - Rate limiter implementation
- [ ] `src/test_audio_events.cpp` - Unit and integration tests
- [ ] CMakeLists.txt updated
- [ ] All tests pass

---

## Files to Create

### include/game/audio/event_rate_limiter.h

```cpp
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
```

### include/game/audio/audio_events.h

```cpp
// include/game/audio/audio_events.h
// Game Audio Event Handlers
// Task 17f - Game Audio Events

#ifndef AUDIO_EVENTS_H
#define AUDIO_EVENTS_H

#include <cstdint>

//=============================================================================
// Forward Declarations (Game Types)
//=============================================================================

// These would normally be in game headers
// For now, use void* or simple types as placeholders

struct ObjectClass;
struct BuildingClass;
struct UnitClass;
struct InfantryClass;
struct AircraftClass;
struct VesselClass;
struct HouseClass;
struct WeaponTypeClass;
struct BulletClass;

//=============================================================================
// Audio Event Types (for rate limiting)
//=============================================================================

enum class AudioEventType : int {
    // Combat events
    DAMAGE_SMALL = 0,
    DAMAGE_LARGE,
    EXPLOSION_SMALL,
    EXPLOSION_LARGE,
    WEAPON_FIRE,
    PROJECTILE_IMPACT,

    // Unit events
    UNIT_SELECT,
    UNIT_MOVE_ORDER,
    UNIT_ATTACK_ORDER,
    UNIT_ACKNOWLEDGE,
    UNIT_DEATH,
    UNIT_CREATED,

    // Building events
    BUILDING_PLACED,
    BUILDING_COMPLETE,
    BUILDING_SOLD,
    BUILDING_DESTROYED,
    BUILDING_CAPTURED,

    // EVA events
    EVA_BASE_ATTACK,
    EVA_UNIT_LOST,
    EVA_BUILDING_LOST,
    EVA_LOW_POWER,
    EVA_INSUFFICIENT_FUNDS,
    EVA_SILOS_NEEDED,
    EVA_RADAR_ONLINE,
    EVA_RADAR_OFFLINE,
    EVA_UNIT_READY,
    EVA_CONSTRUCTION_COMPLETE,
    EVA_MISSION_COMPLETE,
    EVA_MISSION_FAILED,

    // UI events
    UI_CLICK,
    UI_BUILD_CLICK,
    UI_TAB_CLICK,
    UI_SIDEBAR_UP,
    UI_SIDEBAR_DOWN,
    UI_CANNOT,
    UI_PRIMARY_SET,

    // Ambient events
    AMBIENT_MOVEMENT,

    EVENT_TYPE_COUNT
};

//=============================================================================
// Audio Events Configuration
//=============================================================================

/// Initialize audio events system
/// Sets up rate limiters with default cooldowns
void AudioEvents_Init();

/// Shutdown audio events system
void AudioEvents_Shutdown();

//=============================================================================
// Combat Event Handlers
//=============================================================================

/// Called when an object takes damage
/// @param obj The damaged object
/// @param damage Amount of damage taken
/// @param source_x World X of damage source (for directional audio)
/// @param source_y World Y of damage source
void AudioEvent_Damage(ObjectClass* obj, int damage, int source_x, int source_y);

/// Called when an explosion occurs
/// @param world_x World X coordinate
/// @param world_y World Y coordinate
/// @param size Explosion size (0 = small, 1 = medium, 2 = large)
void AudioEvent_Explosion(int world_x, int world_y, int size);

/// Called when a weapon fires
/// @param weapon The weapon type being fired
/// @param world_x World X coordinate
/// @param world_y World Y coordinate
void AudioEvent_WeaponFire(WeaponTypeClass* weapon, int world_x, int world_y);

/// Called when a projectile impacts
/// @param bullet The impacting projectile
/// @param world_x Impact world X
/// @param world_y Impact world Y
void AudioEvent_ProjectileImpact(BulletClass* bullet, int world_x, int world_y);

//=============================================================================
// Unit Event Handlers
//=============================================================================

/// Called when a unit is selected by the player
/// @param obj The selected object
void AudioEvent_UnitSelected(ObjectClass* obj);

/// Called when a unit is given a move order
/// @param obj The ordered unit
void AudioEvent_UnitMoveOrder(ObjectClass* obj);

/// Called when a unit is given an attack order
/// @param obj The ordered unit
void AudioEvent_UnitAttackOrder(ObjectClass* obj);

/// Called when a unit dies
/// @param obj The dying unit
void AudioEvent_UnitDeath(ObjectClass* obj);

/// Called when a new unit is created
/// @param obj The new unit
/// @param house The owning house
void AudioEvent_UnitCreated(ObjectClass* obj, HouseClass* house);

//=============================================================================
// Building Event Handlers
//=============================================================================

/// Called when a building placement begins
/// @param building The building being placed
void AudioEvent_BuildingPlaced(BuildingClass* building);

/// Called when building construction completes
/// @param building The completed building
/// @param house The owning house
void AudioEvent_BuildingComplete(BuildingClass* building, HouseClass* house);

/// Called when a building is sold
/// @param building The sold building
void AudioEvent_BuildingSold(BuildingClass* building);

/// Called when a building is destroyed
/// @param building The destroyed building
/// @param house The owning house
void AudioEvent_BuildingDestroyed(BuildingClass* building, HouseClass* house);

/// Called when a building is captured
/// @param building The captured building
/// @param old_owner Previous owner
/// @param new_owner New owner
void AudioEvent_BuildingCaptured(BuildingClass* building, HouseClass* old_owner, HouseClass* new_owner);

//=============================================================================
// EVA Announcement Handlers
//=============================================================================

/// Called when player's base is under attack
/// @param house The attacked house
void AudioEvent_BaseUnderAttack(HouseClass* house);

/// Called when player loses a unit
/// @param house The owning house
void AudioEvent_UnitLost(HouseClass* house);

/// Called when player loses a building
/// @param house The owning house
void AudioEvent_BuildingLost(HouseClass* house);

/// Called when power drops to low level
/// @param house The affected house
void AudioEvent_LowPower(HouseClass* house);

/// Called when player has insufficient funds for an action
/// @param house The affected house
void AudioEvent_InsufficientFunds(HouseClass* house);

/// Called when ore silos are needed
/// @param house The affected house
void AudioEvent_SilosNeeded(HouseClass* house);

/// Called when radar comes online
/// @param house The affected house
void AudioEvent_RadarOnline(HouseClass* house);

/// Called when radar goes offline
/// @param house The affected house
void AudioEvent_RadarOffline(HouseClass* house);

/// Called when unit training completes
/// @param house The owning house
void AudioEvent_UnitReady(HouseClass* house);

/// Called when building construction completes (EVA)
/// @param house The owning house
void AudioEvent_ConstructionComplete(HouseClass* house);

/// Called when mission is accomplished
void AudioEvent_MissionAccomplished();

/// Called when mission fails
void AudioEvent_MissionFailed();

//=============================================================================
// UI Event Handlers
//=============================================================================

/// Called when UI element is clicked
void AudioEvent_UIClick();

/// Called when build button is clicked
void AudioEvent_UIBuildClick();

/// Called when sidebar tab is clicked
void AudioEvent_UITabClick();

/// Called when sidebar scrolls up
void AudioEvent_UISidebarUp();

/// Called when sidebar scrolls down
void AudioEvent_UISidebarDown();

/// Called when action is not allowed
void AudioEvent_UICannot();

/// Called when primary building is set
void AudioEvent_UIPrimarySet();

//=============================================================================
// Game State Audio
//=============================================================================

/// Called when entering main menu
void AudioEvent_EnterMainMenu();

/// Called when entering mission briefing
/// @param mission_number Mission number (1-14 typically)
/// @param is_allied true for Allied campaign, false for Soviet
void AudioEvent_EnterBriefing(int mission_number, bool is_allied);

/// Called when starting a mission
void AudioEvent_MissionStart();

/// Called when combat begins (enemies spotted)
void AudioEvent_CombatStart();

/// Called when combat ends (no enemies in range for a while)
void AudioEvent_CombatEnd();

/// Called when game is won
void AudioEvent_Victory();

/// Called when game is lost
void AudioEvent_Defeat();

//=============================================================================
// Movement/Ambient Sounds
//=============================================================================

/// Called when a unit moves (for movement sound loop)
/// @param obj The moving unit
/// @param is_moving true if unit is currently moving
void AudioEvent_UnitMovement(ObjectClass* obj, bool is_moving);

//=============================================================================
// Debug/Stats
//=============================================================================

/// Print audio event statistics
void AudioEvents_PrintStats();

/// Get total events triggered this session
uint32_t AudioEvents_GetTotalTriggered();

/// Get total events rate-limited this session
uint32_t AudioEvents_GetTotalRateLimited();

#endif // AUDIO_EVENTS_H
```

### src/game/audio/event_rate_limiter.cpp

```cpp
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
```

### src/game/audio/audio_events.cpp

```cpp
// src/game/audio/audio_events.cpp
// Game Audio Event Handlers
// Task 17f - Game Audio Events

#include "game/audio/audio_events.h"
#include "game/audio/event_rate_limiter.h"
#include "game/audio/audio_system.h"
#include "game/audio/sound_effect.h"
#include "game/audio/voice_event.h"
#include "game/audio/music_track.h"
#include "platform.h"

//=============================================================================
// Statistics Tracking
//=============================================================================

static uint32_t s_total_triggered = 0;
static uint32_t s_total_rate_limited = 0;
static bool s_initialized = false;

//=============================================================================
// Helper Macros
//=============================================================================

#define CHECK_INITIALIZED() if (!s_initialized) return

#define RATE_LIMITED_EVENT(event_type) \
    s_total_triggered++; \
    if (!GetEventRateLimiter().CanFireGlobal(static_cast<int>(event_type))) { \
        s_total_rate_limited++; \
        return; \
    }

#define RATE_LIMITED_POSITION_EVENT(event_type, cx, cy) \
    s_total_triggered++; \
    if (!GetEventRateLimiter().CanFireGlobalAndPosition(static_cast<int>(event_type), cx, cy)) { \
        s_total_rate_limited++; \
        return; \
    }

#define RATE_LIMITED_OBJECT_EVENT(event_type, obj_id) \
    s_total_triggered++; \
    if (!GetEventRateLimiter().CanFireGlobalAndObject(static_cast<int>(event_type), obj_id)) { \
        s_total_rate_limited++; \
        return; \
    }

//=============================================================================
// Coordinate Conversion Helpers
//=============================================================================

// Convert world coordinates to cell coordinates
static inline int WorldToCell(int world_coord) {
    return world_coord / 24;  // 24 pixels per cell (ICON_PIXEL_W)
}

// Get object ID for rate limiting (use pointer as ID for now)
static inline ObjectKey GetObjectId(void* obj) {
    return static_cast<ObjectKey>(reinterpret_cast<uintptr_t>(obj));
}

// Check if house is the player's house
static inline bool IsPlayerHouse(HouseClass* house) {
    // In actual implementation, compare against PlayerPtr
    // For now, assume all houses are player for EVA events
    return house != nullptr;
}

//=============================================================================
// Initialization
//=============================================================================

void AudioEvents_Init() {
    if (s_initialized) {
        return;
    }

    Platform_Log("AudioEvents: Initializing rate limiters...");

    EventRateLimiter& limiter = GetEventRateLimiter();

    // Combat events - position-based limiting to prevent explosion spam
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::DAMAGE_SMALL), 50);
    limiter.SetPositionCooldown(static_cast<int>(AudioEventType::DAMAGE_SMALL), 150);

    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::DAMAGE_LARGE), 100);
    limiter.SetPositionCooldown(static_cast<int>(AudioEventType::DAMAGE_LARGE), 200);

    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::EXPLOSION_SMALL), 50);
    limiter.SetPositionCooldown(static_cast<int>(AudioEventType::EXPLOSION_SMALL), 150);

    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::EXPLOSION_LARGE), 100);
    limiter.SetPositionCooldown(static_cast<int>(AudioEventType::EXPLOSION_LARGE), 300);

    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::WEAPON_FIRE), 30);
    limiter.SetPositionCooldown(static_cast<int>(AudioEventType::WEAPON_FIRE), 100);

    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::PROJECTILE_IMPACT), 50);
    limiter.SetPositionCooldown(static_cast<int>(AudioEventType::PROJECTILE_IMPACT), 100);

    // Unit events - object-based limiting
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::UNIT_SELECT), 100);
    limiter.SetObjectCooldown(static_cast<int>(AudioEventType::UNIT_SELECT), 500);

    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::UNIT_MOVE_ORDER), 150);
    limiter.SetObjectCooldown(static_cast<int>(AudioEventType::UNIT_MOVE_ORDER), 400);

    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::UNIT_ATTACK_ORDER), 150);
    limiter.SetObjectCooldown(static_cast<int>(AudioEventType::UNIT_ATTACK_ORDER), 400);

    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::UNIT_ACKNOWLEDGE), 200);
    limiter.SetObjectCooldown(static_cast<int>(AudioEventType::UNIT_ACKNOWLEDGE), 500);

    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::UNIT_DEATH), 100);
    limiter.SetPositionCooldown(static_cast<int>(AudioEventType::UNIT_DEATH), 200);

    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::UNIT_CREATED), 500);

    // Building events
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::BUILDING_PLACED), 200);
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::BUILDING_COMPLETE), 1000);
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::BUILDING_SOLD), 500);
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::BUILDING_DESTROYED), 300);
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::BUILDING_CAPTURED), 500);

    // EVA events - long cooldowns to prevent annoyance
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::EVA_BASE_ATTACK), 30000);      // 30 seconds
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::EVA_UNIT_LOST), 5000);         // 5 seconds
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::EVA_BUILDING_LOST), 5000);     // 5 seconds
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::EVA_LOW_POWER), 10000);        // 10 seconds
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::EVA_INSUFFICIENT_FUNDS), 5000); // 5 seconds
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::EVA_SILOS_NEEDED), 15000);     // 15 seconds
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::EVA_RADAR_ONLINE), 2000);      // 2 seconds
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::EVA_RADAR_OFFLINE), 2000);     // 2 seconds
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::EVA_UNIT_READY), 1500);        // 1.5 seconds
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::EVA_CONSTRUCTION_COMPLETE), 1500);

    // UI events - short cooldowns
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::UI_CLICK), 50);
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::UI_BUILD_CLICK), 100);
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::UI_TAB_CLICK), 100);
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::UI_SIDEBAR_UP), 100);
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::UI_SIDEBAR_DOWN), 100);
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::UI_CANNOT), 200);
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::UI_PRIMARY_SET), 200);

    // Ambient - per-object
    limiter.SetObjectCooldown(static_cast<int>(AudioEventType::AMBIENT_MOVEMENT), 2000);

    s_total_triggered = 0;
    s_total_rate_limited = 0;
    s_initialized = true;

    Platform_Log("AudioEvents: Initialized with rate limiters");
}

void AudioEvents_Shutdown() {
    if (!s_initialized) {
        return;
    }

    GetEventRateLimiter().Reset();
    s_initialized = false;

    Platform_Log("AudioEvents: Shutdown (triggered: %u, rate-limited: %u)",
                 s_total_triggered, s_total_rate_limited);
}

//=============================================================================
// Combat Event Handlers
//=============================================================================

void AudioEvent_Damage(ObjectClass* obj, int damage, int source_x, int source_y) {
    CHECK_INITIALIZED();

    if (!obj) return;

    // Get position for rate limiting (assuming obj has Coord member)
    // In real implementation: int cx = WorldToCell(Coord_X(obj->Coord));
    int cx = WorldToCell(source_x);
    int cy = WorldToCell(source_y);

    if (damage > 50) {
        RATE_LIMITED_POSITION_EVENT(AudioEventType::DAMAGE_LARGE, cx, cy);
        AudioSystem::Instance().PlaySoundAt(SoundEffect::EXPLODE_LARGE, source_x, source_y);
    } else if (damage > 10) {
        RATE_LIMITED_POSITION_EVENT(AudioEventType::DAMAGE_SMALL, cx, cy);
        AudioSystem::Instance().PlaySoundAt(SoundEffect::EXPLODE_SMALL, source_x, source_y);
    }
    // Very small damage doesn't play sound
}

void AudioEvent_Explosion(int world_x, int world_y, int size) {
    CHECK_INITIALIZED();

    int cx = WorldToCell(world_x);
    int cy = WorldToCell(world_y);

    if (size >= 2) {
        RATE_LIMITED_POSITION_EVENT(AudioEventType::EXPLOSION_LARGE, cx, cy);
        AudioSystem::Instance().PlaySoundAt(SoundEffect::EXPLODE_LARGE, world_x, world_y);
    } else if (size >= 1) {
        RATE_LIMITED_POSITION_EVENT(AudioEventType::EXPLOSION_SMALL, cx, cy);
        AudioSystem::Instance().PlaySoundAt(SoundEffect::EXPLODE_MEDIUM, world_x, world_y);
    } else {
        RATE_LIMITED_POSITION_EVENT(AudioEventType::EXPLOSION_SMALL, cx, cy);
        AudioSystem::Instance().PlaySoundAt(SoundEffect::EXPLODE_SMALL, world_x, world_y);
    }
}

void AudioEvent_WeaponFire(WeaponTypeClass* weapon, int world_x, int world_y) {
    CHECK_INITIALIZED();

    int cx = WorldToCell(world_x);
    int cy = WorldToCell(world_y);

    RATE_LIMITED_POSITION_EVENT(AudioEventType::WEAPON_FIRE, cx, cy);

    // In real implementation, get weapon sound from WeaponTypeClass
    // For now, play generic weapon sound
    AudioSystem::Instance().PlaySoundAt(SoundEffect::WEAPON_MACHINEGUN, world_x, world_y);
}

void AudioEvent_ProjectileImpact(BulletClass* bullet, int world_x, int world_y) {
    CHECK_INITIALIZED();

    int cx = WorldToCell(world_x);
    int cy = WorldToCell(world_y);

    RATE_LIMITED_POSITION_EVENT(AudioEventType::PROJECTILE_IMPACT, cx, cy);

    // In real implementation, get impact sound from bullet type
    AudioSystem::Instance().PlaySoundAt(SoundEffect::EXPLODE_SMALL, world_x, world_y);
}

//=============================================================================
// Unit Event Handlers
//=============================================================================

void AudioEvent_UnitSelected(ObjectClass* obj) {
    CHECK_INITIALIZED();

    if (!obj) return;

    ObjectKey id = GetObjectId(obj);
    RATE_LIMITED_OBJECT_EVENT(AudioEventType::UNIT_SELECT, id);

    // Play unit acknowledgment voice
    // In real implementation, determine unit type and play appropriate voice
    AudioSystem::Instance().PlayUnit(UnitVoice::REPORTING);
}

void AudioEvent_UnitMoveOrder(ObjectClass* obj) {
    CHECK_INITIALIZED();

    if (!obj) return;

    ObjectKey id = GetObjectId(obj);
    RATE_LIMITED_OBJECT_EVENT(AudioEventType::UNIT_MOVE_ORDER, id);

    // Play acknowledgment
    AudioSystem::Instance().PlayUnit(UnitVoice::MOVING);
}

void AudioEvent_UnitAttackOrder(ObjectClass* obj) {
    CHECK_INITIALIZED();

    if (!obj) return;

    ObjectKey id = GetObjectId(obj);
    RATE_LIMITED_OBJECT_EVENT(AudioEventType::UNIT_ATTACK_ORDER, id);

    // Play attack acknowledgment
    AudioSystem::Instance().PlayUnit(UnitVoice::ATTACKING);
}

void AudioEvent_UnitDeath(ObjectClass* obj) {
    CHECK_INITIALIZED();

    if (!obj) return;

    // Use position-based limiting for death sounds
    // In real implementation: int wx = Coord_X(obj->Coord);
    // For now, use 0,0 as placeholder
    int wx = 0;
    int wy = 0;
    int cx = WorldToCell(wx);
    int cy = WorldToCell(wy);

    RATE_LIMITED_POSITION_EVENT(AudioEventType::UNIT_DEATH, cx, cy);

    // Play death sound - in real implementation, based on unit type
    AudioSystem::Instance().PlaySoundAt(SoundEffect::UNIT_DIE_SCREAM, wx, wy);
}

void AudioEvent_UnitCreated(ObjectClass* obj, HouseClass* house) {
    CHECK_INITIALIZED();

    if (!IsPlayerHouse(house)) return;

    RATE_LIMITED_EVENT(AudioEventType::UNIT_CREATED);

    // EVA announcement handled separately by AudioEvent_UnitReady
}

//=============================================================================
// Building Event Handlers
//=============================================================================

void AudioEvent_BuildingPlaced(BuildingClass* building) {
    CHECK_INITIALIZED();

    RATE_LIMITED_EVENT(AudioEventType::BUILDING_PLACED);

    AudioSystem::Instance().PlaySound(SoundEffect::BUILD_PLACE);
}

void AudioEvent_BuildingComplete(BuildingClass* building, HouseClass* house) {
    CHECK_INITIALIZED();

    RATE_LIMITED_EVENT(AudioEventType::BUILDING_COMPLETE);

    AudioSystem::Instance().PlaySound(SoundEffect::BUILD_COMPLETE);

    // EVA announcement for player
    if (IsPlayerHouse(house)) {
        // Queue instead of play to prevent overlap
        AudioSystem::Instance().QueueEva(EvaVoice::CONSTRUCTION_COMPLETE);
    }
}

void AudioEvent_BuildingSold(BuildingClass* building) {
    CHECK_INITIALIZED();

    RATE_LIMITED_EVENT(AudioEventType::BUILDING_SOLD);

    AudioSystem::Instance().PlaySound(SoundEffect::BUILD_SELL);
}

void AudioEvent_BuildingDestroyed(BuildingClass* building, HouseClass* house) {
    CHECK_INITIALIZED();

    RATE_LIMITED_EVENT(AudioEventType::BUILDING_DESTROYED);

    // In real implementation, get building position
    AudioSystem::Instance().PlaySound(SoundEffect::EXPLODE_BUILDING);

    // EVA building lost
    if (IsPlayerHouse(house)) {
        AudioEvent_BuildingLost(house);
    }
}

void AudioEvent_BuildingCaptured(BuildingClass* building, HouseClass* old_owner, HouseClass* new_owner) {
    CHECK_INITIALIZED();

    RATE_LIMITED_EVENT(AudioEventType::BUILDING_CAPTURED);

    AudioSystem::Instance().PlaySound(SoundEffect::BUILD_CAPTURE);
}

//=============================================================================
// EVA Announcement Handlers
//=============================================================================

void AudioEvent_BaseUnderAttack(HouseClass* house) {
    CHECK_INITIALIZED();

    if (!IsPlayerHouse(house)) return;

    RATE_LIMITED_EVENT(AudioEventType::EVA_BASE_ATTACK);

    AudioSystem::Instance().PlayEva(EvaVoice::BASE_UNDER_ATTACK);
}

void AudioEvent_UnitLost(HouseClass* house) {
    CHECK_INITIALIZED();

    if (!IsPlayerHouse(house)) return;

    RATE_LIMITED_EVENT(AudioEventType::EVA_UNIT_LOST);

    AudioSystem::Instance().QueueEva(EvaVoice::UNIT_LOST);
}

void AudioEvent_BuildingLost(HouseClass* house) {
    CHECK_INITIALIZED();

    if (!IsPlayerHouse(house)) return;

    RATE_LIMITED_EVENT(AudioEventType::EVA_BUILDING_LOST);

    // Use same voice as unit lost or a building-specific one
    AudioSystem::Instance().QueueEva(EvaVoice::UNIT_LOST);
}

void AudioEvent_LowPower(HouseClass* house) {
    CHECK_INITIALIZED();

    if (!IsPlayerHouse(house)) return;

    RATE_LIMITED_EVENT(AudioEventType::EVA_LOW_POWER);

    AudioSystem::Instance().PlayEva(EvaVoice::LOW_POWER);
}

void AudioEvent_InsufficientFunds(HouseClass* house) {
    CHECK_INITIALIZED();

    if (!IsPlayerHouse(house)) return;

    RATE_LIMITED_EVENT(AudioEventType::EVA_INSUFFICIENT_FUNDS);

    AudioSystem::Instance().PlayEva(EvaVoice::INSUFFICIENT_FUNDS);
}

void AudioEvent_SilosNeeded(HouseClass* house) {
    CHECK_INITIALIZED();

    if (!IsPlayerHouse(house)) return;

    RATE_LIMITED_EVENT(AudioEventType::EVA_SILOS_NEEDED);

    AudioSystem::Instance().PlayEva(EvaVoice::SILOS_NEEDED);
}

void AudioEvent_RadarOnline(HouseClass* house) {
    CHECK_INITIALIZED();

    if (!IsPlayerHouse(house)) return;

    RATE_LIMITED_EVENT(AudioEventType::EVA_RADAR_ONLINE);

    AudioSystem::Instance().PlayEva(EvaVoice::RADAR_ONLINE);
}

void AudioEvent_RadarOffline(HouseClass* house) {
    CHECK_INITIALIZED();

    if (!IsPlayerHouse(house)) return;

    RATE_LIMITED_EVENT(AudioEventType::EVA_RADAR_OFFLINE);

    AudioSystem::Instance().PlayEva(EvaVoice::RADAR_OFFLINE);
}

void AudioEvent_UnitReady(HouseClass* house) {
    CHECK_INITIALIZED();

    if (!IsPlayerHouse(house)) return;

    RATE_LIMITED_EVENT(AudioEventType::EVA_UNIT_READY);

    AudioSystem::Instance().QueueEva(EvaVoice::UNIT_READY);
}

void AudioEvent_ConstructionComplete(HouseClass* house) {
    CHECK_INITIALIZED();

    if (!IsPlayerHouse(house)) return;

    RATE_LIMITED_EVENT(AudioEventType::EVA_CONSTRUCTION_COMPLETE);

    AudioSystem::Instance().PlayEva(EvaVoice::CONSTRUCTION_COMPLETE);
}

void AudioEvent_MissionAccomplished() {
    CHECK_INITIALIZED();

    // No rate limiting for mission events
    s_total_triggered++;
    AudioSystem::Instance().PlayEva(EvaVoice::MISSION_ACCOMPLISHED);
}

void AudioEvent_MissionFailed() {
    CHECK_INITIALIZED();

    s_total_triggered++;
    AudioSystem::Instance().PlayEva(EvaVoice::MISSION_FAILED);
}

//=============================================================================
// UI Event Handlers
//=============================================================================

void AudioEvent_UIClick() {
    CHECK_INITIALIZED();

    RATE_LIMITED_EVENT(AudioEventType::UI_CLICK);

    AudioSystem::Instance().PlaySound(SoundEffect::UI_CLICK);
}

void AudioEvent_UIBuildClick() {
    CHECK_INITIALIZED();

    RATE_LIMITED_EVENT(AudioEventType::UI_BUILD_CLICK);

    AudioSystem::Instance().PlaySound(SoundEffect::UI_BUILD_CLICK);
}

void AudioEvent_UITabClick() {
    CHECK_INITIALIZED();

    RATE_LIMITED_EVENT(AudioEventType::UI_TAB_CLICK);

    AudioSystem::Instance().PlaySound(SoundEffect::UI_SIDEBAR_TAB);
}

void AudioEvent_UISidebarUp() {
    CHECK_INITIALIZED();

    RATE_LIMITED_EVENT(AudioEventType::UI_SIDEBAR_UP);

    AudioSystem::Instance().PlaySound(SoundEffect::UI_SIDEBAR_SCROLL);
}

void AudioEvent_UISidebarDown() {
    CHECK_INITIALIZED();

    RATE_LIMITED_EVENT(AudioEventType::UI_SIDEBAR_DOWN);

    AudioSystem::Instance().PlaySound(SoundEffect::UI_SIDEBAR_SCROLL);
}

void AudioEvent_UICannot() {
    CHECK_INITIALIZED();

    RATE_LIMITED_EVENT(AudioEventType::UI_CANNOT);

    AudioSystem::Instance().PlaySound(SoundEffect::UI_CANNOT);
}

void AudioEvent_UIPrimarySet() {
    CHECK_INITIALIZED();

    RATE_LIMITED_EVENT(AudioEventType::UI_PRIMARY_SET);

    AudioSystem::Instance().PlaySound(SoundEffect::UI_PRIMARY_SET);
}

//=============================================================================
// Game State Audio
//=============================================================================

void AudioEvent_EnterMainMenu() {
    CHECK_INITIALIZED();

    s_total_triggered++;
    AudioSystem::Instance().PlayMusic(MusicTrack::MENU);
}

void AudioEvent_EnterBriefing(int mission_number, bool is_allied) {
    CHECK_INITIALIZED();

    s_total_triggered++;

    // Play briefing/score music
    AudioSystem::Instance().PlayMusic(MusicTrack::SCORE);
}

void AudioEvent_MissionStart() {
    CHECK_INITIALIZED();

    s_total_triggered++;

    // Play random in-game track
    // Could implement shuffle or mission-specific music
    AudioSystem::Instance().PlayMusic(MusicTrack::BIGFOOT);
    AudioSystem::Instance().SetMusicShuffle(true);
}

void AudioEvent_CombatStart() {
    CHECK_INITIALIZED();

    s_total_triggered++;

    // Could switch to more intense combat music
    // For now, music continues unchanged
}

void AudioEvent_CombatEnd() {
    CHECK_INITIALIZED();

    s_total_triggered++;

    // Could switch to calmer exploration music
}

void AudioEvent_Victory() {
    CHECK_INITIALIZED();

    s_total_triggered++;

    AudioSystem::Instance().StopMusic();
    AudioSystem::Instance().PlayEva(EvaVoice::MISSION_ACCOMPLISHED);

    // Could play victory theme
    AudioSystem::Instance().PlayMusic(MusicTrack::SCORE);
}

void AudioEvent_Defeat() {
    CHECK_INITIALIZED();

    s_total_triggered++;

    AudioSystem::Instance().StopMusic();
    AudioSystem::Instance().PlayEva(EvaVoice::MISSION_FAILED);
}

//=============================================================================
// Movement/Ambient Sounds
//=============================================================================

void AudioEvent_UnitMovement(ObjectClass* obj, bool is_moving) {
    CHECK_INITIALIZED();

    if (!obj) return;

    ObjectKey id = GetObjectId(obj);

    if (is_moving) {
        // Only rate limit starting movement sound
        if (!GetEventRateLimiter().CanFireForObject(static_cast<int>(AudioEventType::AMBIENT_MOVEMENT), id)) {
            return;
        }
        s_total_triggered++;

        // In real implementation, determine unit type for movement sound
        // For now, generic movement
        AudioSystem::Instance().PlaySound(SoundEffect::UNIT_MOVE_VEHICLE);
    } else {
        // Stop movement sound for this unit
        // Would need to track sound handle per unit
    }
}

//=============================================================================
// Debug/Stats
//=============================================================================

void AudioEvents_PrintStats() {
    Platform_Log("=== Audio Events Statistics ===");
    Platform_Log("  Initialized: %s", s_initialized ? "yes" : "no");
    Platform_Log("  Total Triggered: %u", s_total_triggered);
    Platform_Log("  Total Rate-Limited: %u", s_total_rate_limited);

    if (s_total_triggered > 0) {
        float rate = (float)s_total_rate_limited / s_total_triggered * 100.0f;
        Platform_Log("  Rate-Limit Ratio: %.1f%%", rate);
    }

    Platform_Log("  Tracked Entries: %zu", GetEventRateLimiter().GetTrackedCount());
}

uint32_t AudioEvents_GetTotalTriggered() {
    return s_total_triggered;
}

uint32_t AudioEvents_GetTotalRateLimited() {
    return s_total_rate_limited;
}
```

### src/test_audio_events.cpp

```cpp
// src/test_audio_events.cpp
// Tests for Game Audio Events
// Task 17f - Game Audio Events

#include "game/audio/audio_events.h"
#include "game/audio/event_rate_limiter.h"
#include "game/audio/audio_system.h"
#include "platform.h"
#include <cstdio>
#include <cstring>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { printf("  FAILED: %s\n", msg); return false; } } while(0)

#define RUN_TEST(name) \
    do { printf("Test: %s... ", #name); \
         if (Test_##name()) { printf("PASSED\n"); tests_passed++; } \
         else { tests_failed++; } } while(0)

//=============================================================================
// Rate Limiter Tests
//=============================================================================

bool Test_RateLimiter_Global() {
    EventRateLimiter limiter;

    limiter.SetGlobalCooldown(1, 100);  // 100ms cooldown for event type 1

    // First call should pass
    TEST_ASSERT(limiter.CanFireGlobal(1), "First event should fire");

    // Immediate second call should fail
    TEST_ASSERT(!limiter.CanFireGlobal(1), "Immediate repeat should be rate-limited");

    // After cooldown, should pass
    Platform_Timer_Delay(110);
    TEST_ASSERT(limiter.CanFireGlobal(1), "After cooldown should fire");

    return true;
}

bool Test_RateLimiter_Position() {
    EventRateLimiter limiter;

    limiter.SetPositionCooldown(2, 100);  // 100ms cooldown for event type 2

    // Same position: first should pass, second should fail
    TEST_ASSERT(limiter.CanFireAtPosition(2, 10, 10), "First at (10,10) should fire");
    TEST_ASSERT(!limiter.CanFireAtPosition(2, 10, 10), "Second at (10,10) should be limited");

    // Different position should pass
    TEST_ASSERT(limiter.CanFireAtPosition(2, 20, 20), "Different position should fire");

    return true;
}

bool Test_RateLimiter_Object() {
    EventRateLimiter limiter;

    limiter.SetObjectCooldown(3, 100);  // 100ms cooldown for event type 3

    // Same object: first should pass, second should fail
    TEST_ASSERT(limiter.CanFireForObject(3, 100), "First for object 100 should fire");
    TEST_ASSERT(!limiter.CanFireForObject(3, 100), "Second for object 100 should be limited");

    // Different object should pass
    TEST_ASSERT(limiter.CanFireForObject(3, 200), "Different object should fire");

    return true;
}

bool Test_RateLimiter_Combined() {
    EventRateLimiter limiter;

    limiter.SetGlobalCooldown(4, 50);
    limiter.SetPositionCooldown(4, 100);

    // First call should pass both
    TEST_ASSERT(limiter.CanFireGlobalAndPosition(4, 5, 5), "First should fire");

    // Immediate call at different position should fail (global limit)
    TEST_ASSERT(!limiter.CanFireGlobalAndPosition(4, 6, 6), "Global should block different pos");

    // After global cooldown but within position cooldown
    Platform_Timer_Delay(60);
    TEST_ASSERT(!limiter.CanFireGlobalAndPosition(4, 5, 5), "Position should still block");

    // Different position after global cooldown should pass
    TEST_ASSERT(limiter.CanFireGlobalAndPosition(4, 15, 15), "New position should fire");

    return true;
}

bool Test_RateLimiter_Cleanup() {
    EventRateLimiter limiter;

    limiter.SetPositionCooldown(5, 50);

    // Add some entries
    for (int i = 0; i < 100; i++) {
        limiter.CanFireAtPosition(5, i, i);
    }

    TEST_ASSERT(limiter.GetTrackedCount() >= 100, "Should have entries");

    // Cleanup shouldn't remove recent entries
    limiter.Cleanup();
    TEST_ASSERT(limiter.GetTrackedCount() >= 100, "Recent entries should remain");

    // Reset should clear all
    limiter.Reset();
    TEST_ASSERT(limiter.GetTrackedCount() == 0, "Reset should clear all");

    return true;
}

//=============================================================================
// Audio Events Tests
//=============================================================================

bool Test_AudioEvents_Init() {
    AudioEvents_Shutdown();  // Ensure clean state

    AudioEvents_Init();

    // Should be able to init twice without error
    AudioEvents_Init();

    TEST_ASSERT(AudioEvents_GetTotalTriggered() == 0, "Fresh init should have 0 triggered");

    return true;
}

bool Test_AudioEvents_UIClick() {
    AudioEvents_Init();

    uint32_t before = AudioEvents_GetTotalTriggered();

    AudioEvent_UIClick();

    TEST_ASSERT(AudioEvents_GetTotalTriggered() > before, "UI click should trigger");

    // Rapid clicks should be rate-limited
    uint32_t after_first = AudioEvents_GetTotalTriggered();
    uint32_t limited_before = AudioEvents_GetTotalRateLimited();

    AudioEvent_UIClick();
    AudioEvent_UIClick();
    AudioEvent_UIClick();

    uint32_t limited_after = AudioEvents_GetTotalRateLimited();
    TEST_ASSERT(limited_after > limited_before, "Rapid clicks should be rate-limited");

    return true;
}

bool Test_AudioEvents_UnitSelection() {
    AudioEvents_Init();

    // Create fake objects (just use addresses)
    int fake_unit1 = 1;
    int fake_unit2 = 2;

    uint32_t before = AudioEvents_GetTotalTriggered();

    AudioEvent_UnitSelected(reinterpret_cast<ObjectClass*>(&fake_unit1));

    TEST_ASSERT(AudioEvents_GetTotalTriggered() > before, "Unit select should trigger");

    // Same unit immediately should be rate-limited
    uint32_t limited_before = AudioEvents_GetTotalRateLimited();
    AudioEvent_UnitSelected(reinterpret_cast<ObjectClass*>(&fake_unit1));
    TEST_ASSERT(AudioEvents_GetTotalRateLimited() > limited_before, "Same unit should be limited");

    // Different unit should fire (after short delay for global cooldown)
    Platform_Timer_Delay(150);
    before = AudioEvents_GetTotalTriggered();
    AudioEvent_UnitSelected(reinterpret_cast<ObjectClass*>(&fake_unit2));
    TEST_ASSERT(AudioEvents_GetTotalTriggered() > before, "Different unit should fire");

    return true;
}

bool Test_AudioEvents_Explosions() {
    AudioEvents_Init();

    // Reset rate limiters
    GetEventRateLimiter().Reset();
    AudioEvents_Init();  // Re-init to reset rate limiter config

    uint32_t before = AudioEvents_GetTotalTriggered();

    // First explosion should fire
    AudioEvent_Explosion(100, 100, 2);
    TEST_ASSERT(AudioEvents_GetTotalTriggered() > before, "First explosion should fire");

    // Rapid explosions at same spot should be limited
    Platform_Timer_Delay(50);  // Small delay
    uint32_t limited_before = AudioEvents_GetTotalRateLimited();
    AudioEvent_Explosion(100, 100, 2);
    // May or may not be limited depending on exact timing

    // Different position should fire
    Platform_Timer_Delay(150);
    before = AudioEvents_GetTotalTriggered();
    AudioEvent_Explosion(500, 500, 1);
    TEST_ASSERT(AudioEvents_GetTotalTriggered() > before, "Different position should fire");

    return true;
}

bool Test_AudioEvents_EVA() {
    AudioEvents_Init();

    // Reset to ensure clean state
    GetEventRateLimiter().Reset();
    AudioEvents_Init();

    uint32_t before = AudioEvents_GetTotalTriggered();

    // First EVA should fire
    AudioEvent_InsufficientFunds(reinterpret_cast<HouseClass*>(1));
    TEST_ASSERT(AudioEvents_GetTotalTriggered() > before, "First EVA should fire");

    // Immediate second should be limited (5 second cooldown)
    uint32_t limited_before = AudioEvents_GetTotalRateLimited();
    AudioEvent_InsufficientFunds(reinterpret_cast<HouseClass*>(1));
    TEST_ASSERT(AudioEvents_GetTotalRateLimited() > limited_before, "EVA should be rate-limited");

    return true;
}

bool Test_AudioEvents_Stats() {
    AudioEvents_Init();

    // Trigger some events
    AudioEvent_UIClick();
    Platform_Timer_Delay(60);
    AudioEvent_UIClick();

    uint32_t triggered = AudioEvents_GetTotalTriggered();
    uint32_t limited = AudioEvents_GetTotalRateLimited();

    TEST_ASSERT(triggered > 0, "Should have triggered events");

    // Print stats (visual check)
    AudioEvents_PrintStats();

    return true;
}

//=============================================================================
// Interactive Test
//=============================================================================

void Interactive_Test() {
    printf("\n=== Interactive Audio Events Test ===\n");
    printf("Controls:\n");
    printf("  1: Unit select event\n");
    printf("  2: Move order event\n");
    printf("  3: Attack order event\n");
    printf("  4: Small explosion\n");
    printf("  5: Large explosion\n");
    printf("  6: EVA - Unit Ready\n");
    printf("  7: EVA - Base Under Attack\n");
    printf("  8: EVA - Insufficient Funds\n");
    printf("  9: UI Click\n");
    printf("  0: Print stats\n");
    printf("  M: Enter main menu (music)\n");
    printf("  S: Start mission (music)\n");
    printf("  V: Victory\n");
    printf("  D: Defeat\n");
    printf("  ESC: Exit\n\n");

    Platform_Init();
    Audio_Init();
    AudioEvents_Init();

    int fake_unit = 1;
    int fake_house = 1;

    bool running = true;
    while (running) {
        Platform_PumpEvents();
        Platform_Input_Update();

        if (Platform_Key_JustPressed(KEY_CODE_ESCAPE)) {
            running = false;
            continue;
        }

        // Unit events
        if (Platform_Key_JustPressed(KEY_CODE_1)) {
            printf("Event: Unit Selected\n");
            AudioEvent_UnitSelected(reinterpret_cast<ObjectClass*>(&fake_unit));
        }
        if (Platform_Key_JustPressed(KEY_CODE_2)) {
            printf("Event: Move Order\n");
            AudioEvent_UnitMoveOrder(reinterpret_cast<ObjectClass*>(&fake_unit));
        }
        if (Platform_Key_JustPressed(KEY_CODE_3)) {
            printf("Event: Attack Order\n");
            AudioEvent_UnitAttackOrder(reinterpret_cast<ObjectClass*>(&fake_unit));
        }

        // Explosions
        if (Platform_Key_JustPressed(KEY_CODE_4)) {
            printf("Event: Small Explosion at (320, 200)\n");
            AudioEvent_Explosion(320, 200, 0);
        }
        if (Platform_Key_JustPressed(KEY_CODE_5)) {
            printf("Event: Large Explosion at (320, 200)\n");
            AudioEvent_Explosion(320, 200, 2);
        }

        // EVA
        if (Platform_Key_JustPressed(KEY_CODE_6)) {
            printf("Event: EVA Unit Ready\n");
            AudioEvent_UnitReady(reinterpret_cast<HouseClass*>(&fake_house));
        }
        if (Platform_Key_JustPressed(KEY_CODE_7)) {
            printf("Event: EVA Base Under Attack\n");
            AudioEvent_BaseUnderAttack(reinterpret_cast<HouseClass*>(&fake_house));
        }
        if (Platform_Key_JustPressed(KEY_CODE_8)) {
            printf("Event: EVA Insufficient Funds\n");
            AudioEvent_InsufficientFunds(reinterpret_cast<HouseClass*>(&fake_house));
        }

        // UI
        if (Platform_Key_JustPressed(KEY_CODE_9)) {
            printf("Event: UI Click\n");
            AudioEvent_UIClick();
        }

        // Stats
        if (Platform_Key_JustPressed(KEY_CODE_0)) {
            AudioEvents_PrintStats();
        }

        // Game state
        if (Platform_Key_JustPressed(KEY_CODE_M)) {
            printf("Event: Enter Main Menu\n");
            AudioEvent_EnterMainMenu();
        }
        if (Platform_Key_JustPressed(KEY_CODE_S)) {
            printf("Event: Mission Start\n");
            AudioEvent_MissionStart();
        }
        if (Platform_Key_JustPressed(KEY_CODE_V)) {
            printf("Event: Victory!\n");
            AudioEvent_Victory();
        }
        if (Platform_Key_JustPressed(KEY_CODE_D)) {
            printf("Event: Defeat!\n");
            AudioEvent_Defeat();
        }

        Audio_Update();
        Platform_Timer_Delay(16);
    }

    AudioEvents_PrintStats();

    AudioEvents_Shutdown();
    Audio_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Main
//=============================================================================

int main(int argc, char* argv[]) {
    printf("=== Audio Events Tests (Task 17f) ===\n\n");

    Platform_Init();

    // Initialize audio system (needed for event handlers)
    Audio_Init();

    // Rate limiter tests
    printf("--- Rate Limiter Tests ---\n");
    RUN_TEST(RateLimiter_Global);
    RUN_TEST(RateLimiter_Position);
    RUN_TEST(RateLimiter_Object);
    RUN_TEST(RateLimiter_Combined);
    RUN_TEST(RateLimiter_Cleanup);

    // Audio event tests
    printf("\n--- Audio Event Tests ---\n");
    RUN_TEST(AudioEvents_Init);
    RUN_TEST(AudioEvents_UIClick);
    RUN_TEST(AudioEvents_UnitSelection);
    RUN_TEST(AudioEvents_Explosions);
    RUN_TEST(AudioEvents_EVA);
    RUN_TEST(AudioEvents_Stats);

    AudioEvents_Shutdown();
    Audio_Shutdown();
    Platform_Shutdown();

    printf("\n=== Results: %d passed, %d failed ===\n", tests_passed, tests_failed);

    if (argc > 1 && strcmp(argv[1], "-i") == 0) {
        Interactive_Test();
    } else {
        printf("\nRun with -i for interactive test\n");
    }

    return (tests_failed == 0) ? 0 : 1;
}
```

---

## CMakeLists.txt Additions

```cmake
# Task 17f: Game Audio Events
set(AUDIO_EVENTS_SOURCES
    ${CMAKE_SOURCE_DIR}/src/game/audio/audio_events.cpp
    ${CMAKE_SOURCE_DIR}/src/game/audio/event_rate_limiter.cpp
)

add_executable(TestAudioEvents
    ${CMAKE_SOURCE_DIR}/src/test_audio_events.cpp
    ${AUDIO_EVENTS_SOURCES}
    ${CMAKE_SOURCE_DIR}/src/game/audio/audio_system.cpp
    ${CMAKE_SOURCE_DIR}/src/game/audio/sound_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/game/audio/music_player.cpp
    ${CMAKE_SOURCE_DIR}/src/game/audio/voice_manager.cpp
    ${AUDIO_AUD_SOURCES}
)
target_include_directories(TestAudioEvents PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(TestAudioEvents PRIVATE redalert_platform)
add_test(NAME AudioEventsTests COMMAND TestAudioEvents)
```

---

## Verification Script

```bash
#!/bin/bash
set -e
echo "=== Task 17f Verification: Game Audio Events ==="

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"

echo "Checking files..."
for f in include/game/audio/audio_events.h \
         include/game/audio/event_rate_limiter.h \
         src/game/audio/audio_events.cpp \
         src/game/audio/event_rate_limiter.cpp; do
    [ -f "$ROOT_DIR/$f" ] || { echo "Missing: $f"; exit 1; }
    echo "  $f"
done

echo "Building..."
cd "$BUILD_DIR" && cmake --build . --target TestAudioEvents
echo "  Built"

echo "Running tests..."
"$BUILD_DIR/TestAudioEvents" || { echo "Tests failed"; exit 1; }

echo "=== Task 17f Verification PASSED ==="
```

---

## Implementation Steps

1. **Create event_rate_limiter.h/cpp**
   - Implement EventRateLimiter class with global/position/object rate limiting
   - Handle time tracking and cooldown checking
   - Implement cleanup for memory management

2. **Create audio_events.h**
   - Define AudioEventType enum for all event categories
   - Declare event handler functions for all game events
   - Include forward declarations for game types

3. **Create audio_events.cpp**
   - Implement initialization with rate limit configurations
   - Implement all event handlers with rate limit checks
   - Connect to AudioSystem for actual playback
   - Track statistics for debugging

4. **Create test_audio_events.cpp**
   - Unit tests for rate limiter
   - Integration tests for event handlers
   - Interactive test for manual verification

5. **Update CMakeLists.txt**
   - Add new source files
   - Create test executable
   - Register with CTest

---

## Success Criteria

- [ ] EventRateLimiter correctly limits global events
- [ ] EventRateLimiter correctly limits per-position events
- [ ] EventRateLimiter correctly limits per-object events
- [ ] Combined rate limiting (global + position/object) works
- [ ] All event handlers call appropriate audio functions
- [ ] Rate limiting configurations match original game feel
- [ ] Statistics tracking works correctly
- [ ] Cleanup prevents unbounded memory growth
- [ ] Interactive test demonstrates full functionality
- [ ] All automated tests pass

---

## Common Issues

1. **Rate limiter memory growth**: The position/object maps can grow unbounded. Call `Cleanup()` periodically (every few seconds) to remove stale entries.

2. **Timer wraparound**: The rate limiter handles 32-bit timer wraparound by allowing events when elapsed time appears negative (>0x80000000).

3. **Missing game types**: The ObjectClass, BuildingClass, etc. are forward-declared. In actual integration, include the real game headers.

4. **Sound effect enum mismatch**: Ensure SoundEffect enum values match Task 17b definitions.

5. **Voice priorities**: EVA announcements use queue for lower priority events to prevent interruption.

---

## Completion Promise

When verification passes, output:
<promise>TASK_17F_COMPLETE</promise>

## Escape Hatch

If stuck after 15 iterations, document in `ralph-tasks/blocked/17f.md` and output:
<promise>TASK_17F_BLOCKED</promise>

## Max Iterations
15
