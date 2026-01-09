# Task 14h: Type Classes System

## Dependencies
- Task 14g (ObjectClass System) must be complete
- Task 14b (Coordinate Types) must be complete

## Context
Type classes define the static properties of game entities - what a "Medium Tank" is, not a specific tank on the map. Each unit type, building type, and weapon type has a corresponding TypeClass that stores its stats, costs, prerequisites, and graphics references.

## Objective
Implement the TypeClass system:
1. AbstractTypeClass base
2. ObjectTypeClass for visible objects
3. TechnoTypeClass for military units
4. UnitTypeClass for vehicle definitions
5. BuildingTypeClass for structure definitions
6. Static type arrays with sample data

## Technical Background

### Original Type Class Hierarchy
```
AbstractTypeClass (name, RTTI)
    |
    +-- ObjectTypeClass (graphics, armor)
        |
        +-- TechnoTypeClass (weapons, cost)
            |
            +-- UnitTypeClass (speed, turret)
            +-- BuildingTypeClass (power, size)
            +-- InfantryTypeClass (fire positions)
            +-- AircraftTypeClass (flight behavior)
```

### Type vs. Instance
- TypeClass: Static definition ("Medium Tank has 400 HP, costs $800")
- ObjectClass: Runtime instance ("This specific tank at position X,Y")

### Type Lookup
Types are stored in static arrays indexed by enum:
```cpp
enum UnitType { UNIT_MTNK, UNIT_HTNK, ... };
UnitTypeClass UnitTypes[UNIT_COUNT];
```

## Deliverables
- [ ] `include/game/types/abstracttype.h` - Base type class
- [ ] `include/game/types/objecttype.h` - Object type class
- [ ] `include/game/types/technotype.h` - Techno type class
- [ ] `include/game/types/unittype.h` - Unit type definitions
- [ ] `include/game/types/buildingtype.h` - Building type definitions
- [ ] `src/game/types/types.cpp` - Type data tables
- [ ] Sample unit and building definitions
- [ ] Verification script passes

## Files to Create

### include/game/types/abstracttype.h (NEW)
```cpp
/**
 * AbstractTypeClass - Base Type Definition
 *
 * Base class for all type definitions. Types define the static
 * properties of game entities (what a "Medium Tank" is).
 *
 * Original location: CODE/TYPE.H
 */

#pragma once

#include "game/core/rtti.h"
#include <cstdint>

// =============================================================================
// AbstractTypeClass
// =============================================================================

/**
 * AbstractTypeClass - Base for all type definitions
 *
 * Provides:
 * - Internal name (for INI files)
 * - Display name (for UI)
 * - RTTI type identification
 */
class AbstractTypeClass {
public:
    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------

    AbstractTypeClass();
    AbstractTypeClass(const char* ini_name, const char* full_name, RTTIType rtti);
    virtual ~AbstractTypeClass() = default;

    // -------------------------------------------------------------------------
    // Names
    // -------------------------------------------------------------------------

    /**
     * Get internal name (used in INI files)
     * Example: "MTNK" for Medium Tank
     */
    const char* Get_Name() const { return ini_name_; }

    /**
     * Get display name (shown in UI)
     * Example: "Medium Tank"
     */
    const char* Get_Full_Name() const { return full_name_; }

    // -------------------------------------------------------------------------
    // Type Identification
    // -------------------------------------------------------------------------

    /**
     * Get RTTI type (what kind of type is this)
     */
    RTTIType What_Am_I() const { return rtti_type_; }

    // -------------------------------------------------------------------------
    // Index
    // -------------------------------------------------------------------------

    /**
     * Get/set array index
     */
    int Get_Type_Index() const { return type_index_; }
    void Set_Type_Index(int idx) { type_index_ = idx; }

protected:
    const char* ini_name_;       // INI identifier
    const char* full_name_;      // Display name
    RTTIType rtti_type_;         // Type category
    int type_index_;             // Index in type array
};

// =============================================================================
// Inline Implementation
// =============================================================================

inline AbstractTypeClass::AbstractTypeClass()
    : ini_name_("")
    , full_name_("")
    , rtti_type_(RTTI_NONE)
    , type_index_(-1)
{
}

inline AbstractTypeClass::AbstractTypeClass(const char* ini_name, const char* full_name, RTTIType rtti)
    : ini_name_(ini_name ? ini_name : "")
    , full_name_(full_name ? full_name : "")
    , rtti_type_(rtti)
    , type_index_(-1)
{
}
```

