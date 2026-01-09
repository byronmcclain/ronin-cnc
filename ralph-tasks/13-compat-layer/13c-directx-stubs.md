# Task 13c: DirectX Stubs

## Dependencies
- Task 13a (Windows Types) must be complete

## Context
Red Alert uses DirectDraw (part of DirectX 5) for video output and optionally DirectSound for audio. Since we're replacing these with our platform layer (SDL2 + rodio), we need stub definitions that allow the original code to compile without actually implementing DirectX.

The key insight is that we're NOT going to implement DirectDraw - we're going to REPLACE all DirectDraw calls with platform layer calls. But to compile the transitional code, we need the type definitions to exist.

## Objective
Create `include/compat/directx.h` that provides:
1. DirectDraw return codes and constants
2. Surface description structures (DDSURFACEDESC)
3. Pixel format structures (DDPIXELFORMAT)
4. Interface type definitions (as opaque pointers)
5. Creation function stubs that fail gracefully

## Technical Background

### DirectDraw Architecture
```
IDirectDraw (main interface)
├── CreateSurface() → IDirectDrawSurface
├── CreatePalette() → IDirectDrawPalette
├── CreateClipper() → IDirectDrawClipper
└── SetCooperativeLevel(), SetDisplayMode()
```

### Why Stubs Work
The original code pattern is:
```cpp
if (DirectDrawCreate(NULL, &lpDD, NULL) == DD_OK) {
    // Use DirectDraw
} else {
    // Fallback or error
}
```

Our stub makes DirectDrawCreate() return DDERR_GENERIC, so the code takes the error path. Meanwhile, our platform layer initialization (called elsewhere) sets up the actual graphics.

### Structures Still Needed
Some code references DDSURFACEDESC fields for passing dimensions:
```cpp
DDSURFACEDESC ddsd;
ddsd.dwWidth = 640;
ddsd.dwHeight = 400;
```
We need these structures defined even if DirectDraw itself isn't used.

## Deliverables
- [ ] `include/compat/directx.h` - DirectX compatibility definitions
- [ ] Structures have correct binary layout
- [ ] Compiles without errors
- [ ] Verification script passes

## Files to Create

