/**
 * UnitTypeClass - Vehicle Type Definitions
 *
 * Type class for ground vehicles (tanks, APCs, harvesters).
 * Adds movement speed, turret, and vehicle-specific properties.
 *
 * Original location: CODE/UTYPE.H
 */

#pragma once

#include "game/types/technotype.h"

// =============================================================================
// Unit Type Enum
// =============================================================================

/**
 * UnitType - All vehicle types
 */
enum UnitType : int8_t {
    UNIT_NONE = -1,

    // Allied vehicles
    UNIT_MTNK = 0,      // Medium Tank
    UNIT_LTNK = 1,      // Light Tank
    UNIT_HTNK = 2,      // Heavy Tank (Mammoth)
    UNIT_APC = 3,       // Armored Personnel Carrier
    UNIT_ARTY = 4,      // Artillery
    UNIT_HARV = 5,      // Harvester
    UNIT_MCV = 6,       // Mobile Construction Vehicle
    UNIT_JEEP = 7,      // Ranger

    // Soviet vehicles
    UNIT_V2RL = 8,      // V2 Rocket Launcher
    UNIT_FTNK = 9,      // Flame Tank

    // Add more as needed...

    UNIT_COUNT = 10
};

// =============================================================================
// UnitTypeClass
// =============================================================================

/**
 * UnitTypeClass - Vehicle type definition
 *
 * Adds to TechnoTypeClass:
 * - Speed
 * - Turret
 * - Tracked vs wheeled
 * - Crushable infantry
 */
class UnitTypeClass : public TechnoTypeClass {
public:
    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------

    UnitTypeClass();
    UnitTypeClass(const char* ini_name, const char* full_name);

    // -------------------------------------------------------------------------
    // Movement
    // -------------------------------------------------------------------------

    /**
     * Get movement speed
     */
    int Get_Speed() const { return speed_; }
    void Set_Speed(int speed) { speed_ = speed; }

    /**
     * Is tracked (vs wheeled)?
     */
    bool Is_Tracked() const { return is_tracked_; }
    void Set_Tracked(bool val) { is_tracked_ = val; }

    /**
     * Can crush infantry?
     */
    bool Can_Crush() const { return can_crush_; }
    void Set_Can_Crush(bool val) { can_crush_ = val; }

    // -------------------------------------------------------------------------
    // Turret
    // -------------------------------------------------------------------------

    /**
     * Has rotating turret?
     */
    bool Has_Turret() const { return has_turret_; }
    void Set_Has_Turret(bool val) { has_turret_ = val; }

    /**
     * Get turret rotation rate
     */
    int Get_Turret_Rate() const { return turret_rate_; }
    void Set_Turret_Rate(int rate) { turret_rate_ = rate; }

    // -------------------------------------------------------------------------
    // Special
    // -------------------------------------------------------------------------

    /**
     * Is harvester?
     */
    bool Is_Harvester() const { return is_harvester_; }
    void Set_Harvester(bool val) { is_harvester_ = val; }

    /**
     * Is MCV?
     */
    bool Is_MCV() const { return is_mcv_; }
    void Set_MCV(bool val) { is_mcv_ = val; }

protected:
    // Movement
    int speed_;
    bool is_tracked_;
    bool can_crush_;

    // Turret
    bool has_turret_;
    int turret_rate_;

    // Special
    bool is_harvester_;
    bool is_mcv_;
};

// =============================================================================
// Global Unit Types Array
// =============================================================================

extern UnitTypeClass UnitTypes[UNIT_COUNT];

// =============================================================================
// Utility Functions
// =============================================================================

/**
 * Get unit type by index
 */
inline UnitTypeClass* Unit_Type(UnitType type) {
    if (type < 0 || type >= UNIT_COUNT) return nullptr;
    return &UnitTypes[type];
}

/**
 * Get unit type by name
 */
UnitType Unit_Type_From_Name(const char* name);
