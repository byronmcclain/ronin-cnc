# Phase 14: Compatibility Layer

## Overview

The original Red Alert code was written for DOS/Windows 95 using Watcom C++ 10.6 and various proprietary libraries. This phase creates a compatibility shim that allows the code to compile with modern Clang while redirecting API calls to our platform layer.

---

## Compilation Blockers

### 1. Watcom-Specific Syntax

```cpp
// Watcom pragmas (ignore or translate)
#pragma aux ...
#pragma library ...
#pragma pack ...

// Watcom calling conventions
__cdecl
__stdcall
__fastcall
```

### 2. Windows API Dependencies

```cpp
// Types
HWND, HINSTANCE, HDC, HPALETTE, HBITMAP
BOOL, DWORD, WORD, BYTE
LPSTR, LPCSTR, LPVOID

// Functions
WinMain()
GetTickCount()
SetDIBits() / GetDIBits()
CreatePalette()
MessageBox()
```

### 3. DirectX 5 Dependencies

```cpp
// DirectDraw
IDirectDraw
IDirectDrawSurface
IDirectDrawPalette
DDSURFACEDESC

// DirectSound (if used)
IDirectSound
IDirectSoundBuffer
```

### 4. Proprietary Libraries

```cpp
// Greenleaf Communications Library (GCL) - Serial/modem
// HMI SOS - Audio
```

---

## Implementation Strategy

### Approach: Header-Only Shims

Create compatibility headers that:
1. Define missing types as platform-compatible equivalents
2. Redirect function calls to platform layer
3. Stub out unused functionality

```
include/compat/
├── windows.h      # Windows types and functions
├── directx.h      # DirectDraw/DirectSound stubs
├── watcom.h       # Compiler compatibility
├── gcl.h          # GCL stubs
└── hmi_sos.h      # Audio library stubs
```

---

## Task 14.1: Windows Types

**File:** `include/compat/windows.h`

```cpp
#ifndef COMPAT_WINDOWS_H
#define COMPAT_WINDOWS_H

#include <cstdint>
#include <cstddef>

// Prevent including real Windows headers
#define _WINDOWS_
#define WIN32_LEAN_AND_MEAN

// Basic types
typedef int32_t BOOL;
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef uint8_t BYTE;
typedef int32_t LONG;
typedef int16_t SHORT;
typedef uint32_t UINT;
typedef int32_t INT;

typedef char* LPSTR;
typedef const char* LPCSTR;
typedef void* LPVOID;
typedef const void* LPCVOID;
typedef DWORD* LPDWORD;

#define TRUE 1
#define FALSE 0
#define NULL nullptr

// Handle types (opaque pointers)
typedef void* HANDLE;
typedef void* HWND;
typedef void* HINSTANCE;
typedef void* HDC;
typedef void* HPALETTE;
typedef void* HBITMAP;
typedef void* HBRUSH;
typedef void* HFONT;
typedef void* HICON;
typedef void* HCURSOR;
typedef void* HMODULE;
typedef void* HGLOBAL;

// Callback types
typedef LONG (*WNDPROC)(HWND, UINT, UINT, LONG);

// Structures
typedef struct tagPOINT {
    LONG x;
    LONG y;
} POINT, *LPPOINT;

typedef struct tagRECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
} RECT, *LPRECT;

typedef struct tagSIZE {
    LONG cx;
    LONG cy;
} SIZE, *LPSIZE;

typedef struct tagPALETTEENTRY {
    BYTE peRed;
    BYTE peGreen;
    BYTE peBlue;
    BYTE peFlags;
} PALETTEENTRY, *LPPALETTEENTRY;

typedef struct tagRGBQUAD {
    BYTE rgbBlue;
    BYTE rgbGreen;
    BYTE rgbRed;
    BYTE rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER;

typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD bmiColors[1];
} BITMAPINFO;

// Message box constants
#define MB_OK 0x00000000
#define MB_OKCANCEL 0x00000001
#define MB_YESNO 0x00000004
#define MB_ICONERROR 0x00000010
#define MB_ICONWARNING 0x00000030
#define IDOK 1
#define IDCANCEL 2
#define IDYES 6
#define IDNO 7

// Virtual key codes (subset)
#define VK_ESCAPE 0x1B
#define VK_RETURN 0x0D
#define VK_SPACE 0x20
#define VK_LEFT 0x25
#define VK_UP 0x26
#define VK_RIGHT 0x27
#define VK_DOWN 0x28
// ... add as needed

// GDI constants
#define DIB_RGB_COLORS 0
#define BI_RGB 0

// Function stubs (redirect to platform layer)
#ifdef __cplusplus
extern "C" {
#endif

// Implemented via platform layer
DWORD GetTickCount(void);
void Sleep(DWORD milliseconds);
int MessageBoxA(HWND hWnd, LPCSTR text, LPCSTR caption, UINT type);

// Stubbed (not needed)
static inline HWND GetActiveWindow(void) { return NULL; }
static inline HWND GetFocus(void) { return NULL; }
static inline BOOL SetFocus(HWND) { return TRUE; }
static inline BOOL ShowWindow(HWND, int) { return TRUE; }
static inline BOOL UpdateWindow(HWND) { return TRUE; }
static inline BOOL InvalidateRect(HWND, LPRECT, BOOL) { return TRUE; }

// GDI stubs
static inline HDC GetDC(HWND) { return NULL; }
static inline int ReleaseDC(HWND, HDC) { return 1; }
static inline int SetDIBitsToDevice(HDC, int, int, DWORD, DWORD,
    int, int, UINT, UINT, const void*, BITMAPINFO*, UINT) { return 0; }

#ifdef __cplusplus
}
#endif

#endif // COMPAT_WINDOWS_H
```

