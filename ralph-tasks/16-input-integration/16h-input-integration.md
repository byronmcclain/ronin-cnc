# Task 16h: Input Integration & Testing

| Field | Value |
|-------|-------|
| **Task ID** | 16h |
| **Task Name** | Input Integration & Testing |
| **Phase** | 16 - Input Integration |
| **Depends On** | 16a-16g (All previous input tasks) |
| **Estimated Complexity** | Medium (~400 lines) |

---

## Dependencies

All previous Phase 16 tasks must be complete:
- 16a: Input System Foundation
- 16b: GameAction Mapping System
- 16c: Keyboard Input Handler
- 16d: Mouse Handler & Coordinate System
- 16e: Selection System
- 16f: Command System
- 16g: Edge & Keyboard Scrolling

---

## Context

This final task integrates all input components into a cohesive system. It provides:

1. **Unified Input Processing**: Single entry point for all input handling
2. **Game Loop Integration**: Proper initialization, update, and shutdown sequence
3. **Input Processor**: Ties together actions, selection, commands, and scrolling
4. **Integration Tests**: Comprehensive tests verifying all systems work together
5. **Demo Application**: Interactive demo showcasing all input features

---

## Objective

Create the integration layer that:
1. Provides unified `Input_Init()`, `Input_Update()`, `Input_Process()`, `Input_Shutdown()`
2. Coordinates all input subsystems in correct order
3. Handles game state context (menu vs gameplay vs text input)
4. Provides comprehensive integration tests
5. Creates an interactive demo for verification

---

## Technical Background

### Input Processing Order

```
Per-Frame Input Processing:
1. Platform_Input_Update()     - Update raw input state
2. InputState::Update()        - Process edge detection
3. InputMapper::ProcessFrame() - Map keys to actions
4. MouseHandler::ProcessFrame() - Update mouse state
5. KeyboardHandler::ProcessFrame() - Process text input
6. ScrollProcessor::ProcessFrame() - Handle scrolling
7. Input_Process()             - Game-specific handling
   - Selection from clicks
   - Commands from right-clicks
   - Hotkey responses
```

### Input Context

```cpp
enum class InputContext {
    GAMEPLAY,       // Normal game controls
    MENU,           // Menu navigation
    TEXT_INPUT,     // Typing (chat, etc.)
    BUILDING_PLACEMENT, // Placing building
    SELL_MODE,      // Selling buildings
    REPAIR_MODE,    // Repair cursor active
    TARGETING,      // Super weapon targeting
};
```

### Integration Points

```cpp
// Main game loop integration
void Game_Loop() {
    while (running) {
        // Input
        Input_Update();

        // Game Logic
        if (Input_GetContext() == InputContext::GAMEPLAY) {
            Input_Process_Gameplay();
        }

        // Render
        Render();
    }
}
```

---

## Deliverables

- [ ] `src/game/input/input_integration.h` - Unified interface
- [ ] `src/game/input/input_integration.cpp` - Integration implementation
- [ ] `tests/test_input_integration.cpp` - Comprehensive integration tests
- [ ] `demos/input_demo.cpp` - Interactive demo application
- [ ] CMakeLists.txt updated
- [ ] All tests pass

---

## Files to Create

### src/game/input/input_integration.h

