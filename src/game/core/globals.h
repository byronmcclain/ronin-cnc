/**
 * Game Global State
 *
 * Central access to game state. Replaces the scattered globals
 * from EXTERNS.H with a more organized structure.
 */

#ifndef GAME_CORE_GLOBALS_H
#define GAME_CORE_GLOBALS_H

#include "types.h"
#include "rtti.h"

// Forward declarations
class DisplayClass;
class HouseClass;
class ObjectClass;

/**
 * GameState - Central game state container
 *
 * Instead of dozens of global variables, we use a single state object.
 * This makes save/load easier and state management clearer.
 */
struct GameState {
    // Frame counter (increments each logic tick)
    int Frame;

    // Current game speed setting (0=slowest, 7=fastest)
    int GameSpeed;

    // Is game paused?
    bool IsPaused;

    // Is game in debug mode?
    bool IsDebug;

    // Current scenario info
    struct {
        int Number;
        TheaterType Theater;
        HousesType PlayerHouse;
        int Difficulty;  // 0=easy, 1=normal, 2=hard
        char Name[64];
    } Scenario;

    // Screen/display state
    struct {
        int TacticalX;      // Viewport top-left in pixels
        int TacticalY;
        int TacticalWidth;  // Viewport size
        int TacticalHeight;
        bool NeedRedraw;
    } View;

    // Current player
    HouseClass* PlayerPtr;

    // Global flags
    bool GameOver;
    bool PlayerWins;
    bool PlayerLoses;
};

// Global game state instance
extern GameState* Game;

// Convenience accessors
inline int Frame() { return Game ? Game->Frame : 0; }
inline bool IsPaused() { return Game ? Game->IsPaused : false; }
inline HouseClass* PlayerHouse() { return Game ? Game->PlayerPtr : nullptr; }
inline TheaterType CurrentTheater() { return Game ? Game->Scenario.Theater : THEATER_TEMPERATE; }

// Display pointer (for rendering)
extern DisplayClass* Map;

// Object lists (simplified from original)
// Original had complex linked lists; we use vectors for simplicity
#include <vector>
extern std::vector<ObjectClass*> AllObjects;
extern std::vector<ObjectClass*> DisplayObjects;

// Initialize globals
void Globals_Init();
void Globals_Shutdown();

#endif // GAME_CORE_GLOBALS_H
