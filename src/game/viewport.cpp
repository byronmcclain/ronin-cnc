/**
 * Viewport Implementation
 */

#include "game/viewport.h"
#include <cstdlib>  // For abs()

// Global viewport reference
GameViewport& TheViewport = GameViewport::Instance();

// =============================================================================
// Singleton
// =============================================================================

GameViewport::GameViewport()
    : x(0)
    , y(0)
    , width(TACTICAL_WIDTH)
    , height(TACTICAL_HEIGHT)
    , map_width_(64)
    , map_height_(64)
    , scroll_enabled_(true)
    , scroll_speed_multiplier_(100)
    , current_scroll_direction_(SCROLL_NONE)
    , scroll_accel_counter_(0)
    , tracking_enabled_(false)
    , track_target_(0)
{
}

GameViewport& GameViewport::Instance() {
    static GameViewport instance;
    return instance;
}

// =============================================================================
// Initialization
// =============================================================================

void GameViewport::Initialize() {
    x = 0;
    y = 0;
    width = TACTICAL_WIDTH;
    height = TACTICAL_HEIGHT;
    scroll_enabled_ = true;
    current_scroll_direction_ = SCROLL_NONE;
    scroll_accel_counter_ = 0;
    tracking_enabled_ = false;
}

void GameViewport::Reset() {
    Initialize();
}

void GameViewport::SetMapSize(int cells_wide, int cells_high) {
    map_width_ = cells_wide;
    map_height_ = cells_high;
    ClampToBounds();
}

// =============================================================================
// Scrolling
// =============================================================================

void GameViewport::Scroll(int delta_x, int delta_y) {
    if (!scroll_enabled_) return;

    // Apply scroll speed multiplier
    delta_x = (delta_x * scroll_speed_multiplier_) / 100;
    delta_y = (delta_y * scroll_speed_multiplier_) / 100;

    x += delta_x;
    y += delta_y;

    ClampToBounds();
}

void GameViewport::ScrollTo(int world_x, int world_y) {
    x = world_x;
    y = world_y;
    ClampToBounds();
}

void GameViewport::CenterOn(int world_x, int world_y) {
    x = world_x - width / 2;
    y = world_y - height / 2;
    ClampToBounds();
}

void GameViewport::CenterOnCell(int cell_x, int cell_y) {
    int world_x = cell_x * TILE_PIXEL_WIDTH + TILE_PIXEL_WIDTH / 2;
    int world_y = cell_y * TILE_PIXEL_HEIGHT + TILE_PIXEL_HEIGHT / 2;
    CenterOn(world_x, world_y);
}

void GameViewport::CenterOnCoord(COORDINATE coord) {
    int lepton_x = COORD_X(coord);
    int lepton_y = COORD_Y(coord);
    int pixel_x, pixel_y;
    LeptonToPixel(lepton_x, lepton_y, pixel_x, pixel_y);
    CenterOn(pixel_x, pixel_y);
}

void GameViewport::JumpTo(int world_x, int world_y) {
    ScrollTo(world_x, world_y);
}

// =============================================================================
// Edge Scrolling
// =============================================================================

void GameViewport::UpdateEdgeScroll(int mouse_x, int mouse_y) {
    if (!scroll_enabled_) {
        current_scroll_direction_ = SCROLL_NONE;
        scroll_accel_counter_ = 0;
        return;
    }

    // Check if mouse is in tactical area
    if (mouse_x >= TACTICAL_WIDTH || mouse_y < VP_TAB_HEIGHT) {
        current_scroll_direction_ = SCROLL_NONE;
        scroll_accel_counter_ = 0;
        return;
    }

    uint8_t scroll_dir = SCROLL_NONE;
    int scroll_x = 0;
    int scroll_y = 0;

    // Adjust mouse_y for tactical area (subtract tab height)
    int tactical_y = mouse_y - VP_TAB_HEIGHT;

    // Check left edge
    if (mouse_x < EDGE_SCROLL_ZONE) {
        int distance = EDGE_SCROLL_ZONE - mouse_x;
        scroll_x = -CalculateEdgeScrollSpeed(distance);
        scroll_dir |= SCROLL_LEFT;
    }
    // Check right edge
    else if (mouse_x >= TACTICAL_WIDTH - EDGE_SCROLL_ZONE) {
        int distance = mouse_x - (TACTICAL_WIDTH - EDGE_SCROLL_ZONE);
        scroll_x = CalculateEdgeScrollSpeed(distance);
        scroll_dir |= SCROLL_RIGHT;
    }

    // Check top edge
    if (tactical_y < EDGE_SCROLL_ZONE) {
        int distance = EDGE_SCROLL_ZONE - tactical_y;
        scroll_y = -CalculateEdgeScrollSpeed(distance);
        scroll_dir |= SCROLL_UP;
    }
    // Check bottom edge
    else if (tactical_y >= TACTICAL_HEIGHT - EDGE_SCROLL_ZONE) {
        int distance = tactical_y - (TACTICAL_HEIGHT - EDGE_SCROLL_ZONE);
        scroll_y = CalculateEdgeScrollSpeed(distance);
        scroll_dir |= SCROLL_DOWN;
    }

    // Update acceleration counter
    if (scroll_dir != SCROLL_NONE) {
        if (scroll_accel_counter_ < SCROLL_ACCEL_FRAMES) {
            scroll_accel_counter_++;
        }
    } else {
        scroll_accel_counter_ = 0;
    }

    current_scroll_direction_ = scroll_dir;

    // Apply scroll
    if (scroll_x != 0 || scroll_y != 0) {
        Scroll(scroll_x, scroll_y);
    }
}

