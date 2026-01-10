# Task 17b: Sound Effect Manager

| Field | Value |
|-------|-------|
| **Task ID** | 17b |
| **Task Name** | Sound Effect Manager |
| **Phase** | 17 - Audio Integration |
| **Depends On** | Task 17a (AUD File Parser), MixManager |
| **Estimated Complexity** | Medium (~450 lines) |

---

## Dependencies

- Task 17a (AUD File Parser) must be complete:
  - `AudFile` class for loading/decoding AUD files
- MixManager must be available:
  - `MixManager::Instance().FindFile()` for locating sounds in MIX archives
- Platform audio API:
  - `Platform_Sound_CreateFromMemory()`, `Platform_Sound_Play()`, etc.
- Viewport system (for listener position):
  - `Viewport::Instance()` for camera center coordinates

---

## Context

The Sound Effect Manager provides the game-level API for playing sound effects. It handles:

1. **Sound Enumeration**: Maps game sound IDs to AUD filenames
2. **Sound Loading**: Pre-loads sounds from MIX archives on startup
3. **Positional Audio**: Attenuates volume based on distance from camera
4. **Volume Control**: Separate SFX volume from music/voice
5. **Playback Management**: Prevents sound spam, manages concurrent sounds

This is the interface that game code uses to play sounds - game code should never touch the platform audio API directly for SFX.

---

## Objective

Create a Sound Effect Manager that:
1. Defines all game sound effects as an enum
2. Maps sound IDs to AUD filenames
3. Pre-loads all sounds into memory on initialization
4. Provides simple `Play()` and `PlayAt()` APIs
5. Implements distance-based volume attenuation
6. Supports volume control and muting
7. Limits concurrent sounds to prevent audio overload

---

## Technical Background

### Original Game Sound Effects

Red Alert has approximately 50+ sound effects stored in `SOUNDS.MIX`:

**UI Sounds:**
- `CLICK.AUD` - Button click
- `CANCEL.AUD` - Cancel action
- `BUILD.AUD` / `PLACBLDG.AUD` - Building placement

**Combat:**
- `XPLOS.AUD`, `XPLOSML.AUD`, `XPLOBIG.AUD` - Explosions
- `GUNSHOT.AUD`, `GUN1.AUD` - Small arms
- `CANNON.AUD` - Tank cannon
- `MGUN1.AUD`, `MGUN2.AUD` - Machine guns
- `ROCKET1.AUD`, `ROCKET2.AUD` - Rockets/missiles
- `TESLA1.AUD` - Tesla coil

**Units:**
- `VEHICLEA.AUD`, `VEHICLEB.AUD` - Vehicle movement
- `INFANTRY.AUD` - Infantry footsteps
- `CHROTNK1.AUD` - Chrono tank

**Buildings:**
- `POWRUP1.AUD` - Power up
- `POWDOWN1.AUD` - Power down
- `CONSTRU2.AUD` - Construction sounds

### Positional Audio Algorithm

```
Volume = BaseVolume * DistanceFactor * SFXVolume

DistanceFactor:
  if distance >= MAX_AUDIBLE_DISTANCE:
    return 0 (inaudible)
  else:
    return 1.0 - (distance / MAX_AUDIBLE_DISTANCE)

Distance = sqrt((sound_x - listener_x)^2 + (sound_y - listener_y)^2)
```

For Red Alert, reasonable values:
- `MAX_AUDIBLE_DISTANCE` = 1200 pixels (about 50 cells)
- `MIN_AUDIBLE_VOLUME` = 0.05 (5% minimum before cutting off)

### Concurrent Sound Limits

To prevent audio overload:
- Maximum 8 simultaneous sound effects
- Same sound effect can't play more than 3 times concurrently
- Minimum 50ms between same sound plays (debounce)

---

## Deliverables

- [ ] `include/game/audio/sound_effect.h` - Sound effect enumeration
- [ ] `include/game/audio/sound_manager.h` - SoundManager class declaration
- [ ] `src/game/audio/sound_manager.cpp` - Implementation
- [ ] `src/test_sound_manager.cpp` - Unit tests and interactive test
- [ ] CMakeLists.txt updated
- [ ] All tests pass

---

## Files to Create

### include/game/audio/sound_effect.h