```cpp
// src/game/input/input_integration.h
// Unified input system integration
// Task 16h - Input Integration

#ifndef INPUT_INTEGRATION_H
#define INPUT_INTEGRATION_H

#include "input_defs.h"
#include "input_state.h"
#include "input_mapper.h"
#include "keyboard_handler.h"
#include "mouse_handler.h"
#include "selection_manager.h"
#include "command_system.h"
#include "scroll_processor.h"
#include <functional>

//=============================================================================
// Input Context
//=============================================================================

enum class InputContext {
    GAMEPLAY,               // Normal game controls
    MENU,                   // Menu navigation
    TEXT_INPUT,             // Typing (chat, etc.)
    BUILDING_PLACEMENT,     // Placing building
    SELL_MODE,              // Selling buildings
    REPAIR_MODE,            // Repair cursor active
    TARGETING,              // Super weapon targeting
    PAUSED,                 // Game paused
};

//=============================================================================
// Input System
//=============================================================================

class InputSystem {
public:
    // Singleton access
    static InputSystem& Instance();

    //=========================================================================
    // Lifecycle
    //=========================================================================

    bool Initialize();
    void Shutdown();
    bool IsInitialized() const { return initialized_; }

    //=========================================================================
    // Per-Frame Processing
    //=========================================================================

    // Update input state (call first in frame)
    void Update();

    // Process game input (call after Update)
    void Process();

    //=========================================================================
    // Context Management
    //=========================================================================

    void SetContext(InputContext ctx);
    InputContext GetContext() const { return context_; }

    // Convenience context checks
    bool IsInGameplay() const { return context_ == InputContext::GAMEPLAY; }
    bool IsInMenu() const { return context_ == InputContext::MENU; }
    bool IsInTextInput() const { return context_ == InputContext::TEXT_INPUT; }

    //=========================================================================
    // Subsystem Access
    //=========================================================================

    InputState& GetInputState() { return InputState::Instance(); }
    InputMapper& GetMapper() { return InputMapper::Instance(); }
    KeyboardHandler& GetKeyboard() { return KeyboardHandler::Instance(); }
    MouseHandler& GetMouse() { return MouseHandler::Instance(); }
    SelectionManager& GetSelection() { return SelectionManager::Instance(); }
    CommandSystem& GetCommands() { return CommandSystem::Instance(); }
    ScrollProcessor& GetScroll() { return ScrollProcessor::Instance(); }

    //=========================================================================
    // Quick Access Methods
    //=========================================================================

    // Key queries
    bool IsKeyDown(KeyCode key) const;
    bool WasKeyPressed(KeyCode key) const;
    bool WasKeyReleased(KeyCode key) const;

    // Action queries
    bool IsActionActive(GameAction action) const;
    bool WasActionTriggered(GameAction action) const;

    // Mouse queries
    int GetMouseX() const;
    int GetMouseY() const;
    int GetMouseCellX() const;
    int GetMouseCellY() const;
    bool WasLeftClick() const;
    bool WasRightClick() const;
    bool IsDragging() const;

    // Modifier queries
    bool IsShiftDown() const;
    bool IsCtrlDown() const;
    bool IsAltDown() const;

    //=========================================================================
    // Configuration
    //=========================================================================

    void SetWindowScale(int scale);

    //=========================================================================
    // Game Integration Callbacks
    //=========================================================================

    // Selection and commands need game state access
    using ObjectAtPosFunc = std::function<void*(int cell_x, int cell_y)>;
    using ObjectsInRectFunc = std::function<std::vector<void*>(int x1, int y1, int x2, int y2)>;
    using TerrainQueryFunc = std::function<bool(int cell_x, int cell_y)>;
    using AssignMissionFunc = std::function<bool(void* unit, int mission, int x, int y, void* target)>;

    void SetObjectAtPosQuery(ObjectAtPosFunc func);
    void SetObjectsInRectQuery(ObjectsInRectFunc func);
    void SetTerrainPassableQuery(TerrainQueryFunc func);
    void SetAssignMissionCallback(AssignMissionFunc func);

    void SetPlayerHouse(int house);

private:
    InputSystem();
    ~InputSystem();
    InputSystem(const InputSystem&) = delete;
    InputSystem& operator=(const InputSystem&) = delete;

    void ProcessGameplayInput();
    void ProcessMenuInput();
    void ProcessTextInput();
    void ProcessPlacementInput();

    void HandleSelectionClicks();
    void HandleCommandClicks();
    void HandleHotkeys();

    bool initialized_;
    InputContext context_;
    int player_house_;
};

//=============================================================================
// Global Functions (C-compatible interface)
//=============================================================================

// Lifecycle
bool Input_Init();
void Input_Shutdown();

// Per-frame
void Input_Update();
void Input_Process();

// Context
void Input_SetContext(int context);
int Input_GetContext();

// Quick access
bool Input_IsKeyDown(int key);
bool Input_WasKeyPressed(int key);
bool Input_IsActionActive(int action);
bool Input_WasActionTriggered(int action);

int Input_GetMouseX();
int Input_GetMouseY();
bool Input_WasLeftClick();
bool Input_WasRightClick();

bool Input_IsShiftDown();
bool Input_IsCtrlDown();
bool Input_IsAltDown();

#endif // INPUT_INTEGRATION_H
```

### src/game/input/input_integration.cpp

