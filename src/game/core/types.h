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

// Forward reference to game.h constants (defined there to avoid circular deps)
#ifndef MAP_MAX_WIDTH
#define MAP_MAX_WIDTH 128
#endif
#ifndef MAP_MAX_HEIGHT
#define MAP_MAX_HEIGHT 128
#endif
#ifndef CELL_LEPTON_W
#define CELL_LEPTON_W 256
#endif
#ifndef CELL_LEPTON_H
#define CELL_LEPTON_H 256
#endif

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
