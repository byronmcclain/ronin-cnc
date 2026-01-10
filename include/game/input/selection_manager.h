// include/game/input/selection_manager.h
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
