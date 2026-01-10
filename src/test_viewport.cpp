/**
 * Viewport Test Program
 */

#include "game/viewport.h"
#include "game/scroll_manager.h"
#include "game/graphics/graphics_buffer.h"
#include "platform.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

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

    GameViewport& vp1 = GameViewport::Instance();
    GameViewport& vp2 = GameViewport::Instance();

    ASSERT(&vp1 == &vp2, "Should return same instance");
    ASSERT(&vp1 == &TheViewport, "Global reference should match");

    TEST_PASS();
    return true;
}

bool test_initialization() {
    TEST_START("initialization");

    GameViewport& vp = GameViewport::Instance();
    vp.Initialize();

    ASSERT(vp.x == 0, "X should be 0 after init");
    ASSERT(vp.y == 0, "Y should be 0 after init");
    ASSERT(vp.width == TACTICAL_WIDTH, "Width should be TACTICAL_WIDTH");
    ASSERT(vp.height == TACTICAL_HEIGHT, "Height should be TACTICAL_HEIGHT");
    ASSERT(vp.IsScrollEnabled(), "Scroll should be enabled");

    TEST_PASS();
    return true;
}

bool test_coordinate_conversion() {
    TEST_START("coordinate conversion");

    GameViewport& vp = GameViewport::Instance();
    vp.Initialize();
    vp.SetMapSize(64, 64);
    vp.ScrollTo(100, 50);

    // Test WorldToScreen
    int screen_x, screen_y;
    vp.WorldToScreen(200, 100, screen_x, screen_y);
    // Expected: screen_x = 200 - 100 = 100
    //           screen_y = 100 - 50 + TAB_HEIGHT = 66
    ASSERT(screen_x == 100, "WorldToScreen X incorrect");
    ASSERT(screen_y == 66, "WorldToScreen Y incorrect");

    // Test ScreenToWorld
    int world_x, world_y;
    vp.ScreenToWorld(100, 66, world_x, world_y);
    ASSERT(world_x == 200, "ScreenToWorld X incorrect");
    ASSERT(world_y == 100, "ScreenToWorld Y incorrect");

    // Test cell conversion
    int cell_x, cell_y;
    vp.WorldToCell(60, 48, cell_x, cell_y);
    ASSERT(cell_x == 2, "WorldToCell X incorrect");
    ASSERT(cell_y == 2, "WorldToCell Y incorrect");

    TEST_PASS();
    return true;
}

bool test_lepton_conversion() {
    TEST_START("lepton conversion");

    GameViewport& vp = GameViewport::Instance();

    // 256 leptons = 24 pixels
    int pixel_x, pixel_y;
    vp.LeptonToPixel(256, 512, pixel_x, pixel_y);
    ASSERT(pixel_x == 24, "LeptonToPixel X incorrect");
    ASSERT(pixel_y == 48, "LeptonToPixel Y incorrect");

    int lepton_x, lepton_y;
    vp.PixelToLepton(24, 48, lepton_x, lepton_y);
    ASSERT(lepton_x == 256, "PixelToLepton X incorrect");
    ASSERT(lepton_y == 512, "PixelToLepton Y incorrect");

    TEST_PASS();
    return true;
}

bool test_bounds_clamping() {
    TEST_START("bounds clamping");

    GameViewport& vp = GameViewport::Instance();
    vp.Initialize();
    vp.SetMapSize(32, 32);  // 768x768 pixel map

    // Try to scroll past left edge
    vp.ScrollTo(-100, 0);
    ASSERT(vp.x == 0, "Left edge not clamped");

    // Try to scroll past top edge
    vp.ScrollTo(0, -100);
    ASSERT(vp.y == 0, "Top edge not clamped");

    // Try to scroll past right edge (map is 768, viewport is 480)
    vp.ScrollTo(500, 0);
    int max_x = vp.GetMapPixelWidth() - vp.width;  // 768 - 480 = 288
    ASSERT(vp.x == max_x, "Right edge not clamped");

    // Try to scroll past bottom edge (map is 768, viewport is 384)
    vp.ScrollTo(0, 500);
    int max_y = vp.GetMapPixelHeight() - vp.height;  // 768 - 384 = 384
    ASSERT(vp.y == max_y, "Bottom edge not clamped");

    TEST_PASS();
    return true;
}

