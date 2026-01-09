/**
 * CellClass - Map Cell Data
 *
 * Represents a single 24x24 pixel terrain cell. Stores terrain type,
 * overlays, visibility, and occupancy information.
 *
 * Original location: CODE/CELL.H, CODE/CELL.CPP
 */

#pragma once

#include "game/coord.h"
#include <cstdint>

// Forward declarations
class ObjectClass;
class TechnoClass;

// =============================================================================
// Cell Constants
// =============================================================================

// Maximum objects that can occupy a single cell
constexpr int CELL_MAX_OBJECTS = 4;

// Special template values
constexpr uint8_t TEMPLATE_NONE = 0xFF;
constexpr uint8_t TEMPLATE_CLEAR = 0;

// Special overlay values
constexpr uint8_t OVERLAY_NONE = 0xFF;

// =============================================================================
// Land Types
// =============================================================================

/**
 * LandType - Terrain classification affecting movement and passability
 */
enum LandType : int8_t {
    LAND_CLEAR = 0,      // Open ground - all units
    LAND_ROAD = 1,       // Road - speed bonus
    LAND_WATER = 2,      // Water - naval only
    LAND_ROCK = 3,       // Impassable rock
    LAND_WALL = 4,       // Wall structure
    LAND_TIBERIUM = 5,   // Tiberium field
    LAND_BEACH = 6,      // Beach transition
    LAND_ROUGH = 7,      // Rough terrain - slow
    LAND_RIVER = 8,      // River (impassable)

    LAND_COUNT = 9
};

// Speed multipliers per land type (percentage)
extern const int LandSpeedMultiplier[LAND_COUNT];

// =============================================================================
// Overlay Types
// =============================================================================

/**
 * OverlayType - Objects placed on top of terrain
 */
enum OverlayType : int8_t {
    OVERLAY_NONE_TYPE = -1,

    // Resources
    OVERLAY_GOLD1 = 0,      // Gold/Ore stage 1
    OVERLAY_GOLD2 = 1,
    OVERLAY_GOLD3 = 2,
    OVERLAY_GOLD4 = 3,      // Full gold
    OVERLAY_GEMS1 = 4,      // Gems stage 1
    OVERLAY_GEMS2 = 5,
    OVERLAY_GEMS3 = 6,
    OVERLAY_GEMS4 = 7,      // Full gems

    // Walls
    OVERLAY_SANDBAG = 8,    // Sandbag wall
    OVERLAY_CYCLONE = 9,    // Chain link fence
    OVERLAY_BRICK = 10,     // Concrete wall
    OVERLAY_BARBWIRE = 11,  // Barbed wire
    OVERLAY_WOOD = 12,      // Wooden fence

    // Decorative
    OVERLAY_CRATE = 13,     // Bonus crate
    OVERLAY_V12 = 14,       // Haystacks
    OVERLAY_V13 = 15,       // Hay bales

    OVERLAY_COUNT = 16
};

// =============================================================================
// Visibility State
// =============================================================================

/**
 * Cell visibility flags (per-player in multiplayer)
 */
enum CellVisibility : uint8_t {
    CELL_SHROUD = 0,        // Never seen - black
    CELL_EXPLORED = 1,      // Seen before - fog
    CELL_VISIBLE = 2        // Currently visible - clear
};

// =============================================================================
// CellClass
// =============================================================================

/**
 * CellClass - Single map cell
 *
 * Stores all data for one 24x24 pixel terrain cell including:
 * - Terrain template and icon
 * - Overlay (resources, walls)
 * - Land type (derived from terrain)
 * - Visibility state
 * - Occupying objects
 */
class CellClass {
public:
    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------

    CellClass();
    ~CellClass() = default;

    // -------------------------------------------------------------------------
    // Initialization
    // -------------------------------------------------------------------------

    /**
     * Clear - Reset cell to default state
     */
    void Clear();

    /**
     * Set_Cell_Index - Set this cell's position
     */
    void Set_Cell_Index(CELL cell) { cell_index_ = cell; }

    /**
     * Get_Cell_Index - Get this cell's position
     */
    CELL Get_Cell_Index() const { return cell_index_; }

    /**
     * Get_Coord - Get center coordinate of this cell
     */
    COORDINATE Get_Coord() const { return Cell_Coord(cell_index_); }

    /**
     * Get_X/Get_Y - Get cell coordinates
     */
    int Get_X() const { return (int)((cell_index_) & 0xFF); }
    int Get_Y() const { return (int)(((cell_index_) >> 8) & 0xFF); }

    // -------------------------------------------------------------------------
    // Terrain
    // -------------------------------------------------------------------------

    /**
     * Set terrain template and icon
     *
     * @param template_type Template index (from TEMPERAT.MIX, etc.)
     * @param icon Icon within template
     */
    void Set_Template(uint8_t template_type, uint8_t icon = 0);

    uint8_t Get_Template() const { return template_type_; }
    uint8_t Get_Icon() const { return template_icon_; }

