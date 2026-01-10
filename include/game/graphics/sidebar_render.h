/**
 * Sidebar Renderer - Command Panel UI
 *
 * Renders the sidebar with build icons, tabs, and buttons.
 *
 * Original: CODE/SIDEBAR.CPP
 */

#ifndef GAME_GRAPHICS_SIDEBAR_RENDER_H
#define GAME_GRAPHICS_SIDEBAR_RENDER_H

#include "game/graphics/graphics_buffer.h"
#include <cstdint>
#include <vector>

// =============================================================================
// Forward Declarations
// =============================================================================

class ShapeRenderer;
class PaletteManager;

// =============================================================================
// Constants
// =============================================================================

// Sidebar dimensions (original game values)
constexpr int SIDEBAR_WIDTH = 160;
constexpr int SIDEBAR_X = 480;  // Right side of 640x400 screen

// Tab dimensions
constexpr int TAB_WIDTH = 80;
constexpr int TAB_HEIGHT = 24;

// Build icon dimensions
constexpr int ICON_WIDTH = 64;
constexpr int ICON_HEIGHT = 48;
constexpr int ICONS_PER_COLUMN = 4;
constexpr int ICON_COLUMNS = 2;

// Button positions
constexpr int REPAIR_BUTTON_Y = 320;
constexpr int SELL_BUTTON_Y = 344;
constexpr int MAP_BUTTON_Y = 368;

// =============================================================================
// Sidebar Tab Types
// =============================================================================

enum class SidebarTab : int {
    STRUCTURE = 0,    // Building tab
    UNIT = 1,         // Unit/infantry tab

    COUNT = 2
};

// =============================================================================
// Build Queue Item
// =============================================================================

/**
 * BuildQueueItem - Item in the build queue
 */
struct BuildQueueItem {
    int type_id;           // Object type identifier
    int icon_frame;        // Shape frame for the icon
    float progress;        // Build progress (0.0 - 1.0)
    bool on_hold;          // Production paused
    int queue_count;       // Number queued (for infantry)

    BuildQueueItem()
        : type_id(-1)
        , icon_frame(0)
        , progress(0.0f)
        , on_hold(false)
        , queue_count(0)
    {}
};

// =============================================================================
// Sidebar Button State
// =============================================================================

/**
 * ButtonState - Visual state of sidebar buttons
 */
enum class ButtonState : int {
    NORMAL = 0,
    HOVER = 1,
    PRESSED = 2,
    DISABLED = 3,
    ACTIVE = 4     // For toggle buttons like repair mode
};

// =============================================================================
// Sidebar Renderer Class
// =============================================================================

/**
 * SidebarRenderer - Draws the game sidebar UI
 */
class SidebarRenderer {
public:
    SidebarRenderer();
    ~SidebarRenderer();

    // Prevent copying
    SidebarRenderer(const SidebarRenderer&) = delete;
    SidebarRenderer& operator=(const SidebarRenderer&) = delete;

    // =========================================================================
    // Initialization
    // =========================================================================

    /**
     * Initialize sidebar resources
     * @param screen_height Height of screen for positioning
     * @return true if successful
     */
    bool Initialize(int screen_height);

    /**
     * Load sidebar graphics
     * @param icons_filename Icon shapes file (e.g., "CONQUER.SHP")
     * @param buttons_filename Button shapes file
     * @return true if successful
     */
    bool LoadGraphics(const char* icons_filename, const char* buttons_filename);

    /**
     * Shutdown and release resources
     */
    void Shutdown();

    /**
     * Check if initialized
     */
    bool IsInitialized() const { return initialized_; }

    // =========================================================================
    // State Updates
    // =========================================================================

    /**
     * Set the active tab
     */
    void SetActiveTab(SidebarTab tab);

    /**
     * Get the active tab
     */
    SidebarTab GetActiveTab() const { return active_tab_; }

    /**
     * Set scroll position for build icons
     */
    void SetScrollPosition(int position);

    /**
     * Get scroll position
     */
    int GetScrollPosition() const { return scroll_position_; }

    /**
     * Get maximum scroll position
     */
    int GetMaxScrollPosition() const;

    /**
     * Scroll up one row
     */
    void ScrollUp();

    /**
     * Scroll down one row
     */
    void ScrollDown();

    // =========================================================================
    // Build Queue Management
    // =========================================================================

    /**
     * Clear all build items
     */
    void ClearBuildItems();

