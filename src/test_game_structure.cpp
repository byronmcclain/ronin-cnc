/**
 * Game Structure Integration Test
 *
 * Tests that all game components compile and link correctly.
 */

#include "game/game.h"
#include "game/display.h"
#include "game/cell.h"
#include "game/object.h"
#include "game/techno.h"
#include "game/mission.h"
#include "game/types/unittype.h"
#include "game/types/buildingtype.h"
#include "platform.h"
#include <cstdio>
#include <cstring>

#define TEST(name, cond) do { \
    if (!(cond)) { \
        printf("FAIL: %s\n", name); \
        failures++; \
    } else { \
        printf("PASS: %s\n", name); \
        passes++; \
    } \
} while(0)

int main() {
    printf("==========================================\n");
    printf("Game Structure Integration Test\n");
    printf("==========================================\n\n");

    int passes = 0;
    int failures = 0;

    // Test coordinate system
    printf("--- Coordinate System ---\n");
    CELL cell = XY_Cell(64, 64);
    TEST("Cell creation", cell != CELL_NONE);
    TEST("Cell X", Cell_X(cell) == 64);
    TEST("Cell Y", Cell_Y(cell) == 64);

    COORDINATE coord = Cell_Coord(cell);
    TEST("Cell to Coord", coord != COORD_NONE);
    TEST("Coord to Cell", Coord_Cell(coord) == cell);

    // Test cell class
    printf("\n--- Cell Class ---\n");
    CellClass test_cell;
    test_cell.Clear();
    TEST("Cell clear", test_cell.Get_Template() == 0xFF);
    test_cell.Set_Template(5, 3);
    TEST("Cell template", test_cell.Get_Template() == 5);
    test_cell.Set_Overlay(OVERLAY_GOLD2, 10);
    TEST("Cell is tiberium", test_cell.Is_Tiberium());
    TEST("Cell tiberium value", test_cell.Get_Tiberium_Value() > 0);

    // Test type classes
    printf("\n--- Type Classes ---\n");
    UnitTypeClass* mtnk = Unit_Type(UNIT_MTNK);
    TEST("MTNK type exists", mtnk != nullptr);
    TEST("MTNK name", strcmp(mtnk->Get_Name(), "MTNK") == 0);
    TEST("MTNK cost", mtnk->Get_Cost() == 800);

    BuildingTypeClass* fact = Building_Type(BUILDING_FACT);
    TEST("FACT type exists", fact != nullptr);
    TEST("FACT is conyard", fact->Is_Construction_Yard());

    // Test object system
    printf("\n--- Object System ---\n");
    TEST("AllObjects initially null", AllObjects == nullptr);

    // Test facing
    printf("\n--- Facing System ---\n");
    FacingClass facing(DIR_N);
    TEST("Initial facing N", facing.Current() == DIR_N);
    facing.Set_Desired(DIR_E);
    TEST("Desired E", facing.Desired() == DIR_E);
    TEST("Not at target", !facing.Is_At_Target());

    // Test house
    printf("\n--- House System ---\n");
    TEST("USSR is Soviet", House_Side(HOUSE_USSR) == SIDE_SOVIET);
    TEST("GREECE is Allied", House_Side(HOUSE_GREECE) == SIDE_ALLIED);

    // Test mission
    printf("\n--- Mission System ---\n");
    TEST("Attack mission name", strcmp(Mission_Name(MISSION_ATTACK), "Attack") == 0);
    TEST("Mission from name", Mission_From_Name("Guard") == MISSION_GUARD);

    // Summary
    printf("\n==========================================\n");
    printf("Summary: %d passed, %d failed\n", passes, failures);
    printf("==========================================\n");

    if (failures > 0) {
        printf("\nSome tests failed. Review the output above.\n");
        return 1;
    }

    printf("\nAll integration tests passed!\n");
    printf("Game code structure is ready for Phase 15.\n");
    return 0;
}
