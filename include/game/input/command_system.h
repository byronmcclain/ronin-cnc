// include/game/input/command_system.h
// Command system for issuing orders to units
// Task 16f - Command System

#ifndef COMMAND_SYSTEM_H
#define COMMAND_SYSTEM_H

#include "game/input/command_types.h"
#include "game/input/cursor_context.h"
#include "game/input/selection_manager.h"
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
