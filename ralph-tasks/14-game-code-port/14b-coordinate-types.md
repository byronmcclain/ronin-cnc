# Task 14b: Coordinate & Type Definitions

## Dependencies
- Task 14a (Project Structure Setup) must be complete

## Context
Red Alert uses a unique coordinate system where positions are stored as 32-bit "leptons" (1/256th of a pixel). Understanding this system is critical for porting any position-based code. This task creates the complete type definitions used throughout the game.

## Objective
Create comprehensive type definitions including:
1. COORDINATE type (32-bit lepton-based world position)
2. CELL type (16-bit cell index)
3. Direction/facing types
4. House/side definitions
5. Weapon and armor types
6. Coordinate conversion functions

## Technical Background

### Red Alert Coordinate System

The original game uses a fixed-point coordinate system:

```
COORDINATE (32-bit):
  High 16 bits = Cell position (0-127 on 128x128 map)
  Low 16 bits  = Sub-cell position (256 leptons = 1 pixel)

CELL (16-bit):
  High byte = Y cell (0-127)
  Low byte  = X cell (0-127)

Conversion:
  pixel = lepton >> 8
  cell = coordinate >> 24 (Y) | (coordinate >> 8) & 0xFF (X)
```

### Direction System

The game uses 256-step directions:
- 0 = North
- 64 = East
- 128 = South
- 192 = West

This allows smooth rotation with 8-directional sprites.

### House System

