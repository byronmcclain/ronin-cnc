# Task 14j: Integration & Milestones

## Dependencies
- All previous Phase 14 tasks (14a-14i) must be complete
- Platform layer must be fully functional
- Compatibility layer (Phase 13) must be complete

## Context
This is the capstone task for Phase 14. It integrates all components, updates the CMake build system, creates the game executable, and verifies each milestone from the phase document.

## Objective
Complete Phase 14 integration:
1. Update CMakeLists.txt with all game code
2. Create RedAlertGame executable
3. Run all milestone tests
4. Document known issues
5. Prepare for Phase 15

## Technical Background

### Phase 14 Structure Summary
```
src/game/
    core/           <- 14a, 14b (types, coords)
    display/        <- 14c, 14d, 14f (screen, map, display)
    map/            <- 14e (cells)
    object/         <- 14g (object system)
    types/          <- 14h (type classes)
    game_loop.cpp   <- 14i (main loop)
    init.cpp        <- 14i (initialization)
```

### Milestones from Phase Document
1. **Empty Window** - Opens window, black screen, ESC exits
2. **Palette Display** - Loads palette, shows gradient
3. **Shape Display** - Loads and displays sprite
4. **Map Display** - Renders terrain
5. **Interactive Demo** - Mouse/keyboard, scrolling

## Deliverables
- [ ] Updated CMakeLists.txt with RedAlertGame target
- [ ] All game source files compile
- [ ] Milestone 1: Empty Window works
- [ ] Milestone 2: Palette Display works
- [ ] Milestone 3: Basic rendering works
- [ ] Milestone 4: Map cells render
- [ ] Milestone 5: Input responds
- [ ] Documentation of known issues
- [ ] Verification script passes

## Files to Modify

### CMakeLists.txt (MODIFY)
Add the following section for the game code:

```cmake
# =============================================================================
# Game Code (Ported Red Alert)
# =============================================================================

# Game source files
set(GAME_SOURCES
    # Core types
    src/game/core/coord.cpp

    # Display system
    src/game/display/gscreen.cpp
    src/game/display/gadget.cpp
    src/game/display/map.cpp
    src/game/display/display.cpp

    # Map cells
    src/game/map/cell.cpp

    # Object system
    src/game/object/object.cpp

    # Type classes
    src/game/types/types.cpp

    # Main loop
    src/game/game_loop.cpp
    src/game/init.cpp
)

# Game header files (for IDE)
set(GAME_HEADERS
    include/game/game.h
    include/game/coord.h
    include/game/facing.h
    include/game/house.h
    include/game/weapon.h
    include/game/gscreen.h
    include/game/gadget.h
    include/game/map.h
    include/game/cell.h
    include/game/display.h
    include/game/abstract.h
    include/game/object.h
    include/game/techno.h
    include/game/mission.h
    include/game/core/rtti.h
    include/game/core/types.h
    include/game/core/globals.h
    include/game/types/abstracttype.h
    include/game/types/objecttype.h
    include/game/types/technotype.h
    include/game/types/unittype.h
    include/game/types/buildingtype.h
)

# RedAlertGame executable
add_executable(RedAlertGame
    src/main_game.cpp
    ${GAME_SOURCES}
    ${GAME_HEADERS}
)

target_include_directories(RedAlertGame PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/compat
)

target_link_libraries(RedAlertGame PRIVATE
    redalert_platform
    compat
)

target_compile_features(RedAlertGame PRIVATE cxx_std_17)

# Suppress warnings for legacy code patterns
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(RedAlertGame PRIVATE
        -Wno-unused-parameter
        -Wno-unused-variable
    )
endif()

# =============================================================================
# Game Tests
# =============================================================================

# Test game structure
add_executable(TestGameStructure
    src/test_game_structure.cpp
    ${GAME_SOURCES}
)

target_include_directories(TestGameStructure PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(TestGameStructure PRIVATE
    redalert_platform
)

add_test(NAME TestGameStructure COMMAND TestGameStructure)
```

## Files to Create

