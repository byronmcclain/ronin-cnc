# Task 16f: Command System

| Field | Value |
|-------|-------|
| **Task ID** | 16f |
| **Task Name** | Command System |
| **Phase** | 16 - Input Integration |
| **Depends On** | 16e (Selection System), 16d (Mouse Handler) |
| **Estimated Complexity** | Medium-High (~500 lines) |

---

## Dependencies

- Task 16e (Selection System) must be complete
- Task 16d (Mouse Handler) must be complete
- Selection and cursor context information available

---

## Context

The command system translates player input into unit orders. When a player right-clicks (or uses hotkeys), the system determines what command to issue based on:

1. **Current Selection**: What units are selected
2. **Target**: What the mouse is pointing at
3. **Modifiers**: Force-fire (Ctrl), force-move (Alt), etc.

The original Red Alert used context-sensitive commands - right-clicking on empty ground moves, on enemies attacks, on friendly transports enters, etc. Our system replicates this behavior while providing a clean interface for game integration.

---

## Objective

Create a command system that:
1. Defines all command types (Move, Attack, Guard, etc.)
2. Determines appropriate command from mouse context
3. Issues commands to selected units
4. Handles force-fire and force-move modifiers
5. Supports command queueing with Shift

---

## Technical Background

### Command Resolution

```
Right-Click Resolution (with units selected):
1. Check force modifiers:
   - Ctrl+Click = Force attack (even ground)
   - Alt+Click = Force move (ignore targets)
2. Check target:
   - Empty passable terrain → Move
   - Enemy unit/building → Attack
   - Own transport → Enter
   - Own building (repair bay) → Enter
   - Ore/tiberium → Harvest (if harvester)
   - Friendly unit → Guard/Follow
3. Queue if Shift held

Command Types:
- MOVE: Move to location
- ATTACK: Attack target (unit/building/ground)
- GUARD: Guard location or unit
- STOP: Halt current action
- ENTER: Enter transport/building
- HARVEST: Harvest resources
- REPAIR: Go to repair bay
- SELL: Sell building (immediate)
- SCATTER: Spread out (X key)
- DEPLOY: Deploy unit (MCV, etc.)
```

### Mission System Integration

Commands map to the game's mission system:
- `MISSION_MOVE` - Moving to destination
- `MISSION_ATTACK` - Attacking target
- `MISSION_GUARD` - Guarding area
- `MISSION_HARVEST` - Harvesting resources
- etc.

---

## Deliverables

- [ ] `src/game/input/command_types.h` - Command type definitions
- [ ] `src/game/input/command_system.h` - Command system declaration
- [ ] `src/game/input/command_system.cpp` - Command system implementation
- [ ] `tests/test_commands.cpp` - Test program
- [ ] CMakeLists.txt updated
- [ ] All tests pass

---

## Files to Create

### src/game/input/command_types.h

```cpp
// src/game/input/command_types.h
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
```

### src/game/input/command_system.h

