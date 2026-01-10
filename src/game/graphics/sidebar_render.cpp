/**
 * Sidebar Renderer Implementation
 */

#include "game/graphics/sidebar_render.h"
#include "game/graphics/shape_renderer.h"
#include "platform.h"

// =============================================================================
// Construction
// =============================================================================

SidebarRenderer::SidebarRenderer()
    : initialized_(false)
    , sidebar_x_(SIDEBAR_X)
    , sidebar_y_(0)
    , sidebar_height_(400)
    , active_tab_(SidebarTab::STRUCTURE)
    , scroll_position_(0)
    , repair_state_(ButtonState::NORMAL)
    , sell_state_(ButtonState::NORMAL)
    , map_state_(ButtonState::NORMAL)
{
}

SidebarRenderer::~SidebarRenderer() {
    Shutdown();
}

// =============================================================================
// Initialization
// =============================================================================

bool SidebarRenderer::Initialize(int screen_height) {
    if (initialized_) return true;

    sidebar_height_ = screen_height;
    sidebar_y_ = 0;

    // Clear build items
    for (int i = 0; i < static_cast<int>(SidebarTab::COUNT); i++) {
        build_items_[i].clear();
    }

    initialized_ = true;
    return true;
}

bool SidebarRenderer::LoadGraphics(const char* icons_filename, const char* buttons_filename) {
    if (icons_filename) {
        icons_ = std::make_unique<ShapeRenderer>();
        if (!icons_->Load(icons_filename)) {
            Platform_LogWarn("Failed to load sidebar icons");
            icons_.reset();
        }
    }

    if (buttons_filename) {
        buttons_ = std::make_unique<ShapeRenderer>();
        if (!buttons_->Load(buttons_filename)) {
            Platform_LogWarn("Failed to load sidebar buttons");
            buttons_.reset();
        }
    }

    return true;
}

void SidebarRenderer::Shutdown() {
    icons_.reset();
    buttons_.reset();

    for (int i = 0; i < static_cast<int>(SidebarTab::COUNT); i++) {
        build_items_[i].clear();
    }

    initialized_ = false;
}

// =============================================================================
// Tab Control
// =============================================================================

void SidebarRenderer::SetActiveTab(SidebarTab tab) {
    if (tab >= SidebarTab::STRUCTURE && tab < SidebarTab::COUNT) {
        active_tab_ = tab;
        scroll_position_ = 0;  // Reset scroll when changing tabs
    }
}

void SidebarRenderer::SetScrollPosition(int position) {
    int max_scroll = GetMaxScrollPosition();
    scroll_position_ = (position < 0) ? 0 : (position > max_scroll) ? max_scroll : position;
}

int SidebarRenderer::GetMaxScrollPosition() const {
    int items = static_cast<int>(build_items_[static_cast<int>(active_tab_)].size());
    int visible_rows = ICONS_PER_COLUMN;
    int total_rows = (items + ICON_COLUMNS - 1) / ICON_COLUMNS;

    return (total_rows > visible_rows) ? total_rows - visible_rows : 0;
}

void SidebarRenderer::ScrollUp() {
    if (scroll_position_ > 0) {
        scroll_position_--;
    }
}

void SidebarRenderer::ScrollDown() {
    if (scroll_position_ < GetMaxScrollPosition()) {
        scroll_position_++;
    }
}

// =============================================================================
// Build Queue
// =============================================================================

void SidebarRenderer::ClearBuildItems() {
    build_items_[static_cast<int>(active_tab_)].clear();
    scroll_position_ = 0;
}

void SidebarRenderer::AddBuildItem(int type_id, int icon_frame) {
    BuildQueueItem item;
    item.type_id = type_id;
    item.icon_frame = icon_frame;
    item.progress = 0.0f;
    item.on_hold = false;
    item.queue_count = 0;

    build_items_[static_cast<int>(active_tab_)].push_back(item);
}

void SidebarRenderer::SetBuildProgress(int type_id, float progress) {
    auto& items = build_items_[static_cast<int>(active_tab_)];
    for (auto& item : items) {
        if (item.type_id == type_id) {
            item.progress = progress;
            break;
        }
    }
}

