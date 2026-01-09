# Task 14i: Main Game Loop

## Dependencies
- Task 14f (DisplayClass Port) must be complete
- Task 14g (ObjectClass System) must be complete
- Platform layer must be fully functional

## Context
The main game loop ties everything together - it processes input, updates game logic, renders frames, and manages timing. This is the heart of the game that makes it interactive.

## Objective
Implement the main game loop:
1. Game initialization sequence
2. Main loop with proper timing
3. Input processing
4. Logic updates (AI)
5. Render pipeline
6. Clean shutdown

## Technical Background

### Original CONQUER.CPP Structure
The original main loop handles:
- Windows message processing (replaced by platform layer)
- Input polling (keyboard, mouse)
- Fixed-rate logic updates (15 FPS game logic)
- Variable-rate rendering (up to 30 FPS)
- Frame timing and sleep

### Frame Timing
Red Alert runs logic at 15 FPS (66.6ms per tick):
- Game speed setting affects this
- Network sync requires deterministic timing
- Rendering can run faster if system allows

### Logic vs. Render
- Logic: Updates positions, AI, combat - runs at fixed rate
- Render: Draws current state - runs as fast as possible

## Deliverables
- [ ] `include/game/game.h` - Game state management
- [ ] `src/game/game_loop.cpp` - Main loop implementation
- [ ] `src/game/init.cpp` - Initialization sequence
- [ ] Input handling integration
- [ ] Fixed-rate logic updates
- [ ] Variable-rate rendering
- [ ] Verification script passes

## Files to Create

### include/game/game.h (NEW)
```cpp
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
```

### src/game/game_loop.cpp (NEW)
```cpp
/**
 * Main Game Loop Implementation
 *
 * Core game loop and state management.
 */

#include "game/game.h"
#include "game/object.h"
#include "platform.h"
#include <cstdio>

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

    Platform_LogInfo("GameClass::Run: Exiting main loop (frame=%u, tick=%u)",
                     frame_, tick_);
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
            if (Platform_Key_JustPressed(KEY_CODE_ESCAPE)) {
                mode_ = GAME_MODE_PLAYING;
            }
            break;

        default:
            // ESC to quit from other modes
            if (Platform_Key_JustPressed(KEY_CODE_ESCAPE)) {
                mode_ = GAME_MODE_QUIT;
            }
            break;
    }
}

void GameClass::Process_Menu() {
    // Simplified menu - just start playing on ENTER
    if (Platform_Key_JustPressed(KEY_CODE_ENTER)) {
        // Initialize a test scenario
        if (display_) {
            display_->Init(THEATER_TEMPERATE);

            // Reveal some cells for testing
            for (int y = 20; y < 40; y++) {
                for (int x = 20; x < 60; x++) {
                    CellClass* cell = display_->Cell_At(x, y);
                    if (cell) {
                        cell->Reveal(true);
                        cell->Set_Template(TEMPLATE_CLEAR, 0);
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
    if (Platform_Key_JustPressed(KEY_CODE_ESCAPE)) {
        mode_ = GAME_MODE_QUIT;
    }
}

void GameClass::Process_Gameplay() {
    if (!display_) return;

    // ESC to pause
    if (Platform_Key_JustPressed(KEY_CODE_ESCAPE)) {
        mode_ = GAME_MODE_PAUSED;
        return;
    }

    // Arrow keys to scroll
    int scroll_x = 0;
    int scroll_y = 0;

    if (Platform_Key_Held(KEY_CODE_UP) || Platform_Key_Held(KEY_CODE_W)) {
        scroll_y = -SCROLL_SPEED_NORMAL;
    }
    if (Platform_Key_Held(KEY_CODE_DOWN) || Platform_Key_Held(KEY_CODE_S)) {
        scroll_y = SCROLL_SPEED_NORMAL;
    }
    if (Platform_Key_Held(KEY_CODE_LEFT) || Platform_Key_Held(KEY_CODE_A)) {
        scroll_x = -SCROLL_SPEED_NORMAL;
    }
    if (Platform_Key_Held(KEY_CODE_RIGHT) || Platform_Key_Held(KEY_CODE_D)) {
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
    if (Platform_Mouse_ButtonJustPressed(0) && cursor != CELL_NONE) {
        Platform_LogInfo("Clicked cell (%d, %d)", Cell_X(cursor), Cell_Y(cursor));
    }

    // Game speed controls
    if (Platform_Key_JustPressed(KEY_CODE_MINUS)) {
        if (speed_ > GAME_SPEED_SLOWEST) {
            speed_ = (GameSpeed)(speed_ - 1);
            Platform_LogInfo("Game speed: %d", speed_);
        }
    }
    if (Platform_Key_JustPressed(KEY_CODE_EQUALS)) {
        if (speed_ < GAME_SPEED_FASTEST) {
            speed_ = (GameSpeed)(speed_ + 1);
            Platform_LogInfo("Game speed: %d", speed_);
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

    Platform_LogInfo("Red Alert - Exiting (code %d)", result);
    return result;
}
```

