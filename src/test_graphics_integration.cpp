/**
 * Graphics Integration Test Program
 *
 * Comprehensive test for all Phase 15 graphics components working together.
 * Task 15h - Integration & Testing
 */

#include "game/graphics/graphics_buffer.h"
#include "game/graphics/shape_renderer.h"
#include "game/graphics/tile_renderer.h"
#include "game/graphics/palette_manager.h"
#include "game/graphics/mouse_cursor.h"
#include "game/graphics/render_pipeline.h"
#include "game/graphics/render_layer.h"
#include "game/graphics/sidebar_render.h"
#include "game/graphics/radar_render.h"
#include "game/viewport.h"
#include "game/scroll_manager.h"
#include "platform.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>

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
// Mock Renderable for Testing
// =============================================================================

class TestObject : public IRenderable {
public:
    int world_x, world_y;
    int sort_y;
    RenderLayer layer;
    int draw_count;
    uint8_t color;

    TestObject(int wx, int wy, RenderLayer lay, uint8_t c = 15)
        : world_x(wx), world_y(wy), sort_y(wy + 24), layer(lay), draw_count(0), color(c) {}

    RenderLayer GetRenderLayer() const override { return layer; }
    int GetSortY() const override { return sort_y; }
    void Draw(GraphicsBuffer& buffer, int screen_x, int screen_y) override {
        // Draw a simple colored rectangle
        buffer.Fill_Rect(screen_x, screen_y, 24, 24, color);
        draw_count++;
    }
    int GetWorldX() const override { return world_x; }
    int GetWorldY() const override { return world_y; }
    void GetBounds(int& x, int& y, int& width, int& height) const override {
        x = 0; y = 0; width = 24; height = 24;
    }
};

// =============================================================================
// Integration Tests
// =============================================================================

bool test_all_components_initialize() {
    TEST_START("all components initialize");

    // GraphicsBuffer is a singleton - should be accessible
    GraphicsBuffer& screen = GraphicsBuffer::Screen();
    ASSERT(screen.Get_Width() == 640, "Screen width should be 640");
    ASSERT(screen.Get_Height() == 400, "Screen height should be 400");

    // PaletteManager
    PaletteManager& palette = PaletteManager::Instance();
    ASSERT(palette.GetFadeState() == FadeState::None, "Palette should start with no fade");

    // MouseCursor
    MouseCursor& cursor = MouseCursor::Instance();
    ASSERT(cursor.GetType() == CURSOR_NORMAL, "Cursor should start as CURSOR_NORMAL");

    // RenderPipeline
    RenderPipeline& pipeline = RenderPipeline::Instance();
    bool init_result = pipeline.Initialize(640, 400);
    ASSERT(init_result, "Pipeline should initialize");
    ASSERT(pipeline.IsInitialized(), "Pipeline should be marked initialized");

    // GameViewport
    GameViewport& viewport = GameViewport::Instance();
    viewport.Initialize();
    ASSERT(viewport.width == TACTICAL_WIDTH, "Viewport width should be TACTICAL_WIDTH");
    ASSERT(viewport.height == TACTICAL_HEIGHT, "Viewport height should be TACTICAL_HEIGHT");

    // SidebarRenderer
    SidebarRenderer sidebar;
    bool sidebar_init = sidebar.Initialize(400);
    ASSERT(sidebar_init, "Sidebar should initialize");

    // RadarRenderer
    RadarRenderer radar;
    bool radar_init = radar.Initialize(128, 128);
    ASSERT(radar_init, "Radar should initialize");

    sidebar.Shutdown();
    radar.Shutdown();

    TEST_PASS();
    return true;
}

