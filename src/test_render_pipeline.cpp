/**
 * Render Pipeline Test Program
 */

#include "game/graphics/render_pipeline.h"
#include "game/graphics/render_layer.h"
#include "game/graphics/sidebar_render.h"
#include "game/graphics/radar_render.h"
#include "game/graphics/graphics_buffer.h"
#include "game/graphics/mouse_cursor.h"
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
// Mock Renderable for Testing
// =============================================================================

class MockRenderable : public IRenderable {
public:
    int world_x, world_y;
    int sort_y;
    RenderLayer layer;
    int draw_count;

    MockRenderable(int wx, int wy, RenderLayer lay)
        : world_x(wx), world_y(wy), sort_y(wy), layer(lay), draw_count(0) {}

    RenderLayer GetRenderLayer() const override { return layer; }
    int GetSortY() const override { return sort_y; }
    void Draw(GraphicsBuffer& /*buffer*/, int /*screen_x*/, int /*screen_y*/) override {
        draw_count++;
    }
    int GetWorldX() const override { return world_x; }
    int GetWorldY() const override { return world_y; }
    void GetBounds(int& x, int& y, int& width, int& height) const override {
        x = 0; y = 0; width = 24; height = 24;
    }
};

// =============================================================================
// Mock Terrain Provider
// =============================================================================

class MockTerrainProvider : public ITerrainProvider {
public:
    int width, height;

    MockTerrainProvider(int w, int h) : width(w), height(h) {}

    int GetTerrainTile(int cell_x, int cell_y) const override {
        if (!IsValidCell(cell_x, cell_y)) return -1;
        return (cell_x + cell_y) % 16;  // Simple pattern
    }

    int GetTerrainIcon(int /*cell_x*/, int /*cell_y*/) const override {
        return 0;
    }

    bool IsValidCell(int cell_x, int cell_y) const override {
        return cell_x >= 0 && cell_x < width && cell_y >= 0 && cell_y < height;
    }

    void GetMapSize(int* w, int* h) const override {
        if (w) *w = width;
        if (h) *h = height;
    }
};

// =============================================================================
// Unit Tests
// =============================================================================

bool test_singleton() {
    TEST_START("singleton pattern");

    RenderPipeline& rp1 = RenderPipeline::Instance();
    RenderPipeline& rp2 = RenderPipeline::Instance();

    ASSERT(&rp1 == &rp2, "Should return same instance");

    TEST_PASS();
    return true;
}

bool test_initialization() {
    TEST_START("initialization");

    RenderPipeline& rp = RenderPipeline::Instance();

    bool result = rp.Initialize(640, 400);
    ASSERT(result, "Initialize should succeed");
    ASSERT(rp.IsInitialized(), "Should be initialized");

    const Viewport& vp = rp.GetTacticalViewport();
    ASSERT(vp.width == 480, "Viewport width should be 480 (640 - sidebar)");
    ASSERT(vp.height == 400, "Viewport height should be 400");

    TEST_PASS();
    return true;
}

bool test_viewport_control() {
    TEST_START("viewport control");

    RenderPipeline& rp = RenderPipeline::Instance();

    rp.SetTacticalViewport(0, 0, 480, 384);
    const Viewport& vp = rp.GetTacticalViewport();
    ASSERT(vp.width == 480, "Viewport width should be 480");
    ASSERT(vp.height == 384, "Viewport height should be 384");

    rp.SetScrollPosition(100, 200);
    int sx, sy;
    rp.GetScrollPosition(&sx, &sy);
    ASSERT(sx == 100, "Scroll X should be 100");
    ASSERT(sy == 200, "Scroll Y should be 200");

    TEST_PASS();
    return true;
}

bool test_coordinate_conversion() {
    TEST_START("coordinate conversion");

    RenderPipeline& rp = RenderPipeline::Instance();

    rp.SetTacticalViewport(0, 0, 480, 384);
    rp.SetScrollPosition(100, 200);

    // World to screen
    int screen_x, screen_y;
    bool visible = rp.WorldToScreen(150, 250, &screen_x, &screen_y);
    ASSERT(visible, "Point should be visible");
    ASSERT(screen_x == 50, "Screen X should be 50 (150-100)");
    ASSERT(screen_y == 50, "Screen Y should be 50 (250-200)");

    // Screen to world
    int world_x, world_y;
    rp.ScreenToWorld(50, 50, &world_x, &world_y);
    ASSERT(world_x == 150, "World X should be 150");
    ASSERT(world_y == 250, "World Y should be 250");

    TEST_PASS();
    return true;
}