    /**
     * Has valid terrain template?
     */
    bool Has_Template() const { return template_type_ != TEMPLATE_NONE; }

    // -------------------------------------------------------------------------
    // Overlay
    // -------------------------------------------------------------------------

    /**
     * Set overlay type and data
     *
     * @param type Overlay type (OVERLAY_GOLD1, etc.)
     * @param data Overlay-specific data (wall health, tiberium stage)
     */
    void Set_Overlay(OverlayType type, uint8_t data = 0);

    OverlayType Get_Overlay() const { return overlay_type_; }
    uint8_t Get_Overlay_Data() const { return overlay_data_; }

    /**
     * Has overlay?
     */
    bool Has_Overlay() const { return overlay_type_ != OVERLAY_NONE_TYPE; }

    /**
     * Is tiberium?
     */
    bool Is_Tiberium() const;

    /**
     * Is wall?
     */
    bool Is_Wall() const;

    /**
     * Get tiberium value (for harvesting)
     */
    int Get_Tiberium_Value() const;

    /**
     * Reduce tiberium (after harvesting)
     */
    void Reduce_Tiberium(int amount);

    // -------------------------------------------------------------------------
    // Land Type
    // -------------------------------------------------------------------------

    /**
     * Get land type (derived from terrain/overlay)
     */
    LandType Get_Land() const { return land_type_; }

    /**
     * Recalculate land type from terrain and overlay
     */
    void Recalc_Land();

    /**
     * Can unit type pass this cell?
     *
     * @param is_naval True for ships/subs
     * @param is_infantry True for infantry (can walk more places)
     */
    bool Is_Passable(bool is_naval = false, bool is_infantry = false) const;

    /**
     * Get movement speed multiplier (100 = normal)
     */
    int Get_Speed_Multiplier() const;

    // -------------------------------------------------------------------------
    // Visibility
    // -------------------------------------------------------------------------

    /**
     * Get/set visibility state
     */
    CellVisibility Get_Visibility() const { return visibility_; }
    void Set_Visibility(CellVisibility vis) { visibility_ = vis; }

    bool Is_Shrouded() const { return visibility_ == CELL_SHROUD; }
    bool Is_Explored() const { return visibility_ >= CELL_EXPLORED; }
    bool Is_Visible() const { return visibility_ == CELL_VISIBLE; }

    /**
     * Reveal this cell (mark as explored or visible)
     */
    void Reveal(bool make_visible = true);

    /**
     * Shroud this cell (return to fog)
     */
    void Shroud();

    // -------------------------------------------------------------------------
    // Occupancy
    // -------------------------------------------------------------------------

    /**
     * Add object to this cell
     *
     * @return true if added, false if cell full
     */
    bool Add_Object(ObjectClass* obj);

    /**
     * Remove object from this cell
     */
    void Remove_Object(ObjectClass* obj);

    /**
     * Get object at index (0-3)
     */
    ObjectClass* Get_Object(int index) const;

    /**
     * Get first object in cell
     */
    ObjectClass* Get_First_Object() const { return Get_Object(0); }

    /**
     * Count objects in cell
     */
    int Object_Count() const;

    /**
     * Is cell occupied?
     */
    bool Is_Occupied() const { return objects_[0] != nullptr; }

    /**
     * Find techno (unit/building) in cell
     */
    TechnoClass* Find_Techno() const;

    // -------------------------------------------------------------------------
    // Flags
    // -------------------------------------------------------------------------

    /**
     * Various cell flags
     */
    bool Is_Bridge() const { return is_bridge_; }
    void Set_Bridge(bool val) { is_bridge_ = val; }

    bool Is_Waypoint() const { return is_waypoint_; }
    void Set_Waypoint(bool val) { is_waypoint_ = val; }

    bool Is_Flag() const { return is_flag_; }
    void Set_Flag(bool val) { is_flag_ = val; }

    // -------------------------------------------------------------------------
    // Recalculation
    // -------------------------------------------------------------------------

    /**
     * Recalc - Recalculate derived values
     *
     * Call after changing terrain or overlay to update land type.
     */
    void Recalc();

private:
    // -------------------------------------------------------------------------
    // Data Members
    // -------------------------------------------------------------------------

    // Position
    CELL cell_index_;           // This cell's position (y*128+x)

    // Terrain
    uint8_t template_type_;     // Terrain template index
    uint8_t template_icon_;     // Icon within template

    // Overlay
    OverlayType overlay_type_;  // Overlay type
    uint8_t overlay_data_;      // Overlay-specific data

    // Derived
    LandType land_type_;        // Calculated land type

    // Visibility
    CellVisibility visibility_; // Fog of war state

    // Flags
    bool is_bridge_;            // Bridge overlay
    bool is_waypoint_;          // Waypoint marker
    bool is_flag_;              // Capture the flag

    // Occupancy
    ObjectClass* objects_[CELL_MAX_OBJECTS]; // Objects in this cell
};

// =============================================================================
// Land Type Names
// =============================================================================

extern const char* LandTypeNames[LAND_COUNT];
