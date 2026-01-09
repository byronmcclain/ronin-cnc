# Task 14g: ObjectClass System

## Dependencies
- Task 14b (Coordinate Types) must be complete
- Task 14e (CellClass) must be complete

## Context
ObjectClass is the base class for all game entities - units, buildings, infantry, aircraft, terrain objects, overlays, and animations. It provides the common interface for position, health, drawing, AI updates, and damage handling.

## Objective
Implement the ObjectClass hierarchy foundation:
1. AbstractClass base with RTTI
2. ObjectClass with position, health, selection
3. TechnoClass stub for military units
4. Mission system stub
5. Object list management

## Technical Background

### Original Class Hierarchy (from CODE/FUNCTION.H)
```
AbstractClass (RTTI)
    |
    +-- ObjectClass (position, health, draw)
        |
        +-- MissionClass (AI orders)
            |
            +-- RadioClass (communication)
                |
                +-- TechnoClass (military units)
                    |
                    +-- FootClass (mobile units)
                    |   |
                    |   +-- DriveClass (wheeled/tracked)
                    |   |   +-- UnitClass
                    |   |
                    |   +-- InfantryClass
                    |   +-- AircraftClass
                    |
                    +-- BuildingClass
```

### RTTI System
The original uses a custom RTTI (Runtime Type Identification) system instead of C++ RTTI:
```cpp
virtual RTTIType What_Am_I() const = 0;
```

This allows fast type checking and casting without dynamic_cast overhead.

### Object Lists
All active objects are tracked in global lists for iteration:
- AllObjects - all game objects
- Units, Infantry, Buildings, Aircraft - type-specific lists

## Deliverables
- [ ] `include/game/abstract.h` - AbstractClass with RTTI
- [ ] `include/game/object.h` - ObjectClass base
- [ ] `include/game/techno.h` - TechnoClass stub
- [ ] `include/game/mission.h` - Mission enums and stubs
- [ ] `src/game/object/object.cpp` - ObjectClass implementation
- [ ] Object list management
- [ ] Verification script passes

## Files to Create

### include/game/abstract.h (NEW)
```cpp
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
```

### include/game/object.h (NEW)
```cpp
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
```

### include/game/techno.h (NEW)
```cpp
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
```

### include/game/mission.h (NEW)
```cpp
/**
 * Mission System
 *
 * Defines the mission/order types that units can execute.
 * Missions control high-level AI behavior.
 *
 * Original location: CODE/MISSION.H
 */

#pragma once

#include <cstdint>

// =============================================================================
// Mission Types
// =============================================================================

/**
 * MissionType - Unit AI orders
 *
 * Each unit has a current mission that drives its behavior.
 */
enum MissionType : int8_t {
    MISSION_NONE = -1,

    MISSION_SLEEP = 0,      // Do nothing
    MISSION_ATTACK = 1,     // Attack target
    MISSION_MOVE = 2,       // Move to destination
    MISSION_QMOVE = 3,      // Queue move (shift-click)
    MISSION_RETREAT = 4,    // Flee from combat
    MISSION_GUARD = 5,      // Guard position
    MISSION_STICKY = 6,     // Don't auto-acquire targets
    MISSION_ENTER = 7,      // Enter transport/building
    MISSION_CAPTURE = 8,    // Capture building (engineer)
    MISSION_HARVEST = 9,    // Harvest tiberium
    MISSION_GUARD_AREA = 10,// Guard area (patrol)
    MISSION_RETURN = 11,    // Return to base (harvester)
    MISSION_STOP = 12,      // Stop current action
    MISSION_AMBUSH = 13,    // Ambush (stay hidden)
    MISSION_HUNT = 14,      // Seek and destroy
    MISSION_UNLOAD = 15,    // Unload passengers
    MISSION_SABOTAGE = 16,  // Sabotage (spy/thief)
    MISSION_CONSTRUCTION = 17,// Building construction
    MISSION_DECONSTRUCTION = 18,// Building selling
    MISSION_REPAIR = 19,    // Repair building
    MISSION_RESCUE = 20,    // Rescue (medic)
    MISSION_MISSILE = 21,   // Missile launch

    MISSION_COUNT = 22
};

// =============================================================================
// Mission Names
// =============================================================================

extern const char* MissionNames[MISSION_COUNT];

// =============================================================================
// Mission Utility Functions
// =============================================================================

/**
 * Get mission from name string
 */
MissionType Mission_From_Name(const char* name);

/**
 * Get mission name string
 */
const char* Mission_Name(MissionType mission);

/**
 * Is this an attack mission?
 */
inline bool Mission_Is_Attack(MissionType m) {
    return (m == MISSION_ATTACK || m == MISSION_HUNT);
}

/**
 * Is this a movement mission?
 */
inline bool Mission_Is_Move(MissionType m) {
    return (m == MISSION_MOVE || m == MISSION_QMOVE ||
            m == MISSION_ENTER || m == MISSION_RETURN);
}
```

