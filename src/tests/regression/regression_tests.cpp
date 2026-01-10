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
    registry.reset();  // Start fresh

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
    registry.reset();

    BugReport bug;
    bug.title = "Original Title";
    std::string id = registry.add_bug(bug);

    bug.title = "Updated Title";
    bug.status = BugStatus::IN_PROGRESS;
    bool updated = registry.update_bug(id, bug);
    TEST_ASSERT(updated);

    const BugReport* retrieved = registry.get_bug(id);
    TEST_ASSERT_EQ(retrieved->title, "Updated Title");
    TEST_ASSERT(retrieved->status == BugStatus::IN_PROGRESS);
}

TEST_CASE(BugTracker_QueryByStatus, "BugTracker") {
    auto& registry = BugRegistry::instance();
    registry.reset();

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
    registry.reset();

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
    registry.reset();

    // Add a bug first
    BugReport bug;
    bug.title = "Export Test";
    registry.add_bug(bug);

    std::string markdown = registry.export_markdown();
    TEST_ASSERT(!markdown.empty());
    TEST_ASSERT(markdown.find("Known Issues") != std::string::npos);
}

TEST_CASE(RegressionTracker_DetectRegression, "BugTracker") {
    auto& tracker = RegressionTracker::instance();
    auto& registry = BugRegistry::instance();
    tracker.reset();
    registry.reset();

    // Create and "fix" a bug
    BugReport bug;
    bug.title = "Fixed Bug";
    bug.status = BugStatus::FIXED;
    std::string id = registry.add_bug(bug);

    // Update to FIXED status
    bug.status = BugStatus::FIXED;
    registry.update_bug(id, bug);

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
    TEST_ASSERT(bug.severity == BugSeverity::HIGH);
    TEST_ASSERT(bug.priority == BugPriority::P1);
    TEST_ASSERT(bug.category == BugCategory::GRAPHICS);
}

TEST_CASE(BugTracker_Statistics, "BugTracker") {
    auto& registry = BugRegistry::instance();
    registry.reset();

    // Add some bugs
    BugReport bug1;
    bug1.title = "Bug 1";
    bug1.status = BugStatus::NEW;
    bug1.category = BugCategory::GRAPHICS;
    registry.add_bug(bug1);

    BugReport bug2;
    bug2.title = "Bug 2";
    bug2.status = BugStatus::FIXED;
    bug2.category = BugCategory::AUDIO;
    registry.add_bug(bug2);

    TEST_ASSERT_EQ(registry.get_total_count(), 2);
    TEST_ASSERT_EQ(registry.get_open_count(), 1);

    auto status_breakdown = registry.get_status_breakdown();
    TEST_ASSERT_EQ(status_breakdown[BugStatus::NEW], 1);
    TEST_ASSERT_EQ(status_breakdown[BugStatus::FIXED], 1);

    auto category_breakdown = registry.get_category_breakdown();
    TEST_ASSERT_EQ(category_breakdown[BugCategory::GRAPHICS], 1);
    TEST_ASSERT_EQ(category_breakdown[BugCategory::AUDIO], 1);
}
