# Task 15f: Render Pipeline

| Field | Value |
|-------|-------|
| **Task ID** | 15f |
| **Task Name** | Render Pipeline |
| **Phase** | 15 - Graphics Integration |
| **Depends On** | 15a (GraphicsBuffer), 15b (Shape Drawing), 15c (Tile Rendering), 15d (Palette Management), 15e (Mouse Cursor) |
| **Estimated Complexity** | Very High (~1,400+ lines) |

---

## Context

The render pipeline is the central orchestration system that brings all graphics components together. It must draw the game world in the correct order (terrain first, then ground units, buildings, air units, effects, and finally UI) while maintaining 60 FPS performance. The original C&C used a sophisticated layered rendering approach that respected depth sorting and occlusion.

The pipeline must handle:
1. **Layer ordering** - Objects must be drawn back-to-front
2. **Clipping** - Only visible portions of the viewport should render
3. **Dirty rectangles** - Optimization to only redraw changed areas
4. **Double buffering** - Draw to back buffer, then flip to avoid tearing

---

## Objective

Create a complete render pipeline that integrates all previously built graphics components (GraphicsBuffer, ShapeRenderer, TileRenderer, PaletteManager, MouseCursor) into a cohesive frame rendering system.

---

## Technical Background

### Rendering Layer Order (Back to Front)

The game renders in strict layer order:

```
1. Terrain tiles (background layer)
2. Smudges (craters, scorch marks on terrain)
3. Overlays (ore, gems, walls, fences)
4. Building foundations/bibs
5. Ground shadows
6. Infantry (sorted by Y coordinate)
7. Vehicles (sorted by Y coordinate)
8. Buildings (sorted by Y coordinate)
9. Air units and their shadows
10. Projectiles and effects
11. Selection boxes
12. Shroud/fog of war
13. Sidebar and UI
14. Mouse cursor (topmost)
```

### Coordinate Systems

The game uses multiple coordinate systems:
- **Cell coordinates** - Map grid (e.g., cell 50,30)
- **Lepton coordinates** - Sub-cell precision (256 leptons = 1 cell)
- **Pixel coordinates** - Screen position
- **World coordinates** - Cell * 24 (tile size in pixels)

### Y-Sorting for Depth

Objects on the ground layer are sorted by their Y coordinate (plus height offset) so that objects further "up" on the screen appear behind objects lower down, creating proper depth perception.

### Dirty Rectangle Optimization

Rather than redrawing the entire screen every frame, C&C tracked "dirty rectangles" - areas that changed and needed redrawing. This was critical for 1996 hardware but is optional for modern systems. We'll implement it as an optimization flag.

---

## Deliverables

### 1. Render Layer Definitions

**File: `src/game/render_layer.h`**

```cpp
// src/game/render_layer.h
// Render layer definitions for draw ordering
// Task 15f - Render Pipeline

#ifndef RENDER_LAYER_H
#define RENDER_LAYER_H

#include <cstdint>
#include <vector>
#include <algorithm>

// Layer identifiers for rendering order
enum class RenderLayer : int {
    TERRAIN = 0,      // Base terrain tiles
    SMUDGE = 1,       // Craters, scorch marks
    OVERLAY = 2,      // Ore, gems, walls
    BIB = 3,          // Building foundation
    SHADOW = 4,       // Ground shadows
    GROUND = 5,       // Infantry, vehicles on ground
    BUILDING = 6,     // Structures
    AIR = 7,          // Aircraft, parachutes
    PROJECTILE = 8,   // Bullets, missiles, explosions
    EFFECT = 9,       // Visual effects (smoke, fire)
    SELECTION = 10,   // Selection boxes
    SHROUD = 11,      // Fog of war
    UI = 12,          // Sidebar, radar, etc.
    CURSOR = 13,      // Mouse cursor (topmost)

    COUNT = 14
};

// Forward declarations
class ObjectClass;
class CellClass;

// Renderable interface - any object that can be drawn
class IRenderable {
public:
    virtual ~IRenderable() = default;

    // Get the layer this object belongs to
    virtual RenderLayer GetRenderLayer() const = 0;

    // Get Y coordinate for depth sorting (higher Y = drawn later)
    virtual int GetSortY() const = 0;

    // Draw the object at screen coordinates
    virtual void Draw(int screen_x, int screen_y) = 0;

    // Get world coordinates for visibility testing
    virtual int GetWorldX() const = 0;
    virtual int GetWorldY() const = 0;

    // Get bounding box for dirty rect tracking
    virtual void GetBounds(int& x, int& y, int& width, int& height) const = 0;
};

// Dirty rectangle for partial screen updates
struct DirtyRect {
    int x, y;
    int width, height;

    DirtyRect() : x(0), y(0), width(0), height(0) {}
    DirtyRect(int x_, int y_, int w, int h) : x(x_), y(y_), width(w), height(h) {}

    // Check if two rects overlap
    bool Overlaps(const DirtyRect& other) const {
        return !(x + width <= other.x || other.x + other.width <= x ||
                 y + height <= other.y || other.y + other.height <= y);
    }

    // Merge with another rect (union)
    void Merge(const DirtyRect& other) {
        int new_x = (x < other.x) ? x : other.x;
        int new_y = (y < other.y) ? y : other.y;
        int new_right = (x + width > other.x + other.width) ? x + width : other.x + other.width;
        int new_bottom = (y + height > other.y + other.height) ? y + height : other.y + other.height;
        x = new_x;
        y = new_y;
        width = new_right - x;
        height = new_bottom - y;
    }

    // Check if empty
    bool IsEmpty() const { return width <= 0 || height <= 0; }
};

// Render queue entry for sorting
struct RenderEntry {
    IRenderable* object;
    int sort_y;
    RenderLayer layer;

    RenderEntry(IRenderable* obj, int y, RenderLayer lay)
        : object(obj), sort_y(y), layer(lay) {}

    // Sort by layer first, then by Y coordinate
    bool operator<(const RenderEntry& other) const {
        if (layer != other.layer) {
            return static_cast<int>(layer) < static_cast<int>(other.layer);
        }
        return sort_y < other.sort_y;
    }
};

// Render statistics for performance monitoring
struct RenderStats {
    int terrain_tiles_drawn;
    int objects_drawn;
    int dirty_rects_count;
    int pixels_filled;
    float frame_time_ms;

    void Reset() {
        terrain_tiles_drawn = 0;
        objects_drawn = 0;
        dirty_rects_count = 0;
        pixels_filled = 0;
        frame_time_ms = 0.0f;
    }
};

#endif // RENDER_LAYER_H
```

### 2. Main Render Pipeline

**File: `src/game/render_pipeline.h`**

```cpp
// src/game/render_pipeline.h
// Main render pipeline orchestration
// Task 15f - Render Pipeline

#ifndef RENDER_PIPELINE_H
#define RENDER_PIPELINE_H

#include "render_layer.h"
#include "graphics_buffer.h"
#include "shape_renderer.h"
#include "tile_renderer.h"
#include "palette_manager.h"
#include "mouse_cursor.h"
#include <vector>
#include <functional>

// Forward declarations
class Viewport;

// Render pipeline configuration
struct RenderConfig {
    bool enable_dirty_rects;      // Use dirty rectangle optimization
    bool enable_shadows;          // Draw unit shadows
    bool enable_shroud;           // Draw fog of war
    bool draw_debug_info;         // Show debug overlays
    bool draw_cell_grid;          // Show cell grid overlay
    bool draw_pathfinding;        // Show pathfinding debug
    int target_fps;               // Target frame rate

    RenderConfig()
        : enable_dirty_rects(false)  // Disable by default on modern hardware
        , enable_shadows(true)
        , enable_shroud(true)
        , draw_debug_info(false)
        , draw_cell_grid(false)
        , draw_pathfinding(false)
        , target_fps(60) {}
};

class RenderPipeline {
public:
    // Singleton access
    static RenderPipeline& Instance();

    // Initialization
    bool Initialize(int screen_width, int screen_height);
    void Shutdown();
    bool IsInitialized() const { return initialized_; }

    // Configuration
    void SetConfig(const RenderConfig& config) { config_ = config; }
    RenderConfig& GetConfig() { return config_; }
    const RenderConfig& GetConfig() const { return config_; }

    // Viewport management
    void SetViewport(Viewport* viewport) { viewport_ = viewport; }
    Viewport* GetViewport() { return viewport_; }

    // Main render entry point
    void RenderFrame();

    // Layer-specific rendering (can be called individually for debugging)
    void RenderTerrain();
    void RenderSmudges();
    void RenderOverlays();
    void RenderBibs();
    void RenderShadows();
    void RenderGroundObjects();
    void RenderBuildings();
    void RenderAirObjects();
    void RenderProjectiles();
    void RenderEffects();
    void RenderSelectionBoxes();
    void RenderShroud();
    void RenderUI();
    void RenderCursor();

    // Debug rendering
    void RenderCellGrid();
    void RenderDebugInfo();

    // Dirty rectangle management
    void AddDirtyRect(const DirtyRect& rect);
    void AddDirtyRect(int x, int y, int width, int height);
    void MarkFullRedraw();
    void ClearDirtyRects();

    // Object registration for rendering
    void RegisterObject(IRenderable* object);
    void UnregisterObject(IRenderable* object);
    void ClearRenderQueue();

    // Statistics
    const RenderStats& GetStats() const { return stats_; }
    void ResetStats() { stats_.Reset(); }

    // Utility functions
    bool IsVisible(int world_x, int world_y, int width, int height) const;
    void WorldToScreen(int world_x, int world_y, int& screen_x, int& screen_y) const;
    void ScreenToWorld(int screen_x, int screen_y, int& world_x, int& world_y) const;

    // Access to subsystems
    GraphicsBuffer& GetBuffer() { return buffer_; }
    ShapeRenderer& GetShapeRenderer() { return shape_renderer_; }
    TileRenderer& GetTileRenderer() { return tile_renderer_; }

private:
    RenderPipeline();
    ~RenderPipeline();
    RenderPipeline(const RenderPipeline&) = delete;
    RenderPipeline& operator=(const RenderPipeline&) = delete;

    // Internal rendering helpers
    void BeginFrame();
    void EndFrame();
    void SortRenderQueue();
    void ProcessDirtyRects();
    void DrawRect(const DirtyRect& rect, uint8_t color);

    // State
    bool initialized_;
    int screen_width_;
    int screen_height_;
    RenderConfig config_;
    RenderStats stats_;

    // Components
    GraphicsBuffer buffer_;
    ShapeRenderer shape_renderer_;
    TileRenderer tile_renderer_;
    Viewport* viewport_;

    // Render queue
    std::vector<RenderEntry> render_queue_;
    std::vector<IRenderable*> registered_objects_;

    // Dirty rectangles
    std::vector<DirtyRect> dirty_rects_;
    std::vector<DirtyRect> merged_dirty_rects_;
    bool full_redraw_needed_;

    // Timing
    uint64_t last_frame_time_;
    float frame_delta_;
};

#endif // RENDER_PIPELINE_H
```