### include/game/types/objecttype.h (NEW)
```cpp
/**
 * ObjectTypeClass - Visible Object Type
 *
 * Type class for objects that have visual representation.
 * Adds graphics references and basic combat properties.
 *
 * Original location: CODE/OTYPE.H
 */

#pragma once

#include "game/types/abstracttype.h"
#include "game/weapon.h"

// =============================================================================
// ObjectTypeClass
// =============================================================================

/**
 * ObjectTypeClass - Type for visible game objects
 *
 * Adds to AbstractTypeClass:
 * - Shape/graphics reference
 * - Armor type
 * - Maximum strength
 */
class ObjectTypeClass : public AbstractTypeClass {
public:
    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------

    ObjectTypeClass();
    ObjectTypeClass(const char* ini_name, const char* full_name, RTTIType rtti);

    // -------------------------------------------------------------------------
    // Graphics
    // -------------------------------------------------------------------------

    /**
     * Get shape file name (without path or extension)
     * Example: "MTNK" -> load MTNK.SHP
     */
    const char* Get_Shape_Name() const { return shape_name_; }
    void Set_Shape_Name(const char* name) { shape_name_ = name; }

    /**
     * Get number of frames in shape
     */
    int Get_Frame_Count() const { return frame_count_; }
    void Set_Frame_Count(int count) { frame_count_ = count; }

    // -------------------------------------------------------------------------
    // Combat
    // -------------------------------------------------------------------------

    /**
     * Get armor type
     */
    ArmorType Get_Armor() const { return armor_; }
    void Set_Armor(ArmorType armor) { armor_ = armor; }

    /**
     * Get maximum strength (health)
     */
    int Get_Max_Strength() const { return max_strength_; }
    void Set_Max_Strength(int strength) { max_strength_ = strength; }

    // -------------------------------------------------------------------------
    // Selection
    // -------------------------------------------------------------------------

    /**
     * Is this type selectable by player?
     */
    bool Is_Selectable() const { return is_selectable_; }
    void Set_Selectable(bool val) { is_selectable_ = val; }

protected:
    // Graphics
    const char* shape_name_;     // Shape file name
    int frame_count_;            // Number of animation frames

    // Combat
    ArmorType armor_;            // Armor type
    int max_strength_;           // Max health

    // Flags
    bool is_selectable_;         // Can be selected
};

// =============================================================================
// Inline Implementation
// =============================================================================

inline ObjectTypeClass::ObjectTypeClass()
    : AbstractTypeClass()
    , shape_name_("")
    , frame_count_(1)
    , armor_(ARMOR_NONE)
    , max_strength_(1)
    , is_selectable_(true)
{
}

inline ObjectTypeClass::ObjectTypeClass(const char* ini_name, const char* full_name, RTTIType rtti)
    : AbstractTypeClass(ini_name, full_name, rtti)
    , shape_name_(ini_name)
    , frame_count_(1)
    , armor_(ARMOR_NONE)
    , max_strength_(1)
    , is_selectable_(true)
{
}
```

### include/game/types/technotype.h (NEW)
```cpp
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
    if (side_ == SIDE_NEUTRAL) return false;
    return (side_ == side);
}
```

### include/game/types/unittype.h (NEW)
```cpp
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
```

### include/game/types/buildingtype.h (NEW)
```cpp
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
```

