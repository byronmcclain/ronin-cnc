// src/tests/unit/test_input.cpp
// Input System Unit Tests
// Task 18b - Unit Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "platform.h"
#include <cmath>

//=============================================================================
// Keyboard Tests
//=============================================================================

TEST_WITH_FIXTURE(PlatformFixture, Input_KeyDown_InitiallyFalse, "Input") {
    Platform_Input_Update();

    // No keys should be pressed initially
    TEST_ASSERT(!Platform_Key_IsPressed(KEY_CODE_ESCAPE));
    TEST_ASSERT(!Platform_Key_IsPressed(KEY_CODE_SPACE));
    TEST_ASSERT(!Platform_Key_IsPressed(KEY_CODE_RETURN));
}

TEST_CASE(Input_KeyCode_Values, "Input") {
    // Key codes should be distinct
    TEST_ASSERT_NE(KEY_CODE_ESCAPE, KEY_CODE_RETURN);
    TEST_ASSERT_NE(KEY_CODE_SPACE, KEY_CODE_TAB);
    TEST_ASSERT_NE(KEY_CODE_LEFT, KEY_CODE_RIGHT);
}

TEST_CASE(Input_KeyCode_SpecialKeys, "Input") {
    // Special keys should have specific values
    TEST_ASSERT_EQ(KEY_CODE_ESCAPE, 27);
    TEST_ASSERT_EQ(KEY_CODE_SPACE, 32);
    TEST_ASSERT_EQ(KEY_CODE_RETURN, 13);
}

TEST_CASE(Input_KeyCode_FunctionKeys, "Input") {
    // Function keys F1-F12
    TEST_ASSERT_EQ(KEY_CODE_F1, 112);
    TEST_ASSERT_EQ(KEY_CODE_F12, 123);
    TEST_ASSERT_LT(KEY_CODE_F1, KEY_CODE_F12);
}

TEST_CASE(Input_KeyCode_ArrowKeys, "Input") {
    // Arrow keys
    TEST_ASSERT_EQ(KEY_CODE_LEFT, 37);
    TEST_ASSERT_EQ(KEY_CODE_UP, 38);
    TEST_ASSERT_EQ(KEY_CODE_RIGHT, 39);
    TEST_ASSERT_EQ(KEY_CODE_DOWN, 40);
}

TEST_WITH_FIXTURE(PlatformFixture, Input_GetModifiers_InitiallyFalse, "Input") {
    Platform_Input_Update();

    // Modifiers should be off when no keys pressed
    TEST_ASSERT(!Platform_Key_ShiftDown());
    TEST_ASSERT(!Platform_Key_CtrlDown());
    TEST_ASSERT(!Platform_Key_AltDown());
}

//=============================================================================
// Mouse Tests
//=============================================================================

TEST_WITH_FIXTURE(PlatformFixture, Input_MousePosition_Valid, "Input") {
    Platform_Input_Update();

    int32_t x, y;
    Platform_Mouse_GetPosition(&x, &y);

    // Position should be retrievable without crash
    TEST_ASSERT(true);
}

TEST_WITH_FIXTURE(PlatformFixture, Input_MousePosition_GetXY, "Input") {
    Platform_Input_Update();

    int32_t x = Platform_Mouse_GetX();
    int32_t y = Platform_Mouse_GetY();

    // Just verify it doesn't crash
    (void)x;
    (void)y;
    TEST_ASSERT(true);
}

TEST_WITH_FIXTURE(PlatformFixture, Input_MouseButton_InitiallyUp, "Input") {
    Platform_Input_Update();

    // No buttons should be pressed initially
    TEST_ASSERT(!Platform_Mouse_IsPressed(MOUSE_BUTTON_LEFT));
    TEST_ASSERT(!Platform_Mouse_IsPressed(MOUSE_BUTTON_RIGHT));
    TEST_ASSERT(!Platform_Mouse_IsPressed(MOUSE_BUTTON_MIDDLE));
}

TEST_CASE(Input_MouseButton_Values, "Input") {
    // Mouse buttons should have distinct values
    TEST_ASSERT_NE(MOUSE_BUTTON_LEFT, MOUSE_BUTTON_RIGHT);
    TEST_ASSERT_NE(MOUSE_BUTTON_LEFT, MOUSE_BUTTON_MIDDLE);
    TEST_ASSERT_NE(MOUSE_BUTTON_RIGHT, MOUSE_BUTTON_MIDDLE);

    // Check expected values
    TEST_ASSERT_EQ(MOUSE_BUTTON_LEFT, 0);
    TEST_ASSERT_EQ(MOUSE_BUTTON_RIGHT, 1);
    TEST_ASSERT_EQ(MOUSE_BUTTON_MIDDLE, 2);
}

