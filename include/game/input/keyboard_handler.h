// include/game/input/keyboard_handler.h
// Keyboard input handler with context awareness
// Task 16c - Keyboard Input Handler

#ifndef KEYBOARD_HANDLER_H
#define KEYBOARD_HANDLER_H

#include "game_action.h"
#include "input_defs.h"
#include <cstdint>
#include <functional>

//=============================================================================
// Input Focus Mode
//=============================================================================

enum class InputFocus {
    GAME,       // Normal gameplay - hotkeys active
    MENU,       // In menu - only navigation keys
    TEXT,       // Text input - keys go to buffer
    DISABLED    // Input disabled (cutscene, loading)
};

//=============================================================================
// Text Input State
//=============================================================================

struct TextInputState {
    static constexpr int MAX_TEXT_LENGTH = 256;

    char text[MAX_TEXT_LENGTH];
    int cursor_pos;
    int text_length;
    int max_length;  // Configurable max for this input
    bool active;
    bool confirmed;  // Enter was pressed
    bool cancelled;  // Escape was pressed

    void Clear();
    void Reset(int max_len = MAX_TEXT_LENGTH);
    void InsertChar(char c);
    void DeleteChar();
    void Backspace();
    void MoveCursor(int delta);
    void Confirm();
    void Cancel();

    const char* GetText() const { return text; }
};

//=============================================================================
// Keyboard Handler
//=============================================================================

class KeyboardHandler {
public:
    // Singleton access
    static KeyboardHandler& Instance();

    // Lifecycle
    bool Initialize();
    void Shutdown();

    // Per-frame processing (call after InputMapper_ProcessFrame)
    void ProcessFrame();

    //=========================================================================
    // Focus Management
    //=========================================================================

    void SetFocus(InputFocus focus);
    InputFocus GetFocus() const { return focus_; }

    // Begin text input mode
    void BeginTextInput(int max_length = TextInputState::MAX_TEXT_LENGTH);
    void EndTextInput();
    bool IsTextInputActive() const { return text_input_.active; }
    TextInputState& GetTextInput() { return text_input_; }

    //=========================================================================
    // Context-Sensitive Action Queries
    //=========================================================================

    // These check game context before reporting action
    // (e.g., ESCAPE does different things based on state)

    bool ShouldOpenOptionsMenu() const;
    bool ShouldDeselectAll() const;
    bool ShouldCancelPlacement() const;

    //=========================================================================
    // Binding Info (for UI display)
    //=========================================================================

    // Get key name for an action (e.g., "Ctrl+A" for SELECT_ALL)
    const char* GetBindingString(GameAction action) const;

    // Get key name for a raw key code
    static const char* GetKeyName(int key_code);

    //=========================================================================
    // Game State Hooks
    //=========================================================================

    // Set callbacks for game state queries (needed for context sensitivity)
    using StateQueryFunc = std::function<bool()>;

    void SetHasSelectionQuery(StateQueryFunc func) { has_selection_ = func; }
    void SetInPlacementModeQuery(StateQueryFunc func) { in_placement_mode_ = func; }
    void SetInMenuQuery(StateQueryFunc func) { in_menu_ = func; }

private:
    KeyboardHandler();
    ~KeyboardHandler();
    KeyboardHandler(const KeyboardHandler&) = delete;
    KeyboardHandler& operator=(const KeyboardHandler&) = delete;

    void ProcessGameFocus();
    void ProcessMenuFocus();
    void ProcessTextFocus();

    void ProcessKeyForText(int key_code);
    char KeyToChar(int key_code, bool shift);

    InputFocus focus_;
    TextInputState text_input_;
    bool initialized_;

    // Binding string cache
    mutable char binding_string_cache_[64];

    // State query callbacks
    StateQueryFunc has_selection_;
    StateQueryFunc in_placement_mode_;
    StateQueryFunc in_menu_;
};

//=============================================================================
// Global Access
//=============================================================================

bool KeyboardHandler_Init();
void KeyboardHandler_Shutdown();
void KeyboardHandler_ProcessFrame();
void KeyboardHandler_SetFocus(InputFocus focus);
InputFocus KeyboardHandler_GetFocus();

#endif // KEYBOARD_HANDLER_H
