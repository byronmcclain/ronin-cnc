# Task 14a: Project Structure Setup

## Dependencies
- Phase 13 (Compatibility Layer) must be complete
- Platform layer must be fully functional
- Asset system (MIX loading) must work

## Context
Before porting any game code, we need a proper project structure. The original Red Alert code has ~277 files in CODE/ with deep interdependencies. We'll create a clean src/game/ directory with modern organization while maintaining compatibility with original code patterns.

This task establishes the foundation that all subsequent porting tasks will build upon.

## Objective
1. Create the src/game/ directory structure
2. Set up CMake for game code compilation
3. Create foundational header files
4. Establish include paths and dependencies
5. Verify the structure compiles

## Technical Background

### Original CODE/ Structure
The original code is organized with:
- All .CPP and .H files in a single flat directory
- FUNCTION.H as the master include (includes everything)
- Heavy use of global variables in EXTERNS.H
- Class hierarchy defined across multiple files

### New Structure
We'll use a more modular approach:
```
src/game/
├── core/           # Core types and definitions
│   ├── types.h     # COORDINATE, CELL, etc.
│   ├── rtti.h      # Runtime type identification
│   └── globals.h   # Game state globals
├── screen/         # Display hierarchy
│   ├── gscreen.h/cpp
│   ├── map.h/cpp
│   └── display.h/cpp
├── map/            # Map and cell system
│   ├── cell.h/cpp
│   └── terrain.h
├── objects/        # Game objects
│   ├── object.h/cpp
│   ├── techno.h/cpp
│   └── ...
├── types/          # Type definitions (unit stats, etc.)
│   ├── type_class.h
│   └── unit_type.h/cpp
├── game.h          # Master include for game code
└── main_game.cpp   # Entry point
```

## Deliverables
- [ ] Directory structure created
- [ ] CMakeLists.txt updated with game targets
- [ ] Base header files created
- [ ] Compiles without errors
- [ ] Verification script passes

## Files to Create

### src/game/game.h (NEW)
```cpp
/**
 * Red Alert Game Code - Master Header
 *
 * This header provides access to all ported game code.
 * Include this instead of the original FUNCTION.H.
 */

#ifndef GAME_H
#define GAME_H

// Compatibility layer (Windows types, etc.)
#include "compat/compat.h"

// Platform layer
#include "platform.h"

// Core game types
#include "core/types.h"
#include "core/rtti.h"
#include "core/globals.h"

// Game version info
#define GAME_VERSION_MAJOR 1
#define GAME_VERSION_MINOR 0
#define GAME_VERSION_PATCH 0
#define GAME_VERSION_STRING "1.0.0-port"

// Original game constants
#define GAME_FRAME_RATE 15       // Original game logic rate (15 FPS)
#define RENDER_FRAME_RATE 60     // Target render rate
#define TICKS_PER_SECOND 60      // Game ticks per second

// Screen dimensions (original)
#define GAME_WIDTH 640
#define GAME_HEIGHT 400

// Map dimensions
#define MAP_MAX_WIDTH 128
#define MAP_MAX_HEIGHT 128
#define MAP_CELL_SIZE 24         // Pixels per cell (isometric)

// Cell dimensions
#define CELL_PIXEL_W 24
#define CELL_PIXEL_H 24
#define ICON_PIXEL_W 24
#define ICON_PIXEL_H 24

// Lepton (sub-cell) resolution
#define CELL_LEPTON_W 256
#define CELL_LEPTON_H 256
#define ICON_LEPTON_W 256
#define ICON_LEPTON_H 256

// Forward declarations for major classes
class GScreenClass;
class MapClass;
class DisplayClass;
class CellClass;
class ObjectClass;
class TechnoClass;
class HouseClass;

// Game state access
namespace Game {
    bool IsInitialized();
    bool IsRunning();
    int GetFrame();
    uint32_t GetTicks();

    // Initialization
    bool Init(int argc, char* argv[]);
    void Shutdown();

    // Main loop
    void Run();
    void ProcessFrame();
}

#endif // GAME_H
```

