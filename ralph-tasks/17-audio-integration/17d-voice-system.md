# Task 17d: Voice System

| Field | Value |
|-------|-------|
| **Task ID** | 17d |
| **Task Name** | Voice System |
| **Phase** | 17 - Audio Integration |
| **Depends On** | Task 17a (AUD File Parser) |
| **Estimated Complexity** | Medium (~400 lines) |

---

## Dependencies

- Task 17a (AUD File Parser) must be complete
- MixManager for locating voice files in `SPEECH.MIX`
- Platform audio API for playback

---

## Context

The Voice System handles two distinct types of voice audio in Red Alert:

1. **EVA Announcements**: The electronic voice assistant that announces game events like "Construction complete", "Unit ready", "Our base is under attack". These are high-priority, non-positional, and should not overlap.

2. **Unit Acknowledgments**: The voices units make when selected or given orders ("Yes sir!", "Reporting!", "Moving out!"). These can be positional and have less strict overlap rules.

Voice playback has special requirements:
- **Priority System**: Important messages (base under attack) override less important ones
- **Rate Limiting**: Same voice shouldn't repeat too quickly
- **Queue System**: EVA messages can queue if one is already playing
- **Faction Variants**: Some voices differ between Allies and Soviets

---

## Objective

Create a Voice System that:
1. Defines all voice events as enums (EVA and unit voices)
2. Loads voice files from SPEECH.MIX
3. Provides separate playback for EVA vs unit voices
4. Implements priority-based message queuing
5. Rate-limits repeated messages
6. Supports faction-specific voice variants
7. Has independent volume control

---

## Technical Background

### Voice Files in SPEECH.MIX

Red Alert voices are stored in `SPEECH.MIX` (and sometimes faction-specific MIX files).

**EVA Voices:**
```
BLDG1.AUD     - "Building"
CONSTRU1.AUD  - "Construction complete"
UNITREDY.AUD  - "Unit ready"
UNITLOST.AUD  - "Unit lost"
BASEATK1.AUD  - "Our base is under attack"
LOWPOWR1.AUD  - "Low power"
INSUFUND.AUD  - "Insufficient funds"
NEEDSILO.AUD  - "Silos needed"
RADARON1.AUD  - "Radar online"
RADAROFF.AUD  - "Radar offline"
ACCOM1.AUD    - "Mission accomplished"
FAIL1.AUD     - "Mission failed"
NEWOPT1.AUD   - "New construction options"
IRONRDY1.AUD  - "Iron curtain ready"
CHRORDY1.AUD  - "Chronosphere ready"
NUKESRDY.AUD  - "Nuclear missile ready"
NUKLNCH1.AUD  - "Nuclear missile launched"
NUKEATK1.AUD  - "Nuclear attack imminent"
```

**Unit Acknowledgment Voices:**
```
REPORTN1.AUD  - "Reporting"
ACKNO1.AUD    - "Acknowledged"
AFFIRM1.AUD   - "Affirmative"
YESSIR1.AUD   - "Yes sir"
AWARONE1.AUD  - "Awaiting orders"
MOVOUT1.AUD   - "Moving out"
ONWAY1.AUD    - "On my way"
UGOTIT1.AUD   - "You got it"
NODEST1.AUD   - "No problem"
ATACKNG1.AUD  - "Attacking"
FIREONE1.AUD  - "Firing"
```

### Voice Priority System

EVA messages have different priorities:
```
Priority 255 (Critical): Nuclear attack, Mission failed
Priority 200 (High):     Base under attack, Unit lost
Priority 150 (Medium):   Low power, Insufficient funds
Priority 100 (Normal):   Construction complete, Unit ready
Priority 50 (Low):       Building, Radar status
```

Higher priority messages can interrupt lower priority ones. Same priority queues.

### Rate Limiting

To prevent voice spam:
- Same EVA message: minimum 5 seconds between plays
- Same unit voice: minimum 1 second between plays
- "Base under attack": minimum 30 seconds between plays

---

## Deliverables

- [ ] `include/game/audio/voice_event.h` - Voice event enumerations
- [ ] `include/game/audio/voice_manager.h` - VoiceManager class declaration
- [ ] `src/game/audio/voice_manager.cpp` - Implementation
- [ ] `src/test_voice_manager.cpp` - Unit tests and interactive test
- [ ] CMakeLists.txt updated
- [ ] All tests pass

---

## Files to Create

### include/game/audio/voice_event.h

