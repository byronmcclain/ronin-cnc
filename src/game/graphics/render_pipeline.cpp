/**
 * Render Pipeline Implementation
 */

#include "game/graphics/render_pipeline.h"
#include "game/graphics/sidebar_render.h"
#include "game/graphics/radar_render.h"
#include "game/graphics/tile_renderer.h"
#include "game/graphics/mouse_cursor.h"
#include "platform.h"
#include <algorithm>

// Use enum RenderLayer from header (it's an enum class)
using LayerType = RenderLayer;

// =============================================================================
// Singleton
// =============================================================================

RenderPipeline& RenderPipeline::Instance() {
    static RenderPipeline instance;
    return instance;
}

RenderPipeline::RenderPipeline()
    : initialized_(false)
    , screen_width_(640)
    , screen_height_(400)
    , scroll_x_(0)
    , scroll_y_(0)
    , dirty_rect_enabled_(true)
    , full_redraw_pending_(true)
    , tile_renderer_(nullptr)
    , sidebar_(nullptr)
    , radar_(nullptr)
    , debug_mode_(false)
{
    tactical_viewport_ = Viewport(0, 0, DEFAULT_TACTICAL_WIDTH, DEFAULT_TACTICAL_HEIGHT);
}

RenderPipeline::~RenderPipeline() {
    Shutdown();
}

// =============================================================================
// Initialization
// =============================================================================

bool RenderPipeline::Initialize(int screen_width, int screen_height) {
    if (initialized_) {
        return true;
    }

    screen_width_ = screen_width;
    screen_height_ = screen_height;

    // Set default tactical viewport (left side, excluding sidebar)
    tactical_viewport_ = Viewport(0, 0, screen_width - SIDEBAR_WIDTH, screen_height);

    // Reserve space for dirty rects and render queue
    dirty_rects_.reserve(MAX_DIRTY_RECTS);
    render_queue_.reserve(256);

    // Start with full redraw
    full_redraw_pending_ = true;

    initialized_ = true;
    Platform_LogInfo("Render pipeline initialized");

    return true;
}

void RenderPipeline::Shutdown() {
    if (!initialized_) return;

    dirty_rects_.clear();
    render_queue_.clear();

    tile_renderer_ = nullptr;
    sidebar_ = nullptr;
    radar_ = nullptr;

    initialized_ = false;
    Platform_LogInfo("Render pipeline shutdown");
}

// =============================================================================
// Viewport Control
// =============================================================================

void RenderPipeline::SetTacticalViewport(int x, int y, int width, int height) {
    tactical_viewport_ = Viewport(x, y, width, height);
    MarkFullRedraw();
}

void RenderPipeline::SetScrollPosition(int world_x, int world_y) {
    if (world_x != scroll_x_ || world_y != scroll_y_) {
        scroll_x_ = world_x;
        scroll_y_ = world_y;
        MarkFullRedraw();  // Scrolling requires full tactical redraw
    }
}

void RenderPipeline::GetScrollPosition(int* world_x, int* world_y) const {
    if (world_x) *world_x = scroll_x_;
    if (world_y) *world_y = scroll_y_;
}

bool RenderPipeline::WorldToScreen(int world_x, int world_y, int* screen_x, int* screen_y) const {
    int sx = world_x - scroll_x_ + tactical_viewport_.x;
    int sy = world_y - scroll_y_ + tactical_viewport_.y;

    if (screen_x) *screen_x = sx;
    if (screen_y) *screen_y = sy;

    // Check if within tactical viewport
    return (sx >= tactical_viewport_.x &&
            sx < tactical_viewport_.x + tactical_viewport_.width &&
            sy >= tactical_viewport_.y &&
            sy < tactical_viewport_.y + tactical_viewport_.height);
}

void RenderPipeline::ScreenToWorld(int screen_x, int screen_y, int* world_x, int* world_y) const {
    if (world_x) *world_x = screen_x - tactical_viewport_.x + scroll_x_;
    if (world_y) *world_y = screen_y - tactical_viewport_.y + scroll_y_;
}

bool RenderPipeline::IsVisible(int world_x, int world_y, int width, int height) const {
    // Convert to screen coordinates
    int screen_x = world_x - scroll_x_ + tactical_viewport_.x;
    int screen_y = world_y - scroll_y_ + tactical_viewport_.y;

    // Check overlap with tactical viewport
    return !(screen_x + width <= tactical_viewport_.x ||
             screen_x >= tactical_viewport_.x + tactical_viewport_.width ||
             screen_y + height <= tactical_viewport_.y ||
             screen_y >= tactical_viewport_.y + tactical_viewport_.height);
}

// =============================================================================
// Dirty Rectangle Management
// =============================================================================

