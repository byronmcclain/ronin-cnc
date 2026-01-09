# Task 16b: GameAction Mapping System

| Field | Value |
|-------|-------|
| **Task ID** | 16b |
| **Task Name** | GameAction Mapping System |
| **Phase** | 16 - Input Integration |
| **Depends On** | 16a (Input Foundation) |
| **Estimated Complexity** | Medium (~500 lines) |

---

## Dependencies

- Task 16a (Input Foundation) must be complete
- `InputState` class must be functional
- Key code definitions must be in place

---

## Context

This task creates the mapping layer between raw input (keys, buttons) and game-meaningful actions. Instead of game code checking "is KEY_S pressed?", it checks "is ACTION_STOP triggered?". This provides several benefits:

1. **Abstraction**: Game logic doesn't care which key does what
2. **Rebinding**: Keys can be remapped without changing game code
3. **Combo Detection**: Modifier+key combinations handled cleanly
4. **State Machine**: Actions have "active" (held) vs "triggered" (just pressed) states
5. **Conflict Resolution**: Prevents key conflicts (e.g., Ctrl+S vs bare S)

The original Red Alert hardcoded key checks throughout the code. Our implementation provides a cleaner architecture that could support future key remapping.

---

## Objective

Create a `GameAction` enumeration and `InputMapper` class that:
1. Defines all game actions (camera, selection, orders, groups, UI)
2. Maps input combinations to actions
3. Distinguishes between "active" (held) and "triggered" (just pressed) states
4. Handles modifier key combinations correctly
5. Provides a clean API for game code to query

---

## Technical Background

### Action Categories

Game actions fall into several categories:

```
CAMERA ACTIONS:
  - Scroll (continuous while key held)
  - Fast scroll modifier
  - Jump to location

SELECTION ACTIONS:
  - Click select (triggered on click)
  - Add to selection (Shift modifier)
  - Select all (Ctrl+A)
  - Select same type (T key)
  - Deselect all (Escape)

ORDER ACTIONS:
  - Stop (S)
  - Guard (G)
  - Attack Move (A)
  - Scatter (X)
  - Force fire (Ctrl+click)

GROUP ACTIONS:
  - Select group 1-0 (number keys)
  - Create group (Ctrl+number)
  - Add to group (Shift+number)

BUILDING ACTIONS:
  - Sell mode (Z)
  - Repair mode (R)
  - Power toggle (P)

UI ACTIONS:
  - Toggle radar (Tab)
  - Options menu (Escape)
  - Diplomacy (F2)
```

### State Types

Each action has two query types:

```cpp
// "Active" - currently held (for continuous actions like scrolling)
bool IsActionActive(GameAction action);

// "Triggered" - just pressed this frame (for one-shot actions)
bool WasActionTriggered(GameAction action);
```

### Priority and Conflicts

Some keys have multiple meanings:

- **Escape**: Opens options OR deselects (context dependent)
- **A**: Attack move OR alliance menu (context dependent)
- **S**: Stop order (not Ctrl+S which is unused)

The mapper handles priority by checking modifier state first.

---

## Deliverables

- [ ] `src/game/input/game_action.h` - GameAction enum and related types
- [ ] `src/game/input/input_mapper.h` - InputMapper class declaration
- [ ] `src/game/input/input_mapper.cpp` - InputMapper implementation
- [ ] `tests/test_input_mapper.cpp` - Test program
- [ ] CMakeLists.txt updated
- [ ] All tests pass

---

## Files to Create

### src/game/input/game_action.h

```cpp
// src/game/input/game_action.h
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
```

### src/game/input/input_mapper.h

```cpp
// src/game/input/input_mapper.h
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
```

### src/game/input/input_mapper.cpp

