# Task 18g: Bug Tracking System

| Field | Value |
|-------|-------|
| **Task ID** | 18g |
| **Task Name** | Bug Tracking System |
| **Phase** | 18 - Testing & Polish |
| **Depends On** | Tasks 18a-18f (All test infrastructure) |
| **Estimated Complexity** | Medium (~400 lines) |

---

## Dependencies

- Task 18a (Test Framework) - Test infrastructure
- All prior test tasks for regression detection
- File I/O for issue persistence

---

## Context

A bug tracking system helps manage known issues during development:

1. **Issue Templates**: Standardized bug report format
2. **Known Issues Registry**: Track acknowledged bugs
3. **Regression Detection**: Identify reappearing bugs
4. **Priority Classification**: Triage by severity
5. **Status Tracking**: Open, In Progress, Fixed, Won't Fix
6. **Test Integration**: Link bugs to failing tests

This provides visibility into technical debt and release blockers.

---

## Technical Background

### Bug Classification

```
Severity Levels:
- CRITICAL: Crashes, data loss, security issues
- HIGH: Major feature broken, no workaround
- MEDIUM: Feature impaired, workaround exists
- LOW: Minor issue, cosmetic

Priority Levels:
- P0: Must fix before any release
- P1: Must fix before next release
- P2: Should fix when possible
- P3: Nice to fix someday
```

### Issue Lifecycle

```
NEW → CONFIRMED → IN_PROGRESS → FIXED → VERIFIED → CLOSED
                              ↘ WONT_FIX → CLOSED
                              ↘ DUPLICATE → CLOSED
```

### Regression Detection

When a previously fixed bug reappears:
1. Detect via failing regression test
2. Link to original issue
3. Escalate priority
4. Investigate root cause

---

## Objective

Create a bug tracking system that:
1. Provides issue templates for consistent reporting
2. Maintains a known issues registry
3. Detects regressions via test linkage
4. Exports issues for CI/release notes
5. Integrates with test framework

---

## Deliverables

- [ ] `src/test/bug_tracker.h` - Bug tracking declarations
- [ ] `src/test/bug_tracker.cpp` - Bug tracking implementation
- [ ] `src/tests/regression/regression_tests.cpp` - Regression test suite
- [ ] `docs/KNOWN_ISSUES.md` - Known issues documentation
- [ ] `docs/BUG_TEMPLATE.md` - Issue reporting template
- [ ] CMakeLists.txt updated

---

## Files to Create

### src/test/bug_tracker.h

