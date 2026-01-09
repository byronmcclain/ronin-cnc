# Task 16c: Keyboard Input Handler

| Field | Value |
|-------|-------|
| **Task ID** | 16c |
| **Task Name** | Keyboard Input Handler |
| **Phase** | 16 - Input Integration |
| **Depends On** | 16a (Input Foundation), 16b (Action Mapping) |
| **Estimated Complexity** | Medium (~400 lines) |

---

## Dependencies

- Task 16a (Input Foundation) must be complete
- Task 16b (Action Mapping) must be complete
- InputState and InputMapper must be functional

---

## Context

This task implements the keyboard-specific input handler that processes all keyboard shortcuts and actions. While the InputMapper handles the low-level key-to-action mapping, this handler provides the game-specific logic for:

1. **Context-sensitive actions**: Same key may do different things based on game state
2. **Key repeat handling**: For text input in menus/chat
3. **Hotkey display**: For UI hints showing current bindings
4. **Input focus**: Handling when keyboard should be captured vs passed through

The original Red Alert had keyboard handling scattered throughout the codebase. This task centralizes it for better maintainability.

---

## Objective

Create a keyboard handler that:
1. Processes all keyboard shortcuts appropriately
2. Handles context-sensitive key interpretation
3. Supports text input mode for chat/menus
4. Provides binding information for UI display
5. Manages keyboard input focus states

---

## Technical Background

### Input Focus States

The keyboard can be in different focus modes:

```
GAME_FOCUS:     Normal gameplay - all hotkeys active
MENU_FOCUS:     In a menu - only menu navigation keys
TEXT_FOCUS:     Typing text - all keys go to text buffer
NONE:           Input disabled (loading, cutscene)
```

### Context-Sensitive Keys

Some keys change meaning based on game state:

```
Key: ESCAPE
  - Nothing selected → Open options menu
  - Units selected → Deselect all
  - In placement mode → Cancel placement
  - In menu → Close menu

Key: A
  - Units selected → Attack move order
  - In diplomacy → Toggle alliance
```

### Text Input

When in TEXT_FOCUS mode:
- All printable keys go to text buffer
- Backspace deletes
- Enter confirms
- Escape cancels
- Arrow keys move cursor (if supported)

---

## Deliverables

- [ ] `src/game/input/keyboard_handler.h` - Handler class declaration
- [ ] `src/game/input/keyboard_handler.cpp` - Handler implementation
- [ ] `tests/test_keyboard_handler.cpp` - Test program
- [ ] CMakeLists.txt updated
- [ ] All tests pass

---

## Files to Create

### src/game/input/keyboard_handler.h

```cpp
// src/game/input/keyboard_handler.h
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
```

### src/game/input/keyboard_handler.cpp