---

## Task 14.2: Watcom Compatibility

**File:** `include/compat/watcom.h`

```cpp
#ifndef COMPAT_WATCOM_H
#define COMPAT_WATCOM_H

// Pretend to be Watcom for conditional compilation
#define __WATCOMC__ 1100

// Calling conventions (no-op on modern compilers)
#define __cdecl
#define __stdcall
#define __fastcall
#define __pascal
#define __export
#define __loadds
#define __saveregs

// Watcom-specific pragmas (ignore)
#define pragma(x)

// Interrupt handlers
#define __interrupt

// Far/near pointers (flat memory model)
#define __far
#define __near
#define __huge
#define far
#define near

// Segment stuff (ignore)
#define __segment
#define __based(x)

// Inline assembly marker (will need manual conversion)
// #define _asm __asm__

// Pack pragmas (may need attention)
// Modern equivalent: #pragma pack(push, 1) ... #pragma pack(pop)

// Disable some Watcom-specific warnings about these
#ifdef __clang__
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#endif

#endif // COMPAT_WATCOM_H
```

---

## Task 14.3: DirectX Stubs

**File:** `include/compat/directx.h`

```cpp
#ifndef COMPAT_DIRECTX_H
#define COMPAT_DIRECTX_H

#include "windows.h"

// DirectDraw result codes
#define DD_OK 0
#define DDERR_GENERIC 0x80004005
#define DDERR_SURFACELOST 0x887601C2
#define DDERR_WASSTILLDRAWING 0x8876021C

// DirectDraw constants
#define DDSD_CAPS 0x00000001
#define DDSD_HEIGHT 0x00000002
#define DDSD_WIDTH 0x00000004
#define DDSD_PITCH 0x00000008
#define DDSD_PIXELFORMAT 0x00001000
#define DDSD_LPSURFACE 0x00000800

#define DDSCAPS_PRIMARYSURFACE 0x00000200
#define DDSCAPS_OFFSCREENPLAIN 0x00000040
#define DDSCAPS_SYSTEMMEMORY 0x00000800
#define DDSCAPS_VIDEOMEMORY 0x00004000

#define DDLOCK_WAIT 0x00000001
#define DDLOCK_SURFACEMEMORYPTR 0x00000000

// Structures
typedef struct _DDSCAPS {
    DWORD dwCaps;
} DDSCAPS;

typedef struct _DDPIXELFORMAT {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwFourCC;
    DWORD dwRGBBitCount;
    DWORD dwRBitMask;
    DWORD dwGBitMask;
    DWORD dwBBitMask;
    DWORD dwRGBAlphaBitMask;
} DDPIXELFORMAT;

typedef struct _DDSURFACEDESC {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwHeight;
    DWORD dwWidth;
    LONG lPitch;
    DWORD dwBackBufferCount;
    DWORD dwRefreshRate;
    DWORD dwAlphaBitDepth;
    DWORD dwReserved;
    LPVOID lpSurface;
    DDPIXELFORMAT ddpfPixelFormat;
    DDSCAPS ddsCaps;
} DDSURFACEDESC, *LPDDSURFACEDESC;

// Interface forward declarations (as opaque types)
typedef struct IDirectDraw* LPDIRECTDRAW;
typedef struct IDirectDrawSurface* LPDIRECTDRAWSURFACE;
typedef struct IDirectDrawPalette* LPDIRECTDRAWPALETTE;
typedef struct IDirectDrawClipper* LPDIRECTDRAWCLIPPER;

// We won't implement these - the platform layer replaces them
// Just need type definitions for compilation

struct IDirectDraw {
    // Stub vtable
};

struct IDirectDrawSurface {
    // Stub vtable
};

struct IDirectDrawPalette {
    // Stub vtable
};

// DirectDraw creation (stubbed)
static inline DWORD DirectDrawCreate(void*, LPDIRECTDRAW*, void*) {
    return DDERR_GENERIC; // Always fail - use platform layer instead
}

#endif // COMPAT_DIRECTX_H
```

