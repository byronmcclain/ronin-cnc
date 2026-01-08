# Task 11h: Integration Test Infrastructure

## Dependencies
- All previous Phase 11 tasks should be complete
- CMake build system working

## Context
This task creates a comprehensive test infrastructure for verifying the platform layer works correctly. Rather than testing individual functions (unit tests), integration tests verify that:
- All modules work together
- FFI interfaces are correct
- The app initializes and shuts down cleanly
- Platform-specific features work on macOS

Tests are written in both Rust (internal verification) and C/C++ (FFI verification).

## Objective
Create an integration test infrastructure that:
1. Tests all platform subsystems together
2. Verifies FFI function signatures match headers
3. Provides automated test scripts
4. Generates test reports
5. Integrates with CI/CD

## Deliverables
- [ ] `platform/src/tests/mod.rs` - Rust integration tests
- [ ] `src/test_platform.cpp` - C++ FFI integration tests
- [ ] `scripts/run-tests.sh` - Master test runner script
- [ ] Updated CMakeLists.txt with test targets
- [ ] Verification passes

## Technical Background

### Test Categories
1. **Initialization Tests**: Platform init/shutdown cycles
2. **Graphics Tests**: Window creation, rendering (headless if possible)
3. **Input Tests**: Event polling (automated events)
4. **Audio Tests**: Device opening, playback (silent)
5. **Network Tests**: Loopback connections
6. **File System Tests**: Path operations, file I/O
7. **Performance Tests**: Timing accuracy

### Test Levels
- **Smoke Tests**: Quick sanity check (<5 seconds)
- **Unit Tests**: Individual function tests
- **Integration Tests**: Multi-module tests
- **System Tests**: Full application tests

### Headless Testing
Graphics tests can run headless using SDL's dummy video driver:
```bash
SDL_VIDEODRIVER=dummy ./test_platform
```

## Files to Create/Modify

