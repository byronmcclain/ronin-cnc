/**
 * ObjectClass - Game Object Base
 *
 * Base class for all visible/interactive game objects.
 * Provides position, health, selection, and rendering interface.
 *
 * Original location: CODE/OBJECT.H, CODE/OBJECT.CPP
 */

#pragma once

#include "game/abstract.h"
#include "game/coord.h"
#include "game/house.h"
#include <cstdint>

// Forward declarations
class CellClass;
class DisplayClass;

// =============================================================================
// Object Constants
// =============================================================================

// Maximum health value
constexpr int MAX_HEALTH = 0x7FFF;

// Selection box sizes
constexpr int SELECT_BOX_SMALL = 8;
constexpr int SELECT_BOX_MEDIUM = 12;
constexpr int SELECT_BOX_LARGE = 24;

// =============================================================================
// ObjectClass
// =============================================================================

/**
 * ObjectClass - Visible game object
 *
 * Adds to AbstractClass:
 * - World coordinate position
 * - Health/strength
 * - Owner house
 * - Selection state
 * - Drawing interface
 * - Damage handling
 */
class ObjectClass : public AbstractClass {
public:
    // -------------------------------------------------------------------------
    // Construction / Destruction
    // -------------------------------------------------------------------------

    ObjectClass();
    virtual ~ObjectClass();

    // -------------------------------------------------------------------------
    // Position
    // -------------------------------------------------------------------------

    /**
     * Get/set world coordinate
     */
    virtual COORDINATE Get_Coord() const override { return coord_; }
    virtual void Set_Coord(COORDINATE coord);

    /**
     * Get/set cell position
     */
    CELL Get_Cell() const { return Coord_Cell(coord_); }
    void Set_Cell(CELL cell);

    /**
     * Mark position in cell occupancy
     */
    virtual void Mark_Cell();
    virtual void Unmark_Cell();

    // -------------------------------------------------------------------------
    // Health
    // -------------------------------------------------------------------------

    /**
     * Get current strength/health
     */
    int Get_Strength() const { return strength_; }

    /**
     * Get maximum health
     */
    virtual int Max_Strength() const { return MAX_HEALTH; }

    /**
     * Get health as percentage (0-100)
     */
    int Health_Percent() const;

    /**
     * Is object at full health?
     */
    bool Is_Full_Health() const { return strength_ >= Max_Strength(); }

    /**
     * Is object destroyed?
     */
    bool Is_Destroyed() const { return strength_ <= 0; }

    /**
     * Set health directly
     */
    void Set_Strength(int str);

    /**
     * Heal object
     */
    virtual void Heal(int amount);

    // -------------------------------------------------------------------------
    // Owner
    // -------------------------------------------------------------------------

    /**
     * Get owning house
     */
    HousesType Get_Owner() const { return owner_; }

    /**
     * Set owner
     */
    void Set_Owner(HousesType house) { owner_ = house; }

    /**
     * Is owned by specific house?
     */
    bool Is_Owned_By(HousesType house) const { return owner_ == house; }

    // -------------------------------------------------------------------------
    // Selection
    // -------------------------------------------------------------------------

    /**
     * Get/set selection state
     */
    bool Is_Selected() const { return is_selected_; }
    void Select() { is_selected_ = true; }
    void Deselect() { is_selected_ = false; }

    /**
     * Get selection box size
     */
    virtual int Select_Box_Size() const { return SELECT_BOX_SMALL; }

    // -------------------------------------------------------------------------
    // Drawing
    // -------------------------------------------------------------------------

    /**
     * Draw - Render object to screen
     *
     * @param x Screen X position
     * @param y Screen Y position
     * @param window Clipping rectangle (or nullptr)
     */
    virtual void Draw(int x, int y, void* window = nullptr);

    /**
     * Draw_Selection - Draw selection box
     */
    virtual void Draw_Selection(int x, int y);

    /**
     * Draw_Health - Draw health bar
     */
    virtual void Draw_Health(int x, int y);

    // -------------------------------------------------------------------------
    // AI/Updates
    // -------------------------------------------------------------------------

    /**
     * AI - Per-tick update
     *
     * Called each game tick to update object state.
     */
    virtual void AI();

    /**
     * Per_Cell_Process - Called when entering new cell
     *
     * @param from_center True if moving from cell center
     */
    virtual void Per_Cell_Process(bool from_center);

    // -------------------------------------------------------------------------
    // Damage
    // -------------------------------------------------------------------------

    /**
     * Take_Damage - Apply damage to object
     *
     * @param damage Amount of damage
     * @param source Object that caused damage (or nullptr)
     * @param warhead Warhead type (affects armor)
     * @return Actual damage taken
     */
    virtual int Take_Damage(int damage, ObjectClass* source = nullptr, int warhead = 0);

    /**
     * Destroyed - Called when object is destroyed
     */
    virtual void Destroyed();

    // -------------------------------------------------------------------------
    // Object List
    // -------------------------------------------------------------------------

    /**
     * Link/Unlink from global object list
     */
    void Link_To_List();
    void Unlink_From_List();

    /**
     * Get next/previous in list
     */
    ObjectClass* Next_Object() const { return next_; }
    ObjectClass* Prev_Object() const { return prev_; }

protected:
    // -------------------------------------------------------------------------
    // Protected Members
    // -------------------------------------------------------------------------

    COORDINATE coord_;       // World position
    int strength_;           // Current health
    HousesType owner_;       // Owning house
    bool is_selected_;       // Selection state

    // Object list links
    ObjectClass* next_;
    ObjectClass* prev_;
};

// =============================================================================
// Global Object List
// =============================================================================

/**
 * AllObjects - Head of global object list
 */
extern ObjectClass* AllObjects;

/**
 * Object iteration helper
 */
#define FOR_ALL_OBJECTS(var) \
    for (ObjectClass* var = AllObjects; var != nullptr; var = var->Next_Object())