### 3. Render Pipeline Implementation

**File: `src/game/render_pipeline.cpp`**

```cpp
// src/game/render_pipeline.cpp
// Main render pipeline implementation
// Task 15f - Render Pipeline

#include "render_pipeline.h"
#include "viewport.h"
#include "platform.h"
#include <algorithm>
#include <cstring>

// Screen dimensions
static const int SIDEBAR_WIDTH = 160;
static const int TAB_HEIGHT = 16;
static const int TACTICAL_WIDTH = 480;  // 640 - SIDEBAR_WIDTH
static const int TACTICAL_HEIGHT = 384; // 400 - TAB_HEIGHT

// Color indices for debug drawing
static const uint8_t DEBUG_GRID_COLOR = 14;      // Yellow
static const uint8_t DEBUG_BOUNDS_COLOR = 13;    // Cyan
static const uint8_t DEBUG_DIRTY_COLOR = 12;     // Red

RenderPipeline::RenderPipeline()
    : initialized_(false)
    , screen_width_(640)
    , screen_height_(400)
    , viewport_(nullptr)
    , full_redraw_needed_(true)
    , last_frame_time_(0)
    , frame_delta_(0.0f) {
}

RenderPipeline::~RenderPipeline() {
    Shutdown();
}

RenderPipeline& RenderPipeline::Instance() {
    static RenderPipeline instance;
    return instance;
}

bool RenderPipeline::Initialize(int screen_width, int screen_height) {
    if (initialized_) {
        return true;
    }

    screen_width_ = screen_width;
    screen_height_ = screen_height;

    // Initialize graphics buffer
    if (!buffer_.Initialize()) {
        Platform_Log("RenderPipeline: Failed to initialize graphics buffer");
        return false;
    }

    // Initialize shape renderer
    if (!shape_renderer_.Initialize()) {
        Platform_Log("RenderPipeline: Failed to initialize shape renderer");
        return false;
    }

    // Initialize tile renderer (theater will be set later)
    if (!tile_renderer_.Initialize()) {
        Platform_Log("RenderPipeline: Failed to initialize tile renderer");
        return false;
    }

    // Reserve space for render queue
    render_queue_.reserve(1000);
    registered_objects_.reserve(500);
    dirty_rects_.reserve(100);
    merged_dirty_rects_.reserve(50);

    full_redraw_needed_ = true;
    initialized_ = true;

    Platform_Log("RenderPipeline: Initialized (%dx%d)", screen_width_, screen_height_);
    return true;
}

void RenderPipeline::Shutdown() {
    if (!initialized_) return;

    ClearRenderQueue();
    registered_objects_.clear();
    dirty_rects_.clear();
    merged_dirty_rects_.clear();

    tile_renderer_.Shutdown();
    shape_renderer_.Shutdown();
    buffer_.Shutdown();

    initialized_ = false;
    Platform_Log("RenderPipeline: Shutdown");
}

void RenderPipeline::RenderFrame() {
    if (!initialized_) return;

    uint64_t start_time = Platform_GetTicks();
    stats_.Reset();

    BeginFrame();

    // Build render queue from registered objects
    render_queue_.clear();
    for (IRenderable* obj : registered_objects_) {
        if (obj) {
            // Check visibility
            int wx = obj->GetWorldX();
            int wy = obj->GetWorldY();
            int bx, by, bw, bh;
            obj->GetBounds(bx, by, bw, bh);

            if (IsVisible(wx, wy, bw, bh)) {
                render_queue_.emplace_back(obj, obj->GetSortY(), obj->GetRenderLayer());
            }
        }
    }

    // Sort render queue
    SortRenderQueue();

    // Render in layer order
    RenderTerrain();
    RenderSmudges();
    RenderOverlays();
    RenderBibs();

    if (config_.enable_shadows) {
        RenderShadows();
    }

    RenderGroundObjects();
    RenderBuildings();
    RenderAirObjects();
    RenderProjectiles();
    RenderEffects();
    RenderSelectionBoxes();

    if (config_.enable_shroud) {
        RenderShroud();
    }

    RenderUI();

    // Debug overlays
    if (config_.draw_cell_grid) {
        RenderCellGrid();
    }
    if (config_.draw_debug_info) {
        RenderDebugInfo();
    }

    // Cursor is always last
    RenderCursor();

    EndFrame();

    // Update timing
    uint64_t end_time = Platform_GetTicks();
    stats_.frame_time_ms = static_cast<float>(end_time - start_time);
    frame_delta_ = stats_.frame_time_ms / 1000.0f;
}

void RenderPipeline::BeginFrame() {
    buffer_.Lock();

    if (full_redraw_needed_ || !config_.enable_dirty_rects) {
        // Clear entire screen
        buffer_.Clear(0);
        full_redraw_needed_ = false;
    } else {
        // Only clear dirty rectangles
        ProcessDirtyRects();
        for (const DirtyRect& rect : merged_dirty_rects_) {
            buffer_.FillRect(rect.x, rect.y, rect.width, rect.height, 0);
        }
    }
}

void RenderPipeline::EndFrame() {
    buffer_.Unlock();
    Platform_Graphics_Flip();

    // Clear dirty rects for next frame
    ClearDirtyRects();
}

void RenderPipeline::SortRenderQueue() {
    std::sort(render_queue_.begin(), render_queue_.end());
}

void RenderPipeline::RenderTerrain() {
    if (!viewport_) return;

    int cell_start_x = viewport_->x / TILE_WIDTH;
    int cell_start_y = viewport_->y / TILE_HEIGHT;
    int cell_end_x = (viewport_->x + viewport_->width) / TILE_WIDTH + 1;
    int cell_end_y = (viewport_->y + viewport_->height) / TILE_HEIGHT + 1;

    // Offset for tactical area (after tab bar)
    int tactical_offset_y = TAB_HEIGHT;

    for (int cy = cell_start_y; cy <= cell_end_y; cy++) {
        for (int cx = cell_start_x; cx <= cell_end_x; cx++) {
            // Get cell from map (placeholder - actual map access)
            // CellClass* cell = Map.GetCell(cx, cy);
            // if (!cell) continue;

            int screen_x = (cx * TILE_WIDTH) - viewport_->x;
            int screen_y = (cy * TILE_HEIGHT) - viewport_->y + tactical_offset_y;

            // Clip to tactical area
            if (screen_x >= TACTICAL_WIDTH || screen_y >= TACTICAL_HEIGHT + TAB_HEIGHT) continue;
            if (screen_x + TILE_WIDTH < 0 || screen_y + TILE_HEIGHT < TAB_HEIGHT) continue;

            // Draw terrain tile (placeholder)
            // tile_renderer_.DrawTile(buffer_, screen_x, screen_y, cell->GetTemplate(), cell->GetIcon());

            // For now, draw a placeholder pattern
            uint8_t color = ((cx + cy) % 2) ? 21 : 22;  // Alternating green shades
            buffer_.FillRect(screen_x, screen_y, TILE_WIDTH, TILE_HEIGHT, color);

            stats_.terrain_tiles_drawn++;
        }
    }
}

void RenderPipeline::RenderSmudges() {
    // Draw crater/scorch mark smudges on terrain
    for (const RenderEntry& entry : render_queue_) {
        if (entry.layer == RenderLayer::SMUDGE && entry.object) {
            int screen_x, screen_y;
            WorldToScreen(entry.object->GetWorldX(), entry.object->GetWorldY(), screen_x, screen_y);
            entry.object->Draw(screen_x, screen_y);
            stats_.objects_drawn++;
        }
    }
}

void RenderPipeline::RenderOverlays() {
    // Draw ore, gems, walls, fences
    for (const RenderEntry& entry : render_queue_) {
        if (entry.layer == RenderLayer::OVERLAY && entry.object) {
            int screen_x, screen_y;
            WorldToScreen(entry.object->GetWorldX(), entry.object->GetWorldY(), screen_x, screen_y);
            entry.object->Draw(screen_x, screen_y);
            stats_.objects_drawn++;
        }
    }
}

void RenderPipeline::RenderBibs() {
    // Draw building foundations
    for (const RenderEntry& entry : render_queue_) {
        if (entry.layer == RenderLayer::BIB && entry.object) {
            int screen_x, screen_y;
            WorldToScreen(entry.object->GetWorldX(), entry.object->GetWorldY(), screen_x, screen_y);
            entry.object->Draw(screen_x, screen_y);
            stats_.objects_drawn++;
        }
    }
}

void RenderPipeline::RenderShadows() {
    // Draw ground shadows for units and buildings
    for (const RenderEntry& entry : render_queue_) {
        if (entry.layer == RenderLayer::SHADOW && entry.object) {
            int screen_x, screen_y;
            WorldToScreen(entry.object->GetWorldX(), entry.object->GetWorldY(), screen_x, screen_y);
            entry.object->Draw(screen_x, screen_y);
            stats_.objects_drawn++;
        }
    }
}

void RenderPipeline::RenderGroundObjects() {
    // Draw infantry and vehicles (already Y-sorted)
    for (const RenderEntry& entry : render_queue_) {
        if (entry.layer == RenderLayer::GROUND && entry.object) {
            int screen_x, screen_y;
            WorldToScreen(entry.object->GetWorldX(), entry.object->GetWorldY(), screen_x, screen_y);
            entry.object->Draw(screen_x, screen_y);
            stats_.objects_drawn++;
        }
    }
}

void RenderPipeline::RenderBuildings() {
    // Draw structures (Y-sorted)
    for (const RenderEntry& entry : render_queue_) {
        if (entry.layer == RenderLayer::BUILDING && entry.object) {
            int screen_x, screen_y;
            WorldToScreen(entry.object->GetWorldX(), entry.object->GetWorldY(), screen_x, screen_y);
            entry.object->Draw(screen_x, screen_y);
            stats_.objects_drawn++;
        }
    }
}

void RenderPipeline::RenderAirObjects() {
    // Draw aircraft and parachutes
    for (const RenderEntry& entry : render_queue_) {
        if (entry.layer == RenderLayer::AIR && entry.object) {
            int screen_x, screen_y;
            WorldToScreen(entry.object->GetWorldX(), entry.object->GetWorldY(), screen_x, screen_y);
            entry.object->Draw(screen_x, screen_y);
            stats_.objects_drawn++;
        }
    }
}

void RenderPipeline::RenderProjectiles() {
    // Draw bullets, missiles, explosions
    for (const RenderEntry& entry : render_queue_) {
        if (entry.layer == RenderLayer::PROJECTILE && entry.object) {
            int screen_x, screen_y;
            WorldToScreen(entry.object->GetWorldX(), entry.object->GetWorldY(), screen_x, screen_y);
            entry.object->Draw(screen_x, screen_y);
            stats_.objects_drawn++;
        }
    }
}

void RenderPipeline::RenderEffects() {
    // Draw smoke, fire, visual effects
    for (const RenderEntry& entry : render_queue_) {
        if (entry.layer == RenderLayer::EFFECT && entry.object) {
            int screen_x, screen_y;
            WorldToScreen(entry.object->GetWorldX(), entry.object->GetWorldY(), screen_x, screen_y);
            entry.object->Draw(screen_x, screen_y);
            stats_.objects_drawn++;
        }
    }
}

void RenderPipeline::RenderSelectionBoxes() {
    // Draw selection indicators
    for (const RenderEntry& entry : render_queue_) {
        if (entry.layer == RenderLayer::SELECTION && entry.object) {
            int screen_x, screen_y;
            WorldToScreen(entry.object->GetWorldX(), entry.object->GetWorldY(), screen_x, screen_y);
            entry.object->Draw(screen_x, screen_y);
            stats_.objects_drawn++;
        }
    }
}

void RenderPipeline::RenderShroud() {
    // Draw fog of war overlay
    // This draws a semi-transparent black over unexplored areas
    // and revealed-but-not-visible areas get a darker tint

    if (!viewport_) return;

    int cell_start_x = viewport_->x / TILE_WIDTH;
    int cell_start_y = viewport_->y / TILE_HEIGHT;
    int cell_end_x = (viewport_->x + viewport_->width) / TILE_WIDTH + 1;
    int cell_end_y = (viewport_->y + viewport_->height) / TILE_HEIGHT + 1;

    int tactical_offset_y = TAB_HEIGHT;

    for (int cy = cell_start_y; cy <= cell_end_y; cy++) {
        for (int cx = cell_start_x; cx <= cell_end_x; cx++) {
            // Get shroud state from map (placeholder)
            // ShroudState state = Map.GetShroudState(cx, cy);
            // For now, skip shroud rendering
        }
    }
}

void RenderPipeline::RenderUI() {
    // Draw sidebar on right side
    buffer_.FillRect(TACTICAL_WIDTH, 0, SIDEBAR_WIDTH, screen_height_, 0);

    // Draw tab bar at top
    buffer_.FillRect(0, 0, TACTICAL_WIDTH, TAB_HEIGHT, 15);  // Light gray

    // UI elements would be drawn here
    // Sidebar_Render();
    // Radar_Render();
    // Power_Render();
    // etc.
}

void RenderPipeline::RenderCursor() {
    int mouse_x, mouse_y;
    Platform_Mouse_GetPosition(&mouse_x, &mouse_y);

    // Account for window scaling (2x)
    mouse_x /= 2;
    mouse_y /= 2;

    // Draw cursor
    MouseCursor::Instance().Draw(buffer_, mouse_x, mouse_y);
}

void RenderPipeline::RenderCellGrid() {
    if (!viewport_) return;

    int cell_start_x = viewport_->x / TILE_WIDTH;
    int cell_start_y = viewport_->y / TILE_HEIGHT;
    int cell_end_x = (viewport_->x + viewport_->width) / TILE_WIDTH + 1;
    int cell_end_y = (viewport_->y + viewport_->height) / TILE_HEIGHT + 1;

    int tactical_offset_y = TAB_HEIGHT;

    // Draw vertical lines
    for (int cx = cell_start_x; cx <= cell_end_x; cx++) {
        int screen_x = (cx * TILE_WIDTH) - viewport_->x;
        if (screen_x >= 0 && screen_x < TACTICAL_WIDTH) {
            buffer_.DrawVLine(screen_x, tactical_offset_y, TACTICAL_HEIGHT, DEBUG_GRID_COLOR);
        }
    }

    // Draw horizontal lines
    for (int cy = cell_start_y; cy <= cell_end_y; cy++) {
        int screen_y = (cy * TILE_HEIGHT) - viewport_->y + tactical_offset_y;
        if (screen_y >= tactical_offset_y && screen_y < TACTICAL_HEIGHT + tactical_offset_y) {
            buffer_.DrawHLine(0, screen_y, TACTICAL_WIDTH, DEBUG_GRID_COLOR);
        }
    }
}

void RenderPipeline::RenderDebugInfo() {
    // Draw debug text showing FPS and stats
    // This would use a text rendering system

    // For now, draw dirty rectangles in red
    if (config_.enable_dirty_rects) {
        for (const DirtyRect& rect : merged_dirty_rects_) {
            buffer_.DrawRect(rect.x, rect.y, rect.width, rect.height, DEBUG_DIRTY_COLOR);
        }
    }
}

void RenderPipeline::AddDirtyRect(const DirtyRect& rect) {
    if (rect.IsEmpty()) return;
    dirty_rects_.push_back(rect);
    stats_.dirty_rects_count++;
}

void RenderPipeline::AddDirtyRect(int x, int y, int width, int height) {
    AddDirtyRect(DirtyRect(x, y, width, height));
}

void RenderPipeline::MarkFullRedraw() {
    full_redraw_needed_ = true;
}

void RenderPipeline::ClearDirtyRects() {
    dirty_rects_.clear();
    merged_dirty_rects_.clear();
}

void RenderPipeline::ProcessDirtyRects() {
    // Merge overlapping dirty rectangles
    merged_dirty_rects_.clear();

    for (const DirtyRect& rect : dirty_rects_) {
        bool merged = false;
        for (DirtyRect& existing : merged_dirty_rects_) {
            if (existing.Overlaps(rect)) {
                existing.Merge(rect);
                merged = true;
                break;
            }
        }
        if (!merged) {
            merged_dirty_rects_.push_back(rect);
        }
    }
}

void RenderPipeline::RegisterObject(IRenderable* object) {
    if (!object) return;

    // Check if already registered
    for (IRenderable* obj : registered_objects_) {
        if (obj == object) return;
    }

    registered_objects_.push_back(object);
}

void RenderPipeline::UnregisterObject(IRenderable* object) {
    auto it = std::find(registered_objects_.begin(), registered_objects_.end(), object);
    if (it != registered_objects_.end()) {
        registered_objects_.erase(it);
    }
}

void RenderPipeline::ClearRenderQueue() {
    render_queue_.clear();
}

bool RenderPipeline::IsVisible(int world_x, int world_y, int width, int height) const {
    if (!viewport_) return true;

    // Check if the object's bounding box intersects the viewport
    int view_right = viewport_->x + viewport_->width;
    int view_bottom = viewport_->y + viewport_->height;

    return !(world_x + width < viewport_->x || world_x > view_right ||
             world_y + height < viewport_->y || world_y > view_bottom);
}

void RenderPipeline::WorldToScreen(int world_x, int world_y, int& screen_x, int& screen_y) const {
    if (viewport_) {
        screen_x = world_x - viewport_->x;
        screen_y = world_y - viewport_->y + TAB_HEIGHT;
    } else {
        screen_x = world_x;
        screen_y = world_y + TAB_HEIGHT;
    }
}

void RenderPipeline::ScreenToWorld(int screen_x, int screen_y, int& world_x, int& world_y) const {
    if (viewport_) {
        world_x = screen_x + viewport_->x;
        world_y = screen_y - TAB_HEIGHT + viewport_->y;
    } else {
        world_x = screen_x;
        world_y = screen_y - TAB_HEIGHT;
    }
}
```

