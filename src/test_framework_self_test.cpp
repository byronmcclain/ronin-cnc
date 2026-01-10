// src/test_framework_self_test.cpp
// Self-test for the test framework
// Task 18a - Test Framework Foundation

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "test/test_reporter.h"
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

TEST_CASE(XmlReporter_Creation, "Reporters") {
    // XmlReporter should be creatable
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