```cpp
// src/game/input/keyboard_handler.cpp
// Keyboard input handler implementation
// Task 16c - Keyboard Input Handler

#include "keyboard_handler.h"
#include "input_mapper.h"
#include "input_state.h"
#include "platform.h"
#include <cstring>
#include <cstdio>

//=============================================================================
// TextInputState Implementation
//=============================================================================

void TextInputState::Clear() {
    memset(text, 0, sizeof(text));
    cursor_pos = 0;
    text_length = 0;
    active = false;
    confirmed = false;
    cancelled = false;
}

void TextInputState::Reset(int max_len) {
    Clear();
    max_length = (max_len < MAX_TEXT_LENGTH) ? max_len : MAX_TEXT_LENGTH;
    active = true;
}

void TextInputState::InsertChar(char c) {
    if (text_length >= max_length - 1) return;  // Leave room for null
    if (c == 0) return;

    // Shift characters after cursor
    for (int i = text_length; i > cursor_pos; i--) {
        text[i] = text[i - 1];
    }
    text[cursor_pos] = c;
    cursor_pos++;
    text_length++;
    text[text_length] = '\0';
}

void TextInputState::DeleteChar() {
    if (cursor_pos >= text_length) return;

    // Shift characters left
    for (int i = cursor_pos; i < text_length; i++) {
        text[i] = text[i + 1];
    }
    text_length--;
}

void TextInputState::Backspace() {
    if (cursor_pos <= 0) return;
    cursor_pos--;
    DeleteChar();
}

void TextInputState::MoveCursor(int delta) {
    cursor_pos += delta;
    if (cursor_pos < 0) cursor_pos = 0;
    if (cursor_pos > text_length) cursor_pos = text_length;
}

void TextInputState::Confirm() {
    confirmed = true;
    active = false;
}

void TextInputState::Cancel() {
    cancelled = true;
    active = false;
}

//=============================================================================
// KeyboardHandler Singleton
//=============================================================================

KeyboardHandler& KeyboardHandler::Instance() {
    static KeyboardHandler instance;
    return instance;
}

KeyboardHandler::KeyboardHandler()
    : focus_(InputFocus::GAME)
    , initialized_(false)
    , has_selection_(nullptr)
    , in_placement_mode_(nullptr)
    , in_menu_(nullptr) {
    text_input_.Clear();
    memset(binding_string_cache_, 0, sizeof(binding_string_cache_));
}

KeyboardHandler::~KeyboardHandler() {
    Shutdown();
}

bool KeyboardHandler::Initialize() {
    if (initialized_) return true;

    Platform_Log("KeyboardHandler: Initializing...");

    focus_ = InputFocus::GAME;
    text_input_.Clear();

    initialized_ = true;
    Platform_Log("KeyboardHandler: Initialized");
    return true;
}

void KeyboardHandler::Shutdown() {
    if (!initialized_) return;

    Platform_Log("KeyboardHandler: Shutting down");

    text_input_.Clear();
    initialized_ = false;
}

void KeyboardHandler::ProcessFrame() {
    if (!initialized_) return;

    switch (focus_) {
        case InputFocus::GAME:
            ProcessGameFocus();
            break;
        case InputFocus::MENU:
            ProcessMenuFocus();
            break;
        case InputFocus::TEXT:
            ProcessTextFocus();
            break;
        case InputFocus::DISABLED:
            // Do nothing
            break;
    }
}

void KeyboardHandler::ProcessGameFocus() {
    // Game focus processing is mostly handled by InputMapper
    // This layer adds context-sensitive interpretation

    // Nothing special needed here - game code queries actions directly
}

void KeyboardHandler::ProcessMenuFocus() {
    // In menu focus, only allow navigation keys
    InputState& input = InputState::Instance();

    // Menu navigation is typically handled by the menu system itself
    // We just block game actions
}

void KeyboardHandler::ProcessTextFocus() {
    if (!text_input_.active) return;

    InputState& input = InputState::Instance();

    // Handle special keys
    if (input.WasKeyPressed(KEY_RETURN)) {
        text_input_.Confirm();
        return;
    }

    if (input.WasKeyPressed(KEY_ESCAPE)) {
        text_input_.Cancel();
        return;
    }

    if (input.WasKeyPressed(KEY_BACKSPACE)) {
        text_input_.Backspace();
    }

    if (input.WasKeyPressed(KEY_DELETE)) {
        text_input_.DeleteChar();
    }

    if (input.WasKeyPressed(KEY_LEFT)) {
        text_input_.MoveCursor(-1);
    }

    if (input.WasKeyPressed(KEY_RIGHT)) {
        text_input_.MoveCursor(1);
    }

    if (input.WasKeyPressed(KEY_HOME)) {
        text_input_.cursor_pos = 0;
    }

    if (input.WasKeyPressed(KEY_END)) {
        text_input_.cursor_pos = text_input_.text_length;
    }

    // Process character input from key buffer
    KeyboardState& kb = input.Keyboard();
    while (kb.HasBufferedKeys()) {
        int key = kb.GetBufferedKey();
        char c = KeyToChar(key, input.IsShiftDown());
        if (c != 0) {
            text_input_.InsertChar(c);
        }
    }
}

char KeyboardHandler::KeyToChar(int key_code, bool shift) {
    // Convert key code to printable character
    // Letters
    if (key_code >= 'A' && key_code <= 'Z') {
        if (shift) {
            return static_cast<char>(key_code);
        } else {
            return static_cast<char>(key_code + 32);  // lowercase
        }
    }

    // Numbers with shift (symbols)
    if (key_code >= '0' && key_code <= '9') {
        if (shift) {
            const char* shift_nums = ")!@#$%^&*(";
            return shift_nums[key_code - '0'];
        } else {
            return static_cast<char>(key_code);
        }
    }

    // Space
    if (key_code == KEY_SPACE) {
        return ' ';
    }

    // Common punctuation (simplified - expand as needed)
    if (key_code == '-') return shift ? '_' : '-';
    if (key_code == '=') return shift ? '+' : '=';
    if (key_code == '[') return shift ? '{' : '[';
    if (key_code == ']') return shift ? '}' : ']';
    if (key_code == ';') return shift ? ':' : ';';
    if (key_code == '\'') return shift ? '"' : '\'';
    if (key_code == ',') return shift ? '<' : ',';
    if (key_code == '.') return shift ? '>' : '.';
    if (key_code == '/') return shift ? '?' : '/';
    if (key_code == '\\') return shift ? '|' : '\\';
    if (key_code == '`') return shift ? '~' : '`';

    return 0;  // Non-printable
}

//=============================================================================
// Focus Management
//=============================================================================

void KeyboardHandler::SetFocus(InputFocus focus) {
    if (focus_ == InputFocus::TEXT && focus != InputFocus::TEXT) {
        // Leaving text input mode
        if (text_input_.active) {
            text_input_.Cancel();
        }
    }
    focus_ = focus;
}

void KeyboardHandler::BeginTextInput(int max_length) {
    text_input_.Reset(max_length);
    focus_ = InputFocus::TEXT;
}

void KeyboardHandler::EndTextInput() {
    text_input_.active = false;
    focus_ = InputFocus::GAME;
}

//=============================================================================
// Context-Sensitive Queries
//=============================================================================

bool KeyboardHandler::ShouldOpenOptionsMenu() const {
    // ESCAPE opens options only if:
    // - Not in placement mode
    // - Nothing selected (or allow anyway and deselect handles it)
    // - Not in a menu already

    if (in_menu_ && in_menu_()) return false;
    if (in_placement_mode_ && in_placement_mode_()) return false;

    return InputMapper::Instance().WasActionTriggered(GameAction::UI_OPTIONS_MENU);
}

bool KeyboardHandler::ShouldDeselectAll() const {
    // ESCAPE deselects if:
    // - Something is selected
    // - Not in placement mode (that takes priority)

    if (in_placement_mode_ && in_placement_mode_()) return false;
    if (!has_selection_ || !has_selection_()) return false;

    return InputMapper::Instance().WasActionTriggered(GameAction::DESELECT_ALL);
}

bool KeyboardHandler::ShouldCancelPlacement() const {
    // ESCAPE cancels placement if:
    // - In placement mode

    if (!in_placement_mode_ || !in_placement_mode_()) return false;

    return InputMapper::Instance().WasActionTriggered(GameAction::BUILDING_CANCEL);
}

//=============================================================================
// Binding Info
//=============================================================================

const char* KeyboardHandler::GetKeyName(int key_code) {
    switch (key_code) {
        case KEY_ESCAPE: return "ESC";
        case KEY_RETURN: return "Enter";
        case KEY_SPACE: return "Space";
        case KEY_TAB: return "Tab";
        case KEY_BACKSPACE: return "Backspace";
        case KEY_UP: return "Up";
        case KEY_DOWN: return "Down";
        case KEY_LEFT: return "Left";
        case KEY_RIGHT: return "Right";
        case KEY_F1: return "F1";
        case KEY_F2: return "F2";
        case KEY_F3: return "F3";
        case KEY_F4: return "F4";
        case KEY_F5: return "F5";
        case KEY_F6: return "F6";
        case KEY_F7: return "F7";
        case KEY_F8: return "F8";
        case KEY_F9: return "F9";
        case KEY_F10: return "F10";
        case KEY_F11: return "F11";
        case KEY_F12: return "F12";
        case KEY_HOME: return "Home";
        case KEY_END: return "End";
        case KEY_PAGEUP: return "PgUp";
        case KEY_PAGEDOWN: return "PgDn";
        case KEY_INSERT: return "Ins";
        case KEY_DELETE: return "Del";
        case KEY_LSHIFT:
        case KEY_RSHIFT: return "Shift";
        case KEY_LCTRL:
        case KEY_RCTRL: return "Ctrl";
        case KEY_LALT:
        case KEY_RALT: return "Alt";
        default:
            if (key_code >= 'A' && key_code <= 'Z') {
                static char letter[2] = {0, 0};
                letter[0] = static_cast<char>(key_code);
                return letter;
            }
            if (key_code >= '0' && key_code <= '9') {
                static char digit[2] = {0, 0};
                digit[0] = static_cast<char>(key_code);
                return digit;
            }
            return "???";
    }
}

const char* KeyboardHandler::GetBindingString(GameAction action) const {
    const KeyBinding* binding = InputMapper::Instance().GetBinding(action);
    if (!binding || binding->key_code == 0) {
        return "Unbound";
    }

    binding_string_cache_[0] = '\0';

    // Build modifier prefix
    if (binding->required_mods & MOD_CTRL) {
        strcat(binding_string_cache_, "Ctrl+");
    }
    if (binding->required_mods & MOD_SHIFT) {
        strcat(binding_string_cache_, "Shift+");
    }
    if (binding->required_mods & MOD_ALT) {
        strcat(binding_string_cache_, "Alt+");
    }

    // Add key name
    strcat(binding_string_cache_, GetKeyName(binding->key_code));

    return binding_string_cache_;
}

//=============================================================================
// Global Functions
//=============================================================================

bool KeyboardHandler_Init() {
    return KeyboardHandler::Instance().Initialize();
}

void KeyboardHandler_Shutdown() {
    KeyboardHandler::Instance().Shutdown();
}

void KeyboardHandler_ProcessFrame() {
    KeyboardHandler::Instance().ProcessFrame();
}

void KeyboardHandler_SetFocus(InputFocus focus) {
    KeyboardHandler::Instance().SetFocus(focus);
}

InputFocus KeyboardHandler_GetFocus() {
    return KeyboardHandler::Instance().GetFocus();
}
```

