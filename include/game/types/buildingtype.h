/**
 * BuildingTypeClass - Structure Type Definitions
 *
 * Type class for buildings and structures.
 * Adds power, size, factory capabilities.
 *
 * Original location: CODE/BTYPE.H
 */

#pragma once

#include "game/types/technotype.h"

// =============================================================================
// Building Type Enum
// =============================================================================

/**
 * BuildingType - All structure types
 */
enum BuildingType : int8_t {
    BUILDING_NONE = -1,

    // Base structures
    BUILDING_FACT = 0,      // Construction Yard
    BUILDING_POWR = 1,      // Power Plant
    BUILDING_APWR = 2,      // Advanced Power Plant
    BUILDING_BARR = 3,      // Barracks (Allied)
    BUILDING_TENT = 4,      // Barracks (Soviet)
    BUILDING_WEAP = 5,      // War Factory
    BUILDING_DOME = 6,      // Radar Dome
    BUILDING_HPAD = 7,      // Helipad
    BUILDING_AFLD = 8,      // Airfield
    BUILDING_SPEN = 9,      // Sub Pen
    BUILDING_SYRD = 10,     // Shipyard

    // Defenses
    BUILDING_GUN = 11,      // Turret
    BUILDING_AGUN = 12,     // AA Gun
    BUILDING_GTWR = 13,     // Guard Tower
    BUILDING_TSLA = 14,     // Tesla Coil
    BUILDING_SAM = 15,      // SAM Site

    // Other
    BUILDING_SILO = 16,     // Ore Silo
    BUILDING_PROC = 17,     // Ore Refinery
    BUILDING_FCOM = 18,     // Forward Command

    // Tech buildings
    BUILDING_ATEK = 19,     // Allied Tech Center
    BUILDING_STEK = 20,     // Soviet Tech Center

    // Add more as needed...

    BUILDING_COUNT = 21
};

// =============================================================================
// BuildingTypeClass
// =============================================================================

/**
 * BuildingTypeClass - Structure type definition
 *
 * Adds to TechnoTypeClass:
 * - Power production/consumption
 * - Building size
 * - Factory type
 * - Bib (foundation graphic)
 */
class BuildingTypeClass : public TechnoTypeClass {
public:
    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------

    BuildingTypeClass();
    BuildingTypeClass(const char* ini_name, const char* full_name);

    // -------------------------------------------------------------------------
    // Size
    // -------------------------------------------------------------------------

    /**
     * Get building size in cells
     */
    int Get_Width() const { return width_; }
    int Get_Height() const { return height_; }
    void Set_Size(int w, int h) { width_ = w; height_ = h; }

    /**
     * Get total cell count
     */
    int Cell_Count() const { return width_ * height_; }

    // -------------------------------------------------------------------------
    // Power
    // -------------------------------------------------------------------------

    /**
     * Get power production (positive) or consumption (negative)
     */
    int Get_Power() const { return power_; }
    void Set_Power(int power) { power_ = power; }

    /**
     * Is power plant?
     */
    bool Is_Power_Plant() const { return power_ > 0; }

    // -------------------------------------------------------------------------
    // Production
    // -------------------------------------------------------------------------

    /**
     * Get factory type (what this building produces)
     * 0 = nothing, 1 = infantry, 2 = units, 3 = aircraft, 4 = buildings
     */
    int Get_Factory_Type() const { return factory_type_; }
    void Set_Factory_Type(int type) { factory_type_ = type; }

    /**
     * Is construction yard?
     */
    bool Is_Construction_Yard() const { return is_conyard_; }
    void Set_Construction_Yard(bool val) { is_conyard_ = val; }

    // -------------------------------------------------------------------------
    // Defense
    // -------------------------------------------------------------------------

    /**
     * Is base defense (turret, sam, tesla)?
     */
    bool Is_Defense() const { return is_defense_; }
    void Set_Defense(bool val) { is_defense_ = val; }

    // -------------------------------------------------------------------------
    // Bib (Foundation)
    // -------------------------------------------------------------------------

    /**
     * Has concrete bib/foundation?
     */
    bool Has_Bib() const { return has_bib_; }
    void Set_Has_Bib(bool val) { has_bib_ = val; }

protected:
    // Size
    int width_;
    int height_;

    // Power
    int power_;

    // Production
    int factory_type_;
    bool is_conyard_;

    // Defense
    bool is_defense_;

    // Graphics
    bool has_bib_;
};

// =============================================================================
// Global Building Types Array
// =============================================================================

extern BuildingTypeClass BuildingTypes[BUILDING_COUNT];

// =============================================================================
// Utility Functions
// =============================================================================

/**
 * Get building type by index
 */
inline BuildingTypeClass* Building_Type(BuildingType type) {
    if (type < 0 || type >= BUILDING_COUNT) return nullptr;
    return &BuildingTypes[type];
}

/**
 * Get building type by name
 */
BuildingType Building_Type_From_Name(const char* name);
