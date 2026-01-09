# Task 13a: Windows Types Header

## Dependencies
- Phase 12 Asset System must be complete
- Platform layer must be fully functional

## Context
The original Red Alert code was written for Windows 95 using Watcom C++ 10.6. It relies extensively on Windows API types (DWORD, HWND, BOOL, etc.) and structures (POINT, RECT, PALETTEENTRY, etc.). Before we can compile any original CODE/*.CPP files, we must provide compatible type definitions that work with modern Clang on macOS.

This is the foundational task for the compatibility layer. All other compat headers will depend on these type definitions.

## Objective
Create `include/compat/windows.h` that defines:
1. All fundamental Windows integer types with exact sizes
2. Handle types as opaque pointers
3. Common Windows structures with correct binary layout
4. Essential Windows constants and macros
5. Function stubs for commonly-used Windows functions

## Technical Background

### Critical Type Sizes
The original code was 32-bit. We must match these exact sizes:

| Windows Type | Size | C++ Equivalent |
|--------------|------|----------------|
| BOOL | 4 bytes | int32_t |
| BYTE | 1 byte | uint8_t |
| WORD | 2 bytes | uint16_t |
| DWORD | 4 bytes | uint32_t |
| LONG | 4 bytes | int32_t |
| SHORT | 2 bytes | int16_t |
| UINT | 4 bytes | uint32_t |
| INT | 4 bytes | int32_t |

### Handle Types
Windows handles are opaque pointers. We define them as `void*` for source compatibility:
- HWND, HINSTANCE, HDC, HPALETTE, HBITMAP, etc.

### Structures
These MUST have correct binary layout for any serialization/deserialization:
- POINT (8 bytes): x, y as LONG
- RECT (16 bytes): left, top, right, bottom as LONG
- PALETTEENTRY (4 bytes): peRed, peGreen, peBlue, peFlags as BYTE
- RGBQUAD (4 bytes): rgbBlue, rgbGreen, rgbRed, rgbReserved as BYTE
- BITMAPINFOHEADER (40 bytes): Complex structure for DIB format

## Deliverables
- [ ] `include/compat/windows.h` - Windows type definitions
- [ ] Static assertions verify all type sizes
- [ ] No conflicts with platform.h
- [ ] Verification script passes

## Files to Create

### include/compat/windows.h (NEW)
```cpp
/**
 * Windows API Compatibility Layer
 *
 * Provides Windows type definitions for compiling original Red Alert code
 * on macOS with Clang. This replaces the real Windows headers.
 *
 * IMPORTANT: This file must be included INSTEAD of <windows.h>
 * The original CODE files will have their includes replaced.
 */

#ifndef COMPAT_WINDOWS_H
#define COMPAT_WINDOWS_H

#include <cstdint>
#include <cstddef>

// =============================================================================
// Prevent Real Windows Headers
// =============================================================================
// Define guards that would normally be set by including real Windows headers.
// This prevents any accidental inclusion of system Windows headers.

#define _WINDOWS_
#define _WINDOWS_H
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WINDEF_
#define _WINBASE_
#define _WINGDI_
#define _WINUSER_

// =============================================================================
// Fundamental Integer Types
// =============================================================================
// These must match exact sizes from Win32 API (32-bit model)

typedef int32_t  BOOL;      // 4 bytes, not bool (1 byte)!
typedef uint8_t  BYTE;      // 1 byte
typedef uint16_t WORD;      // 2 bytes
typedef uint32_t DWORD;     // 4 bytes (Double WORD)
typedef int32_t  LONG;      // 4 bytes (signed)
typedef uint32_t ULONG;     // 4 bytes (unsigned)
typedef int16_t  SHORT;     // 2 bytes
typedef uint16_t USHORT;    // 2 bytes
typedef uint32_t UINT;      // 4 bytes
typedef int32_t  INT;       // 4 bytes
typedef int64_t  LONGLONG;  // 8 bytes
typedef uint64_t ULONGLONG; // 8 bytes
typedef float    FLOAT;     // 4 bytes
typedef char     CHAR;      // 1 byte
typedef unsigned char UCHAR;// 1 byte
typedef wchar_t  WCHAR;     // Wide character

// Size types
typedef size_t   SIZE_T;
typedef intptr_t INT_PTR;
typedef uintptr_t UINT_PTR;
typedef intptr_t LONG_PTR;
typedef uintptr_t ULONG_PTR;
typedef ULONG_PTR DWORD_PTR;

