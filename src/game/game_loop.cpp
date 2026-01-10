/**
 * Main Game Loop Implementation
 *
 * Core game loop and state management.
 */

#include "game/game.h"
#include "game/object.h"
#include "game/cell.h"
#include "game/graphics/tile_renderer.h"
#include "game/ui/main_menu.h"
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
    , menu_(nullptr)
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

    // Register MIX files from gamedata directory
    Platform_LogInfo("Registering MIX files...");

    // Get the data path (should find gamedata directory)
    char data_path[512];
    Platform_GetDataPath(data_path, sizeof(data_path));

    char mix_path[512];
    int mix_count = 0;

    // Register REDALERT.MIX - contains palettes and core data
    snprintf(mix_path, sizeof(mix_path), "%s/REDALERT.MIX", data_path);
    int reg_result = Platform_Mix_Register(mix_path);
    if (reg_result == 0) {
        mix_count++;
    }

    // Register MAIN.MIX - contains main game assets (and nested MIX files)
    snprintf(mix_path, sizeof(mix_path), "%s/MAIN.MIX", data_path);
    reg_result = Platform_Mix_Register(mix_path);
    if (reg_result == 0) {
        mix_count++;

        // Register nested MIX files from MAIN.MIX
        // These contain assets like TITLE.PCX, unit graphics, etc.
        Platform_Mix_RegisterNested("local.mix");
        Platform_Mix_RegisterNested("conquer.mix");
        Platform_Mix_RegisterNested("lores.mix");
        Platform_Mix_RegisterNested("hires.mix");
    }

    // Register theater-specific MIX files
    snprintf(mix_path, sizeof(mix_path), "%s/interior.mix", data_path);
    Platform_Mix_Register(mix_path);

    snprintf(mix_path, sizeof(mix_path), "%s/winter.mix", data_path);
    Platform_Mix_Register(mix_path);

    snprintf(mix_path, sizeof(mix_path), "%s/temperat.mix", data_path);
    Platform_Mix_Register(mix_path);

    char msg[64];
    snprintf(msg, sizeof(msg), "Registered %d primary MIX files", mix_count);
    Platform_LogInfo(msg);

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

    // Create and initialize main menu
    menu_ = std::make_unique<MainMenu>();
    if (!menu_->Initialize()) {
        Platform_LogWarn("MainMenu initialization failed, using fallback");
        // Set a default palette so something is visible
        PaletteEntry default_palette[256];
        for (int i = 0; i < 256; i++) {
            default_palette[i].r = static_cast<uint8_t>(i);
            default_palette[i].g = static_cast<uint8_t>(i);
            default_palette[i].b = static_cast<uint8_t>(i);
        }
        default_palette[0].r = 0;
        default_palette[0].g = 0;
        default_palette[0].b = 0;
        Platform_Graphics_SetPalette(default_palette, 0, 256);
    }

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

    // Clean up main menu
    menu_.reset();

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

        // Update input state BEFORE polling events
        // This saves current key state as previous, so we can detect "just pressed"
        Platform_Input_Update();

        // Check for system quit (this also processes all SDL events)
        if (Platform_PollEvents()) {
            mode_ = GAME_MODE_QUIT;
            break;
        }

        // Process input (game-specific handling)
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
    // Handle mode-specific input
    // Note: Platform_Input_Update() is called in the main loop before Platform_PollEvents()
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
    // Update main menu
    if (menu_) {
        menu_->Update();

        if (menu_->IsFinished()) {
            MenuResult selection = menu_->GetSelection();

            if (selection == MenuResult::START_NEW_GAME) {
                Platform_LogInfo("Loading theater and initializing TileRenderer...");

                // Initialize TileRenderer with the theater (this loads palette and templates)
                TileRenderer& renderer = TileRenderer::Instance();
                if (!renderer.SetTheater(THEATER_TEMPERATE)) {
                    Platform_LogError("TileRenderer::SetTheater failed");
                } else {
                    Platform_LogInfo("TileRenderer initialized for TEMPERATE theater");
                }

                // Load the theater palette from MIX files
                uint8_t palette_data[768];
                int result = Platform_Palette_Load("TEMPERAT.PAL", palette_data);

                if (result == 0) {
                    Platform_LogInfo("Loaded TEMPERAT.PAL successfully");
                    PaletteEntry entries[256];
                    for (int i = 0; i < 256; i++) {
                        entries[i].r = palette_data[i * 3 + 0];
                        entries[i].g = palette_data[i * 3 + 1];
                        entries[i].b = palette_data[i * 3 + 2];
                    }
                    Platform_Graphics_SetPalette(entries, 0, 256);
                } else {
                    Platform_LogError("Failed to load TEMPERAT.PAL, using fallback palette");
                    PaletteEntry entries[256];
                    for (int i = 0; i < 256; i++) {
                        entries[i].r = (uint8_t)i;
                        entries[i].g = (uint8_t)i;
                        entries[i].b = (uint8_t)i;
                    }
                    entries[141].r = 50; entries[141].g = 120; entries[141].b = 50;
                    entries[154].r = 30; entries[154].g = 60;  entries[154].b = 150;
                    entries[176].r = 128; entries[176].g = 128; entries[176].b = 128;
                    Platform_Graphics_SetPalette(entries, 0, 256);
                }

                // Initialize a test scenario
                if (display_) {
                    display_->Init(THEATER_TEMPERATE);

                    for (int y = 20; y < 40; y++) {
                        for (int x = 20; x < 60; x++) {
                            CellClass* cell = display_->Cell_At(x, y);
                            if (cell) {
                                cell->Reveal(true);
                                cell->Set_Template(0, 0);
                            }
                        }
                    }

                    display_->Set_Map_Bounds(20, 20, 40, 20);
                    display_->Center_On(XY_Cell(40, 30));
                }

                mode_ = GAME_MODE_PLAYING;
                Platform_LogInfo("Starting gameplay");

            } else if (selection == MenuResult::EXIT_GAME) {
                mode_ = GAME_MODE_QUIT;
            }
        }
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
            // Render main menu
            if (menu_) {
                menu_->Render();
            } else {
                // Fallback if no menu
                display_->Lock();
                display_->Clear(0);
                display_->Draw_Rect(200, 180, 240, 40, 255);
                display_->Unlock();
                display_->Flip();
            }
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