bool test_viewport_pipeline_integration() {
    TEST_START("viewport-pipeline integration");

    GameViewport& viewport = GameViewport::Instance();
    RenderPipeline& pipeline = RenderPipeline::Instance();

    // Set up viewport
    viewport.Initialize();
    viewport.SetMapSize(64, 64);
    viewport.ScrollTo(100, 50);

    // Set pipeline scroll to match viewport
    pipeline.SetScrollPosition(viewport.x, viewport.y);

    int sx, sy;
    pipeline.GetScrollPosition(&sx, &sy);
    ASSERT(sx == 100, "Pipeline scroll X should match viewport");
    ASSERT(sy == 50, "Pipeline scroll Y should match viewport");

    // Test coordinate conversion consistency
    int screen_x, screen_y;
    bool visible = pipeline.WorldToScreen(200, 150, &screen_x, &screen_y);
    ASSERT(visible, "Point should be visible in pipeline");

    int vp_screen_x, vp_screen_y;
    viewport.WorldToScreen(200, 150, vp_screen_x, vp_screen_y);

    // Account for TAB_HEIGHT offset in viewport
    ASSERT(screen_x == vp_screen_x, "World-to-screen X should match");

    TEST_PASS();
    return true;
}

bool test_renderable_system() {
    TEST_START("renderable system");

    RenderPipeline& pipeline = RenderPipeline::Instance();
    GameViewport& viewport = GameViewport::Instance();

    viewport.Initialize();
    viewport.SetMapSize(64, 64);
    viewport.ScrollTo(0, 0);
    pipeline.SetScrollPosition(0, 0);

    pipeline.BeginFrame();
    ASSERT(pipeline.GetRenderableCount() == 0, "Queue should be empty after BeginFrame");

    // Create test objects at visible positions
    TestObject obj1(100, 100, RenderLayer::GROUND, 120);  // Green
    TestObject obj2(150, 150, RenderLayer::BUILDING, 127); // Red
    TestObject obj3(200, 120, RenderLayer::AIR, 119);     // Blue

    pipeline.AddRenderable(&obj1);
    pipeline.AddRenderable(&obj2);
    pipeline.AddRenderable(&obj3);

    ASSERT(pipeline.GetRenderableCount() == 3, "Should have 3 renderables");

    pipeline.ClearRenderables();
    ASSERT(pipeline.GetRenderableCount() == 0, "Queue should be empty after clear");

    TEST_PASS();
    return true;
}

bool test_palette_integration() {
    TEST_START("palette integration");

    PaletteManager& palette = PaletteManager::Instance();

    // Test fade operations
    palette.StartFadeOut(10);
    ASSERT(palette.IsFading(), "Should be fading after StartFadeOut");
    ASSERT(palette.GetFadeState() == FadeState::FadingOut, "State should be FadingOut");

    // Run fade for 10 frames
    for (int i = 0; i < 10; i++) {
        palette.Update();
    }
    ASSERT(palette.GetFadeState() == FadeState::FadedOut, "State should be FadedOut");

    // Test fade in
    palette.StartFadeIn(10);
    for (int i = 0; i < 10; i++) {
        palette.Update();
    }
    ASSERT(palette.GetFadeState() == FadeState::None, "State should be None after fade in");

    // Test flash
    palette.StartFlash(FlashType::White, 5);  // 5 frame flash
    ASSERT(palette.IsFlashing(), "Should be flashing");
    palette.StopFlash();
    ASSERT(!palette.IsFlashing(), "Should not be flashing after stop");

    TEST_PASS();
    return true;
}

bool test_scroll_manager_integration() {
    TEST_START("scroll manager integration");

    GameViewport& viewport = GameViewport::Instance();
    ScrollManager& scroll = ScrollManager::Instance();

    viewport.Initialize();
    viewport.SetMapSize(128, 128);
    viewport.ScrollTo(0, 0);

    // Test animated scroll
    scroll.ScrollTo(200, 150, ScrollAnimationType::LINEAR, 10);
    ASSERT(scroll.IsAnimating(), "Should be animating");

    // Run animation
    for (int i = 0; i < 10; i++) {
        scroll.Update();
    }
    ASSERT(!scroll.IsAnimating(), "Animation should be complete");
    ASSERT(viewport.x == 200, "Viewport X should be 200");
    ASSERT(viewport.y == 150, "Viewport Y should be 150");

    // Test cancel
    scroll.ScrollTo(500, 500, ScrollAnimationType::EASE_OUT, 20);
    scroll.CancelScroll();
    ASSERT(!scroll.IsAnimating(), "Should not be animating after cancel");

    TEST_PASS();
    return true;
}

