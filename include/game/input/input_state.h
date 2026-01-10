// include/game/input/input_state.h
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
    // Current frame key states
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
    bool buttons_down[INPUT_MOUSE_MAX];
    bool buttons_down_prev[INPUT_MOUSE_MAX];

    // Timing for double-click detection
    uint64_t last_click_time[INPUT_MOUSE_MAX];
    int last_click_x[INPUT_MOUSE_MAX];
    int last_click_y[INPUT_MOUSE_MAX];
    bool double_clicked[INPUT_MOUSE_MAX];

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