### src/test_platform.cpp (NEW)
```cpp
/**
 * Platform Layer Integration Tests
 *
 * Tests all platform FFI functions from C++ to verify
 * the interface works correctly.
 */

#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// =============================================================================
// Test Framework (Simple)
// =============================================================================

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static void test_##name(void)
#define RUN_TEST(name) do { \
    printf("  Running: %s...", #name); \
    tests_run++; \
    test_##name(); \
    printf(" PASSED\n"); \
    tests_passed++; \
} while(0)

#define ASSERT(expr) do { \
    if (!(expr)) { \
        printf(" FAILED\n"); \
        printf("    Assertion failed: %s\n", #expr); \
        printf("    At line %d\n", __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b) ASSERT((a) == (b))
#define ASSERT_NE(a, b) ASSERT((a) != (b))
#define ASSERT_GT(a, b) ASSERT((a) > (b))
#define ASSERT_GE(a, b) ASSERT((a) >= (b))

// =============================================================================
// Platform Initialization Tests
// =============================================================================

TEST(platform_version) {
    int version = Platform_GetVersion();
    ASSERT_GT(version, 0);
}

TEST(platform_init_shutdown) {
    // First ensure we're in clean state
    Platform_Shutdown();

    // Init should succeed
    int result = (int)Platform_Init();
    ASSERT_EQ(result, 0);  // PlatformResult::Success

    // Should be initialized
    ASSERT(Platform_IsInitialized());

    // Double init should fail
    result = (int)Platform_Init();
    ASSERT_EQ(result, 1);  // PlatformResult::AlreadyInitialized

    // Shutdown should succeed
    result = (int)Platform_Shutdown();
    ASSERT_EQ(result, 0);

    // Should not be initialized
    ASSERT(!Platform_IsInitialized());

    // Double shutdown should fail
    result = (int)Platform_Shutdown();
    ASSERT_EQ(result, 2);  // PlatformResult::NotInitialized
}

// =============================================================================
// Configuration Path Tests
// =============================================================================

TEST(config_path) {
    char buffer[512];
    int len = Platform_GetConfigPath(buffer, sizeof(buffer));

    ASSERT_GT(len, 0);
    ASSERT(strstr(buffer, "RedAlert") != NULL);
}

TEST(saves_path) {
    char buffer[512];
    int len = Platform_GetSavesPath(buffer, sizeof(buffer));

    ASSERT_GT(len, 0);
    ASSERT(strstr(buffer, "saves") != NULL);
}

TEST(log_path) {
    char buffer[512];
    int len = Platform_GetLogPath(buffer, sizeof(buffer));

    ASSERT_GT(len, 0);
    ASSERT(strstr(buffer, "log") != NULL);
}

TEST(ensure_directories) {
    int result = Platform_EnsureDirectories();
    ASSERT_EQ(result, 0);
}

TEST(path_null_safety) {
    // Null buffer should return -1
    ASSERT_EQ(Platform_GetConfigPath(NULL, 100), -1);
    ASSERT_EQ(Platform_GetSavesPath(NULL, 100), -1);

    // Zero size should return -1
    char buffer[10];
    ASSERT_EQ(Platform_GetConfigPath(buffer, 0), -1);
}

// =============================================================================
// Logging Tests
// =============================================================================

TEST(log_init_shutdown) {
    // Init logging (may already be initialized)
    int result = Platform_Log_Init();
    ASSERT(result == 0 || result == -1);  // Success or already init

    // Log a message (should not crash)
    Platform_Log(2, "Test log message from C++");

    // Flush
    Platform_Log_Flush();

    Platform_Log_Shutdown();
}

TEST(log_levels) {
    Platform_Log_SetLevel(3);  // Debug
    ASSERT_EQ(Platform_Log_GetLevel(), 3);

    Platform_Log_SetLevel(1);  // Warn
    ASSERT_EQ(Platform_Log_GetLevel(), 1);

    Platform_Log_SetLevel(2);  // Reset to Info
}

// =============================================================================
// Application Lifecycle Tests
// =============================================================================

TEST(app_state) {
    // Default should be active
    int state = Platform_GetAppState();
    ASSERT_GE(state, 0);
    ASSERT(state <= 2);  // Valid states: 0, 1, 2
}

TEST(app_active) {
    int active = Platform_IsAppActive();
    ASSERT(active == 0 || active == 1);
}

TEST(quit_request) {
    // Should not be quit initially (unless something set it)
    Platform_ClearQuitRequest();
    ASSERT_EQ(Platform_ShouldQuit(), 0);

    Platform_RequestQuit();
    ASSERT_EQ(Platform_ShouldQuit(), 1);

    Platform_ClearQuitRequest();
    ASSERT_EQ(Platform_ShouldQuit(), 0);
}

// =============================================================================
// Performance Tests
// =============================================================================

TEST(frame_timing) {
    Platform_FrameStart();
    Platform_FrameEnd();

    // FPS should be non-negative
    int fps = Platform_GetFPS();
    ASSERT_GE(fps, 0);

    // Frame time should be non-negative
    int frame_time = Platform_GetFrameTime();
    ASSERT_GE(frame_time, 0);

    // Frame count should increment
    uint32_t count1 = Platform_GetFrameCount();
    Platform_FrameStart();
    Platform_FrameEnd();
    uint32_t count2 = Platform_GetFrameCount();
    ASSERT_GT(count2, count1);
}

// =============================================================================
// Network Tests
// =============================================================================

TEST(network_init_shutdown) {
    // Init
    int result = Platform_Network_Init();
    ASSERT(result == 0 || result == -1);  // Success or already init

    if (result == 0) {
        ASSERT_EQ(Platform_Network_IsInitialized(), 1);

        Platform_Network_Shutdown();
        ASSERT_EQ(Platform_Network_IsInitialized(), 0);
    }
}

// =============================================================================
// Test Runner
// =============================================================================

void run_smoke_tests(void) {
    printf("\n=== Smoke Tests ===\n");
    RUN_TEST(platform_version);
}

void run_init_tests(void) {
    printf("\n=== Initialization Tests ===\n");
    RUN_TEST(platform_init_shutdown);
}

void run_config_tests(void) {
    printf("\n=== Configuration Tests ===\n");
    RUN_TEST(config_path);
    RUN_TEST(saves_path);
    RUN_TEST(log_path);
    RUN_TEST(ensure_directories);
    RUN_TEST(path_null_safety);
}

void run_logging_tests(void) {
    printf("\n=== Logging Tests ===\n");
    RUN_TEST(log_init_shutdown);
    RUN_TEST(log_levels);
}

void run_lifecycle_tests(void) {
    printf("\n=== Lifecycle Tests ===\n");
    RUN_TEST(app_state);
    RUN_TEST(app_active);
    RUN_TEST(quit_request);
}

void run_performance_tests(void) {
    printf("\n=== Performance Tests ===\n");
    RUN_TEST(frame_timing);
}

void run_network_tests(void) {
    printf("\n=== Network Tests ===\n");
    RUN_TEST(network_init_shutdown);
}

int main(int argc, char* argv[]) {
    printf("============================================\n");
    printf("Platform Integration Tests\n");
    printf("============================================\n");

    bool quick = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--quick") == 0 || strcmp(argv[i], "-q") == 0) {
            quick = true;
        }
    }

    // Always run smoke tests
    run_smoke_tests();

    if (!quick) {
        run_init_tests();
        run_config_tests();
        run_logging_tests();
        run_lifecycle_tests();
        run_performance_tests();
        run_network_tests();
    }

    printf("\n============================================\n");
    printf("Results: %d/%d tests passed", tests_passed, tests_run);
    if (tests_failed > 0) {
        printf(" (%d FAILED)", tests_failed);
    }
    printf("\n============================================\n");

    return tests_failed > 0 ? 1 : 0;
}
```

