// src/game/input/input_mapper.cpp
// Input to GameAction mapping implementation
// Task 16b - GameAction Mapping System

#include "game/input/input_mapper.h"
#include "game/input/input_state.h"
#include "platform.h"
#include <cstring>

//=============================================================================
// Action Names (for debugging)
//=============================================================================

const char* GetActionName(GameAction action) {
    switch (action) {
        case GameAction::NONE: return "NONE";
        case GameAction::SCROLL_UP: return "SCROLL_UP";
        case GameAction::SCROLL_DOWN: return "SCROLL_DOWN";
        case GameAction::SCROLL_LEFT: return "SCROLL_LEFT";
        case GameAction::SCROLL_RIGHT: return "SCROLL_RIGHT";
        case GameAction::SCROLL_FAST: return "SCROLL_FAST";
        case GameAction::SELECT_CLICK: return "SELECT_CLICK";
        case GameAction::SELECT_ADD: return "SELECT_ADD";
        case GameAction::SELECT_BOX: return "SELECT_BOX";
        case GameAction::SELECT_ALL: return "SELECT_ALL";
        case GameAction::SELECT_TYPE: return "SELECT_TYPE";
        case GameAction::DESELECT_ALL: return "DESELECT_ALL";
        case GameAction::ORDER_STOP: return "ORDER_STOP";
        case GameAction::ORDER_GUARD: return "ORDER_GUARD";
        case GameAction::ORDER_ATTACK_MOVE: return "ORDER_ATTACK_MOVE";
        case GameAction::ORDER_SCATTER: return "ORDER_SCATTER";
        case GameAction::ORDER_DEPLOY: return "ORDER_DEPLOY";
        case GameAction::ORDER_FORCE_FIRE: return "ORDER_FORCE_FIRE";
        case GameAction::ORDER_FORCE_MOVE: return "ORDER_FORCE_MOVE";
        case GameAction::GROUP_SELECT_1: return "GROUP_SELECT_1";
        case GameAction::GROUP_SELECT_2: return "GROUP_SELECT_2";
        case GameAction::GROUP_SELECT_3: return "GROUP_SELECT_3";
        case GameAction::GROUP_SELECT_4: return "GROUP_SELECT_4";
        case GameAction::GROUP_SELECT_5: return "GROUP_SELECT_5";
        case GameAction::GROUP_SELECT_6: return "GROUP_SELECT_6";
        case GameAction::GROUP_SELECT_7: return "GROUP_SELECT_7";
        case GameAction::GROUP_SELECT_8: return "GROUP_SELECT_8";
        case GameAction::GROUP_SELECT_9: return "GROUP_SELECT_9";
        case GameAction::GROUP_SELECT_0: return "GROUP_SELECT_0";
        case GameAction::GROUP_CREATE_1: return "GROUP_CREATE_1";
        case GameAction::GROUP_CREATE_2: return "GROUP_CREATE_2";
        case GameAction::GROUP_CREATE_3: return "GROUP_CREATE_3";
        case GameAction::GROUP_CREATE_4: return "GROUP_CREATE_4";
        case GameAction::GROUP_CREATE_5: return "GROUP_CREATE_5";
        case GameAction::GROUP_CREATE_6: return "GROUP_CREATE_6";
        case GameAction::GROUP_CREATE_7: return "GROUP_CREATE_7";
        case GameAction::GROUP_CREATE_8: return "GROUP_CREATE_8";
        case GameAction::GROUP_CREATE_9: return "GROUP_CREATE_9";
        case GameAction::GROUP_CREATE_0: return "GROUP_CREATE_0";
        case GameAction::GROUP_ADD_1: return "GROUP_ADD_1";
        case GameAction::GROUP_ADD_2: return "GROUP_ADD_2";
        case GameAction::GROUP_ADD_3: return "GROUP_ADD_3";
        case GameAction::GROUP_ADD_4: return "GROUP_ADD_4";
        case GameAction::GROUP_ADD_5: return "GROUP_ADD_5";
        case GameAction::GROUP_ADD_6: return "GROUP_ADD_6";
        case GameAction::GROUP_ADD_7: return "GROUP_ADD_7";
        case GameAction::GROUP_ADD_8: return "GROUP_ADD_8";
        case GameAction::GROUP_ADD_9: return "GROUP_ADD_9";
        case GameAction::GROUP_ADD_0: return "GROUP_ADD_0";
        case GameAction::BUILDING_SELL: return "BUILDING_SELL";
        case GameAction::BUILDING_REPAIR: return "BUILDING_REPAIR";
        case GameAction::BUILDING_POWER: return "BUILDING_POWER";
        case GameAction::BUILDING_PLACE: return "BUILDING_PLACE";
        case GameAction::BUILDING_CANCEL: return "BUILDING_CANCEL";
        case GameAction::UI_TOGGLE_RADAR: return "UI_TOGGLE_RADAR";
        case GameAction::UI_TOGGLE_SIDEBAR: return "UI_TOGGLE_SIDEBAR";
        case GameAction::UI_OPTIONS_MENU: return "UI_OPTIONS_MENU";
        case GameAction::UI_DIPLOMACY: return "UI_DIPLOMACY";
        case GameAction::UI_ALLIANCE: return "UI_ALLIANCE";
        case GameAction::UI_PAUSE: return "UI_PAUSE";
        case GameAction::MAP_CENTER_BASE: return "MAP_CENTER_BASE";
        case GameAction::MAP_CENTER_UNIT: return "MAP_CENTER_UNIT";
        case GameAction::MAP_BOOKMARK_1: return "MAP_BOOKMARK_1";
        case GameAction::MAP_BOOKMARK_2: return "MAP_BOOKMARK_2";
        case GameAction::MAP_BOOKMARK_3: return "MAP_BOOKMARK_3";
        case GameAction::MAP_BOOKMARK_4: return "MAP_BOOKMARK_4";
        case GameAction::MAP_SET_BOOKMARK_1: return "MAP_SET_BOOKMARK_1";
        case GameAction::MAP_SET_BOOKMARK_2: return "MAP_SET_BOOKMARK_2";
        case GameAction::MAP_SET_BOOKMARK_3: return "MAP_SET_BOOKMARK_3";
        case GameAction::MAP_SET_BOOKMARK_4: return "MAP_SET_BOOKMARK_4";
        case GameAction::DEBUG_REVEAL_MAP: return "DEBUG_REVEAL_MAP";
        case GameAction::DEBUG_ADD_MONEY: return "DEBUG_ADD_MONEY";
        case GameAction::DEBUG_INSTANT_BUILD: return "DEBUG_INSTANT_BUILD";
        case GameAction::DEBUG_GOD_MODE: return "DEBUG_GOD_MODE";
        case GameAction::ACTION_COUNT: return "ACTION_COUNT";
        default: return "UNKNOWN";
    }
}