### src/game/object/object.cpp (NEW)
```cpp
/**
 * ObjectClass Implementation
 *
 * Base game object functionality.
 */

#include "game/object.h"
#include "game/cell.h"
#include "game/mission.h"
#include <cstring>
#include <algorithm>

// =============================================================================
// Global Object List
// =============================================================================

ObjectClass* AllObjects = nullptr;

// =============================================================================
// Mission Names
// =============================================================================

const char* MissionNames[MISSION_COUNT] = {
    "Sleep", "Attack", "Move", "QMove", "Retreat",
    "Guard", "Sticky", "Enter", "Capture", "Harvest",
    "Guard Area", "Return", "Stop", "Ambush", "Hunt",
    "Unload", "Sabotage", "Construction", "Deconstruction",
    "Repair", "Rescue", "Missile"
};

MissionType Mission_From_Name(const char* name) {
    if (name == nullptr) return MISSION_NONE;
    for (int i = 0; i < MISSION_COUNT; i++) {
        if (strcmp(MissionNames[i], name) == 0) {
            return (MissionType)i;
        }
    }
    return MISSION_NONE;
}

const char* Mission_Name(MissionType mission) {
    if (mission < 0 || mission >= MISSION_COUNT) return "Unknown";
    return MissionNames[mission];
}

// =============================================================================
// ObjectClass Construction
// =============================================================================

ObjectClass::ObjectClass()
    : AbstractClass()
    , coord_(COORD_NONE)
    , strength_(0)
    , owner_(HOUSE_NONE)
    , is_selected_(false)
    , next_(nullptr)
    , prev_(nullptr)
{
    rtti_type_ = RTTI_NONE;
}

ObjectClass::~ObjectClass() {
    // Remove from cell occupancy
    Unmark_Cell();

    // Remove from global list
    Unlink_From_List();
}

// =============================================================================
// Position
// =============================================================================

void ObjectClass::Set_Coord(COORDINATE coord) {
    if (coord_ != coord) {
        // Remove from old cell
        Unmark_Cell();

        // Update position
        coord_ = coord;

        // Add to new cell
        Mark_Cell();
    }
}

void ObjectClass::Set_Cell(CELL cell) {
    Set_Coord(Cell_Coord(cell));
}

void ObjectClass::Mark_Cell() {
    // Get cell at our position
    CELL cell = Get_Cell();
    if (cell == CELL_NONE) return;

    // This would add us to the cell's object list
    // Requires MapClass integration
    // For now, just a placeholder
}

void ObjectClass::Unmark_Cell() {
    // Get cell at our position
    CELL cell = Get_Cell();
    if (cell == CELL_NONE) return;

    // This would remove us from the cell's object list
    // Requires MapClass integration
}

// =============================================================================
// Health
// =============================================================================

int ObjectClass::Health_Percent() const {
    int max = Max_Strength();
    if (max <= 0) return 0;
    return (strength_ * 100) / max;
}

void ObjectClass::Set_Strength(int str) {
    strength_ = std::clamp(str, 0, Max_Strength());
}

void ObjectClass::Heal(int amount) {
    if (amount > 0) {
        Set_Strength(strength_ + amount);
    }
}

// =============================================================================
// Drawing
// =============================================================================

void ObjectClass::Draw(int x, int y, void* window) {
    (void)window;

    // Base implementation draws nothing
    // Derived classes override to draw their graphics

    if (is_selected_) {
        Draw_Selection(x, y);
    }
}

void ObjectClass::Draw_Selection(int x, int y) {
    // Draw selection box corners
    // This would use GScreenClass drawing methods
    // For now, just a placeholder
    (void)x;
    (void)y;
}

void ObjectClass::Draw_Health(int x, int y) {
    // Draw health bar above object
    // This would use GScreenClass drawing methods
    (void)x;
    (void)y;
}

// =============================================================================
// AI
// =============================================================================

void ObjectClass::AI() {
    // Base implementation does nothing
    // Derived classes override for behavior
}

void ObjectClass::Per_Cell_Process(bool from_center) {
    (void)from_center;
    // Called when object enters a new cell
}

// =============================================================================
// Damage
// =============================================================================

int ObjectClass::Take_Damage(int damage, ObjectClass* source, int warhead) {
    (void)source;
    (void)warhead;

    if (damage <= 0 || strength_ <= 0) {
        return 0;
    }

    int actual = std::min(damage, strength_);
    strength_ -= actual;

    if (strength_ <= 0) {
        Destroyed();
    }

    return actual;
}

void ObjectClass::Destroyed() {
    // Mark as inactive
    is_active_ = false;

    // Deselect
    is_selected_ = false;

    // Remove from cell
    Unmark_Cell();
}

// =============================================================================
// Object List
// =============================================================================

void ObjectClass::Link_To_List() {
    // Add to front of global list
    next_ = AllObjects;
    prev_ = nullptr;

    if (AllObjects != nullptr) {
        AllObjects->prev_ = this;
    }

    AllObjects = this;
}

void ObjectClass::Unlink_From_List() {
    // Remove from list
    if (prev_ != nullptr) {
        prev_->next_ = next_;
    } else if (AllObjects == this) {
        AllObjects = next_;
    }

    if (next_ != nullptr) {
        next_->prev_ = prev_;
    }

    next_ = nullptr;
    prev_ = nullptr;
}

// =============================================================================
// TechnoClass Implementation
// =============================================================================

TechnoClass::TechnoClass()
    : ObjectClass()
    , body_facing_(DIR_N)
    , turret_facing_(DIR_N)
    , armor_(ARMOR_NONE)
    , primary_weapon_(WEAPON_NONE)
    , secondary_weapon_(WEAPON_NONE)
    , target_(nullptr)
    , is_cloaked_(false)
{
}

TechnoClass::~TechnoClass() {
    target_ = nullptr;
}

bool TechnoClass::Can_Attack(ObjectClass* target) const {
    if (target == nullptr || !target->Is_Active()) {
        return false;
    }

    if (primary_weapon_ == WEAPON_NONE && secondary_weapon_ == WEAPON_NONE) {
        return false;
    }

    // Additional checks would go here (range, line of sight, etc.)
    return true;
}

void TechnoClass::AI() {
    ObjectClass::AI();

    // Update facing towards target
    if (target_ != nullptr && target_->Is_Active()) {
        // Calculate direction to target
        DirType desired = Coord_Direction(coord_, target_->Get_Coord());
        body_facing_.Set_Desired(desired);

        if (Has_Turret()) {
            turret_facing_.Set_Desired(desired);
        }
    }

    // Rotate towards desired facing
    body_facing_.Rotate();
    if (Has_Turret()) {
        turret_facing_.Rotate();
    }
}

// =============================================================================
// FootClass Implementation
// =============================================================================

FootClass::FootClass()
    : TechnoClass()
    , speed_(0)
    , destination_(COORD_NONE)
{
}

FootClass::~FootClass() {
}
```

