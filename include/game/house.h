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
