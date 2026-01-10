// src/test_input_mapper.cpp
// Test program for input mapper
// Task 16b - GameAction Mapping System

#include "game/input/input_mapper.h"
#include "game/input/input_state.h"
#include "game/viewport.h"
#include "platform.h"
#include <cstdio>
#include <cstring>

//=============================================================================
// Unit Tests
//=============================================================================

bool Test_ActionNames() {
    printf("Test: Action Names... ");

    // Verify all actions have names
    for (int i = 0; i < static_cast<int>(GameAction::ACTION_COUNT); i++) {
        GameAction action = static_cast<GameAction>(i);
        const char* name = GetActionName(action);
        if (name == nullptr || strcmp(name, "UNKNOWN") == 0) {
            printf("FAILED - Action %d has no name\n", i);
            return false;
        }
    }

    printf("PASSED\n");
    return true;
}

bool Test_GroupNumberHelpers() {
    printf("Test: Group Number Helpers... ");

    // Test GROUP_SELECT
    if (GetGroupNumber(GameAction::GROUP_SELECT_1) != 1) {
        printf("FAILED - GROUP_SELECT_1 should be 1\n");
        return false;
    }
    if (GetGroupNumber(GameAction::GROUP_SELECT_0) != 0) {
        printf("FAILED - GROUP_SELECT_0 should be 0\n");
        return false;
    }
    if (GetGroupNumber(GameAction::GROUP_SELECT_5) != 5) {
        printf("FAILED - GROUP_SELECT_5 should be 5\n");
        return false;
    }

    // Test GROUP_CREATE
    if (GetGroupNumber(GameAction::GROUP_CREATE_3) != 3) {
        printf("FAILED - GROUP_CREATE_3 should be 3\n");
        return false;
    }

    // Test GROUP_ADD
    if (GetGroupNumber(GameAction::GROUP_ADD_7) != 7) {
        printf("FAILED - GROUP_ADD_7 should be 7\n");
        return false;
    }

    // Test non-group action
    if (GetGroupNumber(GameAction::ORDER_STOP) != -1) {
        printf("FAILED - Non-group action should return -1\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_ActionTypeChecks() {
    printf("Test: Action Type Checks... ");

    // Scroll actions
    if (!IsScrollAction(GameAction::SCROLL_UP)) {
        printf("FAILED - SCROLL_UP should be scroll action\n");
        return false;
    }
    if (IsScrollAction(GameAction::ORDER_STOP)) {
        printf("FAILED - ORDER_STOP should not be scroll action\n");
        return false;
    }

    // Group actions
    if (!IsGroupSelectAction(GameAction::GROUP_SELECT_5)) {
        printf("FAILED - GROUP_SELECT_5 should be group select\n");
        return false;
    }
    if (!IsGroupCreateAction(GameAction::GROUP_CREATE_3)) {
        printf("FAILED - GROUP_CREATE_3 should be group create\n");
        return false;
    }
    if (!IsGroupAction(GameAction::GROUP_ADD_1)) {
        printf("FAILED - GROUP_ADD_1 should be group action\n");
        return false;
    }

    // Debug actions
    if (!IsDebugAction(GameAction::DEBUG_REVEAL_MAP)) {
        printf("FAILED - DEBUG_REVEAL_MAP should be debug action\n");
        return false;
    }
    if (IsDebugAction(GameAction::ORDER_STOP)) {
        printf("FAILED - ORDER_STOP should not be debug action\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_MapperInit() {
    printf("Test: InputMapper Initialization... ");

    Platform_Init();
    Platform_Graphics_Init();

    // Initialize viewport for coordinate conversion
    GameViewport::Instance().Initialize();
    GameViewport::Instance().SetMapSize(64, 64);

    Input_Init();

    if (!InputMapper_Init()) {
        printf("FAILED - InputMapper_Init failed\n");
        Input_Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return false;
    }

    // Check that bindings exist
    InputMapper& mapper = InputMapper::Instance();
    const KeyBinding* binding = mapper.GetBinding(GameAction::ORDER_STOP);

    if (binding == nullptr || binding->key_code != KEY_S) {
        printf("FAILED - ORDER_STOP not bound to S\n");
        InputMapper_Shutdown();
        Input_Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return false;
    }

    InputMapper_Shutdown();
    Input_Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_ModifierFiltering() {
    printf("Test: Modifier Filtering... ");

    Platform_Init();
    Platform_Graphics_Init();

    // Initialize viewport for coordinate conversion
    GameViewport::Instance().Initialize();
    GameViewport::Instance().SetMapSize(64, 64);

    Input_Init();
    InputMapper_Init();

    InputMapper& mapper = InputMapper::Instance();

    // Check SELECT_ALL requires Ctrl
    const KeyBinding* select_all = mapper.GetBinding(GameAction::SELECT_ALL);
    if (select_all->required_mods != MOD_CTRL) {
        printf("FAILED - SELECT_ALL should require CTRL\n");
        InputMapper_Shutdown();
        Input_Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return false;
    }

    // Check ORDER_ATTACK_MOVE excludes Ctrl
    const KeyBinding* attack_move = mapper.GetBinding(GameAction::ORDER_ATTACK_MOVE);
    if ((attack_move->excluded_mods & MOD_CTRL) == 0) {
        printf("FAILED - ORDER_ATTACK_MOVE should exclude CTRL\n");
        InputMapper_Shutdown();
        Input_Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return false;
    }

    InputMapper_Shutdown();
    Input_Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_DebugActions() {
    printf("Test: Debug Actions Disabled by Default... ");

    Platform_Init();
    Platform_Graphics_Init();

    // Initialize viewport for coordinate conversion
    GameViewport::Instance().Initialize();
    GameViewport::Instance().SetMapSize(64, 64);

    Input_Init();
    InputMapper_Init();

    InputMapper& mapper = InputMapper::Instance();

    // Debug should be disabled by default
    if (mapper.IsDebugEnabled()) {
        printf("FAILED - Debug should be disabled by default\n");
        InputMapper_Shutdown();
        Input_Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return false;
    }

    // Enable debug
    mapper.SetDebugEnabled(true);
    if (!mapper.IsDebugEnabled()) {
        printf("FAILED - Debug should be enabled now\n");
        InputMapper_Shutdown();
        Input_Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return false;
    }

    InputMapper_Shutdown();
    Input_Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_RebindAction() {
    printf("Test: Rebind Action... ");

    Platform_Init();
    Platform_Graphics_Init();

    // Initialize viewport for coordinate conversion
    GameViewport::Instance().Initialize();
    GameViewport::Instance().SetMapSize(64, 64);

    Input_Init();
    InputMapper_Init();

    InputMapper& mapper = InputMapper::Instance();

    // Rebind ORDER_STOP from S to Q
    if (!mapper.RebindAction(GameAction::ORDER_STOP, KEY_Q, MOD_NONE)) {
        printf("FAILED - RebindAction returned false\n");
        InputMapper_Shutdown();
        Input_Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return false;
    }

    const KeyBinding* binding = mapper.GetBinding(GameAction::ORDER_STOP);
    if (binding->key_code != KEY_Q) {
        printf("FAILED - ORDER_STOP not rebound to Q\n");
        InputMapper_Shutdown();
        Input_Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return false;
    }

    // Reset and verify
    mapper.ResetBindings();
    binding = mapper.GetBinding(GameAction::ORDER_STOP);
    if (binding->key_code != KEY_S) {
        printf("FAILED - ORDER_STOP not reset to S\n");
        InputMapper_Shutdown();
        Input_Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return false;
    }

    InputMapper_Shutdown();
    Input_Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_ConflictDetection() {
    printf("Test: Conflict Detection... ");

    Platform_Init();
    Platform_Graphics_Init();

    // Initialize viewport for coordinate conversion
    GameViewport::Instance().Initialize();
    GameViewport::Instance().SetMapSize(64, 64);

    Input_Init();
    InputMapper_Init();

    InputMapper& mapper = InputMapper::Instance();

    // Check for conflict on S key (should conflict with ORDER_STOP)
    GameAction conflict;
    if (!mapper.HasConflict(KEY_S, MOD_NONE, &conflict)) {
        printf("FAILED - Should detect conflict on S key\n");
        InputMapper_Shutdown();
        Input_Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return false;
    }

    if (conflict != GameAction::ORDER_STOP) {
        printf("FAILED - Conflict should be ORDER_STOP, got %s\n", GetActionName(conflict));
        InputMapper_Shutdown();
        Input_Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return false;
    }

    // Check for no conflict on unused key
    if (mapper.HasConflict(KEY_J, MOD_NONE, nullptr)) {
        printf("FAILED - Should not detect conflict on J key\n");
        InputMapper_Shutdown();
        Input_Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return false;
    }

    InputMapper_Shutdown();
    Input_Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

//=============================================================================
// Interactive Test
//=============================================================================

void Interactive_Test() {
    printf("\n=== Interactive Input Mapper Test ===\n");
    printf("Press keys to see which actions are triggered\n");
    printf("Press ESC to exit\n\n");

    Platform_Init();
    Platform_Graphics_Init();

    // Initialize viewport for coordinate conversion
    GameViewport::Instance().Initialize();
    GameViewport::Instance().SetMapSize(64, 64);

    Input_Init();
    InputMapper_Init();

    // Enable debug for testing
    InputMapper::Instance().SetDebugEnabled(true);

    bool running = true;
    while (running) {
        Platform_PollEvents();
        Input_Update();
        InputMapper_ProcessFrame();

        // Check for exit
        if (Input_KeyPressed(KEY_ESCAPE)) {
            running = false;
            continue;
        }

        // Report any triggered actions
        for (int i = 0; i < static_cast<int>(GameAction::ACTION_COUNT); i++) {
            GameAction action = static_cast<GameAction>(i);
            if (InputMapper_WasTriggered(action)) {
                printf("TRIGGERED: %s\n", GetActionName(action));
            }
        }

        // Report active scroll
        GameAction scroll = InputMapper::Instance().GetActiveScrollAction();
        if (scroll != GameAction::NONE) {
            static GameAction last_scroll = GameAction::NONE;
            if (scroll != last_scroll) {
                printf("SCROLL: %s (fast=%d)\n", GetActionName(scroll),
                       InputMapper_IsActive(GameAction::SCROLL_FAST));
                last_scroll = scroll;
            }
        }

        Platform_Timer_Delay(16);
    }

    InputMapper_Shutdown();
    Input_Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Main
//=============================================================================

int main(int argc, char* argv[]) {
    printf("=== Input Mapper Tests (Task 16b) ===\n\n");

    // Check for quick mode
    bool quick_mode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--quick") == 0 || strcmp(argv[i], "-q") == 0) {
            quick_mode = true;
        }
    }

    int passed = 0;
    int failed = 0;

    // Run unit tests
    if (Test_ActionNames()) passed++; else failed++;
    if (Test_GroupNumberHelpers()) passed++; else failed++;
    if (Test_ActionTypeChecks()) passed++; else failed++;
    if (Test_MapperInit()) passed++; else failed++;
    if (Test_ModifierFiltering()) passed++; else failed++;
    if (Test_DebugActions()) passed++; else failed++;
    if (Test_RebindAction()) passed++; else failed++;
    if (Test_ConflictDetection()) passed++; else failed++;

    printf("\n");
    if (failed == 0) {
        printf("All tests PASSED (%d/%d)\n", passed, passed + failed);
    } else {
        printf("Results: %d passed, %d failed\n", passed, failed);
    }

    // Interactive test
    if (!quick_mode && argc > 1 && strcmp(argv[1], "-i") == 0) {
        Interactive_Test();
    } else if (!quick_mode) {
        printf("\nRun with -i for interactive test\n");
    }

    return (failed == 0) ? 0 : 1;
}
