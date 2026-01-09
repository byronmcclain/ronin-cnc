# Task 15h: Integration & Testing

| Field | Value |
|-------|-------|
| **Task ID** | 15h |
| **Task Name** | Integration & Testing |
| **Phase** | 15 - Graphics Integration |
| **Depends On** | 15a, 15b, 15c, 15d, 15e, 15f, 15g (All previous Phase 15 tasks) |
| **Estimated Complexity** | High (~1,000 lines) |

---

## Context

This final task brings together all the graphics components built in Phase 15 and verifies they work as a cohesive system. The integration includes:

1. **GraphicsBuffer** (15a) - Low-level pixel buffer access
2. **ShapeRenderer** (15b) - Sprite/shape drawing with effects
3. **TileRenderer** (15c) - Terrain tile rendering
4. **PaletteManager** (15d) - Color palette and effects
5. **MouseCursor** (15e) - Cursor display and animation
6. **RenderPipeline** (15f) - Layer-based rendering orchestration
7. **Viewport** (15g) - Map scrolling and coordinates

The integration test will create a complete visual demo showing terrain, objects, UI, scrolling, and all rendering features working together.

---

## Objective

Create comprehensive integration tests and a visual demo that exercises all Phase 15 graphics components together, ensuring they interact correctly and maintain target performance.

---

## Technical Background

### Integration Points

The components interact in specific ways:

```
                    ┌──────────────────┐
                    │   RenderPipeline │  (orchestrates all rendering)
                    └────────┬─────────┘
                             │
    ┌────────────────────────┼────────────────────────┐
    │                        │                        │
    ▼                        ▼                        ▼
┌──────────┐          ┌──────────────┐         ┌──────────┐
│ Viewport │          │ GraphicsBuffer│         │  Palette │
│  (15g)   │          │    (15a)     │         │  Manager │
└────┬─────┘          └──────┬───────┘         │  (15d)   │
     │                       │                 └────┬─────┘
     │    ┌──────────────────┼──────────────────────┤
     │    │                  │                      │
     ▼    ▼                  ▼                      ▼
┌──────────────┐      ┌──────────────┐      ┌──────────────┐
│ TileRenderer │      │ShapeRenderer │      │ MouseCursor  │
│    (15c)     │      │    (15b)     │      │    (15e)     │
└──────────────┘      └──────────────┘      └──────────────┘
```

### Performance Targets

- **Frame Rate:** 60 FPS (16.67ms per frame)
- **Render Budget:** 10ms for rendering, 6ms for logic/input
- **Memory:** No per-frame allocations in render loop
- **Draw Calls:** Minimize texture/buffer switches

### Test Scenarios

1. **Static rendering** - Single frame with all elements
2. **Scrolling** - Continuous map movement
3. **Palette effects** - Fade in/out, flash
4. **Animation** - Cursor animation, unit animations
5. **Stress test** - Maximum visible objects

---

## Deliverables

### 1. Graphics System Integration Header

**File: `src/game/graphics_system.h`**

```cpp
// src/game/graphics_system.h
// Unified graphics system initialization and management
// Task 15h - Integration & Testing

#ifndef GRAPHICS_SYSTEM_H
#define GRAPHICS_SYSTEM_H

#include "graphics_buffer.h"
#include "shape_renderer.h"
#include "tile_renderer.h"
#include "palette_manager.h"
#include "mouse_cursor.h"
#include "render_pipeline.h"
#include "viewport.h"
#include "scroll_manager.h"
#include "scroll_input.h"
#include "sidebar_render.h"
#include "radar_render.h"

// Graphics system state
enum class GraphicsState {
    UNINITIALIZED,
    INITIALIZING,
    READY,
    ERROR,
    SHUTDOWN
};

// Graphics configuration
struct GraphicsConfig {
    int screen_width;
    int screen_height;
    int window_scale;
    bool fullscreen;
    bool vsync;
    bool debug_overlay;
    int target_fps;

    GraphicsConfig()
        : screen_width(640)
        , screen_height(400)
        , window_scale(2)
        , fullscreen(false)
        , vsync(true)
        , debug_overlay(false)
        , target_fps(60) {}
};

// Performance metrics
struct GraphicsMetrics {
    float fps;
    float frame_time_ms;
    float render_time_ms;
    float flip_time_ms;
    int terrain_tiles_drawn;
    int shapes_drawn;
    int dirty_rects_processed;
    uint64_t total_frames;

    void Reset() {
        fps = 0.0f;
        frame_time_ms = 0.0f;
        render_time_ms = 0.0f;
        flip_time_ms = 0.0f;
        terrain_tiles_drawn = 0;
        shapes_drawn = 0;
        dirty_rects_processed = 0;
        total_frames = 0;
    }
};

class GraphicsSystem {
public:
    // Singleton access
    static GraphicsSystem& Instance();

    // Lifecycle
    bool Initialize(const GraphicsConfig& config);
    void Shutdown();
    bool IsInitialized() const { return state_ == GraphicsState::READY; }
    GraphicsState GetState() const { return state_; }

    // Configuration
    void SetConfig(const GraphicsConfig& config);
    const GraphicsConfig& GetConfig() const { return config_; }

    // Theater management
    bool LoadTheater(TheaterType theater);
    TheaterType GetCurrentTheater() const { return current_theater_; }

    // Per-frame operations
    void BeginFrame();
    void EndFrame();
    void ProcessInput();

    // Rendering
    void RenderFrame();
    void RenderDebugOverlay();

    // Palette effects
    void FadeIn(int frames);
    void FadeOut(int frames);
    void FlashScreen(uint8_t r, uint8_t g, uint8_t b, int frames);

    // Metrics
    const GraphicsMetrics& GetMetrics() const { return metrics_; }
    void ResetMetrics() { metrics_.Reset(); }

    // Component access (for direct manipulation if needed)
    GraphicsBuffer& GetBuffer() { return *buffer_; }
    ShapeRenderer& GetShapeRenderer() { return *shape_renderer_; }
    TileRenderer& GetTileRenderer() { return *tile_renderer_; }
    PaletteManager& GetPaletteManager() { return PaletteManager::Instance(); }
    MouseCursor& GetMouseCursor() { return MouseCursor::Instance(); }
    RenderPipeline& GetRenderPipeline() { return RenderPipeline::Instance(); }
    Viewport& GetViewport() { return Viewport::Instance(); }
    SidebarRenderer& GetSidebar() { return SidebarRenderer::Instance(); }
    RadarRenderer& GetRadar() { return RadarRenderer::Instance(); }

private:
    GraphicsSystem();
    ~GraphicsSystem();
    GraphicsSystem(const GraphicsSystem&) = delete;
    GraphicsSystem& operator=(const GraphicsSystem&) = delete;

    void UpdateMetrics();

    GraphicsState state_;
    GraphicsConfig config_;
    GraphicsMetrics metrics_;
    TheaterType current_theater_;

    // Owned components
    GraphicsBuffer* buffer_;
    ShapeRenderer* shape_renderer_;
    TileRenderer* tile_renderer_;

    // Timing
    uint64_t frame_start_time_;
    uint64_t last_fps_update_;
    int frame_count_;
};

#endif // GRAPHICS_SYSTEM_H
```