### src/game/core/types.h (NEW)
```cpp
/**
 * Core Game Types
 *
 * Fundamental type definitions used throughout the game code.
 * These match the original Westwood definitions for compatibility.
 */

#ifndef GAME_CORE_TYPES_H
#define GAME_CORE_TYPES_H

#include "compat/compat.h"
#include <cstdint>

// =============================================================================
// Coordinate Types
// =============================================================================

/**
 * CELL - Map cell index (0-16383 for 128x128 map)
 * Layout: Y * MAP_WIDTH + X
 */
typedef int16_t CELL;

/**
 * COORDINATE - World position in leptons (sub-pixel units)
 * Format: (Y << 16) | X, where each component is 0-32767
 * 256 leptons = 1 pixel
 *
 * Original uses fixed-point: actual_pixel = lepton / 256
 */
typedef uint32_t COORDINATE;

/**
 * LEPTON - Sub-pixel unit (256 per pixel)
 */
typedef int16_t LEPTON;

// CELL constants
#define CELL_NONE       ((CELL)-1)
#define CELL_MAX        (MAP_MAX_WIDTH * MAP_MAX_HEIGHT)

// COORDINATE constants
#define COORD_NONE      ((COORDINATE)0xFFFFFFFF)

// =============================================================================
// Coordinate Conversion Macros
// =============================================================================

// Extract X/Y from COORDINATE
#define Coord_X(coord)      ((LEPTON)((coord) & 0xFFFF))
#define Coord_Y(coord)      ((LEPTON)(((coord) >> 16) & 0xFFFF))

// Create COORDINATE from X/Y leptons
#define XY_Coord(x, y)      ((COORDINATE)(((uint32_t)(y) << 16) | ((uint32_t)(x) & 0xFFFF)))

// Convert leptons to pixels
#define Lepton_To_Pixel(l)  ((l) / 256)
#define Pixel_To_Lepton(p)  ((p) * 256)

// Convert COORDINATE to pixel position
#define Coord_To_Pixel_X(c) (Lepton_To_Pixel(Coord_X(c)))
#define Coord_To_Pixel_Y(c) (Lepton_To_Pixel(Coord_Y(c)))

// CELL to COORDINATE (center of cell)
inline COORDINATE Cell_To_Coord(CELL cell) {
    if (cell == CELL_NONE) return COORD_NONE;
    int x = (cell % MAP_MAX_WIDTH) * CELL_LEPTON_W + CELL_LEPTON_W / 2;
    int y = (cell / MAP_MAX_WIDTH) * CELL_LEPTON_H + CELL_LEPTON_H / 2;
    return XY_Coord(x, y);
}

// COORDINATE to CELL
inline CELL Coord_To_Cell(COORDINATE coord) {
    if (coord == COORD_NONE) return CELL_NONE;
    int x = Coord_X(coord) / CELL_LEPTON_W;
    int y = Coord_Y(coord) / CELL_LEPTON_H;
    if (x < 0 || x >= MAP_MAX_WIDTH || y < 0 || y >= MAP_MAX_HEIGHT) {
        return CELL_NONE;
    }
    return (CELL)(y * MAP_MAX_WIDTH + x);
}

// Cell X/Y extraction
#define Cell_X(cell)        ((cell) % MAP_MAX_WIDTH)
#define Cell_Y(cell)        ((cell) / MAP_MAX_WIDTH)
#define XY_Cell(x, y)       ((CELL)((y) * MAP_MAX_WIDTH + (x)))

// =============================================================================
// Direction Types
// =============================================================================

/**
 * DirType - 8-way direction (0-7, or 0-255 for fine direction)
 * 0 = North, increases clockwise
 */
typedef uint8_t DirType;

// 8-way directions
enum Dir8Type : uint8_t {
    DIR_N  = 0,    // North
    DIR_NE = 32,   // Northeast
    DIR_E  = 64,   // East
    DIR_SE = 96,   // Southeast
    DIR_S  = 128,  // South
    DIR_SW = 160,  // Southwest
    DIR_W  = 192,  // West
    DIR_NW = 224,  // Northwest
    DIR_COUNT = 8
};

// Direction macros
#define Dir_To_8(dir)       ((dir) / 32)
#define Dir_From_8(d8)      ((d8) * 32)

// =============================================================================
// Facing Types
// =============================================================================

/**
 * FacingType - Unit/building facing (32 values for smooth rotation)
 */
typedef int8_t FacingType;

#define FACING_COUNT    32
#define FACING_NONE     ((FacingType)-1)

// Convert between direction and facing
#define Dir_To_Facing(d)    ((FacingType)((d) / 8))
#define Facing_To_Dir(f)    ((DirType)((f) * 8))

// =============================================================================
// House/Player Types
// =============================================================================

/**
 * HousesType - Player/faction identifier
 */
enum HousesType : int8_t {
    HOUSE_NONE = -1,
    HOUSE_SPAIN = 0,
    HOUSE_GREECE,
    HOUSE_USSR,
    HOUSE_ENGLAND,
    HOUSE_UKRAINE,
    HOUSE_GERMANY,
    HOUSE_FRANCE,
    HOUSE_TURKEY,
    HOUSE_GOOD,      // Allied (mission)
    HOUSE_BAD,       // Soviet (mission)
    HOUSE_NEUTRAL,
    HOUSE_SPECIAL,
    HOUSE_MULTI1,
    HOUSE_MULTI2,
    HOUSE_MULTI3,
    HOUSE_MULTI4,
    HOUSE_MULTI5,
    HOUSE_MULTI6,
    HOUSE_MULTI7,
    HOUSE_MULTI8,
    HOUSE_COUNT
};

// Is this a multiplayer house?
inline bool House_Is_Multi(HousesType h) {
    return h >= HOUSE_MULTI1 && h <= HOUSE_MULTI8;
}

// =============================================================================
// Armor Types
// =============================================================================

enum ArmorType : int8_t {
    ARMOR_NONE = 0,
    ARMOR_WOOD,
    ARMOR_LIGHT,
    ARMOR_HEAVY,
    ARMOR_CONCRETE,
    ARMOR_COUNT
};

// =============================================================================
// Land Types (terrain passability)
// =============================================================================

enum LandType : int8_t {
    LAND_CLEAR = 0,
    LAND_ROAD,
    LAND_WATER,
    LAND_ROCK,
    LAND_WALL,
    LAND_TIBERIUM,
    LAND_BEACH,
    LAND_ROUGH,
    LAND_RIVER,
    LAND_COUNT
};

// =============================================================================
// Speed Types
// =============================================================================

enum SpeedType : int8_t {
    SPEED_NONE = 0,
    SPEED_FOOT,
    SPEED_TRACK,
    SPEED_WHEEL,
    SPEED_WINGED,
    SPEED_FLOAT,
    SPEED_HOVER,
    SPEED_COUNT
};

// =============================================================================
// Theater Types
// =============================================================================

enum TheaterType : int8_t {
    THEATER_NONE = -1,
    THEATER_TEMPERATE = 0,
    THEATER_SNOW,
    THEATER_INTERIOR,
    THEATER_COUNT
};

// Theater name prefixes (for asset loading)
inline const char* Theater_Prefix(TheaterType t) {
    switch (t) {
        case THEATER_TEMPERATE: return "TEMPERAT";
        case THEATER_SNOW:      return "SNOW";
        case THEATER_INTERIOR:  return "INTERIOR";
        default:                return "TEMPERAT";
    }
}

// =============================================================================
// Miscellaneous Types
// =============================================================================

// Target type (can be CELL or object pointer encoded)
typedef uint32_t TARGET;

#define TARGET_NONE ((TARGET)0)

// Fixed-point value (8.8 format)
typedef int16_t FIXED;

#define FIXED_ONE   256
#define Fixed_To_Int(f)     ((f) >> 8)
#define Int_To_Fixed(i)     ((i) << 8)

#endif // GAME_CORE_TYPES_H
```