```cpp
// include/game/audio/voice_event.h
// Voice event enumerations
// Task 17d - Voice System

#ifndef VOICE_EVENT_H
#define VOICE_EVENT_H

#include <cstdint>

//=============================================================================
// EVA Voice Events
//=============================================================================
// High-priority announcer voices that play globally

enum class EvaVoice : int16_t {
    NONE = 0,

    //=========================================================================
    // Construction Events
    //=========================================================================
    BUILDING,               // "Building"
    CONSTRUCTION_COMPLETE,  // "Construction complete"
    ON_HOLD,               // "On hold"
    CANCELLED,             // "Cancelled"
    NEW_OPTIONS,           // "New construction options"

    //=========================================================================
    // Unit Events
    //=========================================================================
    TRAINING,              // "Training"
    UNIT_READY,            // "Unit ready"
    UNIT_LOST,             // "Unit lost"
    REINFORCEMENTS,        // "Reinforcements have arrived"

    //=========================================================================
    // Base Status
    //=========================================================================
    BASE_UNDER_ATTACK,     // "Our base is under attack"
    PRIMARY_BUILDING,      // "Primary building selected"
    BUILDING_CAPTURED,     // "Building captured"
    BUILDING_LOST,         // "Building lost"

    //=========================================================================
    // Resource Events
    //=========================================================================
    LOW_POWER,             // "Low power"
    POWER_RESTORED,        // "Power restored"
    INSUFFICIENT_FUNDS,    // "Insufficient funds"
    ORE_DEPLETED,          // "Ore depleted"
    SILOS_NEEDED,          // "Silos needed"

    //=========================================================================
    // Radar Events
    //=========================================================================
    RADAR_ONLINE,          // "Radar online"
    RADAR_OFFLINE,         // "Radar offline"

    //=========================================================================
    // Superweapon Events
    //=========================================================================
    IRON_CURTAIN_READY,    // "Iron curtain ready"
    IRON_CURTAIN_CHARGING, // "Iron curtain charging"
    CHRONOSPHERE_READY,    // "Chronosphere ready"
    CHRONOSPHERE_CHARGING, // "Chronosphere charging"
    NUKE_READY,            // "Nuclear missile ready"
    NUKE_LAUNCHED,         // "Nuclear missile launched"
    NUKE_ATTACK,           // "Nuclear attack imminent"
    GPS_READY,             // "GPS satellite ready"
    PARABOMBS_READY,       // "Parabombs ready"
    SPY_PLANE_READY,       // "Spy plane ready"

    //=========================================================================
    // Mission Events
    //=========================================================================
    MISSION_ACCOMPLISHED,  // "Mission accomplished"
    MISSION_FAILED,        // "Mission failed"
    BATTLE_CONTROL_ONLINE, // "Battle control online"
    BATTLE_CONTROL_TERMINATED,

    //=========================================================================
    // Multiplayer
    //=========================================================================
    PLAYER_DEFEATED,       // "Player defeated"
    ALLY_ATTACK,           // "Our ally is under attack"

    COUNT
};

//=============================================================================
// Unit Voice Events
//=============================================================================
// Unit acknowledgment and response voices

enum class UnitVoice : int16_t {
    NONE = 0,

    //=========================================================================
    // Selection Responses
    //=========================================================================
    REPORTING,             // "Reporting"
    YES_SIR,               // "Yes sir"
    READY,                 // "Ready and waiting"
    AWAITING_ORDERS,       // "Awaiting orders"
    AT_YOUR_SERVICE,       // "At your service"
    ACKNOWLEDGED,          // "Acknowledged"
    AFFIRMATIVE,           // "Affirmative"

    //=========================================================================
    // Movement Responses
    //=========================================================================
    MOVING_OUT,            // "Moving out"
    ON_MY_WAY,             // "On my way"
    DOUBLE_TIME,           // "Double time"
    YOU_GOT_IT,            // "You got it"
    NO_PROBLEM,            // "No problem"
    ROGER,                 // "Roger"

    //=========================================================================
    // Attack Responses
    //=========================================================================
    ATTACKING,             // "Attacking"
    FIRING,                // "Firing"
    LET_EM_HAVE_IT,        // "Let 'em have it"
    FOR_MOTHER_RUSSIA,     // "For Mother Russia" (Soviet)
    FOR_KING_COUNTRY,      // "For King and Country" (Allied)

    //=========================================================================
    // Special Responses
    //=========================================================================
    ENGINEER_READY,        // Engineer-specific
    MEDIC_READY,           // Medic-specific
    SPY_READY,             // Spy-specific
    TANYA_READY,           // "Cha-ching!" (Tanya)
    THIEF_READY,           // Thief-specific

    //=========================================================================
    // Vehicle-Specific
    //=========================================================================
    VEHICLE_MOVING,        // Generic vehicle acknowledgment
    TANK_READY,            // Tank-specific
    HELICOPTER_READY,      // Helicopter-specific
    BOAT_READY,            // Naval unit-specific

    COUNT
};

//=============================================================================
// Faction Type (for voice variants)
//=============================================================================

enum class VoiceFaction : uint8_t {
    NEUTRAL,    // Use default voices
    ALLIED,     // Western/Allied voices
    SOVIET      // Soviet/Russian voices
};

//=============================================================================
// Voice Info Structures
//=============================================================================

struct EvaVoiceInfo {
    const char* filename;       // Primary AUD filename
    const char* filename_alt;   // Alternative filename (if any)
    uint8_t priority;           // Higher = more important (0-255)
    uint16_t min_interval_ms;   // Minimum time between plays
    const char* description;    // Human-readable description
};

struct UnitVoiceInfo {
    const char* filename;       // Primary AUD filename
    const char* filename_soviet;// Soviet variant (if different)
    uint16_t min_interval_ms;   // Minimum time between plays
};

//=============================================================================
// Voice Info Accessors
//=============================================================================

/// Get EVA voice info
const EvaVoiceInfo& GetEvaVoiceInfo(EvaVoice voice);

/// Get unit voice info
const UnitVoiceInfo& GetUnitVoiceInfo(UnitVoice voice);

/// Get EVA voice filename
const char* GetEvaVoiceFilename(EvaVoice voice);

/// Get unit voice filename for faction
const char* GetUnitVoiceFilename(UnitVoice voice, VoiceFaction faction = VoiceFaction::NEUTRAL);

#endif // VOICE_EVENT_H
```

### include/game/audio/voice_manager.h

