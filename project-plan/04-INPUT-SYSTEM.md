# Plan 04: Input System

## Objective
Replace Windows input handling (keyboard, mouse) with SDL2 event-based input, maintaining compatibility with the existing game input code.

## Current State Analysis

### Windows Input in Codebase

**Primary Files:**
- `WIN32LIB/KEYBOARD/KEYBOARD.CPP` (534 lines)
- `WIN32LIB/KEYBOARD/KEYBOARD.H`
- `WIN32LIB/KEYBOARD/MOUSE.CPP`
- `CODE/KEYBOARD.CPP`
- `CODE/KEY.CPP`

**Key Classes:**
```cpp
class WWKeyboardClass {
    // Ring buffer for key events
    unsigned short Buffer[256];
    int Head, Tail;

    // Modifier state
    int DownModifiers;  // Shift, Ctrl, Alt state

    // Methods
    bool Put_Key_Message(UINT vk_key, BOOL release, BOOL dbl);
    int Check();
    int Get();
    bool Is_Key_Down(int key);
};
```

**Windows APIs Used:**
- `WM_KEYDOWN`, `WM_KEYUP`, `WM_SYSKEYDOWN`, `WM_SYSKEYUP`
- `WM_MOUSEMOVE`, `WM_LBUTTONDOWN`, `WM_LBUTTONUP`, `WM_RBUTTONDOWN`, `WM_RBUTTONUP`
- `GetKeyState()`, `GetAsyncKeyState()`
- Virtual key codes (`VK_*`)

**Key Features:**
- 256-entry ring buffer for keyboard events
- Mouse position packed with button state
- Modifier keys tracked separately (Shift, Ctrl, Alt)
- Caps Lock and Num Lock state
- Double-click detection

## SDL2 Implementation

### 4.1 Input State Structure

File: `src/platform/input_sdl.cpp`
```cpp
#include "platform/platform_input.h"
#include <SDL.h>
#include <cstring>
#include <queue>

namespace {

//=============================================================================
// Keyboard State
//=============================================================================

struct KeyEvent {
    KeyCode key;
    bool released;
};

// Current frame state
bool g_keys_current[KEY_COUNT] = {false};
bool g_keys_previous[KEY_COUNT] = {false};

// Event queue (ring buffer style)
std::queue<KeyEvent> g_key_queue;

// Modifier state
bool g_shift_down = false;
bool g_ctrl_down = false;
bool g_alt_down = false;
bool g_caps_lock = false;
bool g_num_lock = false;

//=============================================================================
// Mouse State
//=============================================================================

int g_mouse_x = 0;
int g_mouse_y = 0;
int g_mouse_wheel_delta = 0;

bool g_mouse_buttons_current[3] = {false};
bool g_mouse_buttons_previous[3] = {false};
bool g_mouse_clicked[3] = {false};
bool g_mouse_double_clicked[3] = {false};

Uint32 g_last_click_time[3] = {0};
int g_last_click_x[3] = {0};
int g_last_click_y[3] = {0};

const Uint32 DOUBLE_CLICK_TIME = 500;  // ms
const int DOUBLE_CLICK_DISTANCE = 4;    // pixels

//=============================================================================
// Key Code Mapping
//=============================================================================

KeyCode SDLKeyToKeyCode(SDL_Keycode sdl_key) {
    switch (sdl_key) {
        case SDLK_ESCAPE: return KEY_ESCAPE;
        case SDLK_RETURN: return KEY_RETURN;
        case SDLK_SPACE: return KEY_SPACE;
        case SDLK_UP: return KEY_UP;
        case SDLK_DOWN: return KEY_DOWN;
        case SDLK_LEFT: return KEY_LEFT;
        case SDLK_RIGHT: return KEY_RIGHT;
        case SDLK_LSHIFT:
        case SDLK_RSHIFT: return KEY_SHIFT;
        case SDLK_LCTRL:
        case SDLK_RCTRL: return KEY_CONTROL;
        case SDLK_LALT:
        case SDLK_RALT: return KEY_ALT;
        case SDLK_TAB: return (KeyCode)0x09;
        case SDLK_BACKSPACE: return (KeyCode)0x08;
        case SDLK_DELETE: return (KeyCode)0x2E;
        case SDLK_INSERT: return (KeyCode)0x2D;
        case SDLK_HOME: return (KeyCode)0x24;
        case SDLK_END: return (KeyCode)0x23;
        case SDLK_PAGEUP: return (KeyCode)0x21;
        case SDLK_PAGEDOWN: return (KeyCode)0x22;
        // Function keys
        case SDLK_F1: return (KeyCode)0x70;
        case SDLK_F2: return (KeyCode)0x71;
        case SDLK_F3: return (KeyCode)0x72;
        case SDLK_F4: return (KeyCode)0x73;
        case SDLK_F5: return (KeyCode)0x74;
        case SDLK_F6: return (KeyCode)0x75;
        case SDLK_F7: return (KeyCode)0x76;
        case SDLK_F8: return (KeyCode)0x77;
        case SDLK_F9: return (KeyCode)0x78;
        case SDLK_F10: return (KeyCode)0x79;
        case SDLK_F11: return (KeyCode)0x7A;
        case SDLK_F12: return (KeyCode)0x7B;
        // Alphanumeric
        default:
            if (sdl_key >= SDLK_a && sdl_key <= SDLK_z) {
                return (KeyCode)(0x41 + (sdl_key - SDLK_a));  // VK_A = 0x41
            }
            if (sdl_key >= SDLK_0 && sdl_key <= SDLK_9) {
                return (KeyCode)(0x30 + (sdl_key - SDLK_0));  // VK_0 = 0x30
            }
            return KEY_NONE;
    }
}

MouseButton SDLButtonToMouseButton(Uint8 sdl_button) {
    switch (sdl_button) {
        case SDL_BUTTON_LEFT: return MOUSE_LEFT;
        case SDL_BUTTON_RIGHT: return MOUSE_RIGHT;
        case SDL_BUTTON_MIDDLE: return MOUSE_MIDDLE;
        default: return MOUSE_LEFT;
    }
}

} // anonymous namespace
```

