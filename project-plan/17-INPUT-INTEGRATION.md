# Phase 17: Input Integration

## Overview

This phase connects the platform input system to game controls, enabling keyboard and mouse interaction.

---

## Platform Input API

### Keyboard Functions

```c
// Initialization
int Platform_Input_Init(void);
void Platform_Input_Shutdown(void);
void Platform_Input_Update(void);

// Key state queries
bool Platform_Key_IsPressed(int key_code);
bool Platform_Key_JustPressed(int key_code);
bool Platform_Key_JustReleased(int key_code);

// Modifier keys
bool Platform_Key_ShiftDown(void);
bool Platform_Key_CtrlDown(void);
bool Platform_Key_AltDown(void);

// Key buffer (for text input)
int Platform_Key_GetBuffered(void);
void Platform_Key_ClearBuffer(void);
```

### Mouse Functions

```c
// Position
void Platform_Mouse_GetPosition(int32_t* x, int32_t* y);
void Platform_Mouse_SetPosition(int32_t x, int32_t y);

// Buttons
bool Platform_Mouse_IsPressed(int button);
bool Platform_Mouse_JustPressed(int button);
bool Platform_Mouse_JustReleased(int button);
bool Platform_Mouse_WasDoubleClicked(int button);

// Wheel
int Platform_Mouse_GetWheelDelta(void);

// Button constants
#define MOUSE_BUTTON_LEFT   0
#define MOUSE_BUTTON_RIGHT  1
#define MOUSE_BUTTON_MIDDLE 2
```

---

## Task 17.1: Key Code Mapping

Map platform key codes to game actions.

```cpp
// src/game/input_map.h
#pragma once

// Game actions
enum GameAction {
    ACTION_NONE = 0,

    // Camera
    ACTION_SCROLL_UP,
    ACTION_SCROLL_DOWN,
    ACTION_SCROLL_LEFT,
    ACTION_SCROLL_RIGHT,
    ACTION_SCROLL_FAST,    // Shift modifier

    // Selection
    ACTION_SELECT_ALL,     // Ctrl+A
    ACTION_SELECT_TYPE,    // T - select same type
    ACTION_ADD_SELECT,     // Shift+click
    ACTION_DESELECT_ALL,   // ESC

    // Orders
    ACTION_STOP,           // S
    ACTION_GUARD,          // G
    ACTION_ATTACK_MOVE,    // A
    ACTION_FORCE_FIRE,     // Ctrl+click
    ACTION_SCATTER,        // X

    // Groups
    ACTION_GROUP_1,        // 1
    ACTION_GROUP_2,        // 2
    // ... through 0
    ACTION_CREATE_GROUP,   // Ctrl+1-0
    ACTION_ADD_TO_GROUP,   // Shift+1-0

    // Building
    ACTION_SELL,           // Z or sidebar click
    ACTION_REPAIR,         // R or sidebar click
    ACTION_POWER,          // P - toggle power

    // UI
    ACTION_TOGGLE_RADAR,   // Tab
    ACTION_TOGGLE_SIDEBAR, // F1
    ACTION_OPTIONS_MENU,   // ESC
    ACTION_DIPLOMACY,      // F2
    ACTION_ALLIANCE,       // A

    // Debug (if enabled)
    ACTION_DEBUG_REVEAL,
    ACTION_DEBUG_MONEY,
};

class InputMapper {
public:
    static InputMapper& Instance();

    void ProcessFrame();

    bool IsActionActive(GameAction action);
    bool WasActionTriggered(GameAction action);

    // Rebindable keys (future)
    void BindKey(int key_code, GameAction action);

private:
    bool actions_active_[256];
    bool actions_triggered_[256];

    void UpdateKeyboardActions();
    void UpdateMouseActions();
};
```

---

## Task 17.2: Keyboard Input Handler

