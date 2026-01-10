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
#include <sstream>

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

#define TEST_CASE(test_name, test_category) \
    static void TestFunc_##test_name(); \
    static struct TestRegistrar_##test_name { \
        TestRegistrar_##test_name() { \
            TestCaseInfo info; \
            info.name = #test_name; \
            info.category = test_category; \
            info.file = __FILE__; \
            info.line = __LINE__; \
            info.timeout_ms = 0; \
            TestRegistry::Instance().RegisterTest(info, TestFunc_##test_name); \
        } \
    } g_test_registrar_##test_name; \
    static void TestFunc_##test_name()

#define TEST_CASE_TIMEOUT(test_name, test_category, test_timeout_ms) \
    static void TestFunc_##test_name(); \
    static struct TestRegistrar_##test_name { \
        TestRegistrar_##test_name() { \
            TestCaseInfo info; \
            info.name = #test_name; \
            info.category = test_category; \
            info.file = __FILE__; \
            info.line = __LINE__; \
            info.timeout_ms = test_timeout_ms; \
            TestRegistry::Instance().RegisterTest(info, TestFunc_##test_name); \
        } \
    } g_test_registrar_##test_name; \
    static void TestFunc_##test_name()

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