void RenderPipeline::AddDirtyRect(int x, int y, int width, int height) {
    if (!dirty_rect_enabled_ || full_redraw_pending_) return;

    DirtyRect rect(x, y, width, height);
    ClipToViewport(rect);

    if (rect.IsEmpty()) return;

    // Check for merge with existing rects
    for (auto& existing : dirty_rects_) {
        if (existing.Overlaps(rect)) {
            existing.Merge(rect);
            return;
        }
    }

    // Add new rect if under limit
    if (dirty_rects_.size() < MAX_DIRTY_RECTS) {
        dirty_rects_.push_back(rect);
    } else {
        // Too many rects, merge into existing
        MergeDirtyRects();
        if (dirty_rects_.size() < MAX_DIRTY_RECTS) {
            dirty_rects_.push_back(rect);
        } else {
            // Still too many, force full redraw
            MarkFullRedraw();
        }
    }
}

void RenderPipeline::AddDirtyWorldRect(int world_x, int world_y, int width, int height) {
    int screen_x, screen_y;
    WorldToScreen(world_x, world_y, &screen_x, &screen_y);
    AddDirtyRect(screen_x, screen_y, width, height);
}

void RenderPipeline::MarkFullRedraw() {
    full_redraw_pending_ = true;
    dirty_rects_.clear();
}

void RenderPipeline::ClearDirtyRects() {
    dirty_rects_.clear();
    full_redraw_pending_ = false;
}

void RenderPipeline::MergeDirtyRects() {
    // Simple greedy merge of overlapping rects
    bool merged;
    do {
        merged = false;
        for (size_t i = 0; i < dirty_rects_.size() && !merged; i++) {
            for (size_t j = i + 1; j < dirty_rects_.size() && !merged; j++) {
                if (dirty_rects_[i].Overlaps(dirty_rects_[j])) {
                    dirty_rects_[i].Merge(dirty_rects_[j]);
                    dirty_rects_.erase(dirty_rects_.begin() + static_cast<long>(j));
                    merged = true;
                }
            }
        }
    } while (merged && dirty_rects_.size() > MAX_DIRTY_RECTS / 2);
}

void RenderPipeline::ClipToViewport(DirtyRect& rect) const {
    // Clip to screen bounds
    if (rect.x < 0) {
        rect.width += rect.x;
        rect.x = 0;
    }
    if (rect.y < 0) {
        rect.height += rect.y;
        rect.y = 0;
    }
    if (rect.x + rect.width > screen_width_) {
        rect.width = screen_width_ - rect.x;
    }
    if (rect.y + rect.height > screen_height_) {
        rect.height = screen_height_ - rect.y;
    }
}

// =============================================================================
// Renderable Management
// =============================================================================

void RenderPipeline::AddRenderable(IRenderable* obj) {
    if (!obj) return;

    // Check visibility
    int world_x = obj->GetWorldX();
    int world_y = obj->GetWorldY();
    int bx, by, bw, bh;
    obj->GetBounds(bx, by, bw, bh);

    if (!IsVisible(world_x + bx, world_y + by, bw, bh)) {
        return;  // Object not visible, skip
    }

    // Add to render queue
    render_queue_.emplace_back(obj, obj->GetSortY(), obj->GetRenderLayer());
}

void RenderPipeline::ClearRenderables() {
    render_queue_.clear();
}

void RenderPipeline::SortRenderQueue() {
    std::sort(render_queue_.begin(), render_queue_.end());
}

// =============================================================================
// Frame Rendering
// =============================================================================

void RenderPipeline::BeginFrame() {
    stats_.Reset();
    ClearRenderables();
}

void RenderPipeline::RenderFrame() {
    if (!initialized_) return;

    GraphicsBuffer& screen = GraphicsBuffer::Screen();

    // Sort all renderables
    SortRenderQueue();

    screen.Lock();

    // Render each stage
    RenderTerrain();
    RenderSmudges();
    RenderOverlays();
    RenderBibs();
    RenderShadows();
    RenderGroundObjects();
    RenderBuildings();
    RenderAirUnits();
    RenderProjectiles();
    RenderEffects();
    RenderSelection();
    RenderShroud();
    RenderUI();
    RenderCursor();

    // Debug overlay
    if (debug_mode_) {
        DrawDebugOverlay();
    }

    screen.Unlock();
}

void RenderPipeline::EndFrame() {
    GraphicsBuffer& screen = GraphicsBuffer::Screen();
    screen.Flip();

    // Clear dirty state for next frame
    ClearDirtyRects();
}

void RenderPipeline::DrawFrame() {
    BeginFrame();
    RenderFrame();
    EndFrame();
}

// =============================================================================
// Individual Render Stages
// =============================================================================

