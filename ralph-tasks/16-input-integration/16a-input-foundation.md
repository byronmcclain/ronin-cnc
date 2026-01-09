# Task 16a: Input System Foundation

| Field | Value |
|-------|-------|
| **Task ID** | 16a |
| **Task Name** | Input System Foundation |
| **Phase** | 16 - Input Integration |
| **Depends On** | Phase 15 (Graphics Integration), Platform Input APIs |
| **Estimated Complexity** | Medium (~400 lines) |

---

## Dependencies

- Phase 15 (Graphics Integration) must be complete
- Platform input APIs must be functional:
  - `Platform_Input_Init()`, `Platform_Input_Shutdown()`, `Platform_Input_Update()`
  - `Platform_Key_IsPressed()`, `Platform_Key_JustPressed()`, `Platform_Key_JustReleased()`
  - `Platform_Key_ShiftDown()`, `Platform_Key_CtrlDown()`, `Platform_Key_AltDown()`
  - `Platform_Mouse_GetPosition()`, `Platform_Mouse_IsPressed()`, `Platform_Mouse_JustPressed()`
- Viewport system from Phase 15g must be accessible

---

## Context

The Input Integration phase connects the platform's raw input APIs to the game's control system. This foundation task establishes the core infrastructure that all subsequent input tasks build upon:

1. **State Management**: Track current and previous input states for edge detection
2. **Frame Synchronization**: Input must be sampled once per frame, consistently
3. **Platform Abstraction**: Provide clean interfaces that hide platform details
4. **Key Code Definitions**: Define the key codes used throughout the game

The original Red Alert used DOS keyboard interrupts and DirectInput. Our port uses SDL2 through the platform layer, but the game code should not know this.

---

## Objective

Create the input system foundation including:
1. Key code definitions that map to platform key codes
2. Input state tracking structures
3. Core input update loop that samples platform state
4. Helper functions for common input queries

---

## Technical Background

### Input State Management

The input system needs to track multiple states:

```
Frame N-1           Frame N             Frame N+1
+-------------+     +-------------+     +-------------+
| Key A: DOWN |     | Key A: DOWN |     | Key A: UP   |
| Key B: UP   |  -> | Key B: DOWN |  -> | Key B: DOWN |
+-------------+     +-------------+     +-------------+

Derived States at Frame N:
- Key A: IsPressed=YES, JustPressed=NO (was down last frame)
- Key B: IsPressed=YES, JustPressed=YES (was up last frame)
```

### Platform Key Codes

The platform layer defines key codes (e.g., `KEY_CODE_A`, `KEY_CODE_ESCAPE`). We need to:
1. Define our own key code constants for game use
2. Map them to platform key codes (currently 1:1, but abstraction allows remapping)

### Update Timing

```
Game Loop:
1. Platform_PumpEvents()      <- OS events processed
2. Platform_Input_Update()    <- Platform layer updates state
3. Input_Update()             <- Our layer snapshots state
4. Game logic uses input      <- Queries are consistent for entire frame
5. Render
6. Repeat
```

---

## Deliverables

- [ ] `src/game/input/input_defs.h` - Key codes and constants
- [ ] `src/game/input/input_state.h` - State structures and declarations
- [ ] `src/game/input/input_state.cpp` - State management implementation
- [ ] `tests/test_input_foundation.cpp` - Test program
- [ ] CMakeLists.txt updated
- [ ] All tests pass

---

## Files to Create

### src/game/input/input_defs.h