void GameViewport::UpdateKeyboardScroll(bool up, bool down, bool left, bool right) {
    if (!scroll_enabled_) return;

    int scroll_x = 0;
    int scroll_y = 0;
    uint8_t scroll_dir = SCROLL_NONE;

    if (up) {
        scroll_y = -KEYBOARD_SCROLL_SPEED;
        scroll_dir |= SCROLL_UP;
    }
    if (down) {
        scroll_y = KEYBOARD_SCROLL_SPEED;
        scroll_dir |= SCROLL_DOWN;
    }
    if (left) {
        scroll_x = -KEYBOARD_SCROLL_SPEED;
        scroll_dir |= SCROLL_LEFT;
    }
    if (right) {
        scroll_x = KEYBOARD_SCROLL_SPEED;
        scroll_dir |= SCROLL_RIGHT;
    }

    if (scroll_dir != SCROLL_NONE) {
        current_scroll_direction_ = scroll_dir;
        Scroll(scroll_x, scroll_y);
    }
}

int GameViewport::CalculateEdgeScrollSpeed(int distance_into_zone) const {
    // Base speed varies linearly with distance into edge zone
    float zone_ratio = static_cast<float>(distance_into_zone) / EDGE_SCROLL_ZONE;
    int base_speed = MIN_SCROLL_SPEED +
        static_cast<int>(static_cast<float>(MAX_SCROLL_SPEED - MIN_SCROLL_SPEED) * zone_ratio);

    // Apply acceleration based on how long we've been scrolling
    float accel_ratio = static_cast<float>(scroll_accel_counter_) / SCROLL_ACCEL_FRAMES;
    int accel_bonus = static_cast<int>(static_cast<float>(base_speed) * accel_ratio * 0.5f);

    return base_speed + accel_bonus;
}

// =============================================================================
// Target Tracking
// =============================================================================

void GameViewport::SetTrackTarget(COORDINATE coord) {
    track_target_ = coord;
    tracking_enabled_ = true;
}

void GameViewport::ClearTrackTarget() {
    tracking_enabled_ = false;
}

void GameViewport::UpdateTracking() {
    if (!tracking_enabled_) return;

    // Calculate target position in pixels
    int target_pixel_x, target_pixel_y;
    LeptonToPixel(COORD_X(track_target_), COORD_Y(track_target_), target_pixel_x, target_pixel_y);

    // Calculate center of viewport
    int center_x = x + width / 2;
    int center_y = y + height / 2;

    // Calculate distance to target
    int dx = target_pixel_x - center_x;
    int dy = target_pixel_y - center_y;

    // Dead zone - don't scroll if target is close to center
    int dead_zone = 100;

    if (abs(dx) > dead_zone || abs(dy) > dead_zone) {
        // Smooth scroll - move fraction of distance each frame
        int move_x = dx / 8;
        int move_y = dy / 8;

        // Ensure minimum movement
        if (move_x > 0 && move_x < 1) move_x = 1;
        if (move_x < 0 && move_x > -1) move_x = -1;
        if (move_y > 0 && move_y < 1) move_y = 1;
        if (move_y < 0 && move_y > -1) move_y = -1;

        Scroll(move_x, move_y);
    }
}

// =============================================================================
// Coordinate Conversion
// =============================================================================

void GameViewport::WorldToScreen(int world_x, int world_y, int& screen_x, int& screen_y) const {
    screen_x = world_x - x;
    screen_y = world_y - y + VP_TAB_HEIGHT;
}