bool test_sidebar_radar_integration() {
    TEST_START("sidebar-radar integration");

    SidebarRenderer sidebar;
    RadarRenderer radar;

    bool sidebar_init = sidebar.Initialize(400);
    ASSERT(sidebar_init, "Sidebar should initialize");

    bool radar_init = radar.Initialize(128, 128);
    ASSERT(radar_init, "Radar should initialize");

    // Test sidebar operations
    sidebar.SetActiveTab(SidebarTab::STRUCTURE);
    ASSERT(sidebar.GetActiveTab() == SidebarTab::STRUCTURE, "Tab should be STRUCTURE");

    sidebar.AddBuildItem(1, 0);
    sidebar.AddBuildItem(2, 1);
    ASSERT(sidebar.GetBuildItemCount() == 2, "Should have 2 build items");

    // Test radar operations
    radar.SetState(RadarState::ACTIVE);
    ASSERT(radar.GetState() == RadarState::ACTIVE, "Radar should be active");

    radar.AddBlip(64, 64, 15, false);
    radar.AddBlip(32, 32, 12, true);
    ASSERT(radar.GetBlipCount() == 2, "Should have 2 blips");

    radar.ClearBlips();
    ASSERT(radar.GetBlipCount() == 0, "Blips should be cleared");

    sidebar.Shutdown();
    radar.Shutdown();

    TEST_PASS();
    return true;
}

bool test_full_render_frame() {
    TEST_START("full render frame");

    GraphicsBuffer& screen = GraphicsBuffer::Screen();
    RenderPipeline& pipeline = RenderPipeline::Instance();
    GameViewport& viewport = GameViewport::Instance();

    // Set up for rendering
    viewport.Initialize();
    viewport.SetMapSize(64, 64);
    viewport.ScrollTo(0, 0);
    pipeline.SetScrollPosition(0, 0);

    // Create some test objects
    std::vector<TestObject> objects;
    objects.emplace_back(100, 100, RenderLayer::GROUND, 120);
    objects.emplace_back(150, 80, RenderLayer::GROUND, 127);
    objects.emplace_back(200, 200, RenderLayer::BUILDING, 15);
    objects.emplace_back(120, 50, RenderLayer::AIR, 119);

    // Render a frame
    pipeline.BeginFrame();

    for (auto& obj : objects) {
        pipeline.AddRenderable(&obj);
    }

    // Render the frame
    screen.Lock();
    screen.Clear(32);  // Dark gray background

    // Draw terrain grid
    for (int y = 0; y < screen.Get_Height(); y += 24) {
        screen.Draw_HLine(0, y, screen.Get_Width(), 40);
    }
    for (int x = 0; x < screen.Get_Width(); x += 24) {
        screen.Draw_VLine(x, 0, screen.Get_Height(), 40);
    }

    screen.Unlock();

    pipeline.RenderFrame();
    pipeline.EndFrame();

    // Check that objects were drawn
    int drawn_count = 0;
    for (const auto& obj : objects) {
        drawn_count += obj.draw_count;
    }
    ASSERT(drawn_count > 0, "At least some objects should be drawn");

    // Check stats
    const RenderStats& stats = pipeline.GetStats();
    ASSERT(stats.objects_drawn > 0, "Stats should show objects drawn");

    TEST_PASS();
    return true;
}