### scripts/run-tests.sh (NEW)
```bash
#!/bin/bash
# Master test runner script
#
# Usage: ./scripts/run-tests.sh [--quick] [--rust-only] [--cpp-only]

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# Parse arguments
QUICK=""
RUST_TESTS=true
CPP_TESTS=true

for arg in "$@"; do
    case "$arg" in
        --quick|-q)
            QUICK="--quick"
            ;;
        --rust-only)
            CPP_TESTS=false
            ;;
        --cpp-only)
            RUST_TESTS=false
            ;;
    esac
done

echo "============================================"
echo "Running Platform Tests"
echo "============================================"
echo ""

FAILURES=0

# Run Rust tests
if [ "$RUST_TESTS" = true ]; then
    echo "=== Rust Tests ==="
    echo ""

    cd "$ROOT_DIR"

    # Unit tests
    echo "Running unit tests..."
    if cargo test --manifest-path platform/Cargo.toml --release -- --test-threads=1; then
        echo "  Rust unit tests PASSED"
    else
        echo "  Rust unit tests FAILED"
        FAILURES=$((FAILURES + 1))
    fi
    echo ""
fi

# Build and run C++ tests
if [ "$CPP_TESTS" = true ]; then
    echo "=== C++ Integration Tests ==="
    echo ""

    cd "$ROOT_DIR"

    # Ensure project is built
    if [ ! -d "build" ]; then
        echo "Building project..."
        cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    fi
    cmake --build build

    # Check for test executable
    TEST_EXE="build/TestPlatform"
    if [ ! -f "$TEST_EXE" ]; then
        echo "WARNING: TestPlatform not found, skipping C++ tests"
    else
        echo "Running C++ integration tests..."

        # Set SDL to use dummy driver for headless testing
        export SDL_VIDEODRIVER=dummy
        export SDL_AUDIODRIVER=dummy

        if "$TEST_EXE" $QUICK; then
            echo "  C++ integration tests PASSED"
        else
            echo "  C++ integration tests FAILED"
            FAILURES=$((FAILURES + 1))
        fi
    fi
    echo ""
fi

# Summary
echo "============================================"
if [ $FAILURES -eq 0 ]; then
    echo "All tests PASSED"
    exit 0
else
    echo "$FAILURES test suite(s) FAILED"
    exit 1
fi
```

