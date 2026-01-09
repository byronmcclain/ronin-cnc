# Task 13g: CMake Integration

## Dependencies
- Task 13f (Implementations) must be complete
- CMake build system must be working

## Context
With all compatibility headers and implementations created, we need to integrate them into the CMake build system. This creates a `compat` library that other targets can link against.

## Objective
Update CMakeLists.txt to:
1. Create a `compat` static library
2. Set appropriate include paths
3. Link against platform layer
4. Create a test executable
5. Add to CTest

## Deliverables
- [ ] CMakeLists.txt updated with compat library
- [ ] `compat` library builds successfully
- [ ] `TestCompat` test executable builds
- [ ] CTest integration works
- [ ] Verification script passes

## Files to Modify

### CMakeLists.txt (MODIFY)
Add the following section for the compatibility layer:

```cmake
# =============================================================================
# Compatibility Layer Library
# =============================================================================
# Provides Windows/DOS API compatibility for original Red Alert code

# Compatibility library source
add_library(compat STATIC
    src/compat/compat.cpp
)

# Include directories
target_include_directories(compat PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/compat
)

# Link against platform layer
target_link_libraries(compat PUBLIC
    redalert_platform
)

# Compile definitions
target_compile_definitions(compat PUBLIC
    # These prevent real Windows headers from being included
    _WINDOWS_
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

# C++ standard
target_compile_features(compat PUBLIC cxx_std_17)

# Suppress warnings for legacy compatibility code
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(compat PRIVATE
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-sign-compare
    )
endif()

# =============================================================================
# Compatibility Layer Test
# =============================================================================

add_executable(TestCompat src/test_compat.cpp)

target_link_libraries(TestCompat PRIVATE
    compat
    redalert_platform
)

target_compile_definitions(TestCompat PRIVATE
    # Don't use headers-only mode for the test
)

# Add to CTest
add_test(NAME TestCompat COMMAND TestCompat)
```