bool test_visibility() {
    TEST_START("visibility testing");

    GameViewport& vp = GameViewport::Instance();
    vp.Initialize();
    vp.SetMapSize(64, 64);
    vp.ScrollTo(100, 100);

    // Point inside viewport
    ASSERT(vp.IsPointVisible(200, 200), "Point should be visible");

    // Point outside viewport
    ASSERT(!vp.IsPointVisible(50, 50), "Point should not be visible");

    // Rectangle partially visible
    ASSERT(vp.IsRectVisible(50, 50, 100, 100), "Partial rect should be visible");

    // Cell visibility
    vp.ScrollTo(0, 0);
    ASSERT(vp.IsCellVisible(5, 5), "Cell (5,5) should be visible at origin");

    TEST_PASS();
    return true;
}

bool test_visible_cell_range() {
    TEST_START("visible cell range");

    GameViewport& vp = GameViewport::Instance();
    vp.Initialize();
    vp.SetMapSize(64, 64);
    vp.ScrollTo(48, 24);  // 2 cells right, 1 cell down

    int start_x, start_y, end_x, end_y;
    vp.GetVisibleCellRange(start_x, start_y, end_x, end_y);

    ASSERT(start_x == 2, "Start X should be 2");
    ASSERT(start_y == 1, "Start Y should be 1");

    TEST_PASS();
    return true;
}

bool test_scroll_animation() {
    TEST_START("scroll animation");

    GameViewport& vp = GameViewport::Instance();
    vp.Initialize();
    vp.SetMapSize(64, 64);
    vp.ScrollTo(0, 0);

    ScrollManager& sm = ScrollManager::Instance();

    // Start linear scroll animation
    sm.ScrollTo(200, 100, ScrollAnimationType::LINEAR, 10);

    ASSERT(sm.IsAnimating(), "Animation should be active");

    // Simulate 10 frames
    for (int i = 0; i < 10; i++) {
        sm.Update();
    }

    ASSERT(!sm.IsAnimating(), "Animation should be complete");
    ASSERT(vp.x == 200, "Final X should be 200");
    ASSERT(vp.y == 100, "Final Y should be 100");

    TEST_PASS();
    return true;
}

bool test_center_on() {
    TEST_START("center on");

    GameViewport& vp = GameViewport::Instance();
    vp.Initialize();
    vp.SetMapSize(64, 64);

    // Center on a point
    vp.CenterOn(500, 400);

    int center_x = vp.x + vp.width / 2;
    int center_y = vp.y + vp.height / 2;

    ASSERT(center_x == 500, "Center X should be 500");
    ASSERT(center_y == 400, "Center Y should be 400");

    // Center on cell (10, 10) -> pixel (240, 240) + half tile = (252, 252)
    vp.CenterOnCell(10, 10);
    center_x = vp.x + vp.width / 2;
    center_y = vp.y + vp.height / 2;

    ASSERT(abs(center_x - 252) <= 1, "Cell center X should be ~252");
    ASSERT(abs(center_y - 252) <= 1, "Cell center Y should be ~252");

    TEST_PASS();
    return true;
}

bool test_coord_macros() {
    TEST_START("COORDINATE macros");

    COORDINATE coord = MAKE_COORD(0x1234, 0x5678);

    ASSERT(COORD_X(coord) == 0x1234, "COORD_X should be 0x1234");
    ASSERT(COORD_Y(coord) == 0x5678, "COORD_Y should be 0x5678");

    CELL cell = MAKE_CELL(50, 30);
    ASSERT(CELL_X(cell) == 50, "CELL_X should be 50");
    ASSERT(CELL_Y(cell) == 30, "CELL_Y should be 30");

    TEST_PASS();
    return true;
}