void GameViewport::ScreenToWorld(int screen_x, int screen_y, int& world_x, int& world_y) const {
    world_x = screen_x + x;
    world_y = screen_y - VP_TAB_HEIGHT + y;
}

void GameViewport::WorldToCell(int world_x, int world_y, int& cell_x, int& cell_y) const {
    cell_x = world_x / TILE_PIXEL_WIDTH;
    cell_y = world_y / TILE_PIXEL_HEIGHT;
}

void GameViewport::CellToWorld(int cell_x, int cell_y, int& world_x, int& world_y) const {
    world_x = cell_x * TILE_PIXEL_WIDTH;
    world_y = cell_y * TILE_PIXEL_HEIGHT;
}

void GameViewport::ScreenToCell(int screen_x, int screen_y, int& cell_x, int& cell_y) const {
    int world_x, world_y;
    ScreenToWorld(screen_x, screen_y, world_x, world_y);
    WorldToCell(world_x, world_y, cell_x, cell_y);
}

void GameViewport::CellToScreen(int cell_x, int cell_y, int& screen_x, int& screen_y) const {
    int world_x, world_y;
    CellToWorld(cell_x, cell_y, world_x, world_y);
    WorldToScreen(world_x, world_y, screen_x, screen_y);
}

void GameViewport::LeptonToPixel(int lepton_x, int lepton_y, int& pixel_x, int& pixel_y) const {
    // 256 leptons = 1 cell = 24 pixels
    pixel_x = (lepton_x * TILE_PIXEL_WIDTH) / LEPTONS_PER_CELL;
    pixel_y = (lepton_y * TILE_PIXEL_HEIGHT) / LEPTONS_PER_CELL;
}

void GameViewport::PixelToLepton(int pixel_x, int pixel_y, int& lepton_x, int& lepton_y) const {
    lepton_x = (pixel_x * LEPTONS_PER_CELL) / TILE_PIXEL_WIDTH;
    lepton_y = (pixel_y * LEPTONS_PER_CELL) / TILE_PIXEL_HEIGHT;
}

void GameViewport::CoordToScreen(COORDINATE coord, int& screen_x, int& screen_y) const {
    int pixel_x, pixel_y;
    LeptonToPixel(COORD_X(coord), COORD_Y(coord), pixel_x, pixel_y);
    WorldToScreen(pixel_x, pixel_y, screen_x, screen_y);
}

// =============================================================================
// Visibility Testing
// =============================================================================

bool GameViewport::IsPointVisible(int world_x, int world_y) const {
    return (world_x >= x && world_x < x + width &&
            world_y >= y && world_y < y + height);
}

bool GameViewport::IsRectVisible(int world_x, int world_y, int rect_width, int rect_height) const {
    return !(world_x + rect_width < x || world_x > x + width ||
             world_y + rect_height < y || world_y > y + height);
}

bool GameViewport::IsCellVisible(int cell_x, int cell_y) const {
    int world_x = cell_x * TILE_PIXEL_WIDTH;
    int world_y = cell_y * TILE_PIXEL_HEIGHT;
    return IsRectVisible(world_x, world_y, TILE_PIXEL_WIDTH, TILE_PIXEL_HEIGHT);
}

bool GameViewport::IsCoordVisible(COORDINATE coord) const {
    int pixel_x, pixel_y;
    LeptonToPixel(COORD_X(coord), COORD_Y(coord), pixel_x, pixel_y);
    return IsPointVisible(pixel_x, pixel_y);
}

void GameViewport::GetVisibleCellRange(int& start_x, int& start_y, int& end_x, int& end_y) const {
    start_x = x / TILE_PIXEL_WIDTH;
    start_y = y / TILE_PIXEL_HEIGHT;
    end_x = (x + width) / TILE_PIXEL_WIDTH + 1;
    end_y = (y + height) / TILE_PIXEL_HEIGHT + 1;

    // Clamp to map bounds
    if (start_x < 0) start_x = 0;
    if (start_y < 0) start_y = 0;
    if (end_x > map_width_) end_x = map_width_;
    if (end_y > map_height_) end_y = map_height_;
}

// =============================================================================
// Bounds Clamping
// =============================================================================

void GameViewport::ClampToBounds() {
    int max_x = GetMapPixelWidth() - width;
    int max_y = GetMapPixelHeight() - height;

    if (max_x < 0) max_x = 0;
    if (max_y < 0) max_y = 0;

    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x > max_x) x = max_x;
    if (y > max_y) y = max_y;
}