### src/game/types/types.cpp (NEW)
```cpp
/**
 * Type Classes Implementation
 *
 * Defines the static type arrays with actual game data.
 */

#include "game/types/unittype.h"
#include "game/types/buildingtype.h"
#include <cstring>

// =============================================================================
// UnitTypeClass Implementation
// =============================================================================

UnitTypeClass::UnitTypeClass()
    : TechnoTypeClass()
    , speed_(0)
    , is_tracked_(true)
    , can_crush_(false)
    , has_turret_(false)
    , turret_rate_(5)
    , is_harvester_(false)
    , is_mcv_(false)
{
    rtti_type_ = RTTI_UNIT;
}

UnitTypeClass::UnitTypeClass(const char* ini_name, const char* full_name)
    : TechnoTypeClass(ini_name, full_name, RTTI_UNIT)
    , speed_(10)
    , is_tracked_(true)
    , can_crush_(false)
    , has_turret_(false)
    , turret_rate_(5)
    , is_harvester_(false)
    , is_mcv_(false)
{
}

// =============================================================================
// Unit Types Definition
// =============================================================================

UnitTypeClass UnitTypes[UNIT_COUNT] = {
    // UNIT_MTNK - Medium Tank
    []() {
        UnitTypeClass t("MTNK", "Medium Tank");
        t.Set_Type_Index(UNIT_MTNK);
        t.Set_Shape_Name("MTNK");
        t.Set_Frame_Count(32);
        t.Set_Max_Strength(400);
        t.Set_Armor(ARMOR_HEAVY);
        t.Set_Speed(10);
        t.Set_Primary_Weapon(WEAPON_90MM);
        t.Set_Has_Turret(true);
        t.Set_Can_Crush(true);
        t.Set_Cost(800);
        t.Set_Build_Time(100);
        t.Set_Prerequisites(PREREQ_WARFACTORY);
        t.Set_Tech_Level(2);
        t.Set_Side(SIDE_ALLIED);
        t.Set_Sight_Range(5);
        return t;
    }(),

    // UNIT_LTNK - Light Tank
    []() {
        UnitTypeClass t("LTNK", "Light Tank");
        t.Set_Type_Index(UNIT_LTNK);
        t.Set_Shape_Name("LTNK");
        t.Set_Frame_Count(32);
        t.Set_Max_Strength(300);
        t.Set_Armor(ARMOR_LIGHT);
        t.Set_Speed(12);
        t.Set_Primary_Weapon(WEAPON_75MM);
        t.Set_Has_Turret(true);
        t.Set_Cost(600);
        t.Set_Build_Time(80);
        t.Set_Prerequisites(PREREQ_WARFACTORY);
        t.Set_Tech_Level(1);
        t.Set_Side(SIDE_ALLIED);
        t.Set_Sight_Range(5);
        return t;
    }(),

    // UNIT_HTNK - Heavy Tank (Mammoth)
    []() {
        UnitTypeClass t("HTNK", "Heavy Tank");
        t.Set_Type_Index(UNIT_HTNK);
        t.Set_Shape_Name("HTNK");
        t.Set_Frame_Count(32);
        t.Set_Max_Strength(600);
        t.Set_Armor(ARMOR_HEAVY);
        t.Set_Speed(6);
        t.Set_Primary_Weapon(WEAPON_120MM);
        t.Set_Secondary_Weapon(WEAPON_MAMMOTH_TUSK);
        t.Set_Has_Turret(true);
        t.Set_Can_Crush(true);
        t.Set_Cost(1500);
        t.Set_Build_Time(150);
        t.Set_Prerequisites(PREREQ_WARFACTORY | PREREQ_TECHCENTER);
        t.Set_Tech_Level(5);
        t.Set_Side(SIDE_SOVIET);
        t.Set_Sight_Range(4);
        return t;
    }(),

    // UNIT_APC - Armored Personnel Carrier
    []() {
        UnitTypeClass t("APC", "APC");
        t.Set_Type_Index(UNIT_APC);
        t.Set_Shape_Name("APC");
        t.Set_Max_Strength(200);
        t.Set_Armor(ARMOR_LIGHT);
        t.Set_Speed(14);
        t.Set_Primary_Weapon(WEAPON_M60MG);
        t.Set_Cost(700);
        t.Set_Build_Time(90);
        t.Set_Prerequisites(PREREQ_WARFACTORY);
        t.Set_Tech_Level(2);
        t.Set_Side(SIDE_ALLIED);
        return t;
    }(),

    // UNIT_ARTY - Artillery
    []() {
        UnitTypeClass t("ARTY", "Artillery");
        t.Set_Type_Index(UNIT_ARTY);
        t.Set_Shape_Name("ARTY");
        t.Set_Max_Strength(150);
        t.Set_Armor(ARMOR_LIGHT);
        t.Set_Speed(8);
        t.Set_Primary_Weapon(WEAPON_155MM);
        t.Set_Cost(600);
        t.Set_Build_Time(80);
        t.Set_Prerequisites(PREREQ_WARFACTORY);
        t.Set_Tech_Level(4);
        t.Set_Side(SIDE_ALLIED);
        return t;
    }(),

    // UNIT_HARV - Harvester
    []() {
        UnitTypeClass t("HARV", "Ore Truck");
        t.Set_Type_Index(UNIT_HARV);
        t.Set_Shape_Name("HARV");
        t.Set_Max_Strength(600);
        t.Set_Armor(ARMOR_LIGHT);
        t.Set_Speed(8);
        t.Set_Cost(1400);
        t.Set_Build_Time(120);
        t.Set_Harvester(true);
        t.Set_Side(SIDE_NEUTRAL); // Both sides use
        return t;
    }(),

    // UNIT_MCV - Mobile Construction Vehicle
    []() {
        UnitTypeClass t("MCV", "MCV");
        t.Set_Type_Index(UNIT_MCV);
        t.Set_Shape_Name("MCV");
        t.Set_Max_Strength(600);
        t.Set_Armor(ARMOR_LIGHT);
        t.Set_Speed(6);
        t.Set_Cost(2500);
        t.Set_Build_Time(200);
        t.Set_MCV(true);
        t.Set_Side(SIDE_NEUTRAL);
        return t;
    }(),

    // UNIT_JEEP - Ranger
    []() {
        UnitTypeClass t("JEEP", "Ranger");
        t.Set_Type_Index(UNIT_JEEP);
        t.Set_Shape_Name("JEEP");
        t.Set_Max_Strength(150);
        t.Set_Armor(ARMOR_LIGHT);
        t.Set_Speed(16);
        t.Set_Tracked(false);
        t.Set_Primary_Weapon(WEAPON_M60MG);
        t.Set_Cost(500);
        t.Set_Build_Time(60);
        t.Set_Prerequisites(PREREQ_WARFACTORY);
        t.Set_Tech_Level(1);
        t.Set_Side(SIDE_ALLIED);
        return t;
    }(),

    // UNIT_V2RL - V2 Rocket Launcher
    []() {
        UnitTypeClass t("V2RL", "V2 Launcher");
        t.Set_Type_Index(UNIT_V2RL);
        t.Set_Shape_Name("V2RL");
        t.Set_Max_Strength(150);
        t.Set_Armor(ARMOR_LIGHT);
        t.Set_Speed(8);
        t.Set_Primary_Weapon(WEAPON_SCUD);
        t.Set_Cost(700);
        t.Set_Build_Time(100);
        t.Set_Prerequisites(PREREQ_WARFACTORY | PREREQ_RADAR);
        t.Set_Tech_Level(4);
        t.Set_Side(SIDE_SOVIET);
        return t;
    }(),

    // UNIT_FTNK - Flame Tank
    []() {
        UnitTypeClass t("FTNK", "Flame Tank");
        t.Set_Type_Index(UNIT_FTNK);
        t.Set_Shape_Name("FTNK");
        t.Set_Max_Strength(300);
        t.Set_Armor(ARMOR_HEAVY);
        t.Set_Speed(10);
        t.Set_Primary_Weapon(WEAPON_FIREBALL);
        t.Set_Cost(800);
        t.Set_Build_Time(100);
        t.Set_Prerequisites(PREREQ_WARFACTORY);
        t.Set_Tech_Level(3);
        t.Set_Side(SIDE_SOVIET);
        return t;
    }(),
};

UnitType Unit_Type_From_Name(const char* name) {
    if (name == nullptr) return UNIT_NONE;
    for (int i = 0; i < UNIT_COUNT; i++) {
        if (strcmp(UnitTypes[i].Get_Name(), name) == 0) {
            return (UnitType)i;
        }
    }
    return UNIT_NONE;
}

// =============================================================================
// BuildingTypeClass Implementation
// =============================================================================

BuildingTypeClass::BuildingTypeClass()
    : TechnoTypeClass()
    , width_(1)
    , height_(1)
    , power_(0)
    , factory_type_(0)
    , is_conyard_(false)
    , is_defense_(false)
    , has_bib_(true)
{
    rtti_type_ = RTTI_BUILDING;
}

BuildingTypeClass::BuildingTypeClass(const char* ini_name, const char* full_name)
    : TechnoTypeClass(ini_name, full_name, RTTI_BUILDING)
    , width_(1)
    , height_(1)
    , power_(0)
    , factory_type_(0)
    , is_conyard_(false)
    , is_defense_(false)
    , has_bib_(true)
{
}

// =============================================================================
// Building Types Definition (abbreviated)
// =============================================================================

BuildingTypeClass BuildingTypes[BUILDING_COUNT] = {
    // BUILDING_FACT - Construction Yard
    []() {
        BuildingTypeClass t("FACT", "Construction Yard");
        t.Set_Type_Index(BUILDING_FACT);
        t.Set_Size(3, 3);
        t.Set_Max_Strength(1000);
        t.Set_Power(-30);
        t.Set_Factory_Type(4); // Builds buildings
        t.Set_Construction_Yard(true);
        t.Set_Side(SIDE_NEUTRAL);
        return t;
    }(),

    // BUILDING_POWR - Power Plant
    []() {
        BuildingTypeClass t("POWR", "Power Plant");
        t.Set_Type_Index(BUILDING_POWR);
        t.Set_Size(2, 2);
        t.Set_Max_Strength(400);
        t.Set_Power(100);
        t.Set_Cost(300);
        t.Set_Tech_Level(1);
        t.Set_Side(SIDE_NEUTRAL);
        return t;
    }(),

    // ... (rest initialized similarly)
    // For brevity, remaining buildings use default constructor
    BuildingTypeClass(),  // APWR
    BuildingTypeClass(),  // BARR
    BuildingTypeClass(),  // TENT
    BuildingTypeClass(),  // WEAP
    BuildingTypeClass(),  // DOME
    BuildingTypeClass(),  // HPAD
    BuildingTypeClass(),  // AFLD
    BuildingTypeClass(),  // SPEN
    BuildingTypeClass(),  // SYRD
    BuildingTypeClass(),  // GUN
    BuildingTypeClass(),  // AGUN
    BuildingTypeClass(),  // GTWR
    BuildingTypeClass(),  // TSLA
    BuildingTypeClass(),  // SAM
    BuildingTypeClass(),  // SILO
    BuildingTypeClass(),  // PROC
    BuildingTypeClass(),  // FCOM
    BuildingTypeClass(),  // ATEK
    BuildingTypeClass(),  // STEK
};

BuildingType Building_Type_From_Name(const char* name) {
    if (name == nullptr) return BUILDING_NONE;
    for (int i = 0; i < BUILDING_COUNT; i++) {
        if (strcmp(BuildingTypes[i].Get_Name(), name) == 0) {
            return (BuildingType)i;
        }
    }
    return BUILDING_NONE;
}
```