void SidebarRenderer::SetBuildOnHold(int type_id, bool on_hold) {
    auto& items = build_items_[static_cast<int>(active_tab_)];
    for (auto& item : items) {
        if (item.type_id == type_id) {
            item.on_hold = on_hold;
            break;
        }
    }
}

// =============================================================================
// Rendering
// =============================================================================

void SidebarRenderer::Draw(GraphicsBuffer& buffer) {
    if (!initialized_) return;

    DrawBackground(buffer);
    DrawTabs(buffer);
    DrawBuildIcons(buffer);
    DrawScrollArrows(buffer);
    DrawButtons(buffer);
}

void SidebarRenderer::DrawBackground(GraphicsBuffer& buffer) {
    // Fill sidebar background with dark gray
    buffer.Fill_Rect(sidebar_x_, sidebar_y_, SIDEBAR_WIDTH, sidebar_height_, 14);

    // Draw border line on left edge
    buffer.Draw_VLine(sidebar_x_, sidebar_y_, sidebar_height_, 0);
}

void SidebarRenderer::DrawTabs(GraphicsBuffer& buffer) {
    int tab_y = sidebar_y_;

    for (int i = 0; i < static_cast<int>(SidebarTab::COUNT); i++) {
        int tab_x = sidebar_x_ + (i * TAB_WIDTH);
        bool active = (i == static_cast<int>(active_tab_));

        // Draw tab background
        uint8_t bg_color = active ? static_cast<uint8_t>(15) : static_cast<uint8_t>(13);
        buffer.Fill_Rect(tab_x, tab_y, TAB_WIDTH, TAB_HEIGHT, bg_color);

        // Draw tab border
        buffer.Draw_HLine(tab_x, tab_y, TAB_WIDTH, 0);
        buffer.Draw_HLine(tab_x, tab_y + TAB_HEIGHT - 1, TAB_WIDTH, 0);
        buffer.Draw_VLine(tab_x, tab_y, TAB_HEIGHT, 0);
        buffer.Draw_VLine(tab_x + TAB_WIDTH - 1, tab_y, TAB_HEIGHT, 0);

        // Active tab highlight
        if (active) {
            buffer.Draw_HLine(tab_x + 1, tab_y + 1, TAB_WIDTH - 2, 253);
        }
    }
}

void SidebarRenderer::DrawBuildIcons(GraphicsBuffer& buffer) {
    const auto& items = build_items_[static_cast<int>(active_tab_)];

    int start_y = sidebar_y_ + TAB_HEIGHT + 4;
    int icon_area_x = sidebar_x_ + 8;
    int icon_area_y = start_y;

    int visible_start = scroll_position_ * ICON_COLUMNS;
    int visible_end = visible_start + (ICONS_PER_COLUMN * ICON_COLUMNS);

    for (int i = visible_start; i < visible_end && i < static_cast<int>(items.size()); i++) {
        int local_index = i - visible_start;
        int row = local_index / ICON_COLUMNS;
        int col = local_index % ICON_COLUMNS;

        int icon_x = icon_area_x + col * (ICON_WIDTH + 4);
        int icon_y = icon_area_y + row * (ICON_HEIGHT + 4);

        DrawBuildIcon(buffer, icon_x, icon_y, items[static_cast<size_t>(i)]);
    }
}

void SidebarRenderer::DrawBuildIcon(GraphicsBuffer& buffer, int x, int y, const BuildQueueItem& item) {
    // Draw icon background
    buffer.Fill_Rect(x, y, ICON_WIDTH, ICON_HEIGHT, 12);

    // Draw icon border
    buffer.Draw_HLine(x, y, ICON_WIDTH, 253);
    buffer.Draw_HLine(x, y + ICON_HEIGHT - 1, ICON_WIDTH, 0);
    buffer.Draw_VLine(x, y, ICON_HEIGHT, 253);
    buffer.Draw_VLine(x + ICON_WIDTH - 1, y, ICON_HEIGHT, 0);

    // Draw icon shape if loaded
    if (icons_ && item.icon_frame >= 0) {
        icons_->Draw(buffer, x + 2, y + 2, item.icon_frame);
    }

    // Draw progress bar if building
    if (item.progress > 0.0f && item.progress < 1.0f) {
        DrawProgressBar(buffer, x + 2, y + ICON_HEIGHT - 6, ICON_WIDTH - 4, item.progress);
    }

    // Draw "on hold" indicator
    if (item.on_hold) {
        // Simple cross pattern
        buffer.Draw_HLine(x + 8, y + ICON_HEIGHT / 2, ICON_WIDTH - 16, 252);
    }
}