### 4. Sidebar Renderer

**File: `src/game/sidebar_render.h`**

```cpp
// src/game/sidebar_render.h
// Sidebar UI rendering
// Task 15f - Render Pipeline

#ifndef SIDEBAR_RENDER_H
#define SIDEBAR_RENDER_H

#include "graphics_buffer.h"
#include "shape_renderer.h"
#include <cstdint>

// Sidebar dimensions
static const int SIDEBAR_X = 480;
static const int SIDEBAR_Y = 0;
static const int SIDEBAR_WIDTH_PX = 160;
static const int SIDEBAR_HEIGHT_PX = 400;

// Strip (build queue) dimensions
static const int STRIP_WIDTH = 64;
static const int STRIP_HEIGHT = 48;
static const int STRIP_ICON_SIZE = 48;
static const int STRIPS_VISIBLE = 4;  // Visible icons per column

// Button positions within sidebar
static const int REPAIR_BUTTON_Y = 160;
static const int SELL_BUTTON_Y = 184;
static const int MAP_BUTTON_Y = 208;

// Cameo (unit/building icon) rendering
struct CameoData {
    const char* shape_name;     // E.g., "HTNK-ICO.SHP"
    int frame;                  // Frame within shape
    bool available;             // Can be built
    bool building;              // Currently being built
    float build_progress;       // 0.0 to 1.0
    int queue_count;            // Number queued
};

class SidebarRenderer {
public:
    static SidebarRenderer& Instance();

    bool Initialize();
    void Shutdown();

    // Main render call
    void Render(GraphicsBuffer& buffer);

    // Individual elements
    void RenderBackground(GraphicsBuffer& buffer);
    void RenderStrips(GraphicsBuffer& buffer);
    void RenderButtons(GraphicsBuffer& buffer);
    void RenderPowerBar(GraphicsBuffer& buffer);
    void RenderCredits(GraphicsBuffer& buffer);

    // Cameo rendering
    void RenderCameo(GraphicsBuffer& buffer, int x, int y, const CameoData& cameo);
    void RenderBuildProgress(GraphicsBuffer& buffer, int x, int y, float progress);
    void RenderQueueCount(GraphicsBuffer& buffer, int x, int y, int count);

    // State
    void SetCredits(int credits) { credits_ = credits; }
    void SetPower(int current, int max) { power_current_ = current; power_max_ = max; }
    void SetStripScroll(int column, int scroll) { strip_scroll_[column] = scroll; }

    // Cameo management
    void SetCameo(int column, int index, const CameoData& cameo);
    void ClearCameos();

private:
    SidebarRenderer();
    ~SidebarRenderer();

    bool initialized_;

    // State
    int credits_;
    int power_current_;
    int power_max_;
    int strip_scroll_[2];  // Left and right columns

    // Cameo data
    CameoData left_cameos_[20];   // Building strip
    CameoData right_cameos_[20];  // Unit strip
    int left_count_;
    int right_count_;

    // Cached shapes
    ShapeRenderer shape_renderer_;
};

#endif // SIDEBAR_RENDER_H
```

