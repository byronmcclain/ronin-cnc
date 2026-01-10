# Task 18a: Test Framework Foundation

| Field | Value |
|-------|-------|
| **Task ID** | 18a |
| **Task Name** | Test Framework Foundation |
| **Phase** | 18 - Testing & Polish |
| **Depends On** | All prior phases (platform, graphics, input, audio, game code) |
| **Estimated Complexity** | Medium (~400 lines) |

---

## Dependencies

- Platform layer fully functional
- CMake build system working
- C++ standard library available
- All major subsystems implemented (graphics, audio, input, game)

---

## Context

The Test Framework Foundation establishes the testing infrastructure needed for comprehensive quality assurance of the Red Alert port. This includes:

1. **Test Runner**: Unified test execution with reporting
2. **Test Macros**: Consistent assertion and logging macros
3. **Test Fixtures**: Setup/teardown helpers for common test scenarios
4. **Test Categories**: Organization of tests by type and scope
5. **Test Configuration**: Command-line options and settings
6. **CI Integration**: Exit codes and output formatting for automation

This foundation enables all subsequent testing tasks (unit, integration, visual, gameplay, performance) to share common infrastructure and produce consistent results.

---

## Technical Background

### Test Organization

Tests are organized into categories:
- **Unit Tests**: Isolated component testing
- **Integration Tests**: Multi-component interaction
- **Visual Tests**: Graphics and rendering verification
- **Gameplay Tests**: Game logic and mechanics
- **Performance Tests**: Benchmarks and stress tests

### Test Runner Architecture

```
TestRunner
├── TestRegistry (singleton)
│   ├── test categories (map<string, vector<TestCase>>)
│   └── test metadata (name, file, line, timeout)
├── TestReporter
│   ├── console output
│   ├── XML output (JUnit format)
│   └── summary statistics
└── TestFixtures
    ├── PlatformFixture
    ├── GraphicsFixture
    ├── AudioFixture
    └── GameFixture
```

### Test Macros

The framework provides these macros:
- `TEST_CASE(name, category)` - Define a test case
- `TEST_ASSERT(condition)` - Assert condition is true
- `TEST_ASSERT_EQ(a, b)` - Assert equality
- `TEST_ASSERT_NE(a, b)` - Assert inequality
- `TEST_ASSERT_LT(a, b)` - Assert less than
- `TEST_ASSERT_GT(a, b)` - Assert greater than
- `TEST_ASSERT_NEAR(a, b, eps)` - Assert floating point near
- `TEST_ASSERT_NULL(ptr)` - Assert null pointer
- `TEST_ASSERT_NOT_NULL(ptr)` - Assert non-null pointer
- `TEST_FAIL(message)` - Explicit failure
- `TEST_SKIP(reason)` - Skip test with reason

### Fixture Lifecycle

```cpp
class TestFixture {
    virtual void SetUp();    // Called before each test
    virtual void TearDown(); // Called after each test (even on failure)
};
```

---

## Objective

Create a comprehensive test framework that:
1. Provides a test registry for automatic test discovery
2. Implements assertion macros with detailed failure messages
3. Supports test fixtures for setup/teardown
4. Generates both console and XML output
5. Handles timeouts and crash recovery
6. Supports filtering tests by name or category
7. Tracks test statistics (pass/fail/skip counts)

---

## Deliverables

- [ ] `include/test/test_framework.h` - Test framework header
- [ ] `include/test/test_fixtures.h` - Common test fixtures
- [ ] `include/test/test_reporter.h` - Test output reporters
- [ ] `src/test/test_framework.cpp` - Framework implementation
- [ ] `src/test/test_fixtures.cpp` - Fixture implementations
- [ ] `src/test/test_reporter.cpp` - Reporter implementations
- [ ] `src/test_framework_self_test.cpp` - Self-test for framework
- [ ] CMakeLists.txt updated
- [ ] All tests pass

---

## Files to Create

### include/test/test_framework.h