Each faction (Allied, Soviet) has distinct houses:
- GoodGuy (player's Allies)
- BadGuy (enemy Soviet)
- Neutral (civilian)
- Special (scripted events)
- Multi1-Multi8 (multiplayer)

## Deliverables
- [ ] `include/game/coord.h` - Complete coordinate types and conversions
- [ ] `include/game/facing.h` - Direction/facing system
- [ ] `include/game/house.h` - House/side definitions
- [ ] `include/game/weapon.h` - Weapon and armor enums
- [ ] `src/game/core/coord.cpp` - Coordinate math implementations
- [ ] Unit test for coordinate conversions
- [ ] Verification script passes

## Files to Create

### include/game/coord.h (NEW)
```cpp
/**
 * Red Alert Coordinate System
 *
 * The original game uses a fixed-point coordinate system where world positions
 * are stored as 32-bit "leptons" (1/256th of a pixel). This allows sub-pixel
 * precision for smooth movement while using integer math.
 *
 * Terminology:
 * - LEPTON: Smallest unit of measurement (1/256 pixel)
 * - CELL: 24x24 pixel terrain tile
 * - COORDINATE: Full world position (cell + sub-cell leptons)
 */

#pragma once

#include <cstdint>

// =============================================================================
// Core Coordinate Types
// =============================================================================

/**
 * COORDINATE - 32-bit world position in leptons
 *
 * Format:
 *   Bits 31-24: X cell (0-127)
 *   Bits 23-16: X sub-cell (0-255 leptons within cell)
 *   Bits 15-8:  Y cell (0-127)
 *   Bits 7-0:   Y sub-cell (0-255 leptons within cell)
 *
 * Alternative view:
 *   High word (bits 31-16): X position in leptons
 *   Low word (bits 15-0):   Y position in leptons
 */
typedef uint32_t COORDINATE;

/**
 * CELL - 16-bit cell index
 *
 * Format:
 *   Bits 15-8: Y cell (0-127)
 *   Bits 7-0:  X cell (0-127)
 *
 * This provides direct array indexing: Cells[cell]
 */
typedef int16_t CELL;

/**
 * LEPTON - Sub-pixel measurement unit
 * 256 leptons = 1 pixel
 * 256 * 24 = 6144 leptons = 1 cell
 */
typedef int16_t LEPTON;

// =============================================================================
// Constants
// =============================================================================

// Lepton constants
constexpr int LEPTON_SHIFT = 8;                    // Bits to shift for lepton->pixel
constexpr int LEPTON_PER_PIXEL = 256;              // Leptons per pixel
constexpr int CELL_PIXEL_SIZE = 24;                // Pixels per cell
constexpr int LEPTON_PER_CELL = CELL_PIXEL_SIZE * LEPTON_PER_PIXEL; // 6144

// Map dimensions (standard scenario map)
constexpr int MAP_CELL_WIDTH = 128;
constexpr int MAP_CELL_HEIGHT = 128;
constexpr int MAP_CELL_TOTAL = MAP_CELL_WIDTH * MAP_CELL_HEIGHT;

// Map pixel dimensions
constexpr int MAP_PIXEL_WIDTH = MAP_CELL_WIDTH * CELL_PIXEL_SIZE;   // 3072
constexpr int MAP_PIXEL_HEIGHT = MAP_CELL_HEIGHT * CELL_PIXEL_SIZE; // 3072

// Map lepton dimensions
constexpr int MAP_LEPTON_WIDTH = MAP_CELL_WIDTH * LEPTON_PER_CELL;
constexpr int MAP_LEPTON_HEIGHT = MAP_CELL_HEIGHT * LEPTON_PER_CELL;

// Invalid/null values
constexpr COORDINATE COORD_NONE = 0xFFFFFFFF;
constexpr CELL CELL_NONE = -1;

// =============================================================================
// Coordinate Macros (from original DEFINES.H)
// =============================================================================

/**
 * Extract X lepton from coordinate
 * Original: ((coord) >> 16)
 */
#define Coord_X(coord) ((int16_t)(((COORDINATE)(coord)) >> 16))

/**
 * Extract Y lepton from coordinate
 * Original: ((coord) & 0xFFFF)
 */
#define Coord_Y(coord) ((int16_t)(((COORDINATE)(coord)) & 0xFFFF))

/**
 * Extract X cell from coordinate
 */
#define Coord_XCell(coord) ((uint8_t)(((COORDINATE)(coord)) >> 24))

/**
 * Extract Y cell from coordinate
 */
#define Coord_YCell(coord) ((uint8_t)((((COORDINATE)(coord)) >> 8) & 0xFF))

/**
 * Extract X pixel from coordinate
 */
#define Coord_XPixel(coord) (Coord_X(coord) >> LEPTON_SHIFT)

/**
 * Extract Y pixel from coordinate
 */
#define Coord_YPixel(coord) (Coord_Y(coord) >> LEPTON_SHIFT)

/**
 * Build coordinate from X and Y leptons
 */
#define XY_Coord(x, y) ((COORDINATE)((((uint32_t)(x)) << 16) | ((uint32_t)(y) & 0xFFFF)))

/**
 * Build cell from X and Y cell indices
 */
#define XY_Cell(x, y) ((CELL)(((y) << 8) | ((x) & 0xFF)))

/**
 * Extract X from cell
 */
#define Cell_X(cell) ((int)((cell) & 0xFF))

/**
 * Extract Y from cell
 */
#define Cell_Y(cell) ((int)(((cell) >> 8) & 0xFF))

/**
 * Convert cell to array index
 */
#define Cell_Index(cell) ((int)(cell))

// =============================================================================
// Coordinate Functions (implementations in coord.cpp)
// =============================================================================

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Convert CELL to COORDINATE (centers on cell)
 * Returns coordinate at center of the given cell
 */
COORDINATE Cell_Coord(CELL cell);

/**
 * Convert COORDINATE to CELL
 * Returns cell containing the coordinate
 */
CELL Coord_Cell(COORDINATE coord);

/**
 * Get distance between two coordinates (in leptons)
 * Uses integer approximation of sqrt(dx^2 + dy^2)
 */
int Coord_Distance(COORDINATE coord1, COORDINATE coord2);

/**
 * Calculate direction from one coordinate to another
 * Returns 0-255 direction value (0=North, 64=East, 128=South, 192=West)
 */
uint8_t Coord_Direction(COORDINATE from, COORDINATE to);

/**
 * Move coordinate by distance in given direction
 */
COORDINATE Coord_Move(COORDINATE coord, uint8_t direction, int distance);

/**
 * Check if coordinate is within map bounds
 */
int Coord_InMap(COORDINATE coord);

/**
 * Check if cell is within map bounds
 */
int Cell_InMap(CELL cell);

/**
 * Get adjacent cell in given direction
 * Returns CELL_NONE if out of bounds
 */
CELL Adjacent_Cell(CELL cell, int direction);

/**
 * Snap coordinate to cell center
 */
COORDINATE Coord_Snap(COORDINATE coord);

/**
 * Convert pixel coordinates to world coordinate
 */
COORDINATE Pixel_To_Coord(int pixel_x, int pixel_y);

/**
 * Convert world coordinate to pixel coordinates
 */
void Coord_To_Pixel(COORDINATE coord, int* pixel_x, int* pixel_y);

/**
 * Calculate cell distance (Manhattan or Chebyshev)
 */
int Cell_Distance(CELL cell1, CELL cell2);

#ifdef __cplusplus
}
#endif

// =============================================================================
// Direction Offsets for Adjacent Cells
// =============================================================================

/**
 * Facing direction to cell offset mapping
 * Use: Adjacent_Cell(cell, FACING_N) to get cell to the north
 */
enum FacingType {
    FACING_N = 0,      // North (up)
    FACING_NE = 1,     // Northeast
    FACING_E = 2,      // East (right)
    FACING_SE = 3,     // Southeast
    FACING_S = 4,      // South (down)
    FACING_SW = 5,     // Southwest
    FACING_W = 6,      // West (left)
    FACING_NW = 7,     // Northwest
    FACING_COUNT = 8,
    FACING_NONE = -1
};

// Cell offsets for each facing direction
// Index with FacingType to get delta-cell value
extern const int8_t FacingOffset_X[8];
extern const int8_t FacingOffset_Y[8];
extern const int16_t FacingOffset_Cell[8];
```

### include/game/facing.h (NEW)
```cpp
/**
 * Red Alert Facing/Direction System
 *
 * The game uses a 256-step direction system for smooth rotation.
 * Sprites typically have 8 or 32 rotational frames.
 *
 * Direction values:
 *   0 = North (up)
 *   32 = NE
 *   64 = East (right)
 *   96 = SE
 *   128 = South (down)
 *   160 = SW
 *   192 = West (left)
 *   224 = NW
 */

#pragma once

#include <cstdint>

// =============================================================================
// Direction Type
// =============================================================================

/**
 * DirType - 256-step direction (0-255)
 * Used for unit facing, turret rotation, projectile trajectory
 */
typedef uint8_t DirType;

// =============================================================================
// Direction Constants
// =============================================================================

constexpr DirType DIR_N   = 0;     // North (up)
constexpr DirType DIR_NE  = 32;    // Northeast
constexpr DirType DIR_E   = 64;    // East (right)
constexpr DirType DIR_SE  = 96;    // Southeast
constexpr DirType DIR_S   = 128;   // South (down)
constexpr DirType DIR_SW  = 160;   // Southwest
constexpr DirType DIR_W   = 192;   // West (left)
constexpr DirType DIR_NW  = 224;   // Northwest

// Special values
constexpr DirType DIR_NONE = 255;  // Invalid/unset direction

// Rotation amounts
constexpr DirType DIR_ROTATION_45 = 32;   // 45 degrees
constexpr DirType DIR_ROTATION_90 = 64;   // 90 degrees
constexpr DirType DIR_ROTATION_180 = 128; // 180 degrees

// =============================================================================
// FacingClass - Smooth rotation handler
// =============================================================================

/**
 * FacingClass handles smooth rotation between directions.
 * Used by units and turrets to turn gradually rather than snap.
 *
 * Original location: CODE/FACING.H, CODE/FACING.CPP
 */
class FacingClass {
public:
    FacingClass() : current_(DIR_N), desired_(DIR_N), rate_(0) {}
    explicit FacingClass(DirType initial) : current_(initial), desired_(initial), rate_(0) {}

    // Get current facing
    DirType Current() const { return current_; }

    // Get desired (target) facing
    DirType Desired() const { return desired_; }

    // Set new desired facing
    void Set_Desired(DirType dir) { desired_ = dir; }

    // Set rotation rate (steps per tick)
    void Set_Rate(uint8_t rate) { rate_ = rate; }

    // Snap to desired facing instantly
    void Snap() { current_ = desired_; }

    // Is at desired facing?
    bool Is_At_Target() const { return current_ == desired_; }

    // Get difference between current and desired
    int8_t Difference() const;

    // Update facing towards desired (call each tick)
    // Returns true if facing changed
    bool Rotate();

    // Get which way to rotate (+1 = clockwise, -1 = counter-clockwise)
    int Rotation_Direction() const;

    // Set both current and desired
    void Set(DirType dir) {
        current_ = dir;
        desired_ = dir;
    }

private:
    DirType current_;    // Current facing direction
    DirType desired_;    // Target facing direction
    uint8_t rate_;       // Rotation speed (0 = instant)
};

// =============================================================================
// Direction Utility Functions
// =============================================================================

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get opposite direction
 */
DirType Dir_Opposite(DirType dir);

/**
 * Normalize direction to 8-way facing (for sprite selection)
 * Returns 0-7 (N, NE, E, SE, S, SW, W, NW)
 */
int Dir_To_8Way(DirType dir);

/**
 * Normalize direction to 16-way facing
 * Returns 0-15
 */
int Dir_To_16Way(DirType dir);

/**
 * Normalize direction to 32-way facing
 * Returns 0-31
 */
int Dir_To_32Way(DirType dir);

/**
 * Convert 8-way facing back to DirType
 */
DirType Way8_To_Dir(int way);

/**
 * Shortest rotation direction from one dir to another
 * Returns positive for clockwise, negative for counter-clockwise
 */
int8_t Dir_Delta(DirType from, DirType to);

/**
 * Calculate direction from dx, dy deltas
 */
DirType Dir_From_XY(int dx, int dy);

/**
 * Get X component of direction vector (scaled by 256)
 */
int Dir_X_Factor(DirType dir);

/**
 * Get Y component of direction vector (scaled by 256)
 */
int Dir_Y_Factor(DirType dir);

#ifdef __cplusplus
}
#endif

// =============================================================================
// Precomputed Direction Tables
// =============================================================================

// Sine table for direction (256 entries, -127 to +127)
extern const int8_t DirSineTable[256];

// Cosine table for direction (256 entries, -127 to +127)
extern const int8_t DirCosineTable[256];

// 8-way facing for each 256 direction
extern const uint8_t Dir8Way[256];
```

### include/game/house.h (NEW)
```cpp
/**
 * Red Alert House/Faction Definitions
 *
 * Houses represent the different factions in the game:
 * - Spain, Greece, USSR, etc. for single player
 * - Multi1-Multi8 for multiplayer
 *
 * Each house has a side (Allied or Soviet) which determines
 * available units and buildings.
 */

#pragma once

#include <cstdint>

// =============================================================================
// House Types
// =============================================================================

/**
 * HousesType - All possible factions in the game
 *
 * Single-player houses have fixed sides:
 *   HOUSE_SPAIN through HOUSE_TURKEY = Allied
 *   HOUSE_USSR through HOUSE_BAD = Soviet
 *
 * Multiplayer houses are configurable.
 */
enum HousesType : int8_t {
    // Allied nations (single player)
    HOUSE_SPAIN = 0,       // Spain (tutorial)
    HOUSE_GREECE = 1,      // Greece
    HOUSE_USSR = 2,        // Soviet Union
    HOUSE_ENGLAND = 3,     // England
    HOUSE_UKRAINE = 4,     // Ukraine (Soviet ally)
    HOUSE_GERMANY = 5,     // Germany
    HOUSE_FRANCE = 6,      // France
    HOUSE_TURKEY = 7,      // Turkey

    // Special houses
    HOUSE_GOOD = 8,        // Generic Allied (GoodGuy)
    HOUSE_BAD = 9,         // Generic Soviet (BadGuy)
    HOUSE_NEUTRAL = 10,    // Civilians, wildlife
    HOUSE_SPECIAL = 11,    // Scripted events, triggers

    // Multiplayer houses
    HOUSE_MULTI1 = 12,
    HOUSE_MULTI2 = 13,
    HOUSE_MULTI3 = 14,
    HOUSE_MULTI4 = 15,
    HOUSE_MULTI5 = 16,
    HOUSE_MULTI6 = 17,
    HOUSE_MULTI7 = 18,
    HOUSE_MULTI8 = 19,

    HOUSE_COUNT = 20,
    HOUSE_NONE = -1
};

// =============================================================================
// Side Types
// =============================================================================

/**
 * SideType - The two main factions
 * Determines which units/buildings are available
 */
enum SideType : int8_t {
    SIDE_ALLIED = 0,     // Good guys (NATO)
    SIDE_SOVIET = 1,     // Bad guys (USSR)
    SIDE_NEUTRAL = 2,    // Civilians

    SIDE_COUNT = 3,
    SIDE_NONE = -1
};

// =============================================================================
// House Color
// =============================================================================

/**
 * PlayerColorType - Remap colors for units/buildings
 */
enum PlayerColorType : int8_t {
    PCOLOR_GOLD = 0,      // Yellow (default)
    PCOLOR_LTBLUE = 1,    // Light blue
    PCOLOR_RED = 2,       // Red (Soviet default)
    PCOLOR_GREEN = 3,     // Green
    PCOLOR_ORANGE = 4,    // Orange
    PCOLOR_GREY = 5,      // Grey
    PCOLOR_BLUE = 6,      // Blue (Allied default)
    PCOLOR_BROWN = 7,     // Brown

    PCOLOR_COUNT = 8,
    PCOLOR_NONE = -1
};

// =============================================================================
// House Utility Functions
// =============================================================================

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get side for a house
 */
SideType House_Side(HousesType house);

/**
 * Get default color for a house
 */
PlayerColorType House_Default_Color(HousesType house);

/**
 * Get house name string
 */
const char* House_Name(HousesType house);

/**
 * Get side name string
 */
const char* Side_Name(SideType side);

/**
 * Check if two houses are allies
 */
int Houses_Allied(HousesType house1, HousesType house2);

/**
 * Check if two houses are enemies
 */
int Houses_Enemy(HousesType house1, HousesType house2);

/**
 * Get house from name string
 */
HousesType House_From_Name(const char* name);

/**
 * Is this a multiplayer house?
 */
int House_Is_Multi(HousesType house);

#ifdef __cplusplus
}
#endif

// =============================================================================
// House Data Tables
// =============================================================================

/**
 * House information structure
 */
struct HouseInfo {
    const char* Name;           // Internal name (SPAIN, USSR, etc.)
    const char* FullName;       // Display name
    SideType Side;              // Allied or Soviet
    PlayerColorType Color;      // Default color
    const char* Suffix;         // Scenario filename suffix
};

extern const HouseInfo HouseInfoTable[HOUSE_COUNT];
```

### include/game/weapon.h (NEW)
```cpp
/**
 * Red Alert Weapon and Armor Definitions
 *
 * Defines the combat system types used throughout the game.
 */

#pragma once

#include <cstdint>

// =============================================================================
// Armor Types
// =============================================================================

/**
 * ArmorType - Damage resistance categories
 *
 * Different weapons have different effectiveness against armor types.
 * This creates rock-paper-scissors style combat balance.
 */
enum ArmorType : int8_t {
    ARMOR_NONE = 0,        // No armor (infantry, aircraft)
    ARMOR_WOOD = 1,        // Light structures
    ARMOR_LIGHT = 2,       // Light vehicles (jeeps, APCs)
    ARMOR_HEAVY = 3,       // Heavy tanks
    ARMOR_CONCRETE = 4,    // Buildings

    ARMOR_COUNT = 5
};

// =============================================================================
// Weapon Types
// =============================================================================

/**
 * WeaponType - All weapon systems in the game
 *
 * Each weapon has:
 * - Damage amount
 * - Range
 * - Rate of fire
 * - Projectile type
 * - Warhead type (determines armor effectiveness)
 */
enum WeaponType : int8_t {
    WEAPON_NONE = -1,

    // Infantry weapons
    WEAPON_COLT45 = 0,         // Pistol
    WEAPON_ZSU23 = 1,          // Anti-aircraft gun
    WEAPON_VULCAN = 2,         // Chaingun
    WEAPON_MAVERICK = 3,       // Missile
    WEAPON_CAMERA = 4,         // Spy camera (no damage)
    WEAPON_FIREBALL = 5,       // Flamethrower
    WEAPON_SNIPER = 6,         // Sniper rifle
    WEAPON_CHAINGUN = 7,       // Machine gun
    WEAPON_PISTOL = 8,         // Light pistol
    WEAPON_M1_CARBINE = 9,     // Rifle
    WEAPON_DRAGON = 10,        // Anti-tank missile
    WEAPON_HELLFIRE = 11,      // Heavy missile
    WEAPON_GRENADE = 12,       // Hand grenade
    WEAPON_75MM = 13,          // Light cannon
    WEAPON_90MM = 14,          // Medium cannon
    WEAPON_105MM = 15,         // Heavy cannon
    WEAPON_120MM = 16,         // Main battle tank gun
    WEAPON_TURRET_GUN = 17,    // Defense turret
    WEAPON_MAMMOTH_TUSK = 18,  // Mammoth tank missiles
    WEAPON_155MM = 19,         // Artillery
    WEAPON_M60MG = 20,         // Machine gun
    WEAPON_NAPALM = 21,        // Air-dropped napalm
    WEAPON_TESLA_ZAP = 22,     // Tesla coil
    WEAPON_NIKE = 23,          // SAM site
    WEAPON_8INCH = 24,         // Naval gun
    WEAPON_STINGER = 25,       // AA missile
    WEAPON_TORPEDO = 26,       // Sub torpedo
    WEAPON_2INCH = 27,         // Gunboat
    WEAPON_DEPTH_CHARGE = 28,  // Anti-sub
    WEAPON_PARABOMB = 29,      // Parachute bomb
    WEAPON_DOGJAW = 30,        // Attack dog
    WEAPON_HEAL = 31,          // Medic heal (friendly fire)
    WEAPON_SCUD = 32,          // V2 rocket
    WEAPON_FLAK = 33,          // Anti-air flak
    WEAPON_APHE = 34,          // High explosive

    WEAPON_COUNT = 35
};

// =============================================================================
// Warhead Types
// =============================================================================

/**
 * WarheadType - Damage delivery types
 *
 * Determines:
 * - Armor penetration vs different armor types
 * - Spread/splash damage
 * - Special effects (fire, EMP, etc.)
 */
enum WarheadType : int8_t {
    WARHEAD_SA = 0,        // Small arms (bullets)
    WARHEAD_HE = 1,        // High explosive
    WARHEAD_AP = 2,        // Armor piercing
    WARHEAD_FIRE = 3,      // Incendiary
    WARHEAD_HOLLOW_POINT = 4,  // Anti-infantry
    WARHEAD_TESLA = 5,     // Tesla (electric)
    WARHEAD_NUKE = 6,      // Nuclear
    WARHEAD_MECHANICAL = 7, // Crush damage

    WARHEAD_COUNT = 8
};

// =============================================================================
// Bullet/Projectile Types
// =============================================================================

/**
 * BulletType - Visual projectile types
 */
enum BulletType : int8_t {
    BULLET_INVISIBLE = 0,  // Instant hit (bullets)
    BULLET_CANNON = 1,     // Shell
    BULLET_ACK = 2,        // Anti-air tracer
    BULLET_TORPEDO = 3,    // Water torpedo
    BULLET_FROG = 4,       // Frog (unused)
    BULLET_HEATSEEKER = 5, // Guided missile
    BULLET_LASERGUIDED = 6,// Laser guided bomb
    BULLET_LOBBED = 7,     // Arcing projectile
    BULLET_BOMBLET = 8,    // Cluster bomb
    BULLET_BALLISTIC = 9,  // V2 rocket
    BULLET_PARACHUTE = 10, // Parachute bomb
    BULLET_FIREBALL = 11,  // Flame
    BULLET_DOG = 12,       // Dog bite (instant)
    BULLET_CATAPULT = 13,  // Catapult (unused)
    BULLET_AAMISSILE = 14, // AA missile

    BULLET_COUNT = 15
};

// =============================================================================
// Weapon Data Structures
// =============================================================================

/**
 * Warhead damage vs armor table entry
 */
struct WarheadVsArmor {
    int16_t versus[ARMOR_COUNT];  // Damage multiplier per armor type (%)
};

/**
 * Weapon definition structure
 */
struct WeaponData {
    const char* Name;           // Internal name
    int Damage;                 // Base damage
    int Range;                  // Range in leptons
    int ROF;                    // Rate of fire (frames between shots)
    BulletType Projectile;      // Projectile graphic
    WarheadType Warhead;        // Damage type
    int Speed;                  // Projectile speed
    int Sound;                  // Sound effect ID
    bool TwoShots;              // Fire twice per attack
    bool Anim;                  // Has muzzle flash
};

// =============================================================================
// Weapon Data Tables (defined in weapon.cpp)
// =============================================================================

extern const WeaponData WeaponTable[WEAPON_COUNT];
extern const WarheadVsArmor WarheadTable[WARHEAD_COUNT];

// =============================================================================
// Weapon Utility Functions
// =============================================================================

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Calculate damage after armor reduction
 */
int Calculate_Damage(int base_damage, WarheadType warhead, ArmorType armor);

/**
 * Get weapon name string
 */
const char* Weapon_Name(WeaponType weapon);

/**
 * Get armor name string
 */
const char* Armor_Name(ArmorType armor);

/**
 * Check if weapon can damage armor type
 */
int Weapon_Can_Damage(WeaponType weapon, ArmorType armor);

#ifdef __cplusplus
}
#endif
```

### src/game/core/coord.cpp (NEW)
```cpp
/**
 * Coordinate System Implementation
 *
 * Provides all coordinate conversion and math functions for the
 * Red Alert coordinate system.
 */

#include "game/coord.h"
#include <cmath>

// =============================================================================
// Direction Offset Tables
// =============================================================================

// X offsets for 8 directions (N, NE, E, SE, S, SW, W, NW)
const int8_t FacingOffset_X[8] = { 0,  1, 1, 1, 0, -1, -1, -1 };

// Y offsets for 8 directions (N, NE, E, SE, S, SW, W, NW)
const int8_t FacingOffset_Y[8] = { -1, -1, 0, 1, 1,  1,  0, -1 };

// Cell index offsets for 8 directions
// Format: (y_offset << 8) | x_offset
const int16_t FacingOffset_Cell[8] = {
    -MAP_CELL_WIDTH,      // N:  y-1
    -MAP_CELL_WIDTH + 1,  // NE: y-1, x+1
    1,                    // E:  x+1
    MAP_CELL_WIDTH + 1,   // SE: y+1, x+1
    MAP_CELL_WIDTH,       // S:  y+1
    MAP_CELL_WIDTH - 1,   // SW: y+1, x-1
    -1,                   // W:  x-1
    -MAP_CELL_WIDTH - 1   // NW: y-1, x-1
};

// =============================================================================
// Coordinate Conversion Functions
// =============================================================================

extern "C" {

/**
 * Convert CELL to COORDINATE (centers on cell)
 */
COORDINATE Cell_Coord(CELL cell) {
    if (cell == CELL_NONE) {
        return COORD_NONE;
    }

    int x = Cell_X(cell);
    int y = Cell_Y(cell);

    // Center of cell in leptons
    // Cell center = (cell_index * LEPTON_PER_CELL) + (LEPTON_PER_CELL / 2)
    int x_lepton = (x * LEPTON_PER_CELL) + (LEPTON_PER_CELL / 2);
    int y_lepton = (y * LEPTON_PER_CELL) + (LEPTON_PER_CELL / 2);

    return XY_Coord(x_lepton, y_lepton);
}

/**
 * Convert COORDINATE to CELL
 */
CELL Coord_Cell(COORDINATE coord) {
    if (coord == COORD_NONE) {
        return CELL_NONE;
    }

    int x_lepton = Coord_X(coord);
    int y_lepton = Coord_Y(coord);

    int x_cell = x_lepton / LEPTON_PER_CELL;
    int y_cell = y_lepton / LEPTON_PER_CELL;

    // Clamp to valid range
    if (x_cell < 0) x_cell = 0;
    if (x_cell >= MAP_CELL_WIDTH) x_cell = MAP_CELL_WIDTH - 1;
    if (y_cell < 0) y_cell = 0;
    if (y_cell >= MAP_CELL_HEIGHT) y_cell = MAP_CELL_HEIGHT - 1;

    return XY_Cell(x_cell, y_cell);
}

/**
 * Get distance between two coordinates (in leptons)
 * Uses fast integer approximation
 */
int Coord_Distance(COORDINATE coord1, COORDINATE coord2) {
    if (coord1 == COORD_NONE || coord2 == COORD_NONE) {
        return 0x7FFFFFFF; // Max distance for invalid coords
    }

    int dx = Coord_X(coord2) - Coord_X(coord1);
    int dy = Coord_Y(coord2) - Coord_Y(coord1);

    // Fast distance approximation (octagonal)
    // More accurate than max(dx,dy), faster than sqrt
    int adx = (dx < 0) ? -dx : dx;
    int ady = (dy < 0) ? -dy : dy;

    if (adx > ady) {
        return adx + (ady >> 1);
    } else {
        return ady + (adx >> 1);
    }
}

/**
 * Calculate direction from one coordinate to another
 * Returns 0-255 direction value
 */
uint8_t Coord_Direction(COORDINATE from, COORDINATE to) {
    if (from == COORD_NONE || to == COORD_NONE) {
        return 0;
    }

    int dx = Coord_X(to) - Coord_X(from);
    int dy = Coord_Y(to) - Coord_Y(from);

    // atan2 returns -PI to PI, we need 0-255
    // North (up) = 0, East = 64, South = 128, West = 192
    double angle = atan2((double)dx, (double)-dy);

    // Convert to 0-255 range
    int dir = (int)((angle / 3.14159265358979) * 128.0);
    if (dir < 0) dir += 256;

    return (uint8_t)dir;
}

/**
 * Move coordinate by distance in given direction
 */
COORDINATE Coord_Move(COORDINATE coord, uint8_t direction, int distance) {
    if (coord == COORD_NONE || distance == 0) {
        return coord;
    }

    // Direction to radians (0 = north/up = -Y)
    double angle = (direction / 128.0) * 3.14159265358979;

    int dx = (int)(sin(angle) * distance);
    int dy = (int)(-cos(angle) * distance);

    int x = Coord_X(coord) + dx;
    int y = Coord_Y(coord) + dy;

    // Clamp to valid range
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= MAP_LEPTON_WIDTH) x = MAP_LEPTON_WIDTH - 1;
    if (y >= MAP_LEPTON_HEIGHT) y = MAP_LEPTON_HEIGHT - 1;

    return XY_Coord(x, y);
}

/**
 * Check if coordinate is within map bounds
 */
int Coord_InMap(COORDINATE coord) {
    if (coord == COORD_NONE) return 0;

    int x = Coord_X(coord);
    int y = Coord_Y(coord);

    return (x >= 0 && x < MAP_LEPTON_WIDTH &&
            y >= 0 && y < MAP_LEPTON_HEIGHT);
}

/**
 * Check if cell is within map bounds
 */
int Cell_InMap(CELL cell) {
    if (cell == CELL_NONE) return 0;

    int x = Cell_X(cell);
    int y = Cell_Y(cell);

    return (x >= 0 && x < MAP_CELL_WIDTH &&
            y >= 0 && y < MAP_CELL_HEIGHT);
}

/**
 * Get adjacent cell in given direction
 */
CELL Adjacent_Cell(CELL cell, int direction) {
    if (cell == CELL_NONE || direction < 0 || direction >= FACING_COUNT) {
        return CELL_NONE;
    }

    int x = Cell_X(cell) + FacingOffset_X[direction];
    int y = Cell_Y(cell) + FacingOffset_Y[direction];

    if (x < 0 || x >= MAP_CELL_WIDTH || y < 0 || y >= MAP_CELL_HEIGHT) {
        return CELL_NONE;
    }

    return XY_Cell(x, y);
}

/**
 * Snap coordinate to cell center
 */
COORDINATE Coord_Snap(COORDINATE coord) {
    return Cell_Coord(Coord_Cell(coord));
}

/**
 * Convert pixel coordinates to world coordinate
 */
COORDINATE Pixel_To_Coord(int pixel_x, int pixel_y) {
    int x_lepton = pixel_x * LEPTON_PER_PIXEL;
    int y_lepton = pixel_y * LEPTON_PER_PIXEL;

    return XY_Coord(x_lepton, y_lepton);
}

/**
 * Convert world coordinate to pixel coordinates
 */
void Coord_To_Pixel(COORDINATE coord, int* pixel_x, int* pixel_y) {
    if (pixel_x) *pixel_x = Coord_XPixel(coord);
    if (pixel_y) *pixel_y = Coord_YPixel(coord);
}

/**
 * Calculate cell distance (Chebyshev - max of x,y distance)
 */
int Cell_Distance(CELL cell1, CELL cell2) {
    if (cell1 == CELL_NONE || cell2 == CELL_NONE) {
        return 0x7FFF;
    }

    int dx = Cell_X(cell2) - Cell_X(cell1);
    int dy = Cell_Y(cell2) - Cell_Y(cell1);

    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;

    return (dx > dy) ? dx : dy;
}

} // extern "C"
```

## Verification Command
```bash
ralph-tasks/verify/check-coord-types.sh
```

## Verification Script

### ralph-tasks/verify/check-coord-types.sh (NEW)
```bash
#!/bin/bash
# Verification script for coordinate type definitions

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Coordinate Type Definitions ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check header files exist
echo "[1/5] Checking header files..."
check_file() {
    if [ ! -f "$1" ]; then
        echo "VERIFY_FAILED: $1 not found"
        exit 1
    fi
    echo "  OK: $1"
}

check_file "include/game/coord.h"
check_file "include/game/facing.h"
check_file "include/game/house.h"
check_file "include/game/weapon.h"
check_file "src/game/core/coord.cpp"

# Step 2: Syntax check headers
echo "[2/5] Checking header syntax..."
for header in include/game/coord.h include/game/facing.h include/game/house.h include/game/weapon.h; do
    if ! clang++ -std=c++17 -fsyntax-only -I include "$header" 2>&1; then
        echo "VERIFY_FAILED: $header has syntax errors"
        exit 1
    fi
done
echo "  OK: All headers valid"

# Step 3: Compile implementation
echo "[3/5] Compiling coord.cpp..."
if ! clang++ -std=c++17 -c -I include src/game/core/coord.cpp -o /tmp/coord.o 2>&1; then
    echo "VERIFY_FAILED: coord.cpp compilation failed"
    exit 1
fi
echo "  OK: coord.cpp compiles"

# Step 4: Build test program
echo "[4/5] Building coordinate test..."
cat > /tmp/test_coord.cpp << 'EOF'
#include "game/coord.h"
#include "game/facing.h"
#include "game/house.h"
#include <cstdio>
#include <cstring>

#define TEST(name, cond) do { \
    if (!(cond)) { printf("FAIL: %s\n", name); return 1; } \
    printf("PASS: %s\n", name); \
} while(0)

int main() {
    printf("=== Coordinate Type Tests ===\n\n");

    // Test cell construction and extraction
    CELL cell = XY_Cell(50, 75);
    TEST("Cell X extraction", Cell_X(cell) == 50);
    TEST("Cell Y extraction", Cell_Y(cell) == 75);

    // Test cell to coord conversion
    COORDINATE coord = Cell_Coord(cell);
    TEST("Cell to Coord not null", coord != COORD_NONE);

    // Test coord back to cell
    CELL back = Coord_Cell(coord);
    TEST("Coord to Cell X", Cell_X(back) == 50);
    TEST("Coord to Cell Y", Cell_Y(back) == 75);

    // Test coordinate macros
    int px = Coord_XPixel(coord);
    int py = Coord_YPixel(coord);
    TEST("Coord X pixel in range", px > 0 && px < MAP_PIXEL_WIDTH);
    TEST("Coord Y pixel in range", py > 0 && py < MAP_PIXEL_HEIGHT);

    // Test boundary checks
    TEST("Cell in map - valid", Cell_InMap(XY_Cell(64, 64)) == 1);
    TEST("Cell in map - invalid", Cell_InMap(CELL_NONE) == 0);

    // Test adjacent cell
    CELL center = XY_Cell(64, 64);
    CELL north = Adjacent_Cell(center, FACING_N);
    TEST("Adjacent north Y", Cell_Y(north) == 63);
    TEST("Adjacent north X", Cell_X(north) == 64);

    // Test distance
    CELL c1 = XY_Cell(10, 10);
    CELL c2 = XY_Cell(20, 10);
    int dist = Cell_Distance(c1, c2);
    TEST("Cell distance", dist == 10);

    // Test pixel conversion
    COORDINATE pcoord = Pixel_To_Coord(100, 200);
    int rx, ry;
    Coord_To_Pixel(pcoord, &rx, &ry);
    TEST("Pixel roundtrip X", rx == 100);
    TEST("Pixel roundtrip Y", ry == 200);

    printf("\n=== All coordinate tests passed! ===\n");
    return 0;
}
EOF

if ! clang++ -std=c++17 -I include /tmp/test_coord.cpp src/game/core/coord.cpp -o /tmp/test_coord 2>&1; then
    echo "VERIFY_FAILED: Test program compilation failed"
    rm -f /tmp/coord.o /tmp/test_coord.cpp
    exit 1
fi
echo "  OK: Test program built"

# Step 5: Run tests
echo "[5/5] Running coordinate tests..."
if ! /tmp/test_coord; then
    echo "VERIFY_FAILED: Coordinate tests failed"
    rm -f /tmp/coord.o /tmp/test_coord.cpp /tmp/test_coord
    exit 1
fi

# Cleanup
rm -f /tmp/coord.o /tmp/test_coord.cpp /tmp/test_coord

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Create directories: `mkdir -p include/game src/game/core`
2. Create header files with coordinate types
3. Create implementation file coord.cpp
4. Make verification script executable
5. Run verification script

## Success Criteria
- All header files compile without errors
- Coordinate macros work correctly (Cell_X, Coord_XPixel, etc.)
- Cell <-> Coordinate conversion is correct
- Distance and direction calculations work
- Boundary checking functions correctly
- Adjacent cell navigation works

## Common Issues

### "MAP_CELL_WIDTH not defined"
- Check include order - coord.h should be first

### Distance calculation overflow
- Use long integers for intermediate values
- Check for COORD_NONE before calculation

### Direction calculation wrong
- Remember: North = 0, not 90 degrees
- Y axis is inverted (positive = down)

## Completion Promise
When verification passes, output:
```
<promise>TASK_14B_COMPLETE</promise>
```

## Escape Hatch
If stuck after 12 iterations:
- Document what's blocking in `ralph-tasks/blocked/14b.md`
- Output: `<promise>TASK_14B_BLOCKED</promise>`

## Max Iterations
12