### src/test_compat.cpp (NEW)
```cpp
/**
 * Compatibility Layer Test
 *
 * Tests the compatibility layer with the platform layer integrated.
 */

#include "compat/compat.h"
#include "platform.h"
#include <cstdio>
#include <cstring>

// Forward declarations for implementation functions
extern "C" {
    void Compat_NormalizePath(char* path);
    const char* Compat_GetExtension(const char* path);
    const char* Compat_GetFilename(const char* path);
    BOOL Compat_IntersectRect(LPRECT lprcDst, const RECT* lprcSrc1, const RECT* lprcSrc2);
    int Compat_Init(void);
    void Compat_Shutdown(void);
}

#define TEST(name) printf("  Testing %s... ", name)
#define PASS() printf("PASS\n")
#define FAIL(msg) do { printf("FAIL: %s\n", msg); return 1; } while(0)

int test_types() {
    TEST("Windows types");

    // Verify type sizes
    if (sizeof(BOOL) != 4) FAIL("BOOL size");
    if (sizeof(DWORD) != 4) FAIL("DWORD size");
    if (sizeof(WORD) != 2) FAIL("WORD size");
    if (sizeof(BYTE) != 1) FAIL("BYTE size");
    if (sizeof(POINT) != 8) FAIL("POINT size");
    if (sizeof(RECT) != 16) FAIL("RECT size");

    PASS();
    return 0;
}

int test_macros() {
    TEST("Windows macros");

    DWORD dw = 0x12345678;
    if (LOWORD(dw) != 0x5678) FAIL("LOWORD");
    if (HIWORD(dw) != 0x1234) FAIL("HIWORD");

    DWORD color = RGB(255, 128, 64);
    if (GetRValue(color) != 255) FAIL("GetRValue");
    if (GetGValue(color) != 128) FAIL("GetGValue");
    if (GetBValue(color) != 64) FAIL("GetBValue");

    PASS();
    return 0;
}

int test_time_functions() {
    TEST("Time functions");

    DWORD t1 = GetTickCount();
    Sleep(10);
    DWORD t2 = GetTickCount();

    if (t2 <= t1) {
        printf("(t1=%u, t2=%u) ", t1, t2);
        FAIL("GetTickCount not increasing");
    }

    LONGLONG freq = 0, counter = 0;
    if (!QueryPerformanceFrequency(&freq)) FAIL("QueryPerformanceFrequency");
    if (!QueryPerformanceCounter(&counter)) FAIL("QueryPerformanceCounter");
    if (freq == 0) FAIL("Frequency is 0");

    printf("(elapsed=%ums) ", t2 - t1);
    PASS();
    return 0;
}

int test_path_utilities() {
    TEST("Path utilities");

    // Test normalization
    char path[256] = "DATA\\CONQUER\\FILE.MIX";
    Compat_NormalizePath(path);
    if (strchr(path, '\\')) FAIL("Backslash not converted");

    // Test extension extraction
    const char* ext = Compat_GetExtension("FILE.MIX");
    if (strcmp(ext, "MIX") != 0) FAIL("Extension mismatch");

    // Test filename extraction
    const char* fname = Compat_GetFilename("path/to/file.txt");
    if (strcmp(fname, "file.txt") != 0) FAIL("Filename mismatch");

    PASS();
    return 0;
}

int test_rect_operations() {
    TEST("Rectangle operations");

    RECT r1 = {0, 0, 100, 100};
    RECT r2 = {50, 50, 150, 150};
    RECT result;

    // Test intersection
    if (!Compat_IntersectRect(&result, &r1, &r2)) FAIL("Should intersect");
    if (result.left != 50 || result.top != 50) FAIL("Intersection wrong");
    if (result.right != 100 || result.bottom != 100) FAIL("Intersection wrong");

    // Test no intersection
    RECT r3 = {200, 200, 300, 300};
    if (Compat_IntersectRect(&result, &r1, &r3)) FAIL("Should not intersect");

    PASS();
    return 0;
}

int test_keyboard_bridge() {
    TEST("Keyboard bridge");

    // Just verify GetAsyncKeyState doesn't crash
    SHORT state = GetAsyncKeyState(VK_ESCAPE);
    (void)state; // May or may not be pressed

    state = GetAsyncKeyState(VK_SPACE);
    state = GetAsyncKeyState(VK_RETURN);
    state = GetAsyncKeyState('A');
    state = GetAsyncKeyState(VK_F1);

    PASS();
    return 0;
}

int test_mouse_bridge() {
    TEST("Mouse bridge");

    POINT pt;
    if (!GetCursorPos(&pt)) FAIL("GetCursorPos failed");
    // Position can be anything, just verify no crash

    PASS();
    return 0;
}

int test_memory_functions() {
    TEST("Memory functions");

    // Test GlobalAlloc
    HGLOBAL hMem = GlobalAlloc(GPTR, 1024);
    if (!hMem) FAIL("GlobalAlloc failed");

    // Test GlobalLock
    void* ptr = GlobalLock(hMem);
    if (!ptr) FAIL("GlobalLock failed");

    // Write to memory
    memset(ptr, 0xAB, 1024);

    // Test GlobalUnlock
    if (!GlobalUnlock(hMem)) { /* May return FALSE, that's OK */ }

    // Test GlobalFree
    if (GlobalFree(hMem) != NULL) FAIL("GlobalFree should return NULL on success");

    PASS();
    return 0;
}

int test_directx_stubs() {
    TEST("DirectX stubs");

    // DirectDraw should fail to create
    LPDIRECTDRAW lpDD = nullptr;
    HRESULT hr = DirectDrawCreate(nullptr, &lpDD, nullptr);
    if (hr == DD_OK) FAIL("DirectDrawCreate should fail");
    if (lpDD != nullptr) FAIL("lpDD should be null");

    // Test FAILED/SUCCEEDED macros
    if (!FAILED(DDERR_GENERIC)) FAIL("FAILED macro broken");
    if (!SUCCEEDED(DD_OK)) FAIL("SUCCEEDED macro broken");

    PASS();
    return 0;
}

int test_audio_stubs() {
    TEST("Audio stubs");

    // HMI SOS should "succeed" init (pretend audio available)
    if (sosDIGIInitSystem(nullptr, 0) != _SOS_NO_ERROR) {
        FAIL("SOS init should succeed");
    }

    // But sample playback should fail
    HSAMPLE sample = sosDIGIStartSample(0, nullptr);
    if (sample != _SOS_INVALID_HANDLE) {
        FAIL("Sample should not play");
    }

    // GCL should fail (no modem)
    if (gcl_init() != GCL_NOT_INITIALIZED) {
        FAIL("GCL should not initialize");
    }

    PASS();
    return 0;
}

int main() {
    printf("==========================================\n");
    printf("Compatibility Layer Integration Test\n");
    printf("==========================================\n\n");

    // Initialize platform
    if (Platform_Init() != PLATFORM_RESULT_SUCCESS) {
        printf("Failed to initialize platform!\n");
        return 1;
    }

    // Initialize compat layer
    if (Compat_Init() != 0) {
        printf("Failed to initialize compat layer!\n");
        Platform_Shutdown();
        return 1;
    }

    printf("=== Running Tests ===\n\n");

    int failures = 0;
    failures += test_types();
    failures += test_macros();
    failures += test_time_functions();
    failures += test_path_utilities();
    failures += test_rect_operations();
    failures += test_keyboard_bridge();
    failures += test_mouse_bridge();
    failures += test_memory_functions();
    failures += test_directx_stubs();
    failures += test_audio_stubs();

    printf("\n==========================================\n");
    printf("Test Summary\n");
    printf("==========================================\n");

    if (failures == 0) {
        printf("All tests passed!\n");
    } else {
        printf("%d test(s) failed\n", failures);
    }

    printf("==========================================\n");

    // Cleanup
    Compat_Shutdown();
    Platform_Shutdown();

    return failures;
}
```