```cpp
// include/test/test_framework.h
// Test Framework - Core Types and Macros
// Task 18a - Test Framework Foundation

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <chrono>
#include <memory>

//=============================================================================
// Test Result Types
//=============================================================================

enum class TestResult {
    PASSED,
    FAILED,
    SKIPPED,
    TIMEOUT,
    CRASHED
};

const char* TestResult_ToString(TestResult result);

//=============================================================================
// Test Case Information
//=============================================================================

struct TestCaseInfo {
    std::string name;
    std::string category;
    std::string file;
    int line;
    uint32_t timeout_ms;  // 0 = default timeout
};

struct TestCaseResult {
    TestCaseInfo info;
    TestResult result;
    std::string message;      // Failure/skip message
    double duration_ms;       // Execution time
    std::string failure_file; // File where assertion failed
    int failure_line;         // Line where assertion failed
};

//=============================================================================
// Test Case Function Type
//=============================================================================

using TestFunction = std::function<void()>;

//=============================================================================
// Test Registry - Singleton for Test Registration
//=============================================================================

class TestRegistry {
public:
    static TestRegistry& Instance();

    /// Register a test case
    void RegisterTest(const TestCaseInfo& info, TestFunction func);

    /// Get all registered tests
    const std::vector<std::pair<TestCaseInfo, TestFunction>>& GetAllTests() const;

    /// Get tests by category
    std::vector<std::pair<TestCaseInfo, TestFunction>> GetTestsByCategory(
        const std::string& category) const;

    /// Get all unique categories
    std::vector<std::string> GetCategories() const;

    /// Get test count
    size_t GetTestCount() const { return tests_.size(); }

    /// Clear all tests (for testing the framework itself)
    void Clear();

private:
    TestRegistry() = default;
    std::vector<std::pair<TestCaseInfo, TestFunction>> tests_;
};

//=============================================================================
// Test Exception - Thrown on Assertion Failure
//=============================================================================

class TestAssertionFailed : public std::exception {
public:
    TestAssertionFailed(const std::string& message,
                        const char* file, int line);

    const char* what() const noexcept override { return message_.c_str(); }
    const char* file() const { return file_.c_str(); }
    int line() const { return line_; }

private:
    std::string message_;
    std::string file_;
    int line_;
};

class TestSkipped : public std::exception {
public:
    explicit TestSkipped(const std::string& reason);
    const char* what() const noexcept override { return reason_.c_str(); }

private:
    std::string reason_;
};

//=============================================================================
// Test Runner Configuration
//=============================================================================

struct TestRunnerConfig {
    // Filtering
    std::string filter_name;      // Run only tests matching this pattern
    std::string filter_category;  // Run only this category
    bool list_only = false;       // Just list tests, don't run

    // Output
    bool verbose = false;         // Detailed output
    bool quiet = false;           // Minimal output
    std::string xml_output;       // Output JUnit XML to file

    // Execution
    uint32_t default_timeout_ms = 30000;  // 30 second default
    bool stop_on_failure = false;         // Stop after first failure
    bool shuffle = false;                 // Randomize test order
    uint32_t shuffle_seed = 0;            // Seed for shuffle (0 = random)

    // Parse from command line arguments
    bool ParseArgs(int argc, char* argv[]);
    void PrintUsage(const char* program_name);
};

//=============================================================================
// Test Runner
//=============================================================================

class TestRunner {
public:
    explicit TestRunner(const TestRunnerConfig& config = TestRunnerConfig());

    /// Run all tests matching the filter
    int Run();

    /// Run a specific category
    int RunCategory(const std::string& category);

    /// Get results after running
    const std::vector<TestCaseResult>& GetResults() const { return results_; }

    /// Get summary statistics
    int GetPassedCount() const { return passed_; }
    int GetFailedCount() const { return failed_; }
    int GetSkippedCount() const { return skipped_; }
    int GetTotalCount() const { return passed_ + failed_ + skipped_; }

private:
    TestRunnerConfig config_;
    std::vector<TestCaseResult> results_;
    int passed_ = 0;
    int failed_ = 0;
    int skipped_ = 0;

    TestCaseResult RunSingleTest(const TestCaseInfo& info, TestFunction func);
    void ReportProgress(const TestCaseResult& result);
    void ReportSummary();
    void WriteXmlReport();
};

//=============================================================================
// Test Registration Macro
//=============================================================================

#define TEST_CASE(name, category) \
    static void TestFunc_##name(); \
    static struct TestRegistrar_##name { \
        TestRegistrar_##name() { \
            TestCaseInfo info; \
            info.name = #name; \
            info.category = category; \
            info.file = __FILE__; \
            info.line = __LINE__; \
            info.timeout_ms = 0; \
            TestRegistry::Instance().RegisterTest(info, TestFunc_##name); \
        } \
    } g_test_registrar_##name; \
    static void TestFunc_##name()

#define TEST_CASE_TIMEOUT(name, category, timeout_ms) \
    static void TestFunc_##name(); \
    static struct TestRegistrar_##name { \
        TestRegistrar_##name() { \
            TestCaseInfo info; \
            info.name = #name; \
            info.category = category; \
            info.file = __FILE__; \
            info.line = __LINE__; \
            info.timeout_ms = timeout_ms; \
            TestRegistry::Instance().RegisterTest(info, TestFunc_##name); \
        } \
    } g_test_registrar_##name; \
    static void TestFunc_##name()

//=============================================================================
// Assertion Macros
//=============================================================================

#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            throw TestAssertionFailed( \
                "Assertion failed: " #condition, __FILE__, __LINE__); \
        } \
    } while (0)

#define TEST_ASSERT_MSG(condition, msg) \
    do { \
        if (!(condition)) { \
            throw TestAssertionFailed( \
                std::string("Assertion failed: ") + (msg), __FILE__, __LINE__); \
        } \
    } while (0)

#define TEST_ASSERT_EQ(a, b) \
    do { \
        auto _a = (a); \
        auto _b = (b); \
        if (!(_a == _b)) { \
            std::ostringstream _ss; \
            _ss << "Expected " #a " == " #b ", got " << _a << " != " << _b; \
            throw TestAssertionFailed(_ss.str(), __FILE__, __LINE__); \
        } \
    } while (0)

#define TEST_ASSERT_NE(a, b) \
    do { \
        auto _a = (a); \
        auto _b = (b); \
        if (!(_a != _b)) { \
            std::ostringstream _ss; \
            _ss << "Expected " #a " != " #b ", got " << _a << " == " << _b; \
            throw TestAssertionFailed(_ss.str(), __FILE__, __LINE__); \
        } \
    } while (0)

#define TEST_ASSERT_LT(a, b) \
    do { \
        auto _a = (a); \
        auto _b = (b); \
        if (!(_a < _b)) { \
            std::ostringstream _ss; \
            _ss << "Expected " #a " < " #b ", got " << _a << " >= " << _b; \
            throw TestAssertionFailed(_ss.str(), __FILE__, __LINE__); \
        } \
    } while (0)

#define TEST_ASSERT_GT(a, b) \
    do { \
        auto _a = (a); \
        auto _b = (b); \
        if (!(_a > _b)) { \
            std::ostringstream _ss; \
            _ss << "Expected " #a " > " #b ", got " << _a << " <= " << _b; \
            throw TestAssertionFailed(_ss.str(), __FILE__, __LINE__); \
        } \
    } while (0)

#define TEST_ASSERT_LE(a, b) \
    do { \
        auto _a = (a); \
        auto _b = (b); \
        if (!(_a <= _b)) { \
            std::ostringstream _ss; \
            _ss << "Expected " #a " <= " #b ", got " << _a << " > " << _b; \
            throw TestAssertionFailed(_ss.str(), __FILE__, __LINE__); \
        } \
    } while (0)

#define TEST_ASSERT_GE(a, b) \
    do { \
        auto _a = (a); \
        auto _b = (b); \
        if (!(_a >= _b)) { \
            std::ostringstream _ss; \
            _ss << "Expected " #a " >= " #b ", got " << _a << " < " << _b; \
            throw TestAssertionFailed(_ss.str(), __FILE__, __LINE__); \
        } \
    } while (0)

#define TEST_ASSERT_NEAR(a, b, epsilon) \
    do { \
        auto _a = (a); \
        auto _b = (b); \
        auto _eps = (epsilon); \
        auto _diff = (_a > _b) ? (_a - _b) : (_b - _a); \
        if (_diff > _eps) { \
            std::ostringstream _ss; \
            _ss << "Expected " #a " near " #b " (epsilon=" << _eps << "), " \
                << "got " << _a << " vs " << _b << " (diff=" << _diff << ")"; \
            throw TestAssertionFailed(_ss.str(), __FILE__, __LINE__); \
        } \
    } while (0)

#define TEST_ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != nullptr) { \
            throw TestAssertionFailed( \
                "Expected " #ptr " to be null", __FILE__, __LINE__); \
        } \
    } while (0)

#define TEST_ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == nullptr) { \
            throw TestAssertionFailed( \
                "Expected " #ptr " to be non-null", __FILE__, __LINE__); \
        } \
    } while (0)

#define TEST_ASSERT_THROWS(expr, exception_type) \
    do { \
        bool _caught = false; \
        try { expr; } \
        catch (const exception_type&) { _caught = true; } \
        catch (...) {} \
        if (!_caught) { \
            throw TestAssertionFailed( \
                "Expected " #expr " to throw " #exception_type, \
                __FILE__, __LINE__); \
        } \
    } while (0)

#define TEST_FAIL(msg) \
    throw TestAssertionFailed(msg, __FILE__, __LINE__)

#define TEST_SKIP(reason) \
    throw TestSkipped(reason)

//=============================================================================
// Convenience Main Function
//=============================================================================

/// Create a main() that runs all tests
#define TEST_MAIN() \
    int main(int argc, char* argv[]) { \
        TestRunnerConfig config; \
        if (!config.ParseArgs(argc, argv)) { \
            return 1; \
        } \
        TestRunner runner(config); \
        return runner.Run(); \
    }

#endif // TEST_FRAMEWORK_H
```