### 5. Sidebar Renderer Implementation

**File: `src/game/sidebar_render.cpp`**

```cpp
// src/game/sidebar_render.cpp
// Sidebar UI rendering implementation
// Task 15f - Render Pipeline

#include "sidebar_render.h"
#include "platform.h"
#include <cstring>
#include <cstdio>

// Color indices
static const uint8_t SIDEBAR_BG_COLOR = 0;       // Black
static const uint8_t SIDEBAR_BORDER_COLOR = 15;  // Light gray
static const uint8_t CREDITS_COLOR = 179;        // Gold
static const uint8_t POWER_OK_COLOR = 21;        // Green
static const uint8_t POWER_LOW_COLOR = 12;       // Red
static const uint8_t BUILD_PROGRESS_COLOR = 119; // Light blue
static const uint8_t QUEUE_TEXT_COLOR = 15;      // White

SidebarRenderer::SidebarRenderer()
    : initialized_(false)
    , credits_(0)
    , power_current_(0)
    , power_max_(0)
    , left_count_(0)
    , right_count_(0) {
    strip_scroll_[0] = 0;
    strip_scroll_[1] = 0;
    memset(left_cameos_, 0, sizeof(left_cameos_));
    memset(right_cameos_, 0, sizeof(right_cameos_));
}

SidebarRenderer::~SidebarRenderer() {
    Shutdown();
}

SidebarRenderer& SidebarRenderer::Instance() {
    static SidebarRenderer instance;
    return instance;
}

bool SidebarRenderer::Initialize() {
    if (initialized_) return true;

    if (!shape_renderer_.Initialize()) {
        Platform_Log("SidebarRenderer: Failed to initialize shape renderer");
        return false;
    }

    initialized_ = true;
    return true;
}

void SidebarRenderer::Shutdown() {
    if (!initialized_) return;

    shape_renderer_.Shutdown();
    initialized_ = false;
}

void SidebarRenderer::Render(GraphicsBuffer& buffer) {
    if (!initialized_) return;

    RenderBackground(buffer);
    RenderStrips(buffer);
    RenderButtons(buffer);
    RenderPowerBar(buffer);
    RenderCredits(buffer);
}

void SidebarRenderer::RenderBackground(GraphicsBuffer& buffer) {
    // Fill sidebar background
    buffer.FillRect(SIDEBAR_X, SIDEBAR_Y, SIDEBAR_WIDTH_PX, SIDEBAR_HEIGHT_PX, SIDEBAR_BG_COLOR);

    // Draw border on left edge
    buffer.DrawVLine(SIDEBAR_X, 0, SIDEBAR_HEIGHT_PX, SIDEBAR_BORDER_COLOR);

    // Draw horizontal dividers
    buffer.DrawHLine(SIDEBAR_X, 16, SIDEBAR_WIDTH_PX, SIDEBAR_BORDER_COLOR);  // Below tab
    buffer.DrawHLine(SIDEBAR_X, REPAIR_BUTTON_Y, SIDEBAR_WIDTH_PX, SIDEBAR_BORDER_COLOR);
    buffer.DrawHLine(SIDEBAR_X, 320, SIDEBAR_WIDTH_PX, SIDEBAR_BORDER_COLOR);  // Above credits
}

void SidebarRenderer::RenderStrips(GraphicsBuffer& buffer) {
    // Left column (buildings) - starts at X + 8, Y + 20
    int left_x = SIDEBAR_X + 8;
    int base_y = 20;

    for (int i = 0; i < STRIPS_VISIBLE && i + strip_scroll_[0] < left_count_; i++) {
        int idx = i + strip_scroll_[0];
        int y = base_y + i * (STRIP_HEIGHT + 2);
        RenderCameo(buffer, left_x, y, left_cameos_[idx]);
    }

    // Right column (units) - starts at X + 80, Y + 20
    int right_x = SIDEBAR_X + 80;

    for (int i = 0; i < STRIPS_VISIBLE && i + strip_scroll_[1] < right_count_; i++) {
        int idx = i + strip_scroll_[1];
        int y = base_y + i * (STRIP_HEIGHT + 2);
        RenderCameo(buffer, right_x, y, right_cameos_[idx]);
    }

    // Draw scroll buttons if needed
    if (left_count_ > STRIPS_VISIBLE) {
        // Up arrow at top, down arrow at bottom of left strip
        // (placeholder - would use actual button shapes)
    }
    if (right_count_ > STRIPS_VISIBLE) {
        // Up/down arrows for right strip
    }
}

void SidebarRenderer::RenderButtons(GraphicsBuffer& buffer) {
    // Repair button
    buffer.DrawRect(SIDEBAR_X + 8, REPAIR_BUTTON_Y + 4, 32, 24, SIDEBAR_BORDER_COLOR);

    // Sell button
    buffer.DrawRect(SIDEBAR_X + 48, REPAIR_BUTTON_Y + 4, 32, 24, SIDEBAR_BORDER_COLOR);

    // Map button
    buffer.DrawRect(SIDEBAR_X + 88, REPAIR_BUTTON_Y + 4, 64, 24, SIDEBAR_BORDER_COLOR);
}

void SidebarRenderer::RenderPowerBar(GraphicsBuffer& buffer) {
    // Power bar on right edge of sidebar
    int bar_x = SIDEBAR_X + SIDEBAR_WIDTH_PX - 12;
    int bar_y = 240;
    int bar_height = 70;

    // Background
    buffer.FillRect(bar_x, bar_y, 8, bar_height, 0);
    buffer.DrawRect(bar_x, bar_y, 8, bar_height, SIDEBAR_BORDER_COLOR);

    // Fill based on power ratio
    if (power_max_ > 0) {
        float ratio = static_cast<float>(power_current_) / power_max_;
        if (ratio > 1.0f) ratio = 1.0f;

        int fill_height = static_cast<int>(bar_height * ratio);
        uint8_t color = (ratio >= 0.5f) ? POWER_OK_COLOR : POWER_LOW_COLOR;

        buffer.FillRect(bar_x + 1, bar_y + bar_height - fill_height, 6, fill_height, color);
    }
}

void SidebarRenderer::RenderCredits(GraphicsBuffer& buffer) {
    // Credits display at bottom
    int credits_y = 370;

    // Would render actual digit graphics here
    // For now, just a placeholder rectangle
    buffer.FillRect(SIDEBAR_X + 40, credits_y, 80, 20, SIDEBAR_BG_COLOR);
    buffer.DrawRect(SIDEBAR_X + 40, credits_y, 80, 20, CREDITS_COLOR);
}

void SidebarRenderer::RenderCameo(GraphicsBuffer& buffer, int x, int y, const CameoData& cameo) {
    if (!cameo.shape_name) {
        // Empty slot
        buffer.DrawRect(x, y, STRIP_ICON_SIZE, STRIP_ICON_SIZE, SIDEBAR_BORDER_COLOR);
        return;
    }

    // Draw cameo icon (placeholder)
    buffer.FillRect(x, y, STRIP_ICON_SIZE, STRIP_ICON_SIZE, 100);  // Placeholder color
    buffer.DrawRect(x, y, STRIP_ICON_SIZE, STRIP_ICON_SIZE, SIDEBAR_BORDER_COLOR);

    // Draw build progress if building
    if (cameo.building && cameo.build_progress > 0.0f) {
        RenderBuildProgress(buffer, x, y, cameo.build_progress);
    }

    // Draw queue count if > 1
    if (cameo.queue_count > 1) {
        RenderQueueCount(buffer, x, y, cameo.queue_count);
    }

    // Gray out if unavailable
    if (!cameo.available) {
        // Would apply a dark overlay or different rendering
    }
}

void SidebarRenderer::RenderBuildProgress(GraphicsBuffer& buffer, int x, int y, float progress) {
    // Draw a clock-wipe progress indicator
    int center_x = x + STRIP_ICON_SIZE / 2;
    int center_y = y + STRIP_ICON_SIZE / 2;

    // Simple vertical bar for now
    int fill_height = static_cast<int>(STRIP_ICON_SIZE * progress);
    buffer.FillRect(x, y + STRIP_ICON_SIZE - fill_height, STRIP_ICON_SIZE, fill_height, BUILD_PROGRESS_COLOR);
}

void SidebarRenderer::RenderQueueCount(GraphicsBuffer& buffer, int x, int y, int count) {
    // Draw queue count in corner
    // Would render actual digit graphics here
    int digit_x = x + STRIP_ICON_SIZE - 12;
    int digit_y = y + STRIP_ICON_SIZE - 12;
    buffer.FillRect(digit_x, digit_y, 10, 10, 0);
}

void SidebarRenderer::SetCameo(int column, int index, const CameoData& cameo) {
    if (column == 0 && index < 20) {
        left_cameos_[index] = cameo;
        if (index >= left_count_) left_count_ = index + 1;
    } else if (column == 1 && index < 20) {
        right_cameos_[index] = cameo;
        if (index >= right_count_) right_count_ = index + 1;
    }
}

void SidebarRenderer::ClearCameos() {
    memset(left_cameos_, 0, sizeof(left_cameos_));
    memset(right_cameos_, 0, sizeof(right_cameos_));
    left_count_ = 0;
    right_count_ = 0;
}
```

