// include/game/input/cursor_context.h
// Cursor context types for mouse interaction
// Task 16d - Mouse Handler

#ifndef CURSOR_CONTEXT_H
#define CURSOR_CONTEXT_H

#include <cstdint>

//=============================================================================
// Screen Region
//=============================================================================

enum class ScreenRegion {
    TACTICAL,       // Main game view
    SIDEBAR,        // Right sidebar
    RADAR,          // Radar minimap
    TAB_BAR,        // Top tab bar
    CAMEO_AREA,     // Build cameos in sidebar
    POWER_BAR,      // Power indicator
    CREDITS_AREA,   // Credits display
    OUTSIDE         // Outside game window
};

//=============================================================================
// Cursor Context Type
//=============================================================================

enum class CursorContextType {
    // Basic states
    NORMAL,             // Default arrow cursor

    // Over terrain
    TERRAIN_PASSABLE,   // Can move here
    TERRAIN_BLOCKED,    // Cannot move here
    TERRAIN_WATER,      // Water terrain
    TERRAIN_SHROUD,     // Shrouded area

    // Over own units/buildings
    OWN_UNIT,           // Friendly unit (selectable)
    OWN_BUILDING,       // Friendly building
    OWN_HARVESTER,      // Harvester (special actions)
    OWN_TRANSPORT,      // Transport unit

    // Over enemy units/buildings
    ENEMY_UNIT,         // Enemy unit (attackable)
    ENEMY_BUILDING,     // Enemy building

    // Over neutral/civilian
    NEUTRAL_BUILDING,   // Neutral structure
    CIVILIAN,           // Civilian unit

    // Special contexts
    REPAIR_TARGET,      // Valid repair target
    SELL_TARGET,        // Valid sell target
    ENTER_TARGET,       // Can enter this
    HARVEST_AREA,       // Ore/gems to harvest

    // UI contexts
    UI_BUTTON,          // Over UI button
    UI_CAMEO,           // Over build cameo
    UI_RADAR,           // Over radar

    // Action contexts
    PLACEMENT_VALID,    // Valid building placement
    PLACEMENT_INVALID,  // Invalid placement

    // Edge scroll
    SCROLL_N,
    SCROLL_NE,
    SCROLL_E,
    SCROLL_SE,
    SCROLL_S,
    SCROLL_SW,
    SCROLL_W,
    SCROLL_NW
};

//=============================================================================
// Cursor Context
//=============================================================================

struct CursorContext {
    CursorContextType type;
    ScreenRegion region;

    // World position
    int world_x;
    int world_y;
    int cell_x;
    int cell_y;

    // Object under cursor (if any)
    void* object;           // ObjectClass* when integrated
    uint32_t object_id;     // For reference
    bool is_own;            // Belongs to player
    bool is_enemy;          // Belongs to enemy
    bool is_selectable;     // Can be selected
    bool is_attackable;     // Can be attacked

    // Terrain info
    bool is_passable;
    bool is_buildable;
    bool is_visible;        // Not shrouded

    // UI element (if any)
    int ui_element_id;

    void Clear();
};

//=============================================================================
// Cursor Shape (for rendering)
//=============================================================================

enum class CursorShape {
    ARROW,          // Default pointer
    SELECT,         // Selection hand
    MOVE,           // Move order
    ATTACK,         // Attack target
    NO_MOVE,        // Can't move there
    NO_ATTACK,      // Can't attack
    ENTER,          // Enter transport/building
    DEPLOY,         // Deploy action
    SELL,           // Sell mode
    REPAIR,         // Repair mode
    NO_SELL,        // Can't sell here
    NO_REPAIR,      // Can't repair this
    HARVEST,        // Harvest ore
    SCROLL_N,       // Edge scroll cursors
    SCROLL_NE,
    SCROLL_E,
    SCROLL_SE,
    SCROLL_S,
    SCROLL_SW,
    SCROLL_W,
    SCROLL_NW,
    BEACON,         // Place beacon
    AIR_STRIKE,     // Air strike target
    NUKE,           // Nuke target
    CHRONO,         // Chronosphere target
    CUSTOM          // Custom cursor
};

// Map cursor context to appropriate cursor shape
CursorShape GetCursorShapeForContext(const CursorContext& context, bool has_selection);

#endif // CURSOR_CONTEXT_H
