/**
 * TechnoClass - Military Unit Base
 *
 * Base class for all controllable military units (tanks, infantry,
 * buildings, aircraft). Adds weapons, armor, and tactical AI.
 *
 * Original location: CODE/TECHNO.H, CODE/TECHNO.CPP
 *
 * Note: This is a stub for initial porting. Full implementation
 * will be expanded in later tasks.
 */

#pragma once

#include "game/object.h"
#include "game/house.h"
#include "game/facing.h"
#include "game/weapon.h"

// Forward declarations
class HouseClass;

// =============================================================================
// TechnoClass
// =============================================================================

/**
 * TechnoClass - Military unit base
 *
 * Adds to ObjectClass:
 * - Facing/direction
 * - Weapons and targeting
 * - Armor type
 * - Cloaking (Allies)
 * - Veterancy (if applicable)
 */
class TechnoClass : public ObjectClass {
public:
    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------

    TechnoClass();
    virtual ~TechnoClass();

    // -------------------------------------------------------------------------
    // Facing
    // -------------------------------------------------------------------------

    /**
     * Get primary facing
     */
    DirType Get_Facing() const { return body_facing_.Current(); }
    FacingClass& Body_Facing() { return body_facing_; }

    /**
     * Get turret facing (if has turret)
     */
    DirType Get_Turret_Facing() const { return turret_facing_.Current(); }
    FacingClass& Turret_Facing() { return turret_facing_; }

    /**
     * Does this techno have a turret?
     */
    virtual bool Has_Turret() const { return false; }

    // -------------------------------------------------------------------------
    // Combat
    // -------------------------------------------------------------------------

    /**
     * Get armor type
     */
    virtual ArmorType Get_Armor() const { return armor_; }

    /**
     * Get primary weapon
     */
    virtual WeaponType Get_Primary_Weapon() const { return primary_weapon_; }

    /**
     * Get secondary weapon
     */
    virtual WeaponType Get_Secondary_Weapon() const { return secondary_weapon_; }

    /**
     * Can attack target?
     */
    virtual bool Can_Attack(ObjectClass* target) const;

    /**
     * Get current target
     */
    ObjectClass* Get_Target() const { return target_; }

    /**
     * Set target
     */
    void Set_Target(ObjectClass* target) { target_ = target; }

    // -------------------------------------------------------------------------
    // Cloaking (Allies stealth tech)
    // -------------------------------------------------------------------------

    bool Is_Cloaked() const { return is_cloaked_; }
    void Cloak() { is_cloaked_ = true; }
    void Decloak() { is_cloaked_ = false; }

    // -------------------------------------------------------------------------
    // Selection
    // -------------------------------------------------------------------------

    virtual int Select_Box_Size() const override { return SELECT_BOX_MEDIUM; }

    // -------------------------------------------------------------------------
    // AI
    // -------------------------------------------------------------------------

    virtual void AI() override;

protected:
    // Facing
    FacingClass body_facing_;     // Body direction
    FacingClass turret_facing_;   // Turret direction (if has turret)

    // Combat
    ArmorType armor_;             // Armor type
    WeaponType primary_weapon_;   // Primary weapon
    WeaponType secondary_weapon_; // Secondary weapon
    ObjectClass* target_;         // Current target

    // Stealth
    bool is_cloaked_;             // Cloaking active
};

// =============================================================================
// FootClass - Mobile Units (Stub)
// =============================================================================

/**
 * FootClass - Mobile unit base
 *
 * Base for all units that can move (vehicles, infantry, aircraft).
 * This is a stub - full implementation in later tasks.
 */
class FootClass : public TechnoClass {
public:
    FootClass();
    virtual ~FootClass();

    // Speed and movement
    virtual int Get_Speed() const { return speed_; }
    void Set_Speed(int speed) { speed_ = speed; }

    // Destination
    COORDINATE Get_Destination() const { return destination_; }
    void Set_Destination(COORDINATE dest) { destination_ = dest; }

    // Is moving?
    bool Is_Moving() const { return destination_ != COORD_NONE && destination_ != coord_; }

protected:
    int speed_;                   // Movement speed
    COORDINATE destination_;      // Movement target
};