bool test_edge_scroll() {
    TEST_START("edge scroll detection");

    GameViewport& vp = GameViewport::Instance();
    vp.Initialize();
    vp.SetMapSize(128, 128);
    vp.ScrollTo(500, 500);

    // Save original position
    int orig_x = vp.x;
    int orig_y = vp.y;

    // Mouse at left edge (should scroll left)
    vp.UpdateEdgeScroll(5, 100);
    ASSERT(vp.GetCurrentScrollDirection() & SCROLL_LEFT, "Should detect left scroll");
    ASSERT(vp.x < orig_x, "Should have scrolled left");

    // Mouse at top edge
    orig_y = vp.y;
    vp.UpdateEdgeScroll(200, VP_TAB_HEIGHT + 5);  // Adjust for tab height
    ASSERT(vp.GetCurrentScrollDirection() & SCROLL_UP, "Should detect up scroll");
    ASSERT(vp.y < orig_y, "Should have scrolled up");

    // Mouse in center (no scroll)
    vp.UpdateEdgeScroll(200, 200);
    ASSERT(vp.GetCurrentScrollDirection() == SCROLL_NONE, "Should not scroll in center");

    TEST_PASS();
    return true;
}

bool test_keyboard_scroll() {
    TEST_START("keyboard scroll");

    GameViewport& vp = GameViewport::Instance();
    vp.Initialize();
    vp.SetMapSize(128, 128);
    vp.ScrollTo(500, 500);

    int orig_x = vp.x;
    int orig_y = vp.y;

    // Scroll right
    vp.UpdateKeyboardScroll(false, false, false, true);
    ASSERT(vp.x > orig_x, "Should have scrolled right");

    // Scroll up
    orig_y = vp.y;
    vp.UpdateKeyboardScroll(true, false, false, false);
    ASSERT(vp.y < orig_y, "Should have scrolled up");

    TEST_PASS();
    return true;
}

bool test_tracking() {
    TEST_START("target tracking");

    GameViewport& vp = GameViewport::Instance();
    vp.Initialize();
    vp.SetMapSize(128, 128);
    vp.ScrollTo(0, 0);

    ASSERT(!vp.HasTrackTarget(), "Should not have track target initially");

    // Set track target far from current view
    // COORDINATE uses leptons: 256 leptons = 24 pixels
    // To get pixel position 1000, we need lepton position 1000 * 256 / 24 = ~10666
    // Using a target well beyond the dead zone (100 pixels from center)
    // Center of viewport at (0,0) is at pixel (240, 192)
    // We want a target at pixel (500, 400) which is lepton ~(5333, 4266)
    COORDINATE target = MAKE_COORD(5333, 4266);  // Converts to ~(500, 400) pixels
    vp.SetTrackTarget(target);

    ASSERT(vp.HasTrackTarget(), "Should have track target");

    // Update tracking multiple times to ensure movement
    int orig_x = vp.x;
    int orig_y = vp.y;
    for (int i = 0; i < 10; i++) {
        vp.UpdateTracking();
    }
    ASSERT(vp.x > orig_x || vp.y > orig_y, "Tracking should move viewport");

    vp.ClearTrackTarget();
    ASSERT(!vp.HasTrackTarget(), "Should not have track target after clear");

    TEST_PASS();
    return true;
}

bool test_scroll_manager_singleton() {
    TEST_START("scroll manager singleton");

    ScrollManager& sm1 = ScrollManager::Instance();
    ScrollManager& sm2 = ScrollManager::Instance();

    ASSERT(&sm1 == &sm2, "Should return same instance");

    TEST_PASS();
    return true;
}

bool test_scroll_easing() {
    TEST_START("scroll easing");

    GameViewport& vp = GameViewport::Instance();
    vp.Initialize();
    vp.SetMapSize(64, 64);

    ScrollManager& sm = ScrollManager::Instance();

    // Test ease-out (should move quickly at first)
    vp.ScrollTo(0, 0);
    sm.ScrollTo(240, 0, ScrollAnimationType::EASE_OUT, 10);

    sm.Update();  // Frame 1
    int pos_1 = vp.x;
    sm.Update();  // Frame 2
    int pos_2 = vp.x;

    // With ease-out, early frames should have larger deltas
    // (This is a rough check - ease-out moves faster at start)
    ASSERT(pos_1 > 0, "Should have moved after first frame");

    // Complete animation
    for (int i = 0; i < 8; i++) {
        sm.Update();
    }
    ASSERT(!sm.IsAnimating(), "Animation should complete");

    TEST_PASS();
    return true;
}