```cpp
// src/game/input/input_integration.cpp
// Unified input system integration implementation
// Task 16h - Input Integration

#include "input_integration.h"
#include "platform.h"

//=============================================================================
// InputSystem Singleton
//=============================================================================

InputSystem& InputSystem::Instance() {
    static InputSystem instance;
    return instance;
}

InputSystem::InputSystem()
    : initialized_(false)
    , context_(InputContext::GAMEPLAY)
    , player_house_(0) {
}

InputSystem::~InputSystem() {
    Shutdown();
}

//=============================================================================
// Lifecycle
//=============================================================================

bool InputSystem::Initialize() {
    if (initialized_) return true;

    Platform_Log("InputSystem: Initializing all subsystems...");

    // Initialize in dependency order
    if (!Platform_Input_Init()) {
        Platform_Log("InputSystem: Platform input init failed");
        return false;
    }

    if (!InputState::Instance().Initialize()) {
        Platform_Log("InputSystem: InputState init failed");
        return false;
    }

    if (!InputMapper::Instance().Initialize()) {
        Platform_Log("InputSystem: InputMapper init failed");
        return false;
    }

    if (!KeyboardHandler::Instance().Initialize()) {
        Platform_Log("InputSystem: KeyboardHandler init failed");
        return false;
    }

    if (!MouseHandler::Instance().Initialize()) {
        Platform_Log("InputSystem: MouseHandler init failed");
        return false;
    }

    if (!SelectionManager::Instance().Initialize()) {
        Platform_Log("InputSystem: SelectionManager init failed");
        return false;
    }

    if (!CommandSystem::Instance().Initialize()) {
        Platform_Log("InputSystem: CommandSystem init failed");
        return false;
    }

    if (!ScrollProcessor::Instance().Initialize()) {
        Platform_Log("InputSystem: ScrollProcessor init failed");
        return false;
    }

    initialized_ = true;
    Platform_Log("InputSystem: All subsystems initialized");
    return true;
}

void InputSystem::Shutdown() {
    if (!initialized_) return;

    Platform_Log("InputSystem: Shutting down all subsystems...");

    // Shutdown in reverse order
    ScrollProcessor::Instance().Shutdown();
    CommandSystem::Instance().Shutdown();
    SelectionManager::Instance().Shutdown();
    MouseHandler::Instance().Shutdown();
    KeyboardHandler::Instance().Shutdown();
    InputMapper::Instance().Shutdown();
    InputState::Instance().Shutdown();
    Platform_Input_Shutdown();

    initialized_ = false;
    Platform_Log("InputSystem: All subsystems shut down");
}

//=============================================================================
// Per-Frame Processing
//=============================================================================

void InputSystem::Update() {
    if (!initialized_) return;

    // Update raw platform state
    Platform_Input_Update();

    // Update state tracking (edge detection)
    InputState::Instance().Update();

    // Map keys to actions
    InputMapper::Instance().ProcessFrame();

    // Update keyboard handler
    KeyboardHandler::Instance().ProcessFrame();

    // Update mouse state
    MouseHandler::Instance().ProcessFrame();

    // Process scrolling
    ScrollProcessor::Instance().ProcessFrame();
}

void InputSystem::Process() {
    if (!initialized_) return;

    // Process based on current context
    switch (context_) {
        case InputContext::GAMEPLAY:
            ProcessGameplayInput();
            break;

        case InputContext::MENU:
            ProcessMenuInput();
            break;

        case InputContext::TEXT_INPUT:
            ProcessTextInput();
            break;

        case InputContext::BUILDING_PLACEMENT:
            ProcessPlacementInput();
            break;

        case InputContext::SELL_MODE:
        case InputContext::REPAIR_MODE:
        case InputContext::TARGETING:
            // These modes use mouse clicks but not normal selection
            HandleCommandClicks();
            break;

        case InputContext::PAUSED:
            // Minimal input processing when paused
            break;
    }
}

//=============================================================================
// Context-Specific Processing
//=============================================================================

void InputSystem::ProcessGameplayInput() {
    // Handle selection from mouse
    HandleSelectionClicks();

    // Handle commands from right-click
    HandleCommandClicks();

    // Handle hotkeys
    HandleHotkeys();
}

void InputSystem::ProcessMenuInput() {
    // Menu navigation - handled by UI system
    // Just check for escape to close menu
    if (WasActionTriggered(GameAction::CANCEL)) {
        // Would trigger menu close
    }
}

void InputSystem::ProcessTextInput() {
    // Text input handled by KeyboardHandler
    // Just check for escape to cancel text mode
    if (WasActionTriggered(GameAction::CANCEL)) {
        KeyboardHandler::Instance().StopTextInput();
        SetContext(InputContext::GAMEPLAY);
    }
}

void InputSystem::ProcessPlacementInput() {
    MouseHandler& mouse = MouseHandler::Instance();

    // Left click to place
    if (mouse.WasLeftClicked()) {
        if (mouse.GetContext().type == CursorContextType::PLACEMENT_VALID) {
            // Would trigger building placement
        }
    }

    // Right click or escape to cancel
    if (mouse.WasRightClicked() || WasActionTriggered(GameAction::CANCEL)) {
        MouseHandler::Instance().SetPlacementMode(false);
        SetContext(InputContext::GAMEPLAY);
    }
}

//=============================================================================
// Input Handling
//=============================================================================

void InputSystem::HandleSelectionClicks() {
    MouseHandler& mouse = MouseHandler::Instance();
    SelectionManager& sel = SelectionManager::Instance();

    // Only process clicks in tactical area
    if (!mouse.IsInTacticalArea()) return;

    // Left click - select
    if (mouse.WasLeftClicked() && !mouse.IsDragging()) {
        const CursorContext& ctx = mouse.GetContext();

        if (ctx.object && ctx.is_selectable) {
            if (IsShiftDown()) {
                sel.ToggleSelection(static_cast<SelectableObject*>(ctx.object));
            } else {
                sel.Select(static_cast<SelectableObject*>(ctx.object));
            }
        } else {
            if (!IsShiftDown()) {
                sel.Clear();
            }
        }
    }

    // Box selection on drag complete
    if (mouse.WasDragCompleted()) {
        const DragState& drag = mouse.GetDrag();
        int x1, y1, x2, y2;
        drag.GetScreenRect(x1, y1, x2, y2);
        sel.SelectInBox(x1, y1, x2, y2);
    }

    // Double-click to select all of type
    if (mouse.WasDoubleClicked()) {
        const CursorContext& ctx = mouse.GetContext();
        if (ctx.object) {
            SelectableObject* obj = static_cast<SelectableObject*>(ctx.object);
            sel.SelectAllOfType(obj->rtti_type);
        }
    }
}

void InputSystem::HandleCommandClicks() {
    MouseHandler& mouse = MouseHandler::Instance();

    if (!mouse.IsInTacticalArea()) return;

    if (mouse.WasRightClicked()) {
        const CursorContext& ctx = mouse.GetContext();

        Command cmd = CommandSystem::Instance().ResolveCommand(
            ctx, IsShiftDown(), IsCtrlDown(), IsAltDown());

        if (cmd.type != CommandType::NONE) {
            CommandSystem::Instance().IssueCommand(cmd);
        }
    }
}

void InputSystem::HandleHotkeys() {
    InputMapper& mapper = InputMapper::Instance();
    SelectionManager& sel = SelectionManager::Instance();
    CommandSystem& cmd = CommandSystem::Instance();

    // Stop
    if (mapper.WasActionTriggered(GameAction::STOP)) {
        cmd.IssueStopCommand();
    }

    // Guard
    if (mapper.WasActionTriggered(GameAction::GUARD)) {
        cmd.IssueGuardCommand(false);
    }

    // Scatter
    if (mapper.WasActionTriggered(GameAction::SCATTER)) {
        cmd.IssueScatterCommand();
    }

    // Select all (Ctrl+A)
    if (mapper.WasActionTriggered(GameAction::SELECT_ALL)) {
        sel.SelectAllOfType(-1);  // All types
    }

    // Control groups 0-9
    for (int i = 0; i < 10; i++) {
        GameAction recall = static_cast<GameAction>(
            static_cast<int>(GameAction::RECALL_GROUP_0) + i);
        GameAction create = static_cast<GameAction>(
            static_cast<int>(GameAction::CREATE_GROUP_0) + i);
        GameAction add = static_cast<GameAction>(
            static_cast<int>(GameAction::ADD_TO_GROUP_0) + i);

        if (mapper.WasActionTriggered(recall)) {
            sel.RecallGroup(i);
        }
        if (mapper.WasActionTriggered(create)) {
            sel.SaveGroup(i);
        }
        if (mapper.WasActionTriggered(add)) {
            sel.AddGroupToSelection(i);
        }
    }

    // Deploy
    if (mapper.WasActionTriggered(GameAction::DEPLOY)) {
        cmd.IssueDeployCommand();
    }
}

//=============================================================================
// Context Management
//=============================================================================

void InputSystem::SetContext(InputContext ctx) {
    if (context_ == ctx) return;

    Platform_Log("InputSystem: Context changed from %d to %d",
                 static_cast<int>(context_), static_cast<int>(ctx));

    context_ = ctx;

    // Configure subsystems for context
    switch (ctx) {
        case InputContext::TEXT_INPUT:
            KeyboardHandler::Instance().SetFocus(InputFocus::TEXT_INPUT);
            ScrollProcessor::Instance().SetEdgeScrollEnabled(false);
            break;

        case InputContext::MENU:
            KeyboardHandler::Instance().SetFocus(InputFocus::MENU);
            ScrollProcessor::Instance().SetEdgeScrollEnabled(false);
            break;

        case InputContext::GAMEPLAY:
            KeyboardHandler::Instance().SetFocus(InputFocus::TACTICAL);
            ScrollProcessor::Instance().SetEdgeScrollEnabled(true);
            MouseHandler::Instance().SetPlacementMode(false);
            MouseHandler::Instance().SetSellMode(false);
            MouseHandler::Instance().SetRepairMode(false);
            break;

        case InputContext::BUILDING_PLACEMENT:
            KeyboardHandler::Instance().SetFocus(InputFocus::TACTICAL);
            ScrollProcessor::Instance().SetEdgeScrollEnabled(true);
            MouseHandler::Instance().SetPlacementMode(true);
            break;

        case InputContext::SELL_MODE:
            MouseHandler::Instance().SetSellMode(true);
            break;

        case InputContext::REPAIR_MODE:
            MouseHandler::Instance().SetRepairMode(true);
            break;

        default:
            break;
    }
}

//=============================================================================
// Quick Access Methods
//=============================================================================

bool InputSystem::IsKeyDown(KeyCode key) const {
    return InputState::Instance().IsKeyDown(key);
}

bool InputSystem::WasKeyPressed(KeyCode key) const {
    return InputState::Instance().WasKeyPressed(key);
}

bool InputSystem::WasKeyReleased(KeyCode key) const {
    return InputState::Instance().WasKeyReleased(key);
}

bool InputSystem::IsActionActive(GameAction action) const {
    return InputMapper::Instance().IsActionActive(action);
}

bool InputSystem::WasActionTriggered(GameAction action) const {
    return InputMapper::Instance().WasActionTriggered(action);
}

int InputSystem::GetMouseX() const {
    return MouseHandler::Instance().GetScreenX();
}

int InputSystem::GetMouseY() const {
    return MouseHandler::Instance().GetScreenY();
}

int InputSystem::GetMouseCellX() const {
    return MouseHandler::Instance().GetCellX();
}

int InputSystem::GetMouseCellY() const {
    return MouseHandler::Instance().GetCellY();
}

bool InputSystem::WasLeftClick() const {
    return MouseHandler::Instance().WasLeftClicked();
}

bool InputSystem::WasRightClick() const {
    return MouseHandler::Instance().WasRightClicked();
}

bool InputSystem::IsDragging() const {
    return MouseHandler::Instance().IsDragging();
}

bool InputSystem::IsShiftDown() const {
    return InputState::Instance().IsModifierDown(KEY_LEFT_SHIFT) ||
           InputState::Instance().IsModifierDown(KEY_RIGHT_SHIFT);
}

bool InputSystem::IsCtrlDown() const {
    return InputState::Instance().IsModifierDown(KEY_LEFT_CONTROL) ||
           InputState::Instance().IsModifierDown(KEY_RIGHT_CONTROL);
}

bool InputSystem::IsAltDown() const {
    return InputState::Instance().IsModifierDown(KEY_LEFT_ALT) ||
           InputState::Instance().IsModifierDown(KEY_RIGHT_ALT);
}

//=============================================================================
// Configuration
//=============================================================================

void InputSystem::SetWindowScale(int scale) {
    MouseHandler::Instance().SetWindowScale(scale);
}

void InputSystem::SetPlayerHouse(int house) {
    player_house_ = house;
    SelectionManager::Instance().SetPlayerHouse(house);
}

//=============================================================================
// Global Functions
//=============================================================================

bool Input_Init() {
    return InputSystem::Instance().Initialize();
}

void Input_Shutdown() {
    InputSystem::Instance().Shutdown();
}

void Input_Update() {
    InputSystem::Instance().Update();
}

void Input_Process() {
    InputSystem::Instance().Process();
}

void Input_SetContext(int context) {
    InputSystem::Instance().SetContext(static_cast<InputContext>(context));
}

int Input_GetContext() {
    return static_cast<int>(InputSystem::Instance().GetContext());
}

bool Input_IsKeyDown(int key) {
    return InputSystem::Instance().IsKeyDown(static_cast<KeyCode>(key));
}

bool Input_WasKeyPressed(int key) {
    return InputSystem::Instance().WasKeyPressed(static_cast<KeyCode>(key));
}

bool Input_IsActionActive(int action) {
    return InputSystem::Instance().IsActionActive(static_cast<GameAction>(action));
}

bool Input_WasActionTriggered(int action) {
    return InputSystem::Instance().WasActionTriggered(static_cast<GameAction>(action));
}

int Input_GetMouseX() {
    return InputSystem::Instance().GetMouseX();
}

int Input_GetMouseY() {
    return InputSystem::Instance().GetMouseY();
}

bool Input_WasLeftClick() {
    return InputSystem::Instance().WasLeftClick();
}

bool Input_WasRightClick() {
    return InputSystem::Instance().WasRightClick();
}

bool Input_IsShiftDown() {
    return InputSystem::Instance().IsShiftDown();
}

bool Input_IsCtrlDown() {
    return InputSystem::Instance().IsCtrlDown();
}

bool Input_IsAltDown() {
    return InputSystem::Instance().IsAltDown();
}
```