```cpp
// include/game/audio/voice_manager.h
// Voice playback manager
// Task 17d - Voice System

#ifndef VOICE_MANAGER_H
#define VOICE_MANAGER_H

#include "game/audio/voice_event.h"
#include "platform.h"
#include <vector>
#include <queue>
#include <unordered_map>
#include <cstdint>

//=============================================================================
// Voice Queue Entry
//=============================================================================

struct QueuedVoice {
    EvaVoice voice;
    uint8_t priority;
    uint64_t queue_time;
};

//=============================================================================
// Voice Manager Class
//=============================================================================

class VoiceManager {
public:
    /// Singleton instance
    static VoiceManager& Instance();

    //=========================================================================
    // Lifecycle
    //=========================================================================

    /// Initialize voice manager
    bool Initialize();

    /// Shutdown and release resources
    void Shutdown();

    /// Check if initialized
    bool IsInitialized() const { return initialized_; }

    /// Preload common voices for faster playback
    void PreloadCommonVoices();

    //=========================================================================
    // EVA Voice Playback
    //=========================================================================

    /// Play an EVA announcement
    /// @param voice The voice event to play
    /// @return true if voice started or queued
    bool PlayEva(EvaVoice voice);

    /// Queue an EVA voice (always queues, never interrupts)
    void QueueEva(EvaVoice voice);

    /// Stop current EVA voice and clear queue
    void StopEva();

    /// Check if EVA is currently speaking
    bool IsEvaSpeaking() const;

    /// Get current EVA voice
    EvaVoice GetCurrentEvaVoice() const { return current_eva_; }

    /// Clear EVA voice queue
    void ClearEvaQueue();

    //=========================================================================
    // Unit Voice Playback
    //=========================================================================

    /// Play a unit acknowledgment voice
    /// @param voice The voice event to play
    /// @param faction Faction for voice variant
    /// @return Play handle
    PlayHandle PlayUnit(UnitVoice voice, VoiceFaction faction = VoiceFaction::NEUTRAL);

    /// Play unit voice at world position
    PlayHandle PlayUnitAt(UnitVoice voice, int world_x, int world_y,
                          VoiceFaction faction = VoiceFaction::NEUTRAL);

    /// Stop current unit voice
    void StopUnit();

    /// Check if unit voice is playing
    bool IsUnitSpeaking() const;

    //=========================================================================
    // Volume Control
    //=========================================================================

    /// Set voice volume (0.0-1.0)
    void SetVolume(float volume);

    /// Get voice volume
    float GetVolume() const { return volume_; }

    /// Mute/unmute voices
    void SetMuted(bool muted);

    /// Check if muted
    bool IsMuted() const { return muted_; }

    //=========================================================================
    // Per-Frame Update
    //=========================================================================

    /// Update voice manager (process queue, check completion)
    void Update();

    //=========================================================================
    // Debug
    //=========================================================================

    /// Print current status
    void PrintStatus() const;

    /// Get queue size
    size_t GetQueueSize() const { return eva_queue_.size(); }

private:
    VoiceManager();
    ~VoiceManager();
    VoiceManager(const VoiceManager&) = delete;
    VoiceManager& operator=(const VoiceManager&) = delete;

    //=========================================================================
    // Internal Types
    //=========================================================================

    struct LoadedVoice {
        SoundHandle handle;
        bool loaded;
    };

    //=========================================================================
    // Internal Data
    //=========================================================================

    // EVA voice data
    std::unordered_map<int, LoadedVoice> eva_voices_;
    std::priority_queue<QueuedVoice,
                        std::vector<QueuedVoice>,
                        std::function<bool(const QueuedVoice&, const QueuedVoice&)>> eva_queue_;
    PlayHandle eva_play_handle_;
    EvaVoice current_eva_;
    uint64_t eva_last_play_time_;

    // Rate limiting for EVA voices
    std::unordered_map<int, uint64_t> eva_last_played_;

    // Unit voice data
    std::unordered_map<int, LoadedVoice> unit_voices_;
    PlayHandle unit_play_handle_;
    UnitVoice current_unit_;
    uint64_t unit_last_play_time_;

    // Rate limiting for unit voices
    std::unordered_map<int, uint64_t> unit_last_played_;

    // Settings
    float volume_;
    bool muted_;
    bool initialized_;
    uint64_t current_time_;

    //=========================================================================
    // Internal Methods
    //=========================================================================

    /// Load an EVA voice file
    bool LoadEvaVoice(EvaVoice voice);

    /// Load a unit voice file
    bool LoadUnitVoice(UnitVoice voice, VoiceFaction faction);

    /// Get sound handle for EVA voice (loading if needed)
    SoundHandle GetEvaSound(EvaVoice voice);

    /// Get sound handle for unit voice
    SoundHandle GetUnitSound(UnitVoice voice, VoiceFaction faction);

    /// Actually play an EVA voice
    bool DoPlayEva(EvaVoice voice);

    /// Process EVA queue
    void ProcessEvaQueue();

    /// Check if EVA voice can play (rate limiting)
    bool CanPlayEva(EvaVoice voice) const;

    /// Check if unit voice can play (rate limiting)
    bool CanPlayUnit(UnitVoice voice) const;

    /// Get unique key for voice+faction
    int GetUnitVoiceKey(UnitVoice voice, VoiceFaction faction) const;
};

//=============================================================================
// Global Convenience Functions
//=============================================================================

/// Initialize voice manager
bool Voice_Init();

/// Shutdown voice manager
void Voice_Shutdown();

/// Update voice manager (call each frame)
void Voice_Update();

/// Play EVA announcement
void Voice_PlayEva(EvaVoice voice);

/// Queue EVA announcement
void Voice_QueueEva(EvaVoice voice);

/// Play unit acknowledgment
PlayHandle Voice_PlayUnit(UnitVoice voice);

/// Set voice volume (0-255)
void Voice_SetVolume(int volume);

/// Get voice volume (0-255)
int Voice_GetVolume();

#endif // VOICE_MANAGER_H
```

### src/game/audio/voice_manager.cpp