```cpp
// src/game/input/command_system.h
// Command system for issuing orders to units
// Task 16f - Command System

#ifndef COMMAND_SYSTEM_H
#define COMMAND_SYSTEM_H

#include "command_types.h"
#include "cursor_context.h"
#include "selection_manager.h"
#include <functional>
#include <vector>

//=============================================================================
// Command System
//=============================================================================

class CommandSystem {
public:
    // Singleton access
    static CommandSystem& Instance();

    // Lifecycle
    bool Initialize();
    void Shutdown();

    //=========================================================================
    // Command Resolution
    //=========================================================================

    // Determine what command a right-click would issue
    Command ResolveCommand(const CursorContext& context, bool shift_held,
                          bool ctrl_held, bool alt_held);

    // Get command that would be issued at current mouse position
    Command GetPendingCommand();

    //=========================================================================
    // Command Execution
    //=========================================================================

    // Issue command to all selected units
    CommandResult IssueCommand(const Command& cmd);

    // Issue specific command types
    CommandResult IssueMoveCommand(int world_x, int world_y, bool queued = false);
    CommandResult IssueAttackCommand(void* target, bool queued = false);
    CommandResult IssueAttackGroundCommand(int world_x, int world_y, bool queued = false);
    CommandResult IssueGuardCommand(bool queued = false);
    CommandResult IssueStopCommand();
    CommandResult IssueScatterCommand();
    CommandResult IssueEnterCommand(void* transport);
    CommandResult IssueDeployCommand();
    CommandResult IssueHarvestCommand(int cell_x, int cell_y);
    CommandResult IssueSellCommand(void* building);
    CommandResult IssueRepairCommand(void* building);

    //=========================================================================
    // Callbacks for Game Integration
    //=========================================================================

    // Called to actually assign mission to a unit
    using AssignMissionFunc = std::function<bool(
        void* unit, MissionType mission, const CommandTarget& target)>;

    void SetAssignMissionCallback(AssignMissionFunc func) { assign_mission_ = func; }

    // Query if unit can perform action
    using CanPerformFunc = std::function<bool(void* unit, CommandType cmd)>;
    void SetCanPerformQuery(CanPerformFunc func) { can_perform_ = func; }

    // Get object information
    using IsEnemyFunc = std::function<bool(void* obj)>;
    using IsOwnFunc = std::function<bool(void* obj)>;
    using IsTransportFunc = std::function<bool(void* obj)>;
    using IsBuildingFunc = std::function<bool(void* obj)>;

    void SetIsEnemyQuery(IsEnemyFunc func) { is_enemy_ = func; }
    void SetIsOwnQuery(IsOwnFunc func) { is_own_ = func; }
    void SetIsTransportQuery(IsTransportFunc func) { is_transport_ = func; }
    void SetIsBuildingQuery(IsBuildingFunc func) { is_building_ = func; }

    //=========================================================================
    // Command Feedback
    //=========================================================================

    // Called when command is issued (for sound effects, etc.)
    using CommandFeedbackFunc = std::function<void(CommandType cmd, CommandResult result)>;
    void SetCommandFeedback(CommandFeedbackFunc func) { on_command_ = func; }

    // Get last issued command
    const Command& GetLastCommand() const { return last_command_; }
    CommandResult GetLastResult() const { return last_result_; }

private:
    CommandSystem();
    ~CommandSystem();
    CommandSystem(const CommandSystem&) = delete;
    CommandSystem& operator=(const CommandSystem&) = delete;

    CommandType DetermineCommandForContext(const CursorContext& context,
                                           bool force_attack, bool force_move);

    CommandResult ExecuteOnSelection(const Command& cmd);

    bool initialized_;

    // Last command info
    Command last_command_;
    CommandResult last_result_;

    // Callbacks
    AssignMissionFunc assign_mission_;
    CanPerformFunc can_perform_;
    IsEnemyFunc is_enemy_;
    IsOwnFunc is_own_;
    IsTransportFunc is_transport_;
    IsBuildingFunc is_building_;
    CommandFeedbackFunc on_command_;
};

//=============================================================================
// Global Functions (C-compatible interface)
//=============================================================================

bool CommandSystem_Init();
void CommandSystem_Shutdown();

// Process right-click at mouse position
void Command_ProcessRightClick(int world_x, int world_y, void* target,
                               bool shift, bool ctrl, bool alt);

// Issue specific commands
void Command_Move(int world_x, int world_y, bool queued);
void Command_Attack(void* target, bool queued);
void Command_Stop();
void Command_Guard();
void Command_Scatter();

#endif // COMMAND_SYSTEM_H
```

### src/game/input/command_system.cpp