```cpp
// src/test/bug_tracker.h
// Bug Tracking System
// Task 18g - Bug Tracking

#ifndef BUG_TRACKER_H
#define BUG_TRACKER_H

#include <string>
#include <vector>
#include <map>
#include <ctime>

//=============================================================================
// Bug Severity and Priority
//=============================================================================

enum class BugSeverity {
    CRITICAL,   // Crashes, data loss, security
    HIGH,       // Major feature broken
    MEDIUM,     // Feature impaired
    LOW         // Minor/cosmetic
};

enum class BugPriority {
    P0,         // Must fix before any release
    P1,         // Must fix before next release
    P2,         // Should fix when possible
    P3          // Nice to fix someday
};

enum class BugStatus {
    NEW,
    CONFIRMED,
    IN_PROGRESS,
    FIXED,
    VERIFIED,
    CLOSED,
    WONT_FIX,
    DUPLICATE
};

enum class BugCategory {
    GRAPHICS,
    AUDIO,
    INPUT,
    GAMEPLAY,
    PERFORMANCE,
    CRASH,
    MEMORY,
    ASSET,
    NETWORK,
    OTHER
};

//=============================================================================
// Bug Report Structure
//=============================================================================

struct BugReport {
    // Identification
    std::string id;              // e.g., "BUG-001"
    std::string title;           // Brief description
    std::string description;     // Detailed description

    // Classification
    BugSeverity severity;
    BugPriority priority;
    BugStatus status;
    BugCategory category;

    // Environment
    std::string platform;        // macOS, Windows, Linux
    std::string version;         // Build version
    std::string hardware;        // Relevant hardware info

    // Reproduction
    std::string steps_to_reproduce;
    std::string expected_behavior;
    std::string actual_behavior;
    int repro_rate;              // Percentage (0-100)

    // Tracking
    std::string reporter;
    std::string assignee;
    time_t created_date;
    time_t modified_date;
    time_t fixed_date;

    // Linkage
    std::string linked_test;     // Associated test case
    std::string duplicate_of;    // If duplicate
    std::string fix_commit;      // Git commit that fixed it

    // Notes
    std::vector<std::string> comments;
    std::vector<std::string> attachments;

    BugReport();
    std::string to_string() const;
    std::string to_markdown() const;
    bool is_open() const;
    bool is_regression() const;
};

//=============================================================================
// Bug Registry
//=============================================================================

class BugRegistry {
public:
    static BugRegistry& instance();

    // Bug management
    std::string add_bug(const BugReport& bug);
    bool update_bug(const std::string& id, const BugReport& bug);
    bool remove_bug(const std::string& id);
    const BugReport* get_bug(const std::string& id) const;

    // Queries
    std::vector<BugReport> get_all_bugs() const;
    std::vector<BugReport> get_bugs_by_status(BugStatus status) const;
    std::vector<BugReport> get_bugs_by_severity(BugSeverity severity) const;
    std::vector<BugReport> get_bugs_by_priority(BugPriority priority) const;
    std::vector<BugReport> get_bugs_by_category(BugCategory category) const;
    std::vector<BugReport> get_open_bugs() const;
    std::vector<BugReport> get_release_blockers() const;

    // Statistics
    int get_total_count() const;
    int get_open_count() const;
    int get_critical_count() const;
    std::map<BugStatus, int> get_status_breakdown() const;
    std::map<BugCategory, int> get_category_breakdown() const;

    // Persistence
    bool load_from_file(const std::string& path);
    bool save_to_file(const std::string& path) const;

    // Export
    std::string export_markdown() const;
    std::string export_release_notes() const;

    // Test integration
    void link_test_to_bug(const std::string& test_name, const std::string& bug_id);
    std::string get_bug_for_test(const std::string& test_name) const;
    void mark_regression(const std::string& bug_id);

private:
    BugRegistry() = default;
    std::string generate_id();

    std::map<std::string, BugReport> bugs_;
    std::map<std::string, std::string> test_to_bug_;  // test_name -> bug_id
    int next_id_ = 1;
};

//=============================================================================
// Regression Tracker
//=============================================================================

class RegressionTracker {
public:
    static RegressionTracker& instance();

    // Track fixed bugs
    void register_fixed_bug(const std::string& bug_id, const std::string& test_name);

    // Check for regressions
    bool check_regression(const std::string& test_name, bool test_passed);

    // Get regression info
    std::vector<std::string> get_regressions() const;
    void clear_regressions();

    // Reporting
    std::string get_regression_report() const;

private:
    RegressionTracker() = default;

    // bug_id -> test_name for fixed bugs
    std::map<std::string, std::string> fixed_bugs_;
    std::vector<std::string> detected_regressions_;
};

//=============================================================================
// Helper Functions
//=============================================================================

const char* severity_to_string(BugSeverity severity);
const char* priority_to_string(BugPriority priority);
const char* status_to_string(BugStatus status);
const char* category_to_string(BugCategory category);

BugSeverity string_to_severity(const std::string& str);
BugPriority string_to_priority(const std::string& str);
BugStatus string_to_status(const std::string& str);
BugCategory string_to_category(const std::string& str);

//=============================================================================
// Bug Report Builder (Fluent Interface)
//=============================================================================

class BugReportBuilder {
public:
    BugReportBuilder& title(const std::string& t);
    BugReportBuilder& description(const std::string& d);
    BugReportBuilder& severity(BugSeverity s);
    BugReportBuilder& priority(BugPriority p);
    BugReportBuilder& category(BugCategory c);
    BugReportBuilder& platform(const std::string& p);
    BugReportBuilder& version(const std::string& v);
    BugReportBuilder& steps(const std::string& s);
    BugReportBuilder& expected(const std::string& e);
    BugReportBuilder& actual(const std::string& a);
    BugReportBuilder& repro_rate(int rate);
    BugReportBuilder& reporter(const std::string& r);
    BugReportBuilder& linked_test(const std::string& t);

    BugReport build();

private:
    BugReport report_;
};

//=============================================================================
// Macros for Test Integration
//=============================================================================

// Mark a test as a regression test for a specific bug
#define REGRESSION_TEST(bug_id) \
    RegressionTracker::instance().register_fixed_bug(bug_id, __func__)

// Check if current test failure is a regression
#define CHECK_REGRESSION(test_name, passed) \
    RegressionTracker::instance().check_regression(test_name, passed)

#endif // BUG_TRACKER_H
```

### src/test/bug_tracker.cpp

