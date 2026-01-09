# Task 16e: Selection System

| Field | Value |
|-------|-------|
| **Task ID** | 16e |
| **Task Name** | Selection System |
| **Phase** | 16 - Input Integration |
| **Depends On** | 16a (Input Foundation), 16d (Mouse Handler) |
| **Estimated Complexity** | Medium-High (~500 lines) |

---

## Dependencies

- Task 16a (Input Foundation) must be complete
- Task 16d (Mouse Handler) must be complete for drag selection
- MouseHandler must provide drag rectangle coordinates

---

## Context

The selection system manages which game objects (units, buildings) are currently selected by the player. This is fundamental to the RTS control scheme - players select units then issue commands to them. Key features include:

1. **Click Selection**: Select single object by clicking
2. **Box Selection**: Drag to select multiple objects
3. **Additive Selection**: Shift+click to add/remove from selection
4. **Double-Click Selection**: Select all visible units of same type
5. **Control Groups**: Ctrl+1-9 to save selection, 1-9 to recall

The original Red Alert stored selections in a fixed array with a maximum count. Our implementation uses a more flexible approach with proper ownership tracking.

---

## Objective

Create a selection manager that:
1. Maintains list of currently selected objects
2. Handles click and box selection
3. Supports additive selection with Shift
4. Implements 10 control groups (0-9)
5. Provides selection queries for UI and commands

---

## Technical Background

### Selection Rules

```
Selection Priority (when multiple objects at click point):
1. Player's own units > buildings > neutral > enemy
2. Mobile units > stationary units
3. Combat units > support units

Box Selection:
- Only selects player's own units (not buildings or enemies)
- Infantry prioritized over vehicles if mixed
- 12 unit maximum in original (we'll use 30)

Control Groups:
- Ctrl+[0-9]: Store current selection in group
- [0-9]: Recall stored group (replace selection)
- Shift+[0-9]: Add to current selection
- Double-tap [0-9]: Center view on group
```

### Object Types for Selection

```cpp
// Object type categories for selection
enum class SelectableType {
    NONE,
    INFANTRY,
    VEHICLE,
    AIRCRAFT,
    VESSEL,
    BUILDING
};
```

---

## Deliverables

- [ ] `src/game/input/selection_manager.h` - Manager class declaration
- [ ] `src/game/input/selection_manager.cpp` - Manager implementation
- [ ] `tests/test_selection.cpp` - Test program
- [ ] CMakeLists.txt updated
- [ ] All tests pass

---

## Files to Create

### src/game/input/selection_manager.h

