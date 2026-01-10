// src/test_selection.cpp
// Test program for selection system
// Task 16e - Selection System

#include "game/input/selection_manager.h"
#include "platform.h"
#include <cstdio>
#include <cstring>
#include <vector>

// Test objects
static std::vector<SelectableObject> g_test_objects;

static void CreateTestObjects(int player_house) {
    g_test_objects.clear();

    // Create some test objects
    for (int i = 0; i < 50; i++) {
        SelectableObject obj;
        obj.id = 1000 + i;
        obj.cell_x = (i % 10) * 2;
        obj.cell_y = (i / 10) * 2;
        obj.pixel_x = obj.cell_x * 24;
        obj.pixel_y = obj.cell_y * 24;
        obj.width = 24;
        obj.height = 24;
        obj.owner = (i < 40) ? player_house : (player_house + 1);  // 40 friendly, 10 enemy
        obj.type = i % 5;
        obj.is_unit = (i % 4) != 0;  // 75% units, 25% buildings
        obj.is_active = true;
        obj.rtti_type = obj.is_unit ? 1 : 5;  // Placeholder RTTI
        g_test_objects.push_back(obj);
    }
}

static std::vector<SelectableObject*> QueryObjectsInRect(int x1, int y1, int x2, int y2) {
    std::vector<SelectableObject*> result;
    for (auto& obj : g_test_objects) {
        if (obj.pixel_x >= x1 && obj.pixel_x <= x2 &&
            obj.pixel_y >= y1 && obj.pixel_y <= y2) {
            result.push_back(&obj);
        }
    }
    return result;
}

static SelectableObject* QueryObjectAtPos(int cell_x, int cell_y) {
    for (auto& obj : g_test_objects) {
        if (obj.cell_x == cell_x && obj.cell_y == cell_y) {
            return &obj;
        }
    }
    return nullptr;
}

static std::vector<SelectableObject*> QueryAllObjects() {
    std::vector<SelectableObject*> result;
    for (auto& obj : g_test_objects) {
        result.push_back(&obj);
    }
    return result;
}

bool Test_SelectionInit() {
    printf("Test: Selection Init... ");

    if (!SelectionManager_Init()) {
        printf("FAILED - Init failed\n");
        return false;
    }

    if (Selection_HasSelection()) {
        printf("FAILED - Should start empty\n");
        SelectionManager_Shutdown();
        return false;
    }

    SelectionManager_Shutdown();
    printf("PASSED\n");
    return true;
}

bool Test_SingleSelection() {
    printf("Test: Single Selection... ");

    CreateTestObjects(0);
    SelectionManager_Init();

    auto& mgr = SelectionManager::Instance();
    mgr.SetPlayerHouse(0);

    // Select first object
    mgr.Select(&g_test_objects[0]);

    if (!mgr.HasSelection()) {
        printf("FAILED - Should have selection\n");
        SelectionManager_Shutdown();
        return false;
    }

    if (mgr.GetSelectionCount() != 1) {
        printf("FAILED - Count should be 1\n");
        SelectionManager_Shutdown();
        return false;
    }

    if (!mgr.IsSelected(&g_test_objects[0])) {
        printf("FAILED - Object should be selected\n");
        SelectionManager_Shutdown();
        return false;
    }

    // Select different object (should replace)
    mgr.Select(&g_test_objects[1]);

    if (mgr.GetSelectionCount() != 1) {
        printf("FAILED - Count should still be 1\n");
        SelectionManager_Shutdown();
        return false;
    }

    if (mgr.IsSelected(&g_test_objects[0])) {
        printf("FAILED - First object should not be selected\n");
        SelectionManager_Shutdown();
        return false;
    }

    SelectionManager_Shutdown();
    printf("PASSED\n");
    return true;
}

bool Test_AdditiveSelection() {
    printf("Test: Additive Selection... ");

    CreateTestObjects(0);
    SelectionManager_Init();

    auto& mgr = SelectionManager::Instance();
    mgr.SetPlayerHouse(0);

    // Select first, add second
    mgr.Select(&g_test_objects[0]);
    mgr.AddToSelection(&g_test_objects[1]);

    if (mgr.GetSelectionCount() != 2) {
        printf("FAILED - Count should be 2\n");
        SelectionManager_Shutdown();
        return false;
    }

    // Toggle removes
    mgr.ToggleSelection(&g_test_objects[0]);

    if (mgr.GetSelectionCount() != 1) {
        printf("FAILED - Count should be 1 after toggle\n");
        SelectionManager_Shutdown();
        return false;
    }

    if (mgr.IsSelected(&g_test_objects[0])) {
        printf("FAILED - Object 0 should not be selected\n");
        SelectionManager_Shutdown();
        return false;
    }

    SelectionManager_Shutdown();
    printf("PASSED\n");
    return true;
}

bool Test_BoxSelection() {
    printf("Test: Box Selection... ");

    CreateTestObjects(0);
    SelectionManager_Init();

    auto& mgr = SelectionManager::Instance();
    mgr.SetPlayerHouse(0);
    mgr.SetObjectsInRectQuery(QueryObjectsInRect);

    // Select objects in a region
    mgr.SelectInBox(0, 0, 100, 100);

    // Should have selected some units (not buildings)
    if (!mgr.HasSelection()) {
        printf("FAILED - Should have selection\n");
        SelectionManager_Shutdown();
        return false;
    }

    // All selected should be units
    for (const auto* obj : mgr.GetSelection()) {
        if (!obj->is_unit) {
            printf("FAILED - Box selection included building\n");
            SelectionManager_Shutdown();
            return false;
        }
        if (obj->owner != 0) {
            printf("FAILED - Box selection included enemy\n");
            SelectionManager_Shutdown();
            return false;
        }
    }

    SelectionManager_Shutdown();
    printf("PASSED\n");
    return true;
}