```cpp
// include/game/audio/sound_effect.h
// Sound effect enumeration and metadata
// Task 17b - Sound Effect Manager

#ifndef SOUND_EFFECT_H
#define SOUND_EFFECT_H

#include <cstdint>

//=============================================================================
// Sound Effect Enumeration
//=============================================================================
// These IDs map to AUD files in SOUNDS.MIX
// Order matches original game for compatibility

enum class SoundEffect : int16_t {
    NONE = 0,

    //=========================================================================
    // UI Sounds (1-10)
    //=========================================================================
    UI_CLICK,           // CLICK.AUD - Button/menu click
    UI_BEEP,            // BEEP1.AUD - UI beep
    UI_CANCEL,          // CANCEL.AUD - Cancel action
    UI_ERROR,           // BUZZY1.AUD - Error buzz
    UI_TARGET,          // TARGET1.AUD - Target acquired

    //=========================================================================
    // Construction Sounds (11-20)
    //=========================================================================
    BUILD_PLACE,        // PLACBLDG.AUD - Place building
    BUILD_SOLD,         // SOLD.AUD - Building sold
    BUILD_PRIMARY,      // PRIMARY.AUD - Set primary building
    BUILD_CLOCK,        // CLOCK.AUD - Construction timer tick
    BUILD_CRUMBLE,      // CRUMBLE.AUD - Building crumbling

    //=========================================================================
    // Combat - Explosions (21-35)
    //=========================================================================
    EXPLODE_TINY,       // XPLOSML.AUD - Tiny explosion
    EXPLODE_SMALL,      // XPLOS.AUD - Small explosion
    EXPLODE_MEDIUM,     // XPLOMED.AUD - Medium explosion
    EXPLODE_LARGE,      // XPLOBIG.AUD - Large explosion
    EXPLODE_HUGE,       // XPLOMGE.AUD - Huge explosion
    EXPLODE_FIRE,       // FIRE1.AUD - Fire explosion
    EXPLODE_WATER,      // SPLASH1.AUD - Water splash
    EXPLODE_NUKE,       // NUKEXPLO.AUD - Nuclear explosion

    //=========================================================================
    // Combat - Weapons (36-60)
    //=========================================================================
    WEAPON_PISTOL,      // PISTOL.AUD - Pistol fire
    WEAPON_RIFLE,       // GUN1.AUD - Rifle fire
    WEAPON_MGUN,        // MGUN1.AUD - Machine gun
    WEAPON_MGUN_BURST,  // MGUN2.AUD - MG burst
    WEAPON_CANNON,      // CANNON.AUD - Tank cannon
    WEAPON_ARTILLERY,   // ARTIL.AUD - Artillery
    WEAPON_ROCKET,      // ROCKET1.AUD - Rocket launch
    WEAPON_MISSILE,     // ROCKET2.AUD - Missile launch
    WEAPON_GRENADE,     // GRENADE.AUD - Grenade
    WEAPON_FLAME,       // FLAME.AUD - Flamethrower
    WEAPON_TESLA,       // TESLA1.AUD - Tesla coil
    WEAPON_TESLA_CHARGE,// TELSCHG.AUD - Tesla charging
    WEAPON_AA,          // FLAK1.AUD - AA gun
    WEAPON_DEPTH_CHARGE,// DEPTHCH.AUD - Depth charge
    WEAPON_TORPEDO,     // TORPEDO.AUD - Torpedo

    //=========================================================================
    // Combat - Impacts (61-75)
    //=========================================================================
    IMPACT_BULLET,      // RICOCHET.AUD - Bullet ricochet
    IMPACT_SHELL,       // SHELL.AUD - Shell impact
    IMPACT_METAL,       // METAL1.AUD - Metal impact
    IMPACT_CONCRETE,    // CONCRET1.AUD - Concrete impact
    IMPACT_GROUND,      // DIRT1.AUD - Ground impact
    IMPACT_WATER,       // WATERSPL.AUD - Water impact

    //=========================================================================
    // Unit Movement (76-90)
    //=========================================================================
    MOVE_VEHICLE_START, // VSTART.AUD - Vehicle startup
    MOVE_VEHICLE,       // VEHICLEA.AUD - Vehicle moving
    MOVE_VEHICLE_HEAVY, // VEHICLEB.AUD - Heavy vehicle
    MOVE_TANK_TURRET,   // TURRET.AUD - Tank turret rotation
    MOVE_INFANTRY,      // INFANTRY.AUD - Infantry footsteps
    MOVE_HELICOPTER,    // COPTER.AUD - Helicopter rotor
    MOVE_PLANE,         // PLANE.AUD - Aircraft engine
    MOVE_SHIP,          // SHIP.AUD - Ship engine
    MOVE_SUB,           // SUB.AUD - Submarine

    //=========================================================================
    // Unit Actions (91-105)
    //=========================================================================
    ACTION_REPAIR,      // REPAIR.AUD - Repair sound
    ACTION_HARVEST,     // HARVEST.AUD - Harvesting ore
    ACTION_CRUSH,       // CRUSH.AUD - Vehicle crushing
    ACTION_CLOAK,       // CLOAK.AUD - Cloaking
    ACTION_UNCLOAK,     // UNCLOAK.AUD - Uncloaking
    ACTION_CHRONO,      // CHRONO.AUD - Chronosphere
    ACTION_IRON_CURTAIN,// IRON.AUD - Iron Curtain
    ACTION_PARABOMB,    // PARABOMB.AUD - Parabomb drop

    //=========================================================================
    // Environment (106-115)
    //=========================================================================
    AMBIENT_FIRE,       // BURN.AUD - Fire burning
    AMBIENT_ELECTRIC,   // ELECTRIC.AUD - Electric hum
    AMBIENT_ALARM,      // ALARM.AUD - Base alarm
    AMBIENT_SIREN,      // SIREN.AUD - Air raid siren

    //=========================================================================
    // Special (116-125)
    //=========================================================================
    SPECIAL_RADAR_ON,   // RADARUP.AUD - Radar online
    SPECIAL_RADAR_OFF,  // RADARDN.AUD - Radar offline
    SPECIAL_POWER_UP,   // POWRUP1.AUD - Power restored
    SPECIAL_POWER_DOWN, // POWDOWN1.AUD - Low power
    SPECIAL_GPS,        // GPS.AUD - GPS satellite

    // Sentinel value
    COUNT
};

//=============================================================================
// Sound Categories (for volume mixing)
//=============================================================================

enum class SoundCategory : uint8_t {
    UI,         // Interface sounds - not positional
    COMBAT,     // Weapons and explosions
    UNIT,       // Unit movement and actions
    AMBIENT,    // Environmental sounds
    SPECIAL     // One-shot special effects
};

//=============================================================================
// Sound Metadata
//=============================================================================

struct SoundInfo {
    const char* filename;       // AUD filename
    SoundCategory category;     // Category for mixing
    uint8_t priority;           // 0-255, higher = more important
    uint8_t max_concurrent;     // Max simultaneous plays (0 = unlimited)
    uint16_t min_interval_ms;   // Minimum ms between plays
    float default_volume;       // Default volume multiplier (0.0-1.0)
};

/// Get metadata for a sound effect
const SoundInfo& GetSoundInfo(SoundEffect sfx);

/// Get filename for a sound effect
const char* GetSoundFilename(SoundEffect sfx);

/// Get category for a sound effect
SoundCategory GetSoundCategory(SoundEffect sfx);

#endif // SOUND_EFFECT_H
```

### include/game/audio/sound_manager.h

```cpp
// include/game/audio/sound_manager.h
// Sound Effect Manager
// Task 17b - Sound Effect Manager

#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include "game/audio/sound_effect.h"
#include "platform.h"
#include <cstdint>
#include <vector>
#include <array>

//=============================================================================
// Forward Declarations
//=============================================================================

class AudFile;

//=============================================================================
// Sound Manager Configuration
//=============================================================================

struct SoundManagerConfig {
    int max_concurrent_sounds = 16;       // Max total simultaneous sounds
    int max_same_sound = 3;               // Max plays of same sound
    int default_max_distance = 1200;      // Max audible distance in pixels
    float min_audible_volume = 0.05f;     // Volume threshold for culling
};

//=============================================================================
// Playing Sound Info
//=============================================================================

struct PlayingSoundInfo {
    PlayHandle play_handle;      // Platform play handle
    SoundEffect sound_id;        // Which sound
    uint64_t start_time_ms;      // When playback started
    bool positional;             // Is position-based
    int world_x;                 // World position X
    int world_y;                 // World position Y
    float volume;                // Current volume
};

//=============================================================================
// Sound Manager Class
//=============================================================================

class SoundManager {
public:
    /// Singleton instance
    static SoundManager& Instance();

    //=========================================================================
    // Lifecycle
    //=========================================================================

    /// Initialize sound manager and load all sounds
    /// @param config Optional configuration
    /// @return true if successful
    bool Initialize(const SoundManagerConfig& config = SoundManagerConfig());

    /// Shutdown and release all resources
    void Shutdown();

    /// Check if initialized
    bool IsInitialized() const { return initialized_; }

    /// Load all sound effects from MIX files
    /// Called automatically by Initialize(), can be called again to reload
    void LoadAllSounds();

    /// Unload all sounds to free memory
    void UnloadAllSounds();

    //=========================================================================
    // Playback
    //=========================================================================

    /// Play a sound effect (non-positional, full volume)
    /// @param sfx Sound effect to play
    /// @return Play handle, or INVALID_PLAY_HANDLE if failed
    PlayHandle Play(SoundEffect sfx);

    /// Play a sound effect with volume
    /// @param sfx Sound effect to play
    /// @param volume Volume multiplier (0.0-1.0)
    /// @return Play handle
    PlayHandle Play(SoundEffect sfx, float volume);

    /// Play a sound effect at a world position
    /// Volume is attenuated based on distance from listener
    /// @param sfx Sound effect to play
    /// @param world_x World X coordinate
    /// @param world_y World Y coordinate
    /// @param volume Base volume before attenuation
    /// @return Play handle
    PlayHandle PlayAt(SoundEffect sfx, int world_x, int world_y, float volume = 1.0f);

    /// Play a sound at a cell coordinate
    /// @param sfx Sound effect to play
    /// @param cell_x Cell X coordinate
    /// @param cell_y Cell Y coordinate
    /// @param volume Base volume
    /// @return Play handle
    PlayHandle PlayAtCell(SoundEffect sfx, int cell_x, int cell_y, float volume = 1.0f);

    /// Stop a specific playing sound
    void Stop(PlayHandle handle);

    /// Stop all currently playing sound effects
    void StopAll();

    /// Check if a sound is currently playing
    bool IsPlaying(PlayHandle handle) const;

    //=========================================================================
    // Volume Control
    //=========================================================================

    /// Set SFX master volume (0.0-1.0)
    void SetVolume(float volume);

    /// Get SFX master volume
    float GetVolume() const { return sfx_volume_; }

    /// Set volume for a category
    void SetCategoryVolume(SoundCategory category, float volume);

    /// Get category volume
    float GetCategoryVolume(SoundCategory category) const;

    /// Mute/unmute all sound effects
    void SetMuted(bool muted);

    /// Check if muted
    bool IsMuted() const { return muted_; }

    //=========================================================================
    // Listener Position (for positional audio)
    //=========================================================================

    /// Set listener position (typically camera center)
    void SetListenerPosition(int world_x, int world_y);

    /// Get listener position
    void GetListenerPosition(int& world_x, int& world_y) const;

    /// Update listener position from viewport
    /// Call each frame to track camera movement
    void UpdateListenerFromViewport();

    //=========================================================================
    // Positional Audio Settings
    //=========================================================================

    /// Set maximum audible distance in world pixels
    void SetMaxDistance(int distance);

    /// Get maximum audible distance
    int GetMaxDistance() const { return max_distance_; }

    //=========================================================================
    // Per-Frame Update
    //=========================================================================

    /// Update sound manager state
    /// Cleans up finished sounds, updates positional audio
    void Update();

    //=========================================================================
    // Debug/Stats
    //=========================================================================

    /// Get number of loaded sounds
    int GetLoadedSoundCount() const;

    /// Get number of currently playing sounds
    int GetPlayingSoundCount() const;

    /// Check if a specific sound is loaded
    bool IsSoundLoaded(SoundEffect sfx) const;

    /// Print debug information
    void PrintStats() const;

private:
    SoundManager();
    ~SoundManager();
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    //=========================================================================
    // Internal Data
    //=========================================================================

    /// Sound data for each effect
    struct LoadedSound {
        SoundHandle platform_handle;    // Platform sound handle
        bool loaded;                    // Successfully loaded
        uint64_t last_play_time;        // Last time this sound was played
        int current_play_count;         // How many currently playing
    };

    std::array<LoadedSound, static_cast<size_t>(SoundEffect::COUNT)> sounds_;
    std::vector<PlayingSoundInfo> playing_sounds_;

    // Configuration
    SoundManagerConfig config_;

    // Volume settings
    float sfx_volume_;
    std::array<float, 5> category_volumes_;  // One per SoundCategory
    bool muted_;

    // Listener position
    int listener_x_;
    int listener_y_;
    int max_distance_;

    // State
    bool initialized_;
    uint64_t current_time_;

    //=========================================================================
    // Internal Methods
    //=========================================================================

    /// Load a single sound effect
    bool LoadSound(SoundEffect sfx);

    /// Calculate volume based on distance
    float CalculateDistanceVolume(int world_x, int world_y) const;

    /// Check if sound can play (rate limiting, concurrent limits)
    bool CanPlaySound(SoundEffect sfx) const;

    /// Clean up finished sounds from playing list
    void CleanupFinishedSounds();

    /// Get final volume for a sound
    float GetFinalVolume(SoundEffect sfx, float base_volume) const;
};

//=============================================================================
// Global Convenience Functions
//=============================================================================

/// Initialize sound manager
bool Sound_Init();

/// Shutdown sound manager
void Sound_Shutdown();

/// Update sound manager (call each frame)
void Sound_Update();

/// Play a sound effect
PlayHandle Sound_Play(SoundEffect sfx);

/// Play a sound effect at position
PlayHandle Sound_PlayAt(SoundEffect sfx, int world_x, int world_y);

/// Stop all sound effects
void Sound_StopAll();

/// Set SFX volume (0-255 for compatibility with original)
void Sound_SetVolume(int volume);

/// Get SFX volume (0-255)
int Sound_GetVolume();

#endif // SOUND_MANAGER_H
```