```cpp
// src/game/input/selection_manager.h
// Selection management for RTS gameplay
// Task 16e - Selection System

#ifndef SELECTION_MANAGER_H
#define SELECTION_MANAGER_H

#include <cstdint>
#include <vector>
#include <array>
#include <functional>

//=============================================================================
// Constants
//=============================================================================

static const int MAX_SELECTION = 30;
static const int NUM_CONTROL_GROUPS = 10;

//=============================================================================
// Selectable Object Interface
//=============================================================================

// Forward declare - will be ObjectClass when integrated
struct SelectableObject {
    uint32_t id;
    int cell_x;
    int cell_y;
    int pixel_x;        // World pixel position
    int pixel_y;
    int width;          // Bounding box
    int height;
    int owner;          // Player house index
    int type;           // Object type enum
    bool is_unit;       // true = unit, false = building
    bool is_active;

    // For type filtering
    int rtti_type;      // RTTI_INFANTRY, RTTI_UNIT, etc.
};

//=============================================================================
// Selection Event
//=============================================================================

enum class SelectionEvent {
    CLEARED,            // Selection was cleared
    ADDED,              // Object(s) added to selection
    REMOVED,            // Object(s) removed from selection
    REPLACED,           // Selection completely replaced
    GROUP_SAVED,        // Control group saved
    GROUP_RECALLED      // Control group recalled
};

//=============================================================================
// Selection Manager
//=============================================================================

class SelectionManager {
public:
    // Singleton access
    static SelectionManager& Instance();

    // Lifecycle
    bool Initialize();
    void Shutdown();

    //=========================================================================
    // Selection Operations
    //=========================================================================

    // Clear all selections
    void Clear();

    // Single object operations
    void Select(SelectableObject* obj);
    void AddToSelection(SelectableObject* obj);
    void RemoveFromSelection(SelectableObject* obj);
    void ToggleSelection(SelectableObject* obj);

    // Box selection (screen coordinates)
    void SelectInBox(int screen_x1, int screen_y1, int screen_x2, int screen_y2);

    // Type-based selection
    void SelectAllOfType(int rtti_type);
    void SelectAllVisible(int rtti_type);  // All visible on screen

    //=========================================================================
    // Selection Queries
    //=========================================================================

    bool IsSelected(const SelectableObject* obj) const;
    bool IsSelected(uint32_t object_id) const;
    bool HasSelection() const { return !selected_.empty(); }
    int GetSelectionCount() const { return static_cast<int>(selected_.size()); }

    // Get selected objects
    const std::vector<SelectableObject*>& GetSelection() const { return selected_; }
    SelectableObject* GetPrimarySelection() const;  // First selected object

    // Type queries
    bool HasSelectedUnits() const;
    bool HasSelectedBuildings() const;
    bool AllSelectedAreType(int rtti_type) const;
    int GetSelectedType() const;  // Returns type if all same, -1 otherwise

    //=========================================================================
    // Control Groups
    //=========================================================================

    void SaveGroup(int group_num);           // Ctrl+[0-9]
    void RecallGroup(int group_num);         // [0-9]
    void AddGroupToSelection(int group_num); // Shift+[0-9]

    bool HasGroup(int group_num) const;
    int GetGroupSize(int group_num) const;

    // Get group center (for camera jump)
    bool GetGroupCenter(int group_num, int& out_x, int& out_y) const;

    //=========================================================================
    // Object Lifecycle
    //=========================================================================

    // Call when object is destroyed to remove from selection/groups
    void OnObjectDestroyed(SelectableObject* obj);
    void OnObjectDestroyed(uint32_t object_id);

    //=========================================================================
    // Event Callbacks
    //=========================================================================

    using SelectionCallback = std::function<void(SelectionEvent event)>;
    void SetSelectionCallback(SelectionCallback callback) { on_selection_changed_ = callback; }

    //=========================================================================
    // Game Integration
    //=========================================================================

    // Query functions to find objects in world
    using ObjectsInRectFunc = std::function<std::vector<SelectableObject*>(
        int x1, int y1, int x2, int y2)>;
    using ObjectAtPosFunc = std::function<SelectableObject*(int cell_x, int cell_y)>;
    using AllObjectsFunc = std::function<std::vector<SelectableObject*>()>;

    void SetObjectsInRectQuery(ObjectsInRectFunc func) { get_objects_in_rect_ = func; }
    void SetObjectAtPosQuery(ObjectAtPosFunc func) { get_object_at_ = func; }
    void SetAllObjectsQuery(AllObjectsFunc func) { get_all_objects_ = func; }

    void SetPlayerHouse(int house) { player_house_ = house; }

private:
    SelectionManager();
    ~SelectionManager();
    SelectionManager(const SelectionManager&) = delete;
    SelectionManager& operator=(const SelectionManager&) = delete;

    void NotifySelectionChanged(SelectionEvent event);
    bool CanSelect(const SelectableObject* obj) const;
    void SortSelection();  // Sort by priority
    void TrimSelection();  // Enforce MAX_SELECTION

    bool initialized_;
    int player_house_;

    // Current selection
    std::vector<SelectableObject*> selected_;

    // Control groups (0-9)
    std::array<std::vector<uint32_t>, NUM_CONTROL_GROUPS> groups_;

    // Callbacks
    SelectionCallback on_selection_changed_;
    ObjectsInRectFunc get_objects_in_rect_;
    ObjectAtPosFunc get_object_at_;
    AllObjectsFunc get_all_objects_;
};

//=============================================================================
// Global Functions (C-compatible interface)
//=============================================================================

bool SelectionManager_Init();
void SelectionManager_Shutdown();

void Selection_Clear();
void Selection_Select(void* obj);
void Selection_Add(void* obj);
void Selection_Remove(void* obj);
void Selection_SelectInBox(int x1, int y1, int x2, int y2);

bool Selection_IsSelected(void* obj);
bool Selection_HasSelection();
int Selection_GetCount();

void Selection_SaveGroup(int group);
void Selection_RecallGroup(int group);

#endif // SELECTION_MANAGER_H
```

