// include/game/input/input_integration.h
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
    bool IsKeyDown(int key) const;
    bool WasKeyPressed(int key) const;
    bool WasKeyReleased(int key) const;

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

// Note: Input_Init and Input_Shutdown are already declared in input_state.h
// These extend the functionality for the full integration layer

// Additional lifecycle
bool InputSystem_Init();
void InputSystem_Shutdown();

// Per-frame
void InputSystem_Update();
void InputSystem_Process();

// Context
void InputSystem_SetContext(int context);
int InputSystem_GetContext();

// Quick access
bool InputSystem_IsKeyDown(int key);
bool InputSystem_WasKeyPressed(int key);
bool InputSystem_IsActionActive(int action);
bool InputSystem_WasActionTriggered(int action);

int InputSystem_GetMouseX();
int InputSystem_GetMouseY();
bool InputSystem_WasLeftClick();
bool InputSystem_WasRightClick();

bool InputSystem_IsShiftDown();
bool InputSystem_IsCtrlDown();
bool InputSystem_IsAltDown();

#endif // INPUT_INTEGRATION_H