// =============================================================================
// Pointer Types
// =============================================================================

typedef char*        LPSTR;
typedef const char*  LPCSTR;
typedef char*        PSTR;
typedef const char*  PCSTR;

typedef wchar_t*       LPWSTR;
typedef const wchar_t* LPCWSTR;

typedef void*        LPVOID;
typedef const void*  LPCVOID;
typedef void*        PVOID;

typedef DWORD*       LPDWORD;
typedef DWORD*       PDWORD;
typedef WORD*        LPWORD;
typedef WORD*        PWORD;
typedef BYTE*        LPBYTE;
typedef BYTE*        PBYTE;
typedef LONG*        LPLONG;
typedef LONG*        PLONG;
typedef BOOL*        LPBOOL;
typedef BOOL*        PBOOL;
typedef INT*         LPINT;
typedef INT*         PINT;

// =============================================================================
// Boolean Constants
// =============================================================================

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

// Note: NULL is typically defined by cstddef, but ensure it exists
#ifndef NULL
#define NULL nullptr
#endif

// =============================================================================
// Handle Types
// =============================================================================
// Handles are opaque pointers in Windows. We use void* for compatibility.
// The actual handle values are never dereferenced in game code.

typedef void* HANDLE;
typedef void* HWND;
typedef void* HINSTANCE;
typedef void* HMODULE;
typedef void* HDC;
typedef void* HPALETTE;
typedef void* HBITMAP;
typedef void* HBRUSH;
typedef void* HPEN;
typedef void* HFONT;
typedef void* HICON;
typedef void* HCURSOR;
typedef void* HRGN;
typedef void* HMENU;
typedef void* HACCEL;
typedef void* HGLOBAL;
typedef void* HLOCAL;
typedef void* HRSRC;
typedef void* HGDIOBJ;

// Invalid handle value
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)

// =============================================================================
// Callback Types
// =============================================================================

typedef LONG_PTR LRESULT;
typedef UINT_PTR WPARAM;
typedef LONG_PTR LPARAM;

// Window procedure callback
typedef LRESULT (*WNDPROC)(HWND, UINT, WPARAM, LPARAM);

// Timer callback
typedef void (*TIMERPROC)(HWND, UINT, UINT_PTR, DWORD);

// =============================================================================
// Common Structures
// =============================================================================
// These structures must match Windows binary layout exactly.
// Use #pragma pack to ensure no padding.

#pragma pack(push, 1)

/// Point structure (8 bytes)
typedef struct tagPOINT {
    LONG x;
    LONG y;
} POINT, *PPOINT, *LPPOINT;

/// Rectangle structure (16 bytes)
typedef struct tagRECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
} RECT, *PRECT, *LPRECT;

typedef const RECT* LPCRECT;

/// Size structure (8 bytes)
typedef struct tagSIZE {
    LONG cx;  // width
    LONG cy;  // height
} SIZE, *PSIZE, *LPSIZE;

/// Palette entry (4 bytes)
typedef struct tagPALETTEENTRY {
    BYTE peRed;
    BYTE peGreen;
    BYTE peBlue;
    BYTE peFlags;
} PALETTEENTRY, *PPALETTEENTRY, *LPPALETTEENTRY;

/// RGB quad for DIBs (4 bytes) - Note: BGR order!
typedef struct tagRGBQUAD {
    BYTE rgbBlue;
    BYTE rgbGreen;
    BYTE rgbRed;
    BYTE rgbReserved;
} RGBQUAD, *LPRGBQUAD;

/// Bitmap info header (40 bytes)
typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;           // Size of this structure (40)
    LONG  biWidth;          // Width in pixels
    LONG  biHeight;         // Height in pixels (negative = top-down)
    WORD  biPlanes;         // Must be 1
    WORD  biBitCount;       // Bits per pixel (1, 4, 8, 16, 24, 32)
    DWORD biCompression;    // Compression type (BI_RGB = 0)
    DWORD biSizeImage;      // Size of image data (can be 0 for BI_RGB)
    LONG  biXPelsPerMeter;  // Horizontal resolution
    LONG  biYPelsPerMeter;  // Vertical resolution
    DWORD biClrUsed;        // Number of colors used (0 = all)
    DWORD biClrImportant;   // Number of important colors
} BITMAPINFOHEADER, *PBITMAPINFOHEADER, *LPBITMAPINFOHEADER;

