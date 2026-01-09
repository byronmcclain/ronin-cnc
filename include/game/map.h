/**
 * MapClass - Map Data Management
 *
 * Manages the terrain map data including all cells, map bounds,
 * and basic cell access operations.
 *
 * Original location: CODE/MAP.H, CODE/MAP.CPP
 *
 * Class Hierarchy:
 *   GScreenClass
 *       |
 *       +-- MapClass        <- This class
 *           |
 *           +-- DisplayClass
 */

#pragma once

#include "game/gscreen.h"
#include "game/coord.h"
#include <cstdint>

// Forward declaration
class CellClass;

// =============================================================================
// Map Constants
// =============================================================================

// Theater types (terrain tileset themes)
enum TheaterType : int8_t {
    THEATER_NONE = -1,
    THEATER_TEMPERATE = 0,   // Green grass, trees
    THEATER_SNOW = 1,        // Winter terrain
    THEATER_INTERIOR = 2,    // Indoor missions

    THEATER_COUNT = 3
};

// Theater name strings
extern const char* TheaterNames[THEATER_COUNT];
extern const char* TheaterFileSuffix[THEATER_COUNT];

// =============================================================================
// MapClass
// =============================================================================

/**
 * MapClass - Base map data management
 *
 * Inherits from GScreenClass and adds:
 * - Cell array storage
 * - Map bounds (scenario play area)
 * - Cell access methods
 * - Coordinate validation
 */
class MapClass : public GScreenClass {
public:
    // -------------------------------------------------------------------------
    // Construction / Destruction
    // -------------------------------------------------------------------------

    MapClass();
    virtual ~MapClass();

    // -------------------------------------------------------------------------
    // Lifecycle Methods (override from GScreenClass)
    // -------------------------------------------------------------------------

    /**
     * One_Time - Called once at startup
     *
     * Initializes cell array and map data.
     */
    virtual void One_Time() override;

    /**
     * Init - Called at scenario start
     *
     * Resets map to initial state and loads theater.
     *
     * @param theater Theater type (TEMPERATE, SNOW, etc.)
     */
    virtual void Init(int theater = THEATER_TEMPERATE) override;

    // -------------------------------------------------------------------------
    // Cell Access
    // -------------------------------------------------------------------------

    /**
     * operator[] - Access cell by CELL index
     *
     * @param cell Cell index (XY_Cell format)
     * @return Reference to cell (or dummy cell if out of bounds)
     */
    CellClass& operator[](CELL cell);
    const CellClass& operator[](CELL cell) const;

    /**
     * Cell_At - Access cell by X,Y coordinates
     *
     * @param x Cell X (0-127)
     * @param y Cell Y (0-127)
     * @return Pointer to cell (or nullptr if invalid)
     */
    CellClass* Cell_At(int x, int y);
    const CellClass* Cell_At(int x, int y) const;

    /**
     * Get_Cell_At_Coord - Access cell by world coordinate
     *
     * @param coord World coordinate
     * @return Pointer to cell containing coordinate
     */
    CellClass* Get_Cell_At_Coord(COORDINATE coord);

    /**
     * Is_Valid_Cell - Check if cell index is valid
     */
    bool Is_Valid_Cell(CELL cell) const;

    /**
     * Is_Valid_XY - Check if X,Y is valid cell coordinate
     */
    bool Is_Valid_XY(int x, int y) const;

    // -------------------------------------------------------------------------
    // Map Bounds (Scenario Play Area)
    // -------------------------------------------------------------------------

    /**
     * Set_Map_Bounds - Set the playable area
     *
     * Scenarios don't use the full 128x128 map. This sets
     * the bounds of the actual play area.
     *
     * @param x Left edge cell X
     * @param y Top edge cell Y
     * @param w Width in cells
     * @param h Height in cells
     */
    void Set_Map_Bounds(int x, int y, int w, int h);

    /**
     * Get map bounds
     */
    int Map_Bounds_X() const { return map_x_; }
    int Map_Bounds_Y() const { return map_y_; }
    int Map_Bounds_Width() const { return map_width_; }
    int Map_Bounds_Height() const { return map_height_; }

    /**
     * Get map bounds as cell range
     */
    CELL Map_First_Cell() const { return XY_Cell(map_x_, map_y_); }
    CELL Map_Last_Cell() const { return XY_Cell(map_x_ + map_width_ - 1, map_y_ + map_height_ - 1); }

    /**
     * Is_In_Bounds - Check if cell is within scenario bounds
     */
    bool Is_In_Bounds(CELL cell) const;
    bool Is_In_Bounds(int x, int y) const;

    /**
     * Clamp_To_Bounds - Clamp coordinates to map bounds
     */
    void Clamp_To_Bounds(int& x, int& y) const;
    CELL Clamp_To_Bounds(CELL cell) const;

    // -------------------------------------------------------------------------
    // Map Operations
    // -------------------------------------------------------------------------

    /**
     * Clear_Map - Reset all cells to default state
     */
    void Clear_Map();

    /**
     * Recalc_All - Recalculate all cell data
     *
     * Called after map changes to update passability,
     * visibility, etc.
     */
    void Recalc_All();

    /**
     * Get_Theater_Type - Get current theater type
     */
    TheaterType Get_Theater_Type() const { return theater_type_; }

    // -------------------------------------------------------------------------
    // Coordinate Utilities
    // -------------------------------------------------------------------------

    /**
     * Pick_Cell - Convert screen pixel to cell
     *
     * @param screen_x Screen X coordinate
     * @param screen_y Screen Y coordinate
     * @return Cell under screen position (accounting for scroll)
     */
    CELL Pick_Cell(int screen_x, int screen_y) const;

    /**
     * Cell_To_Screen - Convert cell to screen pixel
     *
     * @param cell Cell index
     * @param[out] screen_x Screen X coordinate
     * @param[out] screen_y Screen Y coordinate
     * @return true if cell is visible on screen
     */
    bool Cell_To_Screen(CELL cell, int& screen_x, int& screen_y) const;

    // -------------------------------------------------------------------------
    // Scroll Position (used by DisplayClass)
    // -------------------------------------------------------------------------

    /**
     * Get/set tactical view scroll position (in leptons)
     */
    COORDINATE Get_Tactical_Position() const { return tactical_pos_; }
    void Set_Tactical_Position(COORDINATE pos);

    /**
     * Get scroll position as cell
     */
    CELL Get_Tactical_Cell() const { return ::Coord_Cell(tactical_pos_); }

protected:
    // -------------------------------------------------------------------------
    // Protected Members
    // -------------------------------------------------------------------------

    // Cell storage
    CellClass* cells_;          // Array of cells [MAP_CELL_TOTAL]
    bool cells_owned_;          // Did we allocate cells_?

    // Map bounds (scenario play area)
    int map_x_;                 // Left edge cell X
    int map_y_;                 // Top edge cell Y
    int map_width_;             // Width in cells
    int map_height_;            // Height in cells

    // Theater
    TheaterType theater_type_;  // Current terrain theme

    // Tactical view position
    COORDINATE tactical_pos_;   // Top-left of viewport in leptons

    // Dummy cell for out-of-bounds access
    static CellClass dummy_cell_;
};

// =============================================================================
// Global Map Pointer
// =============================================================================

/**
 * Map - Global map instance
 *
 * Most game code accesses the map through this global.
 */
extern MapClass* Map;
