#!/bin/bash
# Verification script for MapClass foundation

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking MapClass Foundation ==="
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

check_file "include/game/map.h"
check_file "include/game/cell.h"
check_file "src/game/display/map.cpp"
check_file "src/game/display/cell.cpp"

# Step 2: Syntax check
echo "[2/4] Checking syntax..."
if ! clang++ -std=c++17 -fsyntax-only -I include include/game/map.h 2>&1; then
    echo "VERIFY_FAILED: map.h has syntax errors"
    exit 1
fi
if ! clang++ -std=c++17 -fsyntax-only -I include include/game/cell.h 2>&1; then
    echo "VERIFY_FAILED: cell.h has syntax errors"
    exit 1
fi
echo "  OK: Header syntax valid"

# Step 3: Build test with mock platform
echo "[3/4] Building map test..."

# Create mock platform implementation
cat > /tmp/mock_platform_map.cpp << 'EOF'
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

# Create test program
cat > /tmp/test_map.cpp << 'EOF'
#include "game/map.h"
#include "game/cell.h"
#include <cstdio>

#define TEST(name, cond) do { \
    if (!(cond)) { printf("FAIL: %s\n", name); return 1; } \
    printf("PASS: %s\n", name); \
} while(0)

int main() {
    printf("=== MapClass Tests ===\n\n");

    MapClass map;
    TEST("Map created", true);

    // Initialize
    map.One_Time();
    TEST("One_Time called", map.Is_Initialized());

    map.Init(THEATER_TEMPERATE);
    TEST("Init called", map.Get_Theater_Type() == THEATER_TEMPERATE);

    // Test cell access
    CELL center = XY_Cell(64, 64);
    CellClass& cell = map[center];
    TEST("Cell access works", true);  // If we got here without crash, it works

    // Test by coordinates
    CellClass* ptr = map.Cell_At(64, 64);
    TEST("Cell_At works", ptr != nullptr);

    // Test validation
    TEST("Valid cell is valid", map.Is_Valid_Cell(XY_Cell(64, 64)));
    TEST("Invalid cell is invalid", !map.Is_Valid_Cell(CELL_NONE));

    // Test bounds
    TEST("In bounds", map.Is_In_Bounds(64, 64));
    TEST("Out of bounds - edge", !map.Is_In_Bounds(0, 0));

    // Test bounds setting
    map.Set_Map_Bounds(10, 10, 50, 50);
    TEST("Bounds X", map.Map_Bounds_X() == 10);
    TEST("Bounds Y", map.Map_Bounds_Y() == 10);
    TEST("Bounds W", map.Map_Bounds_Width() == 50);
    TEST("Bounds H", map.Map_Bounds_Height() == 50);

    // Test clamping
    int x = 5, y = 5;
    map.Clamp_To_Bounds(x, y);
    TEST("Clamp X", x == 10);
    TEST("Clamp Y", y == 10);

    // Test coordinate conversion
    COORDINATE coord = ::Cell_Coord(XY_Cell(32, 32));
    CellClass* coord_cell = map.Get_Cell_At_Coord(coord);
    TEST("Get_Cell_At_Coord works", coord_cell != nullptr);

    // Test global pointer
    Map = &map;
    TEST("Global Map set", Map != nullptr);

    printf("\n=== All MapClass tests passed! ===\n");
    return 0;
}
EOF

# Compile
if ! clang++ -std=c++17 -I include \
    /tmp/test_map.cpp \
    /tmp/mock_platform_map.cpp \
    src/game/core/coord.cpp \
    src/game/display/gscreen.cpp \
    src/game/display/gadget.cpp \
    src/game/display/map.cpp \
    src/game/map/cell.cpp \
    -o /tmp/test_map 2>&1; then
    echo "VERIFY_FAILED: Test compilation failed"
    rm -f /tmp/mock_platform_map.cpp /tmp/test_map.cpp
    exit 1
fi
echo "  OK: Test built"

# Step 4: Run test
echo "[4/4] Running map tests..."
if ! /tmp/test_map; then
    echo "VERIFY_FAILED: Map tests failed"
    rm -f /tmp/mock_platform_map.cpp /tmp/test_map.cpp /tmp/test_map
    exit 1
fi

rm -f /tmp/mock_platform_map.cpp /tmp/test_map.cpp /tmp/test_map

echo ""
echo "VERIFY_SUCCESS"
exit 0