```cpp
// src/game/keyboard_input.cpp
#include "input_map.h"
#include "platform.h"

void InputMapper::UpdateKeyboardActions() {
    // Clear triggered states
    memset(actions_triggered_, 0, sizeof(actions_triggered_));

    // Check for escape
    if (Platform_Key_JustPressed(KEY_CODE_ESCAPE)) {
        actions_triggered_[ACTION_OPTIONS_MENU] = true;
    }

    // Scroll keys (arrow keys)
    actions_active_[ACTION_SCROLL_UP] = Platform_Key_IsPressed(KEY_CODE_UP);
    actions_active_[ACTION_SCROLL_DOWN] = Platform_Key_IsPressed(KEY_CODE_DOWN);
    actions_active_[ACTION_SCROLL_LEFT] = Platform_Key_IsPressed(KEY_CODE_LEFT);
    actions_active_[ACTION_SCROLL_RIGHT] = Platform_Key_IsPressed(KEY_CODE_RIGHT);
    actions_active_[ACTION_SCROLL_FAST] = Platform_Key_ShiftDown();

    // Selection
    if (Platform_Key_CtrlDown() && Platform_Key_JustPressed(KEY_CODE_A)) {
        actions_triggered_[ACTION_SELECT_ALL] = true;
    }
    if (Platform_Key_JustPressed(KEY_CODE_T)) {
        actions_triggered_[ACTION_SELECT_TYPE] = true;
    }

    // Orders
    if (Platform_Key_JustPressed(KEY_CODE_S)) {
        actions_triggered_[ACTION_STOP] = true;
    }
    if (Platform_Key_JustPressed(KEY_CODE_G)) {
        actions_triggered_[ACTION_GUARD] = true;
    }
    if (Platform_Key_JustPressed(KEY_CODE_X)) {
        actions_triggered_[ACTION_SCATTER] = true;
    }

    // Groups (1-0)
    for (int i = 0; i < 10; i++) {
        int key = (i == 0) ? KEY_CODE_0 : (KEY_CODE_1 + i - 1);
        if (Platform_Key_JustPressed(key)) {
            if (Platform_Key_CtrlDown()) {
                actions_triggered_[ACTION_CREATE_GROUP + i] = true;
            } else {
                actions_triggered_[ACTION_GROUP_1 + i] = true;
            }
        }
    }

    // Building shortcuts
    if (Platform_Key_JustPressed(KEY_CODE_Z)) {
        actions_triggered_[ACTION_SELL] = true;
    }
    if (Platform_Key_JustPressed(KEY_CODE_R)) {
        actions_triggered_[ACTION_REPAIR] = true;
    }

    // UI toggles
    if (Platform_Key_JustPressed(KEY_CODE_TAB)) {
        actions_triggered_[ACTION_TOGGLE_RADAR] = true;
    }
}
```

---

## Task 17.3: Mouse Input Handler

```cpp
// src/game/mouse_input.cpp
#include "platform.h"
#include "viewport.h"

struct MouseState {
    int screen_x;      // Screen coordinates
    int screen_y;
    int world_x;       // World coordinates
    int world_y;
    int cell_x;        // Cell coordinates
    int cell_y;

    bool left_down;
    bool right_down;
    bool left_clicked;
    bool right_clicked;
    bool left_double;

    bool is_dragging;
    int drag_start_x;
    int drag_start_y;
    int drag_current_x;
    int drag_current_y;
};

static MouseState Mouse;

void Update_Mouse_State() {
    // Get raw position (window coords, need to scale)
    int32_t raw_x, raw_y;
    Platform_Mouse_GetPosition(&raw_x, &raw_y);

    // Account for window scale (2x)
    Mouse.screen_x = raw_x / 2;
    Mouse.screen_y = raw_y / 2;

    // Convert to world coordinates
    Mouse.world_x = Mouse.screen_x + TheViewport.x;
    Mouse.world_y = Mouse.screen_y + TheViewport.y;

    // Convert to cell coordinates
    Mouse.cell_x = Mouse.world_x / TILE_WIDTH;
    Mouse.cell_y = Mouse.world_y / TILE_HEIGHT;

    // Button states
    bool prev_left = Mouse.left_down;
    Mouse.left_down = Platform_Mouse_IsPressed(MOUSE_BUTTON_LEFT);
    Mouse.right_down = Platform_Mouse_IsPressed(MOUSE_BUTTON_RIGHT);

    Mouse.left_clicked = Platform_Mouse_JustPressed(MOUSE_BUTTON_LEFT);
    Mouse.right_clicked = Platform_Mouse_JustPressed(MOUSE_BUTTON_RIGHT);
    Mouse.left_double = Platform_Mouse_WasDoubleClicked(MOUSE_BUTTON_LEFT);

    // Drag tracking
    if (Mouse.left_clicked) {
        Mouse.drag_start_x = Mouse.screen_x;
        Mouse.drag_start_y = Mouse.screen_y;
        Mouse.is_dragging = false;
    }

    if (Mouse.left_down) {
        Mouse.drag_current_x = Mouse.screen_x;
        Mouse.drag_current_y = Mouse.screen_y;

        // Start drag if moved enough
        int dx = abs(Mouse.drag_current_x - Mouse.drag_start_x);
        int dy = abs(Mouse.drag_current_y - Mouse.drag_start_y);
        if (dx > 4 || dy > 4) {
            Mouse.is_dragging = true;
        }
    } else {
        Mouse.is_dragging = false;
    }
}
```

---

## Task 17.4: Selection System

