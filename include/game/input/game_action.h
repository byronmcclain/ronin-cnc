// include/game/input/game_action.h
// Game action definitions
// Task 16b - GameAction Mapping System

#ifndef GAME_ACTION_H
#define GAME_ACTION_H

#include <cstdint>

//=============================================================================
// GameAction Enumeration
//=============================================================================
// All possible game actions that can be triggered by input.
// Actions are grouped by category for clarity.

enum class GameAction : uint16_t {
    NONE = 0,

    //=========================================================================
    // Camera/Scroll Actions (continuous while held)
    //=========================================================================
    SCROLL_UP,
    SCROLL_DOWN,
    SCROLL_LEFT,
    SCROLL_RIGHT,
    SCROLL_FAST,        // Shift modifier - doubles scroll speed

    //=========================================================================
    // Selection Actions
    //=========================================================================
    SELECT_CLICK,       // Left click to select
    SELECT_ADD,         // Shift+click to add to selection
    SELECT_BOX,         // Drag to box select
    SELECT_ALL,         // Ctrl+A - select all units
    SELECT_TYPE,        // T - select all of same type
    DESELECT_ALL,       // Escape - clear selection

    //=========================================================================
    // Unit Order Actions
    //=========================================================================
    ORDER_STOP,         // S - stop all movement/actions
    ORDER_GUARD,        // G - guard current position
    ORDER_ATTACK_MOVE,  // A - move and attack enemies en route
    ORDER_SCATTER,      // X - scatter units randomly
    ORDER_DEPLOY,       // D - deploy (MCV, etc.)
    ORDER_FORCE_FIRE,   // Ctrl+click - attack ground/friendlies
    ORDER_FORCE_MOVE,   // Alt+click - move ignoring obstacles

    //=========================================================================
    // Control Group Actions (0-9)
    //=========================================================================
    GROUP_SELECT_1,     // 1 - select group 1
    GROUP_SELECT_2,     // 2
    GROUP_SELECT_3,     // 3
    GROUP_SELECT_4,     // 4
    GROUP_SELECT_5,     // 5
    GROUP_SELECT_6,     // 6
    GROUP_SELECT_7,     // 7
    GROUP_SELECT_8,     // 8
    GROUP_SELECT_9,     // 9
    GROUP_SELECT_0,     // 0 - select group 10

    GROUP_CREATE_1,     // Ctrl+1 - assign selection to group 1
    GROUP_CREATE_2,     // Ctrl+2
    GROUP_CREATE_3,     // Ctrl+3
    GROUP_CREATE_4,     // Ctrl+4
    GROUP_CREATE_5,     // Ctrl+5
    GROUP_CREATE_6,     // Ctrl+6
    GROUP_CREATE_7,     // Ctrl+7
    GROUP_CREATE_8,     // Ctrl+8
    GROUP_CREATE_9,     // Ctrl+9
    GROUP_CREATE_0,     // Ctrl+0

    GROUP_ADD_1,        // Shift+1 - add selection to group 1
    GROUP_ADD_2,        // Shift+2
    GROUP_ADD_3,        // Shift+3
    GROUP_ADD_4,        // Shift+4
    GROUP_ADD_5,        // Shift+5
    GROUP_ADD_6,        // Shift+6
    GROUP_ADD_7,        // Shift+7
    GROUP_ADD_8,        // Shift+8
    GROUP_ADD_9,        // Shift+9
    GROUP_ADD_0,        // Shift+0

    //=========================================================================
    // Building/Base Actions
    //=========================================================================
    BUILDING_SELL,      // Z - enter sell mode
    BUILDING_REPAIR,    // R - enter repair mode
    BUILDING_POWER,     // P - toggle power
    BUILDING_PLACE,     // Left click - place building
    BUILDING_CANCEL,    // Right click or Escape - cancel placement

