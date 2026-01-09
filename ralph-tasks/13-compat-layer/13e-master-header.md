# Task 13e: Master Compatibility Header

## Dependencies
- Task 13a (Windows Types) must be complete
- Task 13b (Watcom Compatibility) must be complete
- Task 13c (DirectX Stubs) must be complete
- Task 13d (Audio Stubs) must be complete

## Context
With all individual compatibility headers created, we need a master header that:
1. Includes all sub-headers in the correct order
2. Bridges Windows API functions to our platform layer
3. Provides any additional compatibility macros

This is the single header that original CODE files will include to compile on macOS.

## Objective
Create `include/compat/compat.h` that:
1. Includes all compat sub-headers in dependency order
2. Implements inline wrappers for key Windows functions
3. Bridges to platform layer (GetTickCount → Platform_Timer_GetTicks)
4. Ensures no conflicts between headers

## Technical Background

### Include Order
Headers must be included in dependency order:
1. `watcom.h` - Compiler compatibility (defines calling conventions)
2. `windows.h` - Base types (uses calling conventions)
3. `directx.h` - DirectX types (depends on Windows types)
4. `hmi_sos.h` - Audio stubs (standalone)
5. `gcl.h` - Communications stubs (standalone)
6. `platform.h` - Platform layer (for bridging)

### Function Bridges
Key functions that need bridging:
```cpp
GetTickCount()  → Platform_Timer_GetTicks()
Sleep()         → Platform_Timer_Delay()
MessageBoxA()   → Platform log + return IDOK
GetAsyncKeyState() → Platform_Key_IsPressed()
```

### Conflict Prevention
Multiple guards prevent including real system headers:
- `_WINDOWS_` - Main Windows.h guard
- `WIN32_LEAN_AND_MEAN` - Exclude rarely-used stuff
- `NOMINMAX` - Don't define min/max macros

## Deliverables
- [ ] `include/compat/compat.h` - Master compatibility header
- [ ] All function bridges implemented
- [ ] No include order conflicts
- [ ] Verification script passes

## Files to Create

