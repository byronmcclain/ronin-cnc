// src/test_keyboard_handler.cpp
// Test program for keyboard handler
// Task 16c - Keyboard Input Handler

#include "game/input/keyboard_handler.h"
#include "game/input/input_mapper.h"
#include "game/input/input_state.h"
#include "game/viewport.h"
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

bool Test_TextConfirmCancel() {
    printf("Test: Text Confirm/Cancel... ");

    TextInputState tis;
    tis.Reset(32);

    // Insert some text
    tis.InsertChar('T');
    tis.InsertChar('e');
    tis.InsertChar('s');
    tis.InsertChar('t');

    // Confirm
    tis.Confirm();
    if (tis.active) {
        printf("FAILED - Should not be active after confirm\n");
        return false;
    }
    if (!tis.confirmed) {
        printf("FAILED - Should be confirmed\n");
        return false;
    }

    // Test cancel
    tis.Reset(32);
    tis.InsertChar('X');
    tis.Cancel();
    if (tis.active) {
        printf("FAILED - Should not be active after cancel\n");
        return false;
    }
    if (!tis.cancelled) {
        printf("FAILED - Should be cancelled\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_FocusModes() {
    printf("Test: Focus Mode Switching... ");

    Platform_Init();
    Platform_Graphics_Init();

    // Initialize viewport for coordinate conversion
    GameViewport::Instance().Initialize();
    GameViewport::Instance().SetMapSize(64, 64);

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
        Platform_Graphics_Shutdown();
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
        Platform_Graphics_Shutdown();
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
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return false;
    }

    if (!kb.IsTextInputActive()) {
        printf("FAILED - Text input should be active\n");
        KeyboardHandler_Shutdown();
        InputMapper_Shutdown();
        Input_Shutdown();
        Platform_Graphics_Shutdown();
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
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return false;
    }

    KeyboardHandler_Shutdown();
    InputMapper_Shutdown();
    Input_Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_BindingStrings() {
    printf("Test: Binding Strings... ");

    Platform_Init();
    Platform_Graphics_Init();

    // Initialize viewport for coordinate conversion
    GameViewport::Instance().Initialize();
    GameViewport::Instance().SetMapSize(64, 64);

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
        Platform_Graphics_Shutdown();
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
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return false;
    }

    KeyboardHandler_Shutdown();
    InputMapper_Shutdown();
    Input_Shutdown();
    Platform_Graphics_Shutdown();
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

bool Test_CursorBounds() {
    printf("Test: Cursor Bounds... ");

    TextInputState tis;
    tis.Reset(32);

    // Insert text
    tis.InsertChar('A');
    tis.InsertChar('B');
    tis.InsertChar('C');

    // Try to move beyond end
    tis.MoveCursor(100);
    if (tis.cursor_pos != 3) {
        printf("FAILED - Cursor should be clamped to length\n");
        return false;
    }

    // Try to move before start
    tis.MoveCursor(-100);
    if (tis.cursor_pos != 0) {
        printf("FAILED - Cursor should be clamped to 0\n");
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

    // Initialize viewport for coordinate conversion
    GameViewport::Instance().Initialize();
    GameViewport::Instance().SetMapSize(64, 64);

    Input_Init();
    InputMapper_Init();
    KeyboardHandler_Init();

    KeyboardHandler& kb = KeyboardHandler::Instance();

    bool running = true;
    bool in_menu = false;

    while (running) {
        Platform_PollEvents();
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
                    printf("\nText confirmed: '%s'\n", tis.text);
                } else if (tis.cancelled) {
                    printf("\nText cancelled\n");
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

    // Check for quick mode
    bool quick_mode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--quick") == 0 || strcmp(argv[i], "-q") == 0) {
            quick_mode = true;
        }
    }

    int passed = 0;
    int failed = 0;

    // Run unit tests
    if (Test_TextInputState()) passed++; else failed++;
    if (Test_TextConfirmCancel()) passed++; else failed++;
    if (Test_FocusModes()) passed++; else failed++;
    if (Test_BindingStrings()) passed++; else failed++;
    if (Test_KeyNames()) passed++; else failed++;
    if (Test_CursorBounds()) passed++; else failed++;

    printf("\n");
    if (failed == 0) {
        printf("All tests PASSED (%d/%d)\n", passed, passed + failed);
    } else {
        printf("Results: %d passed, %d failed\n", passed, failed);
    }

    // Interactive test
    if (!quick_mode && argc > 1 && strcmp(argv[1], "-i") == 0) {
        Interactive_Test();
    } else if (!quick_mode) {
        printf("\nRun with -i for interactive test\n");
    }

    return (failed == 0) ? 0 : 1;
}