---

## Task 14.4: Audio Library Stubs

**File:** `include/compat/hmi_sos.h`

```cpp
#ifndef COMPAT_HMI_SOS_H
#define COMPAT_HMI_SOS_H

// HMI Sound Operating System (SOS) stubs
// The platform layer provides audio via rodio

#include <cstdint>

// SOS return codes
#define _SOS_NO_ERROR 0
#define _SOS_INVALID_HANDLE -1

// SOS sample handle
typedef int32_t HSAMPLE;

// Stubbed functions - redirect to platform audio
#define sosDIGIInitSystem(a, b, c, d) _SOS_NO_ERROR
#define sosDIGIUnInitSystem(a) _SOS_NO_ERROR
#define sosDIGIStartSample(a, b, c, d) 0
#define sosDIGIStopSample(a, b) _SOS_NO_ERROR
#define sosDIGISetSampleVolume(a, b, c) _SOS_NO_ERROR
#define sosDIGIGetSampleVolume(a, b) 128
#define sosDIGISamplesPlaying(a) 0

// If actual audio integration is needed, these should call:
// Platform_Sound_Play()
// Platform_Sound_Stop()
// etc.

#endif // COMPAT_HMI_SOS_H
```

**File:** `include/compat/gcl.h`

```cpp
#ifndef COMPAT_GCL_H
#define COMPAT_GCL_H

// Greenleaf Communications Library stubs
// Used for modem/serial multiplayer (obsolete)

#define GCL_SUCCESS 0
#define GCL_ERROR -1

// All GCL functions return failure
// Network multiplayer uses enet via platform layer instead

#define gcl_init() GCL_ERROR
#define gcl_dial(x) GCL_ERROR
#define gcl_answer() GCL_ERROR
#define gcl_hangup() GCL_ERROR
#define gcl_send(a, b) GCL_ERROR
#define gcl_receive(a, b) GCL_ERROR

#endif // COMPAT_GCL_H
```

---

## Task 14.5: Main Compatibility Header

**File:** `include/compat/compat.h`

```cpp
#ifndef COMPAT_H
#define COMPAT_H

// Master compatibility header - include this instead of Windows headers

// Order matters!
#include "compat/watcom.h"
#include "compat/windows.h"
#include "compat/directx.h"
#include "compat/hmi_sos.h"
#include "compat/gcl.h"

// Platform layer bridge
#include "platform.h"

// Redirect key Windows functions to platform layer
#ifdef __cplusplus
extern "C" {
#endif

// Time functions -> Platform timer
inline DWORD GetTickCount(void) {
    return Platform_Timer_GetTicks();
}

inline void Sleep(DWORD ms) {
    Platform_Timer_Delay(ms);
}

// Message box -> Platform (or printf for now)
inline int MessageBoxA(HWND, LPCSTR text, LPCSTR caption, UINT) {
    Platform_LogError("MessageBox: %s - %s", caption, text);
    return IDOK;
}

#ifdef __cplusplus
}
#endif

// Common macro definitions
#ifndef NOMINMAX
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#endif // COMPAT_H
```

