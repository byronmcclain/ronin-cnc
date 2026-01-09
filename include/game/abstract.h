/**
 * AbstractClass - Root Base Class
 *
 * Provides runtime type identification (RTTI) for all game objects.
 * This custom RTTI system is faster than C++ RTTI for frequent type checks.
 *
 * Original location: CODE/ABSTRACT.H
 */

#pragma once

#include "game/core/rtti.h"
#include "game/coord.h"
#include <cstdint>

// =============================================================================
// AbstractClass
// =============================================================================

/**
 * AbstractClass - Base class for all game objects
 *
 * Provides:
 * - Runtime type identification (RTTI)
 * - Base coordinate for spatial queries
 * - Virtual destructor for proper cleanup
 */
class AbstractClass {
public:
    // -------------------------------------------------------------------------
    // Construction / Destruction
    // -------------------------------------------------------------------------

    AbstractClass() : rtti_type_(RTTI_NONE), heap_id_(-1), is_active_(true) {}
    virtual ~AbstractClass() = default;

    // Non-copyable (objects have unique identity)
    AbstractClass(const AbstractClass&) = delete;
    AbstractClass& operator=(const AbstractClass&) = delete;

    // -------------------------------------------------------------------------
    // Runtime Type Identification
    // -------------------------------------------------------------------------

    /**
     * What_Am_I - Get runtime type
     *
     * Used for fast type checking without C++ RTTI overhead.
     * Derived classes set rtti_type_ in constructor.
     */
    RTTIType What_Am_I() const { return rtti_type_; }

    /**
     * Type checking helpers
     */
    bool Is_Techno() const;
    bool Is_Foot() const;
    bool Is_Building() const;
    bool Is_Infantry() const;
    bool Is_Unit() const;
    bool Is_Aircraft() const;
    bool Is_Bullet() const;
    bool Is_Anim() const;
    bool Is_Terrain() const;

    // -------------------------------------------------------------------------
    // Coordinate Access
    // -------------------------------------------------------------------------

    /**
     * Get_Coord - Get object's world coordinate
     *
     * Pure virtual - all objects must have a position.
     */
    virtual COORDINATE Get_Coord() const = 0;

    /**
     * Get_Cell - Get cell containing object
     */
    CELL Get_Cell() const { return Coord_Cell(Get_Coord()); }

    // -------------------------------------------------------------------------
    // Active State
    // -------------------------------------------------------------------------

    /**
     * Is object active (alive and in play)?
     */
    bool Is_Active() const { return is_active_; }

    /**
     * Deactivate object (mark for removal)
     */
    virtual void Deactivate() { is_active_ = false; }

    // -------------------------------------------------------------------------
    // Heap Management
    // -------------------------------------------------------------------------

    /**
     * Get/set heap ID (for object pool management)
     */
    int Get_Heap_ID() const { return heap_id_; }
    void Set_Heap_ID(int id) { heap_id_ = id; }

protected:
    RTTIType rtti_type_;    // Runtime type identifier
    int heap_id_;           // Index in object pool
    bool is_active_;        // Is this object alive?
};

// =============================================================================
// Type Checking Inline Implementations
// =============================================================================

inline bool AbstractClass::Is_Techno() const {
    return (rtti_type_ >= RTTI_UNIT && rtti_type_ <= RTTI_BUILDING);
}

inline bool AbstractClass::Is_Foot() const {
    return (rtti_type_ >= RTTI_UNIT && rtti_type_ <= RTTI_AIRCRAFT);
}

inline bool AbstractClass::Is_Building() const {
    return (rtti_type_ == RTTI_BUILDING);
}

inline bool AbstractClass::Is_Infantry() const {
    return (rtti_type_ == RTTI_INFANTRY);
}

inline bool AbstractClass::Is_Unit() const {
    return (rtti_type_ == RTTI_UNIT);
}

inline bool AbstractClass::Is_Aircraft() const {
    return (rtti_type_ == RTTI_AIRCRAFT);
}

inline bool AbstractClass::Is_Bullet() const {
    return (rtti_type_ == RTTI_BULLET);
}

inline bool AbstractClass::Is_Anim() const {
    return (rtti_type_ == RTTI_ANIM);
}

inline bool AbstractClass::Is_Terrain() const {
    return (rtti_type_ == RTTI_TERRAIN);
}
