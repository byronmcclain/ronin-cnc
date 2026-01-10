// src/game/input/command_system.cpp
// Command system implementation
// Task 16f - Command System

#include "game/input/command_system.h"
#include "game/input/selection_manager.h"
#include "platform.h"
#include <algorithm>
#include <cstdio>

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

    Platform_LogInfo("CommandSystem: Initializing...");

    last_command_.Clear();
    last_result_ = CommandResult::SUCCESS;

    initialized_ = true;
    Platform_LogInfo("CommandSystem: Initialized");
    return true;
}

void CommandSystem::Shutdown() {
    if (!initialized_) return;

    Platform_LogInfo("CommandSystem: Shutting down");
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
        char msg[128];
        snprintf(msg, sizeof(msg), "CommandSystem: No mission mapping for command %d",
                 static_cast<int>(cmd.type));
        Platform_LogInfo(msg);
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

    char msg[128];
    snprintf(msg, sizeof(msg), "CommandSystem: Command %d - %d success, %d failed",
             static_cast<int>(cmd.type), success_count, fail_count);
    Platform_LogInfo(msg);

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