### src/game/init.cpp (NEW)
```cpp
/**
 * Game Initialization
 *
 * Handles one-time game initialization and scenario loading.
 */

#include "game/game.h"
#include "game/types/unittype.h"
#include "game/types/buildingtype.h"
#include "platform.h"

// =============================================================================
// Initialization Helpers
// =============================================================================

/**
 * Initialize type classes
 */
static bool Init_Types() {
    Platform_LogInfo("Initializing type classes...");

    // Unit types are statically initialized
    // Just verify they're valid
    for (int i = 0; i < UNIT_COUNT; i++) {
        if (UnitTypes[i].Get_Name()[0] == '\0') {
            Platform_LogError("Unit type %d not initialized", i);
            return false;
        }
    }

    Platform_LogInfo("  %d unit types loaded", UNIT_COUNT);
    Platform_LogInfo("  %d building types loaded", BUILDING_COUNT);

    return true;
}

/**
 * Load palette for theater
 */
static bool Init_Palette(int theater) {
    const char* palette_name;
    switch (theater) {
        case THEATER_TEMPERATE:
            palette_name = "TEMPERAT.PAL";
            break;
        case THEATER_SNOW:
            palette_name = "SNOW.PAL";
            break;
        case THEATER_INTERIOR:
            palette_name = "INTERIOR.PAL";
            break;
        default:
            palette_name = "TEMPERAT.PAL";
            break;
    }

    Platform_LogInfo("Loading palette: %s", palette_name);

    // Placeholder - actual palette loading would go through asset system
    // For now, just set a default palette
    uint8_t palette[768];
    for (int i = 0; i < 256; i++) {
        palette[i * 3 + 0] = i;     // R
        palette[i * 3 + 1] = i;     // G
        palette[i * 3 + 2] = i;     // B
    }

    // Set some recognizable colors
    // Black
    palette[0] = palette[1] = palette[2] = 0;

    // White
    palette[15*3+0] = palette[15*3+1] = palette[15*3+2] = 255;

    // Green (terrain)
    palette[141*3+0] = 50;
    palette[141*3+1] = 120;
    palette[141*3+2] = 50;

    // Blue (water)
    palette[154*3+0] = 30;
    palette[154*3+1] = 60;
    palette[154*3+2] = 150;

    // Grey (road)
    palette[176*3+0] = palette[176*3+1] = palette[176*3+2] = 128;

    // Red
    palette[123*3+0] = 200;
    palette[123*3+1] = 50;
    palette[123*3+2] = 50;

    // Green (selection)
    palette[120*3+0] = 50;
    palette[120*3+1] = 200;
    palette[120*3+2] = 50;

    Platform_Graphics_SetPalette(palette);

    return true;
}

/**
 * Full initialization sequence
 */
bool Game_Full_Init() {
    Platform_LogInfo("=== Game Full Initialization ===");

    // Initialize types
    if (!Init_Types()) {
        return false;
    }

    // Load default palette
    if (!Init_Palette(THEATER_TEMPERATE)) {
        return false;
    }

    // Mount MIX files (placeholder)
    Platform_LogInfo("MIX file mounting would happen here");

    Platform_LogInfo("=== Initialization Complete ===");
    return true;
}
```

## Verification Command
```bash
ralph-tasks/verify/check-game-loop.sh
```

## Verification Script

