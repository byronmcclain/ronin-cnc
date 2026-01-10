/**
 * Standard Color Remap Tables
 *
 * Pre-defined remap tables for house colors, shadows, and effects.
 */

#ifndef GAME_GRAPHICS_REMAP_TABLES_H
#define GAME_GRAPHICS_REMAP_TABLES_H

#include <cstdint>

// =============================================================================
// House Color Remapping
// =============================================================================

// House color indices in the standard palette
// Original palette has these color ranges for house tinting:
//   80-95:  Primary house color (vehicles, main building color)
//   176-191: Secondary house color (trim, accents)

enum HouseColorIndex {
    HOUSE_COLOR_GOLD = 0,      // Yellow/Gold (Spain)
    HOUSE_COLOR_LTBLUE = 1,    // Light Blue (Greece)
    HOUSE_COLOR_RED = 2,       // Red (USSR)
    HOUSE_COLOR_GREEN = 3,     // Green (England)
    HOUSE_COLOR_ORANGE = 4,    // Orange (Ukraine)
    HOUSE_COLOR_GREY = 5,      // Grey (Germany)
    HOUSE_COLOR_BLUE = 6,      // Blue (France)
    HOUSE_COLOR_BROWN = 7,     // Brown (Turkey)
    HOUSE_COLOR_COUNT = 8
};

/**
 * Get the remap table for a house color
 *
 * @param house House color index (0-7)
 * @return Pointer to 256-byte remap table
 */
const uint8_t* GetHouseRemapTable(int house);

/**
 * Get the remap table for a player index
 *
 * Maps player indices to house colors:
 * - Single player: Player 0 = Good (allied), Player 1 = Bad (soviet)
 * - Multiplayer: Players 0-7 map to house colors
 *
 * @param player Player index
 * @param is_multiplayer True for multiplayer games
 * @return Pointer to 256-byte remap table
 */
const uint8_t* GetPlayerRemapTable(int player, bool is_multiplayer);

// =============================================================================
// Shadow Tables
// =============================================================================

/**
 * Get the standard shadow remap table
 *
 * Maps each palette color to a darker version for shadow effects.
 *
 * @return Pointer to 256-byte shadow table
 */
const uint8_t* GetShadowTable();

/**
 * Generate a shadow table from a palette
 *
 * @param palette    RGB palette data (768 bytes)
 * @param output     Output table (256 bytes)
 * @param intensity  Shadow darkness (0.0 = none, 1.0 = black)
 */
void GenerateShadowTable(const uint8_t* palette, uint8_t* output, float intensity);

// =============================================================================
// Fade Tables
// =============================================================================

/**
 * Get the fade table array
 *
 * Returns pointer to 16 remap tables (4096 bytes total).
 * Index by: table[fade_level * 256 + color]
 *
 * Fade levels:
 *   0  = No fade (identity)
 *   15 = Maximum fade (all black)
 *
 * @return Pointer to 4096-byte fade table array
 */
const uint8_t* GetFadeTables();

/**
 * Generate fade tables from a palette
 *
 * @param palette RGB palette data (768 bytes)
 * @param output  Output tables (4096 bytes - 16 tables of 256 entries)
 */
void GenerateFadeTables(const uint8_t* palette, uint8_t* output);

// =============================================================================
// Ghost Tables
// =============================================================================

/**
 * Get the ghost (semi-transparent) remap table
 *
 * Maps colors to lighter versions for ghost effects.
 *
 * @return Pointer to 256-byte ghost table
 */
const uint8_t* GetGhostTable();

// =============================================================================
// Identity Table
// =============================================================================

/**
 * Get the identity remap table (no change)
 *
 * Useful as a default when no remapping is needed.
 *
 * @return Pointer to 256-byte identity table
 */
const uint8_t* GetIdentityTable();

// =============================================================================
// Table Initialization
// =============================================================================

/**
 * Initialize all remap tables
 *
 * Must be called after palette is loaded. Generates house colors,
 * shadows, fades, etc. based on the current palette.
 *
 * @param palette RGB palette data (768 bytes)
 */
void InitRemapTables(const uint8_t* palette);

/**
 * Check if remap tables are initialized
 */
bool AreRemapTablesInitialized();

#endif // GAME_GRAPHICS_REMAP_TABLES_H
