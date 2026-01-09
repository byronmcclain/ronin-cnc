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
#include <cstring>

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