### 2. Graphics System Implementation

**File: `src/game/graphics_system.cpp`**

```cpp
// src/game/graphics_system.cpp
// Unified graphics system implementation
// Task 15h - Integration & Testing

#include "graphics_system.h"
#include "platform.h"

GraphicsSystem::GraphicsSystem()
    : state_(GraphicsState::UNINITIALIZED)
    , current_theater_(TheaterType::TEMPERATE)
    , buffer_(nullptr)
    , shape_renderer_(nullptr)
    , tile_renderer_(nullptr)
    , frame_start_time_(0)
    , last_fps_update_(0)
    , frame_count_(0) {
    metrics_.Reset();
}

GraphicsSystem::~GraphicsSystem() {
    Shutdown();
}

GraphicsSystem& GraphicsSystem::Instance() {
    static GraphicsSystem instance;
    return instance;
}

bool GraphicsSystem::Initialize(const GraphicsConfig& config) {
    if (state_ == GraphicsState::READY) {
        return true;  // Already initialized
    }

    state_ = GraphicsState::INITIALIZING;
    config_ = config;

    Platform_Log("GraphicsSystem: Initializing...");

    // Initialize platform graphics
    if (Platform_Graphics_Init() != 0) {
        Platform_Log("GraphicsSystem: Failed to initialize platform graphics");
        state_ = GraphicsState::ERROR;
        return false;
    }

    // Set display mode
    if (Platform_Graphics_SetMode(config_.screen_width, config_.screen_height, config_.window_scale) != 0) {
        Platform_Log("GraphicsSystem: Failed to set display mode");
        state_ = GraphicsState::ERROR;
        return false;
    }

    // Create components
    buffer_ = new GraphicsBuffer();
    if (!buffer_->Initialize()) {
        Platform_Log("GraphicsSystem: Failed to initialize graphics buffer");
        state_ = GraphicsState::ERROR;
        return false;
    }

    shape_renderer_ = new ShapeRenderer();
    if (!shape_renderer_->Initialize()) {
        Platform_Log("GraphicsSystem: Failed to initialize shape renderer");
        state_ = GraphicsState::ERROR;
        return false;
    }

    tile_renderer_ = new TileRenderer();
    if (!tile_renderer_->Initialize()) {
        Platform_Log("GraphicsSystem: Failed to initialize tile renderer");
        state_ = GraphicsState::ERROR;
        return false;
    }

    // Initialize singletons
    if (!PaletteManager::Instance().Initialize()) {
        Platform_Log("GraphicsSystem: Failed to initialize palette manager");
        state_ = GraphicsState::ERROR;
        return false;
    }

    if (!MouseCursor::Instance().Initialize()) {
        Platform_Log("GraphicsSystem: Failed to initialize mouse cursor");
        state_ = GraphicsState::ERROR;
        return false;
    }

    if (!RenderPipeline::Instance().Initialize(config_.screen_width, config_.screen_height)) {
        Platform_Log("GraphicsSystem: Failed to initialize render pipeline");
        state_ = GraphicsState::ERROR;
        return false;
    }

    if (!SidebarRenderer::Instance().Initialize()) {
        Platform_Log("GraphicsSystem: Failed to initialize sidebar");
        state_ = GraphicsState::ERROR;
        return false;
    }

    if (!RadarRenderer::Instance().Initialize()) {
        Platform_Log("GraphicsSystem: Failed to initialize radar");
        state_ = GraphicsState::ERROR;
        return false;
    }

    // Initialize viewport
    Viewport::Instance().Initialize();
    ScrollInputHandler::Instance().Initialize();

    // Connect components
    RenderPipeline::Instance().SetViewport(&Viewport::Instance());

    // Load default theater
    if (!LoadTheater(TheaterType::TEMPERATE)) {
        Platform_Log("GraphicsSystem: Warning - failed to load default theater");
        // Not fatal, continue anyway
    }

    // Initialize timing
    last_fps_update_ = Platform_GetTicks();
    frame_count_ = 0;

    state_ = GraphicsState::READY;
    Platform_Log("GraphicsSystem: Initialized successfully");
    return true;
}

void GraphicsSystem::Shutdown() {
    if (state_ == GraphicsState::SHUTDOWN || state_ == GraphicsState::UNINITIALIZED) {
        return;
    }

    Platform_Log("GraphicsSystem: Shutting down...");

    // Shutdown singletons (in reverse order)
    RadarRenderer::Instance().Shutdown();
    SidebarRenderer::Instance().Shutdown();
    RenderPipeline::Instance().Shutdown();
    MouseCursor::Instance().Shutdown();
    PaletteManager::Instance().Shutdown();

    // Delete owned components
    if (tile_renderer_) {
        tile_renderer_->Shutdown();
        delete tile_renderer_;
        tile_renderer_ = nullptr;
    }

    if (shape_renderer_) {
        shape_renderer_->Shutdown();
        delete shape_renderer_;
        shape_renderer_ = nullptr;
    }

    if (buffer_) {
        buffer_->Shutdown();
        delete buffer_;
        buffer_ = nullptr;
    }

    Platform_Graphics_Shutdown();

    state_ = GraphicsState::SHUTDOWN;
    Platform_Log("GraphicsSystem: Shutdown complete");
}

void GraphicsSystem::SetConfig(const GraphicsConfig& config) {
    config_ = config;

    // Apply relevant settings
    RenderConfig render_config;
    render_config.draw_debug_info = config_.debug_overlay;
    render_config.target_fps = config_.target_fps;
    RenderPipeline::Instance().SetConfig(render_config);
}

bool GraphicsSystem::LoadTheater(TheaterType theater) {
    Platform_Log("GraphicsSystem: Loading theater %d", static_cast<int>(theater));

    current_theater_ = theater;

    // Load theater palette
    if (!PaletteManager::Instance().LoadTheaterPalette(theater)) {
        Platform_Log("GraphicsSystem: Failed to load theater palette");
        return false;
    }

    // Load theater tiles
    if (!tile_renderer_->LoadTheater(theater)) {
        Platform_Log("GraphicsSystem: Failed to load theater tiles");
        return false;
    }

    // Apply palette
    PaletteManager::Instance().Apply();

    return true;
}

void GraphicsSystem::BeginFrame() {
    frame_start_time_ = Platform_GetTicks();
}

void GraphicsSystem::EndFrame() {
    UpdateMetrics();
}

void GraphicsSystem::ProcessInput() {
    Platform_PumpEvents();
    ScrollInputHandler::Instance().Update();
}

void GraphicsSystem::RenderFrame() {
    if (state_ != GraphicsState::READY) return;

    uint64_t render_start = Platform_GetTicks();

    // Main render
    RenderPipeline::Instance().RenderFrame();

    // Render UI components on top
    buffer_->Lock();
    SidebarRenderer::Instance().Render(*buffer_);
    RadarRenderer::Instance().Render(*buffer_);

    if (config_.debug_overlay) {
        RenderDebugOverlay();
    }

    buffer_->Unlock();

    uint64_t render_end = Platform_GetTicks();
    metrics_.render_time_ms = static_cast<float>(render_end - render_start);

    // Flip
    uint64_t flip_start = Platform_GetTicks();
    Platform_Graphics_Flip();
    uint64_t flip_end = Platform_GetTicks();
    metrics_.flip_time_ms = static_cast<float>(flip_end - flip_start);

    // Update palette effects
    PaletteManager::Instance().Update();
}

void GraphicsSystem::RenderDebugOverlay() {
    // Draw FPS and metrics in top-left corner
    // (Would use text rendering system - placeholder for now)

    int y = 20;
    uint8_t color = 15;  // White

    // Draw background box for readability
    buffer_->FillRect(5, y - 2, 150, 70, 0);
    buffer_->DrawRect(5, y - 2, 150, 70, color);

    // Metrics would be drawn as text here
    // For now, just draw colored bars representing performance

    // FPS bar (green if > 55, yellow if > 30, red otherwise)
    float fps_ratio = metrics_.fps / 60.0f;
    if (fps_ratio > 1.0f) fps_ratio = 1.0f;
    int bar_width = static_cast<int>(140 * fps_ratio);
    uint8_t bar_color = (metrics_.fps >= 55) ? 21 : ((metrics_.fps >= 30) ? 179 : 12);
    buffer_->FillRect(10, y + 10, bar_width, 8, bar_color);

    // Render time bar
    float render_ratio = metrics_.render_time_ms / 16.67f;
    if (render_ratio > 1.0f) render_ratio = 1.0f;
    bar_width = static_cast<int>(140 * render_ratio);
    buffer_->FillRect(10, y + 25, bar_width, 8, 119);  // Blue

    // Object count indicator
    int obj_height = (metrics_.shapes_drawn > 0) ? (metrics_.shapes_drawn / 10) : 0;
    if (obj_height > 20) obj_height = 20;
    buffer_->FillRect(10, y + 60 - obj_height, 20, obj_height, 179);  // Yellow
}

void GraphicsSystem::FadeIn(int frames) {
    PaletteManager::Instance().StartFadeIn(frames);
}

void GraphicsSystem::FadeOut(int frames) {
    PaletteManager::Instance().StartFadeOut(frames);
}

void GraphicsSystem::FlashScreen(uint8_t r, uint8_t g, uint8_t b, int frames) {
    PaletteManager::Instance().StartFlash(r, g, b, frames);
}

void GraphicsSystem::UpdateMetrics() {
    frame_count_++;
    metrics_.total_frames++;

    uint64_t current_time = Platform_GetTicks();
    metrics_.frame_time_ms = static_cast<float>(current_time - frame_start_time_);

    // Update FPS every second
    uint64_t elapsed = current_time - last_fps_update_;
    if (elapsed >= 1000) {
        metrics_.fps = (frame_count_ * 1000.0f) / elapsed;
        frame_count_ = 0;
        last_fps_update_ = current_time;
    }

    // Copy stats from render pipeline
    const RenderStats& render_stats = RenderPipeline::Instance().GetStats();
    metrics_.terrain_tiles_drawn = render_stats.terrain_tiles_drawn;
    metrics_.shapes_drawn = render_stats.objects_drawn;
    metrics_.dirty_rects_processed = render_stats.dirty_rects_count;
}
```