### src/game/core/rtti.h (NEW)
```cpp
/**
 * Runtime Type Identification
 *
 * RTTI system for game objects. Allows safe casting and type checking.
 * Matches original Westwood implementation.
 */

#ifndef GAME_CORE_RTTI_H
#define GAME_CORE_RTTI_H

#include <cstdint>

/**
 * RTTIType - Runtime type identifier for all game objects
 */
enum RTTIType : int8_t {
    RTTI_NONE = 0,

    // Abstract types
    RTTI_ABSTRACT,
    RTTI_OBJECT,
    RTTI_MISSION,
    RTTI_RADIO,
    RTTI_TECHNO,
    RTTI_FOOT,

    // Concrete object types
    RTTI_UNIT,
    RTTI_INFANTRY,
    RTTI_BUILDING,
    RTTI_AIRCRAFT,
    RTTI_VESSEL,
    RTTI_BULLET,
    RTTI_ANIM,
    RTTI_TERRAIN,
    RTTI_OVERLAY,
    RTTI_SMUDGE,
    RTTI_TEMPLATE,

    // Type class types (for editor/data)
    RTTI_UNITTYPE,
    RTTI_INFANTRYTYPE,
    RTTI_BUILDINGTYPE,
    RTTI_AIRCRAFTTYPE,
    RTTI_VESSELTYPE,
    RTTI_BULLETTYPE,
    RTTI_ANIMTYPE,
    RTTI_TERRAINTYPE,
    RTTI_OVERLAYTYPE,
    RTTI_SMUDGETYPE,
    RTTI_TEMPLATETYPE,

    // Special types
    RTTI_HOUSE,
    RTTI_TRIGGER,
    RTTI_TEAM,
    RTTI_TEAMTYPE,

    RTTI_COUNT
};

/**
 * Get RTTI name for debugging
 */
inline const char* RTTI_Name(RTTIType rtti) {
    static const char* names[] = {
        "None",
        "Abstract", "Object", "Mission", "Radio", "Techno", "Foot",
        "Unit", "Infantry", "Building", "Aircraft", "Vessel",
        "Bullet", "Anim", "Terrain", "Overlay", "Smudge", "Template",
        "UnitType", "InfantryType", "BuildingType", "AircraftType", "VesselType",
        "BulletType", "AnimType", "TerrainType", "OverlayType", "SmudgeType", "TemplateType",
        "House", "Trigger", "Team", "TeamType"
    };
    if (rtti >= 0 && rtti < RTTI_COUNT) {
        return names[rtti];
    }
    return "Unknown";
}

/**
 * Check if RTTI is a mobile object (can move)
 */
inline bool RTTI_Is_Foot(RTTIType rtti) {
    return rtti == RTTI_UNIT ||
           rtti == RTTI_INFANTRY ||
           rtti == RTTI_AIRCRAFT ||
           rtti == RTTI_VESSEL;
}

/**
 * Check if RTTI is a techno object (has weapons, can be owned)
 */
inline bool RTTI_Is_Techno(RTTIType rtti) {
    return rtti == RTTI_UNIT ||
           rtti == RTTI_INFANTRY ||
           rtti == RTTI_AIRCRAFT ||
           rtti == RTTI_VESSEL ||
           rtti == RTTI_BUILDING;
}

/**
 * Check if RTTI is a selectable object
 */
inline bool RTTI_Is_Selectable(RTTIType rtti) {
    return RTTI_Is_Techno(rtti);
}

#endif // GAME_CORE_RTTI_H
```

