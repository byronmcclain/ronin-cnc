// src/game/input/input_state.cpp
// Input state management implementation
// Task 16a - Input Foundation

#include "game/input/input_state.h"
#include "game/viewport.h"
#include "platform.h"
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

    Platform_LogInfo("InputState: Initializing...");

    // Initialize platform input
    if (Platform_Input_Init() != 0) {
        Platform_LogError("InputState: Failed to initialize platform input");
        return false;
    }

    // Clear all state
    keyboard_.Clear();
    mouse_.Clear();

    initialized_ = true;
    Platform_LogInfo("InputState: Initialized successfully");
    return true;
}

void InputState::Shutdown() {
    if (!initialized_) {
        return;
    }

    Platform_LogInfo("InputState: Shutting down...");
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
    // Sample key states from platform
    // We check common key codes since platform uses specific KeyCode enum
    for (int k = 0; k < KEY_CODE_MAX; k++) {
        keyboard_.keys_down[k] = Platform_Key_IsPressed(static_cast<KeyCode>(k));
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
    mouse_.buttons_down[INPUT_MOUSE_LEFT] = Platform_Mouse_IsPressed(MOUSE_BUTTON_LEFT);
    mouse_.buttons_down[INPUT_MOUSE_RIGHT] = Platform_Mouse_IsPressed(MOUSE_BUTTON_RIGHT);
    mouse_.buttons_down[INPUT_MOUSE_MIDDLE] = Platform_Mouse_IsPressed(MOUSE_BUTTON_MIDDLE);

    // Get wheel delta
    mouse_.wheel_delta = Platform_Mouse_GetWheelDelta();

    // Update coordinate conversions
    UpdateMouseCoordinates();

    // Update drag state
    UpdateDragState();

    // Check for double clicks
    for (int b = 0; b < INPUT_MOUSE_MAX; b++) {
        if (WasMouseButtonPressed(b)) {
            CheckDoubleClick(b);
        }
    }
}

void InputState::UpdateMouseCoordinates() {
    // Convert screen coordinates to world coordinates using viewport
    GameViewport& vp = GameViewport::Instance();

    // World coordinates = screen + viewport offset
    // Account for the tactical area offset (tab bar at top)
    mouse_.world_x = mouse_.screen_x + vp.x;
    mouse_.world_y = mouse_.screen_y + vp.y - VP_TAB_HEIGHT;

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
    for (int b = 0; b < INPUT_MOUSE_MAX; b++) {
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
    if (mouse_.drag_button >= 0 && mouse_.drag_button < INPUT_MOUSE_MAX
        && mouse_.buttons_down[mouse_.drag_button]) {
        mouse_.drag_current_x = mouse_.screen_x;
        mouse_.drag_current_y = mouse_.screen_y;

        // Check if drag threshold exceeded
        if (!mouse_.is_dragging && mouse_.DragThresholdExceeded()) {
            mouse_.is_dragging = true;
        }
    }

    // End drag when button released
    if (mouse_.drag_button >= 0 && mouse_.drag_button < INPUT_MOUSE_MAX
        && !mouse_.buttons_down[mouse_.drag_button]) {
        // Keep is_dragging true for one frame so code can detect drag end
        // Next frame it will be reset when SavePreviousState is called
        if (!mouse_.buttons_down_prev[mouse_.drag_button]) {
            mouse_.is_dragging = false;
            mouse_.drag_button = -1;
        }
    }
}

void InputState::CheckDoubleClick(int button) {
    if (button < 0 || button >= INPUT_MOUSE_MAX) {
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
    if (button < 0 || button >= INPUT_MOUSE_MAX) {
        return false;
    }
    return mouse_.buttons_down[button];
}

bool InputState::WasMouseButtonPressed(int button) const {
    if (button < 0 || button >= INPUT_MOUSE_MAX) {
        return false;
    }
    return mouse_.buttons_down[button] && !mouse_.buttons_down_prev[button];
}

bool InputState::WasMouseButtonReleased(int button) const {
    if (button < 0 || button >= INPUT_MOUSE_MAX) {
        return false;
    }
    return !mouse_.buttons_down[button] && mouse_.buttons_down_prev[button];
}

bool InputState::WasMouseDoubleClicked(int button) const {
    if (button < 0 || button >= INPUT_MOUSE_MAX) {
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
