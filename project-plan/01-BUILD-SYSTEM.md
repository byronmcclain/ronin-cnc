# Plan 01: Build System Setup

## Objective
Replace the Watcom/TASM build system with CMake, enabling compilation on macOS with modern compilers (Clang/AppleClang).

## Current State
- Watcom `wmake` makefiles (`.MAK` files)
- TASM assembly files (`.ASM`)
- Visual Studio 6.0 project for Launcher (`.dsp/.dsw`)
- No cross-platform build support

## Target State
- Single CMakeLists.txt hierarchy
- Compiles on macOS with Xcode/Clang
- Future-ready for Linux (GCC)
- Proper dependency management

## Tasks

### Phase 1: Project Structure (2-3 days)

#### 1.1 Create Root CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.20)
project(RedAlert VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection
if(APPLE)
    set(PLATFORM_MACOS TRUE)
    add_compile_definitions(PLATFORM_MACOS)
elseif(UNIX)
    set(PLATFORM_LINUX TRUE)
    add_compile_definitions(PLATFORM_LINUX)
endif()

# Subdirectories
add_subdirectory(src/platform)
add_subdirectory(src/game)
add_subdirectory(deps)
```

#### 1.2 Create Directory Structure
```bash
mkdir -p src/platform/{graphics,audio,input,network,system}
mkdir -p src/game
mkdir -p deps
mkdir -p include/platform
mkdir -p include/compat    # Windows compatibility headers
```

#### 1.3 Dependency Management
Options (in order of preference):
1. **FetchContent** - Download at configure time
2. **vcpkg/conan** - Package manager
3. **Git submodules** - Manual but reliable

```cmake
include(FetchContent)

FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.28.5
)

FetchContent_Declare(
    enet
    GIT_REPOSITORY https://github.com/lsalzman/enet.git
    GIT_TAG v1.3.17
)

FetchContent_MakeAvailable(SDL2 enet)
```

### Phase 2: Windows Compatibility Layer (3-4 days)

#### 2.1 Create Windows Type Stubs
File: `include/compat/windows_types.h`
```c
#ifndef WINDOWS_TYPES_H
#define WINDOWS_TYPES_H

#include <stdint.h>
#include <stdbool.h>

// Basic Windows types
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef uint8_t BYTE;
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef int BOOL;
typedef void* HANDLE;
typedef void* HWND;
typedef void* HINSTANCE;
typedef void* HDC;
typedef void* HPALETTE;
typedef void* LPVOID;
typedef const char* LPCSTR;
typedef char* LPSTR;
typedef const wchar_t* LPCWSTR;
typedef wchar_t* LPWSTR;

// Windows constants
#define TRUE 1
#define FALSE 0
#define NULL nullptr
#define CALLBACK
#define WINAPI
#define APIENTRY

// HIWORD/LOWORD macros
#define LOWORD(l) ((WORD)(l))
#define HIWORD(l) ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#define LOBYTE(w) ((BYTE)(w))
#define HIBYTE(w) ((BYTE)(((WORD)(w) >> 8) & 0xFF))

#define MAKELONG(a, b) ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
#define MAKEWORD(a, b) ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))

// Error codes
#define S_OK 0
#define E_FAIL (-1)
typedef long HRESULT;
#define SUCCEEDED(hr) ((hr) >= 0)
#define FAILED(hr) ((hr) < 0)

#endif // WINDOWS_TYPES_H
```

#### 2.2 Create DirectX Stubs
File: `include/compat/ddraw_stub.h`
```c
#ifndef DDRAW_STUB_H
#define DDRAW_STUB_H

#include "windows_types.h"

// Forward declarations - actual implementation in platform layer
typedef struct IDirectDraw* LPDIRECTDRAW;
typedef struct IDirectDraw2* LPDIRECTDRAW2;
typedef struct IDirectDrawSurface* LPDIRECTDRAWSURFACE;
typedef struct IDirectDrawPalette* LPDIRECTDRAWPALETTE;

// Stub structures
typedef struct {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwHeight;
    DWORD dwWidth;
    LONG lPitch;
    LPVOID lpSurface;
    // ... additional fields as needed
} DDSURFACEDESC;

// DirectDraw function stubs - redirect to platform layer
#define DirectDrawCreate(a, b, c) Platform_CreateGraphics(b)

#endif // DDRAW_STUB_H
```

#### 2.3 Create DirectSound Stubs
File: `include/compat/dsound_stub.h`
```c
#ifndef DSOUND_STUB_H
#define DSOUND_STUB_H

#include "windows_types.h"

typedef struct IDirectSound* LPDIRECTSOUND;
typedef struct IDirectSoundBuffer* LPDIRECTSOUNDBUFFER;

typedef struct {
    WORD wFormatTag;
    WORD nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD nBlockAlign;
    WORD wBitsPerSample;
    WORD cbSize;
} WAVEFORMATEX;

#define DirectSoundCreate(a, b, c) Platform_CreateAudio(b)

#endif // DSOUND_STUB_H
```

#### 2.4 Create Master Windows Header
File: `include/compat/windows.h`
```c
#ifndef COMPAT_WINDOWS_H
#define COMPAT_WINDOWS_H