void RenderPipeline::RenderTerrain() {
    if (!tile_renderer_) return;

    GraphicsBuffer& screen = GraphicsBuffer::Screen();

    // Calculate visible cell range
    int cell_size = 24;  // Tile size in pixels
    int start_cell_x = scroll_x_ / cell_size;
    int start_cell_y = scroll_y_ / cell_size;
    int end_cell_x = (scroll_x_ + tactical_viewport_.width + cell_size - 1) / cell_size;
    int end_cell_y = (scroll_y_ + tactical_viewport_.height + cell_size - 1) / cell_size;

    // Offset for partial cell at edges
    int offset_x = -(scroll_x_ % cell_size);
    int offset_y = -(scroll_y_ % cell_size);

    // Draw visible tiles
    for (int cy = start_cell_y; cy <= end_cell_y; cy++) {
        for (int cx = start_cell_x; cx <= end_cell_x; cx++) {
            int screen_x = tactical_viewport_.x + (cx - start_cell_x) * cell_size + offset_x;
            int screen_y = tactical_viewport_.y + (cy - start_cell_y) * cell_size + offset_y;

            // Draw tile (tile renderer handles out-of-bounds)
            tile_renderer_->DrawTile(screen, screen_x, screen_y, TEMPLATE_CLEAR1, 0);  // Clear terrain as placeholder
            stats_.terrain_tiles_drawn++;
        }
    }
}

void RenderPipeline::RenderSmudges() {
    RenderLayer(RenderLayer::SMUDGE);
}

void RenderPipeline::RenderOverlays() {
    RenderLayer(RenderLayer::OVERLAY);
}

void RenderPipeline::RenderBibs() {
    RenderLayer(RenderLayer::BIB);
}

void RenderPipeline::RenderShadows() {
    RenderLayer(RenderLayer::SHADOW);
}

void RenderPipeline::RenderGroundObjects() {
    RenderLayer(RenderLayer::GROUND);
}

void RenderPipeline::RenderBuildings() {
    RenderLayer(RenderLayer::BUILDING);
}

void RenderPipeline::RenderAirUnits() {
    RenderLayer(RenderLayer::AIR);
}

void RenderPipeline::RenderProjectiles() {
    RenderLayer(RenderLayer::PROJECTILE);
}

void RenderPipeline::RenderEffects() {
    RenderLayer(RenderLayer::EFFECT);
}

void RenderPipeline::RenderSelection() {
    RenderLayer(RenderLayer::SELECTION);
}

void RenderPipeline::RenderShroud() {
    RenderLayer(RenderLayer::SHROUD);
}

void RenderPipeline::RenderUI() {
    GraphicsBuffer& screen = GraphicsBuffer::Screen();

    // Render sidebar
    if (sidebar_) {
        sidebar_->Draw(screen);
    }

    // Render radar
    if (radar_) {
        radar_->Draw(screen);
    }
}

void RenderPipeline::RenderCursor() {
    GraphicsBuffer& screen = GraphicsBuffer::Screen();
    MouseCursor::Instance().Draw(screen);
}

void RenderPipeline::RenderLayer(enum RenderLayer layer) {
    GraphicsBuffer& screen = GraphicsBuffer::Screen();

    for (const auto& entry : render_queue_) {
        if (entry.layer != layer) continue;

        // Convert world to screen coordinates
        int world_x = entry.object->GetWorldX();
        int world_y = entry.object->GetWorldY();
        int screen_x, screen_y;

        if (WorldToScreen(world_x, world_y, &screen_x, &screen_y)) {
            entry.object->Draw(screen, screen_x, screen_y);
            stats_.objects_drawn++;
        }
    }
}

// =============================================================================
// Debug
// =============================================================================

void RenderPipeline::DrawDebugOverlay() {
    GraphicsBuffer& screen = GraphicsBuffer::Screen();

    // Draw dirty rectangles in red
    for (const auto& rect : dirty_rects_) {
        // Top line
        screen.Draw_HLine(rect.x, rect.y, rect.width, 252);
        // Bottom line
        screen.Draw_HLine(rect.x, rect.y + rect.height - 1, rect.width, 252);
        // Left line
        screen.Draw_VLine(rect.x, rect.y, rect.height, 252);
        // Right line
        screen.Draw_VLine(rect.x + rect.width - 1, rect.y, rect.height, 252);

        stats_.dirty_rects_count++;
    }

    // Draw viewport border in green
    screen.Draw_HLine(tactical_viewport_.x, tactical_viewport_.y,
                      tactical_viewport_.width, 250);
    screen.Draw_HLine(tactical_viewport_.x, tactical_viewport_.y + tactical_viewport_.height - 1,
                      tactical_viewport_.width, 250);
    screen.Draw_VLine(tactical_viewport_.x, tactical_viewport_.y,
                      tactical_viewport_.height, 250);
    screen.Draw_VLine(tactical_viewport_.x + tactical_viewport_.width - 1, tactical_viewport_.y,
                      tactical_viewport_.height, 250);
}