### 6. Radar Renderer

**File: `src/game/radar_render.h`**

```cpp
// src/game/radar_render.h
// Radar minimap rendering
// Task 15f - Render Pipeline

#ifndef RADAR_RENDER_H
#define RADAR_RENDER_H

#include "graphics_buffer.h"
#include <cstdint>

// Radar dimensions within sidebar
static const int RADAR_X = 496;
static const int RADAR_Y = 232;
static const int RADAR_SIZE = 128;  // Square radar
static const int RADAR_CELL_SIZE = 2;  // Pixels per cell on radar

// Radar blip colors (index into palette)
static const uint8_t RADAR_TERRAIN_CLEAR = 21;   // Green (grass)
static const uint8_t RADAR_TERRAIN_WATER = 119;  // Blue
static const uint8_t RADAR_TERRAIN_ROCK = 24;    // Brown
static const uint8_t RADAR_TERRAIN_ROAD = 15;    // Light gray
static const uint8_t RADAR_FRIENDLY = 120;       // Light green
static const uint8_t RADAR_ENEMY = 127;          // Red
static const uint8_t RADAR_NEUTRAL = 179;        // Yellow
static const uint8_t RADAR_BUILDING = 15;        // White
static const uint8_t RADAR_SHROUD = 0;           // Black

class RadarRenderer {
public:
    static RadarRenderer& Instance();

    bool Initialize();
    void Shutdown();

    // Main render
    void Render(GraphicsBuffer& buffer);

    // Radar state
    void SetActive(bool active) { active_ = active; }
    bool IsActive() const { return active_; }

    void SetJammed(bool jammed) { jammed_ = jammed; }
    bool IsJammed() const { return jammed_; }

    // Map dimensions
    void SetMapSize(int width, int height);
    void SetViewportRect(int x, int y, int width, int height);

    // Terrain data (per-cell)
    void SetTerrainCell(int x, int y, uint8_t terrain_type);
    void SetShroudCell(int x, int y, bool shrouded);

    // Blip management
    void ClearBlips();
    void AddBlip(int x, int y, uint8_t color);

    // Click handling (returns cell coordinates)
    bool GetCellAtRadar(int screen_x, int screen_y, int& cell_x, int& cell_y);

private:
    RadarRenderer();
    ~RadarRenderer();

    void RenderTerrain(GraphicsBuffer& buffer);
    void RenderBlips(GraphicsBuffer& buffer);
    void RenderViewportBox(GraphicsBuffer& buffer);
    void RenderFrame(GraphicsBuffer& buffer);
    void RenderJammedStatic(GraphicsBuffer& buffer);

    bool initialized_;
    bool active_;
    bool jammed_;

    int map_width_;
    int map_height_;
    int viewport_x_, viewport_y_;
    int viewport_width_, viewport_height_;

    // Terrain data (simple array for now)
    uint8_t* terrain_data_;
    uint8_t* shroud_data_;
    int terrain_size_;

    // Blips
    struct Blip {
        int x, y;
        uint8_t color;
    };
    Blip blips_[500];
    int blip_count_;
};

#endif // RADAR_RENDER_H
```