// This replaces <windows.h> for cross-platform builds

#include "windows_types.h"
#include "ddraw_stub.h"
#include "dsound_stub.h"

// Message constants (used by game code)
#define WM_CREATE       0x0001
#define WM_DESTROY      0x0002
#define WM_PAINT        0x000F
#define WM_KEYDOWN      0x0100
#define WM_KEYUP        0x0101
#define WM_MOUSEMOVE    0x0200
#define WM_LBUTTONDOWN  0x0201
#define WM_LBUTTONUP    0x0202
#define WM_RBUTTONDOWN  0x0204
#define WM_RBUTTONUP    0x0205
// ... add as needed

// Virtual key codes - map to SDL later
#define VK_ESCAPE       0x1B
#define VK_RETURN       0x0D
#define VK_SPACE        0x20
#define VK_LEFT         0x25
#define VK_UP           0x26
#define VK_RIGHT        0x27
#define VK_DOWN         0x28
// ... add as needed

// Stub functions - implement in platform layer
inline BOOL PostMessage(HWND h, UINT m, WPARAM w, LPARAM l) { return TRUE; }
inline void Sleep(DWORD ms);  // Implement with SDL_Delay
inline DWORD GetTickCount(); // Implement with SDL_GetTicks

#endif // COMPAT_WINDOWS_H
```

### Phase 3: Source File Integration (3-4 days)

#### 3.1 Inventory Source Files
Create a script to list all source files:
```bash
find CODE -name "*.CPP" -o -name "*.C" > src_files.txt
find CODE -name "*.H" >> src_files.txt
find WIN32LIB -name "*.CPP" -o -name "*.C" >> src_files.txt
```

#### 3.2 Create Game Library CMakeLists.txt
File: `src/game/CMakeLists.txt`
```cmake
# Collect original source files
file(GLOB_RECURSE GAME_SOURCES
    "${CMAKE_SOURCE_DIR}/CODE/*.CPP"
    "${CMAKE_SOURCE_DIR}/CODE/*.C"
)

# Exclude Windows-specific files that we're replacing
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*STARTUP\\.CPP$")
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*DDRAW\\.CPP$")
# ... more exclusions

# Exclude assembly files (handled separately)
file(GLOB ASM_FILES "${CMAKE_SOURCE_DIR}/CODE/*.ASM")

add_library(game STATIC ${GAME_SOURCES})

target_include_directories(game PUBLIC
    ${CMAKE_SOURCE_DIR}/CODE
    ${CMAKE_SOURCE_DIR}/WIN32LIB/INCLUDE
    ${CMAKE_SOURCE_DIR}/include/compat
    ${CMAKE_SOURCE_DIR}/include/platform
)

target_link_libraries(game PRIVATE platform)
```

#### 3.3 Handle Case Sensitivity
macOS/Linux are case-sensitive. Create a script to fix includes:
```bash
#!/bin/bash
# fix_includes.sh - Normalize include statements

find . -name "*.CPP" -o -name "*.H" | while read file; do
    # Convert #include "HEADER.H" to lowercase
    sed -i '' 's/#include "\([^"]*\)"/#include "\L\1"/g' "$file"
done
```

### Phase 4: Initial Compilation (2-3 days)

#### 4.1 Create Stub Main Entry Point
File: `src/platform/main.cpp`
```cpp
#include <SDL.h>
#include <iostream>

// Forward declaration of game's main
extern int Game_Main(int argc, char* argv[]);

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    int result = Game_Main(argc, argv);

    SDL_Quit();
    return result;
}
```

#### 4.2 Iterative Compilation
Process:
1. Run `cmake --build .`
2. Fix first error (missing header, unknown type, etc.)
3. Repeat until it compiles

Common fixes needed:
- Missing Windows types → Add to `windows_types.h`
- Missing DirectX calls → Add stubs
- Inline assembly → Comment out or `#ifdef` around
- Path separators → Will fix in File System phase

#### 4.3 Create Compilation Tracking
File: `project-plan/compilation-status.md`
Track each source file's compilation status.

## Acceptance Criteria

- [ ] `cmake -B build -G Ninja` succeeds
- [ ] `cmake --build build` compiles all C/C++ files
- [ ] Executable links (even with unresolved externals from stubs)
- [ ] No Watcom-specific syntax remains
- [ ] All 277 CPP files from CODE/ included in build

## Files to Create

| File | Purpose |
|------|---------|
| `CMakeLists.txt` (root) | Main build configuration |
| `src/platform/CMakeLists.txt` | Platform library build |
| `src/game/CMakeLists.txt` | Game library build |
| `include/compat/windows.h` | Windows API compatibility |
| `include/compat/windows_types.h` | Basic type definitions |
| `include/compat/ddraw_stub.h` | DirectDraw stubs |
| `include/compat/dsound_stub.h` | DirectSound stubs |
| `src/platform/main.cpp` | Entry point |

## Estimated Duration
**5-7 days**

## Dependencies
- CMake 3.20+
- Xcode Command Line Tools (for Clang)
- SDL2 (fetched automatically)

## Next Plan
Once compilation succeeds, proceed to **Plan 02: Platform Abstraction Layer**