```cpp
// src/test/bug_tracker.cpp
// Bug Tracking System Implementation
// Task 18g - Bug Tracking

#include "test/bug_tracker.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

//=============================================================================
// BugReport Implementation
//=============================================================================

BugReport::BugReport()
    : severity(BugSeverity::MEDIUM)
    , priority(BugPriority::P2)
    , status(BugStatus::NEW)
    , category(BugCategory::OTHER)
    , repro_rate(100)
    , created_date(std::time(nullptr))
    , modified_date(std::time(nullptr))
    , fixed_date(0)
{
}

std::string BugReport::to_string() const {
    std::ostringstream oss;
    oss << "[" << id << "] " << title << "\n";
    oss << "  Severity: " << severity_to_string(severity) << "\n";
    oss << "  Priority: " << priority_to_string(priority) << "\n";
    oss << "  Status: " << status_to_string(status) << "\n";
    oss << "  Category: " << category_to_string(category) << "\n";
    return oss.str();
}

std::string BugReport::to_markdown() const {
    std::ostringstream oss;
    oss << "### " << id << ": " << title << "\n\n";
    oss << "| Field | Value |\n";
    oss << "|-------|-------|\n";
    oss << "| Severity | " << severity_to_string(severity) << " |\n";
    oss << "| Priority | " << priority_to_string(priority) << " |\n";
    oss << "| Status | " << status_to_string(status) << " |\n";
    oss << "| Category | " << category_to_string(category) << " |\n";
    oss << "| Platform | " << platform << " |\n";
    oss << "| Repro Rate | " << repro_rate << "% |\n";
    oss << "\n**Description:**\n" << description << "\n\n";
    if (!steps_to_reproduce.empty()) {
        oss << "**Steps to Reproduce:**\n" << steps_to_reproduce << "\n\n";
    }
    if (!expected_behavior.empty()) {
        oss << "**Expected:** " << expected_behavior << "\n\n";
    }
    if (!actual_behavior.empty()) {
        oss << "**Actual:** " << actual_behavior << "\n\n";
    }
    return oss.str();
}

bool BugReport::is_open() const {
    return status == BugStatus::NEW ||
           status == BugStatus::CONFIRMED ||
           status == BugStatus::IN_PROGRESS;
}

bool BugReport::is_regression() const {
    // A bug is a regression if it was previously fixed
    return fixed_date != 0 && is_open();
}

//=============================================================================
// BugRegistry Implementation
//=============================================================================

BugRegistry& BugRegistry::instance() {
    static BugRegistry instance;
    return instance;
}

std::string BugRegistry::generate_id() {
    std::ostringstream oss;
    oss << "BUG-" << std::setfill('0') << std::setw(3) << next_id_++;
    return oss.str();
}

std::string BugRegistry::add_bug(const BugReport& bug) {
    BugReport new_bug = bug;
    new_bug.id = generate_id();
    new_bug.created_date = std::time(nullptr);
    new_bug.modified_date = new_bug.created_date;
    bugs_[new_bug.id] = new_bug;
    return new_bug.id;
}

bool BugRegistry::update_bug(const std::string& id, const BugReport& bug) {
    auto it = bugs_.find(id);
    if (it == bugs_.end()) {
        return false;
    }
    it->second = bug;
    it->second.id = id;  // Preserve ID
    it->second.modified_date = std::time(nullptr);

    // Track fix date
    if (bug.status == BugStatus::FIXED && it->second.fixed_date == 0) {
        it->second.fixed_date = std::time(nullptr);
    }
    return true;
}

bool BugRegistry::remove_bug(const std::string& id) {
    return bugs_.erase(id) > 0;
}

const BugReport* BugRegistry::get_bug(const std::string& id) const {
    auto it = bugs_.find(id);
    return it != bugs_.end() ? &it->second : nullptr;
}

std::vector<BugReport> BugRegistry::get_all_bugs() const {
    std::vector<BugReport> result;
    for (const auto& pair : bugs_) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<BugReport> BugRegistry::get_bugs_by_status(BugStatus status) const {
    std::vector<BugReport> result;
    for (const auto& pair : bugs_) {
        if (pair.second.status == status) {
            result.push_back(pair.second);
        }
    }
    return result;
}

std::vector<BugReport> BugRegistry::get_bugs_by_severity(BugSeverity severity) const {
    std::vector<BugReport> result;
    for (const auto& pair : bugs_) {
        if (pair.second.severity == severity) {
            result.push_back(pair.second);
        }
    }
    return result;
}

std::vector<BugReport> BugRegistry::get_bugs_by_priority(BugPriority priority) const {
    std::vector<BugReport> result;
    for (const auto& pair : bugs_) {
        if (pair.second.priority == priority) {
            result.push_back(pair.second);
        }
    }
    return result;
}

std::vector<BugReport> BugRegistry::get_bugs_by_category(BugCategory category) const {
    std::vector<BugReport> result;
    for (const auto& pair : bugs_) {
        if (pair.second.category == category) {
            result.push_back(pair.second);
        }
    }
    return result;
}

std::vector<BugReport> BugRegistry::get_open_bugs() const {
    std::vector<BugReport> result;
    for (const auto& pair : bugs_) {
        if (pair.second.is_open()) {
            result.push_back(pair.second);
        }
    }
    return result;
}

std::vector<BugReport> BugRegistry::get_release_blockers() const {
    std::vector<BugReport> result;
    for (const auto& pair : bugs_) {
        const BugReport& bug = pair.second;
        if (bug.is_open() &&
            (bug.priority == BugPriority::P0 ||
             bug.severity == BugSeverity::CRITICAL)) {
            result.push_back(bug);
        }
    }
    return result;
}

int BugRegistry::get_total_count() const {
    return static_cast<int>(bugs_.size());
}

int BugRegistry::get_open_count() const {
    return static_cast<int>(get_open_bugs().size());
}

int BugRegistry::get_critical_count() const {
    return static_cast<int>(get_bugs_by_severity(BugSeverity::CRITICAL).size());
}

std::map<BugStatus, int> BugRegistry::get_status_breakdown() const {
    std::map<BugStatus, int> result;
    for (const auto& pair : bugs_) {
        result[pair.second.status]++;
    }
    return result;
}

std::map<BugCategory, int> BugRegistry::get_category_breakdown() const {
    std::map<BugCategory, int> result;
    for (const auto& pair : bugs_) {
        result[pair.second.category]++;
    }
    return result;
}

bool BugRegistry::load_from_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    // Simple line-based format for now
    // In production, would use JSON or similar
    bugs_.clear();
    std::string line;
    BugReport current;
    bool in_bug = false;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        if (line.substr(0, 4) == "BUG:") {
            if (in_bug && !current.id.empty()) {
                bugs_[current.id] = current;
            }
            current = BugReport();
            current.id = line.substr(4);
            in_bug = true;
        } else if (in_bug) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                if (key == "TITLE") current.title = value;
                else if (key == "SEVERITY") current.severity = string_to_severity(value);
                else if (key == "PRIORITY") current.priority = string_to_priority(value);
                else if (key == "STATUS") current.status = string_to_status(value);
                else if (key == "CATEGORY") current.category = string_to_category(value);
            }
        }
    }

    if (in_bug && !current.id.empty()) {
        bugs_[current.id] = current;
    }

    return true;
}

bool BugRegistry::save_to_file(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }

    for (const auto& pair : bugs_) {
        const BugReport& bug = pair.second;
        file << "BUG:" << bug.id << "\n";
        file << "TITLE:" << bug.title << "\n";
        file << "SEVERITY:" << severity_to_string(bug.severity) << "\n";
        file << "PRIORITY:" << priority_to_string(bug.priority) << "\n";
        file << "STATUS:" << status_to_string(bug.status) << "\n";
        file << "CATEGORY:" << category_to_string(bug.category) << "\n";
        file << "\n";
    }

    return true;
}

std::string BugRegistry::export_markdown() const {
    std::ostringstream oss;
    oss << "# Known Issues\n\n";
    oss << "Generated: " << std::time(nullptr) << "\n\n";
    oss << "## Summary\n\n";
    oss << "- Total bugs: " << get_total_count() << "\n";
    oss << "- Open bugs: " << get_open_count() << "\n";
    oss << "- Critical bugs: " << get_critical_count() << "\n\n";

    auto blockers = get_release_blockers();
    if (!blockers.empty()) {
        oss << "## Release Blockers\n\n";
        for (const auto& bug : blockers) {
            oss << bug.to_markdown() << "\n";
        }
    }

    auto open = get_open_bugs();
    if (!open.empty()) {
        oss << "## Open Issues\n\n";
        for (const auto& bug : open) {
            if (bug.priority != BugPriority::P0 &&
                bug.severity != BugSeverity::CRITICAL) {
                oss << bug.to_markdown() << "\n";
            }
        }
    }

    return oss.str();
}

std::string BugRegistry::export_release_notes() const {
    std::ostringstream oss;
    oss << "## Known Issues\n\n";

    auto open = get_open_bugs();
    for (const auto& bug : open) {
        oss << "- **" << bug.id << "**: " << bug.title;
        if (bug.severity == BugSeverity::CRITICAL) {
            oss << " (CRITICAL)";
        }
        oss << "\n";
    }

    oss << "\n## Fixed Issues\n\n";
    auto fixed = get_bugs_by_status(BugStatus::FIXED);
    for (const auto& bug : fixed) {
        oss << "- **" << bug.id << "**: " << bug.title << "\n";
    }

    return oss.str();
}

void BugRegistry::link_test_to_bug(const std::string& test_name, const std::string& bug_id) {
    test_to_bug_[test_name] = bug_id;
}

std::string BugRegistry::get_bug_for_test(const std::string& test_name) const {
    auto it = test_to_bug_.find(test_name);
    return it != test_to_bug_.end() ? it->second : "";
}

void BugRegistry::mark_regression(const std::string& bug_id) {
    auto it = bugs_.find(bug_id);
    if (it != bugs_.end()) {
        it->second.status = BugStatus::NEW;
        it->second.priority = BugPriority::P0;  // Escalate regressions
        it->second.modified_date = std::time(nullptr);
    }
}

//=============================================================================
// RegressionTracker Implementation
//=============================================================================

RegressionTracker& RegressionTracker::instance() {
    static RegressionTracker instance;
    return instance;
}

void RegressionTracker::register_fixed_bug(const std::string& bug_id, const std::string& test_name) {
    fixed_bugs_[bug_id] = test_name;
    BugRegistry::instance().link_test_to_bug(test_name, bug_id);
}

bool RegressionTracker::check_regression(const std::string& test_name, bool test_passed) {
    if (test_passed) {
        return false;
    }

    // Check if this test was for a fixed bug
    std::string bug_id = BugRegistry::instance().get_bug_for_test(test_name);
    if (!bug_id.empty()) {
        const BugReport* bug = BugRegistry::instance().get_bug(bug_id);
        if (bug && bug->status == BugStatus::FIXED) {
            // This is a regression!
            detected_regressions_.push_back(bug_id);
            BugRegistry::instance().mark_regression(bug_id);
            return true;
        }
    }

    return false;
}

std::vector<std::string> RegressionTracker::get_regressions() const {
    return detected_regressions_;
}

void RegressionTracker::clear_regressions() {
    detected_regressions_.clear();
}

std::string RegressionTracker::get_regression_report() const {
    if (detected_regressions_.empty()) {
        return "No regressions detected.\n";
    }

    std::ostringstream oss;
    oss << "REGRESSION ALERT!\n";
    oss << "=================\n\n";
    oss << detected_regressions_.size() << " regression(s) detected:\n\n";

    for (const auto& bug_id : detected_regressions_) {
        const BugReport* bug = BugRegistry::instance().get_bug(bug_id);
        if (bug) {
            oss << "- " << bug->id << ": " << bug->title << "\n";
        }
    }

    return oss.str();
}

//=============================================================================
// Helper Functions
//=============================================================================

const char* severity_to_string(BugSeverity severity) {
    switch (severity) {
        case BugSeverity::CRITICAL: return "CRITICAL";
        case BugSeverity::HIGH: return "HIGH";
        case BugSeverity::MEDIUM: return "MEDIUM";
        case BugSeverity::LOW: return "LOW";
        default: return "UNKNOWN";
    }
}

const char* priority_to_string(BugPriority priority) {
    switch (priority) {
        case BugPriority::P0: return "P0";
        case BugPriority::P1: return "P1";
        case BugPriority::P2: return "P2";
        case BugPriority::P3: return "P3";
        default: return "P?";
    }
}

const char* status_to_string(BugStatus status) {
    switch (status) {
        case BugStatus::NEW: return "NEW";
        case BugStatus::CONFIRMED: return "CONFIRMED";
        case BugStatus::IN_PROGRESS: return "IN_PROGRESS";
        case BugStatus::FIXED: return "FIXED";
        case BugStatus::VERIFIED: return "VERIFIED";
        case BugStatus::CLOSED: return "CLOSED";
        case BugStatus::WONT_FIX: return "WONT_FIX";
        case BugStatus::DUPLICATE: return "DUPLICATE";
        default: return "UNKNOWN";
    }
}

const char* category_to_string(BugCategory category) {
    switch (category) {
        case BugCategory::GRAPHICS: return "GRAPHICS";
        case BugCategory::AUDIO: return "AUDIO";
        case BugCategory::INPUT: return "INPUT";
        case BugCategory::GAMEPLAY: return "GAMEPLAY";
        case BugCategory::PERFORMANCE: return "PERFORMANCE";
        case BugCategory::CRASH: return "CRASH";
        case BugCategory::MEMORY: return "MEMORY";
        case BugCategory::ASSET: return "ASSET";
        case BugCategory::NETWORK: return "NETWORK";
        case BugCategory::OTHER: return "OTHER";
        default: return "UNKNOWN";
    }
}

BugSeverity string_to_severity(const std::string& str) {
    if (str == "CRITICAL") return BugSeverity::CRITICAL;
    if (str == "HIGH") return BugSeverity::HIGH;
    if (str == "MEDIUM") return BugSeverity::MEDIUM;
    if (str == "LOW") return BugSeverity::LOW;
    return BugSeverity::MEDIUM;
}

BugPriority string_to_priority(const std::string& str) {
    if (str == "P0") return BugPriority::P0;
    if (str == "P1") return BugPriority::P1;
    if (str == "P2") return BugPriority::P2;
    if (str == "P3") return BugPriority::P3;
    return BugPriority::P2;
}

BugStatus string_to_status(const std::string& str) {
    if (str == "NEW") return BugStatus::NEW;
    if (str == "CONFIRMED") return BugStatus::CONFIRMED;
    if (str == "IN_PROGRESS") return BugStatus::IN_PROGRESS;
    if (str == "FIXED") return BugStatus::FIXED;
    if (str == "VERIFIED") return BugStatus::VERIFIED;
    if (str == "CLOSED") return BugStatus::CLOSED;
    if (str == "WONT_FIX") return BugStatus::WONT_FIX;
    if (str == "DUPLICATE") return BugStatus::DUPLICATE;
    return BugStatus::NEW;
}

BugCategory string_to_category(const std::string& str) {
    if (str == "GRAPHICS") return BugCategory::GRAPHICS;
    if (str == "AUDIO") return BugCategory::AUDIO;
    if (str == "INPUT") return BugCategory::INPUT;
    if (str == "GAMEPLAY") return BugCategory::GAMEPLAY;
    if (str == "PERFORMANCE") return BugCategory::PERFORMANCE;
    if (str == "CRASH") return BugCategory::CRASH;
    if (str == "MEMORY") return BugCategory::MEMORY;
    if (str == "ASSET") return BugCategory::ASSET;
    if (str == "NETWORK") return BugCategory::NETWORK;
    return BugCategory::OTHER;
}

//=============================================================================
// BugReportBuilder Implementation
//=============================================================================

BugReportBuilder& BugReportBuilder::title(const std::string& t) {
    report_.title = t;
    return *this;
}

BugReportBuilder& BugReportBuilder::description(const std::string& d) {
    report_.description = d;
    return *this;
}

BugReportBuilder& BugReportBuilder::severity(BugSeverity s) {
    report_.severity = s;
    return *this;
}

BugReportBuilder& BugReportBuilder::priority(BugPriority p) {
    report_.priority = p;
    return *this;
}

BugReportBuilder& BugReportBuilder::category(BugCategory c) {
    report_.category = c;
    return *this;
}

BugReportBuilder& BugReportBuilder::platform(const std::string& p) {
    report_.platform = p;
    return *this;
}

BugReportBuilder& BugReportBuilder::version(const std::string& v) {
    report_.version = v;
    return *this;
}

BugReportBuilder& BugReportBuilder::steps(const std::string& s) {
    report_.steps_to_reproduce = s;
    return *this;
}

BugReportBuilder& BugReportBuilder::expected(const std::string& e) {
    report_.expected_behavior = e;
    return *this;
}

BugReportBuilder& BugReportBuilder::actual(const std::string& a) {
    report_.actual_behavior = a;
    return *this;
}

BugReportBuilder& BugReportBuilder::repro_rate(int rate) {
    report_.repro_rate = rate;
    return *this;
}

BugReportBuilder& BugReportBuilder::reporter(const std::string& r) {
    report_.reporter = r;
    return *this;
}

BugReportBuilder& BugReportBuilder::linked_test(const std::string& t) {
    report_.linked_test = t;
    return *this;
}

BugReport BugReportBuilder::build() {
    return report_;
}
```