### src/game/core/globals.h (NEW)
```cpp
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
```

### CMakeLists.txt additions (MODIFY)
```cmake
# =============================================================================
# Game Code Library
# =============================================================================
# Ported Red Alert game code

# Game core sources
set(GAME_CORE_SOURCES
    src/game/core/globals.cpp
)

# Create game library (will grow as we port more code)
add_library(game_core STATIC
    ${GAME_CORE_SOURCES}
)

target_include_directories(game_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/compat
    ${CMAKE_SOURCE_DIR}/src/game
    ${CMAKE_SOURCE_DIR}/src/game/core
)

target_link_libraries(game_core PUBLIC
    compat
    redalert_platform
)

target_compile_features(game_core PUBLIC cxx_std_17)

# =============================================================================
# Game Structure Test
# =============================================================================

add_executable(TestGameStructure src/test_game_structure.cpp)

target_link_libraries(TestGameStructure PRIVATE
    game_core
    compat
    redalert_platform
)

add_test(NAME TestGameStructure COMMAND TestGameStructure)
```

### src/game/core/globals.cpp (NEW)
```cpp
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
    Game->View.TacticalWidth = GAME_WIDTH;
    Game->View.TacticalHeight = GAME_HEIGHT;
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
```

### src/test_game_structure.cpp (NEW)
```cpp
/**
 * Game Structure Test
 *
 * Verifies that the game code project structure is correctly set up.
 */

#include "game.h"
#include <cstdio>

#define TEST(name) printf("  Testing %s... ", name)
#define PASS() printf("PASS\n")
#define FAIL(msg) do { printf("FAIL: %s\n", msg); return 1; } while(0)

int test_types() {
    TEST("coordinate types");

    // Test CELL
    CELL cell = XY_Cell(10, 20);
    if (Cell_X(cell) != 10) FAIL("Cell_X incorrect");
    if (Cell_Y(cell) != 20) FAIL("Cell_Y incorrect");

    // Test COORDINATE
    COORDINATE coord = XY_Coord(1000, 2000);
    if (Coord_X(coord) != 1000) FAIL("Coord_X incorrect");
    if (Coord_Y(coord) != 2000) FAIL("Coord_Y incorrect");

    // Test conversion
    cell = Coord_To_Cell(Cell_To_Coord(cell));
    if (Cell_X(cell) != 10 || Cell_Y(cell) != 20) FAIL("Cell/Coord conversion failed");

    PASS();
    return 0;
}

int test_rtti() {
    TEST("RTTI system");

    if (RTTI_Name(RTTI_UNIT) == nullptr) FAIL("RTTI_Name returned null");
    if (!RTTI_Is_Techno(RTTI_BUILDING)) FAIL("Building should be techno");
    if (!RTTI_Is_Foot(RTTI_INFANTRY)) FAIL("Infantry should be foot");
    if (RTTI_Is_Foot(RTTI_BUILDING)) FAIL("Building should not be foot");

    PASS();
    return 0;
}

int test_globals() {
    TEST("global state");

    Globals_Init();

    if (!Game) FAIL("Game is null after init");
    if (Game->Frame != 0) FAIL("Frame should be 0");
    if (Game->IsPaused) FAIL("Should not be paused");
    if (CurrentTheater() != THEATER_TEMPERATE) FAIL("Theater should be temperate");

    // Modify and verify
    Game->Frame = 100;
    if (Frame() != 100) FAIL("Frame() accessor broken");

    Globals_Shutdown();

    if (Game != nullptr) FAIL("Game should be null after shutdown");

    PASS();
    return 0;
}

int test_constants() {
    TEST("game constants");

    // Verify constants are defined
    if (GAME_WIDTH != 640) FAIL("GAME_WIDTH should be 640");
    if (GAME_HEIGHT != 400) FAIL("GAME_HEIGHT should be 400");
    if (MAP_MAX_WIDTH != 128) FAIL("MAP_MAX_WIDTH should be 128");
    if (CELL_PIXEL_W != 24) FAIL("CELL_PIXEL_W should be 24");

    // Verify lepton math
    if (Lepton_To_Pixel(256) != 1) FAIL("Lepton conversion broken");
    if (Pixel_To_Lepton(1) != 256) FAIL("Pixel conversion broken");

    PASS();
    return 0;
}

int main() {
    printf("==========================================\n");
    printf("Game Structure Test\n");
    printf("==========================================\n\n");

    // Initialize platform
    if (Platform_Init() != PLATFORM_RESULT_SUCCESS) {
        printf("Failed to initialize platform!\n");
        return 1;
    }

    printf("=== Running Tests ===\n\n");

    int failures = 0;
    failures += test_types();
    failures += test_rtti();
    failures += test_globals();
    failures += test_constants();

    printf("\n==========================================\n");

    if (failures == 0) {
        printf("All tests passed!\n");
        printf("Game structure is correctly set up.\n");
    } else {
        printf("%d test(s) failed\n", failures);
    }

    printf("==========================================\n");

    Platform_Shutdown();
    return failures;
}
```

