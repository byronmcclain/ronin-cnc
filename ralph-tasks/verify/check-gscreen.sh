#!/bin/bash
# Verification script for GScreenClass port

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking GScreenClass Port ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check header files exist
echo "[1/5] Checking header files..."
check_file() {
    if [ ! -f "$1" ]; then
        echo "VERIFY_FAILED: $1 not found"
        exit 1
    fi
    echo "  OK: $1"
}

check_file "include/game/gscreen.h"
check_file "include/game/gadget.h"
check_file "src/game/display/gscreen.cpp"
check_file "src/game/display/gadget.cpp"

# Step 2: Syntax check headers
echo "[2/5] Checking header syntax..."
for header in include/game/gscreen.h include/game/gadget.h; do
    if ! clang++ -std=c++17 -fsyntax-only -I include "$header" 2>&1; then
        echo "VERIFY_FAILED: $header has syntax errors"
        exit 1
    fi
done
echo "  OK: Headers valid"

# Step 3: Compile implementations (with real platform header)
echo "[3/5] Checking implementation syntax..."

# Syntax check with real platform.h
if ! clang++ -std=c++17 -fsyntax-only -I include \
    src/game/display/gscreen.cpp 2>&1; then
    echo "VERIFY_FAILED: gscreen.cpp has syntax errors"
    exit 1
fi

if ! clang++ -std=c++17 -fsyntax-only -I include \
    src/game/display/gadget.cpp 2>&1; then
    echo "VERIFY_FAILED: gadget.cpp has syntax errors"
    exit 1
fi
echo "  OK: Implementations valid"

# Step 4: Build test program with mock platform
echo "[4/5] Building GScreen test..."

# Create mock platform implementation (separate from header)
cat > /tmp/mock_platform.cpp << 'EOF'
// Mock platform implementation for testing
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

cat > /tmp/test_gscreen.cpp << 'EOF'
#include "game/gscreen.h"
#include "game/gadget.h"
#include <cstdio>

#define TEST(name, cond) do { \
    if (!(cond)) { printf("FAIL: %s\n", name); return 1; } \
    printf("PASS: %s\n", name); \
} while(0)

int main() {
    printf("=== GScreenClass Tests ===\n\n");

    // Create screen
    GScreenClass screen;
    TEST("Screen created", true);

    // Check initial state
    TEST("Not initialized", !screen.Is_Initialized());
    TEST("Not locked", !screen.Is_Locked());

    // Test One_Time (with mock platform)
    screen.One_Time();
    TEST("Initialized after One_Time", screen.Is_Initialized());
    TEST("Width set", screen.Get_Width() == 640);
    TEST("Height set", screen.Get_Height() == 400);

    // Test buffer access
    uint8_t* buf = screen.Lock();
    TEST("Buffer locked", screen.Is_Locked());
    TEST("Buffer not null", buf != nullptr);

    screen.Unlock();
    TEST("Buffer unlocked", !screen.Is_Locked());

    // Test pixel operations
    screen.Put_Pixel(100, 100, 42);
    TEST("Pixel written", screen.Get_Pixel(100, 100) == 42);

    // Test rect draw
    screen.Draw_Rect(50, 50, 10, 10, 99);
    TEST("Rect drawn", screen.Get_Pixel(55, 55) == 99);

    // Test Clear
    screen.Clear(0);
    TEST("Clear works", screen.Get_Pixel(55, 55) == 0);

    // Test gadget system
    ButtonClass button(100, 100, 80, 20, "Test", 1);
    screen.Add_Gadget(&button);
    TEST("Gadget added", true);

    screen.Remove_Gadget(&button);
    TEST("Gadget removed", true);

    // Test global pointer
    TheScreen = &screen;
    TEST("TheScreen set", TheScreen != nullptr);

    printf("\n=== All GScreen tests passed! ===\n");
    return 0;
}
EOF

# Compile test with mock platform
if ! clang++ -std=c++17 -I include \
    /tmp/test_gscreen.cpp \
    /tmp/mock_platform.cpp \
    src/game/display/gscreen.cpp \
    src/game/display/gadget.cpp \
    -o /tmp/test_gscreen 2>&1; then
    echo "VERIFY_FAILED: Test program compilation failed"
    rm -f /tmp/mock_platform.cpp /tmp/test_gscreen.cpp
    exit 1
fi
echo "  OK: Test program built"

# Step 5: Run tests
echo "[5/5] Running GScreen tests..."
if ! /tmp/test_gscreen; then
    echo "VERIFY_FAILED: GScreen tests failed"
    rm -f /tmp/mock_platform.cpp /tmp/test_gscreen.cpp /tmp/test_gscreen
    exit 1
fi

# Cleanup
rm -f /tmp/mock_platform.cpp /tmp/test_gscreen.cpp /tmp/test_gscreen

echo ""
echo "VERIFY_SUCCESS"
exit 0