```cpp
// src/game/audio/voice_manager.cpp
// Voice playback manager implementation
// Task 17d - Voice System

#include "game/audio/voice_manager.h"
#include "game/audio/aud_file.h"
#include "game/mix_manager.h"
#include "platform.h"
#include <algorithm>

//=============================================================================
// Voice Info Tables
//=============================================================================

static const EvaVoiceInfo EVA_VOICE_INFO[] = {
    // NONE
    { nullptr, nullptr, 0, 0, "None" },

    // Construction Events
    { "BLDG1.AUD",     nullptr, 50,  2000, "Building" },
    { "CONSTRU1.AUD",  nullptr, 100, 3000, "Construction complete" },
    { "ONHOLD1.AUD",   nullptr, 80,  3000, "On hold" },
    { "CANCLD1.AUD",   nullptr, 80,  2000, "Cancelled" },
    { "NEWOPT1.AUD",   nullptr, 100, 5000, "New construction options" },

    // Unit Events
    { "TRAIN1.AUD",    nullptr, 50,  2000, "Training" },
    { "UNITREDY.AUD",  nullptr, 100, 2000, "Unit ready" },
    { "UNITLOST.AUD",  nullptr, 200, 5000, "Unit lost" },
    { "REINFOR1.AUD",  nullptr, 150, 5000, "Reinforcements" },

    // Base Status
    { "BASEATK1.AUD",  nullptr, 250, 30000, "Base under attack" },
    { "PRIBLDG1.AUD",  nullptr, 80,  3000, "Primary building" },
    { "BLDGCAP1.AUD",  nullptr, 180, 5000, "Building captured" },
    { "BLDGLST1.AUD",  nullptr, 180, 5000, "Building lost" },

    // Resources
    { "LOWPOWR1.AUD",  nullptr, 180, 10000, "Low power" },
    { "POWRRES1.AUD",  nullptr, 150, 5000, "Power restored" },
    { "INSUFUND.AUD",  nullptr, 150, 5000, "Insufficient funds" },
    { "ABORRONE.AUD",  nullptr, 120, 10000, "Ore depleted" },
    { "NEEDSILO.AUD",  nullptr, 150, 10000, "Silos needed" },

    // Radar
    { "RADARON1.AUD",  nullptr, 100, 5000, "Radar online" },
    { "RADAROFF.AUD",  nullptr, 120, 5000, "Radar offline" },

    // Superweapons
    { "IRONRDY1.AUD",  nullptr, 200, 30000, "Iron curtain ready" },
    { "IRONCHG1.AUD",  nullptr, 100, 10000, "Iron curtain charging" },
    { "CHRORDY1.AUD",  nullptr, 200, 30000, "Chronosphere ready" },
    { "CHROCHG1.AUD",  nullptr, 100, 10000, "Chronosphere charging" },
    { "NUKESRDY.AUD",  nullptr, 220, 30000, "Nuke ready" },
    { "NUKLNCH1.AUD",  nullptr, 255, 5000, "Nuke launched" },
    { "NUKEATK1.AUD",  nullptr, 255, 5000, "Nuclear attack imminent" },
    { "GPSRDY1.AUD",   nullptr, 180, 30000, "GPS ready" },
    { "PARABRDY.AUD",  nullptr, 180, 30000, "Parabombs ready" },
    { "SPYRDY1.AUD",   nullptr, 150, 30000, "Spy plane ready" },

    // Mission
    { "ACCOM1.AUD",    nullptr, 255, 0, "Mission accomplished" },
    { "FAIL1.AUD",     nullptr, 255, 0, "Mission failed" },
    { "BATCON1.AUD",   nullptr, 200, 0, "Battle control online" },
    { "BATCONT1.AUD",  nullptr, 200, 0, "Battle control terminated" },

    // Multiplayer
    { "PLYDEFT1.AUD",  nullptr, 200, 5000, "Player defeated" },
    { "ALLATK1.AUD",   nullptr, 220, 30000, "Ally under attack" },
};

static_assert(sizeof(EVA_VOICE_INFO) / sizeof(EVA_VOICE_INFO[0]) ==
              static_cast<size_t>(EvaVoice::COUNT),
              "EVA_VOICE_INFO size mismatch");

static const UnitVoiceInfo UNIT_VOICE_INFO[] = {
    // NONE
    { nullptr, nullptr, 0 },

    // Selection
    { "REPORTN1.AUD", nullptr, 800 },     // REPORTING
    { "YESSIR1.AUD",  nullptr, 800 },     // YES_SIR
    { "READY1.AUD",   nullptr, 800 },     // READY
    { "AWARONE1.AUD", nullptr, 800 },     // AWAITING_ORDERS
    { "ATSERV1.AUD",  nullptr, 800 },     // AT_YOUR_SERVICE
    { "ACKNO1.AUD",   nullptr, 800 },     // ACKNOWLEDGED
    { "AFFIRM1.AUD",  nullptr, 800 },     // AFFIRMATIVE

    // Movement
    { "MOVOUT1.AUD",  nullptr, 600 },     // MOVING_OUT
    { "ONWAY1.AUD",   nullptr, 600 },     // ON_MY_WAY
    { "DOUBLE1.AUD",  nullptr, 600 },     // DOUBLE_TIME
    { "UGOTIT1.AUD",  nullptr, 600 },     // YOU_GOT_IT
    { "NODEST1.AUD",  nullptr, 600 },     // NO_PROBLEM
    { "ROGER1.AUD",   nullptr, 600 },     // ROGER

    // Attack
    { "ATACKNG1.AUD", nullptr, 500 },     // ATTACKING
    { "FIREONE1.AUD", nullptr, 500 },     // FIRING
    { "LETEM1.AUD",   nullptr, 500 },     // LET_EM_HAVE_IT
    { "FORMR1.AUD",   "FORMR1.AUD", 500 },  // FOR_MOTHER_RUSSIA (Soviet)
    { "FORKAC1.AUD",  nullptr, 500 },     // FOR_KING_COUNTRY (Allied)

    // Special units
    { "ENGR1.AUD",    nullptr, 800 },     // ENGINEER
    { "MEDIC1.AUD",   nullptr, 800 },     // MEDIC
    { "SPY1.AUD",     nullptr, 800 },     // SPY
    { "TANYA1.AUD",   nullptr, 800 },     // TANYA
    { "THIEF1.AUD",   nullptr, 800 },     // THIEF

    // Vehicles
    { "VEHIC1.AUD",   nullptr, 600 },     // VEHICLE
    { "TANK1.AUD",    nullptr, 600 },     // TANK
    { "COPTER1.AUD",  nullptr, 600 },     // HELICOPTER
    { "BOAT1.AUD",    nullptr, 600 },     // BOAT
};

static_assert(sizeof(UNIT_VOICE_INFO) / sizeof(UNIT_VOICE_INFO[0]) ==
              static_cast<size_t>(UnitVoice::COUNT),
              "UNIT_VOICE_INFO size mismatch");

//=============================================================================
// Voice Info Accessors
//=============================================================================

const EvaVoiceInfo& GetEvaVoiceInfo(EvaVoice voice) {
    int idx = static_cast<int>(voice);
    if (idx < 0 || idx >= static_cast<int>(EvaVoice::COUNT)) {
        return EVA_VOICE_INFO[0];
    }
    return EVA_VOICE_INFO[idx];
}

const UnitVoiceInfo& GetUnitVoiceInfo(UnitVoice voice) {
    int idx = static_cast<int>(voice);
    if (idx < 0 || idx >= static_cast<int>(UnitVoice::COUNT)) {
        return UNIT_VOICE_INFO[0];
    }
    return UNIT_VOICE_INFO[idx];
}

const char* GetEvaVoiceFilename(EvaVoice voice) {
    return GetEvaVoiceInfo(voice).filename;
}

const char* GetUnitVoiceFilename(UnitVoice voice, VoiceFaction faction) {
    const UnitVoiceInfo& info = GetUnitVoiceInfo(voice);
    if (faction == VoiceFaction::SOVIET && info.filename_soviet) {
        return info.filename_soviet;
    }
    return info.filename;
}

//=============================================================================
// VoiceManager Implementation
//=============================================================================

// Priority queue comparator (higher priority first)
static bool QueueComparator(const QueuedVoice& a, const QueuedVoice& b) {
    if (a.priority != b.priority) {
        return a.priority < b.priority;  // Lower priority comes first (min-heap)
    }
    return a.queue_time > b.queue_time;  // Earlier time has priority
}

VoiceManager& VoiceManager::Instance() {
    static VoiceManager instance;
    return instance;
}

VoiceManager::VoiceManager()
    : eva_queue_(QueueComparator)
    , eva_play_handle_(INVALID_PLAY_HANDLE)
    , current_eva_(EvaVoice::NONE)
    , eva_last_play_time_(0)
    , unit_play_handle_(INVALID_PLAY_HANDLE)
    , current_unit_(UnitVoice::NONE)
    , unit_last_play_time_(0)
    , volume_(1.0f)
    , muted_(false)
    , initialized_(false)
    , current_time_(0) {
}

VoiceManager::~VoiceManager() {
    Shutdown();
}

bool VoiceManager::Initialize() {
    if (initialized_) {
        return true;
    }

    Platform_Log("VoiceManager: Initializing...");

    // Clear any existing data
    eva_voices_.clear();
    unit_voices_.clear();
    eva_last_played_.clear();
    unit_last_played_.clear();

    initialized_ = true;
    Platform_Log("VoiceManager: Initialized");

    return true;
}

void VoiceManager::Shutdown() {
    if (!initialized_) {
        return;
    }

    Platform_Log("VoiceManager: Shutting down...");

    StopEva();
    StopUnit();

    // Destroy all loaded voices
    for (auto& pair : eva_voices_) {
        if (pair.second.loaded && pair.second.handle != INVALID_SOUND_HANDLE) {
            Platform_Sound_Destroy(pair.second.handle);
        }
    }
    eva_voices_.clear();

    for (auto& pair : unit_voices_) {
        if (pair.second.loaded && pair.second.handle != INVALID_SOUND_HANDLE) {
            Platform_Sound_Destroy(pair.second.handle);
        }
    }
    unit_voices_.clear();

    // Clear queue
    while (!eva_queue_.empty()) {
        eva_queue_.pop();
    }

    initialized_ = false;
}

void VoiceManager::PreloadCommonVoices() {
    Platform_Log("VoiceManager: Preloading common voices...");

    // Preload frequently used EVA voices
    static const EvaVoice common_eva[] = {
        EvaVoice::CONSTRUCTION_COMPLETE,
        EvaVoice::UNIT_READY,
        EvaVoice::BASE_UNDER_ATTACK,
        EvaVoice::LOW_POWER,
        EvaVoice::INSUFFICIENT_FUNDS,
    };

    for (EvaVoice v : common_eva) {
        LoadEvaVoice(v);
    }

    // Preload common unit voices
    static const UnitVoice common_unit[] = {
        UnitVoice::REPORTING,
        UnitVoice::ACKNOWLEDGED,
        UnitVoice::AFFIRMATIVE,
        UnitVoice::MOVING_OUT,
        UnitVoice::ATTACKING,
    };

    for (UnitVoice v : common_unit) {
        LoadUnitVoice(v, VoiceFaction::NEUTRAL);
    }
}

//=============================================================================
// Voice Loading
//=============================================================================

bool VoiceManager::LoadEvaVoice(EvaVoice voice) {
    int key = static_cast<int>(voice);

    // Already loaded?
    auto it = eva_voices_.find(key);
    if (it != eva_voices_.end() && it->second.loaded) {
        return true;
    }

    const char* filename = GetEvaVoiceFilename(voice);
    if (!filename) {
        return false;
    }

    AudFile aud;
    if (!aud.LoadFromMix(filename)) {
        Platform_Log("VoiceManager: Failed to load EVA voice: %s", filename);
        eva_voices_[key] = {INVALID_SOUND_HANDLE, false};
        return false;
    }

    SoundHandle handle = Platform_Sound_CreateFromMemory(
        aud.GetPCMDataBytes(),
        static_cast<int32_t>(aud.GetPCMDataSize()),
        aud.GetSampleRate(),
        aud.GetChannels(),
        16
    );

    if (handle == INVALID_SOUND_HANDLE) {
        eva_voices_[key] = {INVALID_SOUND_HANDLE, false};
        return false;
    }

    eva_voices_[key] = {handle, true};
    return true;
}

bool VoiceManager::LoadUnitVoice(UnitVoice voice, VoiceFaction faction) {
    int key = GetUnitVoiceKey(voice, faction);

    auto it = unit_voices_.find(key);
    if (it != unit_voices_.end() && it->second.loaded) {
        return true;
    }

    const char* filename = GetUnitVoiceFilename(voice, faction);
    if (!filename) {
        return false;
    }

    AudFile aud;
    if (!aud.LoadFromMix(filename)) {
        unit_voices_[key] = {INVALID_SOUND_HANDLE, false};
        return false;
    }

    SoundHandle handle = Platform_Sound_CreateFromMemory(
        aud.GetPCMDataBytes(),
        static_cast<int32_t>(aud.GetPCMDataSize()),
        aud.GetSampleRate(),
        aud.GetChannels(),
        16
    );

    if (handle == INVALID_SOUND_HANDLE) {
        unit_voices_[key] = {INVALID_SOUND_HANDLE, false};
        return false;
    }

    unit_voices_[key] = {handle, true};
    return true;
}

SoundHandle VoiceManager::GetEvaSound(EvaVoice voice) {
    int key = static_cast<int>(voice);

    auto it = eva_voices_.find(key);
    if (it != eva_voices_.end() && it->second.loaded) {
        return it->second.handle;
    }

    // Try to load
    if (LoadEvaVoice(voice)) {
        return eva_voices_[key].handle;
    }

    return INVALID_SOUND_HANDLE;
}

SoundHandle VoiceManager::GetUnitSound(UnitVoice voice, VoiceFaction faction) {
    int key = GetUnitVoiceKey(voice, faction);

    auto it = unit_voices_.find(key);
    if (it != unit_voices_.end() && it->second.loaded) {
        return it->second.handle;
    }

    if (LoadUnitVoice(voice, faction)) {
        return unit_voices_[key].handle;
    }

    return INVALID_SOUND_HANDLE;
}

int VoiceManager::GetUnitVoiceKey(UnitVoice voice, VoiceFaction faction) const {
    return static_cast<int>(voice) * 10 + static_cast<int>(faction);
}

//=============================================================================
// EVA Voice Playback
//=============================================================================

bool VoiceManager::PlayEva(EvaVoice voice) {
    if (!initialized_ || muted_ || voice == EvaVoice::NONE) {
        return false;
    }

    // Rate limiting check
    if (!CanPlayEva(voice)) {
        return false;
    }

    const EvaVoiceInfo& info = GetEvaVoiceInfo(voice);

    // If nothing playing, play immediately
    if (!IsEvaSpeaking()) {
        return DoPlayEva(voice);
    }

    // Check if new voice has higher priority
    const EvaVoiceInfo& current_info = GetEvaVoiceInfo(current_eva_);
    if (info.priority > current_info.priority) {
        // Interrupt current voice
        StopEva();
        return DoPlayEva(voice);
    }

    // Queue the voice
    QueueEva(voice);
    return true;
}

void VoiceManager::QueueEva(EvaVoice voice) {
    if (voice == EvaVoice::NONE) return;

    const EvaVoiceInfo& info = GetEvaVoiceInfo(voice);

    QueuedVoice qv;
    qv.voice = voice;
    qv.priority = info.priority;
    qv.queue_time = current_time_;

    eva_queue_.push(qv);
}

bool VoiceManager::DoPlayEva(EvaVoice voice) {
    SoundHandle sound = GetEvaSound(voice);
    if (sound == INVALID_SOUND_HANDLE) {
        return false;
    }

    float vol = muted_ ? 0.0f : volume_;
    eva_play_handle_ = Platform_Sound_Play(sound, vol, 0.0f, false);

    if (eva_play_handle_ != INVALID_PLAY_HANDLE) {
        current_eva_ = voice;
        eva_last_play_time_ = current_time_;
        eva_last_played_[static_cast<int>(voice)] = current_time_;

        const EvaVoiceInfo& info = GetEvaVoiceInfo(voice);
        Platform_Log("VoiceManager: EVA: %s", info.description);
        return true;
    }

    return false;
}

void VoiceManager::StopEva() {
    if (eva_play_handle_ != INVALID_PLAY_HANDLE) {
        Platform_Sound_Stop(eva_play_handle_);
        eva_play_handle_ = INVALID_PLAY_HANDLE;
    }
    current_eva_ = EvaVoice::NONE;
}

bool VoiceManager::IsEvaSpeaking() const {
    if (eva_play_handle_ == INVALID_PLAY_HANDLE) {
        return false;
    }
    return Platform_Sound_IsPlaying(eva_play_handle_);
}

void VoiceManager::ClearEvaQueue() {
    while (!eva_queue_.empty()) {
        eva_queue_.pop();
    }
}

bool VoiceManager::CanPlayEva(EvaVoice voice) const {
    int key = static_cast<int>(voice);
    auto it = eva_last_played_.find(key);
    if (it == eva_last_played_.end()) {
        return true;
    }

    const EvaVoiceInfo& info = GetEvaVoiceInfo(voice);
    return (current_time_ - it->second) >= info.min_interval_ms;
}

void VoiceManager::ProcessEvaQueue() {
    if (IsEvaSpeaking() || eva_queue_.empty()) {
        return;
    }

    // Get highest priority voice
    QueuedVoice qv = eva_queue_.top();
    eva_queue_.pop();

    // Check if still rate-limited
    if (CanPlayEva(qv.voice)) {
        DoPlayEva(qv.voice);
    } else if (!eva_queue_.empty()) {
        // Try next in queue
        ProcessEvaQueue();
    }
}

//=============================================================================
// Unit Voice Playback
//=============================================================================

PlayHandle VoiceManager::PlayUnit(UnitVoice voice, VoiceFaction faction) {
    if (!initialized_ || muted_ || voice == UnitVoice::NONE) {
        return INVALID_PLAY_HANDLE;
    }

    if (!CanPlayUnit(voice)) {
        return INVALID_PLAY_HANDLE;
    }

    SoundHandle sound = GetUnitSound(voice, faction);
    if (sound == INVALID_SOUND_HANDLE) {
        return INVALID_PLAY_HANDLE;
    }

    // Stop current unit voice
    StopUnit();

    float vol = muted_ ? 0.0f : volume_;
    unit_play_handle_ = Platform_Sound_Play(sound, vol, 0.0f, false);

    if (unit_play_handle_ != INVALID_PLAY_HANDLE) {
        current_unit_ = voice;
        unit_last_play_time_ = current_time_;
        unit_last_played_[static_cast<int>(voice)] = current_time_;
    }

    return unit_play_handle_;
}

PlayHandle VoiceManager::PlayUnitAt(UnitVoice voice, int world_x, int world_y,
                                     VoiceFaction faction) {
    // For now, unit voices are not positional (could add later)
    return PlayUnit(voice, faction);
}

void VoiceManager::StopUnit() {
    if (unit_play_handle_ != INVALID_PLAY_HANDLE) {
        Platform_Sound_Stop(unit_play_handle_);
        unit_play_handle_ = INVALID_PLAY_HANDLE;
    }
    current_unit_ = UnitVoice::NONE;
}

bool VoiceManager::IsUnitSpeaking() const {
    if (unit_play_handle_ == INVALID_PLAY_HANDLE) {
        return false;
    }
    return Platform_Sound_IsPlaying(unit_play_handle_);
}

bool VoiceManager::CanPlayUnit(UnitVoice voice) const {
    int key = static_cast<int>(voice);
    auto it = unit_last_played_.find(key);
    if (it == unit_last_played_.end()) {
        return true;
    }

    const UnitVoiceInfo& info = GetUnitVoiceInfo(voice);
    return (current_time_ - it->second) >= info.min_interval_ms;
}

//=============================================================================
// Volume Control
//=============================================================================

void VoiceManager::SetVolume(float volume) {
    volume_ = std::clamp(volume, 0.0f, 1.0f);

    // Update playing voices
    if (eva_play_handle_ != INVALID_PLAY_HANDLE) {
        Platform_Sound_SetVolume(eva_play_handle_, muted_ ? 0.0f : volume_);
    }
    if (unit_play_handle_ != INVALID_PLAY_HANDLE) {
        Platform_Sound_SetVolume(unit_play_handle_, muted_ ? 0.0f : volume_);
    }
}

void VoiceManager::SetMuted(bool muted) {
    muted_ = muted;

    float vol = muted ? 0.0f : volume_;
    if (eva_play_handle_ != INVALID_PLAY_HANDLE) {
        Platform_Sound_SetVolume(eva_play_handle_, vol);
    }
    if (unit_play_handle_ != INVALID_PLAY_HANDLE) {
        Platform_Sound_SetVolume(unit_play_handle_, vol);
    }
}

//=============================================================================
// Update
//=============================================================================

void VoiceManager::Update() {
    if (!initialized_) {
        return;
    }

    current_time_ = Platform_GetTicks();

    // Check if EVA voice finished
    if (eva_play_handle_ != INVALID_PLAY_HANDLE &&
        !Platform_Sound_IsPlaying(eva_play_handle_)) {
        eva_play_handle_ = INVALID_PLAY_HANDLE;
        current_eva_ = EvaVoice::NONE;
    }

    // Check if unit voice finished
    if (unit_play_handle_ != INVALID_PLAY_HANDLE &&
        !Platform_Sound_IsPlaying(unit_play_handle_)) {
        unit_play_handle_ = INVALID_PLAY_HANDLE;
        current_unit_ = UnitVoice::NONE;
    }

    // Process EVA queue
    ProcessEvaQueue();
}

//=============================================================================
// Debug
//=============================================================================

void VoiceManager::PrintStatus() const {
    Platform_Log("VoiceManager Status:");
    Platform_Log("  EVA Speaking: %s", IsEvaSpeaking() ? "yes" : "no");
    if (current_eva_ != EvaVoice::NONE) {
        Platform_Log("  Current EVA: %s", GetEvaVoiceInfo(current_eva_).description);
    }
    Platform_Log("  EVA Queue: %zu", eva_queue_.size());
    Platform_Log("  Unit Speaking: %s", IsUnitSpeaking() ? "yes" : "no");
    Platform_Log("  Volume: %.0f%%", volume_ * 100);
    Platform_Log("  Muted: %s", muted_ ? "yes" : "no");
}

//=============================================================================
// Global Functions
//=============================================================================

bool Voice_Init() {
    return VoiceManager::Instance().Initialize();
}

void Voice_Shutdown() {
    VoiceManager::Instance().Shutdown();
}

void Voice_Update() {
    VoiceManager::Instance().Update();
}

void Voice_PlayEva(EvaVoice voice) {
    VoiceManager::Instance().PlayEva(voice);
}

void Voice_QueueEva(EvaVoice voice) {
    VoiceManager::Instance().QueueEva(voice);
}

PlayHandle Voice_PlayUnit(UnitVoice voice) {
    return VoiceManager::Instance().PlayUnit(voice);
}

void Voice_SetVolume(int volume) {
    float vol = std::clamp(volume, 0, 255) / 255.0f;
    VoiceManager::Instance().SetVolume(vol);
}

int Voice_GetVolume() {
    return static_cast<int>(VoiceManager::Instance().GetVolume() * 255);
}
```