### 7. Radar Renderer Implementation

**File: `src/game/radar_render.cpp`**

```cpp
// src/game/radar_render.cpp
// Radar minimap rendering implementation
// Task 15f - Render Pipeline

#include "radar_render.h"
#include "platform.h"
#include <cstring>
#include <cstdlib>

RadarRenderer::RadarRenderer()
    : initialized_(false)
    , active_(false)
    , jammed_(false)
    , map_width_(64)
    , map_height_(64)
    , viewport_x_(0)
    , viewport_y_(0)
    , viewport_width_(20)
    , viewport_height_(16)
    , terrain_data_(nullptr)
    , shroud_data_(nullptr)
    , terrain_size_(0)
    , blip_count_(0) {
}

RadarRenderer::~RadarRenderer() {
    Shutdown();
}

RadarRenderer& RadarRenderer::Instance() {
    static RadarRenderer instance;
    return instance;
}

bool RadarRenderer::Initialize() {
    if (initialized_) return true;

    // Allocate terrain data for default map size
    terrain_size_ = map_width_ * map_height_;
    terrain_data_ = new uint8_t[terrain_size_];
    shroud_data_ = new uint8_t[terrain_size_];

    memset(terrain_data_, RADAR_TERRAIN_CLEAR, terrain_size_);
    memset(shroud_data_, 1, terrain_size_);  // Start fully shrouded

    initialized_ = true;
    return true;
}

void RadarRenderer::Shutdown() {
    if (!initialized_) return;

    delete[] terrain_data_;
    delete[] shroud_data_;
    terrain_data_ = nullptr;
    shroud_data_ = nullptr;

    initialized_ = false;
}

void RadarRenderer::Render(GraphicsBuffer& buffer) {
    if (!initialized_) return;

    RenderFrame(buffer);

    if (!active_) {
        // Radar not powered - show black
        buffer.FillRect(RADAR_X + 2, RADAR_Y + 2, RADAR_SIZE - 4, RADAR_SIZE - 4, 0);
        return;
    }

    if (jammed_) {
        RenderJammedStatic(buffer);
        return;
    }

    RenderTerrain(buffer);
    RenderBlips(buffer);
    RenderViewportBox(buffer);
}

void RadarRenderer::RenderFrame(GraphicsBuffer& buffer) {
    // Draw radar border
    buffer.DrawRect(RADAR_X, RADAR_Y, RADAR_SIZE, RADAR_SIZE, 15);
    buffer.DrawRect(RADAR_X + 1, RADAR_Y + 1, RADAR_SIZE - 2, RADAR_SIZE - 2, 8);
}

void RadarRenderer::RenderTerrain(GraphicsBuffer& buffer) {
    // Calculate scale factor
    float scale_x = static_cast<float>(RADAR_SIZE - 4) / map_width_;
    float scale_y = static_cast<float>(RADAR_SIZE - 4) / map_height_;

    int radar_left = RADAR_X + 2;
    int radar_top = RADAR_Y + 2;

    for (int cy = 0; cy < map_height_; cy++) {
        for (int cx = 0; cx < map_width_; cx++) {
            int idx = cy * map_width_ + cx;

            // Get radar color
            uint8_t color;
            if (shroud_data_[idx]) {
                color = RADAR_SHROUD;
            } else {
                color = terrain_data_[idx];
            }

            // Calculate radar pixel position
            int rx = radar_left + static_cast<int>(cx * scale_x);
            int ry = radar_top + static_cast<int>(cy * scale_y);

            // Draw as 2x2 pixel block for visibility
            int pixel_w = static_cast<int>(scale_x);
            int pixel_h = static_cast<int>(scale_y);
            if (pixel_w < 1) pixel_w = 1;
            if (pixel_h < 1) pixel_h = 1;

            buffer.FillRect(rx, ry, pixel_w, pixel_h, color);
        }
    }
}

void RadarRenderer::RenderBlips(GraphicsBuffer& buffer) {
    float scale_x = static_cast<float>(RADAR_SIZE - 4) / map_width_;
    float scale_y = static_cast<float>(RADAR_SIZE - 4) / map_height_;

    int radar_left = RADAR_X + 2;
    int radar_top = RADAR_Y + 2;

    for (int i = 0; i < blip_count_; i++) {
        const Blip& blip = blips_[i];

        int rx = radar_left + static_cast<int>(blip.x * scale_x);
        int ry = radar_top + static_cast<int>(blip.y * scale_y);

        // Draw blip as 2x2 block
        buffer.FillRect(rx, ry, 2, 2, blip.color);
    }
}

void RadarRenderer::RenderViewportBox(GraphicsBuffer& buffer) {
    float scale_x = static_cast<float>(RADAR_SIZE - 4) / map_width_;
    float scale_y = static_cast<float>(RADAR_SIZE - 4) / map_height_;

    int radar_left = RADAR_X + 2;
    int radar_top = RADAR_Y + 2;

    int box_x = radar_left + static_cast<int>(viewport_x_ * scale_x);
    int box_y = radar_top + static_cast<int>(viewport_y_ * scale_y);
    int box_w = static_cast<int>(viewport_width_ * scale_x);
    int box_h = static_cast<int>(viewport_height_ * scale_y);

    // Draw viewport rectangle in white
    buffer.DrawRect(box_x, box_y, box_w, box_h, 15);
}

void RadarRenderer::RenderJammedStatic(GraphicsBuffer& buffer) {
    // Draw random static when radar is jammed
    int radar_left = RADAR_X + 2;
    int radar_top = RADAR_Y + 2;
    int radar_right = RADAR_X + RADAR_SIZE - 2;
    int radar_bottom = RADAR_Y + RADAR_SIZE - 2;

    for (int y = radar_top; y < radar_bottom; y += 2) {
        for (int x = radar_left; x < radar_right; x += 2) {
            uint8_t color = (rand() % 2) ? 15 : 0;
            buffer.FillRect(x, y, 2, 2, color);
        }
    }
}

void RadarRenderer::SetMapSize(int width, int height) {
    if (width == map_width_ && height == map_height_) return;

    // Reallocate terrain data
    delete[] terrain_data_;
    delete[] shroud_data_;

    map_width_ = width;
    map_height_ = height;
    terrain_size_ = width * height;

    terrain_data_ = new uint8_t[terrain_size_];
    shroud_data_ = new uint8_t[terrain_size_];

    memset(terrain_data_, RADAR_TERRAIN_CLEAR, terrain_size_);
    memset(shroud_data_, 1, terrain_size_);
}

void RadarRenderer::SetViewportRect(int x, int y, int width, int height) {
    viewport_x_ = x;
    viewport_y_ = y;
    viewport_width_ = width;
    viewport_height_ = height;
}

void RadarRenderer::SetTerrainCell(int x, int y, uint8_t terrain_type) {
    if (x < 0 || x >= map_width_ || y < 0 || y >= map_height_) return;
    terrain_data_[y * map_width_ + x] = terrain_type;
}

void RadarRenderer::SetShroudCell(int x, int y, bool shrouded) {
    if (x < 0 || x >= map_width_ || y < 0 || y >= map_height_) return;
    shroud_data_[y * map_width_ + x] = shrouded ? 1 : 0;
}

void RadarRenderer::ClearBlips() {
    blip_count_ = 0;
}

void RadarRenderer::AddBlip(int x, int y, uint8_t color) {
    if (blip_count_ >= 500) return;

    blips_[blip_count_].x = x;
    blips_[blip_count_].y = y;
    blips_[blip_count_].color = color;
    blip_count_++;
}

bool RadarRenderer::GetCellAtRadar(int screen_x, int screen_y, int& cell_x, int& cell_y) {
    // Check if click is within radar bounds
    if (screen_x < RADAR_X + 2 || screen_x >= RADAR_X + RADAR_SIZE - 2 ||
        screen_y < RADAR_Y + 2 || screen_y >= RADAR_Y + RADAR_SIZE - 2) {
        return false;
    }

    // Convert screen position to cell coordinates
    float scale_x = static_cast<float>(map_width_) / (RADAR_SIZE - 4);
    float scale_y = static_cast<float>(map_height_) / (RADAR_SIZE - 4);

    cell_x = static_cast<int>((screen_x - RADAR_X - 2) * scale_x);
    cell_y = static_cast<int>((screen_y - RADAR_Y - 2) * scale_y);

    return true;
}
```

### 8. Test Program

**File: `tests/test_render_pipeline.cpp`**

