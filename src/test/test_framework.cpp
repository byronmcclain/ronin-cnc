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
    result.failure_line = 0;

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
