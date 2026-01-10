/**
 * MouseCursor Implementation
 */

#include "game/graphics/mouse_cursor.h"
#include "game/graphics/graphics_buffer.h"
#include "game/graphics/shape_renderer.h"
#include "platform.h"

// =============================================================================
// Static Data
// =============================================================================

CursorHotspot MouseCursor::s_hotspots[CURSOR_COUNT];
bool MouseCursor::s_hotspots_initialized = false;

// =============================================================================
// Singleton
// =============================================================================

MouseCursor& MouseCursor::Instance() {
    static MouseCursor instance;
    return instance;
}

MouseCursor::MouseCursor()
    : current_type_(CURSOR_NORMAL)
    , base_type_(CURSOR_NORMAL)
    , state_(CURSOR_STATE_NONE)
    , anim_frame_(0)
    , anim_delay_(4)
    , anim_counter_(0)
    , scale_(2)
{
    InitHotspots();
}

MouseCursor::~MouseCursor() {
    Unload();
}

// =============================================================================
// Initialization
// =============================================================================

void MouseCursor::InitHotspots() {
    if (s_hotspots_initialized) return;

    // Default all hotspots to (0, 0)
    for (int i = 0; i < CURSOR_COUNT; i++) {
        s_hotspots[i] = {0, 0};
    }

    // Arrow cursor - hotspot at tip (top-left)
    s_hotspots[CURSOR_NORMAL] = {0, 0};

    // Scroll cursors - hotspot at center
    for (int i = CURSOR_SCROLL_N; i <= CURSOR_NO_SCROLL; i++) {
        s_hotspots[i] = {12, 12};
    }

    // Action cursors - hotspot at center
    s_hotspots[CURSOR_MOVE] = {12, 12};
    s_hotspots[CURSOR_NO_MOVE] = {12, 12};
    s_hotspots[CURSOR_ENTER] = {12, 12};
    s_hotspots[CURSOR_DEPLOY] = {12, 12};

    // Crosshair cursors - hotspot at exact center
    for (int i = CURSOR_SELECT; i <= CURSOR_SELECT_ANIM3; i++) {
        s_hotspots[i] = {15, 15};
    }
    for (int i = CURSOR_ATTACK; i <= CURSOR_ATTACK_ANIM3; i++) {
        s_hotspots[i] = {15, 15};
    }

    // Icon cursors - hotspot at center
    s_hotspots[CURSOR_SELL] = {12, 12};
    s_hotspots[CURSOR_SELL_OK] = {12, 12};
    s_hotspots[CURSOR_REPAIR] = {12, 12};
    s_hotspots[CURSOR_REPAIR_OK] = {12, 12};
    s_hotspots[CURSOR_NO_SELL] = {12, 12};
    s_hotspots[CURSOR_NO_REPAIR] = {12, 12};

    s_hotspots[CURSOR_GUARD] = {12, 12};
    s_hotspots[CURSOR_WAYPOINT] = {12, 12};

    // Nuke cursor - hotspot at bottom center (bomb drop point)
    for (int i = CURSOR_NUKE; i <= CURSOR_NUKE_ANIM3; i++) {
        s_hotspots[i] = {15, 30};
    }

    s_hotspots[CURSOR_ION] = {15, 15};
    s_hotspots[CURSOR_AIRSTRIKE] = {15, 15};
    s_hotspots[CURSOR_CHRONO] = {15, 15};

    s_hotspots[CURSOR_CAPTURE] = {12, 12};
    s_hotspots[CURSOR_NO_CAPTURE] = {12, 12};
    s_hotspots[CURSOR_HARVEST] = {12, 12};
    s_hotspots[CURSOR_NO_HARVEST] = {12, 12};

    s_hotspots_initialized = true;
}

bool MouseCursor::Load(const char* filename) {
    Unload();

    shape_ = std::make_unique<ShapeRenderer>();
    if (!shape_->Load(filename)) {
        shape_.reset();
        Platform_LogWarn("Failed to load mouse cursor shape");
        return false;
    }

    // Hide system cursor
    Platform_Mouse_Hide();

    return true;
}

void MouseCursor::Unload() {
    shape_.reset();

    // Show system cursor
    Platform_Mouse_Show();
}

// =============================================================================
// Cursor Type
// =============================================================================

void MouseCursor::SetType(CursorType type) {
    if (IsLocked()) return;

    if (type >= 0 && type < CURSOR_COUNT) {
        // Reset animation when cursor type changes
        if (type != base_type_) {
            anim_frame_ = 0;
            anim_counter_ = 0;
        }

        base_type_ = type;
        current_type_ = type;
    }
}

// =============================================================================
// State Control
// =============================================================================

void MouseCursor::Show() {
    state_ &= ~CURSOR_STATE_HIDDEN;
}

void MouseCursor::Hide() {
    state_ |= CURSOR_STATE_HIDDEN;
}

void MouseCursor::Lock() {
    state_ |= CURSOR_STATE_LOCKED;
}

void MouseCursor::Unlock() {
    state_ &= ~CURSOR_STATE_LOCKED;
}

// =============================================================================
// Position
// =============================================================================

void MouseCursor::GetPosition(int* x, int* y) const {
    int window_x, window_y;
    Platform_Mouse_GetPosition(&window_x, &window_y);

    // Convert from window coordinates to game coordinates
    int game_x = window_x / scale_;
    int game_y = window_y / scale_;

    if (x) *x = game_x;
    if (y) *y = game_y;
}

