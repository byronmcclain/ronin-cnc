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
static inline BOOL QueryPerformanceCounter(LONGLONG* lpCounter) {
    if (lpCounter) *lpCounter = 0;
    return TRUE;
}
static inline BOOL QueryPerformanceFrequency(LONGLONG* lpFrequency) {
    if (lpFrequency) *lpFrequency = 1000000;
    return TRUE;
}
#endif // COMPAT_HEADERS_ONLY

// -----------------------------------------------------------------------------
// Message Box
// -----------------------------------------------------------------------------

/// MessageBoxA - Show a message box
/// Logs the message and returns IDOK
/// In the future, could use native macOS dialogs
static inline int MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) {
    (void)hWnd;
    (void)lpCaption;
    (void)lpText;

#ifndef COMPAT_HEADERS_ONLY
    // Log the message (platform only takes single string)
    if (lpText) {
        Platform_LogInfo(lpText);
    }
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
    // Map VK codes to platform key codes
    // Platform uses same values as Windows VK codes for special keys
    KeyCode platformKey;
    switch (vKey) {
        case VK_ESCAPE:  platformKey = KEY_CODE_ESCAPE; break;
        case VK_RETURN:  platformKey = KEY_CODE_RETURN; break;
        case VK_SPACE:   platformKey = KEY_CODE_SPACE; break;
        case VK_LEFT:    platformKey = KEY_CODE_LEFT; break;
        case VK_UP:      platformKey = KEY_CODE_UP; break;
        case VK_RIGHT:   platformKey = KEY_CODE_RIGHT; break;
        case VK_DOWN:    platformKey = KEY_CODE_DOWN; break;
        case VK_SHIFT:   platformKey = KEY_CODE_SHIFT; break;
        case VK_CONTROL: platformKey = KEY_CODE_CONTROL; break;
        case VK_MENU:    platformKey = KEY_CODE_ALT; break;
        case VK_TAB:     platformKey = KEY_CODE_TAB; break;
        case VK_BACK:    platformKey = KEY_CODE_BACKSPACE; break;
        case VK_PAUSE:   platformKey = KEY_CODE_PAUSE; break;
        case VK_CAPITAL: platformKey = KEY_CODE_CAPS_LOCK; break;
        case VK_PRIOR:   platformKey = KEY_CODE_PAGE_UP; break;
        case VK_NEXT:    platformKey = KEY_CODE_PAGE_DOWN; break;
        case VK_END:     platformKey = KEY_CODE_END; break;
        case VK_HOME:    platformKey = KEY_CODE_HOME; break;
        case VK_INSERT:  platformKey = KEY_CODE_INSERT; break;
        case VK_DELETE:  platformKey = KEY_CODE_DELETE; break;
        case VK_NUMLOCK: platformKey = KEY_CODE_NUM_LOCK; break;
        case VK_SCROLL:  platformKey = KEY_CODE_SCROLL_LOCK; break;
        default:
            // F1-F12 keys
            if (vKey >= VK_F1 && vKey <= VK_F12) {
                platformKey = (KeyCode)(KEY_CODE_F1 + (vKey - VK_F1));
            } else {
                // For alphanumeric and other keys, platform doesn't have individual codes
                // Just return 0 (not pressed) - game can use keyboard buffer instead
                return 0;
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
