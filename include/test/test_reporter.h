// include/test/test_reporter.h
// Test Output Reporters
// Task 18a - Test Framework Foundation

#ifndef TEST_REPORTER_H
#define TEST_REPORTER_H

#include "test/test_framework.h"
#include <string>
#include <vector>
#include <fstream>
#include <memory>
#include <chrono>

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