```cpp
// src/game/selection.cpp
#include "object.h"
#include <vector>

class SelectionManager {
public:
    static SelectionManager& Instance();

    // Selection operations
    void Clear();
    void Select(ObjectClass* obj);
    void AddToSelection(ObjectClass* obj);
    void RemoveFromSelection(ObjectClass* obj);
    void SelectAll(RTTIType type);
    void SelectInBox(int x1, int y1, int x2, int y2);

    // Query
    bool IsSelected(ObjectClass* obj) const;
    int Count() const { return selected_.size(); }
    const std::vector<ObjectClass*>& GetSelected() const { return selected_; }

    // Group management
    void CreateGroup(int group_num);
    void SelectGroup(int group_num);
    void AddToGroup(int group_num);

private:
    std::vector<ObjectClass*> selected_;
    std::vector<ObjectClass*> groups_[10];
};

void SelectionManager::SelectInBox(int x1, int y1, int x2, int y2) {
    // Normalize box
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);

    Clear();

    // Find all objects in box
    for (auto* obj : AllObjects) {
        if (obj->IsActive && obj->Owner == PlayerHouse) {
            int ox = Coord_X(obj->Coord) - TheViewport.x;
            int oy = Coord_Y(obj->Coord) - TheViewport.y;

            if (ox >= x1 && ox <= x2 && oy >= y1 && oy <= y2) {
                AddToSelection(obj);
            }
        }
    }
}
```

---

## Task 17.5: Command System

```cpp
// src/game/commands.cpp
#include "selection.h"
#include "object.h"

enum CommandType {
    CMD_MOVE,
    CMD_ATTACK,
    CMD_GUARD,
    CMD_STOP,
    CMD_ENTER,
    CMD_HARVEST,
    CMD_REPAIR,
    CMD_SELL,
};

void Issue_Command(CommandType cmd, int target_x, int target_y,
                   ObjectClass* target_obj = nullptr) {

    auto& selection = SelectionManager::Instance();
    if (selection.Count() == 0) return;

    for (auto* obj : selection.GetSelected()) {
        switch (cmd) {
            case CMD_MOVE:
                obj->Assign_Mission(MISSION_MOVE);
                obj->Assign_Target(Coord_From_XY(target_x, target_y));
                break;

            case CMD_ATTACK:
                if (target_obj) {
                    obj->Assign_Mission(MISSION_ATTACK);
                    obj->Assign_Target(target_obj);
                }
                break;

            case CMD_GUARD:
                obj->Assign_Mission(MISSION_GUARD);
                break;

            case CMD_STOP:
                obj->Assign_Mission(MISSION_STOP);
                break;

            // ... other commands
        }
    }
}

void Process_Mouse_Command() {
    if (!Mouse.right_clicked) return;

    // Get object at mouse position
    ObjectClass* target = Find_Object_At(Mouse.cell_x, Mouse.cell_y);

    if (target && target->Owner != PlayerHouse) {
        // Enemy - attack
        Issue_Command(CMD_ATTACK, Mouse.world_x, Mouse.world_y, target);
    } else if (target && target->Owner == PlayerHouse) {
        // Friendly - context action (enter, guard, etc.)
        Issue_Command(CMD_ENTER, Mouse.world_x, Mouse.world_y, target);
    } else {
        // Empty - move
        Issue_Command(CMD_MOVE, Mouse.world_x, Mouse.world_y);
    }
}
```

---

## Task 17.6: Edge Scrolling

```cpp
// src/game/edge_scroll.cpp

const int EDGE_SCROLL_ZONE = 8;  // Pixels from edge
const int SCROLL_SPEED = 8;
const int SCROLL_SPEED_FAST = 24;

void Process_Edge_Scroll() {
    int scroll_x = 0;
    int scroll_y = 0;

    // Check screen edges
    if (Mouse.screen_x < EDGE_SCROLL_ZONE) {
        scroll_x = -1;
    } else if (Mouse.screen_x > SCREEN_WIDTH - EDGE_SCROLL_ZONE) {
        scroll_x = 1;
    }

    if (Mouse.screen_y < EDGE_SCROLL_ZONE) {
        scroll_y = -1;
    } else if (Mouse.screen_y > SCREEN_HEIGHT - EDGE_SCROLL_ZONE) {
        scroll_y = 1;
    }

    // Also check keyboard
    if (InputMapper::Instance().IsActionActive(ACTION_SCROLL_LEFT)) {
        scroll_x = -1;
    }
    if (InputMapper::Instance().IsActionActive(ACTION_SCROLL_RIGHT)) {
        scroll_x = 1;
    }
    if (InputMapper::Instance().IsActionActive(ACTION_SCROLL_UP)) {
        scroll_y = -1;
    }
    if (InputMapper::Instance().IsActionActive(ACTION_SCROLL_DOWN)) {
        scroll_y = 1;
    }

    // Apply scroll
    if (scroll_x != 0 || scroll_y != 0) {
        int speed = SCROLL_SPEED;
        if (InputMapper::Instance().IsActionActive(ACTION_SCROLL_FAST)) {
            speed = SCROLL_SPEED_FAST;
        }

        TheViewport.Scroll(scroll_x * speed, scroll_y * speed);
    }
}
```