## Verification Command
```bash
ralph-tasks/verify/check-type-classes.sh
```

## Verification Script

### ralph-tasks/verify/check-type-classes.sh (NEW)
```bash
#!/bin/bash
# Verification script for Type Classes system

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Type Classes System ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check files exist
echo "[1/4] Checking files..."
for f in include/game/types/abstracttype.h include/game/types/objecttype.h \
         include/game/types/technotype.h include/game/types/unittype.h \
         include/game/types/buildingtype.h src/game/types/types.cpp; do
    if [ ! -f "$f" ]; then
        echo "VERIFY_FAILED: $f not found"
        exit 1
    fi
    echo "  OK: $f"
done

# Step 2: Syntax check
echo "[2/4] Checking syntax..."
if ! clang++ -std=c++17 -c -I include src/game/types/types.cpp -o /tmp/types.o 2>&1; then
    echo "VERIFY_FAILED: types.cpp compilation failed"
    exit 1
fi
echo "  OK: Syntax valid"

# Step 3: Build test
echo "[3/4] Building types test..."
cat > /tmp/test_types.cpp << 'EOF'
#include "game/types/unittype.h"
#include "game/types/buildingtype.h"
#include <cstdio>

#define TEST(name, cond) do { \
    if (!(cond)) { printf("FAIL: %s\n", name); return 1; } \
    printf("PASS: %s\n", name); \
} while(0)

int main() {
    printf("=== Type Classes Tests ===\n\n");

    // Test unit type
    UnitTypeClass* mtnk = Unit_Type(UNIT_MTNK);
    TEST("MTNK exists", mtnk != nullptr);
    TEST("MTNK name", strcmp(mtnk->Get_Name(), "MTNK") == 0);
    TEST("MTNK full name", strcmp(mtnk->Get_Full_Name(), "Medium Tank") == 0);
    TEST("MTNK has turret", mtnk->Has_Turret());
    TEST("MTNK can crush", mtnk->Can_Crush());
    TEST("MTNK cost", mtnk->Get_Cost() == 800);
    TEST("MTNK strength", mtnk->Get_Max_Strength() == 400);
    TEST("MTNK is allied", mtnk->Get_Side() == SIDE_ALLIED);

    // Test Soviet unit
    UnitTypeClass* htnk = Unit_Type(UNIT_HTNK);
    TEST("HTNK exists", htnk != nullptr);
    TEST("HTNK is soviet", htnk->Get_Side() == SIDE_SOVIET);
    TEST("HTNK secondary weapon", htnk->Get_Secondary_Weapon() != WEAPON_NONE);

    // Test special units
    UnitTypeClass* harv = Unit_Type(UNIT_HARV);
    TEST("Harvester is harvester", harv->Is_Harvester());

    UnitTypeClass* mcv = Unit_Type(UNIT_MCV);
    TEST("MCV is MCV", mcv->Is_MCV());

    // Test lookup by name
    UnitType ltnk = Unit_Type_From_Name("LTNK");
    TEST("Name lookup", ltnk == UNIT_LTNK);
    TEST("Invalid name", Unit_Type_From_Name("INVALID") == UNIT_NONE);

    // Test building type
    BuildingTypeClass* fact = Building_Type(BUILDING_FACT);
    TEST("FACT exists", fact != nullptr);
    TEST("FACT is conyard", fact->Is_Construction_Yard());
    TEST("FACT size", fact->Get_Width() == 3 && fact->Get_Height() == 3);

    BuildingTypeClass* powr = Building_Type(BUILDING_POWR);
    TEST("POWR exists", powr != nullptr);
    TEST("POWR produces power", powr->Get_Power() > 0);
    TEST("POWR is power plant", powr->Is_Power_Plant());

    printf("\n=== All Type Classes tests passed! ===\n");
    return 0;
}
EOF

if ! clang++ -std=c++17 -I include /tmp/test_types.cpp src/game/types/types.cpp \
    -o /tmp/test_types 2>&1; then
    echo "VERIFY_FAILED: Test compilation failed"
    rm -f /tmp/types.o /tmp/test_types.cpp
    exit 1
fi
echo "  OK: Test built"

# Step 4: Run test
echo "[4/4] Running types tests..."
if ! /tmp/test_types; then
    echo "VERIFY_FAILED: Types tests failed"
    rm -f /tmp/types.o /tmp/test_types.cpp /tmp/test_types
    exit 1
fi

rm -f /tmp/types.o /tmp/test_types.cpp /tmp/test_types

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Create directory: `mkdir -p include/game/types src/game/types`
2. Create all header files
3. Create types.cpp with static data
4. Run verification script

## Success Criteria
- Type hierarchy compiles correctly
- Unit types have correct stats
- Building types have correct properties
- Name lookup functions work
- Prerequisite system works

## Common Issues

### Static initialization order
- Use lambda initialization for complex types
- Avoid cross-type dependencies in init

### Missing weapon definitions
- Include weapon.h before type headers

### Name lookup returns wrong type
- Check array indexing matches enum values

## Completion Promise
When verification passes, output:
```
<promise>TASK_14H_COMPLETE</promise>
```

## Escape Hatch
If stuck after 12 iterations:
- Document what's blocking in `ralph-tasks/blocked/14h.md`
- Output: `<promise>TASK_14H_BLOCKED</promise>`

## Max Iterations
12
