/**
 * TechnoTypeClass - Military Unit Type
 *
 * Type class for controllable military units.
 * Adds weapons, costs, prerequisites, and production info.
 *
 * Original location: CODE/TTYPE.H
 */

#pragma once

#include "game/types/objecttype.h"
#include "game/house.h"
#include "game/weapon.h"

// =============================================================================
// Prerequisite Flags
// =============================================================================

/**
 * Flags for building prerequisites
 */
enum PrereqFlags : uint32_t {
    PREREQ_NONE = 0,
    PREREQ_BARRACKS = (1 << 0),     // Requires barracks
    PREREQ_WARFACTORY = (1 << 1),   // Requires war factory
    PREREQ_RADAR = (1 << 2),        // Requires radar
    PREREQ_TECHCENTER = (1 << 3),   // Requires tech center
    PREREQ_AIRFIELD = (1 << 4),     // Requires airfield
    PREREQ_SHIPYARD = (1 << 5),     // Requires shipyard
    PREREQ_HELIPAD = (1 << 6),      // Requires helipad
};

// =============================================================================
// TechnoTypeClass
// =============================================================================

/**
 * TechnoTypeClass - Type for military units
 *
 * Adds to ObjectTypeClass:
 * - Weapons
 * - Cost and build time
 * - Prerequisites
 * - Tech level
 * - Owner restrictions
 */
class TechnoTypeClass : public ObjectTypeClass {
public:
    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------

    TechnoTypeClass();
    TechnoTypeClass(const char* ini_name, const char* full_name, RTTIType rtti);

    // -------------------------------------------------------------------------
    // Weapons
    // -------------------------------------------------------------------------

    /**
     * Get primary weapon
     */
    WeaponType Get_Primary_Weapon() const { return primary_weapon_; }
    void Set_Primary_Weapon(WeaponType weapon) { primary_weapon_ = weapon; }

    /**
     * Get secondary weapon
     */
    WeaponType Get_Secondary_Weapon() const { return secondary_weapon_; }
    void Set_Secondary_Weapon(WeaponType weapon) { secondary_weapon_ = weapon; }

    /**
     * Has any weapon?
     */
    bool Has_Weapon() const {
        return primary_weapon_ != WEAPON_NONE || secondary_weapon_ != WEAPON_NONE;
    }

    // -------------------------------------------------------------------------
    // Cost/Production
    // -------------------------------------------------------------------------

    /**
     * Get cost in credits
     */
    int Get_Cost() const { return cost_; }
    void Set_Cost(int cost) { cost_ = cost; }

    /**
     * Get build time (in ticks at normal speed)
     */
    int Get_Build_Time() const { return build_time_; }
    void Set_Build_Time(int time) { build_time_ = time; }

    // -------------------------------------------------------------------------
    // Prerequisites
    // -------------------------------------------------------------------------

    /**
     * Get prerequisite flags
     */
    uint32_t Get_Prerequisites() const { return prerequisites_; }
    void Set_Prerequisites(uint32_t prereqs) { prerequisites_ = prereqs; }

    /**
     * Check if prerequisites are met
     */
    bool Prerequisites_Met(uint32_t built_flags) const {
        return (prerequisites_ & built_flags) == prerequisites_;
    }

    // -------------------------------------------------------------------------
    // Tech Level
    // -------------------------------------------------------------------------

    /**
     * Get tech level (higher = later in tech tree)
     */
    int Get_Tech_Level() const { return tech_level_; }
    void Set_Tech_Level(int level) { tech_level_ = level; }

    // -------------------------------------------------------------------------
    // Ownership
    // -------------------------------------------------------------------------

    /**
     * Get owning side (Allied/Soviet/Both)
     */
    SideType Get_Side() const { return side_; }
    void Set_Side(SideType side) { side_ = side; }

    /**
     * Can this side build this unit?
     */
    bool Is_Buildable_By(SideType side) const;

    // -------------------------------------------------------------------------
    // Combat Stats
    // -------------------------------------------------------------------------

    /**
     * Get sight range (in cells)
     */
    int Get_Sight_Range() const { return sight_range_; }
    void Set_Sight_Range(int range) { sight_range_ = range; }

protected:
    // Weapons
    WeaponType primary_weapon_;
    WeaponType secondary_weapon_;

    // Cost/Production
    int cost_;
    int build_time_;

    // Prerequisites
    uint32_t prerequisites_;
    int tech_level_;

    // Ownership
    SideType side_;

    // Combat
    int sight_range_;
};

// =============================================================================
// Inline Implementation
// =============================================================================

inline TechnoTypeClass::TechnoTypeClass()
    : ObjectTypeClass()
    , primary_weapon_(WEAPON_NONE)
    , secondary_weapon_(WEAPON_NONE)
    , cost_(0)
    , build_time_(0)
    , prerequisites_(PREREQ_NONE)
    , tech_level_(1)
    , side_(SIDE_ALLIED)
    , sight_range_(3)
{
}

inline TechnoTypeClass::TechnoTypeClass(const char* ini_name, const char* full_name, RTTIType rtti)
    : ObjectTypeClass(ini_name, full_name, rtti)
    , primary_weapon_(WEAPON_NONE)
    , secondary_weapon_(WEAPON_NONE)
    , cost_(0)
    , build_time_(0)
    , prerequisites_(PREREQ_NONE)
    , tech_level_(1)
    , side_(SIDE_ALLIED)
    , sight_range_(3)
{
}

inline bool TechnoTypeClass::Is_Buildable_By(SideType side) const {
    if (side_ == SIDE_NEUTRAL) return true; // Both sides can build
    return (side_ == side);
}