---

## Task 17.7: Input Integration

```cpp
// src/game/input.cpp
#include "platform.h"
#include "input_map.h"

void Input_Init() {
    Platform_Input_Init();
}

void Input_Update() {
    // Update platform input state
    Platform_Input_Update();

    // Update mouse state
    Update_Mouse_State();

    // Update keyboard actions
    InputMapper::Instance().ProcessFrame();
}

void Input_Process() {
    // Process edge scrolling
    Process_Edge_Scroll();

    // Process selection
    if (Mouse.left_clicked && !Mouse.is_dragging) {
        // Click select
        ObjectClass* obj = Find_Object_At(Mouse.cell_x, Mouse.cell_y);
        if (obj) {
            if (Platform_Key_ShiftDown()) {
                SelectionManager::Instance().AddToSelection(obj);
            } else {
                SelectionManager::Instance().Clear();
                SelectionManager::Instance().Select(obj);
            }
        } else {
            SelectionManager::Instance().Clear();
        }
    }

    // Process drag selection
    if (Mouse.is_dragging && !Mouse.left_down) {
        // Finished drag - select in box
        SelectionManager::Instance().SelectInBox(
            Mouse.drag_start_x, Mouse.drag_start_y,
            Mouse.drag_current_x, Mouse.drag_current_y
        );
    }

    // Process commands
    Process_Mouse_Command();

    // Process keyboard actions
    if (InputMapper::Instance().WasActionTriggered(ACTION_STOP)) {
        Issue_Command(CMD_STOP, 0, 0);
    }
    if (InputMapper::Instance().WasActionTriggered(ACTION_GUARD)) {
        Issue_Command(CMD_GUARD, 0, 0);
    }

    // Groups
    for (int i = 0; i < 10; i++) {
        if (InputMapper::Instance().WasActionTriggered(ACTION_GROUP_1 + i)) {
            SelectionManager::Instance().SelectGroup(i);
        }
        if (InputMapper::Instance().WasActionTriggered(ACTION_CREATE_GROUP + i)) {
            SelectionManager::Instance().CreateGroup(i);
        }
    }
}

void Input_Shutdown() {
    Platform_Input_Shutdown();
}
```

---

## Testing

```cpp
// test_input.cpp

void Test_Keyboard() {
    printf("Press keys to test (ESC to exit):\n");

    while (true) {
        Platform_Input_Update();

        if (Platform_Key_JustPressed(KEY_CODE_ESCAPE)) break;

        for (int k = 0; k < 256; k++) {
            if (Platform_Key_JustPressed(k)) {
                printf("Key pressed: %d\n", k);
            }
        }

        Platform_Timer_Delay(16);
    }
}

void Test_Mouse() {
    printf("Move mouse and click (ESC to exit):\n");

    while (true) {
        Platform_Input_Update();

        if (Platform_Key_JustPressed(KEY_CODE_ESCAPE)) break;

        int32_t x, y;
        Platform_Mouse_GetPosition(&x, &y);

        printf("\rMouse: (%4d, %4d) L:%d R:%d",
               x, y,
               Platform_Mouse_IsPressed(MOUSE_BUTTON_LEFT),
               Platform_Mouse_IsPressed(MOUSE_BUTTON_RIGHT));

        Platform_Timer_Delay(16);
    }
    printf("\n");
}
```

---

## Success Criteria

- [ ] Keyboard input detected correctly
- [ ] Mouse position tracked accurately
- [ ] Click and drag selection works
- [ ] Edge scrolling functional
- [ ] Keyboard shortcuts trigger actions
- [ ] Unit groups work (Ctrl+1, etc.)
- [ ] Commands issued to selected units
- [ ] No input lag or missed events

---

## Estimated Effort

| Task | Hours |
|------|-------|
| 17.1 Key Mapping | 2-3 |
| 17.2 Keyboard Handler | 2-3 |
| 17.3 Mouse Handler | 2-3 |
| 17.4 Selection System | 3-4 |
| 17.5 Command System | 3-4 |
| 17.6 Edge Scrolling | 1-2 |
| 17.7 Integration | 2-3 |
| Testing | 2-3 |
| **Total** | **17-25** |
