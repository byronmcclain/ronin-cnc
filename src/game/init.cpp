/**
 * Game Initialization
 *
 * Handles one-time game initialization and scenario loading.
 */

#include "game/game.h"
#include "game/types/unittype.h"
#include "game/types/buildingtype.h"
#include "platform.h"
#include <cstdio>

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
            char msg[64];
            snprintf(msg, sizeof(msg), "Unit type %d not initialized", i);
            Platform_LogError(msg);
            return false;
        }
    }

    char msg1[64];
    snprintf(msg1, sizeof(msg1), "  %d unit types loaded", UNIT_COUNT);
    Platform_LogInfo(msg1);

    char msg2[64];
    snprintf(msg2, sizeof(msg2), "  %d building types loaded", BUILDING_COUNT);
    Platform_LogInfo(msg2);

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

    char msg[64];
    snprintf(msg, sizeof(msg), "Loading palette: %s", palette_name);
    Platform_LogInfo(msg);

    // Placeholder - actual palette loading would go through asset system
    // For now, set a default palette using the platform API
    // Set some recognizable colors

    // Black
    Platform_Graphics_SetPaletteEntry(0, 0, 0, 0);

    // White
    Platform_Graphics_SetPaletteEntry(15, 255, 255, 255);

    // Green (terrain)
    Platform_Graphics_SetPaletteEntry(141, 50, 120, 50);

    // Blue (water)
    Platform_Graphics_SetPaletteEntry(154, 30, 60, 150);

    // Grey (road)
    Platform_Graphics_SetPaletteEntry(176, 128, 128, 128);

    // Red
    Platform_Graphics_SetPaletteEntry(123, 200, 50, 50);

    // Green (selection)
    Platform_Graphics_SetPaletteEntry(120, 50, 200, 50);

    // Set up some grayscale ramp in palette
    for (int i = 1; i < 16; i++) {
        uint8_t v = (uint8_t)(i * 16);
        Platform_Graphics_SetPaletteEntry((uint8_t)i, v, v, v);
    }

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