```cpp
// tests/test_render_pipeline.cpp
// Test program for render pipeline
// Task 15f - Render Pipeline

#include "platform.h"
#include "render_pipeline.h"
#include "viewport.h"
#include "palette_manager.h"
#include "mouse_cursor.h"
#include "sidebar_render.h"
#include "radar_render.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>

// Test renderable object
class TestObject : public IRenderable {
public:
    int x, y;
    int width, height;
    uint8_t color;
    RenderLayer layer;

    TestObject(int x_, int y_, int w, int h, uint8_t c, RenderLayer l)
        : x(x_), y(y_), width(w), height(h), color(c), layer(l) {}

    RenderLayer GetRenderLayer() const override { return layer; }
    int GetSortY() const override { return y + height; }
    void Draw(int screen_x, int screen_y) override {
        RenderPipeline::Instance().GetBuffer().FillRect(
            screen_x, screen_y, width, height, color);
    }
    int GetWorldX() const override { return x; }
    int GetWorldY() const override { return y; }
    void GetBounds(int& bx, int& by, int& bw, int& bh) const override {
        bx = x; by = y; bw = width; bh = height;
    }
};

bool Test_RenderPipelineInit() {
    printf("Test: Render Pipeline Initialization... ");

    if (!RenderPipeline::Instance().Initialize(640, 400)) {
        printf("FAILED - Could not initialize\n");
        return false;
    }

    if (!RenderPipeline::Instance().IsInitialized()) {
        printf("FAILED - Not marked as initialized\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_ViewportSetup() {
    printf("Test: Viewport Setup... ");

    static Viewport viewport;
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = 480;
    viewport.height = 384;

    RenderPipeline::Instance().SetViewport(&viewport);

    if (RenderPipeline::Instance().GetViewport() != &viewport) {
        printf("FAILED - Viewport not set\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_ObjectRegistration() {
    printf("Test: Object Registration... ");

    TestObject obj1(100, 100, 24, 24, 120, RenderLayer::GROUND);
    TestObject obj2(200, 150, 24, 24, 127, RenderLayer::GROUND);

    RenderPipeline::Instance().RegisterObject(&obj1);
    RenderPipeline::Instance().RegisterObject(&obj2);

    // Should not crash on duplicate registration
    RenderPipeline::Instance().RegisterObject(&obj1);

    printf("PASSED\n");
    return true;
}

bool Test_VisibilityCheck() {
    printf("Test: Visibility Check... ");

    static Viewport viewport;
    viewport.x = 100;
    viewport.y = 100;
    viewport.width = 200;
    viewport.height = 200;

    RenderPipeline::Instance().SetViewport(&viewport);

    // Object inside viewport
    if (!RenderPipeline::Instance().IsVisible(150, 150, 24, 24)) {
        printf("FAILED - Object should be visible\n");
        return false;
    }

    // Object outside viewport
    if (RenderPipeline::Instance().IsVisible(0, 0, 24, 24)) {
        printf("FAILED - Object should not be visible\n");
        return false;
    }

    // Object partially visible
    if (!RenderPipeline::Instance().IsVisible(90, 90, 50, 50)) {
        printf("FAILED - Partial object should be visible\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_CoordinateConversion() {
    printf("Test: Coordinate Conversion... ");

    static Viewport viewport;
    viewport.x = 100;
    viewport.y = 50;
    viewport.width = 480;
    viewport.height = 384;

    RenderPipeline::Instance().SetViewport(&viewport);

    int screen_x, screen_y;
    RenderPipeline::Instance().WorldToScreen(200, 100, screen_x, screen_y);

    // screen_x should be 200 - 100 = 100
    // screen_y should be 100 - 50 + TAB_HEIGHT(16) = 66
    if (screen_x != 100 || screen_y != 66) {
        printf("FAILED - WorldToScreen incorrect (%d, %d) expected (100, 66)\n", screen_x, screen_y);
        return false;
    }

    int world_x, world_y;
    RenderPipeline::Instance().ScreenToWorld(100, 66, world_x, world_y);

    if (world_x != 200 || world_y != 100) {
        printf("FAILED - ScreenToWorld incorrect (%d, %d) expected (200, 100)\n", world_x, world_y);
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_DirtyRects() {
    printf("Test: Dirty Rectangle Management... ");

    RenderPipeline::Instance().ClearDirtyRects();

    RenderPipeline::Instance().AddDirtyRect(10, 10, 50, 50);
    RenderPipeline::Instance().AddDirtyRect(20, 20, 50, 50);  // Overlapping
    RenderPipeline::Instance().AddDirtyRect(200, 200, 30, 30);  // Non-overlapping

    const RenderStats& stats = RenderPipeline::Instance().GetStats();
    // Stats won't show merged count until ProcessDirtyRects is called

    printf("PASSED\n");
    return true;
}

bool Test_RenderConfig() {
    printf("Test: Render Configuration... ");

    RenderConfig config;
    config.enable_dirty_rects = true;
    config.enable_shadows = true;
    config.enable_shroud = true;
    config.draw_debug_info = false;
    config.draw_cell_grid = false;
    config.target_fps = 60;

    RenderPipeline::Instance().SetConfig(config);

    const RenderConfig& retrieved = RenderPipeline::Instance().GetConfig();
    if (!retrieved.enable_dirty_rects || !retrieved.enable_shadows) {
        printf("FAILED - Config not applied\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_SidebarRenderer() {
    printf("Test: Sidebar Renderer... ");

    if (!SidebarRenderer::Instance().Initialize()) {
        printf("FAILED - Could not initialize\n");
        return false;
    }

    SidebarRenderer::Instance().SetCredits(5000);
    SidebarRenderer::Instance().SetPower(150, 200);

    CameoData cameo;
    cameo.shape_name = "TEST.SHP";
    cameo.frame = 0;
    cameo.available = true;
    cameo.building = false;
    cameo.build_progress = 0.0f;
    cameo.queue_count = 1;

    SidebarRenderer::Instance().SetCameo(0, 0, cameo);

    printf("PASSED\n");
    return true;
}

bool Test_RadarRenderer() {
    printf("Test: Radar Renderer... ");

    if (!RadarRenderer::Instance().Initialize()) {
        printf("FAILED - Could not initialize\n");
        return false;
    }

    RadarRenderer::Instance().SetMapSize(64, 64);
    RadarRenderer::Instance().SetViewportRect(10, 10, 20, 16);
    RadarRenderer::Instance().SetActive(true);

    // Set some terrain
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
            uint8_t terrain = ((x + y) % 4 == 0) ? RADAR_TERRAIN_WATER : RADAR_TERRAIN_CLEAR;
            RadarRenderer::Instance().SetTerrainCell(x, y, terrain);
            RadarRenderer::Instance().SetShroudCell(x, y, false);
        }
    }

    // Add some blips
    RadarRenderer::Instance().ClearBlips();
    RadarRenderer::Instance().AddBlip(20, 20, RADAR_FRIENDLY);
    RadarRenderer::Instance().AddBlip(40, 30, RADAR_ENEMY);

    printf("PASSED\n");
    return true;
}

void Interactive_Test() {
    printf("\nInteractive Render Test - Press ESC to exit\n\n");

    // Initialize platform
    if (Platform_Graphics_Init() != 0) {
        printf("Failed to initialize graphics\n");
        return;
    }

    // Initialize subsystems
    PaletteManager::Instance().Initialize();
    MouseCursor::Instance().Initialize();
    RenderPipeline::Instance().Initialize(640, 400);
    SidebarRenderer::Instance().Initialize();
    RadarRenderer::Instance().Initialize();

    // Load default palette
    PaletteManager::Instance().LoadTheaterPalette(TheaterType::TEMPERATE);

    // Set up viewport
    static Viewport viewport;
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = 480;
    viewport.height = 384;
    RenderPipeline::Instance().SetViewport(&viewport);

    // Enable debug options
    RenderConfig config;
    config.draw_cell_grid = true;
    config.draw_debug_info = true;
    RenderPipeline::Instance().SetConfig(config);

    // Set up radar
    RadarRenderer::Instance().SetMapSize(64, 64);
    RadarRenderer::Instance().SetActive(true);
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
            RadarRenderer::Instance().SetShroudCell(x, y, false);
        }
    }

    // Create test objects
    TestObject units[10] = {
        TestObject(100, 100, 24, 24, 120, RenderLayer::GROUND),
        TestObject(150, 120, 24, 24, 120, RenderLayer::GROUND),
        TestObject(200, 80, 24, 24, 127, RenderLayer::GROUND),
        TestObject(300, 200, 48, 48, 15, RenderLayer::BUILDING),
        TestObject(100, 300, 24, 24, 120, RenderLayer::GROUND),
        TestObject(400, 100, 24, 24, 179, RenderLayer::AIR),
        TestObject(250, 250, 24, 24, 12, RenderLayer::EFFECT),
        TestObject(350, 150, 24, 24, 120, RenderLayer::GROUND),
        TestObject(50, 200, 24, 24, 127, RenderLayer::GROUND),
        TestObject(180, 180, 24, 24, 120, RenderLayer::GROUND),
    };

    for (int i = 0; i < 10; i++) {
        RenderPipeline::Instance().RegisterObject(&units[i]);
    }

    // Main loop
    bool running = true;
    int scroll_speed = 8;

    while (running) {
        // Handle input
        Platform_PumpEvents();

        if (Platform_Key_Down(KEY_ESCAPE)) {
            running = false;
        }

        // Scroll viewport
        if (Platform_Key_Down(KEY_LEFT)) {
            viewport.x -= scroll_speed;
            if (viewport.x < 0) viewport.x = 0;
        }
        if (Platform_Key_Down(KEY_RIGHT)) {
            viewport.x += scroll_speed;
        }
        if (Platform_Key_Down(KEY_UP)) {
            viewport.y -= scroll_speed;
            if (viewport.y < 0) viewport.y = 0;
        }
        if (Platform_Key_Down(KEY_DOWN)) {
            viewport.y += scroll_speed;
        }

        // Toggle grid with G key
        static bool g_pressed = false;
        if (Platform_Key_Down(KEY_G) && !g_pressed) {
            RenderConfig& cfg = RenderPipeline::Instance().GetConfig();
            cfg.draw_cell_grid = !cfg.draw_cell_grid;
            g_pressed = true;
        }
        if (!Platform_Key_Down(KEY_G)) {
            g_pressed = false;
        }

        // Update radar viewport
        int cells_x = viewport.x / 24;
        int cells_y = viewport.y / 24;
        int cells_w = viewport.width / 24;
        int cells_h = viewport.height / 24;
        RadarRenderer::Instance().SetViewportRect(cells_x, cells_y, cells_w, cells_h);

        // Clear radar blips and add based on unit positions
        RadarRenderer::Instance().ClearBlips();
        for (int i = 0; i < 10; i++) {
            int cx = units[i].x / 24;
            int cy = units[i].y / 24;
            RadarRenderer::Instance().AddBlip(cx, cy, units[i].color);
        }

        // Render frame
        RenderPipeline::Instance().RenderFrame();

        // Render sidebar and radar on top of main render
        GraphicsBuffer& buffer = RenderPipeline::Instance().GetBuffer();
        buffer.Lock();
        SidebarRenderer::Instance().Render(buffer);
        RadarRenderer::Instance().Render(buffer);
        buffer.Unlock();
        Platform_Graphics_Flip();

        // Frame delay
        Platform_Timer_Delay(16);  // ~60 FPS
    }

    // Cleanup
    RadarRenderer::Instance().Shutdown();
    SidebarRenderer::Instance().Shutdown();
    RenderPipeline::Instance().Shutdown();
    MouseCursor::Instance().Shutdown();
    PaletteManager::Instance().Shutdown();
    Platform_Graphics_Shutdown();
}

int main(int argc, char* argv[]) {
    printf("=== Render Pipeline Tests ===\n\n");

    int passed = 0;
    int failed = 0;

    // Initialize platform for testing
    Platform_Init();

    // Run unit tests
    if (Test_RenderPipelineInit()) passed++; else failed++;
    if (Test_ViewportSetup()) passed++; else failed++;
    if (Test_ObjectRegistration()) passed++; else failed++;
    if (Test_VisibilityCheck()) passed++; else failed++;
    if (Test_CoordinateConversion()) passed++; else failed++;
    if (Test_DirtyRects()) passed++; else failed++;
    if (Test_RenderConfig()) passed++; else failed++;
    if (Test_SidebarRenderer()) passed++; else failed++;
    if (Test_RadarRenderer()) passed++; else failed++;

    printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);

    // Run interactive test if requested
    if (argc > 1 && strcmp(argv[1], "-i") == 0) {
        Interactive_Test();
    } else {
        printf("\nRun with -i for interactive visual test\n");
    }

    // Cleanup
    RenderPipeline::Instance().Shutdown();
    Platform_Shutdown();

    return (failed == 0) ? 0 : 1;
}
```