int MouseCursor::GetX() const {
    int x;
    GetPosition(&x, nullptr);
    return x;
}

int MouseCursor::GetY() const {
    int y;
    GetPosition(nullptr, &y);
    return y;
}

const CursorHotspot& MouseCursor::GetHotspot() const {
    int type = current_type_;
    if (type < 0 || type >= CURSOR_COUNT) {
        type = 0;
    }
    return s_hotspots[type];
}

void MouseCursor::GetClickPosition(int* x, int* y) const {
    int mx, my;
    GetPosition(&mx, &my);

    const CursorHotspot& hotspot = GetHotspot();

    if (x) *x = mx + hotspot.x;
    if (y) *y = my + hotspot.y;
}

// =============================================================================
// Animation
// =============================================================================

bool MouseCursor::IsAnimated() const {
    switch (base_type_) {
        case CURSOR_SELECT:
        case CURSOR_ATTACK:
        case CURSOR_NUKE:
            return true;
        default:
            return false;
    }
}

int MouseCursor::GetAnimationFrameCount() const {
    switch (base_type_) {
        case CURSOR_SELECT:
        case CURSOR_ATTACK:
        case CURSOR_NUKE:
            return 4;
        default:
            return 1;
    }
}

void MouseCursor::SetAnimationSpeed(int frame_delay) {
    anim_delay_ = frame_delay > 0 ? frame_delay : 1;
}

void MouseCursor::GetAnimationInfo(CursorType type, int* base_frame, int* frame_count) const {
    switch (type) {
        case CURSOR_SELECT:
            *base_frame = CURSOR_SELECT;
            *frame_count = 4;
            break;
        case CURSOR_ATTACK:
            *base_frame = CURSOR_ATTACK;
            *frame_count = 4;
            break;
        case CURSOR_NUKE:
            *base_frame = CURSOR_NUKE;
            *frame_count = 4;
            break;
        default:
            *base_frame = type;
            *frame_count = 1;
            break;
    }
}

// =============================================================================
// Update & Draw
// =============================================================================

void MouseCursor::Update() {
    if (!IsAnimated()) {
        current_type_ = base_type_;
        return;
    }

    // Advance animation
    anim_counter_++;
    if (anim_counter_ >= anim_delay_) {
        anim_counter_ = 0;

        int base_frame, frame_count;
        GetAnimationInfo(base_type_, &base_frame, &frame_count);

        anim_frame_ = (anim_frame_ + 1) % frame_count;
        current_type_ = static_cast<CursorType>(base_frame + anim_frame_);
    }
}

void MouseCursor::Draw(GraphicsBuffer& buffer) {
    if (IsHidden() || !IsLoaded()) return;

    int x, y;
    GetPosition(&x, &y);
    DrawAt(buffer, x, y);
}

void MouseCursor::DrawAt(GraphicsBuffer& buffer, int x, int y) {
    if (IsHidden() || !IsLoaded()) return;

    // Get hotspot and adjust position
    const CursorHotspot& hotspot = GetHotspot();
    int draw_x = x - hotspot.x;
    int draw_y = y - hotspot.y;

    // Draw cursor sprite
    int frame = GetFrameForType(current_type_);
    shape_->Draw(buffer, draw_x, draw_y, frame);
}

int MouseCursor::GetFrameForType(CursorType type) const {
    // Direct mapping - frame index equals cursor type
    return static_cast<int>(type);
}

// =============================================================================
// Scroll Edge Detection
// =============================================================================

void MouseCursor::SetScrollCursor(int dx, int dy) {
    // Map direction to scroll cursor
    // Direction: (-1,-1) = NW, (0,-1) = N, (1,-1) = NE, etc.

    if (dx == 0 && dy == 0) {
        SetType(CURSOR_NORMAL);
        return;
    }

    CursorType scroll_type = CURSOR_NO_SCROLL;

    if (dy < 0) {
        // North
        if (dx < 0) scroll_type = CURSOR_SCROLL_NW;
        else if (dx > 0) scroll_type = CURSOR_SCROLL_NE;
        else scroll_type = CURSOR_SCROLL_N;
    }
    else if (dy > 0) {
        // South
        if (dx < 0) scroll_type = CURSOR_SCROLL_SW;
        else if (dx > 0) scroll_type = CURSOR_SCROLL_SE;
        else scroll_type = CURSOR_SCROLL_S;
    }
    else {
        // East/West
        if (dx < 0) scroll_type = CURSOR_SCROLL_W;
        else scroll_type = CURSOR_SCROLL_E;
    }

    SetType(scroll_type);
}

bool MouseCursor::CheckScrollEdge(int mouse_x, int mouse_y,
                                   int screen_width, int screen_height,
                                   int edge_size) {
    int dx = 0, dy = 0;

    if (mouse_x < edge_size) dx = -1;
    else if (mouse_x >= screen_width - edge_size) dx = 1;

    if (mouse_y < edge_size) dy = -1;
    else if (mouse_y >= screen_height - edge_size) dy = 1;

    if (dx != 0 || dy != 0) {
        SetScrollCursor(dx, dy);
        return true;
    }

    return false;
}