```cpp
// src/game/input/input_defs.h
// Key code definitions and input constants
// Task 16a - Input Foundation

#ifndef INPUT_DEFS_H
#define INPUT_DEFS_H

#include <cstdint>

//=============================================================================
// Key Code Definitions
//=============================================================================
// These map to platform key codes but provide game-specific abstraction.
// If key remapping is needed later, only this file changes.

// Special keys
constexpr int KEY_NONE          = 0;
constexpr int KEY_ESCAPE        = 27;
constexpr int KEY_RETURN        = 13;
constexpr int KEY_SPACE         = 32;
constexpr int KEY_BACKSPACE     = 8;
constexpr int KEY_TAB           = 9;

// Arrow keys (use SDL scan codes offset)
constexpr int KEY_UP            = 0x100 + 82;  // SDL_SCANCODE_UP
constexpr int KEY_DOWN          = 0x100 + 81;  // SDL_SCANCODE_DOWN
constexpr int KEY_LEFT          = 0x100 + 80;  // SDL_SCANCODE_LEFT
constexpr int KEY_RIGHT         = 0x100 + 79;  // SDL_SCANCODE_RIGHT

// Function keys
constexpr int KEY_F1            = 0x100 + 58;
constexpr int KEY_F2            = 0x100 + 59;
constexpr int KEY_F3            = 0x100 + 60;
constexpr int KEY_F4            = 0x100 + 61;
constexpr int KEY_F5            = 0x100 + 62;
constexpr int KEY_F6            = 0x100 + 63;
constexpr int KEY_F7            = 0x100 + 64;
constexpr int KEY_F8            = 0x100 + 65;
constexpr int KEY_F9            = 0x100 + 66;
constexpr int KEY_F10           = 0x100 + 67;
constexpr int KEY_F11           = 0x100 + 68;
constexpr int KEY_F12           = 0x100 + 69;

// Letter keys (ASCII)
constexpr int KEY_A             = 'A';
constexpr int KEY_B             = 'B';
constexpr int KEY_C             = 'C';
constexpr int KEY_D             = 'D';
constexpr int KEY_E             = 'E';
constexpr int KEY_F             = 'F';
constexpr int KEY_G             = 'G';
constexpr int KEY_H             = 'H';
constexpr int KEY_I             = 'I';
constexpr int KEY_J             = 'J';
constexpr int KEY_K             = 'K';
constexpr int KEY_L             = 'L';
constexpr int KEY_M             = 'M';
constexpr int KEY_N             = 'N';
constexpr int KEY_O             = 'O';
constexpr int KEY_P             = 'P';
constexpr int KEY_Q             = 'Q';
constexpr int KEY_R             = 'R';
constexpr int KEY_S             = 'S';
constexpr int KEY_T             = 'T';
constexpr int KEY_U             = 'U';
constexpr int KEY_V             = 'V';
constexpr int KEY_W             = 'W';
constexpr int KEY_X             = 'X';
constexpr int KEY_Y             = 'Y';
constexpr int KEY_Z             = 'Z';

// Number keys (top row)
constexpr int KEY_0             = '0';
constexpr int KEY_1             = '1';
constexpr int KEY_2             = '2';
constexpr int KEY_3             = '3';
constexpr int KEY_4             = '4';
constexpr int KEY_5             = '5';
constexpr int KEY_6             = '6';
constexpr int KEY_7             = '7';
constexpr int KEY_8             = '8';
constexpr int KEY_9             = '9';

// Modifier keys
constexpr int KEY_LSHIFT        = 0x100 + 225;
constexpr int KEY_RSHIFT        = 0x100 + 229;
constexpr int KEY_LCTRL         = 0x100 + 224;
constexpr int KEY_RCTRL         = 0x100 + 228;
constexpr int KEY_LALT          = 0x100 + 226;
constexpr int KEY_RALT          = 0x100 + 230;

// Other useful keys
constexpr int KEY_HOME          = 0x100 + 74;
constexpr int KEY_END           = 0x100 + 77;
constexpr int KEY_PAGEUP        = 0x100 + 75;
constexpr int KEY_PAGEDOWN      = 0x100 + 78;
constexpr int KEY_INSERT        = 0x100 + 73;
constexpr int KEY_DELETE        = 0x100 + 76;

// Maximum key code value for array sizing
constexpr int KEY_CODE_MAX      = 512;

//=============================================================================
// Mouse Button Definitions
//=============================================================================

constexpr int MOUSE_BUTTON_LEFT     = 0;
constexpr int MOUSE_BUTTON_RIGHT    = 1;
constexpr int MOUSE_BUTTON_MIDDLE   = 2;
constexpr int MOUSE_BUTTON_MAX      = 5;

//=============================================================================
// Input Timing Constants
//=============================================================================

// Double-click timing (in milliseconds)
constexpr int DOUBLE_CLICK_TIME_MS  = 400;

// Drag threshold (pixels moved before considered a drag)
constexpr int DRAG_THRESHOLD_PIXELS = 4;

// Key repeat delay and rate (for text input)
constexpr int KEY_REPEAT_DELAY_MS   = 500;
constexpr int KEY_REPEAT_RATE_MS    = 50;

//=============================================================================
// Modifier Key Flags
//=============================================================================

enum ModifierFlags : uint8_t {
    MOD_NONE    = 0x00,
    MOD_SHIFT   = 0x01,
    MOD_CTRL    = 0x02,
    MOD_ALT     = 0x04,
    MOD_META    = 0x08,  // Command key on Mac
};

#endif // INPUT_DEFS_H
```

### src/game/input/input_state.h