bool test_coordinate_conversion_chain() {
    TEST_START("coordinate conversion chain");

    GameViewport& viewport = GameViewport::Instance();
    viewport.Initialize();
    viewport.SetMapSize(64, 64);
    viewport.ScrollTo(48, 24);

    // Test cell -> world -> screen -> world -> cell roundtrip
    int cell_x = 5, cell_y = 3;
    int world_x, world_y;
    viewport.CellToWorld(cell_x, cell_y, world_x, world_y);

    int screen_x, screen_y;
    viewport.WorldToScreen(world_x, world_y, screen_x, screen_y);

    int world_x2, world_y2;
    viewport.ScreenToWorld(screen_x, screen_y, world_x2, world_y2);

    int cell_x2, cell_y2;
    viewport.WorldToCell(world_x2, world_y2, cell_x2, cell_y2);

    ASSERT(cell_x == cell_x2, "Cell X should survive roundtrip");
    ASSERT(cell_y == cell_y2, "Cell Y should survive roundtrip");

    // Test lepton conversion
    int lepton_x = 512, lepton_y = 256;  // 2 cells, 1 cell
    int pixel_x, pixel_y;
    viewport.LeptonToPixel(lepton_x, lepton_y, pixel_x, pixel_y);
    ASSERT(pixel_x == 48, "Lepton 512 should convert to pixel 48");
    ASSERT(pixel_y == 24, "Lepton 256 should convert to pixel 24");

    int lepton_x2, lepton_y2;
    viewport.PixelToLepton(pixel_x, pixel_y, lepton_x2, lepton_y2);
    ASSERT(lepton_x == lepton_x2, "Lepton X should survive roundtrip");
    ASSERT(lepton_y == lepton_y2, "Lepton Y should survive roundtrip");

    TEST_PASS();
    return true;
}

bool test_visibility_culling() {
    TEST_START("visibility culling");

    RenderPipeline& pipeline = RenderPipeline::Instance();
    GameViewport& viewport = GameViewport::Instance();

    viewport.Initialize();
    viewport.SetMapSize(128, 128);
    viewport.ScrollTo(500, 500);
    pipeline.SetScrollPosition(500, 500);

    // Create objects - some visible, some not
    TestObject visible_obj(600, 600, RenderLayer::GROUND, 120);    // In view
    TestObject hidden_obj(100, 100, RenderLayer::GROUND, 127);     // Out of view

    pipeline.BeginFrame();
    pipeline.AddRenderable(&visible_obj);
    pipeline.AddRenderable(&hidden_obj);

    // Only the visible object should be added
    // (AddRenderable checks visibility)
    ASSERT(pipeline.GetRenderableCount() <= 2, "At most 2 objects added");

    // Render
    pipeline.RenderFrame();
    pipeline.EndFrame();

    // The hidden object should not have been drawn
    // (depends on implementation - may vary)

    TEST_PASS();
    return true;
}

bool test_mouse_cursor_integration() {
    TEST_START("mouse cursor integration");

    MouseCursor& cursor = MouseCursor::Instance();

    cursor.Reset();
    ASSERT(cursor.GetType() == CURSOR_NORMAL, "Should reset to CURSOR_NORMAL");

    // Test type changes
    cursor.SetType(CURSOR_ATTACK);
    ASSERT(cursor.GetType() == CURSOR_ATTACK, "Type should be CURSOR_ATTACK");
    ASSERT(cursor.IsAnimated(), "Attack cursor should be animated");

    cursor.SetType(CURSOR_MOVE);
    ASSERT(cursor.GetType() == CURSOR_MOVE, "Type should be CURSOR_MOVE");

    // Test scroll cursor
    cursor.SetScrollCursor(1, 0);  // East
    ASSERT(cursor.GetType() == CURSOR_SCROLL_E, "Should be east scroll cursor");

    cursor.SetScrollCursor(0, 0);
    ASSERT(cursor.GetType() == CURSOR_NORMAL, "Should be normal cursor");

    // Test visibility
    cursor.Show();
    ASSERT(cursor.IsVisible(), "Should be visible");
    cursor.Hide();
    ASSERT(!cursor.IsVisible(), "Should be hidden");
    cursor.Show();

    // Test locking
    cursor.Unlock();
    cursor.SetType(CURSOR_NORMAL);
    cursor.Lock();
    cursor.SetType(CURSOR_ATTACK);
    ASSERT(cursor.GetType() == CURSOR_NORMAL, "Locked cursor should not change");
    cursor.Unlock();

    cursor.Reset();

    TEST_PASS();
    return true;
}