### 3. Integration Test Program

**File: `tests/test_graphics_integration.cpp`**

```cpp
// tests/test_graphics_integration.cpp
// Comprehensive integration test for all Phase 15 graphics components
// Task 15h - Integration & Testing

#include "graphics_system.h"
#include "platform.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>

//=============================================================================
// Test Renderable Objects
//=============================================================================

class TestUnit : public IRenderable {
public:
    int x, y;
    int frame;
    uint8_t house_color;
    bool selected;
    RenderLayer layer;

    TestUnit(int x_, int y_, uint8_t color, RenderLayer l = RenderLayer::GROUND)
        : x(x_), y(y_), frame(0), house_color(color), selected(false), layer(l) {}

    RenderLayer GetRenderLayer() const override { return layer; }
    int GetSortY() const override { return y + 24; }

    void Draw(int screen_x, int screen_y) override {
        GraphicsBuffer& buf = GraphicsSystem::Instance().GetBuffer();

        // Draw unit body (24x24 placeholder)
        buf.FillRect(screen_x, screen_y, 24, 24, house_color);
        buf.DrawRect(screen_x, screen_y, 24, 24, 0);

        // Draw facing indicator
        buf.FillRect(screen_x + 10, screen_y + 2, 4, 8, 15);

        // Draw selection box if selected
        if (selected) {
            buf.DrawRect(screen_x - 2, screen_y - 2, 28, 28, 15);
            buf.DrawRect(screen_x - 1, screen_y - 1, 26, 26, 179);

            // Health bar
            buf.FillRect(screen_x, screen_y - 5, 24, 3, 12);  // Red background
            buf.FillRect(screen_x, screen_y - 5, 20, 3, 21);  // Green fill
        }
    }

    int GetWorldX() const override { return x; }
    int GetWorldY() const override { return y; }

    void GetBounds(int& bx, int& by, int& bw, int& bh) const override {
        bx = x; by = y; bw = 24; bh = 24;
    }

    void Update() {
        frame++;
    }
};

class TestBuilding : public IRenderable {
public:
    int x, y;
    int width_cells, height_cells;
    uint8_t color;
    bool powered;

    TestBuilding(int x_, int y_, int w, int h, uint8_t c)
        : x(x_), y(y_), width_cells(w), height_cells(h), color(c), powered(true) {}

    RenderLayer GetRenderLayer() const override { return RenderLayer::BUILDING; }
    int GetSortY() const override { return y + height_cells * 24; }

    void Draw(int screen_x, int screen_y) override {
        GraphicsBuffer& buf = GraphicsSystem::Instance().GetBuffer();

        int pixel_w = width_cells * 24;
        int pixel_h = height_cells * 24;

        // Building body
        buf.FillRect(screen_x, screen_y, pixel_w, pixel_h, color);
        buf.DrawRect(screen_x, screen_y, pixel_w, pixel_h, 0);

        // Bib/foundation
        buf.FillRect(screen_x, screen_y + pixel_h - 4, pixel_w, 4, 24);

        // Power indicator
        uint8_t power_color = powered ? 21 : 12;
        buf.FillRect(screen_x + 2, screen_y + 2, 6, 6, power_color);
    }

    int GetWorldX() const override { return x; }
    int GetWorldY() const override { return y; }

    void GetBounds(int& bx, int& by, int& bw, int& bh) const override {
        bx = x; by = y; bw = width_cells * 24; bh = height_cells * 24;
    }
};

//=============================================================================
// Unit Tests
//=============================================================================

bool Test_SystemInitialization() {
    printf("Test: System Initialization... ");

    GraphicsConfig config;
    config.screen_width = 640;
    config.screen_height = 400;
    config.window_scale = 2;
    config.debug_overlay = true;

    if (!GraphicsSystem::Instance().Initialize(config)) {
        printf("FAILED - Could not initialize\n");
        return false;
    }

    if (!GraphicsSystem::Instance().IsInitialized()) {
        printf("FAILED - Not marked as initialized\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_TheaterLoading() {
    printf("Test: Theater Loading... ");

    // Test temperate
    if (!GraphicsSystem::Instance().LoadTheater(TheaterType::TEMPERATE)) {
        printf("FAILED - Could not load TEMPERATE\n");
        return false;
    }

    if (GraphicsSystem::Instance().GetCurrentTheater() != TheaterType::TEMPERATE) {
        printf("FAILED - Theater not set correctly\n");
        return false;
    }

    // Test snow
    if (!GraphicsSystem::Instance().LoadTheater(TheaterType::SNOW)) {
        printf("FAILED - Could not load SNOW\n");
        return false;
    }

    // Back to temperate for remaining tests
    GraphicsSystem::Instance().LoadTheater(TheaterType::TEMPERATE);

    printf("PASSED\n");
    return true;
}

bool Test_ViewportIntegration() {
    printf("Test: Viewport Integration... ");

    Viewport& vp = GraphicsSystem::Instance().GetViewport();
    vp.SetMapSize(64, 64);
    vp.ScrollTo(100, 100);

    if (vp.x != 100 || vp.y != 100) {
        printf("FAILED - Viewport position incorrect\n");
        return false;
    }

    // Test coordinate conversion through system
    int screen_x, screen_y;
    vp.WorldToScreen(200, 150, screen_x, screen_y);

    int world_x, world_y;
    vp.ScreenToWorld(screen_x, screen_y, world_x, world_y);

    if (world_x != 200 || world_y != 150) {
        printf("FAILED - Round-trip conversion failed\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_ObjectRendering() {
    printf("Test: Object Rendering... ");

    RenderPipeline& pipeline = GraphicsSystem::Instance().GetRenderPipeline();
    pipeline.ClearRenderQueue();

    // Create test objects
    TestUnit unit1(100, 100, 120);  // Green unit
    TestUnit unit2(150, 80, 127);   // Red unit
    TestBuilding building(200, 200, 3, 2, 15);

    // Register objects
    pipeline.RegisterObject(&unit1);
    pipeline.RegisterObject(&unit2);
    pipeline.RegisterObject(&building);

    // Render a frame (shouldn't crash)
    GraphicsSystem::Instance().BeginFrame();
    GraphicsSystem::Instance().RenderFrame();
    GraphicsSystem::Instance().EndFrame();

    // Check metrics
    const GraphicsMetrics& metrics = GraphicsSystem::Instance().GetMetrics();
    if (metrics.shapes_drawn < 3) {
        printf("FAILED - Expected at least 3 objects drawn, got %d\n", metrics.shapes_drawn);
        return false;
    }

    // Cleanup
    pipeline.UnregisterObject(&unit1);
    pipeline.UnregisterObject(&unit2);
    pipeline.UnregisterObject(&building);

    printf("PASSED\n");
    return true;
}

bool Test_PaletteEffects() {
    printf("Test: Palette Effects... ");

    PaletteManager& palette = GraphicsSystem::Instance().GetPaletteManager();

    // Test fade out
    GraphicsSystem::Instance().FadeOut(5);

    for (int i = 0; i < 5; i++) {
        palette.Update();
    }

    if (palette.IsFading()) {
        printf("FAILED - Fade should be complete\n");
        return false;
    }

    // Test fade in
    GraphicsSystem::Instance().FadeIn(5);

    for (int i = 0; i < 5; i++) {
        palette.Update();
    }

    // Test flash
    GraphicsSystem::Instance().FlashScreen(255, 255, 255, 3);

    for (int i = 0; i < 3; i++) {
        palette.Update();
    }

    printf("PASSED\n");
    return true;
}

bool Test_ScrollingIntegration() {
    printf("Test: Scrolling Integration... ");

    Viewport& vp = GraphicsSystem::Instance().GetViewport();
    vp.SetMapSize(128, 128);
    vp.ScrollTo(0, 0);

    // Test smooth scroll
    ScrollManager::Instance().CenterOnCell(32, 32, ScrollAnimationType::EASE_OUT, 10);

    for (int i = 0; i < 10; i++) {
        ScrollManager::Instance().Update();
    }

    // Should be approximately centered on cell 32,32
    int expected_x = 32 * 24 - vp.width / 2;
    int expected_y = 32 * 24 - vp.height / 2;

    if (abs(vp.x - expected_x) > 5 || abs(vp.y - expected_y) > 5) {
        printf("FAILED - Scroll didn't reach target\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_SidebarRadarIntegration() {
    printf("Test: Sidebar/Radar Integration... ");

    SidebarRenderer& sidebar = GraphicsSystem::Instance().GetSidebar();
    RadarRenderer& radar = GraphicsSystem::Instance().GetRadar();

    // Set sidebar state
    sidebar.SetCredits(5000);
    sidebar.SetPower(150, 200);

    // Set radar state
    radar.SetMapSize(64, 64);
    radar.SetActive(true);
    radar.SetViewportRect(10, 10, 20, 16);
    radar.AddBlip(20, 20, RADAR_FRIENDLY);
    radar.AddBlip(40, 30, RADAR_ENEMY);

    // Render a frame
    GraphicsSystem::Instance().BeginFrame();
    GraphicsSystem::Instance().RenderFrame();
    GraphicsSystem::Instance().EndFrame();

    printf("PASSED\n");
    return true;
}

bool Test_PerformanceMetrics() {
    printf("Test: Performance Metrics... ");

    GraphicsSystem::Instance().ResetMetrics();

    // Render several frames
    for (int i = 0; i < 60; i++) {
        GraphicsSystem::Instance().BeginFrame();
        GraphicsSystem::Instance().RenderFrame();
        GraphicsSystem::Instance().EndFrame();
        Platform_Timer_Delay(16);
    }

    const GraphicsMetrics& metrics = GraphicsSystem::Instance().GetMetrics();

    if (metrics.total_frames < 60) {
        printf("FAILED - Frame count mismatch\n");
        return false;
    }

    if (metrics.fps < 1.0f) {
        printf("FAILED - FPS not calculated\n");
        return false;
    }

    printf("PASSED (%.1f FPS)\n", metrics.fps);
    return true;
}

//=============================================================================
// Stress Test
//=============================================================================

bool Test_StressTest() {
    printf("Test: Stress Test (500 objects)... ");

    RenderPipeline& pipeline = GraphicsSystem::Instance().GetRenderPipeline();
    pipeline.ClearRenderQueue();

    // Create many objects
    std::vector<TestUnit*> units;
    std::vector<TestBuilding*> buildings;

    for (int i = 0; i < 400; i++) {
        int x = (i % 50) * 48;
        int y = (i / 50) * 48;
        uint8_t color = (i % 2) ? 120 : 127;
        TestUnit* unit = new TestUnit(x, y, color);
        units.push_back(unit);
        pipeline.RegisterObject(unit);
    }

    for (int i = 0; i < 100; i++) {
        int x = (i % 20) * 72;
        int y = (i / 20) * 72 + 400;
        TestBuilding* building = new TestBuilding(x, y, 2, 2, 15);
        buildings.push_back(building);
        pipeline.RegisterObject(building);
    }

    // Set viewport to see many objects
    Viewport::Instance().SetMapSize(128, 128);
    Viewport::Instance().ScrollTo(0, 0);

    // Render several frames and measure
    GraphicsSystem::Instance().ResetMetrics();
    uint64_t start = Platform_GetTicks();

    for (int i = 0; i < 60; i++) {
        GraphicsSystem::Instance().BeginFrame();
        GraphicsSystem::Instance().RenderFrame();
        GraphicsSystem::Instance().EndFrame();
    }

    uint64_t elapsed = Platform_GetTicks() - start;
    float avg_frame_time = static_cast<float>(elapsed) / 60.0f;

    // Cleanup
    for (TestUnit* unit : units) {
        pipeline.UnregisterObject(unit);
        delete unit;
    }
    for (TestBuilding* building : buildings) {
        pipeline.UnregisterObject(building);
        delete building;
    }

    // Should maintain reasonable performance
    if (avg_frame_time > 50.0f) {  // More than 50ms per frame is too slow
        printf("FAILED - Too slow (%.1f ms/frame)\n", avg_frame_time);
        return false;
    }

    printf("PASSED (%.1f ms/frame avg)\n", avg_frame_time);
    return true;
}

//=============================================================================
// Interactive Demo
//=============================================================================

void Interactive_Demo() {
    printf("\n=== Interactive Graphics Demo ===\n");
    printf("Controls:\n");
    printf("  Arrow keys / Mouse edge: Scroll map\n");
    printf("  1-3: Switch theaters (Temperate/Snow/Interior)\n");
    printf("  F: Fade out then in\n");
    printf("  W: White flash\n");
    printf("  R: Red flash\n");
    printf("  D: Toggle debug overlay\n");
    printf("  Click: Select units\n");
    printf("  ESC: Exit\n\n");

    // Set up viewport
    Viewport& vp = GraphicsSystem::Instance().GetViewport();
    vp.SetMapSize(128, 128);
    vp.ScrollTo(0, 0);

    // Set up radar
    RadarRenderer& radar = GraphicsSystem::Instance().GetRadar();
    radar.SetMapSize(128, 128);
    radar.SetActive(true);

    // Create terrain data for radar
    for (int y = 0; y < 128; y++) {
        for (int x = 0; x < 128; x++) {
            // Create varied terrain
            uint8_t terrain;
            if ((x > 40 && x < 60) && (y > 30 && y < 50)) {
                terrain = RADAR_TERRAIN_WATER;
            } else if ((x + y) % 10 < 2) {
                terrain = RADAR_TERRAIN_ROCK;
            } else {
                terrain = RADAR_TERRAIN_CLEAR;
            }
            radar.SetTerrainCell(x, y, terrain);
            radar.SetShroudCell(x, y, false);
        }
    }

    // Create demo objects
    std::vector<TestUnit*> units;
    std::vector<TestBuilding*> buildings;
    RenderPipeline& pipeline = GraphicsSystem::Instance().GetRenderPipeline();

    // Player units (green)
    for (int i = 0; i < 20; i++) {
        int x = 100 + (i % 5) * 30;
        int y = 100 + (i / 5) * 30;
        TestUnit* unit = new TestUnit(x, y, 120);
        units.push_back(unit);
        pipeline.RegisterObject(unit);
    }

    // Enemy units (red)
    for (int i = 0; i < 15; i++) {
        int x = 800 + (i % 5) * 30;
        int y = 400 + (i / 5) * 30;
        TestUnit* unit = new TestUnit(x, y, 127);
        units.push_back(unit);
        pipeline.RegisterObject(unit);
    }

    // Player buildings
    buildings.push_back(new TestBuilding(50, 50, 3, 3, 120));   // Construction Yard
    buildings.push_back(new TestBuilding(150, 50, 2, 2, 120));  // Power Plant
    buildings.push_back(new TestBuilding(50, 150, 3, 2, 120));  // Barracks

    // Enemy buildings
    buildings.push_back(new TestBuilding(850, 350, 3, 3, 127));
    buildings.push_back(new TestBuilding(950, 350, 2, 2, 127));

    for (TestBuilding* b : buildings) {
        pipeline.RegisterObject(b);
    }

    // Set sidebar
    SidebarRenderer& sidebar = GraphicsSystem::Instance().GetSidebar();
    sidebar.SetCredits(5000);
    sidebar.SetPower(150, 200);

    // Main loop
    bool running = true;
    int selected_unit = -1;
    bool debug_enabled = true;

    while (running) {
        GraphicsSystem::Instance().BeginFrame();

        // Process input
        Platform_PumpEvents();
        ScrollInputHandler::Instance().Update();

        // Handle key presses
        if (Platform_Key_Down(KEY_ESCAPE)) {
            running = false;
        }

        // Theater switching
        static bool key_1_pressed = false;
        if (Platform_Key_Down(KEY_1) && !key_1_pressed) {
            GraphicsSystem::Instance().LoadTheater(TheaterType::TEMPERATE);
            key_1_pressed = true;
        }
        if (!Platform_Key_Down(KEY_1)) key_1_pressed = false;

        static bool key_2_pressed = false;
        if (Platform_Key_Down(KEY_2) && !key_2_pressed) {
            GraphicsSystem::Instance().LoadTheater(TheaterType::SNOW);
            key_2_pressed = true;
        }
        if (!Platform_Key_Down(KEY_2)) key_2_pressed = false;

        static bool key_3_pressed = false;
        if (Platform_Key_Down(KEY_3) && !key_3_pressed) {
            GraphicsSystem::Instance().LoadTheater(TheaterType::INTERIOR);
            key_3_pressed = true;
        }
        if (!Platform_Key_Down(KEY_3)) key_3_pressed = false;

        // Effects
        static bool key_f_pressed = false;
        if (Platform_Key_Down(KEY_F) && !key_f_pressed) {
            GraphicsSystem::Instance().FadeOut(30);
            key_f_pressed = true;
        }
        if (!Platform_Key_Down(KEY_F)) {
            key_f_pressed = false;
            // Check if fade out is complete, then fade in
            if (!PaletteManager::Instance().IsFading()) {
                // Start fade in if we just finished a fade out
            }
        }

        static bool key_w_pressed = false;
        if (Platform_Key_Down(KEY_W) && !key_w_pressed) {
            GraphicsSystem::Instance().FlashScreen(255, 255, 255, 10);
            key_w_pressed = true;
        }
        if (!Platform_Key_Down(KEY_W)) key_w_pressed = false;

        static bool key_r_pressed = false;
        if (Platform_Key_Down(KEY_R) && !key_r_pressed) {
            GraphicsSystem::Instance().FlashScreen(255, 0, 0, 10);
            key_r_pressed = true;
        }
        if (!Platform_Key_Down(KEY_R)) key_r_pressed = false;

        static bool key_d_pressed = false;
        if (Platform_Key_Down(KEY_D) && !key_d_pressed) {
            debug_enabled = !debug_enabled;
            GraphicsConfig config = GraphicsSystem::Instance().GetConfig();
            config.debug_overlay = debug_enabled;
            GraphicsSystem::Instance().SetConfig(config);
            key_d_pressed = true;
        }
        if (!Platform_Key_Down(KEY_D)) key_d_pressed = false;

        // Handle mouse click for unit selection
        int mouse_x, mouse_y;
        Platform_Mouse_GetPosition(&mouse_x, &mouse_y);
        mouse_x /= 2;
        mouse_y /= 2;

        static bool mouse_clicked = false;
        if (Platform_Mouse_Button(0)) {
            if (!mouse_clicked) {
                // Check if clicking on a unit
                int world_x, world_y;
                vp.ScreenToWorld(mouse_x, mouse_y, world_x, world_y);

                // Deselect all
                for (TestUnit* u : units) {
                    u->selected = false;
                }
                selected_unit = -1;

                // Check for unit under cursor
                for (size_t i = 0; i < units.size(); i++) {
                    if (world_x >= units[i]->x && world_x < units[i]->x + 24 &&
                        world_y >= units[i]->y && world_y < units[i]->y + 24) {
                        units[i]->selected = true;
                        selected_unit = static_cast<int>(i);
                        break;
                    }
                }

                // Check for radar click
                int cell_x, cell_y;
                if (radar.GetCellAtRadar(mouse_x, mouse_y, cell_x, cell_y)) {
                    ScrollManager::Instance().CenterOnCell(cell_x, cell_y, ScrollAnimationType::EASE_OUT, 15);
                }

                mouse_clicked = true;
            }
        } else {
            mouse_clicked = false;
        }

        // Update radar viewport
        int cells_x = vp.x / 24;
        int cells_y = vp.y / 24;
        int cells_w = vp.width / 24;
        int cells_h = vp.height / 24;
        radar.SetViewportRect(cells_x, cells_y, cells_w, cells_h);

        // Update radar blips
        radar.ClearBlips();
        for (const TestUnit* u : units) {
            int cx = u->x / 24;
            int cy = u->y / 24;
            radar.AddBlip(cx, cy, (u->house_color == 120) ? RADAR_FRIENDLY : RADAR_ENEMY);
        }
        for (const TestBuilding* b : buildings) {
            int cx = b->x / 24;
            int cy = b->y / 24;
            radar.AddBlip(cx, cy, (b->color == 120) ? RADAR_FRIENDLY : RADAR_ENEMY);
        }

        // Render
        GraphicsSystem::Instance().RenderFrame();
        GraphicsSystem::Instance().EndFrame();

        // Frame delay
        Platform_Timer_Delay(16);
    }

    // Cleanup
    for (TestUnit* unit : units) {
        pipeline.UnregisterObject(unit);
        delete unit;
    }
    for (TestBuilding* building : buildings) {
        pipeline.UnregisterObject(building);
        delete building;
    }
}

//=============================================================================
// Main
//=============================================================================

int main(int argc, char* argv[]) {
    printf("=== Graphics Integration Tests (Phase 15h) ===\n\n");

    int passed = 0;
    int failed = 0;

    // Initialize platform
    Platform_Init();

    // Run unit tests
    if (Test_SystemInitialization()) passed++; else failed++;
    if (Test_TheaterLoading()) passed++; else failed++;
    if (Test_ViewportIntegration()) passed++; else failed++;
    if (Test_ObjectRendering()) passed++; else failed++;
    if (Test_PaletteEffects()) passed++; else failed++;
    if (Test_ScrollingIntegration()) passed++; else failed++;
    if (Test_SidebarRadarIntegration()) passed++; else failed++;
    if (Test_PerformanceMetrics()) passed++; else failed++;
    if (Test_StressTest()) passed++; else failed++;

    printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);

    // Interactive demo
    if (argc > 1 && strcmp(argv[1], "-i") == 0) {
        Interactive_Demo();
    } else {
        printf("\nRun with -i for interactive demo\n");
    }

    // Cleanup
    GraphicsSystem::Instance().Shutdown();
    Platform_Shutdown();

    return (failed == 0) ? 0 : 1;
}
```