### ralph-tasks/verify/check-game-loop.sh (NEW)
```bash
#!/bin/bash
# Verification script for main game loop

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Main Game Loop ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check files exist
echo "[1/4] Checking files..."
for f in include/game/game.h src/game/game_loop.cpp src/game/init.cpp; do
    if [ ! -f "$f" ]; then
        echo "VERIFY_FAILED: $f not found"
        exit 1
    fi
    echo "  OK: $f"
done

# Step 2: Create mock platform for compilation
echo "[2/4] Creating mock platform..."
cat > /tmp/mock_game_platform.h << 'EOF'
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdarg>

enum PlatformResult { PLATFORM_RESULT_SUCCESS = 0 };
enum KeyCode {
    KEY_CODE_ESCAPE = 27, KEY_CODE_ENTER = 13,
    KEY_CODE_UP = 38, KEY_CODE_DOWN = 40, KEY_CODE_LEFT = 37, KEY_CODE_RIGHT = 39,
    KEY_CODE_W = 87, KEY_CODE_A = 65, KEY_CODE_S = 83, KEY_CODE_D = 68,
    KEY_CODE_MINUS = 189, KEY_CODE_EQUALS = 187
};

static uint8_t g_buffer[640*400];
static uint8_t g_palette[768];
static uint32_t g_ticks = 0;
static bool g_should_quit = false;

inline PlatformResult Platform_Init() { return PLATFORM_RESULT_SUCCESS; }
inline void Platform_Shutdown() {}
inline PlatformResult Platform_Graphics_Init() { return PLATFORM_RESULT_SUCCESS; }
inline PlatformResult Platform_Graphics_GetBackBuffer(uint8_t** buf, int32_t* w, int32_t* h, int32_t* p) {
    *buf = g_buffer; *w = 640; *h = 400; *p = 640;
    return PLATFORM_RESULT_SUCCESS;
}
inline void Platform_Graphics_Flip() {}
inline void Platform_Graphics_LockBackBuffer() {}
inline void Platform_Graphics_UnlockBackBuffer() {}
inline void Platform_Graphics_SetPalette(const uint8_t* pal) { for(int i=0;i<768;i++) g_palette[i]=pal[i]; }
inline void Platform_Input_Update() {}
inline void Platform_Mouse_GetPosition(int32_t* x, int32_t* y) { *x = 320; *y = 200; }
inline bool Platform_Mouse_ButtonJustPressed(int) { return false; }
inline bool Platform_Mouse_ButtonJustReleased(int) { return false; }
inline bool Platform_Mouse_ButtonHeld(int) { return false; }
inline bool Platform_Key_JustPressed(KeyCode k) {
    static int count = 0;
    count++;
    if (count == 5 && k == KEY_CODE_ENTER) return true;
    if (count == 20 && k == KEY_CODE_ESCAPE) { g_should_quit = true; return true; }
    return false;
}
inline bool Platform_Key_Held(KeyCode) { return false; }
inline uint32_t Platform_Timer_GetTicks() { return g_ticks += 16; }
inline void Platform_Frame_Begin() {}
inline void Platform_Frame_End() {}
inline bool Platform_PollEvents() { return g_should_quit; }
inline void Platform_LogInfo(const char* fmt, ...) { va_list a; va_start(a,fmt); vprintf(fmt,a); va_end(a); printf("\n"); }
inline void Platform_LogError(const char* fmt, ...) { va_list a; va_start(a,fmt); vfprintf(stderr,fmt,a); va_end(a); fprintf(stderr,"\n"); }
EOF

# Step 3: Build test
echo "[3/4] Building game loop test..."
if ! clang++ -std=c++17 -I include -I /tmp \
    -include /tmp/mock_game_platform.h \
    src/game/core/coord.cpp \
    src/game/map/cell.cpp \
    src/game/object/object.cpp \
    src/game/types/types.cpp \
    src/game/display/gscreen.cpp \
    src/game/display/gadget.cpp \
    src/game/display/map.cpp \
    src/game/display/display.cpp \
    src/game/game_loop.cpp \
    src/game/init.cpp \
    -o /tmp/test_game_loop \
    -DTEST_GAME_LOOP << 'MAIN'
#include "game/game.h"
int main() {
    printf("=== Game Loop Test ===\n\n");

    // Initialize
    if (!Game_Init()) {
        printf("FAIL: Game_Init failed\n");
        return 1;
    }
    printf("PASS: Game initialized\n");

    // Check initial state
    if (Game == nullptr) {
        printf("FAIL: Game is null\n");
        return 1;
    }
    printf("PASS: Game instance exists\n");

    if (Game->Get_Mode() != GAME_MODE_MENU) {
        printf("FAIL: Expected GAME_MODE_MENU\n");
        return 1;
    }
    printf("PASS: Initial mode is MENU\n");

    // Run (will auto-quit after few frames via mock)
    int result = Game->Run();
    printf("PASS: Game loop ran (exit code %d)\n", result);

    // Shutdown
    Game_Shutdown();
    printf("PASS: Game shutdown\n");

    printf("\n=== All game loop tests passed! ===\n");
    return 0;
}
MAIN
    2>&1; then
    echo "VERIFY_FAILED: Compilation failed"
    rm -f /tmp/mock_game_platform.h
    exit 1
fi
echo "  OK: Test built"

# Step 4: Run test
echo "[4/4] Running game loop test..."
if ! /tmp/test_game_loop; then
    echo "VERIFY_FAILED: Game loop test failed"
    rm -f /tmp/mock_game_platform.h /tmp/test_game_loop
    exit 1
fi

rm -f /tmp/mock_game_platform.h /tmp/test_game_loop

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Create `include/game/game.h`
2. Create `src/game/game_loop.cpp`
3. Create `src/game/init.cpp`
4. Run verification script

## Success Criteria
- Game initializes without errors
- Main loop runs and exits cleanly
- Input processing works
- Logic updates at fixed rate
- Rendering produces visible output
- Clean shutdown with no leaks

## Common Issues

### Loop runs too fast
- Check tick interval calculation
- Verify Platform_Timer_GetTicks()

### Input not responding
- Ensure Platform_Input_Update() called
- Check key code mappings

### Display not updating
- Verify Render() is called
- Check Lock/Unlock sequence

## Completion Promise
When verification passes, output:
```
<promise>TASK_14I_COMPLETE</promise>
```

## Escape Hatch
If stuck after 15 iterations:
- Document what's blocking in `ralph-tasks/blocked/14i.md`
- Output: `<promise>TASK_14I_BLOCKED</promise>`

## Max Iterations
15
