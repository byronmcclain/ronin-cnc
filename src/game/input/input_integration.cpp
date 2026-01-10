// src/game/input/input_integration.cpp
// Unified input system integration implementation
// Task 16h - Input Integration

#include "game/input/input_integration.h"
#include "game/viewport.h"
#include "platform.h"
#include <cstdio>

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

    Platform_LogInfo("InputSystem: Initializing all subsystems...");

    // Initialize GameViewport first if needed
    GameViewport::Instance().Initialize();

    // Initialize subsystems in dependency order
    // Note: InputState is now initialized through Input_Init() global function
    if (!Input_Init()) {
        Platform_LogInfo("InputSystem: Input_Init failed");
        return false;
    }

    if (!InputMapper::Instance().Initialize()) {
        Platform_LogInfo("InputSystem: InputMapper init failed");
        return false;
    }

    if (!KeyboardHandler::Instance().Initialize()) {
        Platform_LogInfo("InputSystem: KeyboardHandler init failed");
        return false;
    }

    if (!MouseHandler::Instance().Initialize()) {
        Platform_LogInfo("InputSystem: MouseHandler init failed");
        return false;
    }

    if (!SelectionManager::Instance().Initialize()) {
        Platform_LogInfo("InputSystem: SelectionManager init failed");
        return false;
    }

    if (!CommandSystem::Instance().Initialize()) {
        Platform_LogInfo("InputSystem: CommandSystem init failed");
        return false;
    }

    if (!ScrollProcessor::Instance().Initialize()) {
        Platform_LogInfo("InputSystem: ScrollProcessor init failed");
        return false;
    }

    initialized_ = true;
    Platform_LogInfo("InputSystem: All subsystems initialized");
    return true;
}

void InputSystem::Shutdown() {
    if (!initialized_) return;

    Platform_LogInfo("InputSystem: Shutting down all subsystems...");

    // Shutdown in reverse order
    ScrollProcessor::Instance().Shutdown();
    CommandSystem::Instance().Shutdown();
    SelectionManager::Instance().Shutdown();
    MouseHandler::Instance().Shutdown();
    KeyboardHandler::Instance().Shutdown();
    InputMapper::Instance().Shutdown();
    Input_Shutdown();

    initialized_ = false;
    Platform_LogInfo("InputSystem: All subsystems shut down");
}

//=============================================================================
// Per-Frame Processing
//=============================================================================

void InputSystem::Update() {
    if (!initialized_) return;

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
    if (WasActionTriggered(GameAction::UI_OPTIONS_MENU)) {
        // Would trigger menu close
    }
}