/// Bitmap info (header + color table)
typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD          bmiColors[1];  // Variable length color table
} BITMAPINFO, *PBITMAPINFO, *LPBITMAPINFO;

/// Bitmap file header (14 bytes)
typedef struct tagBITMAPFILEHEADER {
    WORD  bfType;       // Must be 'BM' (0x4D42)
    DWORD bfSize;       // File size
    WORD  bfReserved1;  // Reserved (0)
    WORD  bfReserved2;  // Reserved (0)
    DWORD bfOffBits;    // Offset to pixel data
} BITMAPFILEHEADER, *PBITMAPFILEHEADER, *LPBITMAPFILEHEADER;

/// Logical palette
typedef struct tagLOGPALETTE {
    WORD         palVersion;    // Must be 0x0300
    WORD         palNumEntries; // Number of entries
    PALETTEENTRY palPalEntry[1];// Variable length array
} LOGPALETTE, *PLOGPALETTE, *LPLOGPALETTE;

/// Message structure (for message loop)
typedef struct tagMSG {
    HWND   hwnd;
    UINT   message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD  time;
    POINT  pt;
} MSG, *PMSG, *LPMSG;

#pragma pack(pop)

// =============================================================================
// Static Size Assertions
// =============================================================================
// Verify our type definitions match expected sizes

static_assert(sizeof(BOOL) == 4, "BOOL must be 4 bytes");
static_assert(sizeof(BYTE) == 1, "BYTE must be 1 byte");
static_assert(sizeof(WORD) == 2, "WORD must be 2 bytes");
static_assert(sizeof(DWORD) == 4, "DWORD must be 4 bytes");
static_assert(sizeof(LONG) == 4, "LONG must be 4 bytes");
static_assert(sizeof(SHORT) == 2, "SHORT must be 2 bytes");

static_assert(sizeof(POINT) == 8, "POINT must be 8 bytes");
static_assert(sizeof(RECT) == 16, "RECT must be 16 bytes");
static_assert(sizeof(SIZE) == 8, "SIZE must be 8 bytes");
static_assert(sizeof(PALETTEENTRY) == 4, "PALETTEENTRY must be 4 bytes");
static_assert(sizeof(RGBQUAD) == 4, "RGBQUAD must be 4 bytes");
static_assert(sizeof(BITMAPINFOHEADER) == 40, "BITMAPINFOHEADER must be 40 bytes");
static_assert(sizeof(BITMAPFILEHEADER) == 14, "BITMAPFILEHEADER must be 14 bytes");

// =============================================================================
// GDI Constants
// =============================================================================

#define BI_RGB       0  // Uncompressed
#define BI_RLE8      1  // 8-bit RLE
#define BI_RLE4      2  // 4-bit RLE
#define BI_BITFIELDS 3  // Bitfield masks

#define DIB_RGB_COLORS 0  // Color table contains RGB values
#define DIB_PAL_COLORS 1  // Color table contains palette indices

// Palette entry flags
#define PC_RESERVED   0x01
#define PC_EXPLICIT   0x02
#define PC_NOCOLLAPSE 0x04

// =============================================================================
// Message Box Constants
// =============================================================================

// Type flags
#define MB_OK               0x00000000
#define MB_OKCANCEL         0x00000001
#define MB_ABORTRETRYIGNORE 0x00000002
#define MB_YESNOCANCEL      0x00000003
#define MB_YESNO            0x00000004
#define MB_RETRYCANCEL      0x00000005

// Icon flags
#define MB_ICONERROR        0x00000010
#define MB_ICONQUESTION     0x00000020
#define MB_ICONWARNING      0x00000030
#define MB_ICONINFORMATION  0x00000040
#define MB_ICONHAND         MB_ICONERROR
#define MB_ICONSTOP         MB_ICONERROR
#define MB_ICONEXCLAMATION  MB_ICONWARNING
#define MB_ICONASTERISK     MB_ICONINFORMATION

// Default button flags
#define MB_DEFBUTTON1       0x00000000
#define MB_DEFBUTTON2       0x00000100
#define MB_DEFBUTTON3       0x00000200
#define MB_DEFBUTTON4       0x00000300

// Return values
#define IDOK       1
#define IDCANCEL   2
#define IDABORT    3
#define IDRETRY    4
#define IDIGNORE   5
#define IDYES      6
#define IDNO       7