### tests/test_keyboard_handler.cpp

```cpp
// tests/test_keyboard_handler.cpp
// Test program for keyboard handler
// Task 16c - Keyboard Input Handler

#include "keyboard_handler.h"
#include "input_mapper.h"
#include "input_state.h"
#include "platform.h"
#include <cstdio>
#include <cstring>

//=============================================================================
// Unit Tests
//=============================================================================

bool Test_TextInputState() {
    printf("Test: TextInputState... ");

    TextInputState tis;
    tis.Clear();

    // Initial state
    if (tis.active || tis.text_length != 0) {
        printf("FAILED - Not cleared\n");
        return false;
    }

    // Reset activates
    tis.Reset(32);
    if (!tis.active || tis.max_length != 32) {
        printf("FAILED - Reset didn't work\n");
        return false;
    }

    // Insert characters
    tis.InsertChar('H');
    tis.InsertChar('e');
    tis.InsertChar('l');
    tis.InsertChar('l');
    tis.InsertChar('o');

    if (strcmp(tis.text, "Hello") != 0) {
        printf("FAILED - Expected 'Hello', got '%s'\n", tis.text);
        return false;
    }

    if (tis.cursor_pos != 5 || tis.text_length != 5) {
        printf("FAILED - Cursor/length wrong\n");
        return false;
    }

    // Backspace
    tis.Backspace();
    if (strcmp(tis.text, "Hell") != 0) {
        printf("FAILED - Backspace failed, got '%s'\n", tis.text);
        return false;
    }

    // Move cursor and insert
    tis.MoveCursor(-2);  // Now at position 2 (after "He")
    tis.InsertChar('X');
    if (strcmp(tis.text, "HeXll") != 0) {
        printf("FAILED - Insert at cursor failed, got '%s'\n", tis.text);
        return false;
    }

    // Delete at cursor
    tis.DeleteChar();  // Delete 'l' after X
    if (strcmp(tis.text, "HeXl") != 0) {
        printf("FAILED - Delete failed, got '%s'\n", tis.text);
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_KeyToChar() {
    printf("Test: Key to Character Conversion... ");

    // We need to access the private method through testing
    // For now, just verify the public interface works

    Platform_Init();
    Input_Init();
    InputMapper_Init();
    KeyboardHandler_Init();

    // Can't easily test private KeyToChar, but can verify text input works
    // through the interactive test

    KeyboardHandler_Shutdown();
    InputMapper_Shutdown();
    Input_Shutdown();
    Platform_Shutdown();

    printf("PASSED (limited - see interactive test)\n");
    return true;
}

bool Test_FocusModes() {
    printf("Test: Focus Mode Switching... ");

    Platform_Init();
    Input_Init();
    InputMapper_Init();
    KeyboardHandler_Init();

    KeyboardHandler& kb = KeyboardHandler::Instance();

    // Default should be GAME
    if (kb.GetFocus() != InputFocus::GAME) {
        printf("FAILED - Default focus should be GAME\n");
        KeyboardHandler_Shutdown();
        InputMapper_Shutdown();
        Input_Shutdown();
        Platform_Shutdown();
        return false;
    }

    // Switch to MENU
    kb.SetFocus(InputFocus::MENU);
    if (kb.GetFocus() != InputFocus::MENU) {
        printf("FAILED - Should be MENU\n");
        KeyboardHandler_Shutdown();
        InputMapper_Shutdown();
        Input_Shutdown();
        Platform_Shutdown();
        return false;
    }

    // Begin text input
    kb.BeginTextInput(100);
    if (kb.GetFocus() != InputFocus::TEXT) {
        printf("FAILED - Should be TEXT\n");
        KeyboardHandler_Shutdown();
        InputMapper_Shutdown();
        Input_Shutdown();
        Platform_Shutdown();
        return false;
    }

    if (!kb.IsTextInputActive()) {
        printf("FAILED - Text input should be active\n");
        KeyboardHandler_Shutdown();
        InputMapper_Shutdown();
        Input_Shutdown();
        Platform_Shutdown();
        return false;
    }

    // End text input
    kb.EndTextInput();
    if (kb.GetFocus() != InputFocus::GAME) {
        printf("FAILED - Should return to GAME\n");
        KeyboardHandler_Shutdown();
        InputMapper_Shutdown();
        Input_Shutdown();
        Platform_Shutdown();
        return false;
    }

    KeyboardHandler_Shutdown();
    InputMapper_Shutdown();
    Input_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_BindingStrings() {
    printf("Test: Binding Strings... ");

    Platform_Init();
    Input_Init();
    InputMapper_Init();
    KeyboardHandler_Init();

    KeyboardHandler& kb = KeyboardHandler::Instance();

    // Check SELECT_ALL binding string
    const char* select_all = kb.GetBindingString(GameAction::SELECT_ALL);
    if (strcmp(select_all, "Ctrl+A") != 0) {
        printf("FAILED - SELECT_ALL should be 'Ctrl+A', got '%s'\n", select_all);
        KeyboardHandler_Shutdown();
        InputMapper_Shutdown();
        Input_Shutdown();
        Platform_Shutdown();
        return false;
    }

    // Check ORDER_STOP binding string
    const char* stop = kb.GetBindingString(GameAction::ORDER_STOP);
    if (strcmp(stop, "S") != 0) {
        printf("FAILED - ORDER_STOP should be 'S', got '%s'\n", stop);
        KeyboardHandler_Shutdown();
        InputMapper_Shutdown();
        Input_Shutdown();
        Platform_Shutdown();
        return false;
    }

    KeyboardHandler_Shutdown();
    InputMapper_Shutdown();
    Input_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_KeyNames() {
    printf("Test: Key Names... ");

    if (strcmp(KeyboardHandler::GetKeyName(KEY_ESCAPE), "ESC") != 0) {
        printf("FAILED - KEY_ESCAPE name wrong\n");
        return false;
    }

    if (strcmp(KeyboardHandler::GetKeyName(KEY_F1), "F1") != 0) {
        printf("FAILED - KEY_F1 name wrong\n");
        return false;
    }

    if (strcmp(KeyboardHandler::GetKeyName(KEY_A), "A") != 0) {
        printf("FAILED - KEY_A name wrong\n");
        return false;
    }

    if (strcmp(KeyboardHandler::GetKeyName(KEY_1), "1") != 0) {
        printf("FAILED - KEY_1 name wrong\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

//=============================================================================
// Interactive Test
//=============================================================================

void Interactive_Test() {
    printf("\n=== Interactive Keyboard Handler Test ===\n");
    printf("Press keys to test text input and focus modes\n");
    printf("T: Begin text input mode\n");
    printf("ESC: Exit text input or quit\n");
    printf("M: Toggle menu focus\n\n");

    Platform_Init();
    Platform_Graphics_Init();
    Input_Init();
    InputMapper_Init();
    KeyboardHandler_Init();

    KeyboardHandler& kb = KeyboardHandler::Instance();

    bool running = true;
    bool in_menu = false;

    while (running) {
        Platform_PumpEvents();
        Input_Update();
        InputMapper_ProcessFrame();
        KeyboardHandler_ProcessFrame();

        InputFocus focus = kb.GetFocus();

        // Handle based on focus mode
        if (focus == InputFocus::TEXT) {
            TextInputState& tis = kb.GetTextInput();

            if (!tis.active) {
                // Input ended
                if (tis.confirmed) {
                    printf("Text confirmed: '%s'\n", tis.text);
                } else if (tis.cancelled) {
                    printf("Text cancelled\n");
                }
                kb.SetFocus(InputFocus::GAME);
            } else {
                // Show current text
                static int last_len = -1;
                if (tis.text_length != last_len) {
                    printf("\rText: '%s' (cursor at %d)     ", tis.text, tis.cursor_pos);
                    fflush(stdout);
                    last_len = tis.text_length;
                }
            }
        } else {
            // Normal mode
            if (Input_KeyPressed(KEY_ESCAPE)) {
                if (in_menu) {
                    in_menu = false;
                    kb.SetFocus(InputFocus::GAME);
                    printf("Exited menu mode\n");
                } else {
                    running = false;
                }
                continue;
            }

            if (Input_KeyPressed(KEY_T)) {
                printf("Entering text input mode (type, Enter to confirm, ESC to cancel)\n");
                kb.BeginTextInput(64);
                continue;
            }

            if (Input_KeyPressed(KEY_M)) {
                in_menu = !in_menu;
                kb.SetFocus(in_menu ? InputFocus::MENU : InputFocus::GAME);
                printf("Focus: %s\n", in_menu ? "MENU" : "GAME");
                continue;
            }

            // Show any triggered actions (in GAME mode)
            if (focus == InputFocus::GAME) {
                for (int i = 0; i < static_cast<int>(GameAction::ACTION_COUNT); i++) {
                    GameAction action = static_cast<GameAction>(i);
                    if (InputMapper_WasTriggered(action)) {
                        printf("Action: %s (%s)\n",
                               GetActionName(action),
                               kb.GetBindingString(action));
                    }
                }
            }
        }

        Platform_Timer_Delay(16);
    }

    KeyboardHandler_Shutdown();
    InputMapper_Shutdown();
    Input_Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Main
//=============================================================================

int main(int argc, char* argv[]) {
    printf("=== Keyboard Handler Tests (Task 16c) ===\n\n");

    int passed = 0;
    int failed = 0;

    // Run unit tests
    if (Test_TextInputState()) passed++; else failed++;
    if (Test_KeyToChar()) passed++; else failed++;
    if (Test_FocusModes()) passed++; else failed++;
    if (Test_BindingStrings()) passed++; else failed++;
    if (Test_KeyNames()) passed++; else failed++;

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
# Keyboard Handler (Task 16c)
target_sources(game PRIVATE
    input/keyboard_handler.h
    input/keyboard_handler.cpp
)
```