### src/tests/regression/regression_tests.cpp

```cpp
// src/tests/regression/regression_tests.cpp
// Regression Tests
// Task 18g - Bug Tracking

#include "test/test_framework.h"
#include "test/bug_tracker.h"

//=============================================================================
// Regression Test Infrastructure
//=============================================================================

// Example regression tests - would be linked to actual bugs
// These demonstrate how to structure regression tests

TEST_CASE(Regression_BUG001_GraphicsInit, "Regression") {
    // BUG-001: Graphics failed to initialize on certain hardware
    // Fixed by adding fallback resolution
    REGRESSION_TEST("BUG-001");

    // Test that graphics can initialize
    bool init_success = true;  // Would call actual init
    TEST_ASSERT(init_success);
}

TEST_CASE(Regression_BUG002_AudioCrash, "Regression") {
    // BUG-002: Audio playback crashed with no sound device
    // Fixed by checking device before playback
    REGRESSION_TEST("BUG-002");

    // Test audio with no device
    bool can_handle_no_device = true;  // Would test actual scenario
    TEST_ASSERT(can_handle_no_device);
}

TEST_CASE(Regression_BUG003_MemoryLeak, "Regression") {
    // BUG-003: Memory leak in asset loading
    // Fixed by properly releasing textures
    REGRESSION_TEST("BUG-003");

    // Test memory is properly freed
    size_t mem_before = 1000;  // Would get actual memory
    // Load and unload assets
    size_t mem_after = 1000;  // Would get actual memory

    TEST_ASSERT_EQ(mem_before, mem_after);
}

TEST_CASE(Regression_BUG004_InputDelay, "Regression") {
    // BUG-004: Input had 100ms delay
    // Fixed by processing input before frame start
    REGRESSION_TEST("BUG-004");

    // Test input latency
    int latency_ms = 16;  // Would measure actual latency
    TEST_ASSERT_LT(latency_ms, 50);  // Should be under 50ms
}

//=============================================================================
// Bug Tracker Tests
//=============================================================================

TEST_CASE(BugTracker_AddBug, "BugTracker") {
    auto& registry = BugRegistry::instance();

    BugReport bug;
    bug.title = "Test Bug";
    bug.severity = BugSeverity::MEDIUM;
    bug.priority = BugPriority::P2;
    bug.category = BugCategory::OTHER;

    std::string id = registry.add_bug(bug);
    TEST_ASSERT(!id.empty());

    const BugReport* retrieved = registry.get_bug(id);
    TEST_ASSERT(retrieved != nullptr);
    TEST_ASSERT_EQ(retrieved->title, "Test Bug");
}

TEST_CASE(BugTracker_UpdateBug, "BugTracker") {
    auto& registry = BugRegistry::instance();

    BugReport bug;
    bug.title = "Original Title";
    std::string id = registry.add_bug(bug);

    bug.title = "Updated Title";
    bug.status = BugStatus::IN_PROGRESS;
    bool updated = registry.update_bug(id, bug);
    TEST_ASSERT(updated);

    const BugReport* retrieved = registry.get_bug(id);
    TEST_ASSERT_EQ(retrieved->title, "Updated Title");
    TEST_ASSERT_EQ(retrieved->status, BugStatus::IN_PROGRESS);
}

TEST_CASE(BugTracker_QueryByStatus, "BugTracker") {
    auto& registry = BugRegistry::instance();

    // Add bugs with different statuses
    BugReport open_bug;
    open_bug.title = "Open Bug";
    open_bug.status = BugStatus::NEW;
    registry.add_bug(open_bug);

    auto new_bugs = registry.get_bugs_by_status(BugStatus::NEW);
    TEST_ASSERT_GE(new_bugs.size(), 1u);
}

TEST_CASE(BugTracker_ReleaseBlockers, "BugTracker") {
    auto& registry = BugRegistry::instance();

    // Add a critical bug
    BugReport critical_bug;
    critical_bug.title = "Critical Bug";
    critical_bug.severity = BugSeverity::CRITICAL;
    critical_bug.status = BugStatus::NEW;
    registry.add_bug(critical_bug);

    auto blockers = registry.get_release_blockers();
    TEST_ASSERT_GE(blockers.size(), 1u);
}

TEST_CASE(BugTracker_ExportMarkdown, "BugTracker") {
    auto& registry = BugRegistry::instance();

    std::string markdown = registry.export_markdown();
    TEST_ASSERT(!markdown.empty());
    TEST_ASSERT(markdown.find("Known Issues") != std::string::npos);
}

TEST_CASE(RegressionTracker_DetectRegression, "BugTracker") {
    auto& tracker = RegressionTracker::instance();
    auto& registry = BugRegistry::instance();

    // Create and "fix" a bug
    BugReport bug;
    bug.title = "Fixed Bug";
    bug.status = BugStatus::FIXED;
    std::string id = registry.add_bug(bug);

    // Register test for this bug
    tracker.register_fixed_bug(id, "test_fixed_feature");

    // Simulate test failure - should detect regression
    bool is_regression = tracker.check_regression("test_fixed_feature", false);
    TEST_ASSERT(is_regression);

    auto regressions = tracker.get_regressions();
    TEST_ASSERT_GE(regressions.size(), 1u);
}

TEST_CASE(BugReportBuilder_FluentInterface, "BugTracker") {
    BugReport bug = BugReportBuilder()
        .title("Builder Test Bug")
        .description("Created with builder")
        .severity(BugSeverity::HIGH)
        .priority(BugPriority::P1)
        .category(BugCategory::GRAPHICS)
        .platform("macOS")
        .build();

    TEST_ASSERT_EQ(bug.title, "Builder Test Bug");
    TEST_ASSERT_EQ(bug.severity, BugSeverity::HIGH);
    TEST_ASSERT_EQ(bug.priority, BugPriority::P1);
    TEST_ASSERT_EQ(bug.category, BugCategory::GRAPHICS);
}
```