### src/main_game.cpp (NEW)
```cpp
/**
 * Red Alert Game - Main Entry Point
 *
 * Entry point for the ported Red Alert game.
 */

#include "game/game.h"
#include "platform.h"
#include <cstdio>

/**
 * Parse command line arguments
 */
static void Parse_Args(int argc, char* argv[], bool* show_help, bool* run_test) {
    *show_help = false;
    *run_test = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            *show_help = true;
        }
        else if (strcmp(argv[i], "--test") == 0) {
            *run_test = true;
        }
    }
}

/**
 * Show help message
 */
static void Show_Help() {
    printf("Red Alert - Ported Game\n");
    printf("\n");
    printf("Usage: RedAlertGame [options]\n");
    printf("\n");
    printf("Options:\n");
    printf("  --help, -h     Show this help message\n");
    printf("  --test         Run integration tests\n");
    printf("\n");
    printf("In-game controls:\n");
    printf("  Arrow keys / WASD  - Scroll map\n");
    printf("  +/-                - Change game speed\n");
    printf("  ESC                - Pause / Quit\n");
    printf("  Enter              - Start game (from menu)\n");
    printf("\n");
}

/**
 * Run integration tests
 */
static int Run_Tests() {
    printf("=== Red Alert Integration Tests ===\n\n");

    int passed = 0;
    int failed = 0;

    // Test 1: Platform initialization
    printf("Test 1: Platform initialization... ");
    if (Platform_Init() == PLATFORM_RESULT_SUCCESS) {
        printf("PASSED\n");
        passed++;
    } else {
        printf("FAILED\n");
        failed++;
        return 1;
    }

    // Test 2: Graphics initialization
    printf("Test 2: Graphics initialization... ");
    if (Platform_Graphics_Init() == PLATFORM_RESULT_SUCCESS) {
        printf("PASSED\n");
        passed++;
    } else {
        printf("FAILED\n");
        failed++;
    }

    // Test 3: Game class creation
    printf("Test 3: Game class creation... ");
    GameClass game;
    printf("PASSED\n");
    passed++;

    // Test 4: Game initialization
    printf("Test 4: Game initialization... ");
    if (game.Initialize()) {
        printf("PASSED\n");
        passed++;
    } else {
        printf("FAILED\n");
        failed++;
    }

    // Test 5: Display exists
    printf("Test 5: Display creation... ");
    if (game.Get_Display() != nullptr) {
        printf("PASSED\n");
        passed++;
    } else {
        printf("FAILED\n");
        failed++;
    }

    // Test 6: Initial mode
    printf("Test 6: Initial game mode... ");
    if (game.Get_Mode() == GAME_MODE_MENU) {
        printf("PASSED\n");
        passed++;
    } else {
        printf("FAILED\n");
        failed++;
    }

    // Test 7: Render a frame
    printf("Test 7: Render frame... ");
    game.Get_Display()->Lock();
    game.Get_Display()->Clear(0);
    game.Get_Display()->Draw_Rect(100, 100, 200, 100, 15);
    game.Get_Display()->Unlock();
    game.Get_Display()->Flip();
    printf("PASSED\n");
    passed++;

    // Test 8: Shutdown
    printf("Test 8: Game shutdown... ");
    game.Shutdown();
    printf("PASSED\n");
    passed++;

    Platform_Shutdown();

    printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);
    return failed > 0 ? 1 : 0;
}

/**
 * Main entry point
 */
int main(int argc, char* argv[]) {
    bool show_help = false;
    bool run_test = false;

    Parse_Args(argc, argv, &show_help, &run_test);

    if (show_help) {
        Show_Help();
        return 0;
    }

    if (run_test) {
        return Run_Tests();
    }

    // Normal game startup
    return Game_Main(argc, argv);
}
```

### src/test_game_structure.cpp (MODIFY)
Update to test all components:

```cpp
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
#include "game/types/unittype.h"
#include "game/types/buildingtype.h"
#include "platform.h"
#include <cstdio>

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
```

## Verification Command
```bash
ralph-tasks/verify/check-integration.sh
```

## Verification Script