## Verification Command
```bash
ralph-tasks/verify/check-game-structure.sh
```

## Verification Script

### ralph-tasks/verify/check-game-structure.sh (NEW)
```bash
#!/bin/bash
# Verification script for game project structure

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Game Project Structure ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check directories exist
echo "[1/5] Checking directory structure..."
for dir in src/game src/game/core; do
    if [ ! -d "$dir" ]; then
        echo "VERIFY_FAILED: $dir not found"
        exit 1
    fi
done
echo "  OK: Directories exist"

# Step 2: Check header files exist
echo "[2/5] Checking header files..."
for header in src/game/game.h src/game/core/types.h src/game/core/rtti.h src/game/core/globals.h; do
    if [ ! -f "$header" ]; then
        echo "VERIFY_FAILED: $header not found"
        exit 1
    fi
done
echo "  OK: Headers exist"

# Step 3: Syntax check headers
echo "[3/5] Checking header syntax..."
if ! clang++ -std=c++17 -fsyntax-only \
    -I include \
    -I src/game \
    -I src/game/core \
    src/game/game.h 2>&1; then
    echo "VERIFY_FAILED: Headers have syntax errors"
    exit 1
fi
echo "  OK: Headers compile"

# Step 4: Build test
echo "[4/5] Building TestGameStructure..."
cmake --build build --target TestGameStructure 2>&1 || {
    echo "VERIFY_FAILED: Build failed"
    exit 1
}
echo "  OK: Build succeeded"

# Step 5: Run test
echo "[5/5] Running TestGameStructure..."
if ! ./build/TestGameStructure; then
    echo "VERIFY_FAILED: Test failed"
    exit 1
fi

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Create directories: `mkdir -p src/game/core src/game/screen src/game/map src/game/objects src/game/types`
2. Create `src/game/game.h`
3. Create `src/game/core/types.h`
4. Create `src/game/core/rtti.h`
5. Create `src/game/core/globals.h`
6. Create `src/game/core/globals.cpp`
7. Create `src/test_game_structure.cpp`
8. Add CMake configuration
9. Build and run tests
10. Run verification script

## Success Criteria
- All directories created
- All headers compile without errors
- Type conversions work correctly
- RTTI system functional
- Global state initializes and shuts down cleanly
- TestGameStructure passes all tests

## Common Issues

### Include path errors
- Ensure CMake sets up include directories correctly
- Check that game.h includes are in correct order

### Circular dependencies
- Forward declare classes where needed
- Keep core/types.h dependency-free

### MAP_MAX_WIDTH not defined
- Ensure game.h is included before types.h
- Or define constants in types.h directly

## Completion Promise
When verification passes, output:
```
<promise>TASK_14A_COMPLETE</promise>
```

## Escape Hatch
If stuck after 15 iterations:
- Document what's blocking in `ralph-tasks/blocked/14a.md`
- Output: `<promise>TASK_14A_BLOCKED</promise>`

## Max Iterations
15
