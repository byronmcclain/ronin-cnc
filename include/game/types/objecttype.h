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
