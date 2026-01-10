// src/game/input/keyboard_handler.cpp
// Keyboard input handler implementation
// Task 16c - Keyboard Input Handler

#include "game/input/keyboard_handler.h"
#include "game/input/input_mapper.h"
#include "game/input/input_state.h"
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
    max_length = MAX_TEXT_LENGTH;
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

    Platform_LogInfo("KeyboardHandler: Initializing...");

    focus_ = InputFocus::GAME;
    text_input_.Clear();

    initialized_ = true;
    Platform_LogInfo("KeyboardHandler: Initialized");
    return true;
}

void KeyboardHandler::Shutdown() {
    if (!initialized_) return;

    Platform_LogInfo("KeyboardHandler: Shutting down");

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
        case KEY_SHIFT: return "Shift";
        case KEY_CONTROL: return "Ctrl";
        case KEY_ALT: return "Alt";
        case KEY_PAUSE: return "Pause";
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
