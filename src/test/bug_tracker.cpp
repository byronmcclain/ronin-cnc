// src/test/bug_tracker.cpp
// Bug Tracking System Implementation
// Task 18g - Bug Tracking

#include "test/bug_tracker.h"
#include <fstream>
#include <sstream>
#include <iomanip>

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

void BugRegistry::reset() {
    bugs_.clear();
    test_to_bug_.clear();
    next_id_ = 1;
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

void RegressionTracker::reset() {
    fixed_bugs_.clear();
    detected_regressions_.clear();
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