```cpp
// src/game/input/command_system.cpp
// Command system implementation
// Task 16f - Command System

#include "command_system.h"
#include "selection_manager.h"
#include "platform.h"
#include <algorithm>

//=============================================================================
// CommandToMission
//=============================================================================

MissionType CommandToMission(CommandType cmd) {
    switch (cmd) {
        case CommandType::MOVE:
        case CommandType::ATTACK_MOVE:
            return MISSION_MOVE;

        case CommandType::ATTACK:
        case CommandType::FORCE_FIRE:
            return MISSION_ATTACK;

        case CommandType::GUARD:
            return MISSION_GUARD;

        case CommandType::STOP:
            return MISSION_STOP;

        case CommandType::ENTER:
            return MISSION_ENTER;

        case CommandType::HARVEST:
            return MISSION_HARVEST;

        case CommandType::CAPTURE:
            return MISSION_CAPTURE;

        case CommandType::SABOTAGE:
            return MISSION_SABOTAGE;

        case CommandType::UNLOAD:
            return MISSION_UNLOAD;

        case CommandType::REPAIR:
            return MISSION_REPAIR;

        default:
            return MISSION_NONE;
    }
}

//=============================================================================
// CommandSystem Singleton
//=============================================================================

CommandSystem& CommandSystem::Instance() {
    static CommandSystem instance;
    return instance;
}

CommandSystem::CommandSystem()
    : initialized_(false)
    , last_result_(CommandResult::SUCCESS)
    , assign_mission_(nullptr)
    , can_perform_(nullptr)
    , is_enemy_(nullptr)
    , is_own_(nullptr)
    , is_transport_(nullptr)
    , is_building_(nullptr)
    , on_command_(nullptr) {
    last_command_.Clear();
}

CommandSystem::~CommandSystem() {
    Shutdown();
}

bool CommandSystem::Initialize() {
    if (initialized_) return true;

    Platform_Log("CommandSystem: Initializing...");

    last_command_.Clear();
    last_result_ = CommandResult::SUCCESS;

    initialized_ = true;
    Platform_Log("CommandSystem: Initialized");
    return true;
}

void CommandSystem::Shutdown() {
    if (!initialized_) return;

    Platform_Log("CommandSystem: Shutting down");
    initialized_ = false;
}

//=============================================================================
// Command Resolution
//=============================================================================

Command CommandSystem::ResolveCommand(const CursorContext& context,
                                       bool shift_held, bool ctrl_held, bool alt_held) {
    Command cmd;
    cmd.Clear();

    // Set flags
    if (shift_held) cmd.flags |= CMD_FLAG_QUEUED;
    if (ctrl_held) cmd.flags |= CMD_FLAG_FORCED;
    if (alt_held) cmd.flags |= CMD_FLAG_ALT;

    // Determine command type based on context and modifiers
    cmd.type = DetermineCommandForContext(context, ctrl_held, alt_held);

    // Set target
    if (context.object) {
        cmd.target.SetObject(context.object, context.object_id);
        cmd.target.world_x = context.world_x;
        cmd.target.world_y = context.world_y;
        cmd.target.cell_x = context.cell_x;
        cmd.target.cell_y = context.cell_y;
    } else {
        cmd.target.SetGround(context.world_x, context.world_y);
    }

    return cmd;
}

CommandType CommandSystem::DetermineCommandForContext(const CursorContext& context,
                                                       bool force_attack, bool force_move) {
    // Force move overrides everything
    if (force_move) {
        return CommandType::MOVE;
    }

    // Force attack
    if (force_attack) {
        if (context.object) {
            return CommandType::ATTACK;
        } else {
            return CommandType::FORCE_FIRE;  // Attack ground
        }
    }

    // No object - move to location
    if (!context.object) {
        if (context.is_passable) {
            return CommandType::MOVE;
        } else {
            return CommandType::NONE;  // Can't move there
        }
    }

    // Check target type
    if (context.is_enemy && context.is_attackable) {
        return CommandType::ATTACK;
    }

    if (context.is_own) {
        // Check for special interactions
        if (is_transport_ && is_transport_(context.object)) {
            return CommandType::ENTER;
        }

        // Guard/follow friendly unit
        if (context.type == CursorContextType::OWN_UNIT) {
            return CommandType::GUARD;  // Or FOLLOW
        }

        // Interact with own building
        if (is_building_ && is_building_(context.object)) {
            return CommandType::ENTER;  // Service depot, etc.
        }
    }

    // Neutral buildings might be capturable
    if (context.type == CursorContextType::NEUTRAL_BUILDING) {
        return CommandType::CAPTURE;
    }

    // Resource location
    if (context.type == CursorContextType::HARVEST_AREA) {
        return CommandType::HARVEST;
    }

    // Default - if passable, move
    if (context.is_passable) {
        return CommandType::MOVE;
    }

    return CommandType::NONE;
}

Command CommandSystem::GetPendingCommand() {
    // This would use current mouse context
    // Placeholder - actual implementation would query MouseHandler
    Command cmd;
    cmd.Clear();
    return cmd;
}

//=============================================================================
// Command Execution
//=============================================================================

CommandResult CommandSystem::IssueCommand(const Command& cmd) {
    if (cmd.type == CommandType::NONE) {
        return CommandResult::INVALID_TARGET;
    }

    CommandResult result = ExecuteOnSelection(cmd);

    last_command_ = cmd;
    last_result_ = result;

    if (on_command_) {
        on_command_(cmd.type, result);
    }

    return result;
}

CommandResult CommandSystem::ExecuteOnSelection(const Command& cmd) {
    auto& selection = SelectionManager::Instance();

    if (!selection.HasSelection()) {
        return CommandResult::INVALID_SELECTION;
    }

    MissionType mission = CommandToMission(cmd.type);
    if (mission == MISSION_NONE) {
        Platform_Log("CommandSystem: No mission mapping for command %d",
                     static_cast<int>(cmd.type));
        return CommandResult::CANNOT_PERFORM;
    }

    int success_count = 0;
    int fail_count = 0;

    for (auto* obj : selection.GetSelection()) {
        if (!obj) continue;

        // Check if unit can perform this command
        if (can_perform_ && !can_perform_(obj, cmd.type)) {
            fail_count++;
            continue;
        }

        // Assign the mission
        if (assign_mission_) {
            if (assign_mission_(obj, mission, cmd.target)) {
                success_count++;
            } else {
                fail_count++;
            }
        } else {
            // No callback - assume success for testing
            success_count++;
        }
    }

    Platform_Log("CommandSystem: Command %d - %d success, %d failed",
                 static_cast<int>(cmd.type), success_count, fail_count);

    if (success_count == 0) {
        return CommandResult::CANNOT_PERFORM;
    }

    return CommandResult::SUCCESS;
}

//=============================================================================
// Convenience Command Methods
//=============================================================================

CommandResult CommandSystem::IssueMoveCommand(int world_x, int world_y, bool queued) {
    Command cmd;
    cmd.Clear();
    cmd.type = CommandType::MOVE;
    cmd.target.SetGround(world_x, world_y);
    if (queued) cmd.flags |= CMD_FLAG_QUEUED;

    return IssueCommand(cmd);
}

CommandResult CommandSystem::IssueAttackCommand(void* target, bool queued) {
    if (!target) return CommandResult::INVALID_TARGET;

    Command cmd;
    cmd.Clear();
    cmd.type = CommandType::ATTACK;
    cmd.target.SetObject(target, 0);  // ID would come from object
    if (queued) cmd.flags |= CMD_FLAG_QUEUED;

    return IssueCommand(cmd);
}

CommandResult CommandSystem::IssueAttackGroundCommand(int world_x, int world_y, bool queued) {
    Command cmd;
    cmd.Clear();
    cmd.type = CommandType::FORCE_FIRE;
    cmd.target.SetGround(world_x, world_y);
    cmd.flags |= CMD_FLAG_FORCED;
    if (queued) cmd.flags |= CMD_FLAG_QUEUED;

    return IssueCommand(cmd);
}

CommandResult CommandSystem::IssueGuardCommand(bool queued) {
    Command cmd;
    cmd.Clear();
    cmd.type = CommandType::GUARD;
    if (queued) cmd.flags |= CMD_FLAG_QUEUED;

    return IssueCommand(cmd);
}

CommandResult CommandSystem::IssueStopCommand() {
    Command cmd;
    cmd.Clear();
    cmd.type = CommandType::STOP;

    return IssueCommand(cmd);
}

CommandResult CommandSystem::IssueScatterCommand() {
    Command cmd;
    cmd.Clear();
    cmd.type = CommandType::SCATTER;

    return IssueCommand(cmd);
}

CommandResult CommandSystem::IssueEnterCommand(void* transport) {
    if (!transport) return CommandResult::INVALID_TARGET;

    Command cmd;
    cmd.Clear();
    cmd.type = CommandType::ENTER;
    cmd.target.SetObject(transport, 0);

    return IssueCommand(cmd);
}

CommandResult CommandSystem::IssueDeployCommand() {
    Command cmd;
    cmd.Clear();
    cmd.type = CommandType::DEPLOY;

    return IssueCommand(cmd);
}

CommandResult CommandSystem::IssueHarvestCommand(int cell_x, int cell_y) {
    Command cmd;
    cmd.Clear();
    cmd.type = CommandType::HARVEST;
    cmd.target.SetCell(cell_x, cell_y);

    return IssueCommand(cmd);
}

CommandResult CommandSystem::IssueSellCommand(void* building) {
    if (!building) return CommandResult::INVALID_TARGET;

    Command cmd;
    cmd.Clear();
    cmd.type = CommandType::SELL;
    cmd.target.SetObject(building, 0);

    return IssueCommand(cmd);
}

CommandResult CommandSystem::IssueRepairCommand(void* building) {
    if (!building) return CommandResult::INVALID_TARGET;

    Command cmd;
    cmd.Clear();
    cmd.type = CommandType::REPAIR;
    cmd.target.SetObject(building, 0);

    return IssueCommand(cmd);
}

//=============================================================================
// Global Functions
//=============================================================================

bool CommandSystem_Init() {
    return CommandSystem::Instance().Initialize();
}

void CommandSystem_Shutdown() {
    CommandSystem::Instance().Shutdown();
}

void Command_ProcessRightClick(int world_x, int world_y, void* target,
                               bool shift, bool ctrl, bool alt) {
    // Build cursor context from parameters
    CursorContext context;
    context.Clear();
    context.world_x = world_x;
    context.world_y = world_y;
    context.cell_x = world_x / 24;
    context.cell_y = world_y / 24;
    context.object = target;

    // Would need to fill in is_enemy, is_own, etc. from game state
    context.is_passable = true;  // Placeholder

    Command cmd = CommandSystem::Instance().ResolveCommand(context, shift, ctrl, alt);
    CommandSystem::Instance().IssueCommand(cmd);
}

void Command_Move(int world_x, int world_y, bool queued) {
    CommandSystem::Instance().IssueMoveCommand(world_x, world_y, queued);
}

void Command_Attack(void* target, bool queued) {
    CommandSystem::Instance().IssueAttackCommand(target, queued);
}

void Command_Stop() {
    CommandSystem::Instance().IssueStopCommand();
}

void Command_Guard() {
    CommandSystem::Instance().IssueGuardCommand(false);
}

void Command_Scatter() {
    CommandSystem::Instance().IssueScatterCommand();
}
```