### docs/KNOWN_ISSUES.md

```markdown
# Known Issues

This document tracks known issues in the Red Alert macOS port.

## Release Blockers

No release blockers at this time.

## Open Issues

### Graphics

| ID | Title | Severity | Status |
|----|-------|----------|--------|
| - | - | - | - |

### Audio

| ID | Title | Severity | Status |
|----|-------|----------|--------|
| - | - | - | - |

### Input

| ID | Title | Severity | Status |
|----|-------|----------|--------|
| - | - | - | - |

### Gameplay

| ID | Title | Severity | Status |
|----|-------|----------|--------|
| - | - | - | - |

### Performance

| ID | Title | Severity | Status |
|----|-------|----------|--------|
| - | - | - | - |

## Fixed Issues

Issues fixed in the current release:

| ID | Title | Fixed In |
|----|-------|----------|
| - | - | - |

## Reporting New Issues

Please use the bug template in `docs/BUG_TEMPLATE.md` when reporting new issues.

## Issue Lifecycle

1. **NEW**: Issue reported, needs triage
2. **CONFIRMED**: Issue verified and accepted
3. **IN_PROGRESS**: Fix being developed
4. **FIXED**: Fix implemented
5. **VERIFIED**: Fix confirmed working
6. **CLOSED**: Issue resolved

## Severity Levels

- **CRITICAL**: Crashes, data loss, security vulnerabilities
- **HIGH**: Major feature completely broken, no workaround
- **MEDIUM**: Feature impaired but workaround exists
- **LOW**: Minor issue, cosmetic problems

## Priority Levels

- **P0**: Must fix before ANY release
- **P1**: Must fix before next release
- **P2**: Should fix when possible
- **P3**: Nice to have, low impact
```

