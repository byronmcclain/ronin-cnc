/**
 * Compatibility Layer Implementations
 *
 * Provides implementations for functions that can't be inlined in headers.
 * Links against the platform layer for actual functionality.
 */

#include "compat/compat.h"
#include <cstdio>
#include <cstring>
#include <cstdarg>

// =============================================================================
// Thread-Local Error State
// =============================================================================

// Simple thread-local error code (Windows-style GetLastError/SetLastError)
static thread_local DWORD g_lastError = 0;

extern "C" {

DWORD Compat_GetLastError(void) {
    return g_lastError;
}

void Compat_SetLastError(DWORD dwErrCode) {
    g_lastError = dwErrCode;
}

} // extern "C"

// Override the inline stubs if this file is linked
// Note: These need to be declared as weak or in a specific way to avoid
// duplicate symbol errors with the inline versions

// =============================================================================
// Message Box Implementation
// =============================================================================

extern "C" {

/// Enhanced MessageBox that could use native dialogs
/// For now, logs and returns appropriate value
int Compat_MessageBox(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) {
    (void)hWnd;

    // Extract button type
    UINT buttonType = uType & 0x0F;
    // Extract icon type
    UINT iconType = uType & 0xF0;

    // Log based on icon type
    const char* prefix = "[INFO]";
    if (iconType == MB_ICONERROR || iconType == MB_ICONHAND) {
        prefix = "[ERROR]";
    } else if (iconType == MB_ICONWARNING || iconType == MB_ICONEXCLAMATION) {
        prefix = "[WARN]";
    }

#ifndef COMPAT_HEADERS_ONLY
    // Log via platform layer (simple single-string version)
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "%s %s: %s", prefix,
             lpCaption ? lpCaption : "Message",
             lpText ? lpText : "");
    Platform_LogInfo(buffer);
#else
    printf("%s %s: %s\n", prefix,
           lpCaption ? lpCaption : "Message",
           lpText ? lpText : "");
#endif

    // Return appropriate value based on button type
    switch (buttonType) {
        case MB_YESNO:
        case MB_YESNOCANCEL:
            return IDYES; // Default to Yes
        case MB_OKCANCEL:
            return IDOK;
        case MB_RETRYCANCEL:
            return IDRETRY;
        case MB_ABORTRETRYIGNORE:
            return IDIGNORE;
        default:
            return IDOK;
    }
}

} // extern "C"

// =============================================================================
// Path Utilities
// =============================================================================

extern "C" {

/// Convert backslashes to forward slashes for cross-platform paths
void Compat_NormalizePath(char* path) {
    if (!path) return;
    for (char* p = path; *p; p++) {
        if (*p == '\\') *p = '/';
    }
}

/// Get file extension (returns pointer within path)
const char* Compat_GetExtension(const char* path) {
    if (!path) return "";
    const char* dot = strrchr(path, '.');
    const char* slash = strrchr(path, '/');
    const char* bslash = strrchr(path, '\\');

    // Extension must come after last path separator
    if (dot) {
        if (slash && dot < slash) return "";
        if (bslash && dot < bslash) return "";
        return dot + 1;
    }
    return "";
}

/// Extract filename from path
const char* Compat_GetFilename(const char* path) {
    if (!path) return "";
    const char* slash = strrchr(path, '/');
    const char* bslash = strrchr(path, '\\');

    const char* last = slash > bslash ? slash : bslash;
    return last ? last + 1 : path;
}

} // extern "C"

// =============================================================================
// Debug Helpers
// =============================================================================

#ifdef COMPAT_DEBUG

extern "C" {

void Compat_DebugPrint(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

#ifndef COMPAT_HEADERS_ONLY
    char logbuf[1100];
    snprintf(logbuf, sizeof(logbuf), "[COMPAT] %s", buffer);
    Platform_LogDebug(logbuf);
#else
    fprintf(stderr, "[COMPAT] %s\n", buffer);
#endif

    va_end(args);
}

void Compat_DumpMemory(const void* ptr, size_t size) {
    const uint8_t* bytes = (const uint8_t*)ptr;
    char line[80];
    char ascii[17];

    for (size_t i = 0; i < size; i += 16) {
        int pos = snprintf(line, sizeof(line), "%08zX: ", i);

        for (size_t j = 0; j < 16; j++) {
            if (i + j < size) {
                pos += snprintf(line + pos, sizeof(line) - pos, "%02X ", bytes[i + j]);
                ascii[j] = (bytes[i + j] >= 32 && bytes[i + j] < 127) ? bytes[i + j] : '.';
            } else {
                pos += snprintf(line + pos, sizeof(line) - pos, "   ");
                ascii[j] = ' ';
            }
        }
        ascii[16] = '\0';

        Compat_DebugPrint("%s |%s|", line, ascii);
    }
}

} // extern "C"