// =============================================================================
// Virtual Key Codes
// =============================================================================
// Subset commonly used in game code

#define VK_LBUTTON    0x01
#define VK_RBUTTON    0x02
#define VK_CANCEL     0x03
#define VK_MBUTTON    0x04

#define VK_BACK       0x08
#define VK_TAB        0x09
#define VK_CLEAR      0x0C
#define VK_RETURN     0x0D
#define VK_SHIFT      0x10
#define VK_CONTROL    0x11
#define VK_MENU       0x12  // Alt key
#define VK_PAUSE      0x13
#define VK_CAPITAL    0x14  // Caps Lock
#define VK_ESCAPE     0x1B
#define VK_SPACE      0x20
#define VK_PRIOR      0x21  // Page Up
#define VK_NEXT       0x22  // Page Down
#define VK_END        0x23
#define VK_HOME       0x24
#define VK_LEFT       0x25
#define VK_UP         0x26
#define VK_RIGHT      0x27
#define VK_DOWN       0x28
#define VK_SELECT     0x29
#define VK_PRINT      0x2A
#define VK_EXECUTE    0x2B
#define VK_SNAPSHOT   0x2C  // Print Screen
#define VK_INSERT     0x2D
#define VK_DELETE     0x2E
#define VK_HELP       0x2F

// 0-9 keys (0x30 - 0x39)
// A-Z keys (0x41 - 0x5A)

#define VK_LWIN       0x5B
#define VK_RWIN       0x5C
#define VK_APPS       0x5D

#define VK_NUMPAD0    0x60
#define VK_NUMPAD1    0x61
#define VK_NUMPAD2    0x62
#define VK_NUMPAD3    0x63
#define VK_NUMPAD4    0x64
#define VK_NUMPAD5    0x65
#define VK_NUMPAD6    0x66
#define VK_NUMPAD7    0x67
#define VK_NUMPAD8    0x68
#define VK_NUMPAD9    0x69
#define VK_MULTIPLY   0x6A
#define VK_ADD        0x6B
#define VK_SEPARATOR  0x6C
#define VK_SUBTRACT   0x6D
#define VK_DECIMAL    0x6E
#define VK_DIVIDE     0x6F

#define VK_F1         0x70
#define VK_F2         0x71
#define VK_F3         0x72
#define VK_F4         0x73
#define VK_F5         0x74
#define VK_F6         0x75
#define VK_F7         0x76
#define VK_F8         0x77
#define VK_F9         0x78
#define VK_F10        0x79
#define VK_F11        0x7A
#define VK_F12        0x7B

#define VK_NUMLOCK    0x90
#define VK_SCROLL     0x91

#define VK_LSHIFT     0xA0
#define VK_RSHIFT     0xA1
#define VK_LCONTROL   0xA2
#define VK_RCONTROL   0xA3
#define VK_LMENU      0xA4
#define VK_RMENU      0xA5

// =============================================================================
// Window Message Constants
// =============================================================================
// Subset used in game code

#define WM_NULL             0x0000
#define WM_CREATE           0x0001
#define WM_DESTROY          0x0002
#define WM_MOVE             0x0003
#define WM_SIZE             0x0005
#define WM_ACTIVATE         0x0006
#define WM_SETFOCUS         0x0007
#define WM_KILLFOCUS        0x0008
#define WM_ENABLE           0x000A
#define WM_PAINT            0x000F
#define WM_CLOSE            0x0010
#define WM_QUIT             0x0012
#define WM_ERASEBKGND       0x0014
#define WM_ACTIVATEAPP      0x001C
#define WM_SETCURSOR        0x0020
#define WM_MOUSEACTIVATE    0x0021
#define WM_GETMINMAXINFO    0x0024
#define WM_WINDOWPOSCHANGING 0x0046
#define WM_WINDOWPOSCHANGED  0x0047
#define WM_KEYDOWN          0x0100
#define WM_KEYUP            0x0101
#define WM_CHAR             0x0102
#define WM_SYSKEYDOWN       0x0104
#define WM_SYSKEYUP         0x0105
#define WM_COMMAND          0x0111
#define WM_SYSCOMMAND       0x0112
#define WM_TIMER            0x0113
#define WM_MOUSEMOVE        0x0200
#define WM_LBUTTONDOWN      0x0201
#define WM_LBUTTONUP        0x0202
#define WM_LBUTTONDBLCLK    0x0203
#define WM_RBUTTONDOWN      0x0204
#define WM_RBUTTONUP        0x0205
#define WM_RBUTTONDBLCLK    0x0206
#define WM_MBUTTONDOWN      0x0207
#define WM_MBUTTONUP        0x0208
#define WM_MBUTTONDBLCLK    0x0209
#define WM_MOUSEWHEEL       0x020A