## Verification Command
```bash
ralph-tasks/verify/check-object-class.sh
```

## Verification Script

### ralph-tasks/verify/check-object-class.sh (NEW)
```bash
#!/bin/bash
# Verification script for ObjectClass system

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking ObjectClass System ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check files exist
echo "[1/4] Checking files..."
for f in include/game/abstract.h include/game/object.h include/game/techno.h \
         include/game/mission.h src/game/object/object.cpp; do
    if [ ! -f "$f" ]; then
        echo "VERIFY_FAILED: $f not found"
        exit 1
    fi
    echo "  OK: $f"
done

# Step 2: Syntax check
echo "[2/4] Checking syntax..."
if ! clang++ -std=c++17 -fsyntax-only -I include include/game/abstract.h 2>&1; then
    echo "VERIFY_FAILED: abstract.h has syntax errors"
    exit 1
fi

if ! clang++ -std=c++17 -c -I include src/game/object/object.cpp -o /tmp/object.o 2>&1; then
    echo "VERIFY_FAILED: object.cpp compilation failed"
    exit 1
fi
echo "  OK: Syntax valid"

# Step 3: Build test
echo "[3/4] Building object test..."
cat > /tmp/test_object.cpp << 'EOF'
#include "game/object.h"
#include "game/techno.h"
#include "game/mission.h"
#include <cstdio>

// Concrete test class
class TestUnit : public FootClass {
public:
    TestUnit() {
        rtti_type_ = RTTI_UNIT;
        strength_ = 100;
    }
    virtual int Max_Strength() const override { return 100; }
};

#define TEST(name, cond) do { \
    if (!(cond)) { printf("FAIL: %s\n", name); return 1; } \
    printf("PASS: %s\n", name); \
} while(0)

int main() {
    printf("=== ObjectClass Tests ===\n\n");

    // Test object creation
    TestUnit unit;
    TEST("Unit created", true);
    TEST("RTTI is UNIT", unit.What_Am_I() == RTTI_UNIT);
    TEST("Is Techno", unit.Is_Techno());
    TEST("Is Foot", unit.Is_Foot());
    TEST("Is Unit", unit.Is_Unit());
    TEST("Not Building", !unit.Is_Building());

    // Test position
    COORDINATE pos = XY_Coord(1000, 2000);
    unit.Set_Coord(pos);
    TEST("Coord set", unit.Get_Coord() == pos);
    TEST("Cell valid", unit.Get_Cell() != CELL_NONE);

    // Test health
    TEST("Initial health", unit.Get_Strength() == 100);
    TEST("Full health", unit.Is_Full_Health());
    unit.Take_Damage(30, nullptr, 0);
    TEST("Damage taken", unit.Get_Strength() == 70);
    TEST("Health percent", unit.Health_Percent() == 70);
    unit.Heal(10);
    TEST("Healed", unit.Get_Strength() == 80);

    // Test owner
    unit.Set_Owner(HOUSE_USSR);
    TEST("Owner set", unit.Get_Owner() == HOUSE_USSR);
    TEST("Is owned by", unit.Is_Owned_By(HOUSE_USSR));

    // Test selection
    TEST("Not selected", !unit.Is_Selected());
    unit.Select();
    TEST("Selected", unit.Is_Selected());
    unit.Deselect();
    TEST("Deselected", !unit.Is_Selected());

    // Test object list
    TEST("AllObjects empty initially", AllObjects == nullptr);
    unit.Link_To_List();
    TEST("Added to list", AllObjects == &unit);
    unit.Unlink_From_List();
    TEST("Removed from list", AllObjects == nullptr);

    // Test facing
    unit.Body_Facing().Set(DIR_E);
    TEST("Facing set", unit.Get_Facing() == DIR_E);

    // Test mission names
    TEST("Mission name", Mission_Name(MISSION_ATTACK) != nullptr);
    TEST("Mission from name", Mission_From_Name("Attack") == MISSION_ATTACK);

    // Test destruction
    unit.Set_Strength(10);
    unit.Take_Damage(100, nullptr, 0);
    TEST("Destroyed", unit.Is_Destroyed());
    TEST("Inactive", !unit.Is_Active());

    printf("\n=== All ObjectClass tests passed! ===\n");
    return 0;
}
EOF

if ! clang++ -std=c++17 -I include \
    /tmp/test_object.cpp \
    src/game/core/coord.cpp \
    src/game/object/object.cpp \
    -o /tmp/test_object 2>&1; then
    echo "VERIFY_FAILED: Test compilation failed"
    rm -f /tmp/object.o /tmp/test_object.cpp
    exit 1
fi
echo "  OK: Test built"

# Step 4: Run test
echo "[4/4] Running object tests..."
if ! /tmp/test_object; then
    echo "VERIFY_FAILED: Object tests failed"
    rm -f /tmp/object.o /tmp/test_object.cpp /tmp/test_object
    exit 1
fi

rm -f /tmp/object.o /tmp/test_object.cpp /tmp/test_object

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Create directory: `mkdir -p src/game/object`
2. Create all header files
3. Create implementation file
4. Run verification script

## Success Criteria
- RTTI system works for type identification
- Object position and health management works
- Object list links correctly
- TechnoClass facing and targeting works
- Mission system compiles and enums work

## Common Issues

### Circular includes
- Use forward declarations
- Include only in .cpp files

### Object list corruption
- Always unlink before destroying
- Check for nullptr

### RTTI values wrong
- Set rtti_type_ in derived class constructor

## Completion Promise
When verification passes, output:
```
<promise>TASK_14G_COMPLETE</promise>
```

## Escape Hatch
If stuck after 12 iterations:
- Document what's blocking in `ralph-tasks/blocked/14g.md`
- Output: `<promise>TASK_14G_BLOCKED</promise>`

## Max Iterations
12