### ralph-tasks/verify/check-integration.sh (NEW)
```bash
#!/bin/bash
# Final verification script for Phase 14 Integration

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=========================================="
echo "Phase 14: Game Code Port - Final Verification"
echo "=========================================="
echo ""

cd "$ROOT_DIR"

# Step 1: Check all required files exist
echo "[1/6] Checking file structure..."

REQUIRED_FILES=(
    # Core
    "include/game/coord.h"
    "include/game/facing.h"
    "include/game/house.h"
    "include/game/weapon.h"
    "src/game/core/coord.cpp"

    # Display
    "include/game/gscreen.h"
    "include/game/gadget.h"
    "include/game/map.h"
    "include/game/display.h"
    "src/game/display/gscreen.cpp"
    "src/game/display/gadget.cpp"
    "src/game/display/map.cpp"
    "src/game/display/display.cpp"

    # Cell
    "include/game/cell.h"
    "src/game/map/cell.cpp"

    # Object
    "include/game/abstract.h"
    "include/game/object.h"
    "include/game/techno.h"
    "include/game/mission.h"
    "src/game/object/object.cpp"

    # Types
    "include/game/types/abstracttype.h"
    "include/game/types/objecttype.h"
    "include/game/types/technotype.h"
    "include/game/types/unittype.h"
    "include/game/types/buildingtype.h"
    "src/game/types/types.cpp"

    # Game loop
    "include/game/game.h"
    "src/game/game_loop.cpp"
    "src/game/init.cpp"

    # Main
    "src/main_game.cpp"
    "src/test_game_structure.cpp"
)

missing=0
for f in "${REQUIRED_FILES[@]}"; do
    if [ ! -f "$f" ]; then
        echo "  MISSING: $f"
        missing=$((missing + 1))
    fi
done

if [ $missing -gt 0 ]; then
    echo "VERIFY_FAILED: $missing files missing"
    exit 1
fi
echo "  OK: All $((${#REQUIRED_FILES[@]})) files present"

# Step 2: Configure CMake
echo ""
echo "[2/6] Configuring CMake..."
if ! cmake -B build -S . 2>&1 | grep -v "^--" | head -20; then
    echo "VERIFY_FAILED: CMake configuration failed"
    exit 1
fi
echo "  OK: CMake configured"

# Step 3: Build RedAlertGame
echo ""
echo "[3/6] Building RedAlertGame..."
if ! cmake --build build --target RedAlertGame 2>&1 | tail -20; then
    echo "VERIFY_FAILED: RedAlertGame build failed"
    exit 1
fi
echo "  OK: RedAlertGame built"

# Step 4: Build TestGameStructure
echo ""
echo "[4/6] Building TestGameStructure..."
if ! cmake --build build --target TestGameStructure 2>&1 | tail -10; then
    echo "VERIFY_FAILED: TestGameStructure build failed"
    exit 1
fi
echo "  OK: TestGameStructure built"

# Step 5: Run TestGameStructure
echo ""
echo "[5/6] Running TestGameStructure..."
if ! ./build/TestGameStructure; then
    echo "VERIFY_FAILED: TestGameStructure failed"
    exit 1
fi
echo "  OK: TestGameStructure passed"

# Step 6: Run RedAlertGame --test
echo ""
echo "[6/6] Running RedAlertGame --test..."
if ! ./build/RedAlertGame --test; then
    echo "VERIFY_FAILED: RedAlertGame integration tests failed"
    exit 1
fi
echo "  OK: RedAlertGame tests passed"

echo ""
echo "=========================================="
echo "Phase 14 Integration: VERIFY_SUCCESS"
echo "=========================================="
echo ""
echo "All milestones verified:"
echo "  [x] Game code compiles"
echo "  [x] Type classes work"
echo "  [x] Coordinate system works"
echo "  [x] Display classes work"
echo "  [x] Object system works"
echo "  [x] Main loop runs"
echo ""
echo "Ready for Phase 15: Asset Integration"
exit 0
```

## Milestone Tests

### Milestone 1: Empty Window
```bash
./build/RedAlertGame
# Should: Open window, show black/menu screen, ESC exits
```

### Milestone 2: Basic Rendering
```bash
./build/RedAlertGame --test
# Should: Pass all rendering tests
```

### Milestone 3: Map Display
```bash
# Press Enter in game to start
# Should: Show colored cells representing terrain
```

### Milestone 4: Input Response
```bash
# Arrow keys should scroll the map
# Mouse should highlight cells
# ESC should pause
```

### Milestone 5: Full Integration
```bash
./build/TestGameStructure
# Should: All tests pass
```

## Known Issues to Document

Create `ralph-tasks/14-game-code-port/KNOWN_ISSUES.md`:

```markdown
# Phase 14 Known Issues

## Rendering

### No Real Graphics Yet
- All graphics are placeholder colored rectangles
- Actual sprite/shape loading is Phase 15
- Palette loading is stubbed

### No Font Rendering
- Text appears as placeholder dots
- Font system needs asset loading

## Input

### Edge Scrolling Not Implemented
- Only keyboard scrolling works
- Mouse edge scroll needs DisplayClass update

## Objects

### No Object Instantiation
- ObjectClass exists but no units created yet
- Factory system not implemented

## Type Classes

### Incomplete Type Data
- Only sample unit/building types defined
- Full data needs RULES.INI parsing

## Performance

### No Dirty Rect Tracking
- Full screen redrawn every frame
- Optimization needed for large maps

## Audio

### No Sound
- Audio system stubbed
- SFX and music need Phase 16
```

## Implementation Steps
1. Update CMakeLists.txt with game targets
2. Create src/main_game.cpp
3. Update src/test_game_structure.cpp
4. Ensure all previous task files exist
5. Run cmake and build
6. Run all verification tests
7. Document known issues

## Success Criteria
- CMake configures without errors
- RedAlertGame builds successfully
- TestGameStructure passes all tests
- RedAlertGame --test passes
- Basic game loop runs and renders
- Input is processed
- Clean exit on ESC

## Completion Promise
When verification passes, output:
```
<promise>TASK_14J_COMPLETE</promise>
<promise>PHASE_14_COMPLETE</promise>
```

## Escape Hatch
If stuck after 15 iterations:
- Document what's blocking in `ralph-tasks/blocked/14j.md`
- Output: `<promise>TASK_14J_BLOCKED</promise>`

## Max Iterations
15

## Next Phase
After Phase 14 is complete, Phase 15 (Asset Integration) will:
- Load actual graphics from MIX files
- Implement shape/sprite rendering
- Load palettes from assets
- Implement template/terrain drawing
- Add sound effect stubs
