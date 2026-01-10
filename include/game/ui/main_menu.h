/**
 * MainMenu - Main menu screen implementation
 *
 * Displays the title screen background (TITLE.PCX) and provides
 * menu buttons for starting the game or exiting.
 *
 * Based on original: CODE/MENUS.CPP
 */

#ifndef GAME_UI_MAIN_MENU_H
#define GAME_UI_MAIN_MENU_H

#include <cstdint>
#include <memory>

// Forward declarations
class GraphicsBuffer;

// =============================================================================
// Menu Button Definition
// =============================================================================

/**
 * Simple menu button with position and state
 */
struct MenuButton {
    int x;
    int y;
    int width;
    int height;
    const char* label;
    bool highlighted;
};

// =============================================================================
// Menu Selection Result
// =============================================================================

/**
 * Menu selection results matching original Red Alert menu
 * Order matches button array order
 */
enum class MenuResult {
    NONE = -1,              // No selection yet
    NEW_MISSIONS = 0,       // Expansion missions
    START_NEW_GAME = 1,     // Start new game
    INTERNET_GAME = 2,      // Internet game (placeholder)
    LOAD_MISSION = 3,       // Load saved game
    MULTIPLAYER_GAME = 4,   // LAN multiplayer
    INTRO_SNEAK_PEEK = 5,   // Watch intro/movies
    EXIT_GAME = 6,          // Exit application
};

// =============================================================================
// MainMenu Class
// =============================================================================

/**
 * Main menu screen
 *
 * Handles title screen display and menu navigation.
 */
class MainMenu {
public:
    MainMenu();
    ~MainMenu();

    // Non-copyable
    MainMenu(const MainMenu&) = delete;
    MainMenu& operator=(const MainMenu&) = delete;

    // =========================================================================
    // Lifecycle
    // =========================================================================

    /**
     * Initialize the menu (load title screen, set up buttons)
     */
    bool Initialize();

    /**
     * Shutdown and release resources
     */
    void Shutdown();

    /**
     * Check if menu is initialized
     */
    bool IsInitialized() const { return initialized_; }

    // =========================================================================
    // Update/Render
    // =========================================================================

    /**
     * Update menu state (handle input)
     */
    void Update();

    /**
     * Render the menu
     */
    void Render();

    // =========================================================================
    // State
    // =========================================================================

    /**
     * Check if a selection has been made
     */
    bool IsFinished() const { return finished_; }

    /**
     * Get the selected menu option
     */
    MenuResult GetSelection() const { return selection_; }

    /**
     * Reset menu state for re-display
     */
    void Reset();

    // =========================================================================
    // Constants
    // =========================================================================

    // Button layout matching original Red Alert menu
    static constexpr int BUTTON_COUNT = 7;
    static constexpr int BUTTON_WIDTH = 260;        // Match original width
    static constexpr int BUTTON_HEIGHT = 13;        // Compact to fit all 7
    static constexpr int BUTTON_SPACING = 2;        // Minimal spacing

    // Button positioning (based on original MENUS.CPP)
    // Original: d_dialog_y = 75*RESFACTOR=150, starty = dialog_y + 12*RESFACTOR = 174
    // But we'll use 175 for 640x400 mode
    // Total button area: 7*18 + 6*6 = 126 + 36 = 162px, ending at y=337
    static constexpr int BUTTON_START_X = 190;      // Centered: (640 - 260) / 2
    static constexpr int BUTTON_START_Y = 175;      // Match original game position

    // Colors (palette indices from TITLE.PCX palette analysis)
    // Pure red colors found at indices 192-199, 233, 236
    static constexpr uint8_t COLOR_BUTTON_NORMAL = 196;     // Dark red (R=167, G=0, B=0)
    static constexpr uint8_t COLOR_BUTTON_HIGHLIGHT = 194;  // Medium red (R=207, G=0, B=0)
    static constexpr uint8_t COLOR_BUTTON_BORDER = 198;     // Very dark red (R=99, G=0, B=0)
    static constexpr uint8_t COLOR_BUTTON_TEXT = 7;         // Light/white for text

private:
    // =========================================================================
    // Private Methods
    // =========================================================================

    /**
     * Load the title screen background
     */
    bool LoadTitleScreen();

    /**
     * Set up button positions
     */
    void SetupButtons();

    /**
     * Handle keyboard input
     */
    void HandleKeyboard();

    /**
     * Handle mouse input
     */
    void HandleMouse();

    /**
     * Draw a single button
     */
    void DrawButton(int index);

    /**
     * Draw text using simple bitmap font
     * @param text      Text to draw
     * @param center_x  X position (center if centered=true, left edge otherwise)
     * @param center_y  Y position (vertical center of text)
     * @param color     Palette color index
     * @param centered  If true, center text horizontally at center_x
     */
    void DrawText(const char* text, int center_x, int center_y, uint8_t color, bool centered);

    /**
     * Check if a point is within a button
     */
    bool IsPointInButton(int x, int y, int button_index) const;

    // =========================================================================
    // Private Data
    // =========================================================================

    bool initialized_;
    bool finished_;
    MenuResult selection_;
    int highlighted_index_;

    // Title screen background
    std::unique_ptr<GraphicsBuffer> background_;
    uint8_t palette_[768];
    bool has_background_;

    // Menu buttons
    MenuButton buttons_[BUTTON_COUNT];
};

#endif // GAME_UI_MAIN_MENU_H