TEST_WITH_FIXTURE(PlatformFixture, Input_MouseWheel_InitiallyZero, "Input") {
    Platform_Input_Update();

    int32_t wheel = Platform_Mouse_GetWheelDelta();
    // Initial wheel delta should be zero
    TEST_ASSERT_EQ(wheel, 0);
}

//=============================================================================
// Input State Tracking Tests
//=============================================================================

TEST_CASE(Input_JustPressed_Logic, "Input") {
    // JustPressed = down now AND was up last frame
    bool down_now = true;
    bool down_last = false;

    bool just_pressed = down_now && !down_last;
    TEST_ASSERT(just_pressed);

    // If was already down, not "just" pressed
    down_last = true;
    just_pressed = down_now && !down_last;
    TEST_ASSERT(!just_pressed);
}

TEST_CASE(Input_JustReleased_Logic, "Input") {
    // JustReleased = up now AND was down last frame
    bool down_now = false;
    bool down_last = true;

    bool just_released = !down_now && down_last;
    TEST_ASSERT(just_released);

    // If was already up, not "just" released
    down_last = false;
    just_released = !down_now && down_last;
    TEST_ASSERT(!just_released);
}

//=============================================================================
// Event Queue Tests
//=============================================================================

TEST_WITH_FIXTURE(PlatformFixture, Input_PumpEvents_DoesNotCrash, "Input") {
    // Pump events multiple times
    for (int i = 0; i < 10; i++) {
        Platform_PollEvents();
        Platform_Input_Update();
    }

    TEST_ASSERT(true);
}

//=============================================================================
// Cursor Tests
//=============================================================================

TEST_WITH_FIXTURE(PlatformFixture, Input_ShowCursor_Toggle, "Input") {
    Platform_Mouse_Show();
    TEST_ASSERT(true);

    Platform_Mouse_Hide();
    TEST_ASSERT(true);

    Platform_Mouse_Show();
    TEST_ASSERT(true);
}

//=============================================================================
// Key State Tests
//=============================================================================

TEST_WITH_FIXTURE(PlatformFixture, Input_WasPressed_InitiallyFalse, "Input") {
    Platform_Input_Update();

    // No keys should have been "just pressed"
    TEST_ASSERT(!Platform_Key_WasPressed(KEY_CODE_ESCAPE));
    TEST_ASSERT(!Platform_Key_WasPressed(KEY_CODE_SPACE));
}

TEST_WITH_FIXTURE(PlatformFixture, Input_WasReleased_InitiallyFalse, "Input") {
    Platform_Input_Update();

    // No keys should have been "just released"
    TEST_ASSERT(!Platform_Key_WasReleased(KEY_CODE_ESCAPE));
    TEST_ASSERT(!Platform_Key_WasReleased(KEY_CODE_SPACE));
}

TEST_WITH_FIXTURE(PlatformFixture, Input_KeyClear, "Input") {
    Platform_Input_Update();
    Platform_Key_Clear();

    // After clear, no keys should be pressed
    TEST_ASSERT(!Platform_Key_IsPressed(KEY_CODE_ESCAPE));
    TEST_ASSERT(true);
}

//=============================================================================
// Mouse Click Tests
//=============================================================================

TEST_WITH_FIXTURE(PlatformFixture, Input_WasClicked_InitiallyFalse, "Input") {
    Platform_Input_Update();

    // No buttons should have been clicked
    TEST_ASSERT(!Platform_Mouse_WasClicked(MOUSE_BUTTON_LEFT));
    TEST_ASSERT(!Platform_Mouse_WasClicked(MOUSE_BUTTON_RIGHT));
}

TEST_WITH_FIXTURE(PlatformFixture, Input_WasDoubleClicked_InitiallyFalse, "Input") {
    Platform_Input_Update();

    // No double clicks
    TEST_ASSERT(!Platform_Mouse_WasDoubleClicked(MOUSE_BUTTON_LEFT));
    TEST_ASSERT(!Platform_Mouse_WasDoubleClicked(MOUSE_BUTTON_RIGHT));
}
