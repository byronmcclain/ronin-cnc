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