Add to `tests/CMakeLists.txt`:

```cmake
# Keyboard Handler test (Task 16c)
add_executable(test_keyboard_handler test_keyboard_handler.cpp)
target_link_libraries(test_keyboard_handler PRIVATE game platform)
add_test(NAME KeyboardHandlerTest COMMAND test_keyboard_handler)
```

---

## Verification Script

```bash
#!/bin/bash
set -e

echo "=== Task 16c Verification ==="

# Build
echo "Building..."
cd build
cmake --build . --target test_keyboard_handler

# Run tests
echo "Running unit tests..."
./tests/test_keyboard_handler

# Check files exist
echo "Checking source files..."
for file in \
    "../src/game/input/keyboard_handler.h" \
    "../src/game/input/keyboard_handler.cpp"
do
    if [ ! -f "$file" ]; then
        echo "ERROR: Missing $file"
        exit 1
    fi
done

echo ""
echo "=== Task 16c Verification PASSED ==="
echo "Run './tests/test_keyboard_handler -i' for interactive test"
```

---

## Implementation Steps

1. **Create keyboard_handler.h** - Define classes and enums
2. **Create keyboard_handler.cpp** - Full implementation
3. **Create test_keyboard_handler.cpp** - Unit and interactive tests
4. **Update CMakeLists.txt** - Add new files
5. **Build and run tests** - Verify all pass
6. **Test interactively** - Verify text input works

---

## Success Criteria

- [ ] TextInputState manages text buffer correctly
- [ ] Insert, delete, backspace work
- [ ] Cursor movement works
- [ ] Focus mode switching works
- [ ] Text input mode activates/deactivates
- [ ] Binding strings generated correctly
- [ ] Key names return correct strings
- [ ] All unit tests pass
- [ ] Interactive test shows working text input

---

## Common Issues

1. **Text buffer overflow**
   - Check max_length enforcement
   - Ensure null terminator space reserved

2. **Cursor out of bounds**
   - Clamp cursor_pos in MoveCursor
   - Verify after deletions

3. **Key to char mapping incomplete**
   - Add missing punctuation as needed
   - Consider locale issues (simplified for now)

---

## Completion Promise

When verification passes, output:
<promise>TASK_16C_COMPLETE</promise>

## Escape Hatch

If stuck after 15 iterations:
- Document what's blocking in `ralph-tasks/blocked/16c.md`
- Output:
<promise>TASK_16C_BLOCKED</promise>

## Max Iterations
15