### include/test/test_fixtures.h

```cpp
// include/test/test_fixtures.h
// Common Test Fixtures
// Task 18a - Test Framework Foundation

#ifndef TEST_FIXTURES_H
#define TEST_FIXTURES_H

#include "test/test_framework.h"
#include <memory>

//=============================================================================
// Base Test Fixture
//=============================================================================

class TestFixture {
public:
    virtual ~TestFixture() = default;

    /// Called before each test
    virtual void SetUp() {}

    /// Called after each test (even on failure)
    virtual void TearDown() {}

protected:
    TestFixture() = default;
};

//=============================================================================
// Platform Fixture - Initializes Platform Layer
//=============================================================================

class PlatformFixture : public TestFixture {
public:
    void SetUp() override;
    void TearDown() override;

    /// Check if platform initialized successfully
    bool IsInitialized() const { return initialized_; }

protected:
    bool initialized_ = false;
};

//=============================================================================
// Graphics Fixture - Platform + Graphics Subsystem
//=============================================================================

class GraphicsFixture : public PlatformFixture {
public:
    void SetUp() override;
    void TearDown() override;

    /// Get backbuffer for pixel testing
    uint8_t* GetBackBuffer();
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    int GetPitch() const { return pitch_; }

    /// Helper to clear backbuffer
    void ClearBackBuffer(uint8_t color = 0);

    /// Helper to render a frame
    void RenderFrame();

protected:
    int width_ = 0;
    int height_ = 0;
    int pitch_ = 0;
    bool graphics_initialized_ = false;
};

//=============================================================================
// Audio Fixture - Platform + Audio Subsystem
//=============================================================================

class AudioFixture : public PlatformFixture {
public:
    void SetUp() override;
    void TearDown() override;

    /// Check if audio initialized successfully
    bool IsAudioInitialized() const { return audio_initialized_; }

    /// Wait for a duration (for audio playback tests)
    void WaitMs(uint32_t ms);

protected:
    bool audio_initialized_ = false;
};

//=============================================================================
// Asset Fixture - Platform + MIX Loading
//=============================================================================

class AssetFixture : public PlatformFixture {
public:
    void SetUp() override;
    void TearDown() override;

    /// Check if assets loaded successfully
    bool AreAssetsLoaded() const { return assets_loaded_; }

    /// Check if a specific MIX file is loaded
    bool IsMixLoaded(const char* mix_name) const;

protected:
    bool assets_loaded_ = false;
};

//=============================================================================
// Full Game Fixture - All Systems
//=============================================================================

class GameFixture : public TestFixture {
public:
    void SetUp() override;
    void TearDown() override;

    /// Check if game initialized
    bool IsInitialized() const { return game_initialized_; }

    /// Run N frames of game loop
    void RunFrames(int count);

    /// Process input events
    void PumpEvents();

protected:
    bool game_initialized_ = false;
};

//=============================================================================
// Fixture Usage Macro
//=============================================================================

/// Define a test that uses a fixture
#define TEST_WITH_FIXTURE(fixture_class, name, category) \
    static void TestFunc_##name(fixture_class& fixture); \
    static void TestWrapper_##name() { \
        fixture_class fixture; \
        fixture.SetUp(); \
        try { \
            TestFunc_##name(fixture); \
        } catch (...) { \
            fixture.TearDown(); \
            throw; \
        } \
        fixture.TearDown(); \
    } \
    static struct TestRegistrar_##name { \
        TestRegistrar_##name() { \
            TestCaseInfo info; \
            info.name = #name; \
            info.category = category; \
            info.file = __FILE__; \
            info.line = __LINE__; \
            info.timeout_ms = 0; \
            TestRegistry::Instance().RegisterTest(info, TestWrapper_##name); \
        } \
    } g_test_registrar_##name; \
    static void TestFunc_##name(fixture_class& fixture)

#endif // TEST_FIXTURES_H
```

### include/test/test_reporter.h

```cpp
// include/test/test_reporter.h
// Test Output Reporters
// Task 18a - Test Framework Foundation

#ifndef TEST_REPORTER_H
#define TEST_REPORTER_H

#include "test/test_framework.h"
#include <string>
#include <vector>
#include <fstream>

//=============================================================================
// Base Test Reporter
//=============================================================================

class TestReporter {
public:
    virtual ~TestReporter() = default;

    /// Called before any tests run
    virtual void OnTestRunStart(int total_tests) {}

    /// Called when a test starts
    virtual void OnTestStart(const TestCaseInfo& info) {}

    /// Called when a test completes
    virtual void OnTestComplete(const TestCaseResult& result) {}

    /// Called after all tests complete
    virtual void OnTestRunComplete(const std::vector<TestCaseResult>& results,
                                   int passed, int failed, int skipped) {}
};

//=============================================================================
// Console Reporter - Output to stdout/stderr
//=============================================================================

class ConsoleReporter : public TestReporter {
public:
    explicit ConsoleReporter(bool verbose = false, bool use_color = true);

    void OnTestRunStart(int total_tests) override;
    void OnTestStart(const TestCaseInfo& info) override;
    void OnTestComplete(const TestCaseResult& result) override;
    void OnTestRunComplete(const std::vector<TestCaseResult>& results,
                          int passed, int failed, int skipped) override;

private:
    bool verbose_;
    bool use_color_;
    int current_test_ = 0;
    int total_tests_ = 0;

    // ANSI color codes
    const char* ColorReset() const;
    const char* ColorGreen() const;
    const char* ColorRed() const;
    const char* ColorYellow() const;
    const char* ColorCyan() const;
};

//=============================================================================
// XML Reporter - JUnit-compatible XML Output
//=============================================================================

class XmlReporter : public TestReporter {
public:
    explicit XmlReporter(const std::string& output_path);

    void OnTestRunStart(int total_tests) override;
    void OnTestComplete(const TestCaseResult& result) override;
    void OnTestRunComplete(const std::vector<TestCaseResult>& results,
                          int passed, int failed, int skipped) override;

private:
    std::string output_path_;
    std::ofstream file_;
    std::chrono::steady_clock::time_point start_time_;

    std::string EscapeXml(const std::string& str) const;
};

//=============================================================================
// Multi Reporter - Combine Multiple Reporters
//=============================================================================

class MultiReporter : public TestReporter {
public:
    void AddReporter(std::shared_ptr<TestReporter> reporter);

    void OnTestRunStart(int total_tests) override;
    void OnTestStart(const TestCaseInfo& info) override;
    void OnTestComplete(const TestCaseResult& result) override;
    void OnTestRunComplete(const std::vector<TestCaseResult>& results,
                          int passed, int failed, int skipped) override;

private:
    std::vector<std::shared_ptr<TestReporter>> reporters_;
};

#endif // TEST_REPORTER_H
```

