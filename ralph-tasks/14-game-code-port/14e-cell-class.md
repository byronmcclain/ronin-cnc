# Task 14e: CellClass Implementation

## Dependencies
- Task 14d (MapClass Foundation) must be complete
- Task 14b (Coordinate Types) must be complete

## Context
CellClass represents a single 24x24 pixel terrain cell in the game map. It stores terrain type, overlay data, visibility state, and references to objects occupying the cell. This is one of the most frequently accessed classes in the game.

## Objective
Implement CellClass with:
1. Terrain template and icon storage
2. Overlay (tiberium, walls) data
3. Visibility/fog of war state
4. Land type classification
5. Object occupancy tracking
6. Basic draw preparation (not actual rendering)

## Technical Background

### Original CellClass (from CODE/CELL.H)
Each cell stores:
- Template type and icon index (terrain graphics)
- Overlay type and data (tiberium amount, wall type)
- Land type (derived from template - affects passability)
- Visibility state (shroud, explored)
- Object pointers (units/buildings in cell)

### Cell Graphics
Cells are drawn in two passes:
1. Terrain template (ground texture from TEMPERAT.MIX, etc.)
2. Overlay (tiberium crystals, walls, etc.)

### Land Types
Each cell has a derived land type that affects:
- Unit passability (tanks can't go on water)
- Movement speed (roads faster than rough)
- Resource harvesting (tiberium cells)

## Deliverables
- [ ] `include/game/cell.h` - Full CellClass header
- [ ] `src/game/map/cell.cpp` - CellClass implementation
- [ ] Land type derivation
- [ ] Visibility state management
- [ ] Object occupancy stubs
- [ ] Verification script passes

## Files to Create

### include/game/cell.h (NEW)
```cpp
/**
 * CellClass - Map Cell Data
 *
 * Represents a single 24x24 pixel terrain cell. Stores terrain type,
 * overlays, visibility, and occupancy information.
 *
 * Original location: CODE/CELL.H, CODE/CELL.CPP
 */

#pragma once

#include "game/coord.h"
#include <cstdint>

// Forward declarations
class ObjectClass;
class TechnoClass;

// =============================================================================
// Cell Constants
// =============================================================================

// Maximum objects that can occupy a single cell
constexpr int CELL_MAX_OBJECTS = 4;

// Special template values
constexpr uint8_t TEMPLATE_NONE = 0xFF;
constexpr uint8_t TEMPLATE_CLEAR = 0;

// Special overlay values
constexpr uint8_t OVERLAY_NONE = 0xFF;

// =============================================================================
// Land Types
// =============================================================================

/**
 * LandType - Terrain classification affecting movement and passability
 */
enum LandType : int8_t {
    LAND_CLEAR = 0,      // Open ground - all units
    LAND_ROAD = 1,       // Road - speed bonus
    LAND_WATER = 2,      // Water - naval only
    LAND_ROCK = 3,       // Impassable rock
    LAND_WALL = 4,       // Wall structure
    LAND_TIBERIUM = 5,   // Tiberium field
    LAND_BEACH = 6,      // Beach transition
    LAND_ROUGH = 7,      // Rough terrain - slow
    LAND_RIVER = 8,      // River (impassable)

    LAND_COUNT = 9
};

// Speed multipliers per land type (percentage)
extern const int LandSpeedMultiplier[LAND_COUNT];

// =============================================================================
// Overlay Types
// =============================================================================

/**
 * OverlayType - Objects placed on top of terrain
 */
enum OverlayType : int8_t {
    OVERLAY_NONE_TYPE = -1,

    // Resources
    OVERLAY_GOLD1 = 0,      // Gold/Ore stage 1
    OVERLAY_GOLD2 = 1,
    OVERLAY_GOLD3 = 2,
    OVERLAY_GOLD4 = 3,      // Full gold
    OVERLAY_GEMS1 = 4,      // Gems stage 1
    OVERLAY_GEMS2 = 5,
    OVERLAY_GEMS3 = 6,
    OVERLAY_GEMS4 = 7,      // Full gems

    // Walls
    OVERLAY_SANDBAG = 8,    // Sandbag wall
    OVERLAY_CYCLONE = 9,    // Chain link fence
    OVERLAY_BRICK = 10,     // Concrete wall
    OVERLAY_BARBWIRE = 11,  // Barbed wire
    OVERLAY_WOOD = 12,      // Wooden fence

    // Decorative
    OVERLAY_CRATE = 13,     // Bonus crate
    OVERLAY_V12 = 14,       // Haystacks
    OVERLAY_V13 = 15,       // Hay bales

    OVERLAY_COUNT = 16
};

// =============================================================================
// Visibility State
// =============================================================================

/**
 * Cell visibility flags (per-player in multiplayer)
 */
enum CellVisibility : uint8_t {
    CELL_SHROUD = 0,        // Never seen - black
    CELL_EXPLORED = 1,      // Seen before - fog
    CELL_VISIBLE = 2        // Currently visible - clear
};

// =============================================================================
// CellClass
// =============================================================================

/**
 * CellClass - Single map cell
 *
 * Stores all data for one 24x24 pixel terrain cell including:
 * - Terrain template and icon
 * - Overlay (resources, walls)
 * - Land type (derived from terrain)
 * - Visibility state
 * - Occupying objects
 */
class CellClass {
public:
    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------

    CellClass();
    ~CellClass() = default;

    // -------------------------------------------------------------------------
    // Initialization
    // -------------------------------------------------------------------------

    /**
     * Clear - Reset cell to default state
     */
    void Clear();

    /**
     * Set_Cell_Index - Set this cell's position
     */
    void Set_Cell_Index(CELL cell) { cell_index_ = cell; }

    /**
     * Get_Cell_Index - Get this cell's position
     */
    CELL Get_Cell_Index() const { return cell_index_; }

    /**
     * Get_Coord - Get center coordinate of this cell
     */
    COORDINATE Get_Coord() const { return Cell_Coord(cell_index_); }

    // -------------------------------------------------------------------------
    // Terrain
    // -------------------------------------------------------------------------

    /**
     * Set terrain template and icon
     *
     * @param template_type Template index (from TEMPERAT.MIX, etc.)
     * @param icon Icon within template
     */
    void Set_Template(uint8_t template_type, uint8_t icon = 0);

    uint8_t Get_Template() const { return template_type_; }
    uint8_t Get_Icon() const { return template_icon_; }

    /**
     * Has valid terrain template?
     */
    bool Has_Template() const { return template_type_ != TEMPLATE_NONE; }

    // -------------------------------------------------------------------------
    // Overlay
    // -------------------------------------------------------------------------

    /**
     * Set overlay type and data
     *
     * @param type Overlay type (OVERLAY_GOLD1, etc.)
     * @param data Overlay-specific data (wall health, tiberium stage)
     */
    void Set_Overlay(OverlayType type, uint8_t data = 0);

    OverlayType Get_Overlay() const { return overlay_type_; }
    uint8_t Get_Overlay_Data() const { return overlay_data_; }

    /**
     * Has overlay?
     */
    bool Has_Overlay() const { return overlay_type_ != OVERLAY_NONE_TYPE; }

    /**
     * Is tiberium?
     */
    bool Is_Tiberium() const;

    /**
     * Is wall?
     */
    bool Is_Wall() const;

    /**
     * Get tiberium value (for harvesting)
     */
    int Get_Tiberium_Value() const;

    /**
     * Reduce tiberium (after harvesting)
     */
    void Reduce_Tiberium(int amount);

    // -------------------------------------------------------------------------
    // Land Type
    // -------------------------------------------------------------------------

    /**
     * Get land type (derived from terrain/overlay)
     */
    LandType Get_Land() const { return land_type_; }

    /**
     * Recalculate land type from terrain and overlay
     */
    void Recalc_Land();

    /**
     * Can unit type pass this cell?
     *
     * @param is_naval True for ships/subs
     * @param is_infantry True for infantry (can walk more places)
     */
    bool Is_Passable(bool is_naval = false, bool is_infantry = false) const;

    /**
     * Get movement speed multiplier (100 = normal)
     */
    int Get_Speed_Multiplier() const;

    // -------------------------------------------------------------------------
    // Visibility
    // -------------------------------------------------------------------------

    /**
     * Get/set visibility state
     */
    CellVisibility Get_Visibility() const { return visibility_; }
    void Set_Visibility(CellVisibility vis) { visibility_ = vis; }

    bool Is_Shrouded() const { return visibility_ == CELL_SHROUD; }
    bool Is_Explored() const { return visibility_ >= CELL_EXPLORED; }
    bool Is_Visible() const { return visibility_ == CELL_VISIBLE; }

    /**
     * Reveal this cell (mark as explored or visible)
     */
    void Reveal(bool make_visible = true);

    /**
     * Shroud this cell (return to fog)
     */
    void Shroud();

    // -------------------------------------------------------------------------
    // Occupancy
    // -------------------------------------------------------------------------

    /**
     * Add object to this cell
     *
     * @return true if added, false if cell full
     */
    bool Add_Object(ObjectClass* obj);

    /**
     * Remove object from this cell
     */
    void Remove_Object(ObjectClass* obj);

    /**
     * Get object at index (0-3)
     */
    ObjectClass* Get_Object(int index) const;

    /**
     * Get first object in cell
     */
    ObjectClass* Get_First_Object() const { return Get_Object(0); }

    /**
     * Count objects in cell
     */
    int Object_Count() const;

    /**
     * Is cell occupied?
     */
    bool Is_Occupied() const { return objects_[0] != nullptr; }

    /**
     * Find techno (unit/building) in cell
     */
    TechnoClass* Find_Techno() const;

    // -------------------------------------------------------------------------
    // Flags
    // -------------------------------------------------------------------------

    /**
     * Various cell flags
     */
    bool Is_Bridge() const { return is_bridge_; }
    void Set_Bridge(bool val) { is_bridge_ = val; }

    bool Is_Waypoint() const { return is_waypoint_; }
    void Set_Waypoint(bool val) { is_waypoint_ = val; }

    bool Is_Flag() const { return is_flag_; }
    void Set_Flag(bool val) { is_flag_ = val; }

    // -------------------------------------------------------------------------
    // Recalculation
    // -------------------------------------------------------------------------

    /**
     * Recalc - Recalculate derived values
     *
     * Call after changing terrain or overlay to update land type.
     */
    void Recalc();

private:
    // -------------------------------------------------------------------------
    // Data Members
    // -------------------------------------------------------------------------

    // Position
    CELL cell_index_;           // This cell's position (y*128+x)

    // Terrain
    uint8_t template_type_;     // Terrain template index
    uint8_t template_icon_;     // Icon within template

    // Overlay
    OverlayType overlay_type_;  // Overlay type
    uint8_t overlay_data_;      // Overlay-specific data

    // Derived
    LandType land_type_;        // Calculated land type

    // Visibility
    CellVisibility visibility_; // Fog of war state

    // Flags
    bool is_bridge_;            // Bridge overlay
    bool is_waypoint_;          // Waypoint marker
    bool is_flag_;              // Capture the flag

    // Occupancy
    ObjectClass* objects_[CELL_MAX_OBJECTS]; // Objects in this cell
};

// =============================================================================
// Land Type Names
// =============================================================================

extern const char* LandTypeNames[LAND_COUNT];
```

### src/game/map/cell.cpp (NEW)
```cpp
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
        overlay_data_ -= amount;
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
```

## Verification Command
```bash
ralph-tasks/verify/check-cell-class.sh
```

## Verification Script

### ralph-tasks/verify/check-cell-class.sh (NEW)
```bash
#!/bin/bash
# Verification script for CellClass implementation

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking CellClass Implementation ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check files exist
echo "[1/4] Checking files..."
check_file() {
    if [ ! -f "$1" ]; then
        echo "VERIFY_FAILED: $1 not found"
        exit 1
    fi
    echo "  OK: $1"
}

check_file "include/game/cell.h"
check_file "src/game/map/cell.cpp"

# Step 2: Syntax check
echo "[2/4] Checking syntax..."
if ! clang++ -std=c++17 -fsyntax-only -I include include/game/cell.h 2>&1; then
    echo "VERIFY_FAILED: cell.h has syntax errors"
    exit 1
fi

if ! clang++ -std=c++17 -c -I include src/game/map/cell.cpp -o /tmp/cell.o 2>&1; then
    echo "VERIFY_FAILED: cell.cpp compilation failed"
    exit 1
fi
echo "  OK: Syntax valid"

# Step 3: Build test
echo "[3/4] Building cell test..."
cat > /tmp/test_cell.cpp << 'EOF'
#include "game/cell.h"
#include <cstdio>

#define TEST(name, cond) do { \
    if (!(cond)) { printf("FAIL: %s\n", name); return 1; } \
    printf("PASS: %s\n", name); \
} while(0)

int main() {
    printf("=== CellClass Tests ===\n\n");

    CellClass cell;
    TEST("Cell created", true);

    // Test clear
    cell.Clear();
    TEST("Cell cleared", cell.Get_Template() == 0xFF);

    // Test cell index
    CELL idx = XY_Cell(50, 50);
    cell.Set_Cell_Index(idx);
    TEST("Cell index set", cell.Get_Cell_Index() == idx);

    // Test template
    cell.Set_Template(5, 3);
    TEST("Template set", cell.Get_Template() == 5);
    TEST("Icon set", cell.Get_Icon() == 3);
    TEST("Has template", cell.Has_Template());

    // Test overlay
    cell.Set_Overlay(OVERLAY_GOLD2, 10);
    TEST("Overlay set", cell.Get_Overlay() == OVERLAY_GOLD2);
    TEST("Has overlay", cell.Has_Overlay());
    TEST("Is tiberium", cell.Is_Tiberium());
    TEST("Not wall", !cell.Is_Wall());

    // Test tiberium value
    int value = cell.Get_Tiberium_Value();
    TEST("Tiberium has value", value > 0);

    // Test wall
    cell.Set_Overlay(OVERLAY_BRICK, 0);
    TEST("Is wall", cell.Is_Wall());
    TEST("Not tiberium", !cell.Is_Tiberium());

    // Test land type
    cell.Clear();
    cell.Set_Template(TEMPLATE_CLEAR, 0);
    TEST("Land is clear", cell.Get_Land() == LAND_CLEAR);
    TEST("Is passable ground", cell.Is_Passable(false, false));

    // Test visibility
    TEST("Initially shrouded", cell.Is_Shrouded());
    cell.Reveal(false);
    TEST("Explored after reveal", cell.Is_Explored());
    cell.Reveal(true);
    TEST("Visible after full reveal", cell.Is_Visible());
    cell.Shroud();
    TEST("Back to explored", !cell.Is_Visible() && cell.Is_Explored());

    // Test speed multiplier
    int speed = cell.Get_Speed_Multiplier();
    TEST("Has speed multiplier", speed > 0);

    // Test land names exist
    TEST("Land names exist", LandTypeNames[LAND_CLEAR] != nullptr);

    printf("\n=== All CellClass tests passed! ===\n");
    return 0;
}
EOF

if ! clang++ -std=c++17 -I include /tmp/test_cell.cpp src/game/core/coord.cpp src/game/map/cell.cpp \
    -o /tmp/test_cell 2>&1; then
    echo "VERIFY_FAILED: Test compilation failed"
    rm -f /tmp/cell.o /tmp/test_cell.cpp
    exit 1
fi
echo "  OK: Test built"

# Step 4: Run test
echo "[4/4] Running cell tests..."
if ! /tmp/test_cell; then
    echo "VERIFY_FAILED: Cell tests failed"
    rm -f /tmp/cell.o /tmp/test_cell.cpp /tmp/test_cell
    exit 1
fi

rm -f /tmp/cell.o /tmp/test_cell.cpp /tmp/test_cell

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Create directory: `mkdir -p src/game/map`
2. Create `include/game/cell.h`
3. Create `src/game/map/cell.cpp`
4. Run verification script

## Success Criteria
- CellClass stores template and overlay data
- Land type derivation works
- Visibility state management works
- Object occupancy tracking works
- Speed multipliers are correct

## Common Issues

### "ObjectClass incomplete type"
- Forward declare in header
- Don't call methods on ObjectClass yet

### Land type always CLEAR
- Recalc_Land() needs template lookup table
- For now, use simple heuristics

### Tiberium value wrong
- Check overlay type range
- Gems vs gold have different values

## Completion Promise
When verification passes, output:
```
<promise>TASK_14E_COMPLETE</promise>
```

## Escape Hatch
If stuck after 12 iterations:
- Document what's blocking in `ralph-tasks/blocked/14e.md`
- Output: `<promise>TASK_14E_BLOCKED</promise>`

## Max Iterations
12