```cpp
// src/game/input/input_state.h
// Input state tracking structures and functions
// Task 16a - Input Foundation

#ifndef INPUT_STATE_H
#define INPUT_STATE_H

#include "input_defs.h"
#include <cstdint>

//=============================================================================
// Keyboard State
//=============================================================================

struct KeyboardState {
    // Current frame key states (256 keys + 256 extended)
    bool keys_down[KEY_CODE_MAX];

    // Previous frame key states (for edge detection)
    bool keys_down_prev[KEY_CODE_MAX];

    // Current modifier state
    uint8_t modifiers;

    // Key buffer for text input (circular buffer)
    static constexpr int KEY_BUFFER_SIZE = 32;
    int key_buffer[KEY_BUFFER_SIZE];
    int buffer_head;
    int buffer_tail;

    // Initialize all to false/zero
    void Clear();

    // Copy current state to previous (call at start of frame)
    void SavePreviousState();

    // Add key to buffer
    void BufferKey(int key_code);

    // Get buffered key (returns KEY_NONE if empty)
    int GetBufferedKey();

    // Check if buffer has keys
    bool HasBufferedKeys() const;

    // Clear the key buffer
    void ClearBuffer();
};

//=============================================================================
// Mouse State
//=============================================================================

struct MouseState {
    // Position in raw window coordinates (before scaling)
    int raw_x;
    int raw_y;

    // Position in screen coordinates (after 2x scale correction)
    int screen_x;
    int screen_y;

    // Position in world coordinates (screen + viewport offset)
    int world_x;
    int world_y;

    // Position in cell/tile coordinates
    int cell_x;
    int cell_y;

    // Button states
    bool buttons_down[MOUSE_BUTTON_MAX];
    bool buttons_down_prev[MOUSE_BUTTON_MAX];

    // Timing for double-click detection
    uint64_t last_click_time[MOUSE_BUTTON_MAX];
    int last_click_x[MOUSE_BUTTON_MAX];
    int last_click_y[MOUSE_BUTTON_MAX];
    bool double_clicked[MOUSE_BUTTON_MAX];

    // Drag state
    bool is_dragging;
    int drag_start_x;
    int drag_start_y;
    int drag_current_x;
    int drag_current_y;
    int drag_button;  // Which button started the drag

    // Wheel state
    int wheel_delta;  // Positive = up, negative = down

    // Initialize all to zero/false
    void Clear();

    // Copy current button state to previous
    void SavePreviousState();

    // Calculate drag distance
    int GetDragDistanceX() const;
    int GetDragDistanceY() const;
    int GetDragDistanceSquared() const;

    // Check if drag threshold exceeded
    bool DragThresholdExceeded() const;
};

//=============================================================================
// Combined Input State
//=============================================================================

class InputState {
public:
    // Singleton access
    static InputState& Instance();

    // Lifecycle
    bool Initialize();
    void Shutdown();
    bool IsInitialized() const { return initialized_; }

    // Per-frame update (call once at start of game loop)
    void Update();

    // Raw state access
    KeyboardState& Keyboard() { return keyboard_; }
    MouseState& Mouse() { return mouse_; }
    const KeyboardState& Keyboard() const { return keyboard_; }
    const MouseState& Mouse() const { return mouse_; }

    //=========================================================================
    // Keyboard Queries (convenience wrappers)
    //=========================================================================

    // Is key currently held down?
    bool IsKeyDown(int key_code) const;

    // Was key just pressed this frame? (edge detection)
    bool WasKeyPressed(int key_code) const;

    // Was key just released this frame? (edge detection)
    bool WasKeyReleased(int key_code) const;

    // Modifier state
    bool IsShiftDown() const;
    bool IsCtrlDown() const;
    bool IsAltDown() const;
    uint8_t GetModifiers() const;

    // Check key with specific modifiers (e.g., Ctrl+A)
    bool WasKeyPressedWithMods(int key_code, uint8_t required_mods) const;

    //=========================================================================
    // Mouse Queries (convenience wrappers)
    //=========================================================================

    // Button state
    bool IsMouseButtonDown(int button) const;
    bool WasMouseButtonPressed(int button) const;
    bool WasMouseButtonReleased(int button) const;
    bool WasMouseDoubleClicked(int button) const;

    // Position (screen coordinates, after scale correction)
    int GetMouseX() const { return mouse_.screen_x; }
    int GetMouseY() const { return mouse_.screen_y; }

    // Position in world coordinates
    int GetMouseWorldX() const { return mouse_.world_x; }
    int GetMouseWorldY() const { return mouse_.world_y; }

    // Position in cell coordinates
    int GetMouseCellX() const { return mouse_.cell_x; }
    int GetMouseCellY() const { return mouse_.cell_y; }

    // Drag state
    bool IsDragging() const { return mouse_.is_dragging; }
    void GetDragRect(int& x1, int& y1, int& x2, int& y2) const;

    // Wheel
    int GetWheelDelta() const { return mouse_.wheel_delta; }

    //=========================================================================
    // Configuration
    //=========================================================================

    void SetWindowScale(int scale) { window_scale_ = scale; }
    int GetWindowScale() const { return window_scale_; }

private:
    InputState();
    ~InputState();
    InputState(const InputState&) = delete;
    InputState& operator=(const InputState&) = delete;

    void UpdateKeyboardState();
    void UpdateMouseState();
    void UpdateMouseCoordinates();
    void UpdateDragState();
    void CheckDoubleClick(int button);

    bool initialized_;
    int window_scale_;

    KeyboardState keyboard_;
    MouseState mouse_;

    // Frame timing for double-click detection
    uint64_t current_time_ms_;
};

//=============================================================================
// Global Access Functions (for legacy code compatibility)
//=============================================================================

// Initialize input system
bool Input_Init();

// Shutdown input system
void Input_Shutdown();

// Update input state (call once per frame)
void Input_Update();

// Keyboard queries
bool Input_KeyDown(int key_code);
bool Input_KeyPressed(int key_code);
bool Input_KeyReleased(int key_code);
bool Input_ShiftDown();
bool Input_CtrlDown();
bool Input_AltDown();

// Mouse queries
bool Input_MouseButtonDown(int button);
bool Input_MouseButtonPressed(int button);
bool Input_MouseButtonReleased(int button);
bool Input_MouseDoubleClicked(int button);
void Input_GetMousePosition(int* x, int* y);
void Input_GetMouseWorldPosition(int* x, int* y);
void Input_GetMouseCellPosition(int* x, int* y);
bool Input_IsDragging();
void Input_GetDragRect(int* x1, int* y1, int* x2, int* y2);

#endif // INPUT_STATE_H
```

### src/game/input/input_state.cpp