### 4.2 Initialization

```cpp
bool Platform_Input_Init(void) {
    // Clear state
    memset(g_keys_current, 0, sizeof(g_keys_current));
    memset(g_keys_previous, 0, sizeof(g_keys_previous));
    memset(g_mouse_buttons_current, 0, sizeof(g_mouse_buttons_current));
    memset(g_mouse_buttons_previous, 0, sizeof(g_mouse_buttons_previous));

    // Get initial modifier state
    SDL_Keymod mod = SDL_GetModState();
    g_caps_lock = (mod & KMOD_CAPS) != 0;
    g_num_lock = (mod & KMOD_NUM) != 0;

    return true;
}

void Platform_Input_Shutdown(void) {
    // Clear key queue
    while (!g_key_queue.empty()) {
        g_key_queue.pop();
    }
}
```

### 4.3 Frame Update (Called from Event Loop)

```cpp
void Platform_Input_Update(void) {
    // Save previous frame state
    memcpy(g_keys_previous, g_keys_current, sizeof(g_keys_current));
    memcpy(g_mouse_buttons_previous, g_mouse_buttons_current, sizeof(g_mouse_buttons_current));

    // Reset per-frame state
    g_mouse_wheel_delta = 0;
    memset(g_mouse_clicked, 0, sizeof(g_mouse_clicked));
    memset(g_mouse_double_clicked, 0, sizeof(g_mouse_double_clicked));
}

// Called for each SDL event
void Platform_Input_HandleEvent(const SDL_Event* event) {
    switch (event->type) {
        case SDL_KEYDOWN: {
            KeyCode key = SDLKeyToKeyCode(event->key.keysym.sym);
            if (key != KEY_NONE && !event->key.repeat) {
                g_keys_current[key] = true;
                g_key_queue.push({key, false});

                // Track modifiers
                if (key == KEY_SHIFT) g_shift_down = true;
                if (key == KEY_CONTROL) g_ctrl_down = true;
                if (key == KEY_ALT) g_alt_down = true;
            }

            // Update caps/num lock
            SDL_Keymod mod = SDL_GetModState();
            g_caps_lock = (mod & KMOD_CAPS) != 0;
            g_num_lock = (mod & KMOD_NUM) != 0;
            break;
        }

        case SDL_KEYUP: {
            KeyCode key = SDLKeyToKeyCode(event->key.keysym.sym);
            if (key != KEY_NONE) {
                g_keys_current[key] = false;
                g_key_queue.push({key, true});

                // Track modifiers
                if (key == KEY_SHIFT) g_shift_down = false;
                if (key == KEY_CONTROL) g_ctrl_down = false;
                if (key == KEY_ALT) g_alt_down = false;
            }
            break;
        }

        case SDL_MOUSEMOTION:
            g_mouse_x = event->motion.x;
            g_mouse_y = event->motion.y;
            break;

        case SDL_MOUSEBUTTONDOWN: {
            MouseButton btn = SDLButtonToMouseButton(event->button.button);
            g_mouse_buttons_current[btn] = true;
            g_mouse_clicked[btn] = true;

            // Double-click detection
            Uint32 now = SDL_GetTicks();
            int dx = abs(event->button.x - g_last_click_x[btn]);
            int dy = abs(event->button.y - g_last_click_y[btn]);

            if (now - g_last_click_time[btn] < DOUBLE_CLICK_TIME &&
                dx < DOUBLE_CLICK_DISTANCE && dy < DOUBLE_CLICK_DISTANCE) {
                g_mouse_double_clicked[btn] = true;
            }

            g_last_click_time[btn] = now;
            g_last_click_x[btn] = event->button.x;
            g_last_click_y[btn] = event->button.y;
            break;
        }

        case SDL_MOUSEBUTTONUP: {
            MouseButton btn = SDLButtonToMouseButton(event->button.button);
            g_mouse_buttons_current[btn] = false;
            break;
        }

        case SDL_MOUSEWHEEL:
            g_mouse_wheel_delta = event->wheel.y;
            break;
    }
}
```