void SidebarRenderer::DrawProgressBar(GraphicsBuffer& buffer, int x, int y, int width, float progress) {
    // Background
    buffer.Fill_Rect(x, y, width, 4, 0);

    // Progress fill
    int fill_width = static_cast<int>(static_cast<float>(width - 2) * progress);
    if (fill_width > 0) {
        buffer.Fill_Rect(x + 1, y + 1, fill_width, 2, 250);  // Green
    }
}

void SidebarRenderer::DrawScrollArrows(GraphicsBuffer& buffer) {
    int arrow_x = sidebar_x_ + SIDEBAR_WIDTH - 20;
    int up_y = sidebar_y_ + TAB_HEIGHT + 4;
    int down_y = sidebar_y_ + TAB_HEIGHT + 4 + (ICONS_PER_COLUMN * (ICON_HEIGHT + 4)) + 4;

    // Up arrow
    bool can_scroll_up = (scroll_position_ > 0);
    uint8_t up_color = can_scroll_up ? static_cast<uint8_t>(253) : static_cast<uint8_t>(8);
    buffer.Fill_Rect(arrow_x, up_y, 16, 12, 14);
    // Simple triangle pointing up
    for (int i = 0; i < 5; i++) {
        buffer.Draw_HLine(arrow_x + 8 - i, up_y + 2 + i, 1 + i * 2, up_color);
    }

    // Down arrow
    bool can_scroll_down = (scroll_position_ < GetMaxScrollPosition());
    uint8_t down_color = can_scroll_down ? static_cast<uint8_t>(253) : static_cast<uint8_t>(8);
    buffer.Fill_Rect(arrow_x, down_y, 16, 12, 14);
    // Simple triangle pointing down
    for (int i = 0; i < 5; i++) {
        buffer.Draw_HLine(arrow_x + 4 + i, down_y + 2 + i, 9 - i * 2, down_color);
    }
}

void SidebarRenderer::DrawButtons(GraphicsBuffer& buffer) {
    int button_x = sidebar_x_ + 8;
    int button_width = 48;
    int button_height = 20;

    // Repair button
    DrawButton(buffer, button_x, REPAIR_BUTTON_Y, 0, repair_state_);

    // Sell button
    DrawButton(buffer, button_x + button_width + 4, REPAIR_BUTTON_Y, 1, sell_state_);

    // Map button
    DrawButton(buffer, button_x + (button_width + 4) * 2, REPAIR_BUTTON_Y, 2, map_state_);

    // Draw button rectangles (placeholder without shapes)
    if (!buttons_) {
        // Repair
        buffer.Fill_Rect(button_x, REPAIR_BUTTON_Y, button_width, button_height,
                        repair_state_ == ButtonState::ACTIVE ? static_cast<uint8_t>(250) : static_cast<uint8_t>(15));
        // Sell
        buffer.Fill_Rect(button_x + button_width + 4, REPAIR_BUTTON_Y, button_width, button_height,
                        sell_state_ == ButtonState::ACTIVE ? static_cast<uint8_t>(252) : static_cast<uint8_t>(15));
        // Map
        buffer.Fill_Rect(button_x + (button_width + 4) * 2, REPAIR_BUTTON_Y, button_width, button_height, 15);
    }
}