bool Test_ControlGroups() {
    printf("Test: Control Groups... ");

    CreateTestObjects(0);
    SelectionManager_Init();

    auto& mgr = SelectionManager::Instance();
    mgr.SetPlayerHouse(0);
    mgr.SetAllObjectsQuery(QueryAllObjects);

    // Select some objects and save to group 1
    mgr.Select(&g_test_objects[0]);
    mgr.AddToSelection(&g_test_objects[1]);
    mgr.AddToSelection(&g_test_objects[2]);

    mgr.SaveGroup(1);

    if (!mgr.HasGroup(1)) {
        printf("FAILED - Group 1 should exist\n");
        SelectionManager_Shutdown();
        return false;
    }

    if (mgr.GetGroupSize(1) != 3) {
        printf("FAILED - Group 1 should have 3 objects\n");
        SelectionManager_Shutdown();
        return false;
    }

    // Clear and recall
    mgr.Clear();

    if (mgr.HasSelection()) {
        printf("FAILED - Should be empty after clear\n");
        SelectionManager_Shutdown();
        return false;
    }

    mgr.RecallGroup(1);

    if (mgr.GetSelectionCount() != 3) {
        printf("FAILED - Should have 3 after recall\n");
        SelectionManager_Shutdown();
        return false;
    }

    SelectionManager_Shutdown();
    printf("PASSED\n");
    return true;
}

bool Test_MaxSelection() {
    printf("Test: Max Selection Limit... ");

    CreateTestObjects(0);
    SelectionManager_Init();

    auto& mgr = SelectionManager::Instance();
    mgr.SetPlayerHouse(0);

    // Try to select more than MAX_SELECTION
    for (int i = 0; i < 40; i++) {
        if (g_test_objects[i].is_unit && g_test_objects[i].owner == 0) {
            mgr.AddToSelection(&g_test_objects[i]);
        }
    }

    if (mgr.GetSelectionCount() > MAX_SELECTION) {
        printf("FAILED - Exceeded max selection (%d > %d)\n",
               mgr.GetSelectionCount(), MAX_SELECTION);
        SelectionManager_Shutdown();
        return false;
    }

    SelectionManager_Shutdown();
    printf("PASSED\n");
    return true;
}

bool Test_ObjectDestroyed() {
    printf("Test: Object Destroyed... ");

    CreateTestObjects(0);
    SelectionManager_Init();

    auto& mgr = SelectionManager::Instance();
    mgr.SetPlayerHouse(0);
    mgr.SetAllObjectsQuery(QueryAllObjects);

    // Select and group
    mgr.Select(&g_test_objects[0]);
    mgr.AddToSelection(&g_test_objects[1]);
    mgr.SaveGroup(5);

    // "Destroy" object 0
    mgr.OnObjectDestroyed(&g_test_objects[0]);

    if (mgr.IsSelected(&g_test_objects[0])) {
        printf("FAILED - Destroyed object still selected\n");
        SelectionManager_Shutdown();
        return false;
    }

    if (mgr.GetSelectionCount() != 1) {
        printf("FAILED - Selection count wrong\n");
        SelectionManager_Shutdown();
        return false;
    }

    // Group should also be updated
    if (mgr.GetGroupSize(5) != 1) {
        printf("FAILED - Group size wrong after destroy\n");
        SelectionManager_Shutdown();
        return false;
    }

    SelectionManager_Shutdown();
    printf("PASSED\n");
    return true;
}

bool Test_EnemyNotSelectable() {
    printf("Test: Enemy Not Selectable... ");

    CreateTestObjects(0);
    SelectionManager_Init();

    auto& mgr = SelectionManager::Instance();
    mgr.SetPlayerHouse(0);

    // Try to select enemy object (index 45+ are enemies)
    mgr.Select(&g_test_objects[45]);

    if (mgr.HasSelection()) {
        printf("FAILED - Should not select enemy\n");
        SelectionManager_Shutdown();
        return false;
    }

    SelectionManager_Shutdown();
    printf("PASSED\n");
    return true;
}

int main(int argc, char* argv[]) {
    printf("=== Selection System Tests (Task 16e) ===\n\n");

    // Check for quick mode
    bool quick_mode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--quick") == 0 || strcmp(argv[i], "-q") == 0) {
            quick_mode = true;
        }
    }
    (void)quick_mode;

    Platform_Init();

    int passed = 0;
    int failed = 0;

    if (Test_SelectionInit()) passed++; else failed++;
    if (Test_SingleSelection()) passed++; else failed++;
    if (Test_AdditiveSelection()) passed++; else failed++;
    if (Test_BoxSelection()) passed++; else failed++;
    if (Test_ControlGroups()) passed++; else failed++;
    if (Test_MaxSelection()) passed++; else failed++;
    if (Test_ObjectDestroyed()) passed++; else failed++;
    if (Test_EnemyNotSelectable()) passed++; else failed++;

    Platform_Shutdown();

    printf("\n");
    if (failed == 0) {
        printf("All tests PASSED (%d/%d)\n", passed, passed + failed);
    } else {
        printf("Results: %d passed, %d failed\n", passed, failed);
    }

    return (failed == 0) ? 0 : 1;
}