#define WM_USER             0x0400

// =============================================================================
// ShowWindow Constants
// =============================================================================

#define SW_HIDE            0
#define SW_SHOWNORMAL      1
#define SW_NORMAL          1
#define SW_SHOWMINIMIZED   2
#define SW_SHOWMAXIMIZED   3
#define SW_MAXIMIZE        3
#define SW_SHOWNOACTIVATE  4
#define SW_SHOW            5
#define SW_MINIMIZE        6
#define SW_SHOWMINNOACTIVE 7
#define SW_SHOWNA          8
#define SW_RESTORE         9
#define SW_SHOWDEFAULT     10

// =============================================================================
// Memory Flags
// =============================================================================

#define GMEM_FIXED      0x0000
#define GMEM_MOVEABLE   0x0002
#define GMEM_ZEROINIT   0x0040
#define GPTR            (GMEM_FIXED | GMEM_ZEROINIT)

// =============================================================================
// File Attribute Constants
// =============================================================================

#define FILE_ATTRIBUTE_READONLY  0x00000001
#define FILE_ATTRIBUTE_HIDDEN    0x00000002
#define FILE_ATTRIBUTE_SYSTEM    0x00000004
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#define FILE_ATTRIBUTE_ARCHIVE   0x00000020
#define FILE_ATTRIBUTE_NORMAL    0x00000080

#define INVALID_FILE_ATTRIBUTES  ((DWORD)-1)

// =============================================================================
// Utility Macros
// =============================================================================

// Extract low/high parts of DWORD
#define LOWORD(l) ((WORD)((DWORD_PTR)(l) & 0xFFFF))
#define HIWORD(l) ((WORD)(((DWORD_PTR)(l) >> 16) & 0xFFFF))
#define LOBYTE(w) ((BYTE)((DWORD_PTR)(w) & 0xFF))
#define HIBYTE(w) ((BYTE)(((DWORD_PTR)(w) >> 8) & 0xFF))

// Combine two words into a dword
#define MAKELONG(a, b) ((LONG)(((WORD)(((DWORD_PTR)(a)) & 0xFFFF)) | \
                              ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xFFFF))) << 16))
#define MAKEWORD(a, b) ((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xFF)) | \
                              ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xFF))) << 8))

// Extract coordinates from lParam
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

// RGB color macros
#define RGB(r, g, b) ((DWORD)(((BYTE)(r) | ((WORD)((BYTE)(g)) << 8)) | \
                             (((DWORD)(BYTE)(b)) << 16)))
#define GetRValue(rgb) ((BYTE)(rgb))
#define GetGValue(rgb) ((BYTE)(((WORD)(rgb)) >> 8))
#define GetBValue(rgb) ((BYTE)((rgb) >> 16))

// RECT helpers
#define SetRect(prc, l, t, r, b) \
    ((prc)->left = (l), (prc)->top = (t), (prc)->right = (r), (prc)->bottom = (b))
#define SetRectEmpty(prc) SetRect(prc, 0, 0, 0, 0)
#define CopyRect(dst, src) (*(dst) = *(src))
#define InflateRect(prc, dx, dy) \
    ((prc)->left -= (dx), (prc)->top -= (dy), (prc)->right += (dx), (prc)->bottom += (dy))
#define OffsetRect(prc, dx, dy) \
    ((prc)->left += (dx), (prc)->top += (dy), (prc)->right += (dx), (prc)->bottom += (dy))
#define IsRectEmpty(prc) \
    (((prc)->left >= (prc)->right) || ((prc)->top >= (prc)->bottom))
#define PtInRect(prc, pt) \
    (((pt).x >= (prc)->left) && ((pt).x < (prc)->right) && \
     ((pt).y >= (prc)->top) && ((pt).y < (prc)->bottom))

// min/max (unless NOMINMAX defined, which we do define)
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

// =============================================================================
// End of Windows Types Header
// =============================================================================

