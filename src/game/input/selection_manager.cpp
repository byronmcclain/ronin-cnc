// src/game/input/selection_manager.cpp
// Selection management implementation
// Task 16e - Selection System

#include "game/input/selection_manager.h"
#include "platform.h"
#include <algorithm>
#include <cstdio>

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

    Platform_LogInfo("SelectionManager: Initializing...");

    selected_.clear();
    selected_.reserve(MAX_SELECTION);

    // Clear all control groups
    for (auto& group : groups_) {
        group.clear();
    }

    initialized_ = true;
    Platform_LogInfo("SelectionManager: Initialized");
    return true;
}

void SelectionManager::Shutdown() {
    if (!initialized_) return;

    Platform_LogInfo("SelectionManager: Shutting down");

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
    if (static_cast<int>(selected_.size()) >= MAX_SELECTION) return;

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
        Platform_LogInfo("SelectionManager: No objects_in_rect query set");
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
        if (static_cast<int>(selected_.size()) >= MAX_SELECTION) break;
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
            if (static_cast<int>(selected_.size()) >= MAX_SELECTION) break;
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

    char msg[128];
    snprintf(msg, sizeof(msg), "SelectionManager: Saved group %d with %d objects",
             group_num, static_cast<int>(groups_[group_num].size()));
    Platform_LogInfo(msg);

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
        if (static_cast<int>(selected_.size()) >= MAX_SELECTION) break;

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
    while (static_cast<int>(selected_.size()) > MAX_SELECTION) {
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