### tests/test_commands.cpp

```cpp
// tests/test_commands.cpp
// Test program for command system
// Task 16f - Command System

#include "command_system.h"
#include "selection_manager.h"
#include "cursor_context.h"
#include "platform.h"
#include <cstdio>
#include <cstring>
#include <vector>

// Test tracking
static std::vector<std::pair<void*, MissionType>> g_assigned_missions;
static std::vector<SelectableObject> g_test_objects;

static bool TestAssignMission(void* unit, MissionType mission, const CommandTarget& target) {
    g_assigned_missions.push_back({unit, mission});
    return true;
}

static bool TestCanPerform(void* unit, CommandType cmd) {
    // All units can perform all commands for testing
    return true;
}

static void CreateTestObjects(int player_house) {
    g_test_objects.clear();

    for (int i = 0; i < 10; i++) {
        SelectableObject obj;
        obj.id = 2000 + i;
        obj.cell_x = i;
        obj.cell_y = 0;
        obj.pixel_x = i * 24;
        obj.pixel_y = 0;
        obj.width = 24;
        obj.height = 24;
        obj.owner = player_house;
        obj.type = 0;
        obj.is_unit = true;
        obj.is_active = true;
        obj.rtti_type = 1;
        g_test_objects.push_back(obj);
    }
}

static std::vector<SelectableObject*> QueryAllObjects() {
    std::vector<SelectableObject*> result;
    for (auto& obj : g_test_objects) {
        result.push_back(&obj);
    }
    return result;
}

bool Test_CommandSystemInit() {
    printf("Test: Command System Init... ");

    if (!CommandSystem_Init()) {
        printf("FAILED - Init failed\n");
        return false;
    }

    CommandSystem_Shutdown();
    printf("PASSED\n");
    return true;
}

bool Test_MoveCommand() {
    printf("Test: Move Command... ");

    CreateTestObjects(0);
    g_assigned_missions.clear();

    SelectionManager_Init();
    CommandSystem_Init();

    auto& sel = SelectionManager::Instance();
    auto& cmd = CommandSystem::Instance();

    sel.SetPlayerHouse(0);
    sel.SetAllObjectsQuery(QueryAllObjects);
    cmd.SetAssignMissionCallback(TestAssignMission);
    cmd.SetCanPerformQuery(TestCanPerform);

    // Select some units
    sel.Select(&g_test_objects[0]);
    sel.AddToSelection(&g_test_objects[1]);

    // Issue move command
    CommandResult result = cmd.IssueMoveCommand(100, 100, false);

    if (result != CommandResult::SUCCESS) {
        printf("FAILED - Command not successful\n");
        CommandSystem_Shutdown();
        SelectionManager_Shutdown();
        return false;
    }

    if (g_assigned_missions.size() != 2) {
        printf("FAILED - Should have assigned to 2 units\n");
        CommandSystem_Shutdown();
        SelectionManager_Shutdown();
        return false;
    }

    // Check mission type
    for (const auto& assigned : g_assigned_missions) {
        if (assigned.second != MISSION_MOVE) {
            printf("FAILED - Wrong mission type\n");
            CommandSystem_Shutdown();
            SelectionManager_Shutdown();
            return false;
        }
    }

    CommandSystem_Shutdown();
    SelectionManager_Shutdown();
    printf("PASSED\n");
    return true;
}

bool Test_AttackCommand() {
    printf("Test: Attack Command... ");

    CreateTestObjects(0);
    g_assigned_missions.clear();

    SelectionManager_Init();
    CommandSystem_Init();

    auto& sel = SelectionManager::Instance();
    auto& cmd = CommandSystem::Instance();

    sel.SetPlayerHouse(0);
    sel.SetAllObjectsQuery(QueryAllObjects);
    cmd.SetAssignMissionCallback(TestAssignMission);

    // Select a unit
    sel.Select(&g_test_objects[0]);

    // Create a fake enemy target
    SelectableObject enemy;
    enemy.id = 9999;
    enemy.owner = 1;  // Different owner

    // Issue attack command
    CommandResult result = cmd.IssueAttackCommand(&enemy, false);

    if (result != CommandResult::SUCCESS) {
        printf("FAILED - Command not successful\n");
        CommandSystem_Shutdown();
        SelectionManager_Shutdown();
        return false;
    }

    if (g_assigned_missions.empty()) {
        printf("FAILED - No mission assigned\n");
        CommandSystem_Shutdown();
        SelectionManager_Shutdown();
        return false;
    }

    if (g_assigned_missions[0].second != MISSION_ATTACK) {
        printf("FAILED - Wrong mission type\n");
        CommandSystem_Shutdown();
        SelectionManager_Shutdown();
        return false;
    }

    CommandSystem_Shutdown();
    SelectionManager_Shutdown();
    printf("PASSED\n");
    return true;
}

bool Test_QueuedCommand() {
    printf("Test: Queued Command... ");

    CreateTestObjects(0);

    SelectionManager_Init();
    CommandSystem_Init();

    auto& sel = SelectionManager::Instance();
    auto& cmd = CommandSystem::Instance();

    sel.SetPlayerHouse(0);

    sel.Select(&g_test_objects[0]);

    // Issue queued move
    cmd.IssueMoveCommand(100, 100, true);

    const Command& last = cmd.GetLastCommand();
    if (!last.IsQueued()) {
        printf("FAILED - Command should be queued\n");
        CommandSystem_Shutdown();
        SelectionManager_Shutdown();
        return false;
    }

    CommandSystem_Shutdown();
    SelectionManager_Shutdown();
    printf("PASSED\n");
    return true;
}

bool Test_CommandResolution() {
    printf("Test: Command Resolution... ");

    CommandSystem_Init();

    auto& cmd = CommandSystem::Instance();

    // Test with passable empty terrain
    CursorContext ctx;
    ctx.Clear();
    ctx.world_x = 100;
    ctx.world_y = 100;
    ctx.is_passable = true;
    ctx.object = nullptr;

    Command resolved = cmd.ResolveCommand(ctx, false, false, false);
    if (resolved.type != CommandType::MOVE) {
        printf("FAILED - Empty terrain should resolve to MOVE\n");
        CommandSystem_Shutdown();
        return false;
    }

    // Test with enemy
    ctx.object = (void*)0x12345;
    ctx.is_enemy = true;
    ctx.is_attackable = true;

    resolved = cmd.ResolveCommand(ctx, false, false, false);
    if (resolved.type != CommandType::ATTACK) {
        printf("FAILED - Enemy should resolve to ATTACK\n");
        CommandSystem_Shutdown();
        return false;
    }

    // Test force move on enemy
    resolved = cmd.ResolveCommand(ctx, false, false, true);  // Alt = force move
    if (resolved.type != CommandType::MOVE) {
        printf("FAILED - Force move should override ATTACK\n");
        CommandSystem_Shutdown();
        return false;
    }

    // Test force attack on ground
    ctx.object = nullptr;
    ctx.is_enemy = false;
    resolved = cmd.ResolveCommand(ctx, false, true, false);  // Ctrl = force attack
    if (resolved.type != CommandType::FORCE_FIRE) {
        printf("FAILED - Ctrl+click ground should be FORCE_FIRE\n");
        CommandSystem_Shutdown();
        return false;
    }

    CommandSystem_Shutdown();
    printf("PASSED\n");
    return true;
}

bool Test_NoSelection() {
    printf("Test: No Selection... ");

    SelectionManager_Init();
    CommandSystem_Init();

    auto& cmd = CommandSystem::Instance();

    // Clear selection
    Selection_Clear();

    // Try to issue command
    CommandResult result = cmd.IssueMoveCommand(100, 100, false);

    if (result != CommandResult::INVALID_SELECTION) {
        printf("FAILED - Should fail with no selection\n");
        CommandSystem_Shutdown();
        SelectionManager_Shutdown();
        return false;
    }

    CommandSystem_Shutdown();
    SelectionManager_Shutdown();
    printf("PASSED\n");
    return true;
}

bool Test_StopAndGuard() {
    printf("Test: Stop and Guard Commands... ");

    CreateTestObjects(0);
    g_assigned_missions.clear();

    SelectionManager_Init();
    CommandSystem_Init();

    auto& sel = SelectionManager::Instance();
    auto& cmd = CommandSystem::Instance();

    sel.SetPlayerHouse(0);
    cmd.SetAssignMissionCallback(TestAssignMission);

    sel.Select(&g_test_objects[0]);

    // Test stop
    cmd.IssueStopCommand();
    if (g_assigned_missions.empty() || g_assigned_missions.back().second != MISSION_STOP) {
        printf("FAILED - Stop command wrong\n");
        CommandSystem_Shutdown();
        SelectionManager_Shutdown();
        return false;
    }

    // Test guard
    g_assigned_missions.clear();
    cmd.IssueGuardCommand(false);
    if (g_assigned_missions.empty() || g_assigned_missions.back().second != MISSION_GUARD) {
        printf("FAILED - Guard command wrong\n");
        CommandSystem_Shutdown();
        SelectionManager_Shutdown();
        return false;
    }

    CommandSystem_Shutdown();
    SelectionManager_Shutdown();
    printf("PASSED\n");
    return true;
}

int main(int argc, char* argv[]) {
    printf("=== Command System Tests (Task 16f) ===\n\n");

    Platform_Init();

    int passed = 0;
    int failed = 0;

    if (Test_CommandSystemInit()) passed++; else failed++;
    if (Test_MoveCommand()) passed++; else failed++;
    if (Test_AttackCommand()) passed++; else failed++;
    if (Test_QueuedCommand()) passed++; else failed++;
    if (Test_CommandResolution()) passed++; else failed++;
    if (Test_NoSelection()) passed++; else failed++;
    if (Test_StopAndGuard()) passed++; else failed++;

    Platform_Shutdown();

    printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);

    return (failed == 0) ? 0 : 1;
}
```

