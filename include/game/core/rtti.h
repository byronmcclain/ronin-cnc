/**
 * RTTI - Runtime Type Identification
 *
 * Custom RTTI system for game objects. Faster than C++ RTTI
 * for the frequent type checks done in game logic.
 *
 * Original location: CODE/DEFINES.H
 */

#pragma once

#include <cstdint>

// =============================================================================
// RTTI Types
// =============================================================================

/**
 * RTTIType - Object type identifiers
 *
 * Used for fast type checking without C++ dynamic_cast.
 * Values are ordered so range checks can identify type categories.
 */
enum RTTIType : int8_t {
    RTTI_NONE = -1,

    // Military units (Is_Techno range start)
    RTTI_UNIT = 0,          // Ground vehicle
    RTTI_INFANTRY = 1,      // Infantry soldier
    RTTI_VESSEL = 2,        // Naval vessel
    RTTI_AIRCRAFT = 3,      // Aircraft
    RTTI_BUILDING = 4,      // Structure
    // (Is_Techno range end)

    // Non-military objects
    RTTI_BULLET = 5,        // Projectile
    RTTI_ANIM = 6,          // Animation
    RTTI_TERRAIN = 7,       // Terrain object (tree, etc.)
    RTTI_OVERLAY = 8,       // Overlay (tiberium, wall)
    RTTI_SMUDGE = 9,        // Smudge (crater, scorch)
    RTTI_TRIGGER = 10,      // Map trigger

    // Type classes (not actual objects)
    RTTI_UNITTYPE = 11,
    RTTI_INFANTRYTYPE = 12,
    RTTI_VESSELTYPE = 13,
    RTTI_AIRCRAFTTYPE = 14,
    RTTI_BUILDINGTYPE = 15,
    RTTI_BULLETTYPE = 16,
    RTTI_ANIMTYPE = 17,
    RTTI_TERRAINTYPE = 18,
    RTTI_OVERLAYTYPE = 19,

    RTTI_COUNT = 20
};

// =============================================================================
// RTTI Names (for debugging)
// =============================================================================

extern const char* RTTINames[RTTI_COUNT];