#endif // COMPAT_DEBUG

// =============================================================================
// Initialization
// =============================================================================

extern "C" {

/// Initialize compatibility layer
/// Called before main game code runs
int Compat_Init(void) {
    g_lastError = 0;

#ifdef COMPAT_DEBUG
    Compat_DebugPrint("Compatibility layer initialized (version %s)", COMPAT_VERSION_STRING);
#endif

    return 0;
}

/// Shutdown compatibility layer
void Compat_Shutdown(void) {
#ifdef COMPAT_DEBUG
    Compat_DebugPrint("Compatibility layer shutdown");
#endif
}

} // extern "C"

// =============================================================================
// Color Conversion Utilities
// =============================================================================

extern "C" {

/// Convert COLORREF (BGR) to RGB bytes
void Compat_ColorRefToRGB(DWORD colorref, BYTE* r, BYTE* g, BYTE* b) {
    if (r) *r = GetRValue(colorref);
    if (g) *g = GetGValue(colorref);
    if (b) *b = GetBValue(colorref);
}

/// Convert RGB bytes to COLORREF (BGR)
DWORD Compat_RGBToColorRef(BYTE r, BYTE g, BYTE b) {
    return RGB(r, g, b);
}

/// Convert palette entry to COLORREF
DWORD Compat_PaletteEntryToColorRef(const PALETTEENTRY* pe) {
    if (!pe) return 0;
    return RGB(pe->peRed, pe->peGreen, pe->peBlue);
}

/// Convert COLORREF to palette entry
void Compat_ColorRefToPaletteEntry(DWORD colorref, PALETTEENTRY* pe) {
    if (!pe) return;
    pe->peRed = GetRValue(colorref);
    pe->peGreen = GetGValue(colorref);
    pe->peBlue = GetBValue(colorref);
    pe->peFlags = 0;
}

} // extern "C"

// =============================================================================
// Rectangle Utilities
// =============================================================================

extern "C" {

/// Check if rectangles intersect
BOOL Compat_IntersectRect(LPRECT lprcDst, const RECT* lprcSrc1, const RECT* lprcSrc2) {
    if (!lprcDst || !lprcSrc1 || !lprcSrc2) return FALSE;

    lprcDst->left = max(lprcSrc1->left, lprcSrc2->left);
    lprcDst->top = max(lprcSrc1->top, lprcSrc2->top);
    lprcDst->right = min(lprcSrc1->right, lprcSrc2->right);
    lprcDst->bottom = min(lprcSrc1->bottom, lprcSrc2->bottom);

    if (lprcDst->left >= lprcDst->right || lprcDst->top >= lprcDst->bottom) {
        SetRectEmpty(lprcDst);
        return FALSE;
    }
    return TRUE;
}

/// Union two rectangles
BOOL Compat_UnionRect(LPRECT lprcDst, const RECT* lprcSrc1, const RECT* lprcSrc2) {
    if (!lprcDst || !lprcSrc1 || !lprcSrc2) return FALSE;

    lprcDst->left = min(lprcSrc1->left, lprcSrc2->left);
    lprcDst->top = min(lprcSrc1->top, lprcSrc2->top);
    lprcDst->right = max(lprcSrc1->right, lprcSrc2->right);
    lprcDst->bottom = max(lprcSrc1->bottom, lprcSrc2->bottom);

    return !IsRectEmpty(lprcDst);
}

/// Subtract rectangle (result can have up to 4 rectangles, we return just the bounding)
BOOL Compat_SubtractRect(LPRECT lprcDst, const RECT* lprcSrc1, const RECT* lprcSrc2) {
    if (!lprcDst || !lprcSrc1 || !lprcSrc2) return FALSE;

    // Simple case: if no intersection, result is src1
    RECT intersection;
    if (!Compat_IntersectRect(&intersection, lprcSrc1, lprcSrc2)) {
        *lprcDst = *lprcSrc1;
        return TRUE;
    }

    // If src2 completely covers src1, result is empty
    if (lprcSrc2->left <= lprcSrc1->left && lprcSrc2->top <= lprcSrc1->top &&
        lprcSrc2->right >= lprcSrc1->right && lprcSrc2->bottom >= lprcSrc1->bottom) {
        SetRectEmpty(lprcDst);
        return FALSE;
    }

    // Otherwise, return src1 (simplified - real subtraction is complex)
    *lprcDst = *lprcSrc1;
    return TRUE;
}

/// Check if rectangles are equal
BOOL Compat_EqualRect(const RECT* lprc1, const RECT* lprc2) {
    if (!lprc1 || !lprc2) return FALSE;
    return lprc1->left == lprc2->left &&
           lprc1->top == lprc2->top &&
           lprc1->right == lprc2->right &&
           lprc1->bottom == lprc2->bottom;
}

} // extern "C"
