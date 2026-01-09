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
         include/game/mission.h include/game/core/rtti.h src/game/object/object.cpp; do
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
