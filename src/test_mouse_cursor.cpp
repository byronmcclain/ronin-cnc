/**
 * Mouse Cursor Test Program
 */

#include "game/graphics/mouse_cursor.h"
#include "game/graphics/graphics_buffer.h"
#include "platform.h"
#include <cstdio>
#include <cstring>

// =============================================================================
// Test Utilities
// =============================================================================

static int tests_run = 0;
static int tests_passed = 0;

#define TEST_START(name) do { \
    printf("  Testing %s... ", name); \
    tests_run++; \
} while(0)

#define TEST_PASS() do { \
    printf("PASS\n"); \
    tests_passed++; \
} while(0)

#define TEST_FAIL(msg) do { \
    printf("FAIL: %s\n", msg); \
    return false; \
} while(0)

#define ASSERT(cond, msg) do { \
    if (!(cond)) TEST_FAIL(msg); \
} while(0)

// =============================================================================
// Unit Tests
// =============================================================================

bool test_singleton() {
    TEST_START("singleton pattern");

    MouseCursor& mc1 = MouseCursor::Instance();
    MouseCursor& mc2 = MouseCursor::Instance();

    ASSERT(&mc1 == &mc2, "Should return same instance");

    TEST_PASS();
    return true;
}

bool test_cursor_types() {
    TEST_START("cursor type setting");

    MouseCursor& mc = MouseCursor::Instance();

    mc.SetType(CURSOR_MOVE);
    ASSERT(mc.GetType() == CURSOR_MOVE, "Type should be CURSOR_MOVE");

    mc.SetType(CURSOR_ATTACK);
    ASSERT(mc.GetType() == CURSOR_ATTACK, "Type should be CURSOR_ATTACK");

    mc.Reset();
    ASSERT(mc.GetType() == CURSOR_NORMAL, "Reset should set CURSOR_NORMAL");

    TEST_PASS();
    return true;
}

bool test_visibility() {
    TEST_START("visibility control");

    MouseCursor& mc = MouseCursor::Instance();

    mc.Show();
    ASSERT(mc.IsVisible(), "Should be visible after Show()");
    ASSERT(!mc.IsHidden(), "Should not be hidden after Show()");

    mc.Hide();
    ASSERT(!mc.IsVisible(), "Should not be visible after Hide()");
    ASSERT(mc.IsHidden(), "Should be hidden after Hide()");

    mc.Show();
    ASSERT(mc.IsVisible(), "Should be visible again");

    TEST_PASS();
    return true;
}

bool test_locking() {
    TEST_START("type locking");

    MouseCursor& mc = MouseCursor::Instance();

    mc.Unlock();
    mc.SetType(CURSOR_NORMAL);

    mc.Lock();
    ASSERT(mc.IsLocked(), "Should be locked");

    mc.SetType(CURSOR_ATTACK);
    ASSERT(mc.GetType() == CURSOR_NORMAL, "Locked cursor should not change");

    mc.Unlock();
    ASSERT(!mc.IsLocked(), "Should be unlocked");

    mc.SetType(CURSOR_ATTACK);
    ASSERT(mc.GetType() == CURSOR_ATTACK, "Unlocked cursor should change");

    mc.Reset();

    TEST_PASS();
    return true;
}

bool test_animation() {
    TEST_START("animation");

    MouseCursor& mc = MouseCursor::Instance();

    // Non-animated cursor
    mc.SetType(CURSOR_NORMAL);
    ASSERT(!mc.IsAnimated(), "CURSOR_NORMAL should not animate");
    ASSERT(mc.GetAnimationFrameCount() == 1, "Non-animated should have 1 frame");

    // Animated cursor
    mc.SetType(CURSOR_ATTACK);
    ASSERT(mc.IsAnimated(), "CURSOR_ATTACK should animate");
    ASSERT(mc.GetAnimationFrameCount() == 4, "Attack cursor should have 4 frames");

    mc.SetType(CURSOR_SELECT);
    ASSERT(mc.IsAnimated(), "CURSOR_SELECT should animate");

    mc.Reset();

    TEST_PASS();
    return true;
}

bool test_hotspots() {
    TEST_START("hotspot data");

    MouseCursor& mc = MouseCursor::Instance();

    mc.SetType(CURSOR_NORMAL);
    const CursorHotspot& normal_hs = mc.GetHotspot();
    ASSERT(normal_hs.x == 0 && normal_hs.y == 0, "Normal cursor hotspot should be (0,0)");

    mc.SetType(CURSOR_SELECT);
    const CursorHotspot& select_hs = mc.GetHotspot();
    ASSERT(select_hs.x == 15 && select_hs.y == 15, "Select cursor hotspot should be (15,15)");

    mc.Reset();

    TEST_PASS();
    return true;
}

bool test_scroll_cursor() {
    TEST_START("scroll cursor selection");

    MouseCursor& mc = MouseCursor::Instance();

    mc.SetScrollCursor(0, -1);  // North
    ASSERT(mc.GetType() == CURSOR_SCROLL_N, "Should be N scroll cursor");

    mc.SetScrollCursor(1, -1);  // Northeast
    ASSERT(mc.GetType() == CURSOR_SCROLL_NE, "Should be NE scroll cursor");

    mc.SetScrollCursor(1, 0);   // East
    ASSERT(mc.GetType() == CURSOR_SCROLL_E, "Should be E scroll cursor");

    mc.SetScrollCursor(-1, 1);  // Southwest
    ASSERT(mc.GetType() == CURSOR_SCROLL_SW, "Should be SW scroll cursor");

    mc.SetScrollCursor(0, 0);   // No scroll
    ASSERT(mc.GetType() == CURSOR_NORMAL, "Should be normal cursor");

    TEST_PASS();
    return true;
}

