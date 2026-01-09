/**
 * Mission System
 *
 * Defines the mission/order types that units can execute.
 * Missions control high-level AI behavior.
 *
 * Original location: CODE/MISSION.H
 */

#pragma once

#include <cstdint>

// =============================================================================
// Mission Types
// =============================================================================

/**
 * MissionType - Unit AI orders
 *
 * Each unit has a current mission that drives its behavior.
 */
enum MissionType : int8_t {
    MISSION_NONE = -1,

    MISSION_SLEEP = 0,      // Do nothing
    MISSION_ATTACK = 1,     // Attack target
    MISSION_MOVE = 2,       // Move to destination
    MISSION_QMOVE = 3,      // Queue move (shift-click)
    MISSION_RETREAT = 4,    // Flee from combat
    MISSION_GUARD = 5,      // Guard position
    MISSION_STICKY = 6,     // Don't auto-acquire targets
    MISSION_ENTER = 7,      // Enter transport/building
    MISSION_CAPTURE = 8,    // Capture building (engineer)
    MISSION_HARVEST = 9,    // Harvest tiberium
    MISSION_GUARD_AREA = 10,// Guard area (patrol)
    MISSION_RETURN = 11,    // Return to base (harvester)
    MISSION_STOP = 12,      // Stop current action
    MISSION_AMBUSH = 13,    // Ambush (stay hidden)
    MISSION_HUNT = 14,      // Seek and destroy
    MISSION_UNLOAD = 15,    // Unload passengers
    MISSION_SABOTAGE = 16,  // Sabotage (spy/thief)
    MISSION_CONSTRUCTION = 17,// Building construction
    MISSION_DECONSTRUCTION = 18,// Building selling
    MISSION_REPAIR = 19,    // Repair building
    MISSION_RESCUE = 20,    // Rescue (medic)
    MISSION_MISSILE = 21,   // Missile launch

    MISSION_COUNT = 22
};

// =============================================================================
// Mission Names
// =============================================================================

extern const char* MissionNames[MISSION_COUNT];

// =============================================================================
// Mission Utility Functions
// =============================================================================

/**
 * Get mission from name string
 */
MissionType Mission_From_Name(const char* name);

/**
 * Get mission name string
 */
const char* Mission_Name(MissionType mission);

/**
 * Is this an attack mission?
 */
inline bool Mission_Is_Attack(MissionType m) {
    return (m == MISSION_ATTACK || m == MISSION_HUNT);
}

/**
 * Is this a movement mission?
 */
inline bool Mission_Is_Move(MissionType m) {
    return (m == MISSION_MOVE || m == MISSION_QMOVE ||
            m == MISSION_ENTER || m == MISSION_RETURN);
}