//=============================================================================
// InputMapper Singleton
//=============================================================================

InputMapper& InputMapper::Instance() {
    static InputMapper instance;
    return instance;
}

InputMapper::InputMapper()
    : debug_enabled_(false)
    , initialized_(false) {
    memset(bindings_, 0, sizeof(bindings_));
    memset(action_active_, 0, sizeof(action_active_));
    memset(action_triggered_, 0, sizeof(action_triggered_));
    memset(action_released_, 0, sizeof(action_released_));
    memset(action_active_prev_, 0, sizeof(action_active_prev_));
}

InputMapper::~InputMapper() {
    Shutdown();
}

bool InputMapper::Initialize() {
    if (initialized_) {
        return true;
    }

    Platform_LogInfo("InputMapper: Initializing...");

    SetupDefaultBindings();

    initialized_ = true;
    Platform_LogInfo("InputMapper: Initialized with default bindings");
    return true;
}

void InputMapper::Shutdown() {
    initialized_ = false;
}

void InputMapper::SetupDefaultBindings() {
    // Clear all bindings first
    memset(bindings_, 0, sizeof(bindings_));

    // Helper lambda to set binding
    auto Bind = [this](GameAction action, int key, uint8_t req_mods = MOD_NONE,
                       uint8_t excl_mods = MOD_NONE, bool continuous = false) {
        int idx = static_cast<int>(action);
        bindings_[idx].key_code = key;
        bindings_[idx].required_mods = req_mods;
        bindings_[idx].excluded_mods = excl_mods;
        bindings_[idx].is_continuous = continuous;
    };

    //=========================================================================
    // Scroll Actions (continuous)
    //=========================================================================
    Bind(GameAction::SCROLL_UP,    KEY_UP,    MOD_NONE, MOD_NONE, true);
    Bind(GameAction::SCROLL_DOWN,  KEY_DOWN,  MOD_NONE, MOD_NONE, true);
    Bind(GameAction::SCROLL_LEFT,  KEY_LEFT,  MOD_NONE, MOD_NONE, true);
    Bind(GameAction::SCROLL_RIGHT, KEY_RIGHT, MOD_NONE, MOD_NONE, true);
    Bind(GameAction::SCROLL_FAST,  KEY_SHIFT, MOD_NONE, MOD_NONE, true);

    //=========================================================================
    // Selection Actions
    //=========================================================================
    Bind(GameAction::SELECT_ALL,   KEY_A, MOD_CTRL, MOD_NONE, false);
    Bind(GameAction::SELECT_TYPE,  KEY_T, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);
    Bind(GameAction::DESELECT_ALL, KEY_ESCAPE, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);

    //=========================================================================
    // Order Actions
    //=========================================================================
    Bind(GameAction::ORDER_STOP,        KEY_S, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);
    Bind(GameAction::ORDER_GUARD,       KEY_G, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);
    Bind(GameAction::ORDER_ATTACK_MOVE, KEY_A, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);
    Bind(GameAction::ORDER_SCATTER,     KEY_X, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);
    Bind(GameAction::ORDER_DEPLOY,      KEY_D, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);

    //=========================================================================
    // Group Select (1-9, 0)
    //=========================================================================
    Bind(GameAction::GROUP_SELECT_1, KEY_1, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);
    Bind(GameAction::GROUP_SELECT_2, KEY_2, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);
    Bind(GameAction::GROUP_SELECT_3, KEY_3, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);
    Bind(GameAction::GROUP_SELECT_4, KEY_4, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);
    Bind(GameAction::GROUP_SELECT_5, KEY_5, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);
    Bind(GameAction::GROUP_SELECT_6, KEY_6, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);
    Bind(GameAction::GROUP_SELECT_7, KEY_7, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);
    Bind(GameAction::GROUP_SELECT_8, KEY_8, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);
    Bind(GameAction::GROUP_SELECT_9, KEY_9, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);
    Bind(GameAction::GROUP_SELECT_0, KEY_0, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);

    //=========================================================================
    // Group Create (Ctrl+1-0)
    //=========================================================================
    Bind(GameAction::GROUP_CREATE_1, KEY_1, MOD_CTRL, MOD_SHIFT, false);
    Bind(GameAction::GROUP_CREATE_2, KEY_2, MOD_CTRL, MOD_SHIFT, false);
    Bind(GameAction::GROUP_CREATE_3, KEY_3, MOD_CTRL, MOD_SHIFT, false);
    Bind(GameAction::GROUP_CREATE_4, KEY_4, MOD_CTRL, MOD_SHIFT, false);
    Bind(GameAction::GROUP_CREATE_5, KEY_5, MOD_CTRL, MOD_SHIFT, false);
    Bind(GameAction::GROUP_CREATE_6, KEY_6, MOD_CTRL, MOD_SHIFT, false);
    Bind(GameAction::GROUP_CREATE_7, KEY_7, MOD_CTRL, MOD_SHIFT, false);
    Bind(GameAction::GROUP_CREATE_8, KEY_8, MOD_CTRL, MOD_SHIFT, false);
    Bind(GameAction::GROUP_CREATE_9, KEY_9, MOD_CTRL, MOD_SHIFT, false);
    Bind(GameAction::GROUP_CREATE_0, KEY_0, MOD_CTRL, MOD_SHIFT, false);

    //=========================================================================
    // Group Add (Shift+1-0)
    //=========================================================================
    Bind(GameAction::GROUP_ADD_1, KEY_1, MOD_SHIFT, MOD_CTRL, false);
    Bind(GameAction::GROUP_ADD_2, KEY_2, MOD_SHIFT, MOD_CTRL, false);
    Bind(GameAction::GROUP_ADD_3, KEY_3, MOD_SHIFT, MOD_CTRL, false);
    Bind(GameAction::GROUP_ADD_4, KEY_4, MOD_SHIFT, MOD_CTRL, false);
    Bind(GameAction::GROUP_ADD_5, KEY_5, MOD_SHIFT, MOD_CTRL, false);
    Bind(GameAction::GROUP_ADD_6, KEY_6, MOD_SHIFT, MOD_CTRL, false);
    Bind(GameAction::GROUP_ADD_7, KEY_7, MOD_SHIFT, MOD_CTRL, false);
    Bind(GameAction::GROUP_ADD_8, KEY_8, MOD_SHIFT, MOD_CTRL, false);
    Bind(GameAction::GROUP_ADD_9, KEY_9, MOD_SHIFT, MOD_CTRL, false);
    Bind(GameAction::GROUP_ADD_0, KEY_0, MOD_SHIFT, MOD_CTRL, false);

    //=========================================================================
    // Building Actions
    //=========================================================================
    Bind(GameAction::BUILDING_SELL,   KEY_Z, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);
    Bind(GameAction::BUILDING_REPAIR, KEY_R, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);
    Bind(GameAction::BUILDING_POWER,  KEY_P, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);

    //=========================================================================
    // UI Actions
    //=========================================================================
    Bind(GameAction::UI_TOGGLE_RADAR,   KEY_TAB, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);
    Bind(GameAction::UI_TOGGLE_SIDEBAR, KEY_F1, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);
    Bind(GameAction::UI_OPTIONS_MENU,   KEY_ESCAPE, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);
    Bind(GameAction::UI_DIPLOMACY,      KEY_F2, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);
    Bind(GameAction::UI_PAUSE,          KEY_PAUSE, MOD_NONE, MOD_NONE, false);

    //=========================================================================
    // Map Actions
    //=========================================================================
    Bind(GameAction::MAP_CENTER_BASE, KEY_H, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);
    Bind(GameAction::MAP_CENTER_UNIT, KEY_SPACE, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);
    Bind(GameAction::MAP_BOOKMARK_1, KEY_F5, MOD_NONE, MOD_CTRL, false);
    Bind(GameAction::MAP_BOOKMARK_2, KEY_F6, MOD_NONE, MOD_CTRL, false);
    Bind(GameAction::MAP_BOOKMARK_3, KEY_F7, MOD_NONE, MOD_CTRL, false);
    Bind(GameAction::MAP_BOOKMARK_4, KEY_F8, MOD_NONE, MOD_CTRL, false);
    Bind(GameAction::MAP_SET_BOOKMARK_1, KEY_F5, MOD_CTRL, MOD_NONE, false);
    Bind(GameAction::MAP_SET_BOOKMARK_2, KEY_F6, MOD_CTRL, MOD_NONE, false);
    Bind(GameAction::MAP_SET_BOOKMARK_3, KEY_F7, MOD_CTRL, MOD_NONE, false);
    Bind(GameAction::MAP_SET_BOOKMARK_4, KEY_F8, MOD_CTRL, MOD_NONE, false);

    //=========================================================================
    // Debug Actions
    //=========================================================================
    Bind(GameAction::DEBUG_REVEAL_MAP,    KEY_R, MOD_CTRL | MOD_SHIFT, MOD_NONE, false);
    Bind(GameAction::DEBUG_ADD_MONEY,     KEY_M, MOD_CTRL | MOD_SHIFT, MOD_NONE, false);
    Bind(GameAction::DEBUG_INSTANT_BUILD, KEY_B, MOD_CTRL | MOD_SHIFT, MOD_NONE, false);
    Bind(GameAction::DEBUG_GOD_MODE,      KEY_G, MOD_CTRL | MOD_SHIFT, MOD_NONE, false);
}