### tests/test_input_integration.cpp

```cpp
// tests/test_input_integration.cpp
// Comprehensive integration tests for input system
// Task 16h - Input Integration

#include "input_integration.h"
#include "platform.h"
#include <cstdio>
#include <cstring>

bool Test_FullInitialization() {
    printf("Test: Full System Initialization... ");

    Platform_Init();

    if (!Input_Init()) {
        printf("FAILED - Init failed\n");
        Platform_Shutdown();
        return false;
    }

    // Verify all subsystems initialized
    InputSystem& sys = InputSystem::Instance();

    if (!sys.IsInitialized()) {
        printf("FAILED - Not marked initialized\n");
        Input_Shutdown();
        Platform_Shutdown();
        return false;
    }

    Input_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_ContextSwitching() {
    printf("Test: Context Switching... ");

    Platform_Init();
    Input_Init();

    InputSystem& sys = InputSystem::Instance();

    // Start in gameplay
    if (sys.GetContext() != InputContext::GAMEPLAY) {
        printf("FAILED - Should start in GAMEPLAY\n");
        Input_Shutdown();
        Platform_Shutdown();
        return false;
    }

    // Switch to menu
    sys.SetContext(InputContext::MENU);
    if (!sys.IsInMenu()) {
        printf("FAILED - Should be in MENU\n");
        Input_Shutdown();
        Platform_Shutdown();
        return false;
    }

    // Switch to text input
    sys.SetContext(InputContext::TEXT_INPUT);
    if (!sys.IsInTextInput()) {
        printf("FAILED - Should be in TEXT_INPUT\n");
        Input_Shutdown();
        Platform_Shutdown();
        return false;
    }

    // Back to gameplay
    sys.SetContext(InputContext::GAMEPLAY);
    if (!sys.IsInGameplay()) {
        printf("FAILED - Should be in GAMEPLAY\n");
        Input_Shutdown();
        Platform_Shutdown();
        return false;
    }

    Input_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_UpdateCycle() {
    printf("Test: Update Cycle... ");

    Platform_Init();
    Input_Init();

    // Run several update cycles
    for (int i = 0; i < 100; i++) {
        Input_Update();
        Input_Process();
    }

    // Should not crash or hang

    Input_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_QuickAccessMethods() {
    printf("Test: Quick Access Methods... ");

    Platform_Init();
    Input_Init();

    InputSystem& sys = InputSystem::Instance();

    Input_Update();

    // Just verify methods don't crash
    int mx = sys.GetMouseX();
    int my = sys.GetMouseY();
    int cx = sys.GetMouseCellX();
    int cy = sys.GetMouseCellY();

    bool shift = sys.IsShiftDown();
    bool ctrl = sys.IsCtrlDown();
    bool alt = sys.IsAltDown();

    bool drag = sys.IsDragging();

    // Values should be reasonable
    if (mx < 0 || my < 0) {
        printf("FAILED - Invalid mouse position\n");
        Input_Shutdown();
        Platform_Shutdown();
        return false;
    }

    Input_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_SubsystemAccess() {
    printf("Test: Subsystem Access... ");

    Platform_Init();
    Input_Init();

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
    mapper.IsActionActive(GameAction::STOP);
    kb.GetFocus();
    mouse.GetScreenX();
    sel.HasSelection();
    cmd.GetLastResult();
    scroll.IsScrolling();

    Input_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_SelectionIntegration() {
    printf("Test: Selection Integration... ");

    Platform_Init();
    Input_Init();

    InputSystem& sys = InputSystem::Instance();
    SelectionManager& sel = sys.GetSelection();

    // Set up player house
    sys.SetPlayerHouse(0);

    // Verify selection is accessible and empty
    if (sel.HasSelection()) {
        printf("FAILED - Selection should be empty\n");
        Input_Shutdown();
        Platform_Shutdown();
        return false;
    }

    Input_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_CGlobalFunctions() {
    printf("Test: C Global Functions... ");

    Platform_Init();

    if (!Input_Init()) {
        printf("FAILED - Init failed\n");
        Platform_Shutdown();
        return false;
    }

    // Test C API functions
    Input_Update();
    Input_Process();

    Input_SetContext(1);  // MENU
    if (Input_GetContext() != 1) {
        printf("FAILED - Context not set\n");
        Input_Shutdown();
        Platform_Shutdown();
        return false;
    }

    Input_SetContext(0);  // GAMEPLAY

    int mx = Input_GetMouseX();
    int my = Input_GetMouseY();

    bool left = Input_WasLeftClick();
    bool right = Input_WasRightClick();

    bool shift = Input_IsShiftDown();
    bool ctrl = Input_IsCtrlDown();
    bool alt = Input_IsAltDown();

    Input_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_ReinitializationSafe() {
    printf("Test: Reinitialization Safety... ");

    Platform_Init();

    // First init
    Input_Init();
    Input_Update();
    Input_Shutdown();

    // Second init (should work)
    if (!Input_Init()) {
        printf("FAILED - Second init failed\n");
        Platform_Shutdown();
        return false;
    }

    Input_Update();
    Input_Shutdown();

    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

int main(int argc, char* argv[]) {
    printf("=== Input Integration Tests (Task 16h) ===\n\n");

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

    printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);

    return (failed == 0) ? 0 : 1;
}
```