### src/test_voice_manager.cpp

```cpp
// src/test_voice_manager.cpp
// Unit tests for Voice Manager
// Task 17d - Voice System

#include "game/audio/voice_manager.h"
#include "game/mix_manager.h"
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
// Unit Tests
//=============================================================================

bool Test_EvaVoiceEnum() {
    TEST_ASSERT(static_cast<int>(EvaVoice::NONE) == 0, "NONE should be 0");
    TEST_ASSERT(static_cast<int>(EvaVoice::COUNT) > 20, "Should have 20+ EVA voices");
    TEST_ASSERT(static_cast<int>(EvaVoice::CONSTRUCTION_COMPLETE) > 0, "CONSTRUCTION_COMPLETE exists");
    TEST_ASSERT(static_cast<int>(EvaVoice::BASE_UNDER_ATTACK) > 0, "BASE_UNDER_ATTACK exists");
    return true;
}

bool Test_UnitVoiceEnum() {
    TEST_ASSERT(static_cast<int>(UnitVoice::NONE) == 0, "NONE should be 0");
    TEST_ASSERT(static_cast<int>(UnitVoice::COUNT) > 15, "Should have 15+ unit voices");
    TEST_ASSERT(static_cast<int>(UnitVoice::REPORTING) > 0, "REPORTING exists");
    TEST_ASSERT(static_cast<int>(UnitVoice::ACKNOWLEDGED) > 0, "ACKNOWLEDGED exists");
    return true;
}

bool Test_EvaVoiceInfo() {
    const EvaVoiceInfo& info = GetEvaVoiceInfo(EvaVoice::CONSTRUCTION_COMPLETE);
    TEST_ASSERT(info.filename != nullptr, "Should have filename");
    TEST_ASSERT(info.priority > 0, "Should have priority");
    TEST_ASSERT(info.min_interval_ms > 0, "Should have interval");

    const EvaVoiceInfo& attack_info = GetEvaVoiceInfo(EvaVoice::BASE_UNDER_ATTACK);
    TEST_ASSERT(attack_info.priority > info.priority, "BASE_UNDER_ATTACK should be higher priority");

    return true;
}

bool Test_UnitVoiceInfo() {
    const UnitVoiceInfo& info = GetUnitVoiceInfo(UnitVoice::REPORTING);
    TEST_ASSERT(info.filename != nullptr, "Should have filename");
    TEST_ASSERT(info.min_interval_ms > 0, "Should have interval");
    return true;
}

bool Test_VolumeControl() {
    VoiceManager& mgr = VoiceManager::Instance();
    if (!mgr.IsInitialized()) mgr.Initialize();

    mgr.SetVolume(0.5f);
    TEST_ASSERT(mgr.GetVolume() >= 0.49f && mgr.GetVolume() <= 0.51f, "Volume should be 0.5");

    mgr.SetVolume(1.0f);
    return true;
}

bool Test_MuteControl() {
    VoiceManager& mgr = VoiceManager::Instance();
    if (!mgr.IsInitialized()) mgr.Initialize();

    TEST_ASSERT(!mgr.IsMuted(), "Should not be muted initially");
    mgr.SetMuted(true);
    TEST_ASSERT(mgr.IsMuted(), "Should be muted");
    mgr.SetMuted(false);
    TEST_ASSERT(!mgr.IsMuted(), "Should not be muted");
    return true;
}

bool Test_GlobalFunctions() {
    Voice_SetVolume(128);
    int vol = Voice_GetVolume();
    TEST_ASSERT(vol >= 120 && vol <= 136, "Volume should be ~128");
    Voice_SetVolume(255);
    return true;
}

//=============================================================================
// Interactive Test
//=============================================================================

void Interactive_Test() {
    printf("\n=== Interactive Voice Manager Test ===\n");
    printf("Controls:\n");
    printf("  1: Construction Complete\n");
    printf("  2: Unit Ready\n");
    printf("  3: Base Under Attack\n");
    printf("  4: Low Power\n");
    printf("  5: Unit Reporting\n");
    printf("  6: Unit Acknowledged\n");
    printf("  7: Queue multiple EVA voices\n");
    printf("  ESC: Exit\n\n");

    Platform_Init();
    AudioConfig cfg = {22050, 2, 16, 1024};
    Platform_Audio_Init(&cfg);

    MixManager::Instance().Initialize();
    MixManager::Instance().LoadMix("SPEECH.MIX");

    Voice_Init();
    VoiceManager& mgr = VoiceManager::Instance();
    mgr.PreloadCommonVoices();

    bool running = true;
    while (running) {
        Platform_PumpEvents();
        Platform_Input_Update();

        if (Platform_Key_JustPressed(KEY_CODE_ESCAPE)) {
            running = false;
            continue;
        }

        if (Platform_Key_JustPressed(KEY_CODE_1)) {
            printf("Playing: Construction Complete\n");
            Voice_PlayEva(EvaVoice::CONSTRUCTION_COMPLETE);
        }
        if (Platform_Key_JustPressed(KEY_CODE_2)) {
            printf("Playing: Unit Ready\n");
            Voice_PlayEva(EvaVoice::UNIT_READY);
        }
        if (Platform_Key_JustPressed(KEY_CODE_3)) {
            printf("Playing: Base Under Attack\n");
            Voice_PlayEva(EvaVoice::BASE_UNDER_ATTACK);
        }
        if (Platform_Key_JustPressed(KEY_CODE_4)) {
            printf("Playing: Low Power\n");
            Voice_PlayEva(EvaVoice::LOW_POWER);
        }
        if (Platform_Key_JustPressed(KEY_CODE_5)) {
            printf("Playing: Unit Reporting\n");
            Voice_PlayUnit(UnitVoice::REPORTING);
        }
        if (Platform_Key_JustPressed(KEY_CODE_6)) {
            printf("Playing: Unit Acknowledged\n");
            Voice_PlayUnit(UnitVoice::ACKNOWLEDGED);
        }
        if (Platform_Key_JustPressed(KEY_CODE_7)) {
            printf("Queueing multiple EVA voices...\n");
            Voice_QueueEva(EvaVoice::BUILDING);
            Voice_QueueEva(EvaVoice::CONSTRUCTION_COMPLETE);
            Voice_QueueEva(EvaVoice::UNIT_READY);
        }

        Voice_Update();
        Platform_Timer_Delay(16);
    }

    Voice_Shutdown();
    MixManager::Instance().Shutdown();
    Platform_Audio_Shutdown();
    Platform_Shutdown();
}

int main(int argc, char* argv[]) {
    printf("=== Voice Manager Tests (Task 17d) ===\n\n");

    Platform_Init();
    AudioConfig cfg = {22050, 2, 16, 1024};
    Platform_Audio_Init(&cfg);
    MixManager::Instance().Initialize();

    RUN_TEST(EvaVoiceEnum);
    RUN_TEST(UnitVoiceEnum);
    RUN_TEST(EvaVoiceInfo);
    RUN_TEST(UnitVoiceInfo);
    RUN_TEST(VolumeControl);
    RUN_TEST(MuteControl);
    RUN_TEST(GlobalFunctions);

    VoiceManager::Instance().Shutdown();
    MixManager::Instance().Shutdown();
    Platform_Audio_Shutdown();
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
# Task 17d: Voice Manager
add_executable(TestVoiceManager
    ${CMAKE_SOURCE_DIR}/src/test_voice_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/game/audio/voice_manager.cpp
    ${AUDIO_AUD_SOURCES}
)
target_include_directories(TestVoiceManager PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(TestVoiceManager PRIVATE redalert_platform)
add_test(NAME VoiceManagerTests COMMAND TestVoiceManager)
```