```cpp
// src/game/input/input_state.cpp
// Input state management implementation
// Task 16a - Input Foundation

#include "input_state.h"
#include "platform.h"
#include "viewport.h"
#include <cstring>
#include <algorithm>

//=============================================================================
// KeyboardState Implementation
//=============================================================================

void KeyboardState::Clear() {
    memset(keys_down, 0, sizeof(keys_down));
    memset(keys_down_prev, 0, sizeof(keys_down_prev));
    modifiers = MOD_NONE;
    memset(key_buffer, 0, sizeof(key_buffer));
    buffer_head = 0;
    buffer_tail = 0;
}

void KeyboardState::SavePreviousState() {
    memcpy(keys_down_prev, keys_down, sizeof(keys_down));
}

void KeyboardState::BufferKey(int key_code) {
    int next_head = (buffer_head + 1) % KEY_BUFFER_SIZE;
    if (next_head != buffer_tail) {  // Buffer not full
        key_buffer[buffer_head] = key_code;
        buffer_head = next_head;
    }
}

int KeyboardState::GetBufferedKey() {
    if (buffer_tail == buffer_head) {
        return KEY_NONE;  // Buffer empty
    }
    int key = key_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % KEY_BUFFER_SIZE;
    return key;
}

bool KeyboardState::HasBufferedKeys() const {
    return buffer_tail != buffer_head;
}

void KeyboardState::ClearBuffer() {
    buffer_head = 0;
    buffer_tail = 0;
}

//=============================================================================
// MouseState Implementation
//=============================================================================

void MouseState::Clear() {
    raw_x = raw_y = 0;
    screen_x = screen_y = 0;
    world_x = world_y = 0;
    cell_x = cell_y = 0;

    memset(buttons_down, 0, sizeof(buttons_down));
    memset(buttons_down_prev, 0, sizeof(buttons_down_prev));
    memset(last_click_time, 0, sizeof(last_click_time));
    memset(last_click_x, 0, sizeof(last_click_x));
    memset(last_click_y, 0, sizeof(last_click_y));
    memset(double_clicked, 0, sizeof(double_clicked));

    is_dragging = false;
    drag_start_x = drag_start_y = 0;
    drag_current_x = drag_current_y = 0;
    drag_button = -1;

    wheel_delta = 0;
}

void MouseState::SavePreviousState() {
    memcpy(buttons_down_prev, buttons_down, sizeof(buttons_down));

    // Clear per-frame states
    memset(double_clicked, 0, sizeof(double_clicked));
    wheel_delta = 0;
}

int MouseState::GetDragDistanceX() const {
    return drag_current_x - drag_start_x;
}

int MouseState::GetDragDistanceY() const {
    return drag_current_y - drag_start_y;
}

int MouseState::GetDragDistanceSquared() const {
    int dx = GetDragDistanceX();
    int dy = GetDragDistanceY();
    return dx * dx + dy * dy;
}

bool MouseState::DragThresholdExceeded() const {
    int dist_sq = GetDragDistanceSquared();
    return dist_sq >= (DRAG_THRESHOLD_PIXELS * DRAG_THRESHOLD_PIXELS);
}

//=============================================================================
// InputState Singleton
//=============================================================================

InputState& InputState::Instance() {
    static InputState instance;
    return instance;
}

InputState::InputState()
    : initialized_(false)
    , window_scale_(2)
    , current_time_ms_(0) {
    keyboard_.Clear();
    mouse_.Clear();
}

InputState::~InputState() {
    Shutdown();
}

bool InputState::Initialize() {
    if (initialized_) {
        return true;
    }

    Platform_Log("InputState: Initializing...");

    // Initialize platform input
    if (Platform_Input_Init() != 0) {
        Platform_Log("InputState: Failed to initialize platform input");
        return false;
    }

    // Clear all state
    keyboard_.Clear();
    mouse_.Clear();

    initialized_ = true;
    Platform_Log("InputState: Initialized successfully");
    return true;
}

void InputState::Shutdown() {
    if (!initialized_) {
        return;
    }

    Platform_Log("InputState: Shutting down...");
    Platform_Input_Shutdown();

    keyboard_.Clear();
    mouse_.Clear();

    initialized_ = false;
}

void InputState::Update() {
    if (!initialized_) {
        return;
    }

    // Get current time for double-click detection
    current_time_ms_ = Platform_GetTicks();

    // Save previous states for edge detection
    keyboard_.SavePreviousState();
    mouse_.SavePreviousState();

    // Update platform input state
    Platform_Input_Update();

    // Sample new states
    UpdateKeyboardState();
    UpdateMouseState();
}

void InputState::UpdateKeyboardState() {
    // Sample all key states from platform
    for (int k = 0; k < KEY_CODE_MAX; k++) {
        keyboard_.keys_down[k] = Platform_Key_IsPressed(k);
    }

    // Update modifier flags
    keyboard_.modifiers = MOD_NONE;
    if (Platform_Key_ShiftDown()) {
        keyboard_.modifiers |= MOD_SHIFT;
    }
    if (Platform_Key_CtrlDown()) {
        keyboard_.modifiers |= MOD_CTRL;
    }
    if (Platform_Key_AltDown()) {
        keyboard_.modifiers |= MOD_ALT;
    }

    // Buffer any newly pressed keys (for text input)
    for (int k = 0; k < KEY_CODE_MAX; k++) {
        if (keyboard_.keys_down[k] && !keyboard_.keys_down_prev[k]) {
            keyboard_.BufferKey(k);
        }
    }
}

void InputState::UpdateMouseState() {
    // Get raw position from platform
    int32_t raw_x, raw_y;
    Platform_Mouse_GetPosition(&raw_x, &raw_y);
    mouse_.raw_x = raw_x;
    mouse_.raw_y = raw_y;

    // Apply window scale to get screen coordinates
    mouse_.screen_x = raw_x / window_scale_;
    mouse_.screen_y = raw_y / window_scale_;

    // Update button states
    for (int b = 0; b < MOUSE_BUTTON_MAX; b++) {
        mouse_.buttons_down[b] = Platform_Mouse_IsPressed(b);
    }

    // Get wheel delta
    mouse_.wheel_delta = Platform_Mouse_GetWheelDelta();

    // Update coordinate conversions
    UpdateMouseCoordinates();

    // Update drag state
    UpdateDragState();

    // Check for double clicks
    for (int b = 0; b < MOUSE_BUTTON_MAX; b++) {
        if (WasMouseButtonPressed(b)) {
            CheckDoubleClick(b);
        }
    }
}

void InputState::UpdateMouseCoordinates() {
    // Convert screen coordinates to world coordinates using viewport
    // Note: This requires the viewport to be available
    Viewport& vp = Viewport::Instance();

    // World coordinates = screen + viewport offset
    // But we need to account for the tactical area offset (sidebar, etc.)
    mouse_.world_x = mouse_.screen_x + vp.x;
    mouse_.world_y = mouse_.screen_y + vp.y - TAB_HEIGHT;

    // Cell coordinates
    mouse_.cell_x = mouse_.world_x / TILE_PIXEL_WIDTH;
    mouse_.cell_y = mouse_.world_y / TILE_PIXEL_HEIGHT;

    // Clamp cell coordinates to valid range
    if (mouse_.cell_x < 0) mouse_.cell_x = 0;
    if (mouse_.cell_y < 0) mouse_.cell_y = 0;
    if (mouse_.cell_x >= vp.GetMapCellWidth()) {
        mouse_.cell_x = vp.GetMapCellWidth() - 1;
    }
    if (mouse_.cell_y >= vp.GetMapCellHeight()) {
        mouse_.cell_y = vp.GetMapCellHeight() - 1;
    }
}

void InputState::UpdateDragState() {
    // Check for drag start (any button just pressed)
    for (int b = 0; b < MOUSE_BUTTON_MAX; b++) {
        if (WasMouseButtonPressed(b)) {
            // Start potential drag
            mouse_.drag_start_x = mouse_.screen_x;
            mouse_.drag_start_y = mouse_.screen_y;
            mouse_.drag_current_x = mouse_.screen_x;
            mouse_.drag_current_y = mouse_.screen_y;
            mouse_.drag_button = b;
            mouse_.is_dragging = false;  // Not dragging yet, just tracking
            break;
        }
    }

    // Update drag position if button is held
    if (mouse_.drag_button >= 0 && mouse_.buttons_down[mouse_.drag_button]) {
        mouse_.drag_current_x = mouse_.screen_x;
        mouse_.drag_current_y = mouse_.screen_y;

        // Check if drag threshold exceeded
        if (!mouse_.is_dragging && mouse_.DragThresholdExceeded()) {
            mouse_.is_dragging = true;
        }
    }

    // End drag when button released
    if (mouse_.drag_button >= 0 && !mouse_.buttons_down[mouse_.drag_button]) {
        // Keep is_dragging true for one frame so code can detect drag end
        // Next frame it will be reset when SavePreviousState is called
        if (!mouse_.buttons_down_prev[mouse_.drag_button]) {
            mouse_.is_dragging = false;
            mouse_.drag_button = -1;
        }
    }
}

void InputState::CheckDoubleClick(int button) {
    if (button < 0 || button >= MOUSE_BUTTON_MAX) {
        return;
    }

    uint64_t now = current_time_ms_;
    uint64_t last = mouse_.last_click_time[button];

    // Check time window
    if (now - last <= DOUBLE_CLICK_TIME_MS) {
        // Check position proximity (allow some movement)
        int dx = mouse_.screen_x - mouse_.last_click_x[button];
        int dy = mouse_.screen_y - mouse_.last_click_y[button];
        int dist_sq = dx * dx + dy * dy;

        if (dist_sq <= DRAG_THRESHOLD_PIXELS * DRAG_THRESHOLD_PIXELS) {
            mouse_.double_clicked[button] = true;
        }
    }

    // Update last click info
    mouse_.last_click_time[button] = now;
    mouse_.last_click_x[button] = mouse_.screen_x;
    mouse_.last_click_y[button] = mouse_.screen_y;
}

//=============================================================================
// Keyboard Query Methods
//=============================================================================

bool InputState::IsKeyDown(int key_code) const {
    if (key_code < 0 || key_code >= KEY_CODE_MAX) {
        return false;
    }
    return keyboard_.keys_down[key_code];
}

bool InputState::WasKeyPressed(int key_code) const {
    if (key_code < 0 || key_code >= KEY_CODE_MAX) {
        return false;
    }
    return keyboard_.keys_down[key_code] && !keyboard_.keys_down_prev[key_code];
}

bool InputState::WasKeyReleased(int key_code) const {
    if (key_code < 0 || key_code >= KEY_CODE_MAX) {
        return false;
    }
    return !keyboard_.keys_down[key_code] && keyboard_.keys_down_prev[key_code];
}

bool InputState::IsShiftDown() const {
    return (keyboard_.modifiers & MOD_SHIFT) != 0;
}

bool InputState::IsCtrlDown() const {
    return (keyboard_.modifiers & MOD_CTRL) != 0;
}

bool InputState::IsAltDown() const {
    return (keyboard_.modifiers & MOD_ALT) != 0;
}

uint8_t InputState::GetModifiers() const {
    return keyboard_.modifiers;
}

bool InputState::WasKeyPressedWithMods(int key_code, uint8_t required_mods) const {
    if (!WasKeyPressed(key_code)) {
        return false;
    }
    return (keyboard_.modifiers & required_mods) == required_mods;
}

//=============================================================================
// Mouse Query Methods
//=============================================================================

bool InputState::IsMouseButtonDown(int button) const {
    if (button < 0 || button >= MOUSE_BUTTON_MAX) {
        return false;
    }
    return mouse_.buttons_down[button];
}

bool InputState::WasMouseButtonPressed(int button) const {
    if (button < 0 || button >= MOUSE_BUTTON_MAX) {
        return false;
    }
    return mouse_.buttons_down[button] && !mouse_.buttons_down_prev[button];
}

bool InputState::WasMouseButtonReleased(int button) const {
    if (button < 0 || button >= MOUSE_BUTTON_MAX) {
        return false;
    }
    return !mouse_.buttons_down[button] && mouse_.buttons_down_prev[button];
}

bool InputState::WasMouseDoubleClicked(int button) const {
    if (button < 0 || button >= MOUSE_BUTTON_MAX) {
        return false;
    }
    return mouse_.double_clicked[button];
}

void InputState::GetDragRect(int& x1, int& y1, int& x2, int& y2) const {
    x1 = std::min(mouse_.drag_start_x, mouse_.drag_current_x);
    y1 = std::min(mouse_.drag_start_y, mouse_.drag_current_y);
    x2 = std::max(mouse_.drag_start_x, mouse_.drag_current_x);
    y2 = std::max(mouse_.drag_start_y, mouse_.drag_current_y);
}

//=============================================================================
// Global Access Functions (C-style API)
//=============================================================================

bool Input_Init() {
    return InputState::Instance().Initialize();
}

void Input_Shutdown() {
    InputState::Instance().Shutdown();
}

void Input_Update() {
    InputState::Instance().Update();
}

bool Input_KeyDown(int key_code) {
    return InputState::Instance().IsKeyDown(key_code);
}

bool Input_KeyPressed(int key_code) {
    return InputState::Instance().WasKeyPressed(key_code);
}

bool Input_KeyReleased(int key_code) {
    return InputState::Instance().WasKeyReleased(key_code);
}

bool Input_ShiftDown() {
    return InputState::Instance().IsShiftDown();
}

bool Input_CtrlDown() {
    return InputState::Instance().IsCtrlDown();
}

bool Input_AltDown() {
    return InputState::Instance().IsAltDown();
}

bool Input_MouseButtonDown(int button) {
    return InputState::Instance().IsMouseButtonDown(button);
}

bool Input_MouseButtonPressed(int button) {
    return InputState::Instance().WasMouseButtonPressed(button);
}

bool Input_MouseButtonReleased(int button) {
    return InputState::Instance().WasMouseButtonReleased(button);
}

bool Input_MouseDoubleClicked(int button) {
    return InputState::Instance().WasMouseDoubleClicked(button);
}

void Input_GetMousePosition(int* x, int* y) {
    if (x) *x = InputState::Instance().GetMouseX();
    if (y) *y = InputState::Instance().GetMouseY();
}

void Input_GetMouseWorldPosition(int* x, int* y) {
    if (x) *x = InputState::Instance().GetMouseWorldX();
    if (y) *y = InputState::Instance().GetMouseWorldY();
}

void Input_GetMouseCellPosition(int* x, int* y) {
    if (x) *x = InputState::Instance().GetMouseCellX();
    if (y) *y = InputState::Instance().GetMouseCellY();
}

bool Input_IsDragging() {
    return InputState::Instance().IsDragging();
}

void Input_GetDragRect(int* x1, int* y1, int* x2, int* y2) {
    int a, b, c, d;
    InputState::Instance().GetDragRect(a, b, c, d);
    if (x1) *x1 = a;
    if (y1) *y1 = b;
    if (x2) *x2 = c;
    if (y2) *y2 = d;
}
```