void InputMapper::ProcessFrame() {
    if (!initialized_) {
        return;
    }

    // Save previous states
    memcpy(action_active_prev_, action_active_, sizeof(action_active_));

    // Clear triggered/released states
    memset(action_triggered_, 0, sizeof(action_triggered_));
    memset(action_released_, 0, sizeof(action_released_));

    // Update all action states
    UpdateActionStates();
}

void InputMapper::UpdateActionStates() {
    InputState& input = InputState::Instance();
    uint8_t current_mods = input.GetModifiers();

    for (int i = 0; i < MAX_ACTIONS; i++) {
        GameAction action = static_cast<GameAction>(i);

        // Skip debug actions if not enabled
        if (IsDebugAction(action) && !debug_enabled_) {
            action_active_[i] = false;
            continue;
        }

        const KeyBinding& binding = bindings_[i];

        // Skip unbound actions
        if (binding.key_code == 0) {
            action_active_[i] = false;
            continue;
        }

        // Check if required modifiers are present
        bool mods_ok = (current_mods & binding.required_mods) == binding.required_mods;

        // Check if excluded modifiers are absent
        if (binding.excluded_mods != MOD_NONE) {
            if (current_mods & binding.excluded_mods) {
                mods_ok = false;
            }
        }

        // Check key state
        bool key_down = input.IsKeyDown(binding.key_code);
        bool key_pressed = input.WasKeyPressed(binding.key_code);

        if (binding.is_continuous) {
            // Continuous action - active while key held with correct mods
            action_active_[i] = key_down && mods_ok;
        } else {
            // One-shot action - triggered on key press with correct mods
            action_active_[i] = key_pressed && mods_ok;
        }

        // Detect triggered/released
        if (action_active_[i] && !action_active_prev_[i]) {
            action_triggered_[i] = true;
        }
        if (!action_active_[i] && action_active_prev_[i]) {
            action_released_[i] = true;
        }
    }
}