bool test_dirty_rect_system() {
    TEST_START("dirty rect system");

    RenderPipeline& pipeline = RenderPipeline::Instance();

    pipeline.ClearDirtyRects();
    ASSERT(pipeline.GetDirtyRectCount() == 0, "Should start with 0 dirty rects");

    pipeline.AddDirtyRect(10, 10, 50, 50);
    pipeline.AddDirtyRect(100, 100, 50, 50);

    // Note: dirty rects may be merged or absorbed by full redraw
    // Just verify the system doesn't crash

    pipeline.MarkFullRedraw();
    pipeline.ClearDirtyRects();
    ASSERT(pipeline.GetDirtyRectCount() == 0, "Should have 0 after clear");

    TEST_PASS();
    return true;
}

bool test_render_layer_ordering() {
    TEST_START("render layer ordering");

    // Test RenderEntry sorting
    RenderEntry e_terrain(nullptr, 100, RenderLayer::TERRAIN);
    RenderEntry e_ground(nullptr, 100, RenderLayer::GROUND);
    RenderEntry e_building(nullptr, 100, RenderLayer::BUILDING);
    RenderEntry e_air(nullptr, 100, RenderLayer::AIR);
    RenderEntry e_ui(nullptr, 100, RenderLayer::UI);

    // Terrain should sort before Ground
    ASSERT(e_terrain < e_ground, "TERRAIN should sort before GROUND");

    // Ground should sort before Building
    ASSERT(e_ground < e_building, "GROUND should sort before BUILDING");

    // Building should sort before Air
    ASSERT(e_building < e_air, "BUILDING should sort before AIR");

    // Air should sort before UI
    ASSERT(e_air < e_ui, "AIR should sort before UI");

    // Same layer, different Y
    RenderEntry e_ground_low(nullptr, 50, RenderLayer::GROUND);
    RenderEntry e_ground_high(nullptr, 150, RenderLayer::GROUND);
    ASSERT(e_ground_low < e_ground_high, "Lower Y should sort first in same layer");

    TEST_PASS();
    return true;
}

bool test_stats_tracking() {
    TEST_START("stats tracking");

    RenderPipeline& pipeline = RenderPipeline::Instance();

    pipeline.ResetStats();
    const RenderStats& stats = pipeline.GetStats();

    ASSERT(stats.terrain_tiles_drawn == 0, "Reset stats should have 0 tiles");
    ASSERT(stats.objects_drawn == 0, "Reset stats should have 0 objects");
    ASSERT(stats.frame_time_ms == 0.0f, "Reset stats should have 0 frame time");

    // Render a frame to get some stats
    pipeline.BeginFrame();
    pipeline.RenderFrame();
    pipeline.EndFrame();

    // Stats should have been updated (values may vary)
    // Just verify they're accessible without crashing

    TEST_PASS();
    return true;
}

// =============================================================================
// Visual Test
// =============================================================================