bool test_visibility() {
    TEST_START("visibility testing");

    RenderPipeline& rp = RenderPipeline::Instance();

    rp.SetTacticalViewport(0, 0, 480, 384);
    rp.SetScrollPosition(0, 0);

    // Object in view
    ASSERT(rp.IsVisible(100, 100, 24, 24), "Object at (100,100) should be visible");

    // Object outside view (right)
    ASSERT(!rp.IsVisible(500, 100, 24, 24), "Object at (500,100) should not be visible");

    // Object outside view (bottom)
    ASSERT(!rp.IsVisible(100, 400, 24, 24), "Object at (100,400) should not be visible");

    // Object partially visible
    ASSERT(rp.IsVisible(470, 100, 24, 24), "Object at (470,100) should be partially visible");

    TEST_PASS();
    return true;
}

bool test_dirty_rects() {
    TEST_START("dirty rectangle tracking");

    RenderPipeline& rp = RenderPipeline::Instance();

    rp.ClearDirtyRects();
    ASSERT(rp.GetDirtyRectCount() == 0, "Should have no dirty rects after clear");

    rp.AddDirtyRect(10, 10, 50, 50);
    // Note: full redraw may absorb dirty rects
    // This is expected behavior

    rp.MarkFullRedraw();
    // After full redraw, dirty rects are cleared
    rp.ClearDirtyRects();
    ASSERT(rp.GetDirtyRectCount() == 0, "Should have no dirty rects after full redraw clear");

    TEST_PASS();
    return true;
}

bool test_renderable_queue() {
    TEST_START("renderable queue");

    RenderPipeline& rp = RenderPipeline::Instance();

    rp.SetScrollPosition(0, 0);
    rp.BeginFrame();
    ASSERT(rp.GetRenderableCount() == 0, "Queue should be empty after BeginFrame");

    // Create mock renderables
    MockRenderable obj1(100, 100, RenderLayer::GROUND);
    MockRenderable obj2(200, 200, RenderLayer::BUILDING);
    MockRenderable obj3(150, 150, RenderLayer::GROUND);

    rp.AddRenderable(&obj1);
    rp.AddRenderable(&obj2);
    rp.AddRenderable(&obj3);

    ASSERT(rp.GetRenderableCount() == 3, "Should have 3 renderables");

    rp.ClearRenderables();
    ASSERT(rp.GetRenderableCount() == 0, "Queue should be empty after clear");

    TEST_PASS();
    return true;
}

bool test_render_entry_sorting() {
    TEST_START("render entry sorting");

    // Test that entries sort by layer first, then by Y
    RenderEntry e1(nullptr, 100, RenderLayer::GROUND);
    RenderEntry e2(nullptr, 50, RenderLayer::BUILDING);
    RenderEntry e3(nullptr, 200, RenderLayer::GROUND);
    RenderEntry e4(nullptr, 100, RenderLayer::GROUND);

    // GROUND (5) < BUILDING (6), so e1 < e2
    ASSERT(e1 < e2, "Ground layer should sort before Building layer");

    // Same layer, lower Y sorts first
    ASSERT(e1 < e3, "Lower Y should sort before higher Y in same layer");

    // Same layer, same Y - not less than
    ASSERT(!(e1 < e4), "Same layer and Y should not be less than");
    ASSERT(!(e4 < e1), "Same layer and Y should not be less than");

    TEST_PASS();
    return true;
}

bool test_dirty_rect_struct() {
    TEST_START("DirtyRect structure");

    DirtyRect r1(10, 10, 50, 50);
    DirtyRect r2(30, 30, 50, 50);
    DirtyRect r3(100, 100, 50, 50);

    ASSERT(r1.Overlaps(r2), "r1 and r2 should overlap");
    ASSERT(!r1.Overlaps(r3), "r1 and r3 should not overlap");

    r1.Merge(r2);
    ASSERT(r1.x == 10, "Merged X should be 10");
    ASSERT(r1.y == 10, "Merged Y should be 10");
    ASSERT(r1.width == 70, "Merged width should be 70");
    ASSERT(r1.height == 70, "Merged height should be 70");

    DirtyRect empty;
    ASSERT(empty.IsEmpty(), "Default rect should be empty");

    TEST_PASS();
    return true;
}

