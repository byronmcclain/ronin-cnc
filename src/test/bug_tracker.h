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

    // Reset for testing
    void reset();

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

    // Reset for testing
    void reset();

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
