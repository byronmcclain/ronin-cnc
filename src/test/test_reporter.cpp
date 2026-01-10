// src/test/test_reporter.cpp
// Test Output Reporters Implementation
// Task 18a - Test Framework Foundation

#include "test/test_reporter.h"
#include <iostream>
#include <iomanip>
#include <map>

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
    result.reserve(str.size() + str.size() / 10);  // Reserve a bit extra

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
    (void)total_tests;  // Suppress unused parameter warning
    start_time_ = std::chrono::steady_clock::now();
    file_.open(output_path_);

    if (file_.is_open()) {
        file_ << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    }
}

void XmlReporter::OnTestComplete(const TestCaseResult& result) {
    (void)result;  // Individual test results will be written in OnTestRunComplete
}

void XmlReporter::OnTestRunComplete(const std::vector<TestCaseResult>& results,
                                    int passed, int failed, int skipped) {
    (void)passed;
    (void)skipped;

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