### src/game/audio/sound_manager.cpp

```cpp
// src/game/audio/sound_manager.cpp
// Sound Effect Manager implementation
// Task 17b - Sound Effect Manager

#include "game/audio/sound_manager.h"
#include "game/audio/aud_file.h"
#include "game/mix_manager.h"
#include "game/viewport.h"
#include "platform.h"
#include <cstring>
#include <algorithm>
#include <cmath>

//=============================================================================
// Sound Info Database
//=============================================================================

// Sound metadata table - indexed by SoundEffect enum
static const SoundInfo SOUND_INFO[] = {
    // NONE
    { nullptr, SoundCategory::UI, 0, 0, 0, 0.0f },

    // UI Sounds
    { "CLICK.AUD",    SoundCategory::UI, 200, 2, 50, 1.0f },   // UI_CLICK
    { "BEEP1.AUD",    SoundCategory::UI, 180, 2, 100, 0.8f },  // UI_BEEP
    { "CANCEL.AUD",   SoundCategory::UI, 180, 1, 100, 1.0f },  // UI_CANCEL
    { "BUZZY1.AUD",   SoundCategory::UI, 220, 1, 200, 0.9f },  // UI_ERROR
    { "TARGET1.AUD",  SoundCategory::UI, 150, 2, 100, 0.7f },  // UI_TARGET

    // Construction Sounds
    { "PLACBLDG.AUD", SoundCategory::SPECIAL, 180, 1, 200, 1.0f },  // BUILD_PLACE
    { "SOLD.AUD",     SoundCategory::SPECIAL, 180, 1, 200, 1.0f },  // BUILD_SOLD
    { "PRIMARY.AUD",  SoundCategory::SPECIAL, 180, 1, 200, 0.9f },  // BUILD_PRIMARY
    { "CLOCK.AUD",    SoundCategory::UI, 100, 1, 500, 0.5f },       // BUILD_CLOCK
    { "CRUMBLE.AUD",  SoundCategory::COMBAT, 200, 2, 100, 1.0f },   // BUILD_CRUMBLE

    // Explosions
    { "XPLOSML.AUD",  SoundCategory::COMBAT, 150, 4, 30, 0.8f },   // EXPLODE_TINY
    { "XPLOS.AUD",    SoundCategory::COMBAT, 160, 4, 30, 0.9f },   // EXPLODE_SMALL
    { "XPLOMED.AUD",  SoundCategory::COMBAT, 170, 3, 50, 1.0f },   // EXPLODE_MEDIUM
    { "XPLOBIG.AUD",  SoundCategory::COMBAT, 180, 3, 50, 1.0f },   // EXPLODE_LARGE
    { "XPLOMGE.AUD",  SoundCategory::COMBAT, 200, 2, 100, 1.0f },  // EXPLODE_HUGE
    { "FIRE1.AUD",    SoundCategory::COMBAT, 140, 4, 30, 0.7f },   // EXPLODE_FIRE
    { "SPLASH1.AUD",  SoundCategory::COMBAT, 130, 4, 30, 0.6f },   // EXPLODE_WATER
    { "NUKEXPLO.AUD", SoundCategory::COMBAT, 255, 1, 500, 1.0f },  // EXPLODE_NUKE

    // Weapons
    { "PISTOL.AUD",   SoundCategory::COMBAT, 120, 4, 30, 0.6f },   // WEAPON_PISTOL
    { "GUN1.AUD",     SoundCategory::COMBAT, 130, 4, 30, 0.7f },   // WEAPON_RIFLE
    { "MGUN1.AUD",    SoundCategory::COMBAT, 140, 3, 50, 0.8f },   // WEAPON_MGUN
    { "MGUN2.AUD",    SoundCategory::COMBAT, 140, 3, 50, 0.8f },   // WEAPON_MGUN_BURST
    { "CANNON.AUD",   SoundCategory::COMBAT, 160, 3, 50, 1.0f },   // WEAPON_CANNON
    { "ARTIL.AUD",    SoundCategory::COMBAT, 170, 2, 100, 1.0f },  // WEAPON_ARTILLERY
    { "ROCKET1.AUD",  SoundCategory::COMBAT, 150, 3, 50, 0.9f },   // WEAPON_ROCKET
    { "ROCKET2.AUD",  SoundCategory::COMBAT, 150, 3, 50, 0.9f },   // WEAPON_MISSILE
    { "GRENADE.AUD",  SoundCategory::COMBAT, 140, 4, 30, 0.8f },   // WEAPON_GRENADE
    { "FLAME.AUD",    SoundCategory::COMBAT, 140, 2, 100, 0.8f },  // WEAPON_FLAME
    { "TESLA1.AUD",   SoundCategory::COMBAT, 180, 2, 100, 1.0f },  // WEAPON_TESLA
    { "TELSCHG.AUD",  SoundCategory::COMBAT, 170, 1, 200, 0.9f },  // WEAPON_TESLA_CHARGE
    { "FLAK1.AUD",    SoundCategory::COMBAT, 150, 3, 50, 0.9f },   // WEAPON_AA
    { "DEPTHCH.AUD",  SoundCategory::COMBAT, 160, 2, 100, 1.0f },  // WEAPON_DEPTH_CHARGE
    { "TORPEDO.AUD",  SoundCategory::COMBAT, 150, 2, 100, 0.9f },  // WEAPON_TORPEDO

    // Impacts
    { "RICOCHET.AUD", SoundCategory::COMBAT, 100, 6, 20, 0.5f },   // IMPACT_BULLET
    { "SHELL.AUD",    SoundCategory::COMBAT, 120, 4, 30, 0.6f },   // IMPACT_SHELL
    { "METAL1.AUD",   SoundCategory::COMBAT, 110, 4, 30, 0.6f },   // IMPACT_METAL
    { "CONCRET1.AUD", SoundCategory::COMBAT, 110, 4, 30, 0.6f },   // IMPACT_CONCRETE
    { "DIRT1.AUD",    SoundCategory::COMBAT, 100, 4, 30, 0.5f },   // IMPACT_GROUND
    { "WATERSPL.AUD", SoundCategory::COMBAT, 100, 4, 30, 0.5f },   // IMPACT_WATER

    // Unit Movement
    { "VSTART.AUD",   SoundCategory::UNIT, 100, 3, 100, 0.6f },    // MOVE_VEHICLE_START
    { "VEHICLEA.AUD", SoundCategory::UNIT, 80, 2, 200, 0.5f },     // MOVE_VEHICLE
    { "VEHICLEB.AUD", SoundCategory::UNIT, 80, 2, 200, 0.5f },     // MOVE_VEHICLE_HEAVY
    { "TURRET.AUD",   SoundCategory::UNIT, 90, 3, 100, 0.4f },     // MOVE_TANK_TURRET
    { "INFANTRY.AUD", SoundCategory::UNIT, 60, 4, 50, 0.3f },      // MOVE_INFANTRY
    { "COPTER.AUD",   SoundCategory::UNIT, 100, 2, 200, 0.6f },    // MOVE_HELICOPTER
    { "PLANE.AUD",    SoundCategory::UNIT, 100, 2, 200, 0.6f },    // MOVE_PLANE
    { "SHIP.AUD",     SoundCategory::UNIT, 80, 2, 200, 0.5f },     // MOVE_SHIP
    { "SUB.AUD",      SoundCategory::UNIT, 70, 2, 200, 0.4f },     // MOVE_SUB

    // Unit Actions
    { "REPAIR.AUD",   SoundCategory::UNIT, 120, 2, 200, 0.7f },    // ACTION_REPAIR
    { "HARVEST.AUD",  SoundCategory::UNIT, 100, 2, 200, 0.6f },    // ACTION_HARVEST
    { "CRUSH.AUD",    SoundCategory::UNIT, 140, 3, 100, 0.8f },    // ACTION_CRUSH
    { "CLOAK.AUD",    SoundCategory::SPECIAL, 150, 2, 200, 0.8f }, // ACTION_CLOAK
    { "UNCLOAK.AUD",  SoundCategory::SPECIAL, 150, 2, 200, 0.8f }, // ACTION_UNCLOAK
    { "CHRONO.AUD",   SoundCategory::SPECIAL, 200, 1, 500, 1.0f }, // ACTION_CHRONO
    { "IRON.AUD",     SoundCategory::SPECIAL, 200, 1, 500, 1.0f }, // ACTION_IRON_CURTAIN
    { "PARABOMB.AUD", SoundCategory::SPECIAL, 180, 1, 500, 1.0f }, // ACTION_PARABOMB

    // Environment
    { "BURN.AUD",     SoundCategory::AMBIENT, 50, 4, 100, 0.4f },  // AMBIENT_FIRE
    { "ELECTRIC.AUD", SoundCategory::AMBIENT, 60, 2, 200, 0.3f },  // AMBIENT_ELECTRIC
    { "ALARM.AUD",    SoundCategory::AMBIENT, 150, 1, 1000, 0.8f },// AMBIENT_ALARM
    { "SIREN.AUD",    SoundCategory::AMBIENT, 160, 1, 2000, 0.9f },// AMBIENT_SIREN

    // Special
    { "RADARUP.AUD",  SoundCategory::SPECIAL, 180, 1, 500, 1.0f }, // SPECIAL_RADAR_ON
    { "RADARDN.AUD",  SoundCategory::SPECIAL, 180, 1, 500, 1.0f }, // SPECIAL_RADAR_OFF
    { "POWRUP1.AUD",  SoundCategory::SPECIAL, 180, 1, 500, 1.0f }, // SPECIAL_POWER_UP
    { "POWDOWN1.AUD", SoundCategory::SPECIAL, 180, 1, 500, 1.0f }, // SPECIAL_POWER_DOWN
    { "GPS.AUD",      SoundCategory::SPECIAL, 180, 1, 500, 0.9f }, // SPECIAL_GPS
};

// Verify table size matches enum
static_assert(sizeof(SOUND_INFO) / sizeof(SOUND_INFO[0]) == static_cast<size_t>(SoundEffect::COUNT),
              "SOUND_INFO table size mismatch");

//=============================================================================
// Sound Info Accessors
//=============================================================================

const SoundInfo& GetSoundInfo(SoundEffect sfx) {
    int idx = static_cast<int>(sfx);
    if (idx < 0 || idx >= static_cast<int>(SoundEffect::COUNT)) {
        return SOUND_INFO[0];  // Return NONE info
    }
    return SOUND_INFO[idx];
}

const char* GetSoundFilename(SoundEffect sfx) {
    return GetSoundInfo(sfx).filename;
}

SoundCategory GetSoundCategory(SoundEffect sfx) {
    return GetSoundInfo(sfx).category;
}

//=============================================================================
// SoundManager Implementation
//=============================================================================

SoundManager& SoundManager::Instance() {
    static SoundManager instance;
    return instance;
}

SoundManager::SoundManager()
    : sfx_volume_(1.0f)
    , muted_(false)
    , listener_x_(0)
    , listener_y_(0)
    , max_distance_(1200)
    , initialized_(false)
    , current_time_(0) {

    // Initialize all sounds as unloaded
    for (auto& sound : sounds_) {
        sound.platform_handle = INVALID_SOUND_HANDLE;
        sound.loaded = false;
        sound.last_play_time = 0;
        sound.current_play_count = 0;
    }

    // Initialize category volumes
    category_volumes_.fill(1.0f);
}

SoundManager::~SoundManager() {
    Shutdown();
}

bool SoundManager::Initialize(const SoundManagerConfig& config) {
    if (initialized_) {
        return true;
    }

    Platform_Log("SoundManager: Initializing...");

    config_ = config;
    max_distance_ = config.default_max_distance;

    // Load all sounds
    LoadAllSounds();

    initialized_ = true;
    Platform_Log("SoundManager: Initialized with %d sounds loaded", GetLoadedSoundCount());

    return true;
}

void SoundManager::Shutdown() {
    if (!initialized_) {
        return;
    }

    Platform_Log("SoundManager: Shutting down...");

    StopAll();
    UnloadAllSounds();

    initialized_ = false;
}

void SoundManager::LoadAllSounds() {
    Platform_Log("SoundManager: Loading sound effects...");

    int loaded = 0;
    int failed = 0;

    for (int i = 1; i < static_cast<int>(SoundEffect::COUNT); i++) {
        SoundEffect sfx = static_cast<SoundEffect>(i);
        if (LoadSound(sfx)) {
            loaded++;
        } else {
            failed++;
        }
    }

    Platform_Log("SoundManager: Loaded %d sounds, %d failed", loaded, failed);
}

void SoundManager::UnloadAllSounds() {
    for (auto& sound : sounds_) {
        if (sound.loaded && sound.platform_handle != INVALID_SOUND_HANDLE) {
            Platform_Sound_Destroy(sound.platform_handle);
        }
        sound.platform_handle = INVALID_SOUND_HANDLE;
        sound.loaded = false;
    }
}

bool SoundManager::LoadSound(SoundEffect sfx) {
    int idx = static_cast<int>(sfx);
    if (idx <= 0 || idx >= static_cast<int>(SoundEffect::COUNT)) {
        return false;
    }

    const SoundInfo& info = SOUND_INFO[idx];
    if (!info.filename) {
        return false;
    }

    // Load AUD file
    AudFile aud;
    if (!aud.LoadFromMix(info.filename)) {
        // Try with .AUD extension if not present
        char filename_with_ext[64];
        snprintf(filename_with_ext, sizeof(filename_with_ext), "%s.AUD", info.filename);
        if (!aud.LoadFromMix(filename_with_ext)) {
            Platform_Log("SoundManager: Failed to load %s", info.filename);
            return false;
        }
    }

    // Create platform sound
    SoundHandle handle = Platform_Sound_CreateFromMemory(
        aud.GetPCMDataBytes(),
        static_cast<int32_t>(aud.GetPCMDataSize()),
        aud.GetSampleRate(),
        aud.GetChannels(),
        16  // Always 16-bit after decode
    );

    if (handle == INVALID_SOUND_HANDLE) {
        Platform_Log("SoundManager: Failed to create sound for %s", info.filename);
        return false;
    }

    sounds_[idx].platform_handle = handle;
    sounds_[idx].loaded = true;

    return true;
}

//=============================================================================
// Playback
//=============================================================================

PlayHandle SoundManager::Play(SoundEffect sfx) {
    return Play(sfx, 1.0f);
}

PlayHandle SoundManager::Play(SoundEffect sfx, float volume) {
    if (!initialized_ || muted_) {
        return INVALID_PLAY_HANDLE;
    }

    int idx = static_cast<int>(sfx);
    if (idx <= 0 || idx >= static_cast<int>(SoundEffect::COUNT)) {
        return INVALID_PLAY_HANDLE;
    }

    if (!sounds_[idx].loaded) {
        return INVALID_PLAY_HANDLE;
    }

    if (!CanPlaySound(sfx)) {
        return INVALID_PLAY_HANDLE;
    }

    float final_volume = GetFinalVolume(sfx, volume);
    if (final_volume < config_.min_audible_volume) {
        return INVALID_PLAY_HANDLE;
    }

    PlayHandle play_handle = Platform_Sound_Play(
        sounds_[idx].platform_handle,
        final_volume,
        0.0f,   // No pan
        false   // Not looping
    );

    if (play_handle != INVALID_PLAY_HANDLE) {
        // Track playing sound
        PlayingSoundInfo info;
        info.play_handle = play_handle;
        info.sound_id = sfx;
        info.start_time_ms = current_time_;
        info.positional = false;
        info.world_x = 0;
        info.world_y = 0;
        info.volume = final_volume;
        playing_sounds_.push_back(info);

        // Update tracking
        sounds_[idx].last_play_time = current_time_;
        sounds_[idx].current_play_count++;
    }

    return play_handle;
}

PlayHandle SoundManager::PlayAt(SoundEffect sfx, int world_x, int world_y, float volume) {
    if (!initialized_ || muted_) {
        return INVALID_PLAY_HANDLE;
    }

    int idx = static_cast<int>(sfx);
    if (idx <= 0 || idx >= static_cast<int>(SoundEffect::COUNT)) {
        return INVALID_PLAY_HANDLE;
    }

    if (!sounds_[idx].loaded) {
        return INVALID_PLAY_HANDLE;
    }

    if (!CanPlaySound(sfx)) {
        return INVALID_PLAY_HANDLE;
    }

    // Calculate distance-based volume
    float distance_factor = CalculateDistanceVolume(world_x, world_y);
    if (distance_factor <= 0.0f) {
        return INVALID_PLAY_HANDLE;  // Too far to hear
    }

    float final_volume = GetFinalVolume(sfx, volume * distance_factor);
    if (final_volume < config_.min_audible_volume) {
        return INVALID_PLAY_HANDLE;
    }

    // Calculate stereo pan based on position
    float pan = 0.0f;
    int dx = world_x - listener_x_;
    if (max_distance_ > 0) {
        pan = static_cast<float>(dx) / static_cast<float>(max_distance_);
        pan = std::clamp(pan, -1.0f, 1.0f);
    }

    PlayHandle play_handle = Platform_Sound_Play(
        sounds_[idx].platform_handle,
        final_volume,
        pan,
        false
    );

    if (play_handle != INVALID_PLAY_HANDLE) {
        PlayingSoundInfo info;
        info.play_handle = play_handle;
        info.sound_id = sfx;
        info.start_time_ms = current_time_;
        info.positional = true;
        info.world_x = world_x;
        info.world_y = world_y;
        info.volume = final_volume;
        playing_sounds_.push_back(info);

        sounds_[idx].last_play_time = current_time_;
        sounds_[idx].current_play_count++;
    }

    return play_handle;
}

PlayHandle SoundManager::PlayAtCell(SoundEffect sfx, int cell_x, int cell_y, float volume) {
    // Convert cell to world coordinates (center of cell)
    int world_x = cell_x * TILE_PIXEL_WIDTH + TILE_PIXEL_WIDTH / 2;
    int world_y = cell_y * TILE_PIXEL_HEIGHT + TILE_PIXEL_HEIGHT / 2;
    return PlayAt(sfx, world_x, world_y, volume);
}

void SoundManager::Stop(PlayHandle handle) {
    if (handle == INVALID_PLAY_HANDLE) {
        return;
    }

    Platform_Sound_Stop(handle);

    // Remove from playing list
    auto it = std::find_if(playing_sounds_.begin(), playing_sounds_.end(),
        [handle](const PlayingSoundInfo& info) {
            return info.play_handle == handle;
        });

    if (it != playing_sounds_.end()) {
        int idx = static_cast<int>(it->sound_id);
        if (idx > 0 && idx < static_cast<int>(SoundEffect::COUNT)) {
            if (sounds_[idx].current_play_count > 0) {
                sounds_[idx].current_play_count--;
            }
        }
        playing_sounds_.erase(it);
    }
}

void SoundManager::StopAll() {
    Platform_Sound_StopAll();

    // Reset all play counts
    for (auto& sound : sounds_) {
        sound.current_play_count = 0;
    }

    playing_sounds_.clear();
}

bool SoundManager::IsPlaying(PlayHandle handle) const {
    if (handle == INVALID_PLAY_HANDLE) {
        return false;
    }
    return Platform_Sound_IsPlaying(handle);
}

//=============================================================================
// Volume Control
//=============================================================================

void SoundManager::SetVolume(float volume) {
    sfx_volume_ = std::clamp(volume, 0.0f, 1.0f);
}

void SoundManager::SetCategoryVolume(SoundCategory category, float volume) {
    int idx = static_cast<int>(category);
    if (idx >= 0 && idx < static_cast<int>(category_volumes_.size())) {
        category_volumes_[idx] = std::clamp(volume, 0.0f, 1.0f);
    }
}

float SoundManager::GetCategoryVolume(SoundCategory category) const {
    int idx = static_cast<int>(category);
    if (idx >= 0 && idx < static_cast<int>(category_volumes_.size())) {
        return category_volumes_[idx];
    }
    return 1.0f;
}

void SoundManager::SetMuted(bool muted) {
    muted_ = muted;
    if (muted) {
        StopAll();
    }
}

//=============================================================================
// Listener Position
//=============================================================================

void SoundManager::SetListenerPosition(int world_x, int world_y) {
    listener_x_ = world_x;
    listener_y_ = world_y;
}

void SoundManager::GetListenerPosition(int& world_x, int& world_y) const {
    world_x = listener_x_;
    world_y = listener_y_;
}

void SoundManager::UpdateListenerFromViewport() {
    Viewport& vp = Viewport::Instance();

    // Listener is at center of viewport
    int center_x = vp.x + vp.width / 2;
    int center_y = vp.y + vp.height / 2;

    SetListenerPosition(center_x, center_y);
}

void SoundManager::SetMaxDistance(int distance) {
    max_distance_ = std::max(1, distance);
}

//=============================================================================
// Update
//=============================================================================

void SoundManager::Update() {
    if (!initialized_) {
        return;
    }

    current_time_ = Platform_GetTicks();

    // Update listener from viewport
    UpdateListenerFromViewport();

    // Clean up finished sounds
    CleanupFinishedSounds();
}

//=============================================================================
// Internal Methods
//=============================================================================

float SoundManager::CalculateDistanceVolume(int world_x, int world_y) const {
    int dx = world_x - listener_x_;
    int dy = world_y - listener_y_;
    float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));

    if (distance >= static_cast<float>(max_distance_)) {
        return 0.0f;
    }

    return 1.0f - (distance / static_cast<float>(max_distance_));
}

bool SoundManager::CanPlaySound(SoundEffect sfx) const {
    int idx = static_cast<int>(sfx);
    if (idx <= 0 || idx >= static_cast<int>(SoundEffect::COUNT)) {
        return false;
    }

    const LoadedSound& sound = sounds_[idx];
    const SoundInfo& info = SOUND_INFO[idx];

    // Check max concurrent for this sound
    if (info.max_concurrent > 0 && sound.current_play_count >= info.max_concurrent) {
        return false;
    }

    // Check rate limiting
    if (info.min_interval_ms > 0) {
        if (current_time_ - sound.last_play_time < info.min_interval_ms) {
            return false;
        }
    }

    // Check total concurrent sounds
    if (static_cast<int>(playing_sounds_.size()) >= config_.max_concurrent_sounds) {
        return false;
    }

    return true;
}

void SoundManager::CleanupFinishedSounds() {
    auto it = playing_sounds_.begin();
    while (it != playing_sounds_.end()) {
        if (!Platform_Sound_IsPlaying(it->play_handle)) {
            // Update play count
            int idx = static_cast<int>(it->sound_id);
            if (idx > 0 && idx < static_cast<int>(SoundEffect::COUNT)) {
                if (sounds_[idx].current_play_count > 0) {
                    sounds_[idx].current_play_count--;
                }
            }
            it = playing_sounds_.erase(it);
        } else {
            ++it;
        }
    }
}

float SoundManager::GetFinalVolume(SoundEffect sfx, float base_volume) const {
    const SoundInfo& info = GetSoundInfo(sfx);

    float category_vol = GetCategoryVolume(info.category);
    float default_vol = info.default_volume;

    return base_volume * default_vol * category_vol * sfx_volume_;
}

//=============================================================================
// Debug
//=============================================================================

int SoundManager::GetLoadedSoundCount() const {
    int count = 0;
    for (const auto& sound : sounds_) {
        if (sound.loaded) count++;
    }
    return count;
}

int SoundManager::GetPlayingSoundCount() const {
    return static_cast<int>(playing_sounds_.size());
}

bool SoundManager::IsSoundLoaded(SoundEffect sfx) const {
    int idx = static_cast<int>(sfx);
    if (idx <= 0 || idx >= static_cast<int>(SoundEffect::COUNT)) {
        return false;
    }
    return sounds_[idx].loaded;
}

void SoundManager::PrintStats() const {
    Platform_Log("SoundManager Stats:");
    Platform_Log("  Loaded sounds: %d / %d",
                 GetLoadedSoundCount(), static_cast<int>(SoundEffect::COUNT) - 1);
    Platform_Log("  Playing sounds: %d / %d",
                 GetPlayingSoundCount(), config_.max_concurrent_sounds);
    Platform_Log("  SFX Volume: %.0f%%", sfx_volume_ * 100.0f);
    Platform_Log("  Listener: (%d, %d)", listener_x_, listener_y_);
    Platform_Log("  Muted: %s", muted_ ? "yes" : "no");
}

//=============================================================================
// Global Convenience Functions
//=============================================================================

bool Sound_Init() {
    return SoundManager::Instance().Initialize();
}

void Sound_Shutdown() {
    SoundManager::Instance().Shutdown();
}

void Sound_Update() {
    SoundManager::Instance().Update();
}

PlayHandle Sound_Play(SoundEffect sfx) {
    return SoundManager::Instance().Play(sfx);
}

PlayHandle Sound_PlayAt(SoundEffect sfx, int world_x, int world_y) {
    return SoundManager::Instance().PlayAt(sfx, world_x, world_y);
}

void Sound_StopAll() {
    SoundManager::Instance().StopAll();
}

void Sound_SetVolume(int volume) {
    // Convert 0-255 to 0.0-1.0
    float vol = std::clamp(volume, 0, 255) / 255.0f;
    SoundManager::Instance().SetVolume(vol);
}

int Sound_GetVolume() {
    // Convert 0.0-1.0 to 0-255
    return static_cast<int>(SoundManager::Instance().GetVolume() * 255.0f);
}
```