### tests/test_input_foundation.cpp

```cpp
// tests/test_input_foundation.cpp
// Test program for input foundation
// Task 16a - Input Foundation

#include "input_state.h"
#include "platform.h"
#include "viewport.h"
#include <cstdio>
#include <cstring>

//=============================================================================
// Unit Tests
//=============================================================================

bool Test_KeyboardStateInit() {
    printf("Test: KeyboardState Initialization... ");

    KeyboardState ks;
    ks.Clear();

    // All keys should be up
    for (int i = 0; i < KEY_CODE_MAX; i++) {
        if (ks.keys_down[i] || ks.keys_down_prev[i]) {
            printf("FAILED - Key %d not cleared\n", i);
            return false;
        }
    }

    if (ks.modifiers != MOD_NONE) {
        printf("FAILED - Modifiers not cleared\n");
        return false;
    }

    if (ks.HasBufferedKeys()) {
        printf("FAILED - Key buffer not empty\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_KeyBuffer() {
    printf("Test: Key Buffer... ");

    KeyboardState ks;
    ks.Clear();

    // Buffer some keys
    ks.BufferKey(KEY_A);
    ks.BufferKey(KEY_B);
    ks.BufferKey(KEY_C);

    if (!ks.HasBufferedKeys()) {
        printf("FAILED - Buffer should have keys\n");
        return false;
    }

    // Get them back in order
    if (ks.GetBufferedKey() != KEY_A) {
        printf("FAILED - Expected KEY_A\n");
        return false;
    }
    if (ks.GetBufferedKey() != KEY_B) {
        printf("FAILED - Expected KEY_B\n");
        return false;
    }
    if (ks.GetBufferedKey() != KEY_C) {
        printf("FAILED - Expected KEY_C\n");
        return false;
    }

    // Buffer should be empty now
    if (ks.HasBufferedKeys()) {
        printf("FAILED - Buffer should be empty\n");
        return false;
    }

    if (ks.GetBufferedKey() != KEY_NONE) {
        printf("FAILED - Empty buffer should return KEY_NONE\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_MouseStateInit() {
    printf("Test: MouseState Initialization... ");

    MouseState ms;
    ms.Clear();

    if (ms.screen_x != 0 || ms.screen_y != 0) {
        printf("FAILED - Position not cleared\n");
        return false;
    }

    for (int i = 0; i < MOUSE_BUTTON_MAX; i++) {
        if (ms.buttons_down[i]) {
            printf("FAILED - Button %d not cleared\n", i);
            return false;
        }
    }

    if (ms.is_dragging) {
        printf("FAILED - Drag state not cleared\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_DragDistance() {
    printf("Test: Drag Distance Calculation... ");

    MouseState ms;
    ms.Clear();

    ms.drag_start_x = 100;
    ms.drag_start_y = 100;
    ms.drag_current_x = 103;
    ms.drag_current_y = 104;

    if (ms.GetDragDistanceX() != 3) {
        printf("FAILED - Expected dx=3, got %d\n", ms.GetDragDistanceX());
        return false;
    }

    if (ms.GetDragDistanceY() != 4) {
        printf("FAILED - Expected dy=4, got %d\n", ms.GetDragDistanceY());
        return false;
    }

    if (ms.GetDragDistanceSquared() != 25) {
        printf("FAILED - Expected 25, got %d\n", ms.GetDragDistanceSquared());
        return false;
    }

    // With threshold of 4, distance of 5 should exceed
    if (!ms.DragThresholdExceeded()) {
        printf("FAILED - Drag threshold should be exceeded\n");
        return false;
    }

    // Smaller movement should not exceed
    ms.drag_current_x = 102;
    ms.drag_current_y = 102;
    if (ms.DragThresholdExceeded()) {
        printf("FAILED - Small movement should not exceed threshold\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_EdgeDetection() {
    printf("Test: Key Edge Detection... ");

    KeyboardState ks;
    ks.Clear();

    // Frame 1: Key A pressed
    ks.SavePreviousState();
    ks.keys_down[KEY_A] = true;

    // Just pressed check
    bool just_pressed = ks.keys_down[KEY_A] && !ks.keys_down_prev[KEY_A];
    if (!just_pressed) {
        printf("FAILED - Key A should be 'just pressed'\n");
        return false;
    }

    // Frame 2: Key A still held
    ks.SavePreviousState();
    ks.keys_down[KEY_A] = true;

    just_pressed = ks.keys_down[KEY_A] && !ks.keys_down_prev[KEY_A];
    if (just_pressed) {
        printf("FAILED - Key A should NOT be 'just pressed' (held)\n");
        return false;
    }

    // Frame 3: Key A released
    ks.SavePreviousState();
    ks.keys_down[KEY_A] = false;

    bool just_released = !ks.keys_down[KEY_A] && ks.keys_down_prev[KEY_A];
    if (!just_released) {
        printf("FAILED - Key A should be 'just released'\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_InputStateInit() {
    printf("Test: InputState Initialization... ");

    // Note: This test requires platform to be initialized
    Platform_Init();

    InputState& input = InputState::Instance();

    if (!input.Initialize()) {
        printf("FAILED - Could not initialize InputState\n");
        Platform_Shutdown();
        return false;
    }

    if (!input.IsInitialized()) {
        printf("FAILED - Not marked as initialized\n");
        input.Shutdown();
        Platform_Shutdown();
        return false;
    }

    input.Shutdown();

    if (input.IsInitialized()) {
        printf("FAILED - Still marked as initialized after shutdown\n");
        Platform_Shutdown();
        return false;
    }

    Platform_Shutdown();
    printf("PASSED\n");
    return true;
}

bool Test_GlobalFunctions() {
    printf("Test: Global Input Functions... ");

    Platform_Init();

    if (!Input_Init()) {
        printf("FAILED - Input_Init failed\n");
        Platform_Shutdown();
        return false;
    }

    // Call update
    Input_Update();

    // Test position retrieval
    int mx, my;
    Input_GetMousePosition(&mx, &my);
    // Just verify it doesn't crash

    // Test drag rect
    int x1, y1, x2, y2;
    Input_GetDragRect(&x1, &y1, &x2, &y2);
    // Verify it doesn't crash

    Input_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

//=============================================================================
// Interactive Test
//=============================================================================

void Interactive_Test() {
    printf("\n=== Interactive Input Test ===\n");
    printf("Press keys and move mouse to test\n");
    printf("Press ESC to exit\n\n");

    Platform_Init();
    Platform_Graphics_Init();

    // Initialize viewport for coordinate conversion
    Viewport::Instance().Initialize();
    Viewport::Instance().SetMapSize(64, 64);

    Input_Init();

    bool running = true;
    while (running) {
        Platform_PumpEvents();
        Input_Update();

        // Check for exit
        if (Input_KeyPressed(KEY_ESCAPE)) {
            running = false;
            continue;
        }

        // Report any key presses
        for (int k = 0; k < KEY_CODE_MAX; k++) {
            if (Input_KeyPressed(k)) {
                printf("Key pressed: %d", k);
                if (k >= 'A' && k <= 'Z') {
                    printf(" ('%c')", k);
                }
                if (Input_ShiftDown()) printf(" +SHIFT");
                if (Input_CtrlDown()) printf(" +CTRL");
                if (Input_AltDown()) printf(" +ALT");
                printf("\n");
            }
        }

        // Report mouse clicks
        if (Input_MouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            int mx, my;
            Input_GetMousePosition(&mx, &my);
            printf("Left click at screen (%d, %d)", mx, my);

            int wx, wy;
            Input_GetMouseWorldPosition(&wx, &wy);
            printf(" world (%d, %d)", wx, wy);

            int cx, cy;
            Input_GetMouseCellPosition(&cx, &cy);
            printf(" cell (%d, %d)\n", cx, cy);
        }

        if (Input_MouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            printf("Right click\n");
        }

        if (Input_MouseDoubleClicked(MOUSE_BUTTON_LEFT)) {
            printf("Double click!\n");
        }

        // Report drag
        static bool was_dragging = false;
        if (Input_IsDragging() && !was_dragging) {
            printf("Drag started\n");
        }
        if (!Input_IsDragging() && was_dragging) {
            int x1, y1, x2, y2;
            Input_GetDragRect(&x1, &y1, &x2, &y2);
            printf("Drag ended: (%d,%d) to (%d,%d)\n", x1, y1, x2, y2);
        }
        was_dragging = Input_IsDragging();

        Platform_Timer_Delay(16);
    }

    Input_Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Main
//=============================================================================

int main(int argc, char* argv[]) {
    printf("=== Input Foundation Tests (Task 16a) ===\n\n");

    int passed = 0;
    int failed = 0;

    // Run unit tests
    if (Test_KeyboardStateInit()) passed++; else failed++;
    if (Test_KeyBuffer()) passed++; else failed++;
    if (Test_MouseStateInit()) passed++; else failed++;
    if (Test_DragDistance()) passed++; else failed++;
    if (Test_EdgeDetection()) passed++; else failed++;
    if (Test_InputStateInit()) passed++; else failed++;
    if (Test_GlobalFunctions()) passed++; else failed++;

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
# Input System Foundation (Task 16a)
target_sources(game PRIVATE
    input/input_defs.h
    input/input_state.h
    input/input_state.cpp
)
```