---

## CMakeLists.txt Additions

Add to `src/game/CMakeLists.txt`:

```cmake
# Render Pipeline (Task 15f)
target_sources(game PRIVATE
    render_layer.h
    render_pipeline.h
    render_pipeline.cpp
    sidebar_render.h
    sidebar_render.cpp
    radar_render.h
    radar_render.cpp
)
```

Add to `tests/CMakeLists.txt`:

```cmake
# Render Pipeline test (Task 15f)
add_executable(test_render_pipeline test_render_pipeline.cpp)
target_link_libraries(test_render_pipeline PRIVATE game platform)
add_test(NAME RenderPipelineTest COMMAND test_render_pipeline)
```

---

## Implementation Steps

1. **Create render_layer.h** - Define RenderLayer enum, IRenderable interface, DirtyRect struct, RenderEntry for sorting
2. **Create render_pipeline.h** - Define RenderPipeline class with all render stage methods
3. **Create render_pipeline.cpp** - Implement main render loop, layer rendering, coordinate conversion
4. **Create sidebar_render.h/cpp** - Implement sidebar UI rendering with cameos, power bar, buttons
5. **Create radar_render.h/cpp** - Implement minimap rendering with terrain, blips, viewport box
6. **Create test_render_pipeline.cpp** - Unit tests and interactive visual test
7. **Update CMakeLists.txt** - Add new source files and test
8. **Build and run tests** - Verify all unit tests pass
9. **Run interactive test** - Verify visual rendering works correctly

---

## Success Criteria

- [ ] RenderPipeline initializes successfully
- [ ] All render layers draw in correct order
- [ ] Objects are Y-sorted within their layer
- [ ] Coordinate conversion (world↔screen) works correctly
- [ ] Visibility culling works (only visible objects rendered)
- [ ] Dirty rectangle tracking works (when enabled)
- [ ] Sidebar renders with cameos, power bar, buttons
- [ ] Radar renders terrain, blips, and viewport box
- [ ] Mouse cursor renders on top of everything
- [ ] Frame rate stays at 60 FPS with ~100 objects
- [ ] No visual artifacts or tearing
- [ ] All unit tests pass
- [ ] Interactive test shows complete game-like display

---

## Common Issues

1. **Objects rendering in wrong order**
   - Verify layer enum values are correct
   - Check Y-sorting comparison operator
   - Ensure objects register with correct layer

2. **Coordinate mismatch**
   - Account for TAB_HEIGHT offset in screen coordinates
   - Check viewport is set before rendering
   - Verify scale factor if window is scaled

3. **Sidebar/radar drawing over tactical area**
   - Ensure tactical clipping is applied
   - Check SIDEBAR_X and RADAR_X constants

4. **Flickering or tearing**
   - Ensure double buffering (Lock/Unlock/Flip pattern)
   - Don't flip while drawing

5. **Poor performance**
   - Enable dirty rectangles for partial updates
   - Reduce render queue allocations
   - Profile with stats.frame_time_ms

---

## Verification Script

```bash
#!/bin/bash
set -e

echo "=== Task 15f Verification ==="

# Build
echo "Building..."
cd build
cmake --build . --target test_render_pipeline

# Run unit tests
echo "Running unit tests..."
./tests/test_render_pipeline

# Check source files exist
echo "Checking source files..."
for file in \
    "../src/game/render_layer.h" \
    "../src/game/render_pipeline.h" \
    "../src/game/render_pipeline.cpp" \
    "../src/game/sidebar_render.h" \
    "../src/game/sidebar_render.cpp" \
    "../src/game/radar_render.h" \
    "../src/game/radar_render.cpp"
do
    if [ ! -f "$file" ]; then
        echo "ERROR: Missing $file"
        exit 1
    fi
done

echo ""
echo "=== Task 15f Verification PASSED ==="
echo "Run './tests/test_render_pipeline -i' for interactive visual test"
```

---

## Completion Promise

When this task is complete:
1. Complete render pipeline orchestrating all graphics subsystems
2. Proper layer-based rendering with Y-sorting
3. Sidebar UI with cameos, power bar, and buttons
4. Radar minimap with terrain, blips, and viewport indicator
5. Dirty rectangle optimization (optional, configurable)
6. Coordinate conversion between world and screen space
7. Performance monitoring via RenderStats
8. All unit tests passing
9. Interactive test showing complete game display

---

## Escape Hatch

If you get stuck:
1. Start with minimal render loop (just terrain + cursor)
2. Add layers one at a time
3. Skip dirty rectangles initially (full redraw is fine on modern hardware)
4. Use placeholder rectangles for sidebar/radar before full implementation
5. Ask for help with specific rendering issues

---

## Max Iterations

This task should complete in **6-8 iterations**:
1. Create render_layer.h with enums and interfaces
2. Create render_pipeline.h with class definition
3. Implement render_pipeline.cpp core rendering
4. Implement sidebar_render.h/cpp
5. Implement radar_render.h/cpp
6. Create test program and verify
7. Fix any issues found in testing
8. Polish and final verification
