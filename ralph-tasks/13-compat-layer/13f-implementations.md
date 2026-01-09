# Task 13f: Function Implementations

## Dependencies
- Task 13e (Master Header) must be complete
- Platform layer must be functional

## Context
While most compatibility functions can be inlined in headers, some may require actual implementations in a .cpp file. This task creates the implementation file and ensures complex functions work correctly when bridging to the platform layer.

## Objective
Create `src/compat/compat.cpp` that:
1. Implements any functions too complex for inline
2. Provides native macOS implementations where beneficial (e.g., MessageBox as native dialog)
3. Maintains state if needed (e.g., error codes)
4. Exports symbols for linking

## Technical Background

### Why Separate Implementation?
Some scenarios require a .cpp file:
- State that persists across calls
- Complex logic that bloats headers
- Future native platform integration
- Debug/logging hooks

### Thread-Local Error State
Windows has thread-local error codes via GetLastError()/SetLastError(). We can provide a simple approximation.

### Native Dialogs (Future)
MessageBox could use native macOS dialogs (NSAlert) via Objective-C++. For now, we just log.

## Deliverables
- [ ] `src/compat/compat.cpp` - Implementation file
- [ ] Thread-local error state working
- [ ] Compiles and links with platform layer
- [ ] Verification script passes

## Files to Create

### src/compat/compat.cpp (NEW)
```cpp
/**
 * Compatibility Layer Implementations
 *
 * Provides implementations for functions that can't be inlined in headers.
 * Links against the platform layer for actual functionality.
 */

#include "compat/compat.h"
#include <cstdio>
#include <cstring>

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
    Platform_LogInfo("%s %s: %s", prefix,
                     lpCaption ? lpCaption : "Message",
                     lpText ? lpText : "");
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
    Platform_LogDebug("[COMPAT] %s", buffer);
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
```

## Verification Command
```bash
ralph-tasks/verify/check-implementations.sh
```

## Verification Script

### ralph-tasks/verify/check-implementations.sh (NEW)
```bash
#!/bin/bash
# Verification script for compat implementations

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Compatibility Implementations ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check files exist
echo "[1/4] Checking source file exists..."
if [ ! -f "src/compat/compat.cpp" ]; then
    echo "VERIFY_FAILED: src/compat/compat.cpp not found"
    exit 1
fi
echo "  OK: Source file exists"

# Step 2: Syntax check (with headers only mode)
echo "[2/4] Checking syntax..."
if ! clang++ -std=c++17 -fsyntax-only -DCOMPAT_HEADERS_ONLY -I include src/compat/compat.cpp 2>&1; then
    echo "VERIFY_FAILED: Implementation has syntax errors"
    exit 1
fi
echo "  OK: Syntax valid"

# Step 3: Compile test program
echo "[3/4] Compiling test program..."
mkdir -p /tmp/compat_test

cat > /tmp/compat_test/test.cpp << 'EOF'
#define COMPAT_HEADERS_ONLY
#include "compat/compat.h"
#include <cstdio>

// Forward declare the functions we're testing
extern "C" {
    void Compat_NormalizePath(char* path);
    const char* Compat_GetExtension(const char* path);
    const char* Compat_GetFilename(const char* path);
    BOOL Compat_IntersectRect(LPRECT lprcDst, const RECT* lprcSrc1, const RECT* lprcSrc2);
    int Compat_Init(void);
    void Compat_Shutdown(void);
}

int main() {
    printf("Testing compatibility implementations...\n\n");

    // Test init
    if (Compat_Init() != 0) {
        printf("FAIL: Compat_Init failed\n");
        return 1;
    }
    printf("Compat_Init: OK\n");

    // Test path normalization
    char path1[] = "DATA\\CONQUER\\MAPS\\SCG01EA.INI";
    Compat_NormalizePath(path1);
    printf("NormalizePath: %s\n", path1);
    if (strchr(path1, '\\') != nullptr) {
        printf("FAIL: Backslashes not converted\n");
        return 1;
    }

    // Test extension
    const char* ext = Compat_GetExtension("FILE.MIX");
    printf("GetExtension(FILE.MIX): %s\n", ext);
    if (strcmp(ext, "MIX") != 0) {
        printf("FAIL: Expected 'MIX', got '%s'\n", ext);
        return 1;
    }

    // Test filename
    const char* fname = Compat_GetFilename("/path/to/file.txt");
    printf("GetFilename(/path/to/file.txt): %s\n", fname);
    if (strcmp(fname, "file.txt") != 0) {
        printf("FAIL: Expected 'file.txt', got '%s'\n", fname);
        return 1;
    }

    // Test rect intersection
    RECT r1 = {0, 0, 100, 100};
    RECT r2 = {50, 50, 150, 150};
    RECT result;
    BOOL intersects = Compat_IntersectRect(&result, &r1, &r2);
    printf("IntersectRect: intersects=%d, result=(%ld,%ld,%ld,%ld)\n",
           intersects, result.left, result.top, result.right, result.bottom);
    if (!intersects || result.left != 50 || result.top != 50 ||
        result.right != 100 || result.bottom != 100) {
        printf("FAIL: Intersection incorrect\n");
        return 1;
    }

    Compat_Shutdown();
    printf("\nAll implementation tests passed!\n");
    return 0;
}
EOF

# Compile implementation and test
if ! clang++ -std=c++17 -DCOMPAT_HEADERS_ONLY -I include \
    src/compat/compat.cpp /tmp/compat_test/test.cpp -o /tmp/compat_test/test 2>&1; then
    echo "VERIFY_FAILED: Test program compilation failed"
    rm -rf /tmp/compat_test
    exit 1
fi
echo "  OK: Test program compiled"

# Step 4: Run test
echo "[4/4] Running test program..."
if ! /tmp/compat_test/test; then
    echo "VERIFY_FAILED: Test program failed"
    rm -rf /tmp/compat_test
    exit 1
fi

# Cleanup
rm -rf /tmp/compat_test

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Create directory: `mkdir -p src/compat`
2. Create `src/compat/compat.cpp` with the content above
3. Make verification script executable
4. Run verification script
5. Fix any issues and repeat

## Success Criteria
- Implementation file compiles without errors
- Compat_Init/Shutdown work correctly
- Path utilities work correctly
- Rectangle utilities work correctly
- No duplicate symbol errors with headers

## Common Issues

### "Multiple definition of..."
- Ensure functions are either inline in headers OR in .cpp, not both
- Use Compat_ prefix for .cpp implementations

### "Platform_* not declared"
- Happens in COMPAT_HEADERS_ONLY mode
- Use #ifndef COMPAT_HEADERS_ONLY guards

### Thread-local not supported
- Use `static` instead of `thread_local` if needed
- Or check compiler support

## Completion Promise
When verification passes, output:
```
<promise>TASK_13F_COMPLETE</promise>
```

## Escape Hatch
If stuck after 12 iterations:
- Document what's blocking in `ralph-tasks/blocked/13f.md`
- Output: `<promise>TASK_13F_BLOCKED</promise>`

## Max Iterations
12