### src/test/test_framework.cpp

```cpp
// src/test/test_framework.cpp
// Test Framework Implementation
// Task 18a - Test Framework Foundation

#include "test/test_framework.h"
#include "test/test_reporter.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <random>
#include <chrono>
#include <cstring>

//=============================================================================
// TestResult Helpers
//=============================================================================

const char* TestResult_ToString(TestResult result) {
    switch (result) {
        case TestResult::PASSED:  return "PASSED";
        case TestResult::FAILED:  return "FAILED";
        case TestResult::SKIPPED: return "SKIPPED";
        case TestResult::TIMEOUT: return "TIMEOUT";
        case TestResult::CRASHED: return "CRASHED";
        default: return "UNKNOWN";
    }
}

//=============================================================================
// TestRegistry
//=============================================================================

TestRegistry& TestRegistry::Instance() {
    static TestRegistry instance;
    return instance;
}

void TestRegistry::RegisterTest(const TestCaseInfo& info, TestFunction func) {
    tests_.emplace_back(info, func);
}

const std::vector<std::pair<TestCaseInfo, TestFunction>>&
TestRegistry::GetAllTests() const {
    return tests_;
}

std::vector<std::pair<TestCaseInfo, TestFunction>>
TestRegistry::GetTestsByCategory(const std::string& category) const {
    std::vector<std::pair<TestCaseInfo, TestFunction>> result;
    for (const auto& test : tests_) {
        if (test.first.category == category) {
            result.push_back(test);
        }
    }
    return result;
}

std::vector<std::string> TestRegistry::GetCategories() const {
    std::vector<std::string> categories;
    for (const auto& test : tests_) {
        if (std::find(categories.begin(), categories.end(),
                      test.first.category) == categories.end()) {
            categories.push_back(test.first.category);
        }
    }
    std::sort(categories.begin(), categories.end());
    return categories;
}

void TestRegistry::Clear() {
    tests_.clear();
}

//=============================================================================
// TestAssertionFailed
//=============================================================================

TestAssertionFailed::TestAssertionFailed(const std::string& message,
                                         const char* file, int line)
    : message_(message)
    , file_(file)
    , line_(line) {
}

//=============================================================================
// TestSkipped
//=============================================================================

TestSkipped::TestSkipped(const std::string& reason)
    : reason_(reason) {
}

//=============================================================================
// TestRunnerConfig
//=============================================================================

bool TestRunnerConfig::ParseArgs(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            PrintUsage(argv[0]);
            return false;
        }
        else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        }
        else if (arg == "-q" || arg == "--quiet") {
            quiet = true;
        }
        else if (arg == "-l" || arg == "--list") {
            list_only = true;
        }
        else if (arg == "--stop-on-failure") {
            stop_on_failure = true;
        }
        else if (arg == "--shuffle") {
            shuffle = true;
        }
        else if ((arg == "-f" || arg == "--filter") && i + 1 < argc) {
            filter_name = argv[++i];
        }
        else if ((arg == "-c" || arg == "--category") && i + 1 < argc) {
            filter_category = argv[++i];
        }
        else if ((arg == "-x" || arg == "--xml") && i + 1 < argc) {
            xml_output = argv[++i];
        }
        else if (arg == "--timeout" && i + 1 < argc) {
            default_timeout_ms = static_cast<uint32_t>(std::stoi(argv[++i]));
        }
        else if (arg == "--seed" && i + 1 < argc) {
            shuffle_seed = static_cast<uint32_t>(std::stoi(argv[++i]));
        }
        else {
            std::cerr << "Unknown option: " << arg << std::endl;
            PrintUsage(argv[0]);
            return false;
        }
    }
    return true;
}

void TestRunnerConfig::PrintUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]\n"
              << "\nOptions:\n"
              << "  -h, --help            Show this help\n"
              << "  -v, --verbose         Verbose output\n"
              << "  -q, --quiet           Minimal output\n"
              << "  -l, --list            List tests without running\n"
              << "  -f, --filter PATTERN  Run tests matching pattern\n"
              << "  -c, --category CAT    Run only tests in category\n"
              << "  -x, --xml FILE        Output JUnit XML to file\n"
              << "  --stop-on-failure     Stop after first failure\n"
              << "  --shuffle             Randomize test order\n"
              << "  --seed N              Seed for shuffle (0 = random)\n"
              << "  --timeout MS          Default timeout in milliseconds\n";
}

//=============================================================================
// TestRunner
//=============================================================================

TestRunner::TestRunner(const TestRunnerConfig& config)
    : config_(config) {
}

int TestRunner::Run() {
    auto& registry = TestRegistry::Instance();
    auto tests = registry.GetAllTests();

    // Filter by category
    if (!config_.filter_category.empty()) {
        tests = registry.GetTestsByCategory(config_.filter_category);
    }

    // Filter by name
    if (!config_.filter_name.empty()) {
        std::vector<std::pair<TestCaseInfo, TestFunction>> filtered;
        for (const auto& test : tests) {
            if (test.first.name.find(config_.filter_name) != std::string::npos) {
                filtered.push_back(test);
            }
        }
        tests = filtered;
    }

    // List only mode
    if (config_.list_only) {
        std::cout << "Available tests (" << tests.size() << "):\n";
        for (const auto& test : tests) {
            std::cout << "  [" << test.first.category << "] "
                      << test.first.name << "\n";
        }
        return 0;
    }

    // Shuffle if requested
    if (config_.shuffle) {
        uint32_t seed = config_.shuffle_seed;
        if (seed == 0) {
            seed = static_cast<uint32_t>(
                std::chrono::steady_clock::now().time_since_epoch().count());
        }
        std::shuffle(tests.begin(), tests.end(), std::default_random_engine(seed));
        if (!config_.quiet) {
            std::cout << "Shuffled with seed: " << seed << "\n";
        }
    }

    // Set up reporters
    auto multi_reporter = std::make_shared<MultiReporter>();
    if (!config_.quiet) {
        multi_reporter->AddReporter(
            std::make_shared<ConsoleReporter>(config_.verbose));
    }
    if (!config_.xml_output.empty()) {
        multi_reporter->AddReporter(
            std::make_shared<XmlReporter>(config_.xml_output));
    }

    // Run tests
    multi_reporter->OnTestRunStart(static_cast<int>(tests.size()));

    for (const auto& test : tests) {
        multi_reporter->OnTestStart(test.first);

        TestCaseResult result = RunSingleTest(test.first, test.second);
        results_.push_back(result);

        switch (result.result) {
            case TestResult::PASSED:  passed_++; break;
            case TestResult::SKIPPED: skipped_++; break;
            default: failed_++; break;
        }

        multi_reporter->OnTestComplete(result);

        if (config_.stop_on_failure && result.result == TestResult::FAILED) {
            break;
        }
    }

    multi_reporter->OnTestRunComplete(results_, passed_, failed_, skipped_);

    return (failed_ > 0) ? 1 : 0;
}

int TestRunner::RunCategory(const std::string& category) {
    config_.filter_category = category;
    return Run();
}

TestCaseResult TestRunner::RunSingleTest(const TestCaseInfo& info,
                                         TestFunction func) {
    TestCaseResult result;
    result.info = info;
    result.result = TestResult::PASSED;

    auto start = std::chrono::steady_clock::now();

    try {
        func();
    }
    catch (const TestAssertionFailed& e) {
        result.result = TestResult::FAILED;
        result.message = e.what();
        result.failure_file = e.file();
        result.failure_line = e.line();
    }
    catch (const TestSkipped& e) {
        result.result = TestResult::SKIPPED;
        result.message = e.what();
    }
    catch (const std::exception& e) {
        result.result = TestResult::CRASHED;
        result.message = std::string("Exception: ") + e.what();
    }
    catch (...) {
        result.result = TestResult::CRASHED;
        result.message = "Unknown exception";
    }

    auto end = std::chrono::steady_clock::now();
    result.duration_ms = std::chrono::duration<double, std::milli>(
        end - start).count();

    return result;
}
```