//=============================================================================
// Query Methods
//=============================================================================

bool InputMapper::IsActionActive(GameAction action) const {
    int idx = static_cast<int>(action);
    if (idx < 0 || idx >= MAX_ACTIONS) return false;
    return action_active_[idx];
}

bool InputMapper::WasActionTriggered(GameAction action) const {
    int idx = static_cast<int>(action);
    if (idx < 0 || idx >= MAX_ACTIONS) return false;
    return action_triggered_[idx];
}

bool InputMapper::WasActionReleased(GameAction action) const {
    int idx = static_cast<int>(action);
    if (idx < 0 || idx >= MAX_ACTIONS) return false;
    return action_released_[idx];
}

GameAction InputMapper::GetActiveScrollAction() const {
    if (IsActionActive(GameAction::SCROLL_UP)) return GameAction::SCROLL_UP;
    if (IsActionActive(GameAction::SCROLL_DOWN)) return GameAction::SCROLL_DOWN;
    if (IsActionActive(GameAction::SCROLL_LEFT)) return GameAction::SCROLL_LEFT;
    if (IsActionActive(GameAction::SCROLL_RIGHT)) return GameAction::SCROLL_RIGHT;
    return GameAction::NONE;
}

GameAction InputMapper::GetTriggeredGroupSelectAction() const {
    for (int i = static_cast<int>(GameAction::GROUP_SELECT_1);
         i <= static_cast<int>(GameAction::GROUP_SELECT_0); i++) {
        if (action_triggered_[i]) {
            return static_cast<GameAction>(i);
        }
    }
    return GameAction::NONE;
}