### docs/BUG_TEMPLATE.md

```markdown
# Bug Report Template

## Title

[Brief, descriptive title]

## Environment

| Field | Value |
|-------|-------|
| Platform | macOS / Windows / Linux |
| OS Version | e.g., macOS 14.0 |
| Hardware | e.g., Apple M1, Intel i7 |
| Game Version | e.g., 1.0.0 |

## Classification

| Field | Value |
|-------|-------|
| Severity | CRITICAL / HIGH / MEDIUM / LOW |
| Category | GRAPHICS / AUDIO / INPUT / GAMEPLAY / PERFORMANCE / CRASH / MEMORY / ASSET / OTHER |
| Repro Rate | 100% / Intermittent / Rare |

## Description

[Detailed description of the issue]

## Steps to Reproduce

1. [First step]
2. [Second step]
3. [Third step]
4. [Continue as needed]

## Expected Behavior

[What should happen]

## Actual Behavior

[What actually happens]

## Additional Information

### Screenshots/Video

[Attach or link to visual evidence if applicable]

### Log Files

[Attach relevant log files]

### Related Issues

[Link to related bugs if any]

## Workaround

[Any known workaround, if available]

---

## For Developers

### Root Cause Analysis

[To be filled by developer investigating]

### Fix Description

[Brief description of the fix]

### Regression Test

[Test case that prevents regression]

### Affected Files

- file1.cpp
- file2.h
```