### src/test/test_fixtures.cpp

```cpp
// src/test/test_fixtures.cpp
// Common Test Fixtures Implementation
// Task 18a - Test Framework Foundation

#include "test/test_fixtures.h"
#include "platform.h"

//=============================================================================
// PlatformFixture
//=============================================================================

void PlatformFixture::SetUp() {
    initialized_ = (Platform_Init() == 0);
}

void PlatformFixture::TearDown() {
    if (initialized_) {
        Platform_Shutdown();
        initialized_ = false;
    }
}

//=============================================================================
// GraphicsFixture
//=============================================================================

void GraphicsFixture::SetUp() {
    PlatformFixture::SetUp();
    if (!initialized_) return;

    GraphicsConfig config;
    config.width = 640;
    config.height = 480;
    config.fullscreen = false;
    config.vsync = false;

    graphics_initialized_ = (Platform_Graphics_Init(&config) == 0);

    if (graphics_initialized_) {
        uint8_t* buffer;
        int32_t w, h, p;
        if (Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p) == 0) {
            width_ = w;
            height_ = h;
            pitch_ = p;
        }
    }
}

void GraphicsFixture::TearDown() {
    if (graphics_initialized_) {
        Platform_Graphics_Shutdown();
        graphics_initialized_ = false;
    }
    PlatformFixture::TearDown();
}

uint8_t* GraphicsFixture::GetBackBuffer() {
    uint8_t* buffer = nullptr;
    int32_t w, h, p;
    Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);
    return buffer;
}

void GraphicsFixture::ClearBackBuffer(uint8_t color) {
    uint8_t* buffer = GetBackBuffer();
    if (buffer) {
        for (int y = 0; y < height_; y++) {
            memset(buffer + y * pitch_, color, width_);
        }
    }
}

void GraphicsFixture::RenderFrame() {
    Platform_Graphics_Flip();
}

//=============================================================================
// AudioFixture
//=============================================================================

void AudioFixture::SetUp() {
    PlatformFixture::SetUp();
    if (!initialized_) return;

    AudioConfig config;
    config.sample_rate = 22050;
    config.channels = 2;
    config.bits_per_sample = 16;
    config.buffer_size = 1024;

    audio_initialized_ = (Platform_Audio_Init(&config) == 0);
}

void AudioFixture::TearDown() {
    if (audio_initialized_) {
        Platform_Audio_Shutdown();
        audio_initialized_ = false;
    }
    PlatformFixture::TearDown();
}

void AudioFixture::WaitMs(uint32_t ms) {
    Platform_Timer_Delay(ms);
}

//=============================================================================
// AssetFixture
//=============================================================================

void AssetFixture::SetUp() {
    PlatformFixture::SetUp();
    if (!initialized_) return;

    // Initialize MixManager and load essential MIX files
    // In real implementation:
    // MixManager::Instance().Initialize();
    // assets_loaded_ = MixManager::Instance().LoadMix("REDALERT.MIX");

    assets_loaded_ = true;  // Placeholder
}

void AssetFixture::TearDown() {
    if (assets_loaded_) {
        // MixManager::Instance().Shutdown();
        assets_loaded_ = false;
    }
    PlatformFixture::TearDown();
}

bool AssetFixture::IsMixLoaded(const char* mix_name) const {
    // return MixManager::Instance().IsMixLoaded(mix_name);
    return assets_loaded_;  // Placeholder
}

//=============================================================================
// GameFixture
//=============================================================================

void GameFixture::SetUp() {
    // In real implementation:
    // game_initialized_ = Game_Init();

    game_initialized_ = true;  // Placeholder
}

void GameFixture::TearDown() {
    if (game_initialized_) {
        // Game_Shutdown();
        game_initialized_ = false;
    }
}

void GameFixture::RunFrames(int count) {
    for (int i = 0; i < count; i++) {
        PumpEvents();
        // Game_Update();
        // Game_Render();
        Platform_Timer_Delay(16);  // ~60fps
    }
}

void GameFixture::PumpEvents() {
    Platform_PumpEvents();
    Platform_Input_Update();
}
```

### src/test/test_reporter.cpp