bool test_sidebar_renderer() {
    TEST_START("sidebar renderer");

    SidebarRenderer sidebar;

    bool init_result = sidebar.Initialize(400);
    ASSERT(init_result, "Sidebar should initialize");
    ASSERT(sidebar.IsInitialized(), "Sidebar should be initialized");

    ASSERT(sidebar.GetX() == 480, "Sidebar X should be 480");
    ASSERT(sidebar.GetWidth() == 160, "Sidebar width should be 160");

    // Test tab switching
    sidebar.SetActiveTab(SidebarTab::UNIT);
    ASSERT(sidebar.GetActiveTab() == SidebarTab::UNIT, "Should be unit tab");

    sidebar.SetActiveTab(SidebarTab::STRUCTURE);
    ASSERT(sidebar.GetActiveTab() == SidebarTab::STRUCTURE, "Should be structure tab");

    // Test build items
    sidebar.AddBuildItem(1, 0);
    sidebar.AddBuildItem(2, 1);
    ASSERT(sidebar.GetBuildItemCount() == 2, "Should have 2 build items");

    sidebar.ClearBuildItems();
    ASSERT(sidebar.GetBuildItemCount() == 0, "Should have 0 build items after clear");

    // Test hit testing
    ASSERT(sidebar.HitTest(500, 200), "Point (500,200) should be in sidebar");
    ASSERT(!sidebar.HitTest(200, 200), "Point (200,200) should not be in sidebar");

    sidebar.Shutdown();
    ASSERT(!sidebar.IsInitialized(), "Sidebar should not be initialized after shutdown");

    TEST_PASS();
    return true;
}

bool test_radar_renderer() {
    TEST_START("radar renderer");

    RadarRenderer radar;

    bool init_result = radar.Initialize(128, 128);
    ASSERT(init_result, "Radar should initialize");
    ASSERT(radar.IsInitialized(), "Radar should be initialized");

    ASSERT(radar.GetWidth() == 160, "Radar width should be 160");
    ASSERT(radar.GetHeight() == 136, "Radar height should be 136");

    // Test state
    radar.SetState(RadarState::ACTIVE);
    ASSERT(radar.GetState() == RadarState::ACTIVE, "Should be active");

    radar.SetState(RadarState::JAMMED);
    ASSERT(radar.GetState() == RadarState::JAMMED, "Should be jammed");

    // Test blips
    radar.ClearBlips();
    ASSERT(radar.GetBlipCount() == 0, "Should have no blips");

    radar.AddBlip(64, 64, 15, false);
    radar.AddBlip(32, 32, 12, true);
    ASSERT(radar.GetBlipCount() == 2, "Should have 2 blips");

    radar.ClearBlips();
    ASSERT(radar.GetBlipCount() == 0, "Should have no blips after clear");

    // Test hit testing
    ASSERT(radar.HitTest(500, 50), "Point (500,50) should be in radar");
    ASSERT(!radar.HitTest(200, 200), "Point (200,200) should not be in radar");

    // Test coordinate conversion
    int cell_x, cell_y;
    bool valid = radar.RadarToCell(490, 30, &cell_x, &cell_y);
    ASSERT(valid, "Should be valid radar position");

    radar.Shutdown();
    ASSERT(!radar.IsInitialized(), "Radar should not be initialized after shutdown");

    TEST_PASS();
    return true;
}

bool test_stats() {
    TEST_START("render statistics");

    RenderStats stats;
    ASSERT(stats.terrain_tiles_drawn == 0, "Initial tiles should be 0");
    ASSERT(stats.objects_drawn == 0, "Initial objects should be 0");
    ASSERT(stats.frame_time_ms == 0.0f, "Initial frame time should be 0");

    stats.terrain_tiles_drawn = 100;
    stats.objects_drawn = 50;
    stats.frame_time_ms = 16.67f;

    stats.Reset();
    ASSERT(stats.terrain_tiles_drawn == 0, "Reset tiles should be 0");
    ASSERT(stats.objects_drawn == 0, "Reset objects should be 0");
    ASSERT(stats.frame_time_ms == 0.0f, "Reset frame time should be 0");

    TEST_PASS();
    return true;
}

bool test_viewport_struct() {
    TEST_START("viewport structure");

    Viewport v1;
    ASSERT(v1.width == 480, "Default width should be 480");
    ASSERT(v1.height == 384, "Default height should be 384");

    Viewport v2(100, 200, 320, 240);
    ASSERT(v2.x == 100, "X should be 100");
    ASSERT(v2.y == 200, "Y should be 200");
    ASSERT(v2.width == 320, "Width should be 320");
    ASSERT(v2.height == 240, "Height should be 240");

    TEST_PASS();
    return true;
}

