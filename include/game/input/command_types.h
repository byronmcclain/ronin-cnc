// include/game/input/command_types.h
// Command type definitions
// Task 16f - Command System

#ifndef COMMAND_TYPES_H
#define COMMAND_TYPES_H

#include <cstdint>

//=============================================================================
// Command Type Enumeration
//=============================================================================

enum class CommandType {
    NONE = 0,

    // Movement
    MOVE,               // Move to location
    ATTACK_MOVE,        // Move, attack anything along the way

    // Combat
    ATTACK,             // Attack specific target
    FORCE_FIRE,         // Attack ground/force attack
    GUARD,              // Guard area or unit
    STOP,               // Stop all actions

    // Unit interactions
    ENTER,              // Enter transport/building
    DEPLOY,             // Deploy (MCV, etc.)
    UNLOAD,             // Unload passengers

    // Resource
    HARVEST,            // Harvest at location

    // Building
    REPAIR,             // Request repair
    SELL,               // Sell structure

    // Formation/Group
    SCATTER,            // Scatter/spread out
    FOLLOW,             // Follow target unit

    // Special
    PATROL,             // Patrol between points
    CAPTURE,            // Capture building (engineer)
    SABOTAGE,           // Sabotage building (spy)
    INFILTRATE,         // Infiltrate building (spy/thief)

    // Super weapons
    AIRSTRIKE,          // Call airstrike at location
    NUKE,               // Launch nuke at location
    CHRONOSPHERE,       // Chronosphere teleport

    COUNT
};

//=============================================================================
// Command Flags
//=============================================================================

enum CommandFlags : uint32_t {
    CMD_FLAG_NONE       = 0,
    CMD_FLAG_QUEUED     = (1 << 0),  // Add to queue, don't replace
    CMD_FLAG_FORCED     = (1 << 1),  // Force action (Ctrl modifier)
    CMD_FLAG_ALT        = (1 << 2),  // Alt modifier (force move)
};

//=============================================================================
// Command Target
//=============================================================================

struct CommandTarget {
    enum class Type {
        NONE,
        GROUND,         // World coordinate
        CELL,           // Cell coordinate
        OBJECT,         // Specific object
    };

    Type type;

    // Ground/cell target
    int world_x;
    int world_y;
    int cell_x;
    int cell_y;

    // Object target
    void* object;       // ObjectClass* when integrated
    uint32_t object_id;

    void Clear() {
        type = Type::NONE;
        world_x = world_y = 0;
        cell_x = cell_y = 0;
        object = nullptr;
        object_id = 0;
    }

    void SetGround(int wx, int wy) {
        type = Type::GROUND;
        world_x = wx;
        world_y = wy;
        cell_x = wx / 24;
        cell_y = wy / 24;
        object = nullptr;
        object_id = 0;
    }

    void SetCell(int cx, int cy) {
        type = Type::CELL;
        cell_x = cx;
        cell_y = cy;
        world_x = cx * 24 + 12;
        world_y = cy * 24 + 12;
        object = nullptr;
        object_id = 0;
    }

    void SetObject(void* obj, uint32_t id) {
        type = Type::OBJECT;
        object = obj;
        object_id = id;
        // World position would be filled from object
    }
};

//=============================================================================
// Command Structure
//=============================================================================

struct Command {
    CommandType type;
    CommandTarget target;
    uint32_t flags;

    // Source information
    int source_count;       // How many units received this command

    void Clear() {
        type = CommandType::NONE;
        target.Clear();
        flags = CMD_FLAG_NONE;
        source_count = 0;
    }

    bool IsQueued() const { return (flags & CMD_FLAG_QUEUED) != 0; }
    bool IsForced() const { return (flags & CMD_FLAG_FORCED) != 0; }
};

//=============================================================================
// Mission Type (game integration)
//=============================================================================

// These match the original game's mission types
enum MissionType {
    MISSION_NONE = 0,
    MISSION_SLEEP,
    MISSION_ATTACK,
    MISSION_MOVE,
    MISSION_QMOVE,
    MISSION_RETREAT,
    MISSION_GUARD,
    MISSION_STICKY,
    MISSION_ENTER,
    MISSION_CAPTURE,
    MISSION_HARVEST,
    MISSION_GUARD_AREA,
    MISSION_RETURN,
    MISSION_STOP,
    MISSION_AMBUSH,
    MISSION_HUNT,
    MISSION_UNLOAD,
    MISSION_SABOTAGE,
    MISSION_CONSTRUCTION,
    MISSION_DECONSTRUCTION,
    MISSION_REPAIR,
    MISSION_RESCUE,
    MISSION_MISSILE,
    MISSION_HARMLESS,
    MISSION_COUNT
};

// Convert command type to mission type
MissionType CommandToMission(CommandType cmd);

//=============================================================================
// Command Result
//=============================================================================

enum class CommandResult {
    SUCCESS,            // Command accepted
    INVALID_TARGET,     // Target not valid for command
    INVALID_SELECTION,  // No valid units selected
    CANNOT_PERFORM,     // Units cannot perform this command
    OUT_OF_RANGE,       // Target out of range (super weapons)
    BLOCKED,            // Path blocked or inaccessible
};

#endif // COMMAND_TYPES_H