#endif // COMPAT_WINDOWS_H
```

## Verification Command
```bash
ralph-tasks/verify/check-windows-types.sh
```

## Verification Script

### ralph-tasks/verify/check-windows-types.sh (NEW)
```bash
#!/bin/bash
# Verification script for Windows types header

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Windows Types Header ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check header exists
echo "[1/4] Checking header file exists..."
if [ ! -f "include/compat/windows.h" ]; then
    echo "VERIFY_FAILED: include/compat/windows.h not found"
    exit 1
fi
echo "  OK: Header file exists"

# Step 2: Syntax check
echo "[2/4] Checking syntax..."
if ! clang++ -std=c++17 -fsyntax-only -I include include/compat/windows.h 2>&1; then
    echo "VERIFY_FAILED: Header has syntax errors"
    exit 1
fi
echo "  OK: Syntax valid"

# Step 3: Compile test program
echo "[3/4] Compiling test program..."
cat > /tmp/test_windows_types.cpp << 'EOF'
#include "compat/windows.h"
#include <cstdio>

int main() {
    // Test type sizes
    printf("BOOL:   %zu bytes (expected 4)\n", sizeof(BOOL));
    printf("DWORD:  %zu bytes (expected 4)\n", sizeof(DWORD));
    printf("WORD:   %zu bytes (expected 2)\n", sizeof(WORD));
    printf("BYTE:   %zu bytes (expected 1)\n", sizeof(BYTE));
    printf("POINT:  %zu bytes (expected 8)\n", sizeof(POINT));
    printf("RECT:   %zu bytes (expected 16)\n", sizeof(RECT));

    // Test constants
    if (TRUE != 1) return 1;
    if (FALSE != 0) return 1;

    // Test structures
    POINT pt = {100, 200};
    if (pt.x != 100 || pt.y != 200) return 1;

    RECT rc = {0, 0, 640, 400};
    if (rc.right != 640 || rc.bottom != 400) return 1;

    // Test macros
    DWORD dw = MAKELONG(0x1234, 0x5678);
    if (LOWORD(dw) != 0x1234) return 1;
    if (HIWORD(dw) != 0x5678) return 1;

    DWORD color = RGB(255, 128, 64);
    if (GetRValue(color) != 255) return 1;
    if (GetGValue(color) != 128) return 1;
    if (GetBValue(color) != 64) return 1;

    printf("\nAll type tests passed!\n");
    return 0;
}
EOF

if ! clang++ -std=c++17 -I include /tmp/test_windows_types.cpp -o /tmp/test_windows_types 2>&1; then
    echo "VERIFY_FAILED: Test program compilation failed"
    rm -f /tmp/test_windows_types.cpp
    exit 1
fi
echo "  OK: Test program compiled"

# Step 4: Run test
echo "[4/4] Running test program..."
if ! /tmp/test_windows_types; then
    echo "VERIFY_FAILED: Test program failed"
    rm -f /tmp/test_windows_types.cpp /tmp/test_windows_types
    exit 1
fi

# Cleanup
rm -f /tmp/test_windows_types.cpp /tmp/test_windows_types

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Create directory: `mkdir -p include/compat`
2. Create `include/compat/windows.h` with the content above
3. Make verification script executable: `chmod +x ralph-tasks/verify/check-windows-types.sh`
4. Run verification script
5. Fix any issues and repeat

## Success Criteria
- Header compiles without errors or warnings
- All static assertions pass (type sizes)
- Test program runs successfully
- MAKELONG/LOWORD/HIWORD macros work correctly
- RGB macros work correctly
- Structures have correct binary sizes

## Common Issues

### "conflicting types" errors
- May conflict with other headers that define similar types
- Ensure the compat header is included FIRST
- Check that guard macros prevent real Windows header inclusion

### Size assertion failures
- Check that you're using exact integer types (int32_t, not int)
- Use #pragma pack for structures that need exact layout

### min/max macro conflicts
- NOMINMAX is defined to prevent conflicts with std::min/std::max
- We still provide min/max macros for legacy code

### Handle types causing issues
- All handles are void* - this is intentional
- Original code never dereferences handles, just passes them around

## Completion Promise
When verification passes, output:
```
<promise>TASK_13A_COMPLETE</promise>
```

## Escape Hatch
If stuck after 15 iterations:
- Document what's blocking in `ralph-tasks/blocked/13a.md`
- List attempted approaches
- Output: `<promise>TASK_13A_BLOCKED</promise>`

## Max Iterations
15