---

## Task 14.6: Function Implementations

**File:** `src/compat/compat.cpp`

```cpp
#include "compat/compat.h"
#include "platform.h"

// Implement any functions that can't be inlined

extern "C" {

// If we need more complex MessageBox behavior
int Platform_MessageBox(const char* text, const char* caption, int type) {
    // For now, just log it
    Platform_LogInfo("[%s] %s", caption, text);

    // Could show a native macOS alert via Cocoa
    // For now, return OK
    return IDOK;
}

// GDI color operations (if needed)
DWORD RGB(BYTE r, BYTE g, BYTE b) {
    return ((DWORD)r) | (((DWORD)g) << 8) | (((DWORD)b) << 16);
}

BYTE GetRValue(DWORD rgb) { return (BYTE)(rgb & 0xFF); }
BYTE GetGValue(DWORD rgb) { return (BYTE)((rgb >> 8) & 0xFF); }
BYTE GetBValue(DWORD rgb) { return (BYTE)((rgb >> 16) & 0xFF); }

} // extern "C"
```

---

## Integration with Build System

Update `CMakeLists.txt`:

```cmake
# Compatibility layer
add_library(compat STATIC
    src/compat/compat.cpp
)

target_include_directories(compat PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/compat
)

target_link_libraries(compat PRIVATE redalert_platform)

# For game code compilation
target_compile_definitions(compat PUBLIC
    # Suppress Windows header inclusion
    _WINDOWS_
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)
```

---

## Testing Strategy

### Compilation Test

```cpp
// test_compat.cpp
#include "compat/compat.h"

int main() {
    // Test Windows types
    DWORD dw = 0;
    BOOL b = TRUE;
    HWND hwnd = NULL;

    // Test functions
    DWORD ticks = GetTickCount();
    Sleep(10);

    // Test structures
    POINT pt = {0, 0};
    RECT rc = {0, 0, 100, 100};

    printf("Compat layer compiled successfully\n");
    printf("Ticks: %u\n", ticks);

    return 0;
}
```

### Verification Script

```bash
#!/bin/bash
# verify/check-compat-headers.sh

echo "Testing compatibility header compilation..."

cat > /tmp/test_compat.cpp << 'EOF'
#include "compat/compat.h"
int main() {
    DWORD t = GetTickCount();
    Sleep(1);
    return (t > 0) ? 0 : 1;
}
EOF

clang++ -std=c++17 -I include -c /tmp/test_compat.cpp -o /tmp/test_compat.o
if [ $? -eq 0 ]; then
    echo "PASS: Compatibility headers compile"
    rm /tmp/test_compat.cpp /tmp/test_compat.o
    exit 0
else
    echo "FAIL: Compilation error"
    exit 1
fi
```

---

## Files to Modify in Original Code

When porting original CODE/*.CPP files:

1. **Replace includes:**
```cpp
// Before
#include <windows.h>
#include <ddraw.h>

// After
#include "compat/compat.h"
```

2. **Remove WinMain:**
```cpp
// Before
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)

// After
int main(int argc, char* argv[])
```

3. **Replace DirectDraw calls:**
```cpp
// Before
lpDD->CreateSurface(&ddsd, &lpSurface, NULL);

// After
uint8_t* pixels;
int32_t width, height, pitch;
Platform_Graphics_GetBackBuffer(&pixels, &width, &height, &pitch);
```

---

## Success Criteria

- [ ] All compat headers compile without errors
- [ ] GetTickCount() returns valid values
- [ ] Sleep() delays correctly
- [ ] Windows types (DWORD, HWND, etc.) defined
- [ ] DirectDraw structures defined (for type compatibility)
- [ ] Original CODE files can include compat.h
- [ ] No conflicts with platform.h

---

## Estimated Effort

| Task | Hours |
|------|-------|
| 14.1 Windows Types | 3-4 |
| 14.2 Watcom Compat | 1-2 |
| 14.3 DirectX Stubs | 2-3 |
| 14.4 Audio Stubs | 1-2 |
| 14.5 Master Header | 1-2 |
| 14.6 Implementations | 2-3 |
| Testing | 2-3 |
| **Total** | **12-19** |