### include/compat/compat.h (NEW)
```cpp
/**
 * Master Compatibility Header for Red Alert macOS Port
 *
 * This single header replaces all Windows/DOS includes in the original code.
 * It includes all compatibility sub-headers and provides function bridges
 * to the platform layer.
 *
 * Usage in original CODE files:
 *   #include "compat/compat.h"
 *   // instead of:
 *   // #include <windows.h>
 *   // #include <ddraw.h>
 *   // etc.
 */

#ifndef COMPAT_H
#define COMPAT_H

// =============================================================================
// Version and Configuration
// =============================================================================

#define COMPAT_VERSION 1
#define COMPAT_VERSION_STRING "1.0.0"

// Enable to get debug output from compat layer
// #define COMPAT_DEBUG 1

// =============================================================================
// Include All Compatibility Headers
// =============================================================================
// Order matters! Dependencies must come first.

// 1. Compiler compatibility (calling conventions, pragmas)
#include "compat/watcom.h"

// 2. Windows types and structures
#include "compat/windows.h"

// 3. DirectX stubs
#include "compat/directx.h"

// 4. Audio library stubs
#include "compat/hmi_sos.h"

// 5. Communications library stubs
#include "compat/gcl.h"

// =============================================================================
// Platform Layer Bridge
// =============================================================================
// Only include platform.h if we're actually building the game
// (not just checking header syntax)

#ifndef COMPAT_HEADERS_ONLY
#include "platform.h"
#endif

// =============================================================================
// Windows API Function Bridges
// =============================================================================

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// Time Functions
// -----------------------------------------------------------------------------

#ifndef COMPAT_HEADERS_ONLY

/// GetTickCount - Get milliseconds since system start
/// Bridges to Platform_Timer_GetTicks()
static inline DWORD GetTickCount(void) {
    return (DWORD)Platform_Timer_GetTicks();
}

/// timeGetTime - Same as GetTickCount (multimedia timer)
static inline DWORD timeGetTime(void) {
    return GetTickCount();
}

/// Sleep - Pause execution for specified milliseconds
/// Bridges to Platform_Timer_Delay()
static inline void Sleep(DWORD dwMilliseconds) {
    Platform_Timer_Delay(dwMilliseconds);
}

/// QueryPerformanceCounter - High-resolution timer
static inline BOOL QueryPerformanceCounter(LONGLONG* lpCounter) {
    if (lpCounter) {
        *lpCounter = (LONGLONG)Platform_Timer_GetPerformanceCounter();
        return TRUE;
    }
    return FALSE;
}

/// QueryPerformanceFrequency - Timer frequency
static inline BOOL QueryPerformanceFrequency(LONGLONG* lpFrequency) {
    if (lpFrequency) {
        *lpFrequency = (LONGLONG)Platform_Timer_GetPerformanceFrequency();
        return TRUE;
    }
    return FALSE;
}

#else
// When only checking headers, provide dummy implementations
static inline DWORD GetTickCount(void) { return 0; }
static inline DWORD timeGetTime(void) { return 0; }
static inline void Sleep(DWORD ms) { (void)ms; }
#endif // COMPAT_HEADERS_ONLY

// -----------------------------------------------------------------------------
// Message Box
// -----------------------------------------------------------------------------

/// MessageBoxA - Show a message box
/// Logs the message and returns IDOK
/// In the future, could use native macOS dialogs
static inline int MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) {
    (void)hWnd;
    (void)uType;

#ifndef COMPAT_HEADERS_ONLY
    // Log the message
    Platform_LogInfo("[%s] %s", lpCaption ? lpCaption : "Message", lpText ? lpText : "");
#endif

    // Determine return value based on type
    switch (uType & 0x0F) {
        case MB_YESNO:
        case MB_YESNOCANCEL:
            return IDYES; // Default to Yes
        case MB_OKCANCEL:
        case MB_RETRYCANCEL:
            return IDOK;
        case MB_ABORTRETRYIGNORE:
            return IDIGNORE;
        default:
            return IDOK;
    }
}

#define MessageBox MessageBoxA

// -----------------------------------------------------------------------------
// Keyboard State
// -----------------------------------------------------------------------------

#ifndef COMPAT_HEADERS_ONLY

/// GetAsyncKeyState - Check if key is currently pressed
/// Bridges to Platform_Key_IsPressed()
static inline SHORT GetAsyncKeyState(int vKey) {
    // Map VK codes to platform key codes (simplified)
    KeyCode platformKey;
    switch (vKey) {
        case VK_ESCAPE:  platformKey = KEY_CODE_ESCAPE; break;
        case VK_RETURN:  platformKey = KEY_CODE_RETURN; break;
        case VK_SPACE:   platformKey = KEY_CODE_SPACE; break;
        case VK_LEFT:    platformKey = KEY_CODE_LEFT; break;
        case VK_UP:      platformKey = KEY_CODE_UP; break;
        case VK_RIGHT:   platformKey = KEY_CODE_RIGHT; break;
        case VK_DOWN:    platformKey = KEY_CODE_DOWN; break;
        case VK_SHIFT:   platformKey = KEY_CODE_LSHIFT; break;
        case VK_CONTROL: platformKey = KEY_CODE_LCTRL; break;
        case VK_MENU:    platformKey = KEY_CODE_LALT; break;
        case VK_TAB:     platformKey = KEY_CODE_TAB; break;
        default:
            // For alphanumeric, VK code often matches ASCII
            if (vKey >= 'A' && vKey <= 'Z') {
                platformKey = (KeyCode)(KEY_CODE_A + (vKey - 'A'));
            } else if (vKey >= '0' && vKey <= '9') {
                platformKey = (KeyCode)(KEY_CODE_0 + (vKey - '0'));
            } else if (vKey >= VK_F1 && vKey <= VK_F12) {
                platformKey = (KeyCode)(KEY_CODE_F1 + (vKey - VK_F1));
            } else {
                return 0; // Unknown key
            }
    }

    // Return value: bit 15 = currently down, bit 0 = was pressed since last call
    if (Platform_Key_IsPressed(platformKey)) {
        return (SHORT)0x8001; // Currently pressed
    }
    return 0;
}

/// GetKeyState - Similar to GetAsyncKeyState but checks message queue
/// We just alias to GetAsyncKeyState
static inline SHORT GetKeyState(int vKey) {
    return GetAsyncKeyState(vKey);
}

#else
static inline SHORT GetAsyncKeyState(int vKey) { (void)vKey; return 0; }
static inline SHORT GetKeyState(int vKey) { (void)vKey; return 0; }
#endif

// -----------------------------------------------------------------------------
// Mouse Functions (Stubbed)
// -----------------------------------------------------------------------------

/// GetCursorPos - Get mouse cursor position
static inline BOOL GetCursorPos(LPPOINT lpPoint) {
    if (lpPoint) {
#ifndef COMPAT_HEADERS_ONLY
        int32_t x, y;
        Platform_Mouse_GetPosition(&x, &y);
        lpPoint->x = x;
        lpPoint->y = y;
#else
        lpPoint->x = 0;
        lpPoint->y = 0;
#endif
        return TRUE;
    }
    return FALSE;
}

/// SetCursorPos - Set mouse cursor position (stubbed)
static inline BOOL SetCursorPos(int X, int Y) {
    (void)X; (void)Y;
    // Platform layer doesn't support cursor warping currently
    return TRUE;
}

/// ShowCursor - Show/hide cursor (stubbed)
static inline int ShowCursor(BOOL bShow) {
    (void)bShow;
    // TODO: Could implement via Platform_Mouse_ShowCursor()
    return bShow ? 1 : 0;
}

// -----------------------------------------------------------------------------
// Window Functions (Stubbed - we use fullscreen)
// -----------------------------------------------------------------------------

static inline HWND GetActiveWindow(void) { return NULL; }
static inline HWND GetFocus(void) { return NULL; }
static inline HWND SetFocus(HWND hWnd) { (void)hWnd; return NULL; }
static inline HWND GetForegroundWindow(void) { return NULL; }
static inline BOOL SetForegroundWindow(HWND hWnd) { (void)hWnd; return TRUE; }
static inline BOOL ShowWindow(HWND hWnd, int nCmdShow) { (void)hWnd; (void)nCmdShow; return TRUE; }
static inline BOOL UpdateWindow(HWND hWnd) { (void)hWnd; return TRUE; }
static inline BOOL InvalidateRect(HWND hWnd, const RECT* lpRect, BOOL bErase) {
    (void)hWnd; (void)lpRect; (void)bErase; return TRUE;
}
static inline BOOL MoveWindow(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint) {
    (void)hWnd; (void)X; (void)Y; (void)nWidth; (void)nHeight; (void)bRepaint;
    return TRUE;
}
static inline BOOL GetClientRect(HWND hWnd, LPRECT lpRect) {
    (void)hWnd;
    if (lpRect) {
        lpRect->left = 0;
        lpRect->top = 0;
        lpRect->right = 640;  // Default game resolution
        lpRect->bottom = 400;
        return TRUE;
    }
    return FALSE;
}
static inline BOOL GetWindowRect(HWND hWnd, LPRECT lpRect) {
    return GetClientRect(hWnd, lpRect);
}

// -----------------------------------------------------------------------------
// GDI Functions (Stubbed)
// -----------------------------------------------------------------------------

static inline HDC GetDC(HWND hWnd) { (void)hWnd; return NULL; }
static inline int ReleaseDC(HWND hWnd, HDC hDC) { (void)hWnd; (void)hDC; return 1; }
static inline HDC CreateCompatibleDC(HDC hdc) { (void)hdc; return NULL; }
static inline BOOL DeleteDC(HDC hdc) { (void)hdc; return TRUE; }
static inline HBITMAP CreateCompatibleBitmap(HDC hdc, int cx, int cy) { (void)hdc; (void)cx; (void)cy; return NULL; }
static inline HGDIOBJ SelectObject(HDC hdc, HGDIOBJ h) { (void)hdc; (void)h; return NULL; }
static inline BOOL DeleteObject(HGDIOBJ ho) { (void)ho; return TRUE; }
static inline HPALETTE CreatePalette(const LOGPALETTE* plpal) { (void)plpal; return NULL; }
static inline HPALETTE SelectPalette(HDC hdc, HPALETTE hPal, BOOL bForceBkgd) { (void)hdc; (void)hPal; (void)bForceBkgd; return NULL; }
static inline UINT RealizePalette(HDC hdc) { (void)hdc; return 0; }
static inline int SetDIBitsToDevice(HDC hdc, int xDest, int yDest, DWORD w, DWORD h,
                                     int xSrc, int ySrc, UINT StartScan, UINT cLines,
                                     const void* lpvBits, const BITMAPINFO* lpbmi, UINT ColorUse) {
    (void)hdc; (void)xDest; (void)yDest; (void)w; (void)h;
    (void)xSrc; (void)ySrc; (void)StartScan; (void)cLines;
    (void)lpvBits; (void)lpbmi; (void)ColorUse;
    return 0;
}

// -----------------------------------------------------------------------------
// Memory Functions
// -----------------------------------------------------------------------------

static inline HGLOBAL GlobalAlloc(UINT uFlags, SIZE_T dwBytes) {
    (void)uFlags;
#ifndef COMPAT_HEADERS_ONLY
    return Platform_Alloc(dwBytes, (uFlags & GMEM_ZEROINIT) ? 0x0004 : 0);
#else
    (void)dwBytes;
    return NULL;
#endif
}

static inline HGLOBAL GlobalFree(HGLOBAL hMem) {
#ifndef COMPAT_HEADERS_ONLY
    if (hMem) Platform_Free(hMem, 0);
#else
    (void)hMem;
#endif
    return NULL;
}

static inline LPVOID GlobalLock(HGLOBAL hMem) {
    return hMem; // Memory is always "locked" in flat model
}

static inline BOOL GlobalUnlock(HGLOBAL hMem) {
    (void)hMem;
    return TRUE;
}

// -----------------------------------------------------------------------------
// String Functions
// -----------------------------------------------------------------------------

static inline int lstrlenA(LPCSTR lpString) {
    if (!lpString) return 0;
    int len = 0;
    while (lpString[len]) len++;
    return len;
}

static inline LPSTR lstrcpyA(LPSTR lpString1, LPCSTR lpString2) {
    if (!lpString1) return NULL;
    if (!lpString2) { lpString1[0] = '\0'; return lpString1; }
    char* dst = lpString1;
    while ((*dst++ = *lpString2++) != '\0');
    return lpString1;
}

static inline int lstrcmpA(LPCSTR lpString1, LPCSTR lpString2) {
    if (!lpString1 || !lpString2) return lpString1 == lpString2 ? 0 : (lpString1 ? 1 : -1);
    while (*lpString1 && *lpString1 == *lpString2) { lpString1++; lpString2++; }
    return (unsigned char)*lpString1 - (unsigned char)*lpString2;
}

static inline int lstrcmpiA(LPCSTR lpString1, LPCSTR lpString2) {
    if (!lpString1 || !lpString2) return lpString1 == lpString2 ? 0 : (lpString1 ? 1 : -1);
    while (*lpString1 && ((*lpString1 | 0x20) == (*lpString2 | 0x20))) { lpString1++; lpString2++; }
    return (unsigned char)(*lpString1 | 0x20) - (unsigned char)(*lpString2 | 0x20);
}

#define lstrlen lstrlenA
#define lstrcpy lstrcpyA
#define lstrcmp lstrcmpA
#define lstrcmpi lstrcmpiA

// -----------------------------------------------------------------------------
// Misc Stubs
// -----------------------------------------------------------------------------

static inline BOOL CloseHandle(HANDLE hObject) { (void)hObject; return TRUE; }
static inline DWORD GetLastError(void) { return 0; }
static inline void SetLastError(DWORD dwErrCode) { (void)dwErrCode; }
static inline HMODULE GetModuleHandleA(LPCSTR lpModuleName) { (void)lpModuleName; return NULL; }
static inline DWORD GetCurrentDirectory(DWORD nBufferLength, LPSTR lpBuffer) {
    (void)nBufferLength; (void)lpBuffer; return 0;
}
static inline BOOL SetCurrentDirectory(LPCSTR lpPathName) { (void)lpPathName; return TRUE; }

#ifdef __cplusplus
}
#endif

// =============================================================================
// Additional Compatibility Macros
// =============================================================================

// Calling convention macros for WinMain
#ifndef WINAPI
#define WINAPI
#endif

#ifndef CALLBACK
#define CALLBACK
#endif

// WinMain signature (will be replaced with standard main)
#define WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow) \
    main(int argc, char* argv[])

// UNREFERENCED_PARAMETER macro
#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P) (void)(P)
#endif

// ZeroMemory macro (if not already defined)
#ifndef ZeroMemory
#define ZeroMemory(Destination, Length) memset((Destination), 0, (Length))
#endif

#ifndef CopyMemory
#define CopyMemory(Destination, Source, Length) memcpy((Destination), (Source), (Length))
#endif

#ifndef FillMemory
#define FillMemory(Destination, Length, Fill) memset((Destination), (Fill), (Length))
#endif

// =============================================================================
// End of Master Compatibility Header
// =============================================================================

#endif // COMPAT_H
```