### CMakeLists.txt (MODIFY)
Add test target:
```cmake
# =============================================================================
# Platform Integration Test
# =============================================================================

add_executable(TestPlatform src/test_platform.cpp)

target_include_directories(TestPlatform PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

add_dependencies(TestPlatform generate_headers)

target_link_libraries(TestPlatform PRIVATE redalert_platform)

if(APPLE)
    target_link_libraries(TestPlatform PRIVATE
        "-framework CoreFoundation"
        "-framework Security"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework Carbon"
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework ForceFeedback"
        "-framework CoreVideo"
        "-framework Metal"
        "-framework QuartzCore"
        "-framework GameController"
        "-framework CoreHaptics"
        iconv
    )
endif()

# Enable testing
enable_testing()
add_test(NAME PlatformTests COMMAND TestPlatform)
add_test(NAME PlatformQuickTests COMMAND TestPlatform --quick)
```

## Verification Command
```bash
ralph-tasks/verify/check-test-infrastructure.sh
```

## Verification Script
Create `ralph-tasks/verify/check-test-infrastructure.sh`:
```bash
#!/bin/bash
# Verification script for test infrastructure

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Test Infrastructure ==="

cd "$ROOT_DIR"

# Step 1: Check test files exist
echo "Step 1: Checking test files exist..."
if [ ! -f "src/test_platform.cpp" ]; then
    echo "VERIFY_FAILED: src/test_platform.cpp not found"
    exit 1
fi
if [ ! -f "scripts/run-tests.sh" ]; then
    echo "VERIFY_FAILED: scripts/run-tests.sh not found"
    exit 1
fi
echo "  Test files exist"

# Step 2: Check scripts are executable
echo "Step 2: Checking scripts are executable..."
if [ ! -x "scripts/run-tests.sh" ]; then
    chmod +x "scripts/run-tests.sh"
fi
echo "  Scripts are executable"

# Step 3: Build project with tests
echo "Step 3: Building project with tests..."
mkdir -p build
cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release 2>&1 || exit 1
cmake --build . 2>&1 || exit 1
cd "$ROOT_DIR"
echo "  Build successful"

# Step 4: Check test executable exists
echo "Step 4: Checking test executable..."
if [ ! -f "build/TestPlatform" ]; then
    echo "VERIFY_FAILED: TestPlatform executable not built"
    exit 1
fi
echo "  TestPlatform built"

# Step 5: Run quick smoke test
echo "Step 5: Running smoke tests..."
export SDL_VIDEODRIVER=dummy
export SDL_AUDIODRIVER=dummy

if ! ./build/TestPlatform --quick 2>&1; then
    echo "VERIFY_FAILED: Smoke tests failed"
    exit 1
fi
echo "  Smoke tests passed"

# Step 6: Run Rust tests
echo "Step 6: Running Rust unit tests..."
if ! cargo test --manifest-path platform/Cargo.toml --release -- --test-threads=1 2>&1 | tail -5; then
    echo "VERIFY_FAILED: Rust tests failed"
    exit 1
fi
echo "  Rust tests passed"

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Create `src/test_platform.cpp`
2. Create `scripts/run-tests.sh` and make executable
3. Update CMakeLists.txt with TestPlatform target
4. Build project: `cmake -B build -G Ninja && cmake --build build`
5. Run smoke tests: `./build/TestPlatform --quick`
6. Run full tests: `./scripts/run-tests.sh`
7. Run verification script

## Success Criteria
- TestPlatform compiles without errors
- Smoke tests pass (--quick flag)
- All integration tests pass
- Rust tests pass
- Verification script exits with 0

## Common Issues

### "undefined reference to Platform_*"
Ensure the platform library is linked correctly in CMakeLists.txt.

### "SDL video initialization failed"
Use dummy drivers for headless testing:
```bash
export SDL_VIDEODRIVER=dummy
export SDL_AUDIODRIVER=dummy
```

### "test failed but no output"
Tests use simple assertions. Check the line number in output.

### Timeout during tests
Some tests may hang if SDL initialization fails. Use --quick for smoke tests.

## Completion Promise
When verification passes, output:
```
<promise>TASK_11H_COMPLETE</promise>
```

## Escape Hatch
If stuck after 10 iterations:
- Document what's blocking in `ralph-tasks/blocked/11h.md`
- List attempted approaches
- Output: `<promise>TASK_11H_BLOCKED</promise>`

## Max Iterations
10