### demos/input_demo.cpp

```cpp
// demos/input_demo.cpp
// Interactive demo for input system
// Task 16h - Input Integration

#include "input_integration.h"
#include "viewport.h"
#include "platform.h"
#include <cstdio>
#include <cstring>
#include <vector>

// Simulated game objects for demo
struct DemoObject {
    int id;
    int x, y;           // Pixel position
    int cell_x, cell_y;
    int owner;
    bool selected;
};

static std::vector<DemoObject> g_objects;

static void CreateDemoObjects() {
    g_objects.clear();

    // Create a grid of objects
    for (int i = 0; i < 20; i++) {
        DemoObject obj;
        obj.id = 1000 + i;
        obj.cell_x = 5 + (i % 5) * 3;
        obj.cell_y = 5 + (i / 5) * 3;
        obj.x = obj.cell_x * 24 + 12;
        obj.y = obj.cell_y * 24 + 12;
        obj.owner = 0;  // Player owned
        obj.selected = false;
        g_objects.push_back(obj);
    }

    // Add some "enemy" objects
    for (int i = 0; i < 5; i++) {
        DemoObject obj;
        obj.id = 2000 + i;
        obj.cell_x = 30 + i * 3;
        obj.cell_y = 10;
        obj.x = obj.cell_x * 24 + 12;
        obj.y = obj.cell_y * 24 + 12;
        obj.owner = 1;  // Enemy
        obj.selected = false;
        g_objects.push_back(obj);
    }
}

static DemoObject* FindObjectAt(int cell_x, int cell_y) {
    for (auto& obj : g_objects) {
        if (obj.cell_x == cell_x && obj.cell_y == cell_y) {
            return &obj;
        }
    }
    return nullptr;
}

void RunDemo() {
    printf("\n=== Input System Interactive Demo ===\n");
    printf("Controls:\n");
    printf("  Left-click: Select unit at cursor\n");
    printf("  Left-drag: Box select\n");
    printf("  Shift+click: Add to selection\n");
    printf("  Right-click: Issue move command\n");
    printf("  Ctrl+right-click: Force attack\n");
    printf("  S: Stop\n");
    printf("  G: Guard\n");
    printf("  X: Scatter\n");
    printf("  Ctrl+1-9: Save control group\n");
    printf("  1-9: Recall control group\n");
    printf("  Arrow keys: Scroll viewport\n");
    printf("  Shift+arrows: Fast scroll\n");
    printf("  T: Toggle text input mode\n");
    printf("  M: Toggle menu mode\n");
    printf("  P: Toggle placement mode\n");
    printf("  ESC: Exit\n\n");

    // Initialize everything
    Platform_Init();
    Platform_Graphics_Init();

    Viewport::Instance().Initialize();
    Viewport::Instance().SetMapSize(128, 128);

    Input_Init();

    InputSystem& input = InputSystem::Instance();
    input.SetPlayerHouse(0);
    input.SetWindowScale(2);

    CreateDemoObjects();

    bool running = true;
    InputContext last_context = InputContext::GAMEPLAY;

    while (running) {
        Platform_PumpEvents();
        Input_Update();
        Input_Process();

        // ESC to exit
        if (input.WasKeyPressed(KEY_ESCAPE)) {
            if (input.GetContext() != InputContext::GAMEPLAY) {
                input.SetContext(InputContext::GAMEPLAY);
            } else {
                running = false;
            }
            continue;
        }

        // Context toggles
        if (input.WasKeyPressed(KEY_T)) {
            if (input.IsInTextInput()) {
                input.SetContext(InputContext::GAMEPLAY);
            } else {
                input.SetContext(InputContext::TEXT_INPUT);
            }
        }

        if (input.WasKeyPressed(KEY_M)) {
            if (input.IsInMenu()) {
                input.SetContext(InputContext::GAMEPLAY);
            } else {
                input.SetContext(InputContext::MENU);
            }
        }

        if (input.WasKeyPressed(KEY_P)) {
            if (input.GetContext() == InputContext::BUILDING_PLACEMENT) {
                input.SetContext(InputContext::GAMEPLAY);
            } else {
                input.SetContext(InputContext::BUILDING_PLACEMENT);
            }
        }

        // Report context changes
        if (input.GetContext() != last_context) {
            const char* ctx_names[] = {
                "GAMEPLAY", "MENU", "TEXT_INPUT", "BUILDING_PLACEMENT",
                "SELL_MODE", "REPAIR_MODE", "TARGETING", "PAUSED"
            };
            printf("Context: %s\n", ctx_names[static_cast<int>(input.GetContext())]);
            last_context = input.GetContext();
        }

        // Report selection changes
        SelectionManager& sel = input.GetSelection();
        static int last_sel_count = 0;
        if (sel.GetSelectionCount() != last_sel_count) {
            printf("Selection count: %d\n", sel.GetSelectionCount());
            last_sel_count = sel.GetSelectionCount();
        }

        // Report commands
        CommandSystem& cmd = input.GetCommands();
        static CommandType last_cmd = CommandType::NONE;
        if (cmd.GetLastCommand().type != last_cmd &&
            cmd.GetLastCommand().type != CommandType::NONE) {
            printf("Command issued: type %d at (%d, %d)\n",
                   static_cast<int>(cmd.GetLastCommand().type),
                   cmd.GetLastCommand().target.world_x,
                   cmd.GetLastCommand().target.world_y);
            last_cmd = cmd.GetLastCommand().type;
        }

        // Report scroll
        ScrollProcessor& scroll = input.GetScroll();
        if (scroll.IsScrolling()) {
            static int frame = 0;
            if (++frame % 30 == 0) {
                Viewport& vp = Viewport::Instance();
                printf("Viewport: (%d, %d)\n", vp.x, vp.y);
            }
        }

        // Report mouse position changes
        static int last_cx = -1, last_cy = -1;
        int cx = input.GetMouseCellX();
        int cy = input.GetMouseCellY();
        if (cx != last_cx || cy != last_cy) {
            DemoObject* obj = FindObjectAt(cx, cy);
            if (obj) {
                printf("Mouse at cell (%d,%d) - %s object ID %d\n",
                       cx, cy,
                       obj->owner == 0 ? "friendly" : "enemy",
                       obj->id);
            }
            last_cx = cx;
            last_cy = cy;
        }

        // Report text input
        if (input.IsInTextInput()) {
            KeyboardHandler& kb = input.GetKeyboard();
            const TextInputState& text = kb.GetTextInput();
            static size_t last_len = 0;
            if (text.buffer.length() != last_len) {
                printf("Text: \"%s\"\n", text.buffer.c_str());
                last_len = text.buffer.length();
            }
        }

        // Report control groups
        static bool last_group_status[10] = {false};
        for (int i = 0; i < 10; i++) {
            bool has = sel.HasGroup(i);
            if (has != last_group_status[i]) {
                if (has) {
                    printf("Group %d saved (%d units)\n", i, sel.GetGroupSize(i));
                }
                last_group_status[i] = has;
            }
        }

        Platform_Timer_Delay(16);
    }

    Input_Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();

    printf("\nDemo ended.\n");
}

int main(int argc, char* argv[]) {
    RunDemo();
    return 0;
}
```