### src/test_sound_manager.cpp

```cpp
// src/test_sound_manager.cpp
// Unit tests for Sound Manager
// Task 17b - Sound Effect Manager

#include "game/audio/sound_manager.h"
#include "game/audio/aud_file.h"
#include "game/mix_manager.h"
#include "game/viewport.h"
#include "platform.h"
#include <cstdio>
#include <cstring>
#include <cmath>

//=============================================================================
// Test Utilities
//=============================================================================

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("  FAILED: %s\n", msg); \
            return false; \
        } \
    } while(0)

#define RUN_TEST(name) \
    do { \
        printf("Test: %s... ", #name); \
        if (Test_##name()) { \
            printf("PASSED\n"); \
            tests_passed++; \
        } else { \
            tests_failed++; \
        } \
    } while(0)

//=============================================================================
// Unit Tests
//=============================================================================

bool Test_SoundEffectEnum() {
    // Verify enum values are sequential
    TEST_ASSERT(static_cast<int>(SoundEffect::NONE) == 0, "NONE should be 0");
    TEST_ASSERT(static_cast<int>(SoundEffect::COUNT) > 50, "Should have 50+ sounds");

    // Verify some key sounds exist
    TEST_ASSERT(static_cast<int>(SoundEffect::UI_CLICK) > 0, "UI_CLICK should exist");
    TEST_ASSERT(static_cast<int>(SoundEffect::EXPLODE_LARGE) > 0, "EXPLODE_LARGE should exist");
    TEST_ASSERT(static_cast<int>(SoundEffect::WEAPON_CANNON) > 0, "WEAPON_CANNON should exist");

    return true;
}

bool Test_SoundInfo() {
    // Test GetSoundInfo
    const SoundInfo& click_info = GetSoundInfo(SoundEffect::UI_CLICK);
    TEST_ASSERT(click_info.filename != nullptr, "UI_CLICK should have filename");
    TEST_ASSERT(click_info.category == SoundCategory::UI, "UI_CLICK should be UI category");

    const SoundInfo& cannon_info = GetSoundInfo(SoundEffect::WEAPON_CANNON);
    TEST_ASSERT(cannon_info.filename != nullptr, "WEAPON_CANNON should have filename");
    TEST_ASSERT(cannon_info.category == SoundCategory::COMBAT, "WEAPON_CANNON should be COMBAT");

    // Test invalid sound
    const SoundInfo& none_info = GetSoundInfo(SoundEffect::NONE);
    TEST_ASSERT(none_info.filename == nullptr, "NONE should have no filename");

    return true;
}

bool Test_SoundFilename() {
    const char* click = GetSoundFilename(SoundEffect::UI_CLICK);
    TEST_ASSERT(click != nullptr, "UI_CLICK filename should exist");
    TEST_ASSERT(strstr(click, "CLICK") != nullptr, "Should contain CLICK");

    const char* none = GetSoundFilename(SoundEffect::NONE);
    TEST_ASSERT(none == nullptr, "NONE should have no filename");

    return true;
}

bool Test_DistanceAttenuation() {
    // Test volume calculation at various distances
    SoundManager& mgr = SoundManager::Instance();

    // Initialize if needed
    if (!mgr.IsInitialized()) {
        mgr.Initialize();
    }

    mgr.SetMaxDistance(1000);
    mgr.SetListenerPosition(500, 500);

    // At listener position - full volume
    // (We can't directly test internal method, so just verify settings)
    int lx, ly;
    mgr.GetListenerPosition(lx, ly);
    TEST_ASSERT(lx == 500 && ly == 500, "Listener position should be set");
    TEST_ASSERT(mgr.GetMaxDistance() == 1000, "Max distance should be 1000");

    return true;
}

bool Test_VolumeControl() {
    SoundManager& mgr = SoundManager::Instance();

    if (!mgr.IsInitialized()) {
        mgr.Initialize();
    }

    // Test SFX volume
    mgr.SetVolume(0.5f);
    TEST_ASSERT(std::abs(mgr.GetVolume() - 0.5f) < 0.01f, "Volume should be 0.5");

    mgr.SetVolume(1.0f);
    TEST_ASSERT(std::abs(mgr.GetVolume() - 1.0f) < 0.01f, "Volume should be 1.0");

    // Test clamping
    mgr.SetVolume(2.0f);
    TEST_ASSERT(mgr.GetVolume() <= 1.0f, "Volume should be clamped to 1.0");

    mgr.SetVolume(-1.0f);
    TEST_ASSERT(mgr.GetVolume() >= 0.0f, "Volume should be clamped to 0.0");

    mgr.SetVolume(1.0f);  // Reset

    // Test category volume
    mgr.SetCategoryVolume(SoundCategory::COMBAT, 0.7f);
    TEST_ASSERT(std::abs(mgr.GetCategoryVolume(SoundCategory::COMBAT) - 0.7f) < 0.01f,
                "Combat volume should be 0.7");

    mgr.SetCategoryVolume(SoundCategory::COMBAT, 1.0f);  // Reset

    return true;
}

bool Test_Muting() {
    SoundManager& mgr = SoundManager::Instance();

    if (!mgr.IsInitialized()) {
        mgr.Initialize();
    }

    TEST_ASSERT(!mgr.IsMuted(), "Should not be muted initially");

    mgr.SetMuted(true);
    TEST_ASSERT(mgr.IsMuted(), "Should be muted");

    mgr.SetMuted(false);
    TEST_ASSERT(!mgr.IsMuted(), "Should not be muted after unmute");

    return true;
}

bool Test_GlobalFunctions() {
    // Test the C-style global functions

    // Initialize through global function
    // (May already be initialized from previous tests)

    Sound_SetVolume(128);  // 50%
    int vol = Sound_GetVolume();
    TEST_ASSERT(vol >= 120 && vol <= 136, "Volume should be ~128");

    Sound_SetVolume(255);  // 100%
    vol = Sound_GetVolume();
    TEST_ASSERT(vol >= 250, "Volume should be ~255");

    return true;
}

//=============================================================================
// Integration Tests (require game assets)
//=============================================================================

bool Test_LoadSounds() {
    printf("\n--- Integration Test: Load Sounds ---\n");

    // Try to load MIX files if not already loaded
    MixManager& mix = MixManager::Instance();
    if (!mix.HasAnyMixLoaded()) {
        mix.LoadMix("SOUNDS.MIX");
    }

    SoundManager& mgr = SoundManager::Instance();

    // Reinitialize to reload sounds
    mgr.Shutdown();
    mgr.Initialize();

    int loaded = mgr.GetLoadedSoundCount();
    printf("  Loaded %d sounds\n", loaded);

    if (loaded > 0) {
        // Check some specific sounds
        if (mgr.IsSoundLoaded(SoundEffect::UI_CLICK)) {
            printf("  ✓ UI_CLICK loaded\n");
        }
        if (mgr.IsSoundLoaded(SoundEffect::EXPLODE_SMALL)) {
            printf("  ✓ EXPLODE_SMALL loaded\n");
        }
        if (mgr.IsSoundLoaded(SoundEffect::WEAPON_CANNON)) {
            printf("  ✓ WEAPON_CANNON loaded\n");
        }
    } else {
        printf("  SKIPPED: No sounds loaded (game assets may not be present)\n");
    }

    return true;  // Not a failure if no assets
}

//=============================================================================
// Interactive Test
//=============================================================================

void Interactive_Test() {
    printf("\n=== Interactive Sound Manager Test ===\n");
    printf("Controls:\n");
    printf("  1-5: Play different sound effects\n");
    printf("  +/-: Adjust volume\n");
    printf("  M: Toggle mute\n");
    printf("  Arrow keys: Move listener position\n");
    printf("  ESC: Exit\n\n");

    // Initialize platform
    Platform_Init();

    AudioConfig audio_config = {};
    audio_config.sample_rate = 22050;
    audio_config.channels = 2;
    audio_config.bits_per_sample = 16;
    audio_config.buffer_size = 1024;

    if (Platform_Audio_Init(&audio_config) != 0) {
        printf("Failed to initialize audio\n");
        Platform_Shutdown();
        return;
    }

    // Initialize viewport
    Viewport::Instance().Initialize();
    Viewport::Instance().SetMapSize(128, 128);

    // Load MIX files
    MixManager::Instance().Initialize();
    MixManager::Instance().LoadMix("SOUNDS.MIX");

    // Initialize sound manager
    Sound_Init();

    SoundManager& mgr = SoundManager::Instance();
    mgr.PrintStats();

    int listener_x = 1000;
    int listener_y = 1000;
    mgr.SetListenerPosition(listener_x, listener_y);

    bool running = true;
    while (running) {
        Platform_PumpEvents();
        Platform_Input_Update();

        // Check for exit
        if (Platform_Key_JustPressed(KEY_CODE_ESCAPE)) {
            running = false;
            continue;
        }

        // Play sounds with number keys
        if (Platform_Key_JustPressed(KEY_CODE_1)) {
            printf("Playing UI_CLICK\n");
            mgr.Play(SoundEffect::UI_CLICK);
        }
        if (Platform_Key_JustPressed(KEY_CODE_2)) {
            printf("Playing EXPLODE_SMALL\n");
            mgr.Play(SoundEffect::EXPLODE_SMALL);
        }
        if (Platform_Key_JustPressed(KEY_CODE_3)) {
            printf("Playing WEAPON_CANNON\n");
            mgr.Play(SoundEffect::WEAPON_CANNON);
        }
        if (Platform_Key_JustPressed(KEY_CODE_4)) {
            printf("Playing positional sound at (2000, 2000)\n");
            mgr.PlayAt(SoundEffect::EXPLODE_LARGE, 2000, 2000);
        }
        if (Platform_Key_JustPressed(KEY_CODE_5)) {
            printf("Playing positional sound at listener\n");
            mgr.PlayAt(SoundEffect::EXPLODE_LARGE, listener_x, listener_y);
        }

        // Volume control
        if (Platform_Key_JustPressed(KEY_CODE_EQUALS) || Platform_Key_JustPressed(KEY_CODE_PLUS)) {
            float vol = mgr.GetVolume() + 0.1f;
            mgr.SetVolume(vol);
            printf("Volume: %.0f%%\n", mgr.GetVolume() * 100);
        }
        if (Platform_Key_JustPressed(KEY_CODE_MINUS)) {
            float vol = mgr.GetVolume() - 0.1f;
            mgr.SetVolume(vol);
            printf("Volume: %.0f%%\n", mgr.GetVolume() * 100);
        }

        // Mute toggle
        if (Platform_Key_JustPressed(KEY_CODE_M)) {
            mgr.SetMuted(!mgr.IsMuted());
            printf("Muted: %s\n", mgr.IsMuted() ? "yes" : "no");
        }

        // Move listener
        const int MOVE_SPEED = 50;
        if (Platform_Key_IsPressed(KEY_CODE_UP)) {
            listener_y -= MOVE_SPEED;
        }
        if (Platform_Key_IsPressed(KEY_CODE_DOWN)) {
            listener_y += MOVE_SPEED;
        }
        if (Platform_Key_IsPressed(KEY_CODE_LEFT)) {
            listener_x -= MOVE_SPEED;
        }
        if (Platform_Key_IsPressed(KEY_CODE_RIGHT)) {
            listener_x += MOVE_SPEED;
        }
        mgr.SetListenerPosition(listener_x, listener_y);

        // Update sound manager
        Sound_Update();

        Platform_Timer_Delay(16);
    }

    Sound_Shutdown();
    MixManager::Instance().Shutdown();
    Platform_Audio_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Main
//=============================================================================

int main(int argc, char* argv[]) {
    printf("=== Sound Manager Tests (Task 17b) ===\n\n");

    // Initialize platform for tests
    Platform_Init();

    AudioConfig audio_config = {};
    audio_config.sample_rate = 22050;
    audio_config.channels = 2;
    audio_config.bits_per_sample = 16;
    audio_config.buffer_size = 1024;
    Platform_Audio_Init(&audio_config);

    MixManager::Instance().Initialize();

    // Run unit tests
    RUN_TEST(SoundEffectEnum);
    RUN_TEST(SoundInfo);
    RUN_TEST(SoundFilename);
    RUN_TEST(DistanceAttenuation);
    RUN_TEST(VolumeControl);
    RUN_TEST(Muting);
    RUN_TEST(GlobalFunctions);

    // Integration tests
    RUN_TEST(LoadSounds);

    // Cleanup
    SoundManager::Instance().Shutdown();
    MixManager::Instance().Shutdown();
    Platform_Audio_Shutdown();
    Platform_Shutdown();

    printf("\n=== Results: %d passed, %d failed ===\n", tests_passed, tests_failed);

    // Interactive test
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
# =============================================================================
# Audio Integration - Task 17b: Sound Manager
# =============================================================================

# Sound manager sources (depends on Task 17a)
set(AUDIO_SOUND_MANAGER_SOURCES
    ${CMAKE_SOURCE_DIR}/src/game/audio/sound_manager.cpp
)

set(AUDIO_SOUND_MANAGER_HEADERS
    ${CMAKE_SOURCE_DIR}/include/game/audio/sound_effect.h
    ${CMAKE_SOURCE_DIR}/include/game/audio/sound_manager.h
)

# Test executable
add_executable(TestSoundManager
    ${CMAKE_SOURCE_DIR}/src/test_sound_manager.cpp
    ${AUDIO_AUD_SOURCES}           # From Task 17a
    ${AUDIO_SOUND_MANAGER_SOURCES}
)

target_include_directories(TestSoundManager PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(TestSoundManager PRIVATE
    redalert_platform
    # mix_manager / asset library
)

add_test(NAME SoundManagerTests COMMAND TestSoundManager)
```