### 4.4 Keyboard Query Functions

```cpp
bool Platform_Key_IsPressed(KeyCode key) {
    if (key < 0 || key >= KEY_COUNT) return false;
    return g_keys_current[key];
}

bool Platform_Key_WasPressed(KeyCode key) {
    if (key < 0 || key >= KEY_COUNT) return false;
    return g_keys_current[key] && !g_keys_previous[key];
}

bool Platform_Key_WasReleased(KeyCode key) {
    if (key < 0 || key >= KEY_COUNT) return false;
    return !g_keys_current[key] && g_keys_previous[key];
}

bool Platform_Key_GetNext(KeyCode* key, bool* released) {
    if (g_key_queue.empty()) return false;

    KeyEvent event = g_key_queue.front();
    g_key_queue.pop();

    *key = event.key;
    *released = event.released;
    return true;
}

void Platform_Key_Clear(void) {
    while (!g_key_queue.empty()) {
        g_key_queue.pop();
    }
}

bool Platform_Key_ShiftDown(void) { return g_shift_down; }
bool Platform_Key_CtrlDown(void) { return g_ctrl_down; }
bool Platform_Key_AltDown(void) { return g_alt_down; }
bool Platform_Key_CapsLock(void) { return g_caps_lock; }
bool Platform_Key_NumLock(void) { return g_num_lock; }
```

### 4.5 Mouse Query Functions

```cpp
void Platform_Mouse_GetPosition(int* x, int* y) {
    if (x) *x = g_mouse_x;
    if (y) *y = g_mouse_y;
}

void Platform_Mouse_SetPosition(int x, int y) {
    // Get window from graphics system
    extern SDL_Window* g_window;  // From graphics_sdl.cpp
    SDL_WarpMouseInWindow(g_window, x, y);
    g_mouse_x = x;
    g_mouse_y = y;
}

bool Platform_Mouse_IsPressed(MouseButton button) {
    return g_mouse_buttons_current[button];
}

bool Platform_Mouse_WasClicked(MouseButton button) {
    return g_mouse_clicked[button];
}

bool Platform_Mouse_WasDoubleClicked(MouseButton button) {
    return g_mouse_double_clicked[button];
}

int Platform_Mouse_GetWheelDelta(void) {
    return g_mouse_wheel_delta;
}

void Platform_Mouse_Show(void) {
    SDL_ShowCursor(SDL_ENABLE);
}

void Platform_Mouse_Hide(void) {
    SDL_ShowCursor(SDL_DISABLE);
}

void Platform_Mouse_Confine(bool confine) {
    extern SDL_Window* g_window;
    SDL_SetWindowGrab(g_window, confine ? SDL_TRUE : SDL_FALSE);
}
```

### 4.6 WWKeyboardClass Compatibility Wrapper