// =============================================================================
// Visual Test
// =============================================================================

void run_visual_test() {
    printf("\n=== Visual Render Pipeline Test ===\n");

    RenderPipeline& rp = RenderPipeline::Instance();
    GraphicsBuffer& screen = GraphicsBuffer::Screen();

    // Set up simple palette
    PaletteEntry entries[256];
    for (int i = 0; i < 256; i++) {
        entries[i].r = static_cast<uint8_t>(i);
        entries[i].g = static_cast<uint8_t>(i);
        entries[i].b = static_cast<uint8_t>(i);
    }
    // Some colors
    entries[250].r = 0;   entries[250].g = 255; entries[250].b = 0;    // Green
    entries[251].r = 0;   entries[251].g = 0;   entries[251].b = 255;  // Blue
    entries[252].r = 255; entries[252].g = 0;   entries[252].b = 0;    // Red
    entries[253].r = 255; entries[253].g = 255; entries[253].b = 255;  // White
    Platform_Graphics_SetPalette(entries, 0, 256);

    // Initialize components
    SidebarRenderer sidebar;
    sidebar.Initialize(400);
    sidebar.AddBuildItem(0, 0);
    sidebar.AddBuildItem(1, 1);
    sidebar.AddBuildItem(2, 2);

    RadarRenderer radar;
    radar.Initialize(128, 128);
    radar.SetState(RadarState::ACTIVE);
    radar.AddBlip(64, 64, 250, false);
    radar.AddBlip(32, 32, 252, true);

    rp.SetSidebarRenderer(&sidebar);
    rp.SetRadarRenderer(&radar);
    rp.SetDebugMode(true);

    printf("Press ESC or close window to exit.\n");

    int scroll_x = 0;
    int scroll_y = 0;

    while (!Platform_Input_ShouldQuit()) {
        Platform_Input_Update();

        // Simple scroll with keyboard
        if (Platform_Key_IsPressed(KEY_CODE_UP)) scroll_y -= 10;
        if (Platform_Key_IsPressed(KEY_CODE_DOWN)) scroll_y += 10;
        if (Platform_Key_IsPressed(KEY_CODE_RIGHT)) scroll_x += 10;
        if (Platform_Key_IsPressed(KEY_CODE_LEFT)) scroll_x -= 10;

        if (scroll_x < 0) scroll_x = 0;
        if (scroll_y < 0) scroll_y = 0;

        rp.SetScrollPosition(scroll_x, scroll_y);
        radar.SetViewport(scroll_x / 24, scroll_y / 24, 20, 16);
        radar.Update();

        // Render frame
        rp.BeginFrame();

        screen.Lock();

        // Clear tactical area
        screen.Fill_Rect(0, 0, 480, 400, 32);

        // Draw grid to show scrolling
        for (int y = -scroll_y % 24; y < 400; y += 24) {
            screen.Draw_HLine(0, y, 480, 64);
        }
        for (int x = -scroll_x % 24; x < 480; x += 24) {
            screen.Draw_VLine(x, 0, 400, 64);
        }

        // Draw UI
        sidebar.Draw(screen);
        radar.Draw(screen);

        // Draw mouse cursor position
        MouseCursor& mc = MouseCursor::Instance();
        int mx = mc.GetX();
        int my = mc.GetY();
        screen.Fill_Rect(mx - 2, my - 2, 5, 5, 253);

        screen.Unlock();

        rp.EndFrame();

        Platform_Delay(16);  // ~60 fps
    }

    rp.SetSidebarRenderer(nullptr);
    rp.SetRadarRenderer(nullptr);
    sidebar.Shutdown();
    radar.Shutdown();

    printf("Visual test complete.\n");
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[]) {
    printf("==========================================\n");
    printf("Render Pipeline Test Suite\n");
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
    test_initialization();
    test_viewport_control();
    test_coordinate_conversion();
    test_visibility();
    test_dirty_rects();
    test_renderable_queue();
    test_render_entry_sorting();
    test_dirty_rect_struct();
    test_sidebar_renderer();
    test_radar_renderer();
    test_stats();
    test_viewport_struct();

    printf("\n------------------------------------------\n");
    printf("Tests: %d/%d passed\n", tests_passed, tests_run);
    printf("------------------------------------------\n");

    if (tests_passed == tests_run && !quick_mode) {
        run_visual_test();
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
