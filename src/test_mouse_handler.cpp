// src/test_mouse_handler.cpp
// Test program for mouse handler
// Task 16d - Mouse Handler

#include "game/input/mouse_handler.h"
#include "game/input/input_state.h"
#include "game/viewport.h"
#include "platform.h"
#include <cstdio>
#include <cstring>

//=============================================================================
// Unit Tests
//=============================================================================

bool Test_DragState() {
    printf("Test: DragState... ");

    DragState drag;
    drag.Clear();

    if (drag.active || drag.started) {
        printf("FAILED - Not cleared\n");
        return false;
    }

    drag.Begin(100, 100, 200, 200, 0);
    if (!drag.active || drag.started) {
        printf("FAILED - Begin state wrong\n");
        return false;
    }

    // Move within threshold
    drag.Update(102, 102, 202, 202);
    if (drag.started) {
        printf("FAILED - Started too early\n");
        return false;
    }

    // Move past threshold
    drag.Update(110, 110, 210, 210);
    if (!drag.started) {
        printf("FAILED - Should have started\n");
        return false;
    }

    // Get rect
    int x1, y1, x2, y2;
    drag.GetScreenRect(x1, y1, x2, y2);
    if (x1 != 100 || y1 != 100 || x2 != 110 || y2 != 110) {
        printf("FAILED - Rect wrong\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_CursorContext() {
    printf("Test: CursorContext... ");

    CursorContext ctx;
    ctx.Clear();

    if (ctx.type != CursorContextType::NORMAL) {
        printf("FAILED - Default type wrong\n");
        return false;
    }

    if (ctx.object != nullptr) {
        printf("FAILED - Object not null\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_MouseHandlerInit() {
    printf("Test: MouseHandler Init... ");

    Platform_Init();
    Platform_Graphics_Init();
    Input_Init();
    GameViewport::Instance().Initialize();
    GameViewport::Instance().SetMapSize(64, 64);

    if (!MouseHandler_Init()) {
        printf("FAILED - Init failed\n");
        Input_Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return false;
    }

    MouseHandler_ProcessFrame();

    // Just verify no crash
    int sx = MouseHandler_GetScreenX();
    int sy = MouseHandler_GetScreenY();
    (void)sx;
    (void)sy;

    MouseHandler_Shutdown();
    Input_Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_CursorShapeMapping() {
    printf("Test: Cursor Shape Mapping... ");

    CursorContext ctx;
    ctx.Clear();

    // Normal context, no selection
    ctx.type = CursorContextType::NORMAL;
    CursorShape shape = GetCursorShapeForContext(ctx, false);
    if (shape != CursorShape::ARROW) {
        printf("FAILED - Normal should be ARROW\n");
        return false;
    }

    // Enemy with selection
    ctx.type = CursorContextType::ENEMY_UNIT;
    ctx.is_enemy = true;
    ctx.is_attackable = true;
    shape = GetCursorShapeForContext(ctx, true);
    if (shape != CursorShape::ATTACK) {
        printf("FAILED - Enemy should be ATTACK\n");
        return false;
    }

    // Passable terrain with selection
    ctx.Clear();
    ctx.type = CursorContextType::TERRAIN_PASSABLE;
    ctx.is_passable = true;
    shape = GetCursorShapeForContext(ctx, true);
    if (shape != CursorShape::MOVE) {
        printf("FAILED - Passable should be MOVE\n");
        return false;
    }

    printf("PASSED\n");
    return true;
}

bool Test_SpecialModes() {
    printf("Test: Special Modes... ");

    Platform_Init();
    Platform_Graphics_Init();
    Input_Init();
    GameViewport::Instance().Initialize();
    GameViewport::Instance().SetMapSize(64, 64);
    MouseHandler_Init();

    MouseHandler& mouse = MouseHandler::Instance();

    // Default states
    if (mouse.IsInPlacementMode() || mouse.IsInSellMode() || mouse.IsInRepairMode()) {
        printf("FAILED - Default modes should be off\n");
        MouseHandler_Shutdown();
        Input_Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return false;
    }

    // Enable placement
    mouse.SetPlacementMode(true, 5);
    if (!mouse.IsInPlacementMode()) {
        printf("FAILED - Placement should be on\n");
        MouseHandler_Shutdown();
        Input_Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return false;
    }

    // Enable sell - should disable placement
    mouse.SetSellMode(true);
    if (mouse.IsInPlacementMode() || !mouse.IsInSellMode()) {
        printf("FAILED - Sell should disable placement\n");
        MouseHandler_Shutdown();
        Input_Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return false;
    }

    MouseHandler_Shutdown();
    Input_Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();

    printf("PASSED\n");
    return true;
}

bool Test_DragRectNormalization() {
    printf("Test: Drag Rect Normalization... ");

    DragState drag;
    drag.Clear();

    // Start at (100, 100), drag to (50, 50) - opposite direction
    drag.Begin(100, 100, 200, 200, 0);
    drag.Update(50, 50, 150, 150);

    // Rect should be normalized (min, min) to (max, max)
    int x1, y1, x2, y2;
    drag.GetScreenRect(x1, y1, x2, y2);

    if (x1 != 50 || y1 != 50 || x2 != 100 || y2 != 100) {
        printf("FAILED - Expected (50,50)-(100,100), got (%d,%d)-(%d,%d)\n", x1, y1, x2, y2);
        return false;
    }

    printf("PASSED\n");
    return true;
}

//=============================================================================
// Interactive Test
//=============================================================================

void Interactive_Test() {
    printf("\n=== Interactive Mouse Handler Test ===\n");
    printf("Move mouse around to see coordinates and context\n");
    printf("Left-drag to test drag selection\n");
    printf("Press P for placement mode, S for sell mode, R for repair mode\n");
    printf("ESC to exit\n\n");

    Platform_Init();
    Platform_Graphics_Init();
    Input_Init();
    GameViewport::Instance().Initialize();
    GameViewport::Instance().SetMapSize(128, 128);
    MouseHandler_Init();

    MouseHandler& mouse = MouseHandler::Instance();

    bool running = true;
    while (running) {
        Platform_PollEvents();
        Input_Update();
        MouseHandler_ProcessFrame();

        if (Input_KeyPressed(KEY_ESCAPE)) {
            running = false;
            continue;
        }

        // Mode toggles
        if (Input_KeyPressed(KEY_P)) {
            mouse.SetPlacementMode(!mouse.IsInPlacementMode());
            printf("Placement mode: %s\n", mouse.IsInPlacementMode() ? "ON" : "OFF");
        }
        if (Input_KeyPressed(KEY_S)) {
            mouse.SetSellMode(!mouse.IsInSellMode());
            printf("Sell mode: %s\n", mouse.IsInSellMode() ? "ON" : "OFF");
        }
        if (Input_KeyPressed(KEY_R)) {
            mouse.SetRepairMode(!mouse.IsInRepairMode());
            printf("Repair mode: %s\n", mouse.IsInRepairMode() ? "ON" : "OFF");
        }

        // Report position changes
        static int last_cx = -1, last_cy = -1;
        int cx = mouse.GetCellX();
        int cy = mouse.GetCellY();
        if (cx != last_cx || cy != last_cy) {
            printf("Screen(%d,%d) World(%d,%d) Cell(%d,%d) Region:%d Cursor:%d\n",
                   mouse.GetScreenX(), mouse.GetScreenY(),
                   mouse.GetWorldX(), mouse.GetWorldY(),
                   cx, cy,
                   static_cast<int>(mouse.GetScreenRegion()),
                   static_cast<int>(mouse.GetCurrentCursorShape()));
            last_cx = cx;
            last_cy = cy;
        }

        // Report clicks
        if (MouseHandler_WasLeftClicked()) {
            printf("LEFT CLICK at cell (%d, %d)\n", cx, cy);
        }
        if (MouseHandler_WasRightClicked()) {
            printf("RIGHT CLICK at cell (%d, %d)\n", cx, cy);
        }

        // Report drag
        if (mouse.WasDragCompleted()) {
            int x1, y1, x2, y2;
            mouse.GetDrag().GetScreenRect(x1, y1, x2, y2);
            printf("DRAG completed: (%d,%d) to (%d,%d)\n", x1, y1, x2, y2);
        }

        Platform_Timer_Delay(16);
    }

    MouseHandler_Shutdown();
    Input_Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Main
//=============================================================================

int main(int argc, char* argv[]) {
    printf("=== Mouse Handler Tests (Task 16d) ===\n\n");

    // Check for quick mode
    bool quick_mode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--quick") == 0 || strcmp(argv[i], "-q") == 0) {
            quick_mode = true;
        }
    }

    int passed = 0;
    int failed = 0;

    if (Test_DragState()) passed++; else failed++;
    if (Test_CursorContext()) passed++; else failed++;
    if (Test_MouseHandlerInit()) passed++; else failed++;
    if (Test_CursorShapeMapping()) passed++; else failed++;
    if (Test_SpecialModes()) passed++; else failed++;
    if (Test_DragRectNormalization()) passed++; else failed++;

    printf("\n");
    if (failed == 0) {
        printf("All tests PASSED (%d/%d)\n", passed, passed + failed);
    } else {
        printf("Results: %d passed, %d failed\n", passed, failed);
    }

    if (!quick_mode && argc > 1 && strcmp(argv[1], "-i") == 0) {
        Interactive_Test();
    } else if (!quick_mode) {
        printf("\nRun with -i for interactive test\n");
    }

    return (failed == 0) ? 0 : 1;
}