GameAction InputMapper::GetTriggeredGroupCreateAction() const {
    for (int i = static_cast<int>(GameAction::GROUP_CREATE_1);
         i <= static_cast<int>(GameAction::GROUP_CREATE_0); i++) {
        if (action_triggered_[i]) {
            return static_cast<GameAction>(i);
        }
    }
    return GameAction::NONE;
}

GameAction InputMapper::GetTriggeredGroupAddAction() const {
    for (int i = static_cast<int>(GameAction::GROUP_ADD_1);
         i <= static_cast<int>(GameAction::GROUP_ADD_0); i++) {
        if (action_triggered_[i]) {
            return static_cast<GameAction>(i);
        }
    }
    return GameAction::NONE;
}

const KeyBinding* InputMapper::GetBinding(GameAction action) const {
    int idx = static_cast<int>(action);
    if (idx < 0 || idx >= MAX_ACTIONS) return nullptr;
    return &bindings_[idx];
}

bool InputMapper::RebindAction(GameAction action, int new_key, uint8_t new_mods) {
    int idx = static_cast<int>(action);
    if (idx < 0 || idx >= MAX_ACTIONS) return false;

    bindings_[idx].key_code = new_key;
    bindings_[idx].required_mods = new_mods;
    return true;
}

void InputMapper::ResetBindings() {
    SetupDefaultBindings();
}

bool InputMapper::HasConflict(int key_code, uint8_t mods, GameAction* conflicting_action) const {
    for (int i = 0; i < MAX_ACTIONS; i++) {
        if (bindings_[i].key_code == key_code &&
            bindings_[i].required_mods == mods) {
            if (conflicting_action) {
                *conflicting_action = static_cast<GameAction>(i);
            }
            return true;
        }
    }
    return false;
}

//=============================================================================
// Global Functions
//=============================================================================

bool InputMapper_Init() {
    return InputMapper::Instance().Initialize();
}

void InputMapper_Shutdown() {
    InputMapper::Instance().Shutdown();
}

void InputMapper_ProcessFrame() {
    InputMapper::Instance().ProcessFrame();
}

bool InputMapper_IsActive(GameAction action) {
    return InputMapper::Instance().IsActionActive(action);
}

bool InputMapper_WasTriggered(GameAction action) {
    return InputMapper::Instance().WasActionTriggered(action);
}

bool InputMapper_WasReleased(GameAction action) {
    return InputMapper::Instance().WasActionReleased(action);
}
