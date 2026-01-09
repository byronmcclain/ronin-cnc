/**
 * Red Alert Game - Main Entry Point
 *
 * Entry point for the ported Red Alert game.
 */

#include "game/game.h"
#include "platform.h"
#include <cstdio>
#include <cstring>

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
    printf("  Arrow keys     - Scroll map\n");
    printf("  F5/F6          - Change game speed\n");
    printf("  ESC            - Pause / Quit\n");
    printf("  Enter          - Start game (from menu)\n");
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
    if (game.Get_Display()) {
        game.Get_Display()->Lock();
        game.Get_Display()->Clear(0);
        game.Get_Display()->Draw_Rect(100, 100, 200, 100, 15);
        game.Get_Display()->Unlock();
        game.Get_Display()->Flip();
        printf("PASSED\n");
        passed++;
    } else {
        printf("FAILED\n");
        failed++;
    }

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