---

## CMakeLists.txt Additions

Add to `src/game/CMakeLists.txt`:

```cmake
# Graphics System Integration (Task 15h)
target_sources(game PRIVATE
    graphics_system.h
    graphics_system.cpp
)
```

Add to `tests/CMakeLists.txt`:

```cmake
# Graphics Integration test (Task 15h)
add_executable(test_graphics_integration test_graphics_integration.cpp)
target_link_libraries(test_graphics_integration PRIVATE game platform)
add_test(NAME GraphicsIntegrationTest COMMAND test_graphics_integration)
```

---

## Implementation Steps

1. **Create graphics_system.h** - Define unified GraphicsSystem class
2. **Create graphics_system.cpp** - Implement initialization, shutdown, and frame management
3. **Create test_graphics_integration.cpp** - Comprehensive integration tests and demo
4. **Update CMakeLists.txt** - Add new source files and test
5. **Build all Phase 15 components** - Ensure everything compiles together
6. **Run unit tests** - Verify all integration tests pass
7. **Run interactive demo** - Verify visual output and performance
8. **Profile and optimize** - Ensure 60 FPS target is met

---

## Success Criteria

- [ ] GraphicsSystem initializes all subsystems successfully
- [ ] All subsystems shut down cleanly without leaks
- [ ] Theater loading works for all three theater types
- [ ] Viewport and scrolling work correctly
- [ ] Objects render with proper layer ordering
- [ ] Palette effects (fade/flash) work
- [ ] Sidebar and radar integrate properly
- [ ] Performance metrics are accurate
- [ ] Stress test with 500 objects maintains acceptable FPS
- [ ] All unit tests pass
- [ ] Interactive demo shows complete game-like graphics

