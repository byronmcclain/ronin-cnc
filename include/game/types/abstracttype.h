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