### include/compat/directx.h (NEW)
```cpp
/**
 * DirectX Compatibility Layer
 *
 * Provides DirectDraw type definitions for compiling original Red Alert code.
 * These are STUBS only - actual graphics use the platform layer (SDL2).
 *
 * DirectDraw was part of DirectX 5 (Windows 95/NT era).
 * We only define enough to allow compilation and graceful failure.
 */

#ifndef COMPAT_DIRECTX_H
#define COMPAT_DIRECTX_H

#include "compat/windows.h"

// =============================================================================
// DirectDraw Return Codes
// =============================================================================

// Success
#define DD_OK                       0x00000000

// Error codes (HRESULT format: negative values)
#define DDERR_GENERIC               0x80004005
#define DDERR_UNSUPPORTED           0x80004001
#define DDERR_INVALIDPARAMS         0x80070057
#define DDERR_OUTOFMEMORY           0x8007000E
#define DDERR_SURFACELOST           0x887601C2
#define DDERR_WASSTILLDRAWING       0x8876021C
#define DDERR_SURFACEBUSY           0x8876021D
#define DDERR_NOTFOUND              0x887601F5
#define DDERR_NOTINITIALIZED        0x887601F4
#define DDERR_ALREADYINITIALIZED    0x887601F3
#define DDERR_NOCOOPERATIVELEVELSET 0x887601F2
#define DDERR_PRIMARYSURFACEALREADYEXISTS 0x887601F1
#define DDERR_NOHWND                0x887601EF
#define DDERR_HWNDSUBCLASSED        0x887601EE
#define DDERR_HWNDALREADYSET        0x887601ED
#define DDERR_NOPALETTEATTACHED     0x887601EC
#define DDERR_NOPALETTEHW           0x887601EB
#define DDERR_BLTFASTCANTCLIP       0x887601EA
#define DDERR_NOCLIPPERATTACHED     0x887601E9
#define DDERR_NOMIPMAPHW            0x887601E8
#define DDERR_INVALIDSURFACETYPE    0x887601E7
#define DDERR_LOCKEDSURFACES        0x887601E6
#define DDERR_NO3D                  0x887601E5
#define DDERR_NOEXCLUSIVEMODE       0x887601E4
#define DDERR_NOFLIPHW              0x887601E3
#define DDERR_NOTFLIPPABLE          0x887601E2
#define DDERR_INCOMPATIBLEPRIMARY   0x887601E1
#define DDERR_CANNOTATTACHSURFACE   0x887601E0
#define DDERR_DCALREADYCREATED      0x887601DF
#define DDERR_INVALIDDIRECTDRAWGUID 0x887601DE

// Test macro for success
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define FAILED(hr)    (((HRESULT)(hr)) < 0)

typedef LONG HRESULT;

// =============================================================================
// DirectDraw Flags - DDSURFACEDESC.dwFlags
// =============================================================================

#define DDSD_CAPS           0x00000001  // ddsCaps is valid
#define DDSD_HEIGHT         0x00000002  // dwHeight is valid
#define DDSD_WIDTH          0x00000004  // dwWidth is valid
#define DDSD_PITCH          0x00000008  // lPitch is valid
#define DDSD_BACKBUFFERCOUNT 0x00000020 // dwBackBufferCount is valid
#define DDSD_ZBUFFERBITDEPTH 0x00000040 // dwZBufferBitDepth is valid
#define DDSD_ALPHABITDEPTH  0x00000080  // dwAlphaBitDepth is valid
#define DDSD_LPSURFACE      0x00000800  // lpSurface is valid
#define DDSD_PIXELFORMAT    0x00001000  // ddpfPixelFormat is valid
#define DDSD_CKDESTOVERLAY  0x00002000  // ddckCKDestOverlay is valid
#define DDSD_CKDESTBLT      0x00004000  // ddckCKDestBlt is valid
#define DDSD_CKSRCOVERLAY   0x00008000  // ddckCKSrcOverlay is valid
#define DDSD_CKSRCBLT       0x00010000  // ddckCKSrcBlt is valid
#define DDSD_MIPMAPCOUNT    0x00020000  // dwMipMapCount is valid
#define DDSD_REFRESHRATE    0x00040000  // dwRefreshRate is valid
#define DDSD_ALL            0x0007F9EE

// =============================================================================
// DirectDraw Flags - DDSCAPS.dwCaps
// =============================================================================

#define DDSCAPS_ALPHA           0x00000002
#define DDSCAPS_BACKBUFFER      0x00000004
#define DDSCAPS_COMPLEX         0x00000008
#define DDSCAPS_FLIP            0x00000010
#define DDSCAPS_FRONTBUFFER     0x00000020
#define DDSCAPS_OFFSCREENPLAIN  0x00000040
#define DDSCAPS_OVERLAY         0x00000080
#define DDSCAPS_PALETTE         0x00000100
#define DDSCAPS_PRIMARYSURFACE  0x00000200
#define DDSCAPS_SYSTEMMEMORY    0x00000800
#define DDSCAPS_TEXTURE         0x00001000
#define DDSCAPS_3DDEVICE        0x00002000
#define DDSCAPS_VIDEOMEMORY     0x00004000
#define DDSCAPS_VISIBLE         0x00008000
#define DDSCAPS_WRITEONLY       0x00010000
#define DDSCAPS_ZBUFFER         0x00020000
#define DDSCAPS_OWNDC           0x00040000
#define DDSCAPS_LIVEVIDEO       0x00080000
#define DDSCAPS_HWCODEC         0x00100000
#define DDSCAPS_MODEX           0x00200000
#define DDSCAPS_MIPMAP          0x00400000
#define DDSCAPS_ALLOCONLOAD     0x04000000
#define DDSCAPS_VIDEOPORT       0x08000000
#define DDSCAPS_LOCALVIDMEM     0x10000000
#define DDSCAPS_NONLOCALVIDMEM  0x20000000
#define DDSCAPS_STANDARDVGAMODE 0x40000000
#define DDSCAPS_OPTIMIZED       0x80000000

// =============================================================================
// DirectDraw Flags - Lock
// =============================================================================

#define DDLOCK_SURFACEMEMORYPTR 0x00000000
#define DDLOCK_WAIT             0x00000001
#define DDLOCK_EVENT            0x00000002
#define DDLOCK_READONLY         0x00000010
#define DDLOCK_WRITEONLY        0x00000020
#define DDLOCK_NOSYSLOCK        0x00000800

// =============================================================================
// DirectDraw Flags - Blt
// =============================================================================

#define DDBLT_ALPHADEST         0x00000001
#define DDBLT_ALPHASRC          0x00000002
#define DDBLT_ASYNC             0x00000200
#define DDBLT_COLORFILL         0x00000400
#define DDBLT_DDFX              0x00000800
#define DDBLT_KEYDEST           0x00002000
#define DDBLT_KEYSRC            0x00008000
#define DDBLT_ROP               0x00020000
#define DDBLT_ROTATIONANGLE     0x00040000
#define DDBLT_WAIT              0x01000000

// =============================================================================
// DirectDraw Flags - BltFast
// =============================================================================

#define DDBLTFAST_NOCOLORKEY    0x00000000
#define DDBLTFAST_SRCCOLORKEY   0x00000001
#define DDBLTFAST_DESTCOLORKEY  0x00000002
#define DDBLTFAST_WAIT          0x00000010

// =============================================================================
// DirectDraw Flags - Flip
// =============================================================================

#define DDFLIP_WAIT             0x00000001
#define DDFLIP_EVEN             0x00000002
#define DDFLIP_ODD              0x00000004
#define DDFLIP_NOVSYNC          0x00000008

// =============================================================================
// DirectDraw Flags - Pixel Format
// =============================================================================

#define DDPF_ALPHAPIXELS        0x00000001
#define DDPF_ALPHA              0x00000002
#define DDPF_FOURCC             0x00000004
#define DDPF_PALETTEINDEXED4    0x00000008
#define DDPF_PALETTEINDEXED8    0x00000020
#define DDPF_RGB                0x00000040
#define DDPF_COMPRESSED         0x00000080
#define DDPF_RGBTOYUV           0x00000100
#define DDPF_YUV                0x00000200
#define DDPF_ZBUFFER            0x00000400
#define DDPF_PALETTEINDEXED1    0x00000800
#define DDPF_PALETTEINDEXED2    0x00001000

// =============================================================================
// DirectDraw Flags - SetCooperativeLevel
// =============================================================================

#define DDSCL_FULLSCREEN        0x00000001
#define DDSCL_ALLOWREBOOT       0x00000002
#define DDSCL_NOWINDOWCHANGES   0x00000004
#define DDSCL_NORMAL            0x00000008
#define DDSCL_EXCLUSIVE         0x00000010
#define DDSCL_ALLOWMODEX        0x00000040
#define DDSCL_SETFOCUSWINDOW    0x00000080
#define DDSCL_SETDEVICEWINDOW   0x00000100
#define DDSCL_CREATEDEVICEWINDOW 0x00000200

// =============================================================================
// DirectDraw Flags - EnumDisplayModes
// =============================================================================

#define DDEDM_REFRESHRATES      0x00000001
#define DDEDM_STANDARDVGAMODES  0x00000002

// =============================================================================
// DirectDraw Structures
// =============================================================================

#pragma pack(push, 1)

/// Capability flags
typedef struct _DDSCAPS {
    DWORD dwCaps;
} DDSCAPS, *LPDDSCAPS;

/// Color key (for transparency)
typedef struct _DDCOLORKEY {
    DWORD dwColorSpaceLowValue;
    DWORD dwColorSpaceHighValue;
} DDCOLORKEY, *LPDDCOLORKEY;

/// Pixel format descriptor
typedef struct _DDPIXELFORMAT {
    DWORD dwSize;           // Size of this structure (32)
    DWORD dwFlags;          // DDPF_* flags
    DWORD dwFourCC;         // FourCC code (for compressed formats)
    union {
        DWORD dwRGBBitCount;        // Bits per pixel
        DWORD dwYUVBitCount;
        DWORD dwZBufferBitDepth;
        DWORD dwAlphaBitDepth;
    };
    union {
        DWORD dwRBitMask;           // Red mask
        DWORD dwYBitMask;
    };
    union {
        DWORD dwGBitMask;           // Green mask
        DWORD dwUBitMask;
    };
    union {
        DWORD dwBBitMask;           // Blue mask
        DWORD dwVBitMask;
    };
    union {
        DWORD dwRGBAlphaBitMask;    // Alpha mask
        DWORD dwYUVAlphaBitMask;
        DWORD dwRGBZBitMask;
        DWORD dwYUVZBitMask;
    };
} DDPIXELFORMAT, *LPDDPIXELFORMAT;

/// Surface description
typedef struct _DDSURFACEDESC {
    DWORD         dwSize;               // Size of this structure
    DWORD         dwFlags;              // DDSD_* flags
    DWORD         dwHeight;             // Surface height
    DWORD         dwWidth;              // Surface width
    union {
        LONG      lPitch;               // Bytes per row
        DWORD     dwLinearSize;         // Total size for compressed
    };
    DWORD         dwBackBufferCount;    // Number of back buffers
    union {
        DWORD     dwMipMapCount;        // Number of mipmap levels
        DWORD     dwRefreshRate;        // Refresh rate
        DWORD     dwZBufferBitDepth;    // Z-buffer depth
    };
    DWORD         dwAlphaBitDepth;      // Alpha bit depth
    DWORD         dwReserved;           // Reserved
    LPVOID        lpSurface;            // Pointer to surface memory
    DDCOLORKEY    ddckCKDestOverlay;    // Dest overlay color key
    DDCOLORKEY    ddckCKDestBlt;        // Dest blt color key
    DDCOLORKEY    ddckCKSrcOverlay;     // Src overlay color key
    DDCOLORKEY    ddckCKSrcBlt;         // Src blt color key
    DDPIXELFORMAT ddpfPixelFormat;      // Pixel format
    DDSCAPS       ddsCaps;              // Surface capabilities
} DDSURFACEDESC, *LPDDSURFACEDESC;

/// DirectDraw surface description (version 2)
typedef DDSURFACEDESC DDSURFACEDESC2, *LPDDSURFACEDESC2;

/// Blit effects structure
typedef struct _DDBLTFX {
    DWORD dwSize;
    DWORD dwDDFX;
    DWORD dwROP;
    DWORD dwDDROP;
    DWORD dwRotationAngle;
    DWORD dwZBufferOpCode;
    DWORD dwZBufferLow;
    DWORD dwZBufferHigh;
    DWORD dwZBufferBaseDest;
    DWORD dwZDestConstBitDepth;
    union {
        DWORD dwZDestConst;
        LPVOID lpDDSZBufferDest;
    };
    DWORD dwZSrcConstBitDepth;
    union {
        DWORD dwZSrcConst;
        LPVOID lpDDSZBufferSrc;
    };
    DWORD dwAlphaEdgeBlendBitDepth;
    DWORD dwAlphaEdgeBlend;
    DWORD dwReserved;
    DWORD dwAlphaDestConstBitDepth;
    union {
        DWORD dwAlphaDestConst;
        LPVOID lpDDSAlphaDest;
    };
    DWORD dwAlphaSrcConstBitDepth;
    union {
        DWORD dwAlphaSrcConst;
        LPVOID lpDDSAlphaSrc;
    };
    union {
        DWORD dwFillColor;
        DWORD dwFillDepth;
        DWORD dwFillPixel;
        LPVOID lpDDSPattern;
    };
    DDCOLORKEY ddckDestColorkey;
    DDCOLORKEY ddckSrcColorkey;
} DDBLTFX, *LPDDBLTFX;

#pragma pack(pop)

// Static assertions for structure sizes
static_assert(sizeof(DDSCAPS) == 4, "DDSCAPS must be 4 bytes");
static_assert(sizeof(DDCOLORKEY) == 8, "DDCOLORKEY must be 8 bytes");
static_assert(sizeof(DDPIXELFORMAT) == 32, "DDPIXELFORMAT must be 32 bytes");

// =============================================================================
// DirectDraw Interface Pointers (Opaque)
// =============================================================================
// We define these as opaque struct pointers. The actual structs are empty
// because we never dereference them - all calls are stubbed to fail.

struct IDirectDraw;
struct IDirectDraw2;
struct IDirectDraw4;
struct IDirectDrawSurface;
struct IDirectDrawSurface2;
struct IDirectDrawSurface3;
struct IDirectDrawPalette;
struct IDirectDrawClipper;

typedef struct IDirectDraw*         LPDIRECTDRAW;
typedef struct IDirectDraw2*        LPDIRECTDRAW2;
typedef struct IDirectDraw4*        LPDIRECTDRAW4;
typedef struct IDirectDrawSurface*  LPDIRECTDRAWSURFACE;
typedef struct IDirectDrawSurface2* LPDIRECTDRAWSURFACE2;
typedef struct IDirectDrawSurface3* LPDIRECTDRAWSURFACE3;
typedef struct IDirectDrawPalette*  LPDIRECTDRAWPALETTE;
typedef struct IDirectDrawClipper*  LPDIRECTDRAWCLIPPER;

// =============================================================================
// DirectDraw Function Stubs
// =============================================================================

#ifdef __cplusplus
extern "C" {
#endif

// DirectDrawCreate - ALWAYS fails, forcing fallback to platform layer
static inline HRESULT DirectDrawCreate(void* lpGUID, LPDIRECTDRAW* lplpDD, void* pUnkOuter) {
    (void)lpGUID;
    (void)pUnkOuter;
    if (lplpDD) *lplpDD = nullptr;
    return DDERR_NOTINITIALIZED; // Force failure
}

static inline HRESULT DirectDrawCreateEx(void* lpGUID, void** lplpDD, void* iid, void* pUnkOuter) {
    (void)lpGUID;
    (void)iid;
    (void)pUnkOuter;
    if (lplpDD) *lplpDD = nullptr;
    return DDERR_NOTINITIALIZED;
}

// DirectDrawEnumerate - reports no devices
static inline HRESULT DirectDrawEnumerateA(void* lpCallback, LPVOID lpContext) {
    (void)lpCallback;
    (void)lpContext;
    return DD_OK; // Success but no devices enumerated
}

#ifdef __cplusplus
}
#endif

// =============================================================================
// DirectSound Stubs (if needed)
// =============================================================================

// DirectSound return codes
#define DS_OK           0
#define DSERR_GENERIC   0x80004005

struct IDirectSound;
struct IDirectSoundBuffer;

typedef struct IDirectSound*       LPDIRECTSOUND;
typedef struct IDirectSoundBuffer* LPDIRECTSOUNDBUFFER;

#ifdef __cplusplus
extern "C" {
#endif

static inline HRESULT DirectSoundCreate(void* lpGUID, LPDIRECTSOUND* lplpDS, void* pUnkOuter) {
    (void)lpGUID;
    (void)pUnkOuter;
    if (lplpDS) *lplpDS = nullptr;
    return DSERR_GENERIC;
}

#ifdef __cplusplus
}
#endif

// =============================================================================
// End of DirectX Compatibility Header
// =============================================================================

#endif // COMPAT_DIRECTX_H
```