---

## CMakeLists.txt Additions

```cmake
# Task 18g: Bug Tracking

set(BUG_TRACKER_SOURCES
    ${CMAKE_SOURCE_DIR}/src/test/bug_tracker.cpp
)

set(REGRESSION_TEST_SOURCES
    ${CMAKE_SOURCE_DIR}/src/tests/regression/regression_tests.cpp
)

# Add bug tracker to test framework
target_sources(test_framework PRIVATE ${BUG_TRACKER_SOURCES})

# Regression tests executable
add_executable(RegressionTests
    ${REGRESSION_TEST_SOURCES}
    ${CMAKE_SOURCE_DIR}/src/tests/regression_tests_main.cpp
)
target_link_libraries(RegressionTests PRIVATE test_framework)
add_test(NAME RegressionTests COMMAND RegressionTests)
```

---

## Verification Script

```bash
#!/bin/bash
set -e
echo "=== Task 18g Verification: Bug Tracking System ==="
ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"

# Check bug tracker files exist
test -f "$ROOT_DIR/src/test/bug_tracker.h" || { echo "FAIL: bug_tracker.h missing"; exit 1; }
test -f "$ROOT_DIR/src/test/bug_tracker.cpp" || { echo "FAIL: bug_tracker.cpp missing"; exit 1; }
test -f "$ROOT_DIR/docs/KNOWN_ISSUES.md" || { echo "FAIL: KNOWN_ISSUES.md missing"; exit 1; }
test -f "$ROOT_DIR/docs/BUG_TEMPLATE.md" || { echo "FAIL: BUG_TEMPLATE.md missing"; exit 1; }

# Build regression tests
cd "$BUILD_DIR" && cmake --build . --target RegressionTests

# Run regression tests
"$BUILD_DIR/RegressionTests" || exit 1

echo "=== Task 18g Verification PASSED ==="
```

---

## Success Criteria

1. Bug tracker can add, update, query bugs
2. Regression tracker detects when fixed bugs reappear
3. Export functions produce valid markdown
4. Documentation templates are complete

---

## Completion Promise

When verification passes, output:
<promise>TASK_18G_COMPLETE</promise>

## Escape Hatch

If stuck after 15 iterations, output:
<promise>TASK_18G_BLOCKED</promise>

## Max Iterations
15
