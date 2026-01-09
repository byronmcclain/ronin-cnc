/**
 * ObjectClass Implementation
 *
 * Base game object functionality.
 */

#include "game/object.h"
#include "game/techno.h"
#include "game/cell.h"
#include "game/mission.h"
#include "game/core/rtti.h"
#include <cstring>
#include <algorithm>

// =============================================================================
// Global Object List
// =============================================================================

ObjectClass* AllObjects = nullptr;

// =============================================================================
// RTTI Names
// =============================================================================

const char* RTTINames[RTTI_COUNT] = {
    "Unit", "Infantry", "Vessel", "Aircraft", "Building",
    "Bullet", "Anim", "Terrain", "Overlay", "Smudge", "Trigger",
    "UnitType", "InfantryType", "VesselType", "AircraftType",
    "BuildingType", "BulletType", "AnimType", "TerrainType", "OverlayType"
};

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