Add to `tests/CMakeLists.txt`:

```cmake
# Input Foundation test (Task 16a)
add_executable(test_input_foundation test_input_foundation.cpp)
target_link_libraries(test_input_foundation PRIVATE game platform)
add_test(NAME InputFoundationTest COMMAND test_input_foundation)
```

---

## Verification Script

```bash
#!/bin/bash
set -e

echo "=== Task 16a Verification ==="

# Build
echo "Building..."
cd build
cmake --build . --target test_input_foundation

# Run tests
echo "Running unit tests..."
./tests/test_input_foundation

# Check files exist
echo "Checking source files..."
for file in \
    "../src/game/input/input_defs.h" \
    "../src/game/input/input_state.h" \
    "../src/game/input/input_state.cpp"
do
    if [ ! -f "$file" ]; then
        echo "ERROR: Missing $file"
        exit 1
    fi
done

echo ""
echo "=== Task 16a Verification PASSED ==="
echo "Run './tests/test_input_foundation -i' for interactive test"
```

---

## Implementation Steps

1. **Create directory structure** - `src/game/input/`
2. **Create input_defs.h** - Key codes and constants
3. **Create input_state.h** - State structures and declarations
4. **Create input_state.cpp** - Full implementation
5. **Create test_input_foundation.cpp** - Unit tests and interactive test
6. **Update CMakeLists.txt** - Add new files
7. **Build and run tests** - Verify all pass
8. **Run interactive test** - Verify input is responsive

