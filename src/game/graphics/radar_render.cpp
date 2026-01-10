/**
 * Radar Renderer Implementation
 */

#include "game/graphics/radar_render.h"
#include "game/graphics/render_pipeline.h"  // For ITerrainProvider
#include "platform.h"
#include <cstring>

// =============================================================================
// Construction
// =============================================================================

RadarRenderer::RadarRenderer()
    : initialized_(false)
    , state_(RadarState::DISABLED)
    , player_house_(0)
    , radar_x_(RADAR_X)
    , radar_y_(RADAR_Y)
    , radar_width_(RADAR_WIDTH)
    , radar_height_(RADAR_HEIGHT)
    , map_width_(128)
    , map_height_(128)
    , scale_x_(1.0f)
    , scale_y_(1.0f)
    , viewport_x_(0)
    , viewport_y_(0)
    , viewport_width_(20)
    , viewport_height_(16)
    , terrain_(nullptr)
    , terrain_dirty_(true)
    , blip_count_(0)
    , blink_counter_(0)
    , blink_rate_(8)
    , blink_state_(false)
    , static_frame_(0)
{
}

RadarRenderer::~RadarRenderer() {
    Shutdown();
}

// =============================================================================
// Initialization
// =============================================================================

bool RadarRenderer::Initialize(int map_width, int map_height) {
    if (initialized_) return true;

    map_width_ = map_width;
    map_height_ = map_height;

    // Calculate scale factors for fitting map to radar
    scale_x_ = static_cast<float>(radar_width_) / static_cast<float>(map_width);
    scale_y_ = static_cast<float>(radar_height_) / static_cast<float>(map_height);

    // Create terrain buffer
    CreateTerrainBuffer();

    initialized_ = true;
    return true;
}

void RadarRenderer::Shutdown() {
    terrain_buffer_.reset();
    terrain_ = nullptr;
    initialized_ = false;
}

void RadarRenderer::CreateTerrainBuffer() {
    int buffer_size = radar_width_ * radar_height_;
    terrain_buffer_ = std::make_unique<uint8_t[]>(static_cast<size_t>(buffer_size));
    std::memset(terrain_buffer_.get(), 0, static_cast<size_t>(buffer_size));
    terrain_dirty_ = true;
}

// =============================================================================
// Viewport
// =============================================================================

void RadarRenderer::SetViewport(int cell_x, int cell_y, int width_cells, int height_cells) {
    viewport_x_ = cell_x;
    viewport_y_ = cell_y;
    viewport_width_ = width_cells;
    viewport_height_ = height_cells;
}

bool RadarRenderer::RadarToCell(int radar_x, int radar_y, int* cell_x, int* cell_y) const {
    // Convert radar pixel to map cell
    int local_x = radar_x - radar_x_;
    int local_y = radar_y - radar_y_;

    if (local_x < 0 || local_x >= radar_width_ ||
        local_y < 0 || local_y >= radar_height_) {
        return false;
    }

    if (cell_x) *cell_x = static_cast<int>(static_cast<float>(local_x) / scale_x_);
    if (cell_y) *cell_y = static_cast<int>(static_cast<float>(local_y) / scale_y_);

    return true;
}

void RadarRenderer::CellToRadar(int cell_x, int cell_y, int* radar_x, int* radar_y) const {
    if (radar_x) *radar_x = radar_x_ + static_cast<int>(static_cast<float>(cell_x) * scale_x_);
    if (radar_y) *radar_y = radar_y_ + static_cast<int>(static_cast<float>(cell_y) * scale_y_);
}

// =============================================================================
// Blip Management
// =============================================================================

void RadarRenderer::ClearBlips() {
    blip_count_ = 0;
}

void RadarRenderer::AddBlip(const RadarBlip& blip) {
    if (blip_count_ < MAX_BLIPS) {
        blips_[blip_count_++] = blip;
    }
}

void RadarRenderer::AddBlip(int cell_x, int cell_y, uint8_t color, bool is_building) {
    RadarBlip blip(cell_x, cell_y, color, is_building);
    AddBlip(blip);
}

// =============================================================================
// Terrain Image
// =============================================================================

void RadarRenderer::UpdateTerrainImage() {
    if (!terrain_buffer_ || !terrain_) return;

    // Fill terrain buffer from map data
    for (int y = 0; y < radar_height_; y++) {
        int cell_y = static_cast<int>(static_cast<float>(y) / scale_y_);

        for (int x = 0; x < radar_width_; x++) {
            int cell_x = static_cast<int>(static_cast<float>(x) / scale_x_);

            uint8_t color = GetTerrainColor(cell_x, cell_y);
            terrain_buffer_[static_cast<size_t>(y * radar_width_ + x)] = color;
        }
    }

    terrain_dirty_ = false;
}

uint8_t RadarRenderer::GetTerrainColor(int cell_x, int cell_y) const {
    if (!terrain_ || !terrain_->IsValidCell(cell_x, cell_y)) {
        return 0;  // Black for invalid
    }

    // Get terrain tile and return appropriate color
    // These colors approximate C&C radar colors
    int tile = terrain_->GetTerrainTile(cell_x, cell_y);

    // Simple color mapping based on tile type
    // In the real game, this would be based on theater palette
    if (tile < 0) return 0;       // Invalid
    if (tile < 16) return 175;    // Clear - light green
    if (tile < 32) return 173;    // Water - blue
    if (tile < 48) return 176;    // Shore - tan
    if (tile < 64) return 174;    // Rough - dark green
    if (tile < 80) return 177;    // Road - gray
    if (tile < 96) return 178;    // Rock - dark gray

    return 175;  // Default green
}

// =============================================================================
// Layout
// =============================================================================