### src/game/input/selection_manager.cpp

```cpp
// src/game/input/selection_manager.cpp
// Selection management implementation
// Task 16e - Selection System

#include "selection_manager.h"
#include "platform.h"
#include <algorithm>
#include <cstring>

//=============================================================================
// Selection Priority
//=============================================================================

static int GetSelectionPriority(const SelectableObject* obj) {
    // Higher priority = selected first when overlapping
    // Units before buildings, combat before support
    if (!obj) return 0;

    int priority = 0;

    // Units have higher priority than buildings
    if (obj->is_unit) {
        priority += 1000;

        // Infantry > Vehicles (easier to click)
        if (obj->rtti_type == 1) {  // RTTI_INFANTRY placeholder
            priority += 100;
        }
    }

    return priority;
}

static bool SelectionPriorityCompare(const SelectableObject* a, const SelectableObject* b) {
    return GetSelectionPriority(a) > GetSelectionPriority(b);
}

//=============================================================================
// SelectionManager Singleton
//=============================================================================

SelectionManager& SelectionManager::Instance() {
    static SelectionManager instance;
    return instance;
}

SelectionManager::SelectionManager()
    : initialized_(false)
    , player_house_(0)
    , on_selection_changed_(nullptr)
    , get_objects_in_rect_(nullptr)
    , get_object_at_(nullptr)
    , get_all_objects_(nullptr) {
    selected_.reserve(MAX_SELECTION);
}

SelectionManager::~SelectionManager() {
    Shutdown();
}

bool SelectionManager::Initialize() {
    if (initialized_) return true;

    Platform_Log("SelectionManager: Initializing...");

    selected_.clear();
    selected_.reserve(MAX_SELECTION);

    // Clear all control groups
    for (auto& group : groups_) {
        group.clear();
    }

    initialized_ = true;
    Platform_Log("SelectionManager: Initialized");
    return true;
}

void SelectionManager::Shutdown() {
    if (!initialized_) return;

    Platform_Log("SelectionManager: Shutting down");

    selected_.clear();
    for (auto& group : groups_) {
        group.clear();
    }

    initialized_ = false;
}

//=============================================================================
// Selection Operations
//=============================================================================

void SelectionManager::Clear() {
    if (selected_.empty()) return;

    selected_.clear();
    NotifySelectionChanged(SelectionEvent::CLEARED);
}

void SelectionManager::Select(SelectableObject* obj) {
    if (!CanSelect(obj)) return;

    selected_.clear();
    selected_.push_back(obj);
    NotifySelectionChanged(SelectionEvent::REPLACED);
}

void SelectionManager::AddToSelection(SelectableObject* obj) {
    if (!CanSelect(obj)) return;

    // Already selected?
    if (IsSelected(obj)) return;

    // At max?
    if (selected_.size() >= MAX_SELECTION) return;

    selected_.push_back(obj);
    SortSelection();
    NotifySelectionChanged(SelectionEvent::ADDED);
}

void SelectionManager::RemoveFromSelection(SelectableObject* obj) {
    auto it = std::find(selected_.begin(), selected_.end(), obj);
    if (it != selected_.end()) {
        selected_.erase(it);
        NotifySelectionChanged(SelectionEvent::REMOVED);
    }
}

void SelectionManager::ToggleSelection(SelectableObject* obj) {
    if (IsSelected(obj)) {
        RemoveFromSelection(obj);
    } else {
        AddToSelection(obj);
    }
}

void SelectionManager::SelectInBox(int screen_x1, int screen_y1,
                                    int screen_x2, int screen_y2) {
    if (!get_objects_in_rect_) {
        Platform_Log("SelectionManager: No objects_in_rect query set");
        return;
    }

    // Normalize coordinates
    if (screen_x1 > screen_x2) std::swap(screen_x1, screen_x2);
    if (screen_y1 > screen_y2) std::swap(screen_y1, screen_y2);

    // Get all objects in the rectangle
    auto objects = get_objects_in_rect_(screen_x1, screen_y1, screen_x2, screen_y2);

    // Filter to only player's units (not buildings)
    std::vector<SelectableObject*> selectable;
    for (auto* obj : objects) {
        if (obj && obj->is_unit && obj->owner == player_house_ && obj->is_active) {
            selectable.push_back(obj);
        }
    }

    if (selectable.empty()) return;

    // Sort by priority
    std::sort(selectable.begin(), selectable.end(), SelectionPriorityCompare);

    // Clear and add new selection
    selected_.clear();
    for (auto* obj : selectable) {
        if (selected_.size() >= MAX_SELECTION) break;
        selected_.push_back(obj);
    }

    NotifySelectionChanged(SelectionEvent::REPLACED);
}

void SelectionManager::SelectAllOfType(int rtti_type) {
    if (!get_all_objects_) return;

    auto all = get_all_objects_();

    selected_.clear();
    for (auto* obj : all) {
        if (obj && obj->rtti_type == rtti_type &&
            obj->owner == player_house_ && obj->is_active) {
            if (selected_.size() >= MAX_SELECTION) break;
            selected_.push_back(obj);
        }
    }

    if (!selected_.empty()) {
        SortSelection();
        NotifySelectionChanged(SelectionEvent::REPLACED);
    }
}

void SelectionManager::SelectAllVisible(int rtti_type) {
    // This would use viewport bounds - implemented in integration
    // For now, same as SelectAllOfType
    SelectAllOfType(rtti_type);
}

//=============================================================================
// Selection Queries
//=============================================================================

bool SelectionManager::IsSelected(const SelectableObject* obj) const {
    if (!obj) return false;
    return std::find(selected_.begin(), selected_.end(), obj) != selected_.end();
}

bool SelectionManager::IsSelected(uint32_t object_id) const {
    for (const auto* obj : selected_) {
        if (obj && obj->id == object_id) return true;
    }
    return false;
}

SelectableObject* SelectionManager::GetPrimarySelection() const {
    return selected_.empty() ? nullptr : selected_[0];
}

bool SelectionManager::HasSelectedUnits() const {
    for (const auto* obj : selected_) {
        if (obj && obj->is_unit) return true;
    }
    return false;
}

bool SelectionManager::HasSelectedBuildings() const {
    for (const auto* obj : selected_) {
        if (obj && !obj->is_unit) return true;
    }
    return false;
}

bool SelectionManager::AllSelectedAreType(int rtti_type) const {
    if (selected_.empty()) return false;

    for (const auto* obj : selected_) {
        if (!obj || obj->rtti_type != rtti_type) return false;
    }
    return true;
}

int SelectionManager::GetSelectedType() const {
    if (selected_.empty()) return -1;

    int type = selected_[0]->rtti_type;
    for (size_t i = 1; i < selected_.size(); i++) {
        if (selected_[i]->rtti_type != type) return -1;
    }
    return type;
}

//=============================================================================
// Control Groups
//=============================================================================

void SelectionManager::SaveGroup(int group_num) {
    if (group_num < 0 || group_num >= NUM_CONTROL_GROUPS) return;

    groups_[group_num].clear();
    for (const auto* obj : selected_) {
        if (obj) {
            groups_[group_num].push_back(obj->id);
        }
    }

    Platform_Log("SelectionManager: Saved group %d with %zu objects",
                 group_num, groups_[group_num].size());

    NotifySelectionChanged(SelectionEvent::GROUP_SAVED);
}

void SelectionManager::RecallGroup(int group_num) {
    if (group_num < 0 || group_num >= NUM_CONTROL_GROUPS) return;

    if (groups_[group_num].empty()) return;

    // Rebuild selection from stored IDs
    selected_.clear();

    if (get_all_objects_) {
        auto all = get_all_objects_();

        for (uint32_t id : groups_[group_num]) {
            for (auto* obj : all) {
                if (obj && obj->id == id && obj->is_active) {
                    selected_.push_back(obj);
                    break;
                }
            }
        }
    }

    // Clean up group - remove invalid IDs
    std::vector<uint32_t> valid_ids;
    for (const auto* obj : selected_) {
        if (obj) valid_ids.push_back(obj->id);
    }
    groups_[group_num] = valid_ids;

    if (!selected_.empty()) {
        NotifySelectionChanged(SelectionEvent::GROUP_RECALLED);
    }
}

void SelectionManager::AddGroupToSelection(int group_num) {
    if (group_num < 0 || group_num >= NUM_CONTROL_GROUPS) return;

    if (groups_[group_num].empty()) return;

    if (!get_all_objects_) return;

    auto all = get_all_objects_();

    for (uint32_t id : groups_[group_num]) {
        if (selected_.size() >= MAX_SELECTION) break;

        for (auto* obj : all) {
            if (obj && obj->id == id && obj->is_active && !IsSelected(obj)) {
                selected_.push_back(obj);
                break;
            }
        }
    }

    if (!selected_.empty()) {
        SortSelection();
        NotifySelectionChanged(SelectionEvent::ADDED);
    }
}

bool SelectionManager::HasGroup(int group_num) const {
    if (group_num < 0 || group_num >= NUM_CONTROL_GROUPS) return false;
    return !groups_[group_num].empty();
}

int SelectionManager::GetGroupSize(int group_num) const {
    if (group_num < 0 || group_num >= NUM_CONTROL_GROUPS) return 0;
    return static_cast<int>(groups_[group_num].size());
}

bool SelectionManager::GetGroupCenter(int group_num, int& out_x, int& out_y) const {
    if (group_num < 0 || group_num >= NUM_CONTROL_GROUPS) return false;
    if (groups_[group_num].empty()) return false;

    if (!get_all_objects_) return false;

    auto all = get_all_objects_();

    int total_x = 0, total_y = 0, count = 0;

    for (uint32_t id : groups_[group_num]) {
        for (const auto* obj : all) {
            if (obj && obj->id == id && obj->is_active) {
                total_x += obj->pixel_x;
                total_y += obj->pixel_y;
                count++;
                break;
            }
        }
    }

    if (count == 0) return false;

    out_x = total_x / count;
    out_y = total_y / count;
    return true;
}

//=============================================================================
// Object Lifecycle
//=============================================================================

void SelectionManager::OnObjectDestroyed(SelectableObject* obj) {
    if (!obj) return;
    OnObjectDestroyed(obj->id);
}

void SelectionManager::OnObjectDestroyed(uint32_t object_id) {
    // Remove from selection
    auto it = std::remove_if(selected_.begin(), selected_.end(),
        [object_id](const SelectableObject* obj) {
            return obj && obj->id == object_id;
        });

    if (it != selected_.end()) {
        selected_.erase(it, selected_.end());
        NotifySelectionChanged(SelectionEvent::REMOVED);
    }

    // Remove from all control groups
    for (auto& group : groups_) {
        auto git = std::remove(group.begin(), group.end(), object_id);
        group.erase(git, group.end());
    }
}

//=============================================================================
// Private Methods
//=============================================================================

void SelectionManager::NotifySelectionChanged(SelectionEvent event) {
    if (on_selection_changed_) {
        on_selection_changed_(event);
    }
}

bool SelectionManager::CanSelect(const SelectableObject* obj) const {
    if (!obj) return false;
    if (!obj->is_active) return false;
    if (obj->owner != player_house_) return false;
    return true;
}

void SelectionManager::SortSelection() {
    std::sort(selected_.begin(), selected_.end(), SelectionPriorityCompare);
}

void SelectionManager::TrimSelection() {
    while (selected_.size() > MAX_SELECTION) {
        selected_.pop_back();
    }
}

//=============================================================================
// Global Functions
//=============================================================================

bool SelectionManager_Init() {
    return SelectionManager::Instance().Initialize();
}

void SelectionManager_Shutdown() {
    SelectionManager::Instance().Shutdown();
}

void Selection_Clear() {
    SelectionManager::Instance().Clear();
}

void Selection_Select(void* obj) {
    SelectionManager::Instance().Select(static_cast<SelectableObject*>(obj));
}

void Selection_Add(void* obj) {
    SelectionManager::Instance().AddToSelection(static_cast<SelectableObject*>(obj));
}

void Selection_Remove(void* obj) {
    SelectionManager::Instance().RemoveFromSelection(static_cast<SelectableObject*>(obj));
}

void Selection_SelectInBox(int x1, int y1, int x2, int y2) {
    SelectionManager::Instance().SelectInBox(x1, y1, x2, y2);
}

bool Selection_IsSelected(void* obj) {
    return SelectionManager::Instance().IsSelected(static_cast<SelectableObject*>(obj));
}

bool Selection_HasSelection() {
    return SelectionManager::Instance().HasSelection();
}

int Selection_GetCount() {
    return SelectionManager::Instance().GetSelectionCount();
}

void Selection_SaveGroup(int group) {
    SelectionManager::Instance().SaveGroup(group);
}

void Selection_RecallGroup(int group) {
    SelectionManager::Instance().RecallGroup(group);
}
```

