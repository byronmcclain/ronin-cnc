// include/game/input/input_mapper.h
// Input to GameAction mapping
// Task 16b - GameAction Mapping System

#ifndef INPUT_MAPPER_H
#define INPUT_MAPPER_H

#include "game_action.h"
#include "input_defs.h"
#include <cstdint>

//=============================================================================
// Key Binding Structure
//=============================================================================

struct KeyBinding {
    int key_code;           // Primary key
    uint8_t required_mods;  // Required modifiers (MOD_CTRL, etc.)
    uint8_t excluded_mods;  // Modifiers that must NOT be present
    bool is_continuous;     // True if action is active while held
};

//=============================================================================
// InputMapper Class
//=============================================================================

class InputMapper {
public:
    // Singleton access
    static InputMapper& Instance();

    // Lifecycle
    bool Initialize();
    void Shutdown();

    // Call once per frame after Input_Update()
    void ProcessFrame();

    //=========================================================================
    // Action State Queries
    //=========================================================================

    // Is action currently active? (for continuous actions like scroll)
    bool IsActionActive(GameAction action) const;

    // Was action just triggered this frame? (for one-shot actions)
    bool WasActionTriggered(GameAction action) const;

    // Was action just released this frame?
    bool WasActionReleased(GameAction action) const;

    // Check any action in a range
    GameAction GetActiveScrollAction() const;
    GameAction GetTriggeredGroupSelectAction() const;
    GameAction GetTriggeredGroupCreateAction() const;
    GameAction GetTriggeredGroupAddAction() const;

    //=========================================================================
    // Configuration
    //=========================================================================

    // Enable/disable debug actions
    void SetDebugEnabled(bool enabled) { debug_enabled_ = enabled; }
    bool IsDebugEnabled() const { return debug_enabled_; }

    // Get current binding for an action
    const KeyBinding* GetBinding(GameAction action) const;

    // Rebind a key (for options menu)
    bool RebindAction(GameAction action, int new_key, uint8_t new_mods = MOD_NONE);

    // Reset to defaults
    void ResetBindings();

    // Check for binding conflicts
    bool HasConflict(int key_code, uint8_t mods, GameAction* conflicting_action = nullptr) const;

private:
    InputMapper();
    ~InputMapper();
    InputMapper(const InputMapper&) = delete;
    InputMapper& operator=(const InputMapper&) = delete;

    void SetupDefaultBindings();
    void UpdateActionStates();

    // Binding storage (indexed by GameAction)
    static constexpr int MAX_ACTIONS = static_cast<int>(GameAction::ACTION_COUNT);
    KeyBinding bindings_[MAX_ACTIONS];

    // Action states
    bool action_active_[MAX_ACTIONS];       // Currently held
    bool action_triggered_[MAX_ACTIONS];    // Just pressed this frame
    bool action_released_[MAX_ACTIONS];     // Just released this frame
    bool action_active_prev_[MAX_ACTIONS];  // Previous frame state

    bool debug_enabled_;
    bool initialized_;
};

//=============================================================================
// Global Access Functions
//=============================================================================

// Initialize the input mapper
bool InputMapper_Init();

// Shutdown
void InputMapper_Shutdown();

// Process frame (call after Input_Update)
void InputMapper_ProcessFrame();

// Query actions
bool InputMapper_IsActive(GameAction action);
bool InputMapper_WasTriggered(GameAction action);
bool InputMapper_WasReleased(GameAction action);

#endif // INPUT_MAPPER_H