void RadarRenderer::SetPosition(int x, int y) {
    radar_x_ = x;
    radar_y_ = y;
}

// =============================================================================
// Animation
// =============================================================================

void RadarRenderer::Update() {
    // Update blink animation
    blink_counter_++;
    if (blink_counter_ >= blink_rate_) {
        blink_counter_ = 0;
        blink_state_ = !blink_state_;
    }

    // Update static animation for jammed state
    if (state_ == RadarState::JAMMED) {
        static_frame_ = (static_frame_ + 1) % 8;
    }
}

// =============================================================================
// Rendering
// =============================================================================

void RadarRenderer::Draw(GraphicsBuffer& buffer) {
    if (!initialized_) return;

    DrawFrame(buffer);

    switch (state_) {
        case RadarState::DISABLED:
            // Draw dark/empty radar
            buffer.Fill_Rect(radar_x_, radar_y_, radar_width_, radar_height_, 0);
            break;

        case RadarState::JAMMED:
            DrawJammed(buffer);
            break;

        case RadarState::ACTIVE:
        case RadarState::SPYING:
            DrawTerrain(buffer);
            DrawBlips(buffer);
            DrawViewport(buffer);
            break;
    }
}

void RadarRenderer::DrawFrame(GraphicsBuffer& buffer) {
    // Draw border around radar
    int x1 = radar_x_ - 1;
    int y1 = radar_y_ - 1;
    int x2 = radar_x_ + radar_width_;
    int y2 = radar_y_ + radar_height_;

    // Top and bottom
    buffer.Draw_HLine(x1, y1, radar_width_ + 2, 0);
    buffer.Draw_HLine(x1, y2, radar_width_ + 2, 0);

    // Left and right
    buffer.Draw_VLine(x1, y1, radar_height_ + 2, 0);
    buffer.Draw_VLine(x2, y1, radar_height_ + 2, 0);
}

void RadarRenderer::DrawTerrain(GraphicsBuffer& buffer) {
    if (!terrain_buffer_) return;

    // Update terrain if dirty
    if (terrain_dirty_ && terrain_) {
        UpdateTerrainImage();
    }

    // Copy terrain buffer to screen
    for (int y = 0; y < radar_height_; y++) {
        for (int x = 0; x < radar_width_; x++) {
            uint8_t color = terrain_buffer_[static_cast<size_t>(y * radar_width_ + x)];
            buffer.Put_Pixel(radar_x_ + x, radar_y_ + y, color);
        }
    }
}

void RadarRenderer::DrawBlips(GraphicsBuffer& buffer) {
    for (int i = 0; i < blip_count_; i++) {
        const RadarBlip& blip = blips_[i];

        // Skip selected blips on blink-off frames
        if (blip.is_selected && !blink_state_) continue;

        // Convert cell to radar position
        int rx, ry;
        CellToRadar(blip.cell_x, blip.cell_y, &rx, &ry);

        // Draw blip
        if (blip.is_building) {
            // Buildings are 2x2 pixels
            DrawPixel(buffer, rx, ry, blip.color);
            DrawPixel(buffer, rx + 1, ry, blip.color);
            DrawPixel(buffer, rx, ry + 1, blip.color);
            DrawPixel(buffer, rx + 1, ry + 1, blip.color);
        } else {
            // Units are 1 pixel
            DrawPixel(buffer, rx, ry, blip.color);
        }
    }
}

void RadarRenderer::DrawViewport(GraphicsBuffer& buffer) {
    // Convert viewport cell coordinates to radar coordinates
    int vx1, vy1, vx2, vy2;
    CellToRadar(viewport_x_, viewport_y_, &vx1, &vy1);
    CellToRadar(viewport_x_ + viewport_width_, viewport_y_ + viewport_height_, &vx2, &vy2);

    // Clamp to radar bounds
    if (vx1 < radar_x_) vx1 = radar_x_;
    if (vy1 < radar_y_) vy1 = radar_y_;
    if (vx2 > radar_x_ + radar_width_) vx2 = radar_x_ + radar_width_;
    if (vy2 > radar_y_ + radar_height_) vy2 = radar_y_ + radar_height_;

    int vw = vx2 - vx1;
    int vh = vy2 - vy1;

    // Draw viewport rectangle in white
    uint8_t color = 15;  // White

    // Top and bottom
    buffer.Draw_HLine(vx1, vy1, vw, color);
    buffer.Draw_HLine(vx1, vy2 - 1, vw, color);

    // Left and right
    buffer.Draw_VLine(vx1, vy1, vh, color);
    buffer.Draw_VLine(vx2 - 1, vy1, vh, color);
}

void RadarRenderer::DrawJammed(GraphicsBuffer& buffer) {
    // Draw random static effect
    for (int y = 0; y < radar_height_; y++) {
        for (int x = 0; x < radar_width_; x++) {
            // Simple pseudo-random pattern based on position and frame
            int noise = ((x * 7 + y * 11 + static_frame_ * 13) % 16);
            uint8_t color = static_cast<uint8_t>(noise + 8);  // Gray tones
            buffer.Put_Pixel(radar_x_ + x, radar_y_ + y, color);
        }
    }
}

void RadarRenderer::DrawPixel(GraphicsBuffer& buffer, int x, int y, uint8_t color) {
    // Bounds check for radar area
    if (x >= radar_x_ && x < radar_x_ + radar_width_ &&
        y >= radar_y_ && y < radar_y_ + radar_height_) {
        buffer.Put_Pixel(x, y, color);
    }
}

// =============================================================================
// Hit Testing
// =============================================================================

bool RadarRenderer::HitTest(int x, int y) const {
    return (x >= radar_x_ && x < radar_x_ + radar_width_ &&
            y >= radar_y_ && y < radar_y_ + radar_height_);
}
