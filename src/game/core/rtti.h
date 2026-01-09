/**
 * Runtime Type Identification
 *
 * RTTI system for game objects. Allows safe casting and type checking.
 * Matches original Westwood implementation.
 */

#ifndef GAME_CORE_RTTI_H
#define GAME_CORE_RTTI_H

#include <cstdint>

/**
 * RTTIType - Runtime type identifier for all game objects
 */
enum RTTIType : int8_t {
    RTTI_NONE = 0,

    // Abstract types
    RTTI_ABSTRACT,
    RTTI_OBJECT,
    RTTI_MISSION,
    RTTI_RADIO,
    RTTI_TECHNO,
    RTTI_FOOT,

    // Concrete object types
    RTTI_UNIT,
    RTTI_INFANTRY,
    RTTI_BUILDING,
    RTTI_AIRCRAFT,
    RTTI_VESSEL,
    RTTI_BULLET,
    RTTI_ANIM,
    RTTI_TERRAIN,
    RTTI_OVERLAY,
    RTTI_SMUDGE,
    RTTI_TEMPLATE,

    // Type class types (for editor/data)
    RTTI_UNITTYPE,
    RTTI_INFANTRYTYPE,
    RTTI_BUILDINGTYPE,
    RTTI_AIRCRAFTTYPE,
    RTTI_VESSELTYPE,
    RTTI_BULLETTYPE,
    RTTI_ANIMTYPE,
    RTTI_TERRAINTYPE,
    RTTI_OVERLAYTYPE,
    RTTI_SMUDGETYPE,
    RTTI_TEMPLATETYPE,

    // Special types
    RTTI_HOUSE,
    RTTI_TRIGGER,
    RTTI_TEAM,
    RTTI_TEAMTYPE,

    RTTI_COUNT
};

/**
 * Get RTTI name for debugging
 */
inline const char* RTTI_Name(RTTIType rtti) {
    static const char* names[] = {
        "None",
        "Abstract", "Object", "Mission", "Radio", "Techno", "Foot",
        "Unit", "Infantry", "Building", "Aircraft", "Vessel",
        "Bullet", "Anim", "Terrain", "Overlay", "Smudge", "Template",
        "UnitType", "InfantryType", "BuildingType", "AircraftType", "VesselType",
        "BulletType", "AnimType", "TerrainType", "OverlayType", "SmudgeType", "TemplateType",
        "House", "Trigger", "Team", "TeamType"
    };
    if (rtti >= 0 && rtti < RTTI_COUNT) {
        return names[rtti];
    }
    return "Unknown";
}

/**
 * Check if RTTI is a mobile object (can move)
 */
inline bool RTTI_Is_Foot(RTTIType rtti) {
    return rtti == RTTI_UNIT ||
           rtti == RTTI_INFANTRY ||
           rtti == RTTI_AIRCRAFT ||
           rtti == RTTI_VESSEL;
}

/**
 * Check if RTTI is a techno object (has weapons, can be owned)
 */
inline bool RTTI_Is_Techno(RTTIType rtti) {
    return rtti == RTTI_UNIT ||
           rtti == RTTI_INFANTRY ||
           rtti == RTTI_AIRCRAFT ||
           rtti == RTTI_VESSEL ||
           rtti == RTTI_BUILDING;
}

/**
 * Check if RTTI is a selectable object
 */
inline bool RTTI_Is_Selectable(RTTIType rtti) {
    return RTTI_Is_Techno(rtti);
}

#endif // GAME_CORE_RTTI_H
