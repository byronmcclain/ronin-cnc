/**
 * Main Game Loop Implementation
 *
 * Core game loop and state management.
 */

#include "game/game.h"
#include "game/object.h"
#include "game/cell.h"
#include "platform.h"
#include <cstdio>
#include <cstring>

// =============================================================================
// Game Speed Table
// =============================================================================

const uint32_t GameSpeedTicks[5] = {
    100,    // GAME_SPEED_SLOWEST - 10 FPS
    83,     // GAME_SPEED_SLOW - 12 FPS
    66,     // GAME_SPEED_NORMAL - 15 FPS
    50,     // GAME_SPEED_FAST - 20 FPS
    33      // GAME_SPEED_FASTEST - 30 FPS
};

// =============================================================================
// Global Instance
// =============================================================================

GameClass* Game = nullptr;

// =============================================================================
// Construction
// =============================================================================

GameClass::GameClass()
    : mode_(GAME_MODE_NONE)
    , speed_(GAME_SPEED_NORMAL)
    , is_initialized_(false)
    , frame_(0)
    , tick_(0)
    , last_tick_time_(0)
    , last_frame_time_(0)
    , player_house_(HOUSE_GOOD)
    , display_(nullptr)
{
}

GameClass::~GameClass() {
    Shutdown();
}

// =============================================================================
// Lifecycle
// =============================================================================

bool GameClass::Initialize() {
    if (is_initialized_) {
        return true;
    }

    Platform_LogInfo("GameClass::Initialize: Starting...");

    // Initialize platform
    if (Platform_Init() != PLATFORM_RESULT_SUCCESS) {
        Platform_LogError("Failed to initialize platform");
        return false;
    }

    // Create display
    display_ = new DisplayClass();
    if (display_ == nullptr) {
        Platform_LogError("Failed to create display");
        Shutdown();
        return false;
    }

    // Initialize display
    display_->One_Time();

    // Set as global
    Display = display_;
    Map = display_;
    TheScreen = display_;

    // Initialize timing
    last_tick_time_ = Platform_Timer_GetTicks();
    last_frame_time_ = last_tick_time_;
    frame_ = 0;
    tick_ = 0;

    // Start in menu mode
    mode_ = GAME_MODE_MENU;
    is_initialized_ = true;

    Platform_LogInfo("GameClass::Initialize: Complete");
    return true;
}

void GameClass::Shutdown() {
    if (!is_initialized_) {
        return;
    }

    Platform_LogInfo("GameClass::Shutdown: Starting...");

    // Clean up display
    if (display_ != nullptr) {
        delete display_;
        display_ = nullptr;
        Display = nullptr;
        Map = nullptr;
        TheScreen = nullptr;
    }

    // Shutdown platform
    Platform_Shutdown();

    is_initialized_ = false;
    mode_ = GAME_MODE_NONE;

    Platform_LogInfo("GameClass::Shutdown: Complete");
}

// =============================================================================
// Main Loop
// =============================================================================

int GameClass::Run() {
    if (!is_initialized_) {
        Platform_LogError("Game not initialized");
        return 1;
    }

    Platform_LogInfo("GameClass::Run: Entering main loop");

    while (Is_Running()) {
        // Start frame timing
        Platform_Frame_Begin();

        // Check for system quit
        if (Platform_PollEvents()) {
            mode_ = GAME_MODE_QUIT;
            break;
        }

        // Process input
        Process_Input();

        // Update logic (if time for tick)
        uint32_t now = Platform_Timer_GetTicks();
        uint32_t tick_interval = Get_Tick_Interval();

        while (now - last_tick_time_ >= tick_interval) {
            if (!Is_Paused() && mode_ == GAME_MODE_PLAYING) {
                Update_Logic();
                tick_++;
            }
            last_tick_time_ += tick_interval;
        }

        // Render frame
        Render_Frame();
        frame_++;
        last_frame_time_ = now;

        // End frame (may sleep for vsync)
        Platform_Frame_End();
    }

    Platform_LogInfo("GameClass::Run: Exiting main loop");
    return 0;
}

uint32_t GameClass::Get_Tick_Interval() const {
    if (speed_ < 0 || speed_ > GAME_SPEED_FASTEST) {
        return GameSpeedTicks[GAME_SPEED_NORMAL];
    }
    return GameSpeedTicks[speed_];
}

// =============================================================================
// Input Processing
// =============================================================================

void GameClass::Process_Input() {
    // Update platform input state
    Platform_Input_Update();

    // Handle mode-specific input
    switch (mode_) {
        case GAME_MODE_MENU:
            Process_Menu();
            break;

        case GAME_MODE_PLAYING:
            Process_Gameplay();
            break;

        case GAME_MODE_PAUSED:
            // ESC to unpause
            if (Platform_Key_WasPressed(KEY_CODE_ESCAPE)) {
                mode_ = GAME_MODE_PLAYING;
            }
            break;

        default:
            // ESC to quit from other modes
            if (Platform_Key_WasPressed(KEY_CODE_ESCAPE)) {
                mode_ = GAME_MODE_QUIT;
            }
            break;
    }
}