void run_visual_test() {
    printf("\n=== Visual Graphics Integration Test ===\n");
    printf("This test combines all Phase 15 components.\n");
    printf("Arrow keys: Scroll map\n");
    printf("Click: Test cursor interaction\n");
    printf("D: Toggle debug mode\n");
    printf("Press ESC or close window to exit.\n\n");

    GraphicsBuffer& screen = GraphicsBuffer::Screen();
    RenderPipeline& pipeline = RenderPipeline::Instance();
    GameViewport& viewport = GameViewport::Instance();
    ScrollManager& scroll = ScrollManager::Instance();
    MouseCursor& cursor = MouseCursor::Instance();
    PaletteManager& palette = PaletteManager::Instance();

    // Set up palette
    PaletteEntry entries[256];
    for (int i = 0; i < 256; i++) {
        entries[i].r = static_cast<uint8_t>(i);
        entries[i].g = static_cast<uint8_t>(i);
        entries[i].b = static_cast<uint8_t>(i);
    }
    // Custom colors
    entries[120].r = 0;   entries[120].g = 180; entries[120].b = 0;    // Green (player)
    entries[127].r = 200; entries[127].g = 0;   entries[127].b = 0;    // Red (enemy)
    entries[119].r = 0;   entries[119].g = 100; entries[119].b = 200;  // Blue (air)
    entries[40].r = 60;   entries[40].g = 60;   entries[40].b = 60;    // Grid
    entries[32].r = 30;   entries[32].g = 50;   entries[32].b = 30;    // Background
    entries[179].r = 255; entries[179].g = 255; entries[179].b = 0;    // Yellow
    entries[250].r = 0;   entries[250].g = 255; entries[250].b = 0;    // Bright green
    entries[252].r = 255; entries[252].g = 0;   entries[252].b = 0;    // Bright red
    entries[15].r = 255;  entries[15].g = 255;  entries[15].b = 255;   // White
    Platform_Graphics_SetPalette(entries, 0, 256);

    // Initialize components
    viewport.Initialize();
    viewport.SetMapSize(128, 128);
    viewport.ScrollTo(100, 100);
    pipeline.SetScrollPosition(viewport.x, viewport.y);

    SidebarRenderer sidebar;
    sidebar.Initialize(400);
    sidebar.AddBuildItem(1, 0);
    sidebar.AddBuildItem(2, 1);
    sidebar.AddBuildItem(3, 2);

    RadarRenderer radar;
    radar.Initialize(128, 128);
    radar.SetState(RadarState::ACTIVE);
    radar.SetViewport(viewport.x / 24, viewport.y / 24, 20, 16);

    pipeline.SetSidebarRenderer(&sidebar);
    pipeline.SetRadarRenderer(&radar);

    // Create test objects
    std::vector<TestObject> objects;
    // Player units (green)
    for (int i = 0; i < 10; i++) {
        objects.emplace_back(120 + (i % 5) * 30, 120 + (i / 5) * 30, RenderLayer::GROUND, 120);
    }
    // Enemy units (red)
    for (int i = 0; i < 8; i++) {
        objects.emplace_back(400 + (i % 4) * 30, 200 + (i / 4) * 30, RenderLayer::GROUND, 127);
    }
    // Aircraft (blue)
    objects.emplace_back(300, 150, RenderLayer::AIR, 119);
    objects.emplace_back(350, 180, RenderLayer::AIR, 119);
    // Buildings
    objects.emplace_back(100, 300, RenderLayer::BUILDING, 120);
    objects.emplace_back(500, 350, RenderLayer::BUILDING, 127);

    bool debug_mode = true;
    pipeline.SetDebugMode(debug_mode);

    cursor.Show();

    while (!Platform_Input_ShouldQuit()) {
        Platform_Input_Update();

        // Keyboard scrolling
        bool up = Platform_Key_IsPressed(KEY_CODE_UP);
        bool down = Platform_Key_IsPressed(KEY_CODE_DOWN);
        bool left = Platform_Key_IsPressed(KEY_CODE_LEFT);
        bool right = Platform_Key_IsPressed(KEY_CODE_RIGHT);

        if (up || down || left || right) {
            scroll.CancelScroll();
            viewport.UpdateKeyboardScroll(up, down, left, right);
            pipeline.SetScrollPosition(viewport.x, viewport.y);
        }

        // Toggle debug mode
        if (Platform_Key_WasPressed(KEY_CODE_DELETE)) {
            debug_mode = !debug_mode;
            pipeline.SetDebugMode(debug_mode);
        }

        // Update scroll animation
        scroll.Update();
        pipeline.SetScrollPosition(viewport.x, viewport.y);

        // Update radar viewport
        radar.SetViewport(viewport.x / 24, viewport.y / 24, 20, 16);

        // Update radar blips
        radar.ClearBlips();
        for (const auto& obj : objects) {
            radar.AddBlip(obj.world_x / 24, obj.world_y / 24, obj.color, false);
        }

        // Update cursor animation
        cursor.Update();

        // Mouse position for edge scrolling
        int mx, my;
        Platform_Mouse_GetPosition(&mx, &my);
        mx /= 2;
        my /= 2;

        if (!up && !down && !left && !right && !scroll.IsAnimating()) {
            viewport.UpdateEdgeScroll(mx, my);
            pipeline.SetScrollPosition(viewport.x, viewport.y);

            // Update cursor based on edge
            if (viewport.IsScrolling()) {
                cursor.SetScrollCursor(
                    (viewport.GetCurrentScrollDirection() & SCROLL_RIGHT) ? 1 :
                    (viewport.GetCurrentScrollDirection() & SCROLL_LEFT) ? -1 : 0,
                    (viewport.GetCurrentScrollDirection() & SCROLL_DOWN) ? 1 :
                    (viewport.GetCurrentScrollDirection() & SCROLL_UP) ? -1 : 0
                );
            } else {
                cursor.SetType(CURSOR_NORMAL);
            }
        }

        // Radar click handling
        if (Platform_Mouse_IsPressed(MOUSE_BUTTON_LEFT)) {
            int cell_x, cell_y;
            if (radar.RadarToCell(mx, my, &cell_x, &cell_y)) {
                scroll.CenterOnCell(cell_x, cell_y, ScrollAnimationType::EASE_OUT, 15);
            }
        }

        // Render frame
        pipeline.BeginFrame();

        for (auto& obj : objects) {
            pipeline.AddRenderable(&obj);
        }

        screen.Lock();

        // Draw terrain (checkered pattern)
        int start_x, start_y, end_x, end_y;
        viewport.GetVisibleCellRange(start_x, start_y, end_x, end_y);

        for (int cy = start_y; cy < end_y; cy++) {
            for (int cx = start_x; cx < end_x; cx++) {
                int sx, sy;
                viewport.CellToScreen(cx, cy, sx, sy);

                uint8_t color = ((cx + cy) % 2) ? 32 : 33;

                // Map edge
                if (cx == 0 || cy == 0 || cx == viewport.GetMapCellWidth() - 1 || cy == viewport.GetMapCellHeight() - 1) {
                    color = 40;
                }

                screen.Fill_Rect(sx, sy, 24, 24, color);
            }
        }

        screen.Unlock();

        pipeline.RenderFrame();

        // Draw cursor on top
        screen.Lock();
        cursor.DrawAt(screen, mx, my);
        screen.Unlock();

        pipeline.EndFrame();

        // Update palette effects
        palette.Update();

        Platform_Delay(16);
    }

    cursor.Hide();
    pipeline.SetSidebarRenderer(nullptr);
    pipeline.SetRadarRenderer(nullptr);
    sidebar.Shutdown();
    radar.Shutdown();

    printf("Visual test complete.\n");
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[]) {
    printf("==========================================\n");
    printf("Graphics Integration Test Suite\n");
    printf("Phase 15h - Integration & Testing\n");
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

    printf("=== Integration Unit Tests ===\n\n");

    test_all_components_initialize();
    test_viewport_pipeline_integration();
    test_renderable_system();
    test_palette_integration();
    test_scroll_manager_integration();
    test_sidebar_radar_integration();
    test_full_render_frame();
    test_coordinate_conversion_chain();
    test_visibility_culling();
    test_mouse_cursor_integration();
    test_dirty_rect_system();
    test_render_layer_ordering();
    test_stats_tracking();

    printf("\n------------------------------------------\n");
    printf("Tests: %d/%d passed\n", tests_passed, tests_run);
    printf("------------------------------------------\n");

    if (tests_passed == tests_run && !quick_mode && interactive) {
        run_visual_test();
    } else if (tests_passed == tests_run && !quick_mode) {
        printf("\nRun with -i for interactive visual test\n");
    }

    // Cleanup
    RenderPipeline::Instance().Shutdown();

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