void InputSystem::ProcessTextInput() {
    // Text input handled by KeyboardHandler
    // Just check for escape to cancel text mode
    if (WasActionTriggered(GameAction::UI_OPTIONS_MENU)) {
        KeyboardHandler::Instance().EndTextInput();
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
    if (mouse.WasRightClicked() || WasActionTriggered(GameAction::BUILDING_CANCEL)) {
        MouseHandler::Instance().SetPlacementMode(false, 0);
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
    if (mapper.WasActionTriggered(GameAction::ORDER_STOP)) {
        cmd.IssueStopCommand();
    }

    // Guard
    if (mapper.WasActionTriggered(GameAction::ORDER_GUARD)) {
        cmd.IssueGuardCommand(false);
    }

    // Scatter
    if (mapper.WasActionTriggered(GameAction::ORDER_SCATTER)) {
        cmd.IssueScatterCommand();
    }

    // Select all (Ctrl+A)
    if (mapper.WasActionTriggered(GameAction::SELECT_ALL)) {
        sel.SelectAllOfType(-1);  // All types
    }

    // Control groups 1-9 and 0
    // GROUP_SELECT_1 through GROUP_SELECT_0 map to groups 1-9, then 0
    for (int i = 0; i < 10; i++) {
        GameAction select_action = static_cast<GameAction>(
            static_cast<int>(GameAction::GROUP_SELECT_1) + i);
        GameAction create_action = static_cast<GameAction>(
            static_cast<int>(GameAction::GROUP_CREATE_1) + i);
        GameAction add_action = static_cast<GameAction>(
            static_cast<int>(GameAction::GROUP_ADD_1) + i);

        int group_num = GetGroupNumber(select_action);

        if (mapper.WasActionTriggered(select_action)) {
            sel.RecallGroup(group_num);
        }
        if (mapper.WasActionTriggered(create_action)) {
            sel.SaveGroup(group_num);
        }
        if (mapper.WasActionTriggered(add_action)) {
            sel.AddGroupToSelection(group_num);
        }
    }

    // Deploy
    if (mapper.WasActionTriggered(GameAction::ORDER_DEPLOY)) {
        cmd.IssueDeployCommand();
    }
}

//=============================================================================
// Context Management
//=============================================================================

void InputSystem::SetContext(InputContext ctx) {
    if (context_ == ctx) return;

    char msg[128];
    snprintf(msg, sizeof(msg), "InputSystem: Context changed from %d to %d",
             static_cast<int>(context_), static_cast<int>(ctx));
    Platform_LogInfo(msg);

    context_ = ctx;

    // Configure subsystems for context
    switch (ctx) {
        case InputContext::TEXT_INPUT:
            KeyboardHandler::Instance().SetFocus(InputFocus::TEXT);
            ScrollProcessor::Instance().SetEdgeScrollEnabled(false);
            break;

        case InputContext::MENU:
            KeyboardHandler::Instance().SetFocus(InputFocus::MENU);
            ScrollProcessor::Instance().SetEdgeScrollEnabled(false);
            break;

        case InputContext::GAMEPLAY:
            KeyboardHandler::Instance().SetFocus(InputFocus::GAME);
            ScrollProcessor::Instance().SetEdgeScrollEnabled(true);
            MouseHandler::Instance().SetPlacementMode(false, 0);
            MouseHandler::Instance().SetSellMode(false);
            MouseHandler::Instance().SetRepairMode(false);
            break;

        case InputContext::BUILDING_PLACEMENT:
            KeyboardHandler::Instance().SetFocus(InputFocus::GAME);
            ScrollProcessor::Instance().SetEdgeScrollEnabled(true);
            MouseHandler::Instance().SetPlacementMode(true, 0);
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

bool InputSystem::IsKeyDown(int key) const {
    return InputState::Instance().IsKeyDown(key);
}

bool InputSystem::WasKeyPressed(int key) const {
    return InputState::Instance().WasKeyPressed(key);
}

bool InputSystem::WasKeyReleased(int key) const {
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
    return (InputState::Instance().GetModifiers() & MOD_SHIFT) != 0;
}

bool InputSystem::IsCtrlDown() const {
    return (InputState::Instance().GetModifiers() & MOD_CTRL) != 0;
}

bool InputSystem::IsAltDown() const {
    return (InputState::Instance().GetModifiers() & MOD_ALT) != 0;
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

bool InputSystem_Init() {
    return InputSystem::Instance().Initialize();
}

void InputSystem_Shutdown() {
    InputSystem::Instance().Shutdown();
}

void InputSystem_Update() {
    InputSystem::Instance().Update();
}

void InputSystem_Process() {
    InputSystem::Instance().Process();
}

void InputSystem_SetContext(int context) {
    InputSystem::Instance().SetContext(static_cast<InputContext>(context));
}

int InputSystem_GetContext() {
    return static_cast<int>(InputSystem::Instance().GetContext());
}

bool InputSystem_IsKeyDown(int key) {
    return InputSystem::Instance().IsKeyDown(static_cast<KeyCode>(key));
}

bool InputSystem_WasKeyPressed(int key) {
    return InputSystem::Instance().WasKeyPressed(static_cast<KeyCode>(key));
}

bool InputSystem_IsActionActive(int action) {
    return InputSystem::Instance().IsActionActive(static_cast<GameAction>(action));
}

bool InputSystem_WasActionTriggered(int action) {
    return InputSystem::Instance().WasActionTriggered(static_cast<GameAction>(action));
}

int InputSystem_GetMouseX() {
    return InputSystem::Instance().GetMouseX();
}

int InputSystem_GetMouseY() {
    return InputSystem::Instance().GetMouseY();
}

bool InputSystem_WasLeftClick() {
    return InputSystem::Instance().WasLeftClick();
}

bool InputSystem_WasRightClick() {
    return InputSystem::Instance().WasRightClick();
}

bool InputSystem_IsShiftDown() {
    return InputSystem::Instance().IsShiftDown();
}

bool InputSystem_IsCtrlDown() {
    return InputSystem::Instance().IsCtrlDown();
}

bool InputSystem_IsAltDown() {
    return InputSystem::Instance().IsAltDown();
}