```cpp
// src/game/input/input_mapper.cpp
// Input to GameAction mapping implementation
// Task 16b - GameAction Mapping System

#include "input_mapper.h"
#include "input_state.h"
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

    Platform_Log("InputMapper: Initializing...");

    SetupDefaultBindings();

    initialized_ = true;
    Platform_Log("InputMapper: Initialized with default bindings");
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
    Bind(GameAction::SCROLL_FAST,  KEY_LSHIFT, MOD_NONE, MOD_NONE, true);

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
    Bind(GameAction::UI_TOGGLE_RADAR,  KEY_TAB, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);
    Bind(GameAction::UI_TOGGLE_SIDEBAR, KEY_F1, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);
    Bind(GameAction::UI_OPTIONS_MENU,  KEY_ESCAPE, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);
    Bind(GameAction::UI_DIPLOMACY,     KEY_F2, MOD_NONE, MOD_CTRL | MOD_SHIFT, false);
    Bind(GameAction::UI_PAUSE,         KEY_P, MOD_CTRL, MOD_SHIFT, false);

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
    Bind(GameAction::DEBUG_REVEAL_MAP,   KEY_R, MOD_CTRL | MOD_SHIFT, MOD_NONE, false);
    Bind(GameAction::DEBUG_ADD_MONEY,    KEY_M, MOD_CTRL | MOD_SHIFT, MOD_NONE, false);
    Bind(GameAction::DEBUG_INSTANT_BUILD, KEY_B, MOD_CTRL | MOD_SHIFT, MOD_NONE, false);
    Bind(GameAction::DEBUG_GOD_MODE,     KEY_G, MOD_CTRL | MOD_SHIFT, MOD_NONE, false);
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
```

### tests/test_input_mapper.cpp

```cpp
// tests/test_input_mapper.cpp
// Test program for input mapper
// Task 16b - GameAction Mapping System

#include "input_mapper.h"
#include "input_state.h"
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
    Input_Init();

    if (!InputMapper_Init()) {
        printf("FAILED - InputMapper_Init failed\n");
        Input_Shutdown();
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
        Platform_Shutdown();
        return false;
    }

    InputMapper_Shutdown();
    Input_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_ModifierFiltering() {
    printf("Test: Modifier Filtering... ");

    // This tests that Ctrl+A triggers SELECT_ALL but not ORDER_ATTACK_MOVE
    // We can't easily simulate key presses, so just verify bindings are correct

    InputMapper& mapper = InputMapper::Instance();

    Platform_Init();
    Input_Init();
    InputMapper_Init();

    // Check SELECT_ALL requires Ctrl
    const KeyBinding* select_all = mapper.GetBinding(GameAction::SELECT_ALL);
    if (select_all->required_mods != MOD_CTRL) {
        printf("FAILED - SELECT_ALL should require CTRL\n");
        InputMapper_Shutdown();
        Input_Shutdown();
        Platform_Shutdown();
        return false;
    }

    // Check ORDER_ATTACK_MOVE excludes Ctrl
    const KeyBinding* attack_move = mapper.GetBinding(GameAction::ORDER_ATTACK_MOVE);
    if ((attack_move->excluded_mods & MOD_CTRL) == 0) {
        printf("FAILED - ORDER_ATTACK_MOVE should exclude CTRL\n");
        InputMapper_Shutdown();
        Input_Shutdown();
        Platform_Shutdown();
        return false;
    }

    InputMapper_Shutdown();
    Input_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_DebugActions() {
    printf("Test: Debug Actions Disabled by Default... ");

    Platform_Init();
    Input_Init();
    InputMapper_Init();

    InputMapper& mapper = InputMapper::Instance();

    // Debug should be disabled by default
    if (mapper.IsDebugEnabled()) {
        printf("FAILED - Debug should be disabled by default\n");
        InputMapper_Shutdown();
        Input_Shutdown();
        Platform_Shutdown();
        return false;
    }

    // Enable debug
    mapper.SetDebugEnabled(true);
    if (!mapper.IsDebugEnabled()) {
        printf("FAILED - Debug should be enabled now\n");
        InputMapper_Shutdown();
        Input_Shutdown();
        Platform_Shutdown();
        return false;
    }

    InputMapper_Shutdown();
    Input_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_RebindAction() {
    printf("Test: Rebind Action... ");

    Platform_Init();
    Input_Init();
    InputMapper_Init();

    InputMapper& mapper = InputMapper::Instance();

    // Rebind ORDER_STOP from S to Q
    if (!mapper.RebindAction(GameAction::ORDER_STOP, KEY_Q, MOD_NONE)) {
        printf("FAILED - RebindAction returned false\n");
        InputMapper_Shutdown();
        Input_Shutdown();
        Platform_Shutdown();
        return false;
    }

    const KeyBinding* binding = mapper.GetBinding(GameAction::ORDER_STOP);
    if (binding->key_code != KEY_Q) {
        printf("FAILED - ORDER_STOP not rebound to Q\n");
        InputMapper_Shutdown();
        Input_Shutdown();
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
        Platform_Shutdown();
        return false;
    }

    InputMapper_Shutdown();
    Input_Shutdown();
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
    Input_Init();
    InputMapper_Init();

    // Enable debug for testing
    InputMapper::Instance().SetDebugEnabled(true);

    bool running = true;
    while (running) {
        Platform_PumpEvents();
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

    printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);

    // Interactive test
    if (argc > 1 && strcmp(argv[1], "-i") == 0) {
        Interactive_Test();
    } else {
        printf("\nRun with -i for interactive test\n");
    }

    return (failed == 0) ? 0 : 1;
}
```