---

## Verification Script

```bash
#!/bin/bash
# Verification script for Task 17b: Sound Manager
set -e

echo "=== Task 17b Verification: Sound Manager ==="
echo ""

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"

# Step 1: Check source files exist
echo "Step 1: Checking source files..."

FILES=(
    "include/game/audio/sound_effect.h"
    "include/game/audio/sound_manager.h"
    "src/game/audio/sound_manager.cpp"
    "src/test_sound_manager.cpp"
)

for file in "${FILES[@]}"; do
    if [ ! -f "$ROOT_DIR/$file" ]; then
        echo "VERIFY_FAILED: Missing file $file"
        exit 1
    fi
    echo "  ✓ $file"
done

# Step 2: Build
echo ""
echo "Step 2: Building..."

cd "$BUILD_DIR"
cmake --build . --target TestSoundManager

if [ ! -f "$BUILD_DIR/TestSoundManager" ]; then
    echo "VERIFY_FAILED: TestSoundManager not built"
    exit 1
fi

echo "  ✓ TestSoundManager built"

# Step 3: Run tests
echo ""
echo "Step 3: Running tests..."

if ! "$BUILD_DIR/TestSoundManager"; then
    echo "VERIFY_FAILED: Tests failed"
    exit 1
fi

echo "  ✓ All tests passed"

# Step 4: Verify enum completeness
echo ""
echo "Step 4: Checking SoundEffect enum..."

SOUND_COUNT=$(grep -c "SoundEffect::" "$ROOT_DIR/include/game/audio/sound_effect.h" || echo "0")
if [ "$SOUND_COUNT" -lt 50 ]; then
    echo "VERIFY_FAILED: SoundEffect enum seems incomplete ($SOUND_COUNT entries)"
    exit 1
fi

echo "  ✓ $SOUND_COUNT sound effects defined"

# Step 5: Verify sound info table
echo ""
echo "Step 5: Checking SOUND_INFO table..."

if ! grep -q "SOUND_INFO\[\]" "$ROOT_DIR/src/game/audio/sound_manager.cpp"; then
    echo "VERIFY_FAILED: SOUND_INFO table not found"
    exit 1
fi

echo "  ✓ SOUND_INFO table present"

echo ""
echo "=== Task 17b Verification PASSED ==="
```