---

## Common Issues

1. **Initialization order problems**
   - Platform must init before graphics components
   - Components that depend on others must init after dependencies
   - Use GraphicsSystem to manage initialization order

2. **Shutdown crashes**
   - Shutdown in reverse order of initialization
   - Clear references before deleting objects
   - Check for null pointers

3. **Performance issues**
   - Profile with metrics to find bottleneck
   - Check for per-frame allocations
   - Verify visibility culling is working

4. **Visual glitches**
   - Check layer ordering
   - Verify coordinate conversion
   - Ensure double buffering (Lock/Unlock/Flip)

5. **Memory leaks**
   - Ensure all registered objects are unregistered
   - Delete all allocated objects
   - Use ASAN/Valgrind to verify

---

## Verification Script

```bash
#!/bin/bash
set -e

echo "=== Task 15h Verification ==="
echo "=== Phase 15 Complete Integration Test ==="

# Build
echo "Building all Phase 15 components..."
cd build
cmake --build .

# Run all Phase 15 tests
echo ""
echo "Running individual component tests..."

echo "  15a - Graphics Buffer..."
./tests/test_graphics_buffer

echo "  15b - Shape Renderer..."
./tests/test_shape_renderer

echo "  15c - Tile Renderer..."
./tests/test_tile_renderer

echo "  15d - Palette Manager..."
./tests/test_palette_manager

echo "  15e - Mouse Cursor..."
./tests/test_mouse_cursor

echo "  15f - Render Pipeline..."
./tests/test_render_pipeline

echo "  15g - Viewport..."
./tests/test_viewport

echo ""
echo "Running integration test..."
./tests/test_graphics_integration

# Check all source files exist
echo ""
echo "Checking Phase 15 source files..."
FILES=(
    # 15a
    "../src/game/graphics_buffer.h"
    "../src/game/graphics_buffer.cpp"
    # 15b
    "../src/game/shape_renderer.h"
    "../src/game/shape_renderer.cpp"
    "../src/game/remap_tables.h"
    "../src/game/remap_tables.cpp"
    # 15c
    "../src/game/tile_renderer.h"
    "../src/game/tile_renderer.cpp"
    # 15d
    "../src/game/palette_manager.h"
    "../src/game/palette_manager.cpp"
    # 15e
    "../src/game/mouse_cursor.h"
    "../src/game/mouse_cursor.cpp"
    # 15f
    "../src/game/render_layer.h"
    "../src/game/render_pipeline.h"
    "../src/game/render_pipeline.cpp"
    "../src/game/sidebar_render.h"
    "../src/game/sidebar_render.cpp"
    "../src/game/radar_render.h"
    "../src/game/radar_render.cpp"
    # 15g
    "../src/game/viewport.h"
    "../src/game/viewport.cpp"
    "../src/game/scroll_manager.h"
    "../src/game/scroll_manager.cpp"
    "../src/game/scroll_input.h"
    "../src/game/scroll_input.cpp"
    # 15h
    "../src/game/graphics_system.h"
    "../src/game/graphics_system.cpp"
)

for file in "${FILES[@]}"; do
    if [ ! -f "$file" ]; then
        echo "ERROR: Missing $file"
        exit 1
    fi
    echo "  Found: $file"
done

echo ""
echo "============================================"
echo "=== PHASE 15 VERIFICATION COMPLETE ==="
echo "============================================"
echo ""
echo "All tests passed! Graphics Integration is complete."
echo ""
echo "Interactive demos available:"
echo "  ./tests/test_graphics_buffer -i"
echo "  ./tests/test_render_pipeline -i"
echo "  ./tests/test_viewport -i"
echo "  ./tests/test_graphics_integration -i"
```