## Verification Command
```bash
ralph-tasks/verify/check-directx-stubs.sh
```

## Verification Script

### ralph-tasks/verify/check-directx-stubs.sh (NEW)
```bash
#!/bin/bash
# Verification script for DirectX stubs

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking DirectX Stubs ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check header exists
echo "[1/4] Checking header file exists..."
if [ ! -f "include/compat/directx.h" ]; then
    echo "VERIFY_FAILED: include/compat/directx.h not found"
    exit 1
fi
echo "  OK: Header file exists"

# Step 2: Check dependency exists
echo "[2/4] Checking dependencies..."
if [ ! -f "include/compat/windows.h" ]; then
    echo "VERIFY_FAILED: Dependency include/compat/windows.h not found"
    exit 1
fi
echo "  OK: Dependencies exist"

# Step 3: Compile test program
echo "[3/4] Compiling test program..."
cat > /tmp/test_directx.cpp << 'EOF'
#include "compat/directx.h"
#include <cstdio>

int main() {
    printf("Testing DirectX stubs...\n\n");

    // Test DirectDraw creation (should fail gracefully)
    LPDIRECTDRAW lpDD = nullptr;
    HRESULT hr = DirectDrawCreate(nullptr, &lpDD, nullptr);
    if (hr == DD_OK) {
        printf("FAIL: DirectDrawCreate should not succeed\n");
        return 1;
    }
    printf("DirectDrawCreate: Failed as expected (0x%08X)\n", (unsigned)hr);

    // Test DirectSound creation (should fail gracefully)
    LPDIRECTSOUND lpDS = nullptr;
    hr = DirectSoundCreate(nullptr, &lpDS, nullptr);
    if (hr == DS_OK) {
        printf("FAIL: DirectSoundCreate should not succeed\n");
        return 1;
    }
    printf("DirectSoundCreate: Failed as expected (0x%08X)\n", (unsigned)hr);

    // Test structure sizes
    printf("\nStructure sizes:\n");
    printf("  DDSCAPS:       %zu bytes\n", sizeof(DDSCAPS));
    printf("  DDCOLORKEY:    %zu bytes\n", sizeof(DDCOLORKEY));
    printf("  DDPIXELFORMAT: %zu bytes\n", sizeof(DDPIXELFORMAT));
    printf("  DDSURFACEDESC: %zu bytes\n", sizeof(DDSURFACEDESC));

    // Test using structures
    DDSURFACEDESC ddsd;
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
    ddsd.dwWidth = 640;
    ddsd.dwHeight = 400;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    printf("\nTest DDSURFACEDESC: %ux%u\n", ddsd.dwWidth, ddsd.dwHeight);

    // Test macros
    if (!FAILED(DDERR_GENERIC)) {
        printf("FAIL: FAILED macro not working\n");
        return 1;
    }
    if (!SUCCEEDED(DD_OK)) {
        printf("FAIL: SUCCEEDED macro not working\n");
        return 1;
    }
    printf("FAILED/SUCCEEDED macros: OK\n");

    printf("\nAll DirectX stub tests passed!\n");
    return 0;
}
EOF

if ! clang++ -std=c++17 -I include /tmp/test_directx.cpp -o /tmp/test_directx 2>&1; then
    echo "VERIFY_FAILED: Test program compilation failed"
    rm -f /tmp/test_directx.cpp
    exit 1
fi
echo "  OK: Test program compiled"

# Step 4: Run test
echo "[4/4] Running test program..."
if ! /tmp/test_directx; then
    echo "VERIFY_FAILED: Test program failed"
    rm -f /tmp/test_directx.cpp /tmp/test_directx
    exit 1
fi

# Cleanup
rm -f /tmp/test_directx.cpp /tmp/test_directx

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Ensure Task 13a is complete (windows.h dependency)
2. Create `include/compat/directx.h` with the content above
3. Make verification script executable
4. Run verification script
5. Fix any issues and repeat

## Success Criteria
- Header compiles without errors
- DirectDrawCreate() returns failure code
- DirectSoundCreate() returns failure code
- Structures can be instantiated and used
- FAILED/SUCCEEDED macros work correctly
- Static assertions pass for structure sizes

## Common Issues

### "DDSURFACEDESC size mismatch"
- Structure has padding issues
- May need to adjust #pragma pack
- Check that all unions are correctly defined

### "Redefinition of HRESULT"
- May conflict with other headers
- Ensure proper include order
- Check guard macros

### "DirectDraw interface methods needed"
- Original code may call lpDD->SetCooperativeLevel()
- These need to be replaced with platform layer calls
- The compatibility layer doesn't implement interface methods

## Completion Promise
When verification passes, output:
```
<promise>TASK_13C_COMPLETE</promise>
```

## Escape Hatch
If stuck after 12 iterations:
- Document what's blocking in `ralph-tasks/blocked/13c.md`
- Output: `<promise>TASK_13C_BLOCKED</promise>`

## Max Iterations
12