    //=========================================================================
    // UI Actions
    //=========================================================================
    UI_TOGGLE_RADAR,    // Tab - toggle radar on/off
    UI_TOGGLE_SIDEBAR,  // F1 - toggle sidebar
    UI_OPTIONS_MENU,    // Escape - open options menu
    UI_DIPLOMACY,       // F2 - open diplomacy
    UI_ALLIANCE,        // A (in diplomacy) - toggle alliance
    UI_PAUSE,           // P or Pause - pause game

    //=========================================================================
    // Map Actions
    //=========================================================================
    MAP_CENTER_BASE,    // H - center view on base/MCV
    MAP_CENTER_UNIT,    // Spacebar - center on selected unit
    MAP_BOOKMARK_1,     // F5 - go to bookmark 1
    MAP_BOOKMARK_2,     // F6
    MAP_BOOKMARK_3,     // F7
    MAP_BOOKMARK_4,     // F8
    MAP_SET_BOOKMARK_1, // Ctrl+F5 - set bookmark 1
    MAP_SET_BOOKMARK_2, // Ctrl+F6
    MAP_SET_BOOKMARK_3, // Ctrl+F7
    MAP_SET_BOOKMARK_4, // Ctrl+F8

    //=========================================================================
    // Debug Actions (only active if debug enabled)
    //=========================================================================
    DEBUG_REVEAL_MAP,   // Ctrl+R - reveal entire map
    DEBUG_ADD_MONEY,    // Ctrl+M - add 10000 credits
    DEBUG_INSTANT_BUILD,// Ctrl+B - instant building
    DEBUG_GOD_MODE,     // Ctrl+G - invulnerability

    //=========================================================================
    // Meta
    //=========================================================================
    ACTION_COUNT        // Total number of actions
};

//=============================================================================
// Helper Functions
//=============================================================================

// Get the group number (0-9) from a group action
// Returns -1 if not a group action
inline int GetGroupNumber(GameAction action) {
    if (action >= GameAction::GROUP_SELECT_1 && action <= GameAction::GROUP_SELECT_0) {
        int idx = static_cast<int>(action) - static_cast<int>(GameAction::GROUP_SELECT_1);
        return (idx == 9) ? 0 : (idx + 1);  // 0 is stored at index 9
    }
    if (action >= GameAction::GROUP_CREATE_1 && action <= GameAction::GROUP_CREATE_0) {
        int idx = static_cast<int>(action) - static_cast<int>(GameAction::GROUP_CREATE_1);
        return (idx == 9) ? 0 : (idx + 1);
    }
    if (action >= GameAction::GROUP_ADD_1 && action <= GameAction::GROUP_ADD_0) {
        int idx = static_cast<int>(action) - static_cast<int>(GameAction::GROUP_ADD_1);
        return (idx == 9) ? 0 : (idx + 1);
    }
    return -1;
}

// Check if action is a scroll action
inline bool IsScrollAction(GameAction action) {
    return action >= GameAction::SCROLL_UP && action <= GameAction::SCROLL_FAST;
}

// Check if action is a group select action
inline bool IsGroupSelectAction(GameAction action) {
    return action >= GameAction::GROUP_SELECT_1 && action <= GameAction::GROUP_SELECT_0;
}

// Check if action is a group create action
inline bool IsGroupCreateAction(GameAction action) {
    return action >= GameAction::GROUP_CREATE_1 && action <= GameAction::GROUP_CREATE_0;
}

// Check if action is a group add action
inline bool IsGroupAddAction(GameAction action) {
    return action >= GameAction::GROUP_ADD_1 && action <= GameAction::GROUP_ADD_0;
}

// Check if action is any group action
inline bool IsGroupAction(GameAction action) {
    return IsGroupSelectAction(action) || IsGroupCreateAction(action) || IsGroupAddAction(action);
}

// Check if action is a debug action
inline bool IsDebugAction(GameAction action) {
    return action >= GameAction::DEBUG_REVEAL_MAP && action <= GameAction::DEBUG_GOD_MODE;
}

// Get action name for debugging
const char* GetActionName(GameAction action);

#endif // GAME_ACTION_H
