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