### tests/test_selection.cpp

```cpp
// tests/test_selection.cpp
// Test program for selection system
// Task 16e - Selection System

#include "selection_manager.h"
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

    printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);

    return (failed == 0) ? 0 : 1;
}
```

---

## CMakeLists.txt Additions

Add to `src/game/CMakeLists.txt`:

```cmake
# Selection System (Task 16e)
target_sources(game PRIVATE
    input/selection_manager.h
    input/selection_manager.cpp
)
```

Add test target:

```cmake
# Selection test
add_executable(test_selection
    tests/test_selection.cpp
    src/game/input/selection_manager.cpp
)
target_include_directories(test_selection PRIVATE
    src/game/input
    src/platform
)
target_link_libraries(test_selection platform)
```

---

## Verification Script

```bash
#!/bin/bash
set -e

echo "=== Task 16e Verification ==="

cd build
cmake --build . --target test_selection

echo "Running tests..."
./tests/test_selection

echo "Checking files..."
for file in \
    "../src/game/input/selection_manager.h" \
    "../src/game/input/selection_manager.cpp"
do
    if [ ! -f "$file" ]; then
        echo "ERROR: Missing $file"
        exit 1
    fi
done

echo ""
echo "=== Task 16e Verification PASSED ==="
```

---

## Implementation Steps

1. **Create selection_manager.h** - Define manager class with all methods
2. **Create selection_manager.cpp** - Implement selection logic
3. **Create test_selection.cpp** - Comprehensive tests
4. **Update CMakeLists.txt** - Add files and test target
5. **Build and test** - Verify all pass

---

## Success Criteria

- [ ] Single click selection works
- [ ] Additive (Shift) selection works
- [ ] Toggle selection works
- [ ] Box selection only selects player's units
- [ ] Control groups save and recall correctly
- [ ] MAX_SELECTION limit enforced
- [ ] Destroyed objects removed from selection and groups
- [ ] Enemy objects cannot be selected
- [ ] All unit tests pass

---

## Common Issues

### Selection persists after object destroyed
Make sure to call `OnObjectDestroyed()` when objects are removed from the game.

### Control group recalls wrong objects
Groups store object IDs, not pointers. Ensure the ID lookup is correct.

### Box selection includes buildings
The `SelectInBox` method should filter by `is_unit == true`.

---

## Completion Promise

When verification passes, output:
<promise>TASK_16E_COMPLETE</promise>

## Escape Hatch

If stuck after 15 iterations, output:
<promise>TASK_16E_BLOCKED</promise>

## Max Iterations
15