```cpp
// src/test/test_reporter.cpp
// Test Output Reporters Implementation
// Task 18a - Test Framework Foundation

#include "test/test_reporter.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>

//=============================================================================
// ConsoleReporter
//=============================================================================

ConsoleReporter::ConsoleReporter(bool verbose, bool use_color)
    : verbose_(verbose)
    , use_color_(use_color) {
}

const char* ConsoleReporter::ColorReset() const {
    return use_color_ ? "\033[0m" : "";
}

const char* ConsoleReporter::ColorGreen() const {
    return use_color_ ? "\033[32m" : "";
}

const char* ConsoleReporter::ColorRed() const {
    return use_color_ ? "\033[31m" : "";
}

const char* ConsoleReporter::ColorYellow() const {
    return use_color_ ? "\033[33m" : "";
}

const char* ConsoleReporter::ColorCyan() const {
    return use_color_ ? "\033[36m" : "";
}

void ConsoleReporter::OnTestRunStart(int total_tests) {
    total_tests_ = total_tests;
    current_test_ = 0;

    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "  Running " << total_tests << " tests\n";
    std::cout << "========================================\n\n";
}

void ConsoleReporter::OnTestStart(const TestCaseInfo& info) {
    current_test_++;

    if (verbose_) {
        std::cout << "[" << current_test_ << "/" << total_tests_ << "] "
                  << ColorCyan() << info.category << ColorReset()
                  << " :: " << info.name << " ... " << std::flush;
    }
}

void ConsoleReporter::OnTestComplete(const TestCaseResult& result) {
    if (verbose_) {
        switch (result.result) {
            case TestResult::PASSED:
                std::cout << ColorGreen() << "PASSED" << ColorReset()
                          << " (" << std::fixed << std::setprecision(1)
                          << result.duration_ms << "ms)\n";
                break;
            case TestResult::FAILED:
                std::cout << ColorRed() << "FAILED" << ColorReset() << "\n";
                std::cout << "    " << result.message << "\n";
                if (!result.failure_file.empty()) {
                    std::cout << "    at " << result.failure_file
                              << ":" << result.failure_line << "\n";
                }
                break;
            case TestResult::SKIPPED:
                std::cout << ColorYellow() << "SKIPPED" << ColorReset()
                          << " (" << result.message << ")\n";
                break;
            case TestResult::TIMEOUT:
                std::cout << ColorRed() << "TIMEOUT" << ColorReset() << "\n";
                break;
            case TestResult::CRASHED:
                std::cout << ColorRed() << "CRASHED" << ColorReset()
                          << " (" << result.message << ")\n";
                break;
        }
    } else {
        // Compact output
        switch (result.result) {
            case TestResult::PASSED:  std::cout << ColorGreen() << "." << ColorReset(); break;
            case TestResult::FAILED:  std::cout << ColorRed() << "F" << ColorReset(); break;
            case TestResult::SKIPPED: std::cout << ColorYellow() << "S" << ColorReset(); break;
            case TestResult::TIMEOUT: std::cout << ColorRed() << "T" << ColorReset(); break;
            case TestResult::CRASHED: std::cout << ColorRed() << "X" << ColorReset(); break;
        }
        std::cout << std::flush;
    }
}

void ConsoleReporter::OnTestRunComplete(const std::vector<TestCaseResult>& results,
                                        int passed, int failed, int skipped) {
    if (!verbose_) {
        std::cout << "\n";
    }

    std::cout << "\n";
    std::cout << "========================================\n";

    // Print failures in detail
    bool has_failures = false;
    for (const auto& result : results) {
        if (result.result == TestResult::FAILED ||
            result.result == TestResult::CRASHED ||
            result.result == TestResult::TIMEOUT) {

            if (!has_failures) {
                std::cout << "\nFailed Tests:\n";
                has_failures = true;
            }

            std::cout << ColorRed() << "  FAIL: " << ColorReset()
                      << result.info.category << "::" << result.info.name << "\n";
            std::cout << "        " << result.message << "\n";
            if (!result.failure_file.empty()) {
                std::cout << "        at " << result.failure_file
                          << ":" << result.failure_line << "\n";
            }
        }
    }

    // Summary
    std::cout << "\nSummary:\n";
    std::cout << "  " << ColorGreen() << passed << " passed" << ColorReset() << "\n";

    if (failed > 0) {
        std::cout << "  " << ColorRed() << failed << " failed" << ColorReset() << "\n";
    }

    if (skipped > 0) {
        std::cout << "  " << ColorYellow() << skipped << " skipped" << ColorReset() << "\n";
    }

    int total = passed + failed + skipped;
    std::cout << "  " << total << " total\n";

    // Calculate total time
    double total_time = 0;
    for (const auto& result : results) {
        total_time += result.duration_ms;
    }
    std::cout << "  Time: " << std::fixed << std::setprecision(2)
              << total_time << "ms\n";

    std::cout << "========================================\n";

    if (failed == 0) {
        std::cout << ColorGreen() << "All tests passed!" << ColorReset() << "\n";
    } else {
        std::cout << ColorRed() << "Some tests failed." << ColorReset() << "\n";
    }
}

//=============================================================================
// XmlReporter
//=============================================================================

XmlReporter::XmlReporter(const std::string& output_path)
    : output_path_(output_path) {
}

std::string XmlReporter::EscapeXml(const std::string& str) const {
    std::string result;
    result.reserve(str.size() * 1.1);

    for (char c : str) {
        switch (c) {
            case '&':  result += "&amp;"; break;
            case '<':  result += "&lt;"; break;
            case '>':  result += "&gt;"; break;
            case '"':  result += "&quot;"; break;
            case '\'': result += "&apos;"; break;
            default:   result += c; break;
        }
    }
    return result;
}

void XmlReporter::OnTestRunStart(int total_tests) {
    start_time_ = std::chrono::steady_clock::now();
    file_.open(output_path_);

    if (file_.is_open()) {
        file_ << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    }
}

void XmlReporter::OnTestComplete(const TestCaseResult& result) {
    // Individual test results will be written in OnTestRunComplete
}

void XmlReporter::OnTestRunComplete(const std::vector<TestCaseResult>& results,
                                    int passed, int failed, int skipped) {
    if (!file_.is_open()) return;

    auto end_time = std::chrono::steady_clock::now();
    double total_time_sec = std::chrono::duration<double>(
        end_time - start_time_).count();

    // Group tests by category
    std::map<std::string, std::vector<const TestCaseResult*>> by_category;
    for (const auto& result : results) {
        by_category[result.info.category].push_back(&result);
    }

    // Write testsuites root
    file_ << "<testsuites tests=\"" << results.size()
          << "\" failures=\"" << failed
          << "\" time=\"" << std::fixed << std::setprecision(3) << total_time_sec
          << "\">\n";

    // Write each category as a testsuite
    for (const auto& pair : by_category) {
        const std::string& category = pair.first;
        const auto& tests = pair.second;

        int suite_failures = 0;
        double suite_time = 0;
        for (const auto* test : tests) {
            if (test->result != TestResult::PASSED &&
                test->result != TestResult::SKIPPED) {
                suite_failures++;
            }
            suite_time += test->duration_ms / 1000.0;
        }

        file_ << "  <testsuite name=\"" << EscapeXml(category)
              << "\" tests=\"" << tests.size()
              << "\" failures=\"" << suite_failures
              << "\" time=\"" << std::fixed << std::setprecision(3) << suite_time
              << "\">\n";

        for (const auto* test : tests) {
            file_ << "    <testcase name=\"" << EscapeXml(test->info.name)
                  << "\" classname=\"" << EscapeXml(category)
                  << "\" time=\"" << std::fixed << std::setprecision(3)
                  << (test->duration_ms / 1000.0) << "\"";

            if (test->result == TestResult::PASSED) {
                file_ << "/>\n";
            } else if (test->result == TestResult::SKIPPED) {
                file_ << ">\n";
                file_ << "      <skipped message=\""
                      << EscapeXml(test->message) << "\"/>\n";
                file_ << "    </testcase>\n";
            } else {
                file_ << ">\n";
                file_ << "      <failure message=\""
                      << EscapeXml(test->message) << "\"";
                if (!test->failure_file.empty()) {
                    file_ << " type=\"" << EscapeXml(test->failure_file)
                          << ":" << test->failure_line << "\"";
                }
                file_ << "/>\n";
                file_ << "    </testcase>\n";
            }
        }

        file_ << "  </testsuite>\n";
    }

    file_ << "</testsuites>\n";
    file_.close();
}

//=============================================================================
// MultiReporter
//=============================================================================

void MultiReporter::AddReporter(std::shared_ptr<TestReporter> reporter) {
    reporters_.push_back(reporter);
}

void MultiReporter::OnTestRunStart(int total_tests) {
    for (auto& reporter : reporters_) {
        reporter->OnTestRunStart(total_tests);
    }
}

void MultiReporter::OnTestStart(const TestCaseInfo& info) {
    for (auto& reporter : reporters_) {
        reporter->OnTestStart(info);
    }
}

void MultiReporter::OnTestComplete(const TestCaseResult& result) {
    for (auto& reporter : reporters_) {
        reporter->OnTestComplete(result);
    }
}

void MultiReporter::OnTestRunComplete(const std::vector<TestCaseResult>& results,
                                      int passed, int failed, int skipped) {
    for (auto& reporter : reporters_) {
        reporter->OnTestRunComplete(results, passed, failed, skipped);
    }
}
```

