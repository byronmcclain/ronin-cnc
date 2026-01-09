/**
 * CellClass Implementation
 *
 * Map cell data management.
 */

#include "game/cell.h"
#include <cstring>

// =============================================================================
// Land Type Data
// =============================================================================

const char* LandTypeNames[LAND_COUNT] = {
    "Clear",
    "Road",
    "Water",
    "Rock",
    "Wall",
    "Tiberium",
    "Beach",
    "Rough",
    "River"
};

// Speed multiplier percentage per land type
const int LandSpeedMultiplier[LAND_COUNT] = {
    100,    // LAND_CLEAR - normal
    150,    // LAND_ROAD - speed bonus
    0,      // LAND_WATER - naval only
    0,      // LAND_ROCK - impassable
    0,      // LAND_WALL - blocked
    80,     // LAND_TIBERIUM - slightly slow
    70,     // LAND_BEACH - slow
    50,     // LAND_ROUGH - very slow
    0       // LAND_RIVER - impassable
};

// =============================================================================
// Construction
// =============================================================================

CellClass::CellClass()
    : cell_index_(0)
    , template_type_(TEMPLATE_NONE)
    , template_icon_(0)
    , overlay_type_(OVERLAY_NONE_TYPE)
    , overlay_data_(0)
    , land_type_(LAND_CLEAR)
    , visibility_(CELL_SHROUD)
    , is_bridge_(false)
    , is_waypoint_(false)
    , is_flag_(false)
{
    memset(objects_, 0, sizeof(objects_));
}

void CellClass::Clear() {
    template_type_ = TEMPLATE_NONE;
    template_icon_ = 0;
    overlay_type_ = OVERLAY_NONE_TYPE;
    overlay_data_ = 0;
    land_type_ = LAND_CLEAR;
    visibility_ = CELL_SHROUD;
    is_bridge_ = false;
    is_waypoint_ = false;
    is_flag_ = false;
    memset(objects_, 0, sizeof(objects_));
}

// =============================================================================
// Terrain
// =============================================================================

void CellClass::Set_Template(uint8_t template_type, uint8_t icon) {
    template_type_ = template_type;
    template_icon_ = icon;
    Recalc_Land();
}

// =============================================================================
// Overlay
// =============================================================================

void CellClass::Set_Overlay(OverlayType type, uint8_t data) {
    overlay_type_ = type;
    overlay_data_ = data;
    Recalc_Land();
}

bool CellClass::Is_Tiberium() const {
    return (overlay_type_ >= OVERLAY_GOLD1 && overlay_type_ <= OVERLAY_GEMS4);
}

bool CellClass::Is_Wall() const {
    return (overlay_type_ >= OVERLAY_SANDBAG && overlay_type_ <= OVERLAY_WOOD);
}

int CellClass::Get_Tiberium_Value() const {
    if (!Is_Tiberium()) {
        return 0;
    }

    // Base value depends on overlay type
    bool is_gems = (overlay_type_ >= OVERLAY_GEMS1);
    int base_value = is_gems ? 50 : 25;  // Gems worth more

    // Stage multiplier
    int stage = (overlay_type_ - (is_gems ? OVERLAY_GEMS1 : OVERLAY_GOLD1)) + 1;

    return base_value * stage;
}

void CellClass::Reduce_Tiberium(int amount) {
    if (!Is_Tiberium()) {
        return;
    }

    // Reduce overlay data (represents remaining tiberium)
    if (overlay_data_ > amount) {
        overlay_data_ -= (uint8_t)amount;
    } else {
        // Depleted - remove overlay
        overlay_type_ = OVERLAY_NONE_TYPE;
        overlay_data_ = 0;
        Recalc_Land();
    }
}

// =============================================================================
// Land Type
// =============================================================================

void CellClass::Recalc_Land() {
    // Default to clear
    land_type_ = LAND_CLEAR;

    // Check overlay first (takes precedence)
    if (Is_Tiberium()) {
        land_type_ = LAND_TIBERIUM;
        return;
    }

    if (Is_Wall()) {
        land_type_ = LAND_WALL;
        return;
    }

    // Check terrain template
    // (In full implementation, this would look up template land type)
    // For now, use simple heuristics based on template
    if (template_type_ == TEMPLATE_NONE || template_type_ == TEMPLATE_CLEAR) {
        land_type_ = LAND_CLEAR;
    }
    // Template-based land type lookup would go here
    // Different templates map to water, rock, road, etc.
}

bool CellClass::Is_Passable(bool is_naval, bool is_infantry) const {
    if (is_naval) {
        // Naval units can only go on water
        return (land_type_ == LAND_WATER);
    }

    // Ground units
    switch (land_type_) {
        case LAND_WATER:
        case LAND_ROCK:
        case LAND_RIVER:
            return false;

        case LAND_WALL:
            // Infantry can pass some walls
            return is_infantry;

        default:
            return true;
    }
}

int CellClass::Get_Speed_Multiplier() const {
    if (land_type_ < 0 || land_type_ >= LAND_COUNT) {
        return 100;
    }
    return LandSpeedMultiplier[land_type_];
}

void CellClass::Recalc() {
    Recalc_Land();
}

// =============================================================================
// Visibility
// =============================================================================

void CellClass::Reveal(bool make_visible) {
    if (make_visible) {
        visibility_ = CELL_VISIBLE;
    } else if (visibility_ == CELL_SHROUD) {
        visibility_ = CELL_EXPLORED;
    }
}

void CellClass::Shroud() {
    if (visibility_ == CELL_VISIBLE) {
        visibility_ = CELL_EXPLORED;  // Was visible, now fogged
    }
}

// =============================================================================
// Occupancy
// =============================================================================

bool CellClass::Add_Object(ObjectClass* obj) {
    if (obj == nullptr) {
        return false;
    }

    // Find empty slot
    for (int i = 0; i < CELL_MAX_OBJECTS; i++) {
        if (objects_[i] == nullptr) {
            objects_[i] = obj;
            return true;
        }
    }

    return false;  // Cell full
}

void CellClass::Remove_Object(ObjectClass* obj) {
    if (obj == nullptr) {
        return;
    }

    for (int i = 0; i < CELL_MAX_OBJECTS; i++) {
        if (objects_[i] == obj) {
            // Shift remaining objects
            for (int j = i; j < CELL_MAX_OBJECTS - 1; j++) {
                objects_[j] = objects_[j + 1];
            }
            objects_[CELL_MAX_OBJECTS - 1] = nullptr;
            return;
        }
    }
}

ObjectClass* CellClass::Get_Object(int index) const {
    if (index < 0 || index >= CELL_MAX_OBJECTS) {
        return nullptr;
    }
    return objects_[index];
}

int CellClass::Object_Count() const {
    int count = 0;
    for (int i = 0; i < CELL_MAX_OBJECTS; i++) {
        if (objects_[i] != nullptr) {
            count++;
        }
    }
    return count;
}

TechnoClass* CellClass::Find_Techno() const {
    // This would check each object's RTTI and return first techno
    // For now, return nullptr (requires ObjectClass implementation)
    return nullptr;
}