---

## CMakeLists.txt Additions

Add to `src/game/CMakeLists.txt`:

```cmake
# Input Integration (Task 16h)
target_sources(game PRIVATE
    input/input_integration.h
    input/input_integration.cpp
)
```

Add test and demo targets:

```cmake
# Input integration test
add_executable(test_input_integration
    tests/test_input_integration.cpp
    src/game/input/input_integration.cpp
    src/game/input/input_state.cpp
    src/game/input/input_mapper.cpp
    src/game/input/keyboard_handler.cpp
    src/game/input/mouse_handler.cpp
    src/game/input/selection_manager.cpp
    src/game/input/command_system.cpp
    src/game/input/scroll_processor.cpp
)
target_include_directories(test_input_integration PRIVATE
    src/game/input
    src/game/graphics
    src/platform
)
target_link_libraries(test_input_integration platform)

# Input demo
add_executable(input_demo
    demos/input_demo.cpp
    src/game/input/input_integration.cpp
    src/game/input/input_state.cpp
    src/game/input/input_mapper.cpp
    src/game/input/keyboard_handler.cpp
    src/game/input/mouse_handler.cpp
    src/game/input/selection_manager.cpp
    src/game/input/command_system.cpp
    src/game/input/scroll_processor.cpp
)
target_include_directories(input_demo PRIVATE
    src/game/input
    src/game/graphics
    src/platform
)
target_link_libraries(input_demo platform)
```