### src/test_framework_self_test.cpp

```cpp
// src/test_framework_self_test.cpp
// Self-test for the test framework
// Task 18a - Test Framework Foundation

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include <sstream>

//=============================================================================
// Basic Test Registration Tests
//=============================================================================

TEST_CASE(TestRegistration_Basic, "Framework") {
    // This test simply runs - if we got here, registration worked
    TEST_ASSERT(true);
}

TEST_CASE(TestRegistration_Multiple, "Framework") {
    // Multiple tests in same file should all register
    TEST_ASSERT(TestRegistry::Instance().GetTestCount() >= 2);
}

//=============================================================================
// Assertion Tests
//=============================================================================

TEST_CASE(Assert_True, "Assertions") {
    TEST_ASSERT(1 == 1);
    TEST_ASSERT(true);
    TEST_ASSERT(42 > 0);
}

TEST_CASE(Assert_Equal, "Assertions") {
    TEST_ASSERT_EQ(5, 5);
    TEST_ASSERT_EQ(std::string("hello"), std::string("hello"));
    TEST_ASSERT_EQ(3.14, 3.14);
}

TEST_CASE(Assert_NotEqual, "Assertions") {
    TEST_ASSERT_NE(5, 6);
    TEST_ASSERT_NE(std::string("hello"), std::string("world"));
}

TEST_CASE(Assert_LessThan, "Assertions") {
    TEST_ASSERT_LT(1, 2);
    TEST_ASSERT_LT(-5, 0);
    TEST_ASSERT_LT(0.1, 0.2);
}

TEST_CASE(Assert_GreaterThan, "Assertions") {
    TEST_ASSERT_GT(2, 1);
    TEST_ASSERT_GT(0, -5);
    TEST_ASSERT_GT(0.2, 0.1);
}

TEST_CASE(Assert_LessOrEqual, "Assertions") {
    TEST_ASSERT_LE(1, 2);
    TEST_ASSERT_LE(2, 2);
}

TEST_CASE(Assert_GreaterOrEqual, "Assertions") {
    TEST_ASSERT_GE(2, 1);
    TEST_ASSERT_GE(2, 2);
}

TEST_CASE(Assert_Near, "Assertions") {
    TEST_ASSERT_NEAR(3.14159, 3.14, 0.01);
    TEST_ASSERT_NEAR(1.0, 1.0001, 0.001);
}

TEST_CASE(Assert_Null, "Assertions") {
    int* null_ptr = nullptr;
    TEST_ASSERT_NULL(null_ptr);
}

TEST_CASE(Assert_NotNull, "Assertions") {
    int value = 42;
    int* ptr = &value;
    TEST_ASSERT_NOT_NULL(ptr);
}

TEST_CASE(Assert_Throws, "Assertions") {
    TEST_ASSERT_THROWS(throw std::runtime_error("test"), std::runtime_error);
}

//=============================================================================
// Skip Test
//=============================================================================

TEST_CASE(Skip_Example, "Skipping") {
    TEST_SKIP("This test is intentionally skipped");
    // Code below should not execute
    TEST_FAIL("Should not reach here");
}

//=============================================================================
// Category Tests
//=============================================================================

TEST_CASE(Category_Test1, "CategoryA") {
    TEST_ASSERT(true);
}

TEST_CASE(Category_Test2, "CategoryA") {
    TEST_ASSERT(true);
}

TEST_CASE(Category_Test3, "CategoryB") {
    TEST_ASSERT(true);
}

TEST_CASE(Categories_Listed, "Framework") {
    auto categories = TestRegistry::Instance().GetCategories();
    TEST_ASSERT_GE(categories.size(), 3u);  // At least Framework, Assertions, CategoryA
}

//=============================================================================
// Test with Message
//=============================================================================

TEST_CASE(Assert_WithMessage, "Assertions") {
    int value = 42;
    TEST_ASSERT_MSG(value > 0, "value should be positive");
}

//=============================================================================
// Reporter Tests
//=============================================================================

TEST_CASE(ConsoleReporter_Creation, "Reporters") {
    ConsoleReporter reporter(true, false);
    // Should not crash
    TEST_ASSERT(true);
}

TEST_CASE(XmlReporter_EscapeXml, "Reporters") {
    // XmlReporter should escape special characters
    // Testing indirectly through file output would be complex
    // Just verify it can be created
    XmlReporter reporter("/tmp/test_output.xml");
    TEST_ASSERT(true);
}

//=============================================================================
// Fixture Tests
//=============================================================================

class SimpleFixture : public TestFixture {
public:
    void SetUp() override {
        setup_called = true;
        value = 42;
    }

    void TearDown() override {
        teardown_called = true;
    }

    bool setup_called = false;
    bool teardown_called = false;
    int value = 0;
};

TEST_WITH_FIXTURE(SimpleFixture, Fixture_SetupCalled, "Fixtures") {
    TEST_ASSERT(fixture.setup_called);
    TEST_ASSERT_EQ(fixture.value, 42);
}

//=============================================================================
// Performance/Timing Tests
//=============================================================================

TEST_CASE(Timing_BasicDelay, "Timing") {
    auto start = std::chrono::steady_clock::now();

    // Small delay
    for (volatile int i = 0; i < 1000000; i++) {}

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end - start).count();

    // Should take at least some time
    TEST_ASSERT_GT(duration, 0);
}

//=============================================================================
// Edge Cases
//=============================================================================

TEST_CASE(EdgeCase_EmptyString, "EdgeCases") {
    std::string empty;
    TEST_ASSERT(empty.empty());
    TEST_ASSERT_EQ(empty.size(), 0u);
}

TEST_CASE(EdgeCase_LargeNumbers, "EdgeCases") {
    int64_t large = 9223372036854775807LL;
    TEST_ASSERT_GT(large, 0);
    TEST_ASSERT_EQ(large, 9223372036854775807LL);
}

TEST_CASE(EdgeCase_FloatingPoint, "EdgeCases") {
    double a = 0.1 + 0.2;
    double b = 0.3;
    // These are not exactly equal due to floating point
    TEST_ASSERT_NEAR(a, b, 1e-10);
}

//=============================================================================
// Main
//=============================================================================

TEST_MAIN()
```

