#!/bin/bash
# Verification script for coordinate type definitions

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Coordinate Type Definitions ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check header files exist
echo "[1/5] Checking header files..."
check_file() {
    if [ ! -f "$1" ]; then
        echo "VERIFY_FAILED: $1 not found"
        exit 1
    fi
    echo "  OK: $1"
}

check_file "include/game/coord.h"
check_file "include/game/facing.h"
check_file "include/game/house.h"
check_file "include/game/weapon.h"
check_file "src/game/core/coord.cpp"

# Step 2: Syntax check headers
echo "[2/5] Checking header syntax..."
for header in include/game/coord.h include/game/facing.h include/game/house.h include/game/weapon.h; do
    if ! clang++ -std=c++17 -fsyntax-only -I include "$header" 2>&1; then
        echo "VERIFY_FAILED: $header has syntax errors"
        exit 1
    fi
done
echo "  OK: All headers valid"

# Step 3: Compile implementation
echo "[3/5] Compiling coord.cpp..."
if ! clang++ -std=c++17 -c -I include src/game/core/coord.cpp -o /tmp/coord.o 2>&1; then
    echo "VERIFY_FAILED: coord.cpp compilation failed"
    exit 1
fi
echo "  OK: coord.cpp compiles"

# Step 4: Build test program
echo "[4/5] Building coordinate test..."
cat > /tmp/test_coord.cpp << 'EOF'
#include "game/coord.h"
#include "game/facing.h"
#include "game/house.h"
#include <cstdio>
#include <cstring>

#define TEST(name, cond) do { \
    if (!(cond)) { printf("FAIL: %s\n", name); return 1; } \
    printf("PASS: %s\n", name); \
} while(0)

int main() {
    printf("=== Coordinate Type Tests ===\n\n");

    // Test cell construction and extraction
    CELL cell = XY_Cell(50, 75);
    TEST("Cell X extraction", Cell_X(cell) == 50);
    TEST("Cell Y extraction", Cell_Y(cell) == 75);

    // Test cell to coord conversion
    COORDINATE coord = Cell_Coord(cell);
    TEST("Cell to Coord not null", coord != COORD_NONE);

    // Test coord back to cell
    CELL back = Coord_Cell(coord);
    TEST("Coord to Cell X", Cell_X(back) == 50);
    TEST("Coord to Cell Y", Cell_Y(back) == 75);

    // Test coordinate macros
    int px = Coord_XPixel(coord);
    int py = Coord_YPixel(coord);
    TEST("Coord X pixel in range", px > 0 && px < MAP_PIXEL_WIDTH);
    TEST("Coord Y pixel in range", py > 0 && py < MAP_PIXEL_HEIGHT);

    // Test boundary checks
    TEST("Cell in map - valid", Cell_InMap(XY_Cell(64, 64)) == 1);
    TEST("Cell in map - invalid", Cell_InMap(CELL_NONE) == 0);

    // Test adjacent cell
    CELL center = XY_Cell(64, 64);
    CELL north = Adjacent_Cell(center, FACING_N);
    TEST("Adjacent north Y", Cell_Y(north) == 63);
    TEST("Adjacent north X", Cell_X(north) == 64);

    // Test distance
    CELL c1 = XY_Cell(10, 10);
    CELL c2 = XY_Cell(20, 10);
    int dist = Cell_Distance(c1, c2);
    TEST("Cell distance", dist == 10);

    // Test pixel conversion
    // Note: Due to integer math (256/24 ratio), exact roundtrip is not possible
    // Allow +/- 1 pixel tolerance
    COORDINATE pcoord = Pixel_To_Coord(100, 200);
    int rx, ry;
    Coord_To_Pixel(pcoord, &rx, &ry);
    TEST("Pixel roundtrip X (within 1)", rx >= 99 && rx <= 101);
    TEST("Pixel roundtrip Y (within 1)", ry >= 199 && ry <= 201);

    // Test exact roundtrip at cell boundaries (multiples of 24)
    COORDINATE ccoord = Pixel_To_Coord(24, 48);
    int cx, cy;
    Coord_To_Pixel(ccoord, &cx, &cy);
    TEST("Pixel at cell boundary X", cx == 24);
    TEST("Pixel at cell boundary Y", cy == 48);

    printf("\n=== All coordinate tests passed! ===\n");
    return 0;
}
EOF

if ! clang++ -std=c++17 -I include /tmp/test_coord.cpp src/game/core/coord.cpp -o /tmp/test_coord 2>&1; then
    echo "VERIFY_FAILED: Test program compilation failed"
    rm -f /tmp/coord.o /tmp/test_coord.cpp
    exit 1
fi
echo "  OK: Test program built"

# Step 5: Run tests
echo "[5/5] Running coordinate tests..."
if ! /tmp/test_coord; then
    echo "VERIFY_FAILED: Coordinate tests failed"
    rm -f /tmp/coord.o /tmp/test_coord.cpp /tmp/test_coord
    exit 1
fi

# Cleanup
rm -f /tmp/coord.o /tmp/test_coord.cpp /tmp/test_coord

echo ""
echo "VERIFY_SUCCESS"
exit 0
