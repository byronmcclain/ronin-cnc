// src/test_input_integration.cpp
// Comprehensive integration tests for input system
// Task 16h - Input Integration

#include "game/input/input_integration.h"
#include "game/viewport.h"
#include "platform.h"
#include <cstdio>
#include <cstring>

bool Test_FullInitialization() {
    printf("Test: Full System Initialization... ");

    Platform_Init();
    GameViewport::Instance().Initialize();
    GameViewport::Instance().SetMapSize(128, 128);

    if (!InputSystem_Init()) {
        printf("FAILED - Init failed\n");
        Platform_Shutdown();
        return false;
    }

    // Verify all subsystems initialized
    InputSystem& sys = InputSystem::Instance();

    if (!sys.IsInitialized()) {
        printf("FAILED - Not marked initialized\n");
        InputSystem_Shutdown();
        Platform_Shutdown();
        return false;
    }

    InputSystem_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_ContextSwitching() {
    printf("Test: Context Switching... ");

    Platform_Init();
    GameViewport::Instance().Initialize();
    GameViewport::Instance().SetMapSize(128, 128);
    InputSystem_Init();

    InputSystem& sys = InputSystem::Instance();

    // Start in gameplay
    if (sys.GetContext() != InputContext::GAMEPLAY) {
        printf("FAILED - Should start in GAMEPLAY\n");
        InputSystem_Shutdown();
        Platform_Shutdown();
        return false;
    }

    // Switch to menu
    sys.SetContext(InputContext::MENU);
    if (!sys.IsInMenu()) {
        printf("FAILED - Should be in MENU\n");
        InputSystem_Shutdown();
        Platform_Shutdown();
        return false;
    }

    // Switch to text input
    sys.SetContext(InputContext::TEXT_INPUT);
    if (!sys.IsInTextInput()) {
        printf("FAILED - Should be in TEXT_INPUT\n");
        InputSystem_Shutdown();
        Platform_Shutdown();
        return false;
    }

    // Back to gameplay
    sys.SetContext(InputContext::GAMEPLAY);
    if (!sys.IsInGameplay()) {
        printf("FAILED - Should be in GAMEPLAY\n");
        InputSystem_Shutdown();
        Platform_Shutdown();
        return false;
    }

    InputSystem_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_UpdateCycle() {
    printf("Test: Update Cycle... ");

    Platform_Init();
    GameViewport::Instance().Initialize();
    GameViewport::Instance().SetMapSize(128, 128);
    InputSystem_Init();

    // Run several update cycles
    for (int i = 0; i < 100; i++) {
        InputSystem_Update();
        InputSystem_Process();
    }

    // Should not crash or hang

    InputSystem_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_QuickAccessMethods() {
    printf("Test: Quick Access Methods... ");

    Platform_Init();
    GameViewport::Instance().Initialize();
    GameViewport::Instance().SetMapSize(128, 128);
    InputSystem_Init();

    InputSystem& sys = InputSystem::Instance();

    InputSystem_Update();

    // Just verify methods don't crash
    int mx = sys.GetMouseX();
    int my = sys.GetMouseY();
    int cx = sys.GetMouseCellX();
    int cy = sys.GetMouseCellY();
    (void)cx;
    (void)cy;

    bool shift = sys.IsShiftDown();
    bool ctrl = sys.IsCtrlDown();
    bool alt = sys.IsAltDown();
    (void)shift;
    (void)ctrl;
    (void)alt;

    bool drag = sys.IsDragging();
    (void)drag;

    // Values should be reasonable
    if (mx < 0 || my < 0) {
        printf("FAILED - Invalid mouse position\n");
        InputSystem_Shutdown();
        Platform_Shutdown();
        return false;
    }

    InputSystem_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_SubsystemAccess() {
    printf("Test: Subsystem Access... ");

    Platform_Init();
    GameViewport::Instance().Initialize();
    GameViewport::Instance().SetMapSize(128, 128);
    InputSystem_Init();

    InputSystem& sys = InputSystem::Instance();

    // Access all subsystems
    InputState& state = sys.GetInputState();
    InputMapper& mapper = sys.GetMapper();
    KeyboardHandler& kb = sys.GetKeyboard();
    MouseHandler& mouse = sys.GetMouse();
    SelectionManager& sel = sys.GetSelection();
    CommandSystem& cmd = sys.GetCommands();
    ScrollProcessor& scroll = sys.GetScroll();

    // Verify they're usable (call a method on each)
    state.IsKeyDown(KEY_A);
    mapper.IsActionActive(GameAction::ORDER_STOP);
    kb.GetFocus();
    mouse.GetScreenX();
    sel.HasSelection();
    cmd.GetLastResult();
    scroll.IsScrolling();

    InputSystem_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_SelectionIntegration() {
    printf("Test: Selection Integration... ");

    Platform_Init();
    GameViewport::Instance().Initialize();
    GameViewport::Instance().SetMapSize(128, 128);
    InputSystem_Init();

    InputSystem& sys = InputSystem::Instance();
    SelectionManager& sel = sys.GetSelection();

    // Set up player house
    sys.SetPlayerHouse(0);

    // Verify selection is accessible and empty
    if (sel.HasSelection()) {
        printf("FAILED - Selection should be empty\n");
        InputSystem_Shutdown();
        Platform_Shutdown();
        return false;
    }

    InputSystem_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_CGlobalFunctions() {
    printf("Test: C Global Functions... ");

    Platform_Init();
    GameViewport::Instance().Initialize();
    GameViewport::Instance().SetMapSize(128, 128);

    if (!InputSystem_Init()) {
        printf("FAILED - Init failed\n");
        Platform_Shutdown();
        return false;
    }

    // Test C API functions
    InputSystem_Update();
    InputSystem_Process();

    InputSystem_SetContext(1);  // MENU
    if (InputSystem_GetContext() != 1) {
        printf("FAILED - Context not set\n");
        InputSystem_Shutdown();
        Platform_Shutdown();
        return false;
    }

    InputSystem_SetContext(0);  // GAMEPLAY

    int mx = InputSystem_GetMouseX();
    int my = InputSystem_GetMouseY();
    (void)mx;
    (void)my;

    bool left = InputSystem_WasLeftClick();
    bool right = InputSystem_WasRightClick();
    (void)left;
    (void)right;

    bool shift = InputSystem_IsShiftDown();
    bool ctrl = InputSystem_IsCtrlDown();
    bool alt = InputSystem_IsAltDown();
    (void)shift;
    (void)ctrl;
    (void)alt;

    InputSystem_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_ReinitializationSafe() {
    printf("Test: Reinitialization Safety... ");

    Platform_Init();
    GameViewport::Instance().Initialize();
    GameViewport::Instance().SetMapSize(128, 128);

    // First init
    InputSystem_Init();
    InputSystem_Update();
    InputSystem_Shutdown();

    // Second init (should work)
    if (!InputSystem_Init()) {
        printf("FAILED - Second init failed\n");
        Platform_Shutdown();
        return false;
    }

    InputSystem_Update();
    InputSystem_Shutdown();

    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

int main(int argc, char* argv[]) {
    printf("=== Input Integration Tests (Task 16h) ===\n\n");

    // Check for quick mode
    bool quick_mode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--quick") == 0 || strcmp(argv[i], "-q") == 0) {
            quick_mode = true;
        }
    }
    (void)quick_mode;

    int passed = 0;
    int failed = 0;

    if (Test_FullInitialization()) passed++; else failed++;
    if (Test_ContextSwitching()) passed++; else failed++;
    if (Test_UpdateCycle()) passed++; else failed++;
    if (Test_QuickAccessMethods()) passed++; else failed++;
    if (Test_SubsystemAccess()) passed++; else failed++;
    if (Test_SelectionIntegration()) passed++; else failed++;
    if (Test_CGlobalFunctions()) passed++; else failed++;
    if (Test_ReinitializationSafe()) passed++; else failed++;

    printf("\n");
    if (failed == 0) {
        printf("All tests PASSED (%d/%d)\n", passed, passed + failed);
    } else {
        printf("Results: %d passed, %d failed\n", passed, failed);
    }

    return (failed == 0) ? 0 : 1;
}