## Verification Command
```bash
ralph-tasks/verify/check-master-header.sh
```

## Verification Script

### ralph-tasks/verify/check-master-header.sh (NEW)
```bash
#!/bin/bash
# Verification script for master compatibility header

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Master Compatibility Header ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check all headers exist
echo "[1/5] Checking all header files exist..."
for header in watcom.h windows.h directx.h hmi_sos.h gcl.h compat.h; do
    if [ ! -f "include/compat/$header" ]; then
        echo "VERIFY_FAILED: include/compat/$header not found"
        exit 1
    fi
done
echo "  OK: All headers exist"

# Step 2: Syntax check (headers only mode)
echo "[2/5] Checking syntax (headers only)..."
if ! clang++ -std=c++17 -fsyntax-only -DCOMPAT_HEADERS_ONLY -I include include/compat/compat.h 2>&1; then
    echo "VERIFY_FAILED: Master header has syntax errors"
    exit 1
fi
echo "  OK: Syntax valid"

# Step 3: Compile test with headers only
echo "[3/5] Compiling test program (headers only mode)..."
cat > /tmp/test_compat.cpp << 'EOF'
#define COMPAT_HEADERS_ONLY
#include "compat/compat.h"
#include <cstdio>

int main() {
    printf("Testing compatibility header...\n\n");

    // Test Windows types
    DWORD dw = 0x12345678;
    WORD w = LOWORD(dw);
    printf("LOWORD(0x12345678) = 0x%04X (expected 0x5678)\n", w);
    if (w != 0x5678) return 1;

    w = HIWORD(dw);
    printf("HIWORD(0x12345678) = 0x%04X (expected 0x1234)\n", w);
    if (w != 0x1234) return 1;

    // Test RGB macros
    DWORD color = RGB(255, 128, 64);
    printf("RGB(255, 128, 64) = 0x%06X\n", (unsigned)color);
    if (GetRValue(color) != 255) return 1;
    if (GetGValue(color) != 128) return 1;
    if (GetBValue(color) != 64) return 1;
    printf("  GetRValue = %d, GetGValue = %d, GetBValue = %d\n",
           GetRValue(color), GetGValue(color), GetBValue(color));

    // Test function stubs
    DWORD ticks = GetTickCount();
    printf("GetTickCount() = %u\n", ticks);

    Sleep(1);
    printf("Sleep(1) completed\n");

    int result = MessageBoxA(NULL, "Test message", "Test", MB_OK);
    printf("MessageBoxA returned %d (expected IDOK=%d)\n", result, IDOK);
    if (result != IDOK) return 1;

    // Test DirectX stubs
    LPDIRECTDRAW lpDD = nullptr;
    HRESULT hr = DirectDrawCreate(nullptr, &lpDD, nullptr);
    printf("DirectDrawCreate returned 0x%08X (expected failure)\n", (unsigned)hr);
    if (hr == DD_OK) return 1;

    // Test memory stubs
    HGLOBAL hMem = GlobalAlloc(GPTR, 1024);
    void* ptr = GlobalLock(hMem);
    GlobalUnlock(hMem);
    GlobalFree(hMem);
    printf("GlobalAlloc/Lock/Unlock/Free: OK\n");

    // Test string functions
    char buf[64];
    lstrcpy(buf, "Hello");
    printf("lstrlen(\"Hello\") = %d\n", lstrlen(buf));
    if (lstrlen(buf) != 5) return 1;

    printf("\nAll compatibility tests passed!\n");
    return 0;
}
EOF

if ! clang++ -std=c++17 -DCOMPAT_HEADERS_ONLY -I include /tmp/test_compat.cpp -o /tmp/test_compat 2>&1; then
    echo "VERIFY_FAILED: Test program compilation failed"
    rm -f /tmp/test_compat.cpp
    exit 1
fi
echo "  OK: Test program compiled"

# Step 4: Run test
echo "[4/5] Running test program..."
if ! /tmp/test_compat; then
    echo "VERIFY_FAILED: Test program failed"
    rm -f /tmp/test_compat.cpp /tmp/test_compat
    exit 1
fi

# Step 5: Check it compiles with platform layer (if available)
echo "[5/5] Checking integration with platform.h..."
if [ -f "include/platform.h" ]; then
    cat > /tmp/test_compat_full.cpp << 'EOF'
#include "compat/compat.h"
int main() {
    DWORD t = GetTickCount();
    return t > 0 ? 0 : 0;
}
EOF
    # Just syntax check - don't link
    if ! clang++ -std=c++17 -fsyntax-only -I include /tmp/test_compat_full.cpp 2>&1; then
        echo "VERIFY_WARNING: Integration with platform.h has issues"
        # Don't fail - platform layer may not be fully compatible yet
    else
        echo "  OK: Integrates with platform.h"
    fi
    rm -f /tmp/test_compat_full.cpp
else
    echo "  SKIP: platform.h not found"
fi

# Cleanup
rm -f /tmp/test_compat.cpp /tmp/test_compat

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Ensure Tasks 13a-13d are complete
2. Create `include/compat/compat.h` with the content above
3. Make verification script executable
4. Run verification script
5. Fix any issues and repeat

## Success Criteria
- Master header compiles without errors
- All sub-headers included without conflicts
- Function bridges work (GetTickCount, MessageBox, etc.)
- COMPAT_HEADERS_ONLY mode works for testing without platform layer
- No duplicate symbol errors

## Common Issues

### "Redefinition of function"
- Some functions may be defined in multiple headers
- Use static inline to avoid multiple definition errors
- Check for duplicate includes

### "Unknown type 'KeyCode'"
- This type comes from platform.h
- In COMPAT_HEADERS_ONLY mode, we skip the key mapping
- Full integration needs platform layer

### "Platform_* not found"
- Ensure platform.h is included when not in headers-only mode
- Build with the platform library linked

## Completion Promise
When verification passes, output:
```
<promise>TASK_13E_COMPLETE</promise>
```

## Escape Hatch
If stuck after 15 iterations:
- Document what's blocking in `ralph-tasks/blocked/13e.md`
- Output: `<promise>TASK_13E_BLOCKED</promise>`

## Max Iterations
15
