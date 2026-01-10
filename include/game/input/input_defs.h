// include/game/input/input_defs.h
// Key code definitions and input constants
// Task 16a - Input Foundation

#ifndef INPUT_DEFS_H
#define INPUT_DEFS_H

#include <cstdint>

//=============================================================================
// Key Code Definitions
//=============================================================================
// These map to platform key codes but provide game-specific abstraction.
// We use the platform's KeyCode enum values directly for compatibility.

// Special keys (match platform.h KeyCode values)
constexpr int KEY_NONE          = 0;
constexpr int KEY_ESCAPE        = 27;
constexpr int KEY_RETURN        = 13;
constexpr int KEY_SPACE         = 32;
constexpr int KEY_BACKSPACE     = 8;
constexpr int KEY_TAB           = 9;

// Arrow keys (Windows VK values from platform)
constexpr int KEY_UP            = 38;
constexpr int KEY_DOWN          = 40;
constexpr int KEY_LEFT          = 37;
constexpr int KEY_RIGHT         = 39;

// Function keys
constexpr int KEY_F1            = 112;
constexpr int KEY_F2            = 113;
constexpr int KEY_F3            = 114;
constexpr int KEY_F4            = 115;
constexpr int KEY_F5            = 116;
constexpr int KEY_F6            = 117;
constexpr int KEY_F7            = 118;
constexpr int KEY_F8            = 119;
constexpr int KEY_F9            = 120;
constexpr int KEY_F10           = 121;
constexpr int KEY_F11           = 122;
constexpr int KEY_F12           = 123;

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

// Modifier keys (match platform values)
constexpr int KEY_SHIFT         = 16;
constexpr int KEY_CONTROL       = 17;
constexpr int KEY_ALT           = 18;

// Other useful keys
constexpr int KEY_HOME          = 36;
constexpr int KEY_END           = 35;
constexpr int KEY_PAGEUP        = 33;
constexpr int KEY_PAGEDOWN      = 34;
constexpr int KEY_INSERT        = 45;
constexpr int KEY_DELETE        = 46;
constexpr int KEY_PAUSE         = 19;

// Maximum key code value for array sizing
constexpr int KEY_CODE_MAX      = 256;

//=============================================================================
// Mouse Button Definitions
//=============================================================================

constexpr int INPUT_MOUSE_LEFT      = 0;
constexpr int INPUT_MOUSE_RIGHT     = 1;
constexpr int INPUT_MOUSE_MIDDLE    = 2;
constexpr int INPUT_MOUSE_MAX       = 5;

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