---

## Verification Script

```bash
#!/bin/bash
set -e

echo "=== Task 16h Verification ==="

cd build
cmake --build . --target test_input_integration
cmake --build . --target input_demo

echo "Running integration tests..."
./tests/test_input_integration

echo "Checking files..."
for file in \
    "../src/game/input/input_integration.h" \
    "../src/game/input/input_integration.cpp"
do
    if [ ! -f "$file" ]; then
        echo "ERROR: Missing $file"
        exit 1
    fi
done

echo ""
echo "=== Task 16h Verification PASSED ==="
echo ""
echo "Run './demos/input_demo' for interactive testing"
```

---

## Implementation Steps

1. **Create input_integration.h** - Define unified interface
2. **Create input_integration.cpp** - Implement integration layer
3. **Create test_input_integration.cpp** - Integration tests
4. **Create input_demo.cpp** - Interactive demo
5. **Update CMakeLists.txt** - Add all targets
6. **Build and test** - Verify integration tests pass
7. **Run demo** - Manual verification

---

## Success Criteria

- [ ] Full system initializes without errors
- [ ] Context switching works correctly
- [ ] Update cycle runs without crashes
- [ ] All quick access methods work
- [ ] All subsystems accessible
- [ ] Selection integration functional
- [ ] C global functions work
- [ ] Reinitialization is safe
- [ ] Demo shows all features working
- [ ] All integration tests pass

---

## Common Issues

### Initialization order
Subsystems must initialize in dependency order. Check Initialize() for correct sequence.

### Context not propagating
Ensure SetContext() updates all relevant subsystem states.

### Update cycle crashes
Verify all subsystems handle null callbacks gracefully.

---

## Phase 16 Completion Checklist

After completing Task 16h, verify the entire phase:

- [ ] 16a: Input foundation (states, defs)
- [ ] 16b: GameAction mapping system
- [ ] 16c: Keyboard handler with text input
- [ ] 16d: Mouse handler with coordinate conversion
- [ ] 16e: Selection system with control groups
- [ ] 16f: Command system for unit orders
- [ ] 16g: Edge and keyboard scrolling
- [ ] 16h: Full integration with demo

---

## Completion Promise

When verification passes, output:
<promise>TASK_16H_COMPLETE</promise>

## Escape Hatch

If stuck after 15 iterations, output:
<promise>TASK_16H_BLOCKED</promise>

## Max Iterations
15