## Verification Command
```bash
ralph-tasks/verify/check-cmake-compat.sh
```

## Verification Script

### ralph-tasks/verify/check-cmake-compat.sh (NEW)
```bash
#!/bin/bash
# Verification script for CMake compatibility integration

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking CMake Compatibility Integration ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check source files exist
echo "[1/5] Checking source files..."
if [ ! -f "src/compat/compat.cpp" ]; then
    echo "VERIFY_FAILED: src/compat/compat.cpp not found"
    exit 1
fi
if [ ! -f "src/test_compat.cpp" ]; then
    echo "VERIFY_FAILED: src/test_compat.cpp not found"
    exit 1
fi
echo "  OK: Source files exist"

# Step 2: Configure CMake (if needed)
echo "[2/5] Configuring CMake..."
if [ ! -f "build/Makefile" ] && [ ! -f "build/build.ninja" ]; then
    cmake -B build -S . 2>&1 || {
        echo "VERIFY_FAILED: CMake configure failed"
        exit 1
    }
fi
echo "  OK: CMake configured"

# Step 3: Build compat library
echo "[3/5] Building compat library..."
if ! cmake --build build --target compat 2>&1; then
    echo "VERIFY_FAILED: compat library build failed"
    exit 1
fi
echo "  OK: compat library built"

# Step 4: Build TestCompat
echo "[4/5] Building TestCompat..."
if ! cmake --build build --target TestCompat 2>&1; then
    echo "VERIFY_FAILED: TestCompat build failed"
    exit 1
fi
echo "  OK: TestCompat built"

# Step 5: Run TestCompat
echo "[5/5] Running TestCompat..."
if ! ./build/TestCompat; then
    echo "VERIFY_FAILED: TestCompat failed"
    exit 1
fi

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Ensure src/compat/compat.cpp exists (from Task 13f)
2. Create src/test_compat.cpp with the test code above
3. Add the CMake configuration to CMakeLists.txt
4. Run `cmake -B build -S .` to reconfigure
5. Run verification script
6. Fix any issues and repeat

## Success Criteria
- CMake configures without errors
- `compat` library builds successfully
- `TestCompat` executable builds
- All tests in TestCompat pass
- Library links correctly with platform layer

## Common Issues

### "Cannot find compat.cpp"
- Ensure the path in CMakeLists.txt is correct
- File should be at src/compat/compat.cpp

### Link errors with platform
- Ensure redalert_platform target exists
- Check that PUBLIC linking is used

### "Undefined reference to Compat_*"
- Functions must be declared extern "C" for C++ linking
- Check that compat.cpp is actually being compiled

## Completion Promise
When verification passes, output:
```
<promise>TASK_13G_COMPLETE</promise>
```

## Escape Hatch
If stuck after 12 iterations:
- Document what's blocking in `ralph-tasks/blocked/13g.md`
- Output: `<promise>TASK_13G_BLOCKED</promise>`

## Max Iterations
12