---

## CMakeLists.txt Additions

```cmake
# Task 18a: Test Framework Foundation

# Test framework library
set(TEST_FRAMEWORK_SOURCES
    ${CMAKE_SOURCE_DIR}/src/test/test_framework.cpp
    ${CMAKE_SOURCE_DIR}/src/test/test_fixtures.cpp
    ${CMAKE_SOURCE_DIR}/src/test/test_reporter.cpp
)

add_library(test_framework STATIC ${TEST_FRAMEWORK_SOURCES})
target_include_directories(test_framework PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_framework PUBLIC redalert_platform)

# Framework self-test
add_executable(TestFrameworkSelfTest
    ${CMAKE_SOURCE_DIR}/src/test_framework_self_test.cpp
)
target_link_libraries(TestFrameworkSelfTest PRIVATE test_framework)
add_test(NAME TestFrameworkSelfTest COMMAND TestFrameworkSelfTest)
```

---

## Verification Script

```bash
#!/bin/bash
set -e
echo "=== Task 18a Verification: Test Framework Foundation ==="

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"

echo "Checking files..."
for f in include/test/test_framework.h \
         include/test/test_fixtures.h \
         include/test/test_reporter.h \
         src/test/test_framework.cpp \
         src/test/test_fixtures.cpp \
         src/test/test_reporter.cpp; do
    [ -f "$ROOT_DIR/$f" ] || { echo "Missing: $f"; exit 1; }
    echo "  $f"
done

echo "Building..."
cd "$BUILD_DIR" && cmake --build . --target test_framework
cmake --build . --target TestFrameworkSelfTest
echo "  Built"

echo "Running self-test..."
"$BUILD_DIR/TestFrameworkSelfTest" || { echo "Self-test failed"; exit 1; }

echo "Testing XML output..."
"$BUILD_DIR/TestFrameworkSelfTest" --xml /tmp/test_results.xml
[ -f /tmp/test_results.xml ] || { echo "XML output not created"; exit 1; }

echo "Testing filtering..."
"$BUILD_DIR/TestFrameworkSelfTest" --category Assertions

echo "Testing list mode..."
"$BUILD_DIR/TestFrameworkSelfTest" --list

echo "=== Task 18a Verification PASSED ==="
```

---

## Implementation Steps

1. **Create directory structure**
   - `include/test/` for headers
   - `src/test/` for implementations

2. **Implement test_framework.h/cpp**
   - TestRegistry singleton
   - TestRunner with configuration
   - Assertion macros
   - TEST_CASE macro for registration

3. **Implement test_fixtures.h/cpp**
   - Base TestFixture class
   - Platform, Graphics, Audio, Asset fixtures
   - TEST_WITH_FIXTURE macro

4. **Implement test_reporter.h/cpp**
   - ConsoleReporter with colors
   - XmlReporter for CI integration
   - MultiReporter for combining outputs

5. **Create self-test**
   - Tests for all assertion macros
   - Tests for skip functionality
   - Tests for fixtures
   - Tests for reporters

6. **Update CMakeLists.txt**
   - Add test_framework library
   - Add self-test executable

---

## Success Criteria

- [ ] TestRegistry properly registers all test cases
- [ ] All assertion macros work correctly
- [ ] TEST_ASSERT_EQ shows actual values on failure
- [ ] TEST_SKIP properly skips tests
- [ ] TEST_WITH_FIXTURE calls SetUp/TearDown
- [ ] ConsoleReporter produces readable output
- [ ] XmlReporter produces valid JUnit XML
- [ ] Command-line filtering works
- [ ] Self-test passes all tests

---

## Common Issues

1. **Static initialization order**: Test registration uses static initializers. Ensure TestRegistry::Instance() is always safe to call.

2. **Exception safety**: TearDown must be called even when tests throw. Use try/catch in the fixture wrapper.

3. **Color output**: Detect if stdout is a TTY before using ANSI colors. Can disable with `--no-color` flag.

4. **XML escaping**: Special characters in test names/messages must be escaped for valid XML.

---

## Completion Promise

When verification passes, output:
<promise>TASK_18A_COMPLETE</promise>

## Escape Hatch

If stuck after 15 iterations, document in `ralph-tasks/blocked/18a.md` and output:
<promise>TASK_18A_BLOCKED</promise>

## Max Iterations
15