---

## Implementation Steps

1. **Create sound_effect.h** - Define SoundEffect enum with all game sounds
2. **Create sound_manager.h** - SoundManager class with all method declarations
3. **Create sound_manager.cpp** - Full implementation including:
   - SOUND_INFO table mapping enum to filenames
   - Sound loading via AudFile
   - Positional audio calculation
   - Rate limiting and concurrent sound management
4. **Create test_sound_manager.cpp** - Unit and integration tests
5. **Update CMakeLists.txt** - Add build targets
6. **Build and run tests**
7. **Test with actual game assets** (if available)

---

## Success Criteria

- [ ] SoundEffect enum defines 50+ sounds matching original game
- [ ] SOUND_INFO table maps all sounds to AUD filenames
- [ ] Sounds load from MIX archives via AudFile
- [ ] Non-positional Play() works correctly
- [ ] Positional PlayAt() attenuates volume with distance
- [ ] Volume controls (master, category) work
- [ ] Muting stops all sounds
- [ ] Rate limiting prevents sound spam
- [ ] Concurrent sound limit enforced
- [ ] Listener position updates from viewport
- [ ] All unit tests pass
- [ ] Interactive test plays sounds correctly

---

## Common Issues

1. **Sounds don't play**
   - Check AudFile loaded successfully
   - Verify platform audio initialized
   - Check volume isn't 0 or muted

2. **Positional audio not working**
   - Verify listener position is set
   - Check max_distance isn't too small
   - Ensure world coordinates are correct

3. **Sound spam / flooding**
   - Verify rate limiting in CanPlaySound()
   - Check max_concurrent limits
   - Ensure cleanup runs each frame

4. **Missing sounds**
   - Verify filename in SOUND_INFO matches actual AUD file
   - Check case sensitivity
   - Ensure MIX file is loaded

---

## Completion Promise

When verification passes, output:
<promise>TASK_17B_COMPLETE</promise>

## Escape Hatch

If stuck after 15 iterations:
- Document what's blocking in `ralph-tasks/blocked/17b.md`
- List attempted approaches
- Output:
<promise>TASK_17B_BLOCKED</promise>

## Max Iterations
15