---

## Success Criteria

- [ ] KeyboardState correctly tracks key down/up states
- [ ] Key buffer works as circular queue
- [ ] MouseState tracks position and buttons
- [ ] Drag distance calculation correct
- [ ] Edge detection (just pressed/released) works
- [ ] Double-click detection works with timing
- [ ] InputState singleton initializes/shuts down cleanly
- [ ] Global C-style functions work correctly
- [ ] All unit tests pass
- [ ] Interactive test shows responsive input

---

## Common Issues

1. **Key codes don't match platform**
   - Verify KEY_* constants match platform layer definitions
   - Check SDL scan code offsets are correct

2. **Edge detection misses events**
   - Ensure SavePreviousState() called at frame start
   - Check Platform_Input_Update() is called

3. **Mouse coordinates wrong**
   - Verify window_scale_ matches actual window scale
   - Check viewport offset calculations

4. **Double-click not detected**
   - Check DOUBLE_CLICK_TIME_MS value
   - Verify Platform_GetTicks() returns milliseconds

5. **Drag not starting**
   - Check DRAG_THRESHOLD_PIXELS value
   - Verify drag_button tracking

---

## Completion Promise

When verification passes, output:
<promise>TASK_16A_COMPLETE</promise>

## Escape Hatch

If stuck after 15 iterations:
- Document what's blocking in `ralph-tasks/blocked/16a.md`
- List attempted approaches
- Output:
<promise>TASK_16A_BLOCKED</promise>

## Max Iterations
15