---

## CMakeLists.txt Additions

Add to `src/game/CMakeLists.txt`:

```cmake
# Input Mapper (Task 16b)
target_sources(game PRIVATE
    input/game_action.h
    input/input_mapper.h
    input/input_mapper.cpp
)
```

Add to `tests/CMakeLists.txt`:

```cmake
# Input Mapper test (Task 16b)
add_executable(test_input_mapper test_input_mapper.cpp)
target_link_libraries(test_input_mapper PRIVATE game platform)
add_test(NAME InputMapperTest COMMAND test_input_mapper)
```

---

## Verification Script

```bash
#!/bin/bash
set -e

echo "=== Task 16b Verification ==="

# Build
echo "Building..."
cd build
cmake --build . --target test_input_mapper

# Run tests
echo "Running unit tests..."
./tests/test_input_mapper

# Check files exist
echo "Checking source files..."
for file in \
    "../src/game/input/game_action.h" \
    "../src/game/input/input_mapper.h" \
    "../src/game/input/input_mapper.cpp"
do
    if [ ! -f "$file" ]; then
        echo "ERROR: Missing $file"
        exit 1
    fi
done

echo ""
echo "=== Task 16b Verification PASSED ==="
echo "Run './tests/test_input_mapper -i' for interactive test"
```

---

## Implementation Steps

1. **Create game_action.h** - Define GameAction enum and helper functions
2. **Create input_mapper.h** - InputMapper class declaration
3. **Create input_mapper.cpp** - Full implementation with default bindings
4. **Create test_input_mapper.cpp** - Unit tests and interactive demo
5. **Update CMakeLists.txt** - Add new files
6. **Build and run tests** - Verify all pass
7. **Test interactively** - Verify actions trigger correctly

---

## Success Criteria

- [ ] All GameAction values have names
- [ ] GetGroupNumber() returns correct values
- [ ] Action type check functions work correctly
- [ ] InputMapper initializes with default bindings
- [ ] Modifier filtering works (Ctrl+A vs bare A)
- [ ] Debug actions respect enable flag
- [ ] Rebinding and reset work
- [ ] All unit tests pass
- [ ] Interactive test shows correct action triggering

---

## Common Issues

1. **Modifier not detected**
   - Check InputState::GetModifiers() is returning correct flags
   - Verify MOD_* constants match platform definitions

2. **Actions conflicting**
   - Check excluded_mods are set correctly
   - Verify required_mods vs excluded_mods logic

3. **Continuous action not working**
   - Check is_continuous flag is set
   - Verify IsKeyDown vs WasKeyPressed usage

4. **Group numbers off by one**
   - Remember GROUP_SELECT_0 maps to group 0 (tenth group)
   - Indices go 1-9 then 0

---

## Completion Promise

When verification passes, output:
<promise>TASK_16B_COMPLETE</promise>

## Escape Hatch

If stuck after 15 iterations:
- Document what's blocking in `ralph-tasks/blocked/16b.md`
- List attempted approaches
- Output:
<promise>TASK_16B_BLOCKED</promise>

## Max Iterations
15
