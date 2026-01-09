/**
 * Game Global State Implementation
 */

#include "globals.h"
#include "platform.h"

// Global state instance
GameState* Game = nullptr;

// Display pointer
DisplayClass* Map = nullptr;

// Object lists
std::vector<ObjectClass*> AllObjects;
std::vector<ObjectClass*> DisplayObjects;

void Globals_Init() {
    if (Game) return; // Already initialized

    Game = new GameState();

    // Initialize to defaults
    Game->Frame = 0;
    Game->GameSpeed = 4; // Medium speed
    Game->IsPaused = false;
    Game->IsDebug = false;

    Game->Scenario.Number = 0;
    Game->Scenario.Theater = THEATER_TEMPERATE;
    Game->Scenario.PlayerHouse = HOUSE_GOOD;
    Game->Scenario.Difficulty = 1;
    Game->Scenario.Name[0] = '\0';

    Game->View.TacticalX = 0;
    Game->View.TacticalY = 0;
    Game->View.TacticalWidth = 640;  // GAME_WIDTH
    Game->View.TacticalHeight = 400; // GAME_HEIGHT
    Game->View.NeedRedraw = true;

    Game->PlayerPtr = nullptr;
    Game->GameOver = false;
    Game->PlayerWins = false;
    Game->PlayerLoses = false;

    // Reserve space for objects
    AllObjects.reserve(1024);
    DisplayObjects.reserve(256);

    Platform_LogInfo("Game globals initialized");
}

void Globals_Shutdown() {
    // Clear object lists (objects themselves are managed elsewhere)
    AllObjects.clear();
    DisplayObjects.clear();

    delete Game;
    Game = nullptr;
    Map = nullptr;

    Platform_LogInfo("Game globals shutdown");
}
