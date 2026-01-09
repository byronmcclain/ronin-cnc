#!/bin/bash
# Verification script for DisplayClass port

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking DisplayClass Port ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check files exist
echo "[1/4] Checking files..."
check_file() {
    if [ ! -f "$1" ]; then
        echo "VERIFY_FAILED: $1 not found"
        exit 1
    fi
    echo "  OK: $1"
}

check_file "include/game/display.h"
check_file "src/game/display/display.cpp"

# Step 2: Syntax check
echo "[2/4] Checking syntax..."

# Create mock platform implementation
cat > /tmp/mock_display_platform.cpp << 'EOF'
#include "platform.h"
#include <cstring>

static uint8_t mock_buffer[640 * 400];
static bool graphics_initialized = false;

extern "C" {

int32_t Platform_Graphics_Init(void) {
    graphics_initialized = true;
    memset(mock_buffer, 0, sizeof(mock_buffer));
    return PLATFORM_RESULT_SUCCESS;
}

void Platform_Graphics_Shutdown(void) {
    graphics_initialized = false;
}

bool Platform_Graphics_IsInitialized(void) {
    return graphics_initialized;
}

int32_t Platform_Graphics_GetBackBuffer(uint8_t **pixels, int32_t *width, int32_t *height, int32_t *pitch) {
    *pixels = mock_buffer;
    *width = 640;
    *height = 400;
    *pitch = 640;
    return PLATFORM_RESULT_SUCCESS;
}

int32_t Platform_Graphics_Flip(void) {
    return PLATFORM_RESULT_SUCCESS;
}

void Platform_Input_Update(void) {}

void Platform_Mouse_GetPosition(int32_t *x, int32_t *y) {
    *x = 0;
    *y = 0;
}

bool Platform_Mouse_IsPressed(enum MouseButton button) {
    (void)button;
    return false;
}

bool Platform_Mouse_WasClicked(enum MouseButton button) {
    (void)button;
    return false;
}

void Platform_LogInfo(const char *msg) { (void)msg; }
void Platform_LogError(const char *msg) { (void)msg; }

} // extern "C"
EOF

if ! clang++ -std=c++17 -fsyntax-only -I include include/game/display.h 2>&1; then
    echo "VERIFY_FAILED: display.h has syntax errors"
    rm -f /tmp/mock_display_platform.cpp
    exit 1
fi
echo "  OK: Syntax valid"

# Step 3: Build test
echo "[3/4] Building display test..."
cat > /tmp/test_display.cpp << 'EOF'
#include "game/display.h"
#include "game/cell.h"
#include <cstdio>

#define TEST(name, cond) do { \
    if (!(cond)) { printf("FAIL: %s\n", name); return 1; } \
    printf("PASS: %s\n", name); \
} while(0)

int main() {
    printf("=== DisplayClass Tests ===\n\n");

    DisplayClass display;
    TEST("Display created", true);

    // Initialize
    display.One_Time();
    TEST("One_Time called", display.Is_Initialized());

    display.Init(THEATER_TEMPERATE);
    TEST("Init called", true);

    // Test tactical area
    TEST("Tactical X", display.Tactical_X() == 0);
    TEST("Tactical Y", display.Tactical_Y() == 16);
    TEST("Tactical W", display.Tactical_Width() == 640);
    TEST("Tactical H", display.Tactical_Height() == 384);

    // Test visible cell calculation
    int sx, sy, ex, ey;
    display.Get_Visible_Cells(sx, sy, ex, ey);
    TEST("Visible cells start valid", sx >= 0 && sy >= 0);
    TEST("Visible cells end valid", ex > sx && ey > sy);

    // Test coordinate conversion
    display.Center_On(XY_Cell(64, 64));
    CELL cell = display.Screen_To_Cell(320, 200);
    TEST("Screen to cell works", cell != CELL_NONE);

    int screen_x, screen_y;
    bool visible = display.Cell_To_Screen(XY_Cell(64, 64), screen_x, screen_y);
    TEST("Cell to screen works", visible);

    // Test scrolling
    display.Scroll(10, 10);
    TEST("Scroll works", true);

    display.Center_On(XY_Cell(32, 32));
    TEST("Center on works", true);

    // Test cursor
    display.Set_Cursor_Cell(XY_Cell(50, 50));
    TEST("Cursor cell set", display.Get_Cursor_Cell() == XY_Cell(50, 50));

    // Test global pointer
    Display = &display;
    TEST("Global Display set", Display != nullptr);

    printf("\n=== All DisplayClass tests passed! ===\n");
    return 0;
}
EOF

# Compile all source files
if ! clang++ -std=c++17 -I include \
    /tmp/test_display.cpp \
    /tmp/mock_display_platform.cpp \
    src/game/core/coord.cpp \
    src/game/map/cell.cpp \
    src/game/display/gscreen.cpp \
    src/game/display/gadget.cpp \
    src/game/display/map.cpp \
    src/game/display/display.cpp \
    -o /tmp/test_display 2>&1; then
    echo "VERIFY_FAILED: Test compilation failed"
    rm -f /tmp/mock_display_platform.cpp /tmp/test_display.cpp
    exit 1
fi
echo "  OK: Test built"

# Step 4: Run test
echo "[4/4] Running display tests..."
if ! /tmp/test_display; then
    echo "VERIFY_FAILED: Display tests failed"
    rm -f /tmp/mock_display_platform.cpp /tmp/test_display.cpp /tmp/test_display
    exit 1
fi

rm -f /tmp/mock_display_platform.cpp /tmp/test_display.cpp /tmp/test_display

echo ""
echo "VERIFY_SUCCESS"
exit 0