---

## Verification Script

```bash
#!/bin/bash
set -e
echo "=== Task 17d Verification: Voice Manager ==="

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"

echo "Checking files..."
for f in include/game/audio/voice_event.h include/game/audio/voice_manager.h \
         src/game/audio/voice_manager.cpp src/test_voice_manager.cpp; do
    [ -f "$ROOT_DIR/$f" ] || { echo "Missing: $f"; exit 1; }
    echo "  ✓ $f"
done

echo "Building..."
cd "$BUILD_DIR" && cmake --build . --target TestVoiceManager
echo "  ✓ Built"

echo "Running tests..."
"$BUILD_DIR/TestVoiceManager" || { echo "Tests failed"; exit 1; }
echo "  ✓ Tests passed"

echo "=== Task 17d Verification PASSED ==="
```

---

## Success Criteria

- [ ] EvaVoice enum defines 25+ EVA announcements
- [ ] UnitVoice enum defines 20+ unit voices
- [ ] Priority-based EVA playback works
- [ ] EVA queue system works
- [ ] Rate limiting prevents voice spam
- [ ] Unit voices play correctly
- [ ] Volume control works
- [ ] All tests pass

---

## Completion Promise

When verification passes, output:
<promise>TASK_17D_COMPLETE</promise>

## Escape Hatch

If stuck after 15 iterations:
- Document in `ralph-tasks/blocked/17d.md`
- Output: <promise>TASK_17D_BLOCKED</promise>

## Max Iterations
15
