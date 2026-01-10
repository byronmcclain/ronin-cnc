/**
 * Game State Management
 *
 * Central game state and main loop control.
 *
 * Based on CODE/CONQUER.CPP, CODE/INIT.CPP
 */

#pragma once

#include "game/display.h"
#include "game/house.h"
#include <cstdint>
#include <memory>

// Forward declarations
class MainMenu;

// =============================================================================
// Game State
// =============================================================================

/**
 * GameMode - Current game state/mode
 */
enum GameMode : int8_t {
    GAME_MODE_NONE = 0,
    GAME_MODE_MENU,         // Main menu
    GAME_MODE_LOADING,      // Loading scenario
    GAME_MODE_PLAYING,      // In-game
    GAME_MODE_PAUSED,       // Game paused
    GAME_MODE_VICTORY,      // Mission won
    GAME_MODE_DEFEAT,       // Mission lost
    GAME_MODE_QUIT          // Exiting
};

/**
 * GameSpeed - Logic update rate
 */
enum GameSpeed : int8_t {
    GAME_SPEED_SLOWEST = 0, // 10 FPS
    GAME_SPEED_SLOW = 1,    // 12 FPS
    GAME_SPEED_NORMAL = 2,  // 15 FPS
    GAME_SPEED_FAST = 3,    // 20 FPS
    GAME_SPEED_FASTEST = 4  // 30 FPS
};

// Logic tick intervals (milliseconds)
extern const uint32_t GameSpeedTicks[5];

// =============================================================================
// GameClass
// =============================================================================

/**
 * GameClass - Central game state
 *
 * Manages:
 * - Game mode (menu, playing, etc.)
 * - Frame/tick counting
 * - Input state
 * - Global game settings
 */
class GameClass {
public:
    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------

    GameClass();
    ~GameClass();

    // Non-copyable
    GameClass(const GameClass&) = delete;
    GameClass& operator=(const GameClass&) = delete;

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /**
     * Initialize - Full game initialization
     *
     * Initializes platform, graphics, audio, assets.
     * @return true if successful
     */
    bool Initialize();

    /**
     * Shutdown - Clean shutdown
     */
    void Shutdown();

    /**
     * Run - Enter main game loop
     *
     * Returns when game exits.
     * @return Exit code (0 = success)
     */
    int Run();

    // -------------------------------------------------------------------------
    // Game Mode
    // -------------------------------------------------------------------------

    /**
     * Get/set game mode
     */
    GameMode Get_Mode() const { return mode_; }
    void Set_Mode(GameMode mode) { mode_ = mode; }

    /**
     * Is game running?
     */
    bool Is_Running() const { return mode_ != GAME_MODE_QUIT && mode_ != GAME_MODE_NONE; }

    /**
     * Is game paused?
     */
    bool Is_Paused() const { return mode_ == GAME_MODE_PAUSED; }

    /**
     * Request quit
     */
    void Quit() { mode_ = GAME_MODE_QUIT; }

    // -------------------------------------------------------------------------
    // Timing
    // -------------------------------------------------------------------------

    /**
     * Get current frame number
     */
    uint32_t Get_Frame() const { return frame_; }

    /**
     * Get current logic tick
     */
    uint32_t Get_Tick() const { return tick_; }

    /**
     * Get/set game speed
     */
    GameSpeed Get_Speed() const { return speed_; }
    void Set_Speed(GameSpeed speed) { speed_ = speed; }

    /**
     * Get tick interval for current speed
     */
    uint32_t Get_Tick_Interval() const;

    // -------------------------------------------------------------------------
    // Player
    // -------------------------------------------------------------------------

    /**
     * Get/set player house
     */
    HousesType Get_Player_House() const { return player_house_; }
    void Set_Player_House(HousesType house) { player_house_ = house; }

    // -------------------------------------------------------------------------
    // Display
    // -------------------------------------------------------------------------

    /**
     * Get display instance
     */
    DisplayClass* Get_Display() { return display_; }

private:
    // -------------------------------------------------------------------------
    // Main Loop Steps
    // -------------------------------------------------------------------------

    /**
     * Process input
     */
    void Process_Input();

    /**
     * Update logic (one tick)
     */
    void Update_Logic();

    /**
     * Render frame
     */
    void Render_Frame();

    /**
     * Handle menu input
     */
    void Process_Menu();

    /**
     * Handle gameplay input
     */
    void Process_Gameplay();

    // -------------------------------------------------------------------------
    // Private Members
    // -------------------------------------------------------------------------

    // State
    GameMode mode_;
    GameSpeed speed_;
    bool is_initialized_;

    // Timing
    uint32_t frame_;            // Render frame count
    uint32_t tick_;             // Logic tick count
    uint32_t last_tick_time_;   // Time of last logic tick
    uint32_t last_frame_time_;  // Time of last render

    // Player
    HousesType player_house_;

    // Display
    DisplayClass* display_;

    // Main Menu
    std::unique_ptr<MainMenu> menu_;
};

// =============================================================================
// Global Game Instance
// =============================================================================

extern GameClass* Game;

// =============================================================================
// Utility Functions
// =============================================================================

/**
 * Initialize game systems
 */
bool Game_Init();

/**
 * Shutdown game systems
 */
void Game_Shutdown();

/**
 * Main entry point
 */
int Game_Main(int argc, char* argv[]);
