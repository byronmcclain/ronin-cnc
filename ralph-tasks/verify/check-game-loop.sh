#!/bin/bash
# Verification script for main game loop

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Main Game Loop ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check files exist
echo "[1/4] Checking files..."
for f in include/game/game.h src/game/game_loop.cpp src/game/init.cpp; do
    if [ ! -f "$f" ]; then
        echo "VERIFY_FAILED: $f not found"
        exit 1
    fi
    echo "  OK: $f"
done

# Step 2: Create mock platform for compilation
echo "[2/4] Creating mock platform..."
cat > /tmp/mock_game_platform.cpp << 'EOF'
#include "platform.h"
#include <cstring>
#include <cstdio>

static uint8_t mock_buffer[640 * 400];
static uint8_t mock_palette[768];
static bool graphics_initialized = false;
static uint32_t mock_ticks = 0;
static int frame_count = 0;

extern "C" {

enum PlatformResult Platform_Init(void) {
    graphics_initialized = true;
    memset(mock_buffer, 0, sizeof(mock_buffer));
    return PLATFORM_RESULT_SUCCESS;
}

enum PlatformResult Platform_Shutdown(void) {
    graphics_initialized = false;
    return PLATFORM_RESULT_SUCCESS;
}

bool Platform_IsInitialized(void) { return graphics_initialized; }
int32_t Platform_Graphics_Init(void) { return PLATFORM_RESULT_SUCCESS; }
void Platform_Graphics_Shutdown(void) {}
bool Platform_Graphics_IsInitialized(void) { return graphics_initialized; }
int32_t Platform_Graphics_GetBackBuffer(uint8_t **p, int32_t *w, int32_t *h, int32_t *s) {
    *p = mock_buffer; *w = 640; *h = 400; *s = 640;
    return PLATFORM_RESULT_SUCCESS;
}
void Platform_Graphics_LockBackBuffer(void) {}
void Platform_Graphics_UnlockBackBuffer(void) {}
int32_t Platform_Graphics_Flip(void) { return PLATFORM_RESULT_SUCCESS; }
void Platform_Graphics_SetPalette(const struct PaletteEntry *e, int32_t s, int32_t c) { (void)e;(void)s;(void)c; }
void Platform_Graphics_SetPaletteEntry(uint8_t i, uint8_t r, uint8_t g, uint8_t b) { (void)i;(void)r;(void)g;(void)b; }
void Platform_Input_Update(void) {}
void Platform_Mouse_GetPosition(int32_t *x, int32_t *y) { *x = 320; *y = 200; }
bool Platform_Mouse_IsPressed(enum MouseButton b) { (void)b; return false; }
bool Platform_Mouse_WasClicked(enum MouseButton b) { (void)b; return false; }
bool Platform_Key_WasPressed(enum KeyCode k) {
    if (frame_count == 3 && k == KEY_CODE_RETURN) return true;
    return false;
}
bool Platform_Key_IsPressed(enum KeyCode k) { (void)k; return false; }
uint32_t Platform_Timer_GetTicks(void) { return mock_ticks; }
void Platform_Frame_Begin(void) {
    frame_count++;
    mock_ticks += 17;
}
void Platform_Frame_End(void) {}
bool Platform_PollEvents(void) { return (frame_count >= 10); }
void Platform_LogInfo(const char *msg) { printf("%s\n", msg); }
void Platform_LogError(const char *msg) { fprintf(stderr, "%s\n", msg); }

} // extern "C"
EOF

# Step 3: Build test
echo "[3/4] Building game loop test..."
cat > /tmp/test_game_main.cpp << 'EOF'
#include "game/game.h"
#include <cstdio>

#define TEST(name, cond) do { \
    if (!(cond)) { printf("FAIL: %s\n", name); return 1; } \
    printf("PASS: %s\n", name); \
} while(0)

int main() {
    printf("=== Game Loop Test ===\n\n");

    // Initialize
    if (!Game_Init()) {
        printf("FAIL: Game_Init failed\n");
        return 1;
    }
    printf("PASS: Game initialized\n");

    // Check initial state
    TEST("Game instance exists", Game != nullptr);
    TEST("Initial mode is MENU", Game->Get_Mode() == GAME_MODE_MENU);
    TEST("Game is running", Game->Is_Running());
    TEST("Game not paused", !Game->Is_Paused());
    TEST("Display exists", Game->Get_Display() != nullptr);

    // Check timing
    TEST("Initial frame is 0", Game->Get_Frame() == 0);
    TEST("Initial tick is 0", Game->Get_Tick() == 0);
    TEST("Default speed is NORMAL", Game->Get_Speed() == GAME_SPEED_NORMAL);
    TEST("Tick interval is 66ms", Game->Get_Tick_Interval() == 66);

    // Run (will auto-quit after few frames via mock PollEvents)
    int result = Game->Run();
    printf("PASS: Game loop ran (exit code %d)\n", result);

    // Verify it ran some frames
    TEST("Some frames rendered", Game->Get_Frame() > 0);

    // Shutdown
    Game_Shutdown();
    TEST("Game shutdown", Game == nullptr);

    printf("\n=== All game loop tests passed! ===\n");
    return 0;
}
EOF

if ! clang++ -std=c++17 -I include \
    /tmp/test_game_main.cpp \
    /tmp/mock_game_platform.cpp \
    src/game/core/coord.cpp \
    src/game/map/cell.cpp \
    src/game/object/object.cpp \
    src/game/types/types.cpp \
    src/game/display/gscreen.cpp \
    src/game/display/gadget.cpp \
    src/game/display/map.cpp \
    src/game/display/display.cpp \
    src/game/game_loop.cpp \
    src/game/init.cpp \
    -o /tmp/test_game_loop 2>&1; then
    echo "VERIFY_FAILED: Compilation failed"
    rm -f /tmp/mock_game_platform.cpp /tmp/test_game_main.cpp
    exit 1
fi
echo "  OK: Test built"

# Step 4: Run test (directly, no timeout complexity)
echo "[4/4] Running game loop test..."
if ! /tmp/test_game_loop; then
    echo "VERIFY_FAILED: Game loop test failed"
    rm -f /tmp/mock_game_platform.cpp /tmp/test_game_main.cpp /tmp/test_game_loop
    exit 1
fi

rm -f /tmp/mock_game_platform.cpp /tmp/test_game_main.cpp /tmp/test_game_loop

echo ""
echo "VERIFY_SUCCESS"
exit 0