---

## Completion Promise

When this task is complete:
1. Unified GraphicsSystem managing all graphics subsystems
2. Complete initialization and shutdown sequence
3. Comprehensive integration tests covering all components
4. Performance monitoring and metrics
5. Stress test verifying scalability
6. Interactive demo showing full game-like graphics
7. All Phase 15 components working together seamlessly
8. Documentation of integration points
9. Ready for Phase 16 (Audio) or further game development

When verification passes, output:
<promise>TASK_15H_COMPLETE</promise>

## Phase 15 Summary

Upon completion of Task 15h, Phase 15 delivers:

| Task | Component | Lines | Status |
|------|-----------|-------|--------|
| 15a | GraphicsBuffer | ~500 | ✓ |
| 15b | ShapeRenderer | ~1,200 | ✓ |
| 15c | TileRenderer | ~800 | ✓ |
| 15d | PaletteManager | ~900 | ✓ |
| 15e | MouseCursor | ~700 | ✓ |
| 15f | RenderPipeline | ~1,400 | ✓ |
| 15g | Viewport | ~800 | ✓ |
| 15h | Integration | ~1,000 | ✓ |

**Total: ~7,300 lines of graphics code**

## Escape Hatch

If you get stuck:
1. Test each component in isolation first
2. Add verbose logging to track initialization order
3. Use simpler test objects (colored rectangles) before full shapes
4. Skip stress test if performance is blocking
5. Focus on getting basic rendering working before polish

If blocked after max iterations, document blockers and output:
<promise>TASK_15H_BLOCKED</promise>

## Max Iterations
6
