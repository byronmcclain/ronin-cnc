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