void GameClass::Process_Menu() {
    // Simplified menu - just start playing on ENTER
    if (Platform_Key_WasPressed(KEY_CODE_RETURN)) {
        // Initialize a test scenario
        if (display_) {
            display_->Init(THEATER_TEMPERATE);

            // Reveal some cells for testing
            for (int y = 20; y < 40; y++) {
                for (int x = 20; x < 60; x++) {
                    CellClass* cell = display_->Cell_At(x, y);
                    if (cell) {
                        cell->Reveal(true);
                        cell->Set_Template(0, 0); // TEMPLATE_CLEAR = 0
                    }
                }
            }

            display_->Set_Map_Bounds(20, 20, 40, 20);
            display_->Center_On(XY_Cell(40, 30));
        }

        mode_ = GAME_MODE_PLAYING;
        Platform_LogInfo("Starting gameplay");
    }

    // ESC to quit
    if (Platform_Key_WasPressed(KEY_CODE_ESCAPE)) {
        mode_ = GAME_MODE_QUIT;
    }
}

void GameClass::Process_Gameplay() {
    if (!display_) return;

    // ESC to pause
    if (Platform_Key_WasPressed(KEY_CODE_ESCAPE)) {
        mode_ = GAME_MODE_PAUSED;
        return;
    }

    // Arrow keys to scroll
    int scroll_x = 0;
    int scroll_y = 0;

    if (Platform_Key_IsPressed(KEY_CODE_UP)) {
        scroll_y = -SCROLL_SPEED_NORMAL;
    }
    if (Platform_Key_IsPressed(KEY_CODE_DOWN)) {
        scroll_y = SCROLL_SPEED_NORMAL;
    }
    if (Platform_Key_IsPressed(KEY_CODE_LEFT)) {
        scroll_x = -SCROLL_SPEED_NORMAL;
    }
    if (Platform_Key_IsPressed(KEY_CODE_RIGHT)) {
        scroll_x = SCROLL_SPEED_NORMAL;
    }

    if (scroll_x != 0 || scroll_y != 0) {
        display_->Scroll(scroll_x, scroll_y);
    }

    // Update cursor cell based on mouse
    int32_t mx, my;
    Platform_Mouse_GetPosition(&mx, &my);
    CELL cursor = display_->Screen_To_Cell(mx, my);
    display_->Set_Cursor_Cell(cursor);

    // Mouse click to select cell (placeholder)
    if (Platform_Mouse_WasClicked(MOUSE_BUTTON_LEFT) && cursor != CELL_NONE) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Clicked cell (%d, %d)", Cell_X(cursor), Cell_Y(cursor));
        Platform_LogInfo(msg);
    }

    // Game speed controls
    if (Platform_Key_WasPressed(KEY_CODE_F5)) {
        if (speed_ > GAME_SPEED_SLOWEST) {
            speed_ = (GameSpeed)(speed_ - 1);
        }
    }
    if (Platform_Key_WasPressed(KEY_CODE_F6)) {
        if (speed_ < GAME_SPEED_FASTEST) {
            speed_ = (GameSpeed)(speed_ + 1);
        }
    }
}

// =============================================================================
// Logic Update
// =============================================================================

void GameClass::Update_Logic() {
    // Update all game objects
    FOR_ALL_OBJECTS(obj) {
        if (obj->Is_Active()) {
            obj->AI();
        }
    }

    // Process gadgets
    if (display_) {
        display_->Process_Gadgets(0);
    }

    // Additional logic updates would go here:
    // - House income
    // - Production queues
    // - Trigger events
    // - Pathfinding
}

// =============================================================================
// Rendering
// =============================================================================

void GameClass::Render_Frame() {
    if (!display_) return;

    // Render based on mode
    switch (mode_) {
        case GAME_MODE_MENU:
            // Draw simple menu placeholder
            display_->Lock();
            display_->Clear(0);
            display_->Draw_Rect(200, 180, 240, 40, 8);
            display_->Draw_Rect(202, 182, 236, 36, 7);
            display_->Unlock();
            display_->Flip();
            break;

        case GAME_MODE_PLAYING:
        case GAME_MODE_PAUSED:
            // Render tactical view
            display_->Render();

            // Draw pause overlay if paused
            if (mode_ == GAME_MODE_PAUSED) {
                display_->Lock();
                display_->Draw_Rect(280, 190, 80, 20, 0);
                display_->Unlock();
                display_->Flip();
            }
            break;

        default:
            // Just clear
            display_->Lock();
            display_->Clear(0);
            display_->Unlock();
            display_->Flip();
            break;
    }
}

// =============================================================================
// Global Functions
// =============================================================================

bool Game_Init() {
    if (Game != nullptr) {
        return true;
    }

    Game = new GameClass();
    if (Game == nullptr) {
        return false;
    }

    return Game->Initialize();
}

void Game_Shutdown() {
    if (Game != nullptr) {
        delete Game;
        Game = nullptr;
    }
}

int Game_Main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    Platform_LogInfo("Red Alert - Starting...");

    if (!Game_Init()) {
        Platform_LogError("Failed to initialize game");
        return 1;
    }

    int result = Game->Run();

    Game_Shutdown();

    Platform_LogInfo("Red Alert - Exiting");
    return result;
}