// =============================================================================
// Visual Test
// =============================================================================

void run_visual_test() {
    printf("\n=== Visual Viewport Test ===\n");
    printf("Arrow keys: Scroll\n");
    printf("H: Jump to center\n");
    printf("Press ESC or close window to exit.\n\n");

    GameViewport& vp = GameViewport::Instance();
    ScrollManager& sm = ScrollManager::Instance();
    GraphicsBuffer& screen = GraphicsBuffer::Screen();

    // Set up palette
    PaletteEntry entries[256];
    for (int i = 0; i < 256; i++) {
        entries[i].r = static_cast<uint8_t>(i);
        entries[i].g = static_cast<uint8_t>(i);
        entries[i].b = static_cast<uint8_t>(i);
    }
    // Colors for grid
    entries[21].r = 40; entries[21].g = 100; entries[21].b = 40;   // Green 1
    entries[22].r = 30; entries[22].g = 80; entries[22].b = 30;    // Green 2
    entries[12].r = 200; entries[12].g = 50; entries[12].b = 50;   // Red (edges)
    entries[179].r = 255; entries[179].g = 255; entries[179].b = 0; // Yellow (radar)
    entries[15].r = 255; entries[15].g = 255; entries[15].b = 255; // White
    Platform_Graphics_SetPalette(entries, 0, 256);

    vp.Initialize();
    vp.SetMapSize(128, 128);

    while (!Platform_Input_ShouldQuit()) {
        Platform_Input_Update();

        // Keyboard scrolling
        bool up = Platform_Key_IsPressed(KEY_CODE_UP);
        bool down = Platform_Key_IsPressed(KEY_CODE_DOWN);
        bool left = Platform_Key_IsPressed(KEY_CODE_LEFT);
        bool right = Platform_Key_IsPressed(KEY_CODE_RIGHT);

        if (up || down || left || right) {
            sm.CancelScroll();
            vp.UpdateKeyboardScroll(up, down, left, right);
        }

        // H key: jump to center
        if (Platform_Key_WasPressed(KEY_CODE_HOME)) {
            int center_x = (vp.GetMapCellWidth() / 2) * TILE_PIXEL_WIDTH;
            int center_y = (vp.GetMapCellHeight() / 2) * TILE_PIXEL_HEIGHT;
            sm.CenterOn(center_x, center_y, ScrollAnimationType::EASE_OUT, 20);
        }

        // Update animations
        sm.Update();

        // Get mouse for edge scrolling (when not using keyboard)
        if (!up && !down && !left && !right && !sm.IsAnimating()) {
            int mx, my;
            Platform_Mouse_GetPosition(&mx, &my);
            mx /= 2;  // Scale for 2x window
            my /= 2;
            vp.UpdateEdgeScroll(mx, my);
        }

        // Render
        screen.Lock();
        screen.Clear(0);

        // Draw checkerboard map
        int start_x, start_y, end_x, end_y;
        vp.GetVisibleCellRange(start_x, start_y, end_x, end_y);

        for (int cy = start_y; cy < end_y; cy++) {
            for (int cx = start_x; cx < end_x; cx++) {
                int screen_x, screen_y;
                vp.CellToScreen(cx, cy, screen_x, screen_y);

                // Alternating colors
                uint8_t color = ((cx + cy) % 2) ? 21 : 22;

                // Highlight edge cells
                if (cx == 0 || cy == 0 ||
                    cx == vp.GetMapCellWidth() - 1 ||
                    cy == vp.GetMapCellHeight() - 1) {
                    color = 12;
                }

                screen.Fill_Rect(screen_x, screen_y, TILE_PIXEL_WIDTH, TILE_PIXEL_HEIGHT, color);
            }
        }

        // Draw sidebar placeholder
        screen.Fill_Rect(TACTICAL_WIDTH, 0, VP_SIDEBAR_WIDTH, VP_SCREEN_HEIGHT, 0);
        screen.Draw_VLine(TACTICAL_WIDTH, 0, VP_SCREEN_HEIGHT, 15);

        // Draw tab bar placeholder
        screen.Fill_Rect(0, 0, TACTICAL_WIDTH, VP_TAB_HEIGHT, 15);

        // Draw simple radar
        int radar_x = 496;
        int radar_y = 232;
        int radar_size = 128;

        // Radar background
        screen.Fill_Rect(radar_x, radar_y, radar_size, radar_size, 8);

        // Radar border
        screen.Draw_HLine(radar_x, radar_y, radar_size, 15);
        screen.Draw_HLine(radar_x, radar_y + radar_size - 1, radar_size, 15);
        screen.Draw_VLine(radar_x, radar_y, radar_size, 15);
        screen.Draw_VLine(radar_x + radar_size - 1, radar_y, radar_size, 15);

        // Viewport position on radar
        float scale_x = static_cast<float>(radar_size) / static_cast<float>(vp.GetMapPixelWidth());
        float scale_y = static_cast<float>(radar_size) / static_cast<float>(vp.GetMapPixelHeight());
        int box_x = radar_x + static_cast<int>(static_cast<float>(vp.x) * scale_x);
        int box_y = radar_y + static_cast<int>(static_cast<float>(vp.y) * scale_y);
        int box_w = static_cast<int>(static_cast<float>(vp.width) * scale_x);
        int box_h = static_cast<int>(static_cast<float>(vp.height) * scale_y);

        // Draw viewport rectangle
        screen.Draw_HLine(box_x, box_y, box_w, 179);
        screen.Draw_HLine(box_x, box_y + box_h - 1, box_w, 179);
        screen.Draw_VLine(box_x, box_y, box_h, 179);
        screen.Draw_VLine(box_x + box_w - 1, box_y, box_h, 179);

        // Mouse cursor
        int mx, my;
        Platform_Mouse_GetPosition(&mx, &my);
        mx /= 2;
        my /= 2;
        screen.Fill_Rect(mx - 2, my - 2, 5, 5, 15);

        // Check for radar click
        if (Platform_Mouse_IsPressed(MOUSE_BUTTON_LEFT)) {
            if (mx >= radar_x && mx < radar_x + radar_size &&
                my >= radar_y && my < radar_y + radar_size) {
                int click_x = mx - radar_x;
                int click_y = my - radar_y;
                int world_x = static_cast<int>(static_cast<float>(click_x) / scale_x);
                int world_y = static_cast<int>(static_cast<float>(click_y) / scale_y);
                sm.CenterOn(world_x, world_y, ScrollAnimationType::EASE_OUT, 20);
            }
        }

        screen.Unlock();
        screen.Flip();

        Platform_Delay(16);
    }

    printf("Visual test complete.\n");
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[]) {
    printf("==========================================\n");
    printf("Viewport & Scrolling Test Suite\n");
    printf("==========================================\n\n");

    // Check for quick mode
    bool quick_mode = false;
    bool interactive = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--quick") == 0) {
            quick_mode = true;
        }
        if (strcmp(argv[i], "-i") == 0) {
            interactive = true;
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
    test_initialization();
    test_coordinate_conversion();
    test_lepton_conversion();
    test_bounds_clamping();
    test_visibility();
    test_visible_cell_range();
    test_scroll_animation();
    test_center_on();
    test_coord_macros();
    test_edge_scroll();
    test_keyboard_scroll();
    test_tracking();
    test_scroll_manager_singleton();
    test_scroll_easing();

    printf("\n------------------------------------------\n");
    printf("Tests: %d/%d passed\n", tests_passed, tests_run);
    printf("------------------------------------------\n");

    if (tests_passed == tests_run && !quick_mode && interactive) {
        run_visual_test();
    } else if (tests_passed == tests_run && !quick_mode) {
        printf("\nRun with -i for interactive visual test\n");
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