bool test_scroll_edge() {
    TEST_START("scroll edge detection");

    MouseCursor& mc = MouseCursor::Instance();

    // Test top-left corner
    bool in_edge = mc.CheckScrollEdge(5, 5, 640, 400, 16);
    ASSERT(in_edge, "Should detect top-left edge");
    ASSERT(mc.GetType() == CURSOR_SCROLL_NW, "Should be NW scroll cursor");

    // Test center
    in_edge = mc.CheckScrollEdge(320, 200, 640, 400, 16);
    ASSERT(!in_edge, "Should not detect center as edge");

    // Test right edge
    in_edge = mc.CheckScrollEdge(630, 200, 640, 400, 16);
    ASSERT(in_edge, "Should detect right edge");
    ASSERT(mc.GetType() == CURSOR_SCROLL_E, "Should be E scroll cursor");

    mc.Reset();

    TEST_PASS();
    return true;
}

// =============================================================================
// Visual Test
// =============================================================================

void run_visual_test() {
    printf("\n=== Visual Mouse Cursor Test ===\n");

    MouseCursor& mc = MouseCursor::Instance();
    GraphicsBuffer& screen = GraphicsBuffer::Screen();

    // Set up simple palette
    PaletteEntry entries[256];
    for (int i = 0; i < 256; i++) {
        entries[i].r = static_cast<uint8_t>(i);
        entries[i].g = static_cast<uint8_t>(i);
        entries[i].b = static_cast<uint8_t>(i);
    }
    Platform_Graphics_SetPalette(entries, 0, 256);

    // Try to load cursor (may fail without MIX files)
    bool cursor_loaded = mc.Load("MOUSE.SHP");

    if (!cursor_loaded) {
        printf("Could not load MOUSE.SHP - drawing placeholder cursor\n");
    }

    // Test cursor types
    CursorType test_types[] = {
        CURSOR_NORMAL,
        CURSOR_MOVE,
        CURSOR_ATTACK,
        CURSOR_SELECT,
        CURSOR_SELL,
        CURSOR_REPAIR,
        CURSOR_NUKE,
    };

    int type_count = static_cast<int>(sizeof(test_types) / sizeof(test_types[0]));
    int current_type_index = 0;
    int frame_count = 0;

    printf("Move mouse around. Cursor type cycles every 60 frames.\n");
    printf("Press ESC or close window to exit.\n");

    mc.Show();

    while (!Platform_Input_ShouldQuit()) {
        Platform_Input_Update();

        // Cycle cursor type
        frame_count++;
        if (frame_count >= 60) {
            frame_count = 0;
            current_type_index = (current_type_index + 1) % type_count;
            mc.SetType(test_types[current_type_index]);
        }

        // Update cursor animation
        mc.Update();

        // Draw
        screen.Lock();
        screen.Clear(32);  // Dark gray background

        // Draw grid
        for (int y = 0; y < screen.Get_Height(); y += 50) {
            screen.Draw_HLine(0, y, screen.Get_Width(), 64);
        }
        for (int x = 0; x < screen.Get_Width(); x += 50) {
            screen.Draw_VLine(x, 0, screen.Get_Height(), 64);
        }

        // Draw cursor info
        int mx, my;
        mc.GetPosition(&mx, &my);

        // Draw a marker at click position
        int cx, cy;
        mc.GetClickPosition(&cx, &cy);
        screen.Fill_Rect(cx - 2, cy - 2, 5, 5, 200);

        // Draw cursor (if loaded) or placeholder
        if (cursor_loaded) {
            mc.Draw(screen);
        } else {
            // Draw simple crosshair as placeholder
            screen.Draw_HLine(mx - 10, my, 21, 255);
            screen.Draw_VLine(mx, my - 10, 21, 255);
        }

        screen.Unlock();
        screen.Flip();

        Platform_Delay(16);  // ~60 fps
    }

    mc.Hide();
    printf("Visual test complete.\n");
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[]) {
    printf("==========================================\n");
    printf("Mouse Cursor Test Suite\n");
    printf("==========================================\n\n");

    // Check for quick mode
    bool quick_mode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--quick") == 0) {
            quick_mode = true;
        }
    }

    if (Platform_Init() != PLATFORM_RESULT_SUCCESS) {
        printf("ERROR: Failed to initialize platform\n");
        return 1;
    }

    if (Platform_Graphics_Init() != 0) {
        printf("ERROR: Failed to initialize graphics\n");
        Platform_Shutdown();
        return 1;
    }

    Platform_Input_Init();

    printf("=== Unit Tests ===\n\n");

    test_singleton();
    test_cursor_types();
    test_visibility();
    test_locking();
    test_animation();
    test_hotspots();
    test_scroll_cursor();
    test_scroll_edge();

    printf("\n------------------------------------------\n");
    printf("Tests: %d/%d passed\n", tests_passed, tests_run);
    printf("------------------------------------------\n");

    if (tests_passed == tests_run && !quick_mode) {
        run_visual_test();
    }

    Platform_Input_Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();

    printf("\n==========================================\n");
    if (tests_passed == tests_run) {
        printf("All tests PASSED!\n");
    } else {
        printf("Some tests FAILED\n");
    }
    printf("==========================================\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