void SidebarRenderer::DrawButton(GraphicsBuffer& buffer, int x, int y, int frame, ButtonState state) {
    if (!buttons_) return;

    // Adjust frame based on state
    int actual_frame = frame * 4;  // 4 states per button
    switch (state) {
        case ButtonState::HOVER:   actual_frame += 1; break;
        case ButtonState::PRESSED: actual_frame += 2; break;
        case ButtonState::DISABLED: actual_frame += 3; break;
        case ButtonState::ACTIVE:  actual_frame += 2; break;  // Use pressed frame
        default: break;
    }

    buttons_->Draw(buffer, x, y, actual_frame);
}

// =============================================================================
// Hit Testing
// =============================================================================

bool SidebarRenderer::HitTest(int x, int y) const {
    return (x >= sidebar_x_ && x < sidebar_x_ + SIDEBAR_WIDTH &&
            y >= sidebar_y_ && y < sidebar_y_ + sidebar_height_);
}

int SidebarRenderer::HitTestTab(int x, int y) const {
    if (y < sidebar_y_ || y >= sidebar_y_ + TAB_HEIGHT) return -1;

    for (int i = 0; i < static_cast<int>(SidebarTab::COUNT); i++) {
        int tab_x = sidebar_x_ + i * TAB_WIDTH;
        if (x >= tab_x && x < tab_x + TAB_WIDTH) {
            return i;
        }
    }
    return -1;
}

int SidebarRenderer::HitTestBuildIcon(int x, int y) const {
    int start_y = sidebar_y_ + TAB_HEIGHT + 4;
    int icon_area_x = sidebar_x_ + 8;

    // Check bounds
    if (x < icon_area_x || y < start_y) return -1;

    int col = (x - icon_area_x) / (ICON_WIDTH + 4);
    int row = (y - start_y) / (ICON_HEIGHT + 4);

    if (col < 0 || col >= ICON_COLUMNS || row < 0 || row >= ICONS_PER_COLUMN) {
        return -1;
    }

    // Check within icon (not in gap)
    int icon_x = icon_area_x + col * (ICON_WIDTH + 4);
    int icon_y = start_y + row * (ICON_HEIGHT + 4);
    if (x >= icon_x + ICON_WIDTH || y >= icon_y + ICON_HEIGHT) {
        return -1;
    }

    int index = scroll_position_ * ICON_COLUMNS + row * ICON_COLUMNS + col;
    const auto& items = build_items_[static_cast<int>(active_tab_)];

    return (index < static_cast<int>(items.size())) ? index : -1;
}

bool SidebarRenderer::HitTestRepairButton(int x, int y) const {
    int button_x = sidebar_x_ + 8;
    return (x >= button_x && x < button_x + 48 &&
            y >= REPAIR_BUTTON_Y && y < REPAIR_BUTTON_Y + 20);
}

bool SidebarRenderer::HitTestSellButton(int x, int y) const {
    int button_x = sidebar_x_ + 8 + 52;
    return (x >= button_x && x < button_x + 48 &&
            y >= REPAIR_BUTTON_Y && y < REPAIR_BUTTON_Y + 20);
}

bool SidebarRenderer::HitTestMapButton(int x, int y) const {
    int button_x = sidebar_x_ + 8 + 104;
    return (x >= button_x && x < button_x + 48 &&
            y >= REPAIR_BUTTON_Y && y < REPAIR_BUTTON_Y + 20);
}

bool SidebarRenderer::HitTestScrollUp(int x, int y) const {
    int arrow_x = sidebar_x_ + SIDEBAR_WIDTH - 20;
    int up_y = sidebar_y_ + TAB_HEIGHT + 4;
    return (x >= arrow_x && x < arrow_x + 16 &&
            y >= up_y && y < up_y + 12);
}

bool SidebarRenderer::HitTestScrollDown(int x, int y) const {
    int arrow_x = sidebar_x_ + SIDEBAR_WIDTH - 20;
    int down_y = sidebar_y_ + TAB_HEIGHT + 4 + (ICONS_PER_COLUMN * (ICON_HEIGHT + 4)) + 4;
    return (x >= arrow_x && x < arrow_x + 16 &&
            y >= down_y && y < down_y + 12);
}
