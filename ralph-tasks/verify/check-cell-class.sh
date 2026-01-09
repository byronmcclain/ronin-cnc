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