---

## CMakeLists.txt Additions

Add to `src/game/CMakeLists.txt`:

```cmake
# Command System (Task 16f)
target_sources(game PRIVATE
    input/command_types.h
    input/command_system.h
    input/command_system.cpp
)
```

Add test target:

```cmake
# Command test
add_executable(test_commands
    tests/test_commands.cpp
    src/game/input/command_system.cpp
    src/game/input/selection_manager.cpp
)
target_include_directories(test_commands PRIVATE
    src/game/input
    src/platform
)
target_link_libraries(test_commands platform)
```

---

## Verification Script

```bash
#!/bin/bash
set -e

echo "=== Task 16f Verification ==="

cd build
cmake --build . --target test_commands

echo "Running tests..."
./tests/test_commands

echo "Checking files..."
for file in \
    "../src/game/input/command_types.h" \
    "../src/game/input/command_system.h" \
    "../src/game/input/command_system.cpp"
do
    if [ ! -f "$file" ]; then
        echo "ERROR: Missing $file"
        exit 1
    fi
done

echo ""
echo "=== Task 16f Verification PASSED ==="
```

---

## Implementation Steps

1. **Create command_types.h** - Define command types, flags, and targets
2. **Create command_system.h** - System class declaration
3. **Create command_system.cpp** - Full implementation
4. **Create test_commands.cpp** - Comprehensive tests
5. **Update CMakeLists.txt** - Add files and test
6. **Build and test** - Verify all pass

---

## Success Criteria

- [ ] Command types properly defined
- [ ] Move command issues MISSION_MOVE
- [ ] Attack command issues MISSION_ATTACK
- [ ] Force fire (Ctrl+click ground) works
- [ ] Force move (Alt+click) overrides attack
- [ ] Queued commands (Shift) have flag set
- [ ] Stop and Guard commands work
- [ ] No selection returns INVALID_SELECTION
- [ ] Command resolution matches context
- [ ] All unit tests pass

---

## Common Issues

### Commands not reaching units
Ensure `SetAssignMissionCallback` is set with proper integration.

### Wrong mission type
Check `CommandToMission()` mapping is correct.

### Force modifiers not working
Verify modifier flags are passed through `ResolveCommand`.

---

## Completion Promise

When verification passes, output:
<promise>TASK_16F_COMPLETE</promise>

## Escape Hatch

If stuck after 15 iterations, output:
<promise>TASK_16F_BLOCKED</promise>

## Max Iterations
15