    /**
     * Add a buildable item to the sidebar
     */
    void AddBuildItem(int type_id, int icon_frame);

    /**
     * Update build progress for an item
     */
    void SetBuildProgress(int type_id, float progress);

    /**
     * Set item on hold
     */
    void SetBuildOnHold(int type_id, bool on_hold);

    /**
     * Get number of build items
     */
    int GetBuildItemCount() const { return static_cast<int>(build_items_[static_cast<int>(active_tab_)].size()); }

    // =========================================================================
    // Button States
    // =========================================================================

    /**
     * Set repair button state
     */
    void SetRepairButtonState(ButtonState state) { repair_state_ = state; }

    /**
     * Set sell button state
     */
    void SetSellButtonState(ButtonState state) { sell_state_ = state; }

    /**
     * Set map button state
     */
    void SetMapButtonState(ButtonState state) { map_state_ = state; }

    /**
     * Get repair button state
     */
    ButtonState GetRepairButtonState() const { return repair_state_; }

    /**
     * Get sell button state
     */
    ButtonState GetSellButtonState() const { return sell_state_; }

    /**
     * Get map button state
     */
    ButtonState GetMapButtonState() const { return map_state_; }

    // =========================================================================
    // Rendering
    // =========================================================================

    /**
     * Draw the complete sidebar
     * @param buffer Target graphics buffer
     */
    void Draw(GraphicsBuffer& buffer);

    /**
     * Draw just the tab bar
     */
    void DrawTabs(GraphicsBuffer& buffer);

    /**
     * Draw the build icons grid
     */
    void DrawBuildIcons(GraphicsBuffer& buffer);

    /**
     * Draw the action buttons (repair, sell, map)
     */
    void DrawButtons(GraphicsBuffer& buffer);

    /**
     * Draw the scroll arrows
     */
    void DrawScrollArrows(GraphicsBuffer& buffer);

    // =========================================================================
    // Hit Testing
    // =========================================================================

    /**
     * Check if point is in sidebar
     */
    bool HitTest(int x, int y) const;

    /**
     * Get which tab was clicked (-1 if none)
     */
    int HitTestTab(int x, int y) const;

    /**
     * Get which build icon was clicked (-1 if none)
     * Returns index in current tab's build list
     */
    int HitTestBuildIcon(int x, int y) const;

    /**
     * Check if repair button was clicked
     */
    bool HitTestRepairButton(int x, int y) const;

    /**
     * Check if sell button was clicked
     */
    bool HitTestSellButton(int x, int y) const;

    /**
     * Check if map button was clicked
     */
    bool HitTestMapButton(int x, int y) const;

    /**
     * Check if scroll up arrow was clicked
     */
    bool HitTestScrollUp(int x, int y) const;

    /**
     * Check if scroll down arrow was clicked
     */
    bool HitTestScrollDown(int x, int y) const;

    // =========================================================================
    // Layout
    // =========================================================================

    /**
     * Get sidebar X position
     */
    int GetX() const { return sidebar_x_; }

    /**
     * Get sidebar Y position
     */
    int GetY() const { return sidebar_y_; }

    /**
     * Get sidebar width
     */
    int GetWidth() const { return SIDEBAR_WIDTH; }

    /**
     * Get sidebar height
     */
    int GetHeight() const { return sidebar_height_; }

private:
    // Internal draw helpers
    void DrawBackground(GraphicsBuffer& buffer);
    void DrawBuildIcon(GraphicsBuffer& buffer, int x, int y, const BuildQueueItem& item);
    void DrawProgressBar(GraphicsBuffer& buffer, int x, int y, int width, float progress);
    void DrawButton(GraphicsBuffer& buffer, int x, int y, int frame, ButtonState state);

    // State
    bool initialized_;
    int sidebar_x_;
    int sidebar_y_;
    int sidebar_height_;

    // Active tab and scroll
    SidebarTab active_tab_;
    int scroll_position_;

    // Build items per tab
    std::vector<BuildQueueItem> build_items_[static_cast<int>(SidebarTab::COUNT)];

    // Button states
    ButtonState repair_state_;
    ButtonState sell_state_;
    ButtonState map_state_;

    // Graphics
    std::unique_ptr<ShapeRenderer> icons_;
    std::unique_ptr<ShapeRenderer> buttons_;
};

#endif // GAME_GRAPHICS_SIDEBAR_RENDER_H