File: `include/compat/keyboard_wrapper.h`
```cpp
#ifndef KEYBOARD_WRAPPER_H
#define KEYBOARD_WRAPPER_H

#include "platform/platform_input.h"

// Virtual key code mapping (Windows VK_* to our KeyCode)
#define VK_ESCAPE   KEY_ESCAPE
#define VK_RETURN   KEY_RETURN
#define VK_SPACE    KEY_SPACE
#define VK_LEFT     KEY_LEFT
#define VK_UP       KEY_UP
#define VK_RIGHT    KEY_RIGHT
#define VK_DOWN     KEY_DOWN
#define VK_SHIFT    KEY_SHIFT
#define VK_CONTROL  KEY_CONTROL
#define VK_MENU     KEY_ALT
// ... add more as needed

class WWKeyboardClass {
public:
    WWKeyboardClass() {}

    // Check if key event available
    int Check() {
        KeyCode key;
        bool released;
        // Peek without removing
        // (Would need to modify Platform API or use internal access)
        return !g_key_queue.empty();
    }

    // Get next key event
    int Get() {
        KeyCode key;
        bool released;
        if (Platform_Key_GetNext(&key, &released)) {
            // Pack into original format: key | (released << 8)
            return key | (released ? 0x100 : 0);
        }
        return 0;
    }

    bool Is_Key_Down(int vk_key) {
        return Platform_Key_IsPressed((KeyCode)vk_key);
    }

    void Clear() {
        Platform_Key_Clear();
    }

    // Modifier queries
    bool Is_Shift_Down() { return Platform_Key_ShiftDown(); }
    bool Is_Ctrl_Down() { return Platform_Key_CtrlDown(); }
    bool Is_Alt_Down() { return Platform_Key_AltDown(); }
};

// Global keyboard instance
extern WWKeyboardClass* Keyboard;

#endif // KEYBOARD_WRAPPER_H
```

### 4.7 Mouse Compatibility

File: `include/compat/mouse_wrapper.h`
```cpp
#ifndef MOUSE_WRAPPER_H
#define MOUSE_WRAPPER_H

#include "platform/platform_input.h"

// Mouse button constants matching original code
#define LEFT_BUTTON     MOUSE_LEFT
#define RIGHT_BUTTON    MOUSE_RIGHT

inline void Get_Mouse_XY(int* x, int* y) {
    Platform_Mouse_GetPosition(x, y);
}

inline void Set_Mouse_XY(int x, int y) {
    Platform_Mouse_SetPosition(x, y);
}

inline bool Mouse_Left_Pressed() {
    return Platform_Mouse_IsPressed(MOUSE_LEFT);
}

inline bool Mouse_Right_Pressed() {
    return Platform_Mouse_IsPressed(MOUSE_RIGHT);
}

inline void Hide_Mouse() {
    Platform_Mouse_Hide();
}

inline void Show_Mouse() {
    Platform_Mouse_Show();
}

#endif // MOUSE_WRAPPER_H
```

## Integration with Game Loop

The input system needs to be called from the main event loop:

```cpp
// In main.cpp or platform initialization
void Platform_PumpEvents(void) {
    Platform_Input_Update();  // Prepare for new frame

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                g_should_quit = true;
                break;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
            case SDL_MOUSEMOTION:
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEWHEEL:
                Platform_Input_HandleEvent(&event);
                break;
        }
    }
}
```

## Tasks Breakdown

### Phase 1: Basic Keyboard (2 days)
- [ ] Implement key code mapping table
- [ ] Implement key state tracking
- [ ] Implement key event queue
- [ ] Test with simple key display

### Phase 2: Mouse Input (1 day)
- [ ] Implement mouse position tracking
- [ ] Implement button state tracking
- [ ] Implement double-click detection
- [ ] Test with cursor display

### Phase 3: Compatibility Layer (1 day)
- [ ] Create `WWKeyboardClass` wrapper
- [ ] Create mouse function wrappers
- [ ] Map VK_* constants
- [ ] Test with existing game code

### Phase 4: Integration (1 day)
- [ ] Hook into main event loop
- [ ] Test modifier keys
- [ ] Test mouse confinement
- [ ] Verify game menu navigation works

## Testing Strategy

### Test 1: Key Echo
Print each key press/release to console.

### Test 2: Mouse Tracking
Display mouse coordinates and button state.

### Test 3: Menu Navigation
Navigate game menus using keyboard and mouse.

### Test 4: Game Control
Test unit selection and movement commands.

## Acceptance Criteria

- [ ] All keyboard keys mapped correctly
- [ ] Mouse position accurate to game coordinates
- [ ] Double-click detection works
- [ ] Modifier keys (Shift/Ctrl/Alt) work
- [ ] Game menus navigable with keyboard
- [ ] Unit selection works with mouse

## Estimated Duration
**4-5 days**

## Dependencies
- Plan 03 (Graphics) completed (for window handle)
- SDL2 event system working

## Next Plan
Once input is working, proceed to **Plan 05: Timing System**
