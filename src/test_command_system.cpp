// src/test_command_system.cpp
// Test program for command system
// Task 16f - Command System

#include "game/input/command_system.h"
#include "game/input/selection_manager.h"
#include "game/input/cursor_context.h"
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
    (void)unit;
    (void)cmd;
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

    // Check for quick mode
    bool quick_mode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--quick") == 0 || strcmp(argv[i], "-q") == 0) {
            quick_mode = true;
        }
    }
    (void)quick_mode;

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

    printf("\n");
    if (failed == 0) {
        printf("All tests PASSED (%d/%d)\n", passed, passed + failed);
    } else {
        printf("Results: %d passed, %d failed\n", passed, failed);
    }

    return (failed == 0) ? 0 : 1;
}
