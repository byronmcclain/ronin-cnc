# Plan 02: Platform Abstraction Layer

## Objective
Create a clean abstraction layer that isolates all platform-specific code, allowing the game logic to remain unchanged while supporting multiple platforms.

## Design Philosophy

The abstraction layer follows these principles:
1. **Game code calls abstract interfaces** - Never platform APIs directly
2. **Platform layer implements interfaces** - SDL2/POSIX/etc behind the scenes
3. **Compile-time platform selection** - No runtime overhead
4. **C-style API** - Easy to call from existing C++ code

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     GAME CODE (Original)                      │
│                  (CODE/*.CPP, WIN32LIB/*.CPP)                │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                   PLATFORM INTERFACE                          │
│         (include/platform/*.h - Abstract C API)               │
├─────────────────────────────────────────────────────────────┤
│  Platform_Graphics_*  │  Platform_Audio_*  │  Platform_Input_* │
│  Platform_Timer_*     │  Platform_File_*   │  Platform_Net_*   │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                  PLATFORM IMPLEMENTATION                      │
│               (src/platform/*_sdl.cpp)                        │
├─────────────────────────────────────────────────────────────┤
│      SDL2       │     POSIX      │     enet      │  physfs   │
└─────────────────────────────────────────────────────────────┘
```

## Interface Definitions

### 2.1 Core Platform Interface
File: `include/platform/platform.h`
```c
#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Platform Initialization
//=============================================================================

typedef struct {
    const char* window_title;
    int window_width;
    int window_height;
    bool fullscreen;
    bool vsync;
} PlatformConfig;

bool Platform_Init(const PlatformConfig* config);
void Platform_Shutdown(void);
void Platform_PumpEvents(void);  // Process OS events
bool Platform_ShouldQuit(void);

//=============================================================================
// Error Handling
//=============================================================================

const char* Platform_GetError(void);
void Platform_SetError(const char* fmt, ...);
void Platform_ClearError(void);

//=============================================================================
// Logging
//=============================================================================

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} LogLevel;

void Platform_Log(LogLevel level, const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif // PLATFORM_H
```

### 2.2 Graphics Interface
File: `include/platform/platform_graphics.h`
```c
#ifndef PLATFORM_GRAPHICS_H
#define PLATFORM_GRAPHICS_H

#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Display Modes
//=============================================================================

typedef struct {
    int width;
    int height;
    int bits_per_pixel;  // 8 for palettized
} DisplayMode;

bool Platform_Graphics_Init(void);
void Platform_Graphics_Shutdown(void);
bool Platform_Graphics_SetMode(const DisplayMode* mode);
void Platform_Graphics_GetMode(DisplayMode* mode);

//=============================================================================
// Surfaces (replaces DirectDraw surfaces)
//=============================================================================

typedef struct PlatformSurface PlatformSurface;

// Surface creation
PlatformSurface* Platform_Surface_Create(int width, int height, int bpp);
PlatformSurface* Platform_Surface_CreateFromMemory(void* pixels, int width, int height, int pitch, int bpp);
void Platform_Surface_Destroy(PlatformSurface* surface);

// Primary surface (the screen)
PlatformSurface* Platform_Graphics_GetPrimarySurface(void);
PlatformSurface* Platform_Graphics_GetBackBuffer(void);

// Surface operations
bool Platform_Surface_Lock(PlatformSurface* surface, void** pixels, int* pitch);
void Platform_Surface_Unlock(PlatformSurface* surface);
bool Platform_Surface_Blit(PlatformSurface* dest, int dx, int dy,
                           PlatformSurface* src, int sx, int sy, int sw, int sh);
bool Platform_Surface_BlitScaled(PlatformSurface* dest, int dx, int dy, int dw, int dh,
                                 PlatformSurface* src, int sx, int sy, int sw, int sh);
void Platform_Surface_Fill(PlatformSurface* surface, int x, int y, int w, int h, uint8_t color);
void Platform_Surface_Clear(PlatformSurface* surface, uint8_t color);

//=============================================================================
// Palette (for 8-bit mode)
//=============================================================================

typedef struct {
    uint8_t r, g, b;
} PaletteEntry;

void Platform_Graphics_SetPalette(const PaletteEntry* palette, int start, int count);
void Platform_Graphics_GetPalette(PaletteEntry* palette, int start, int count);

//=============================================================================
// Page Flipping
//=============================================================================

void Platform_Graphics_Flip(void);           // Swap front/back buffers
void Platform_Graphics_WaitVSync(void);      // Wait for vertical blank

//=============================================================================
// Video Memory Info (for compatibility)
//=============================================================================

uint32_t Platform_Graphics_GetFreeVideoMemory(void);
bool Platform_Graphics_IsHardwareAccelerated(void);

#ifdef __cplusplus
}
#endif

#endif // PLATFORM_GRAPHICS_H
```

### 2.3 Input Interface
File: `include/platform/platform_input.h`
```c
#ifndef PLATFORM_INPUT_H
#define PLATFORM_INPUT_H

#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Keyboard
//=============================================================================

// Key codes (map from Windows VK_* codes)
typedef enum {
    KEY_NONE = 0,
    KEY_ESCAPE = 0x1B,
    KEY_RETURN = 0x0D,
    KEY_SPACE = 0x20,
    KEY_UP = 0x26,
    KEY_DOWN = 0x28,
    KEY_LEFT = 0x25,
    KEY_RIGHT = 0x27,
    KEY_SHIFT = 0x10,
    KEY_CONTROL = 0x11,
    KEY_ALT = 0x12,
    // ... full mapping in implementation
    KEY_COUNT = 256
} KeyCode;

bool Platform_Input_Init(void);
void Platform_Input_Shutdown(void);
void Platform_Input_Update(void);  // Called each frame

bool Platform_Key_IsPressed(KeyCode key);    // Currently held
bool Platform_Key_WasPressed(KeyCode key);   // Just pressed this frame
bool Platform_Key_WasReleased(KeyCode key);  // Just released this frame

// Key buffer (for text input and key queue)
bool Platform_Key_GetNext(KeyCode* key, bool* released);
void Platform_Key_Clear(void);

// Modifier state
bool Platform_Key_ShiftDown(void);
bool Platform_Key_CtrlDown(void);
bool Platform_Key_AltDown(void);
bool Platform_Key_CapsLock(void);
bool Platform_Key_NumLock(void);

//=============================================================================
// Mouse
//=============================================================================

typedef enum {
    MOUSE_LEFT = 0,
    MOUSE_RIGHT = 1,
    MOUSE_MIDDLE = 2
} MouseButton;

void Platform_Mouse_GetPosition(int* x, int* y);
void Platform_Mouse_SetPosition(int x, int y);
bool Platform_Mouse_IsPressed(MouseButton button);
bool Platform_Mouse_WasClicked(MouseButton button);
bool Platform_Mouse_WasDoubleClicked(MouseButton button);
int Platform_Mouse_GetWheelDelta(void);

void Platform_Mouse_Show(void);
void Platform_Mouse_Hide(void);
void Platform_Mouse_Confine(bool confine);  // Lock to window

#ifdef __cplusplus
}
#endif

#endif // PLATFORM_INPUT_H
```

### 2.4 Audio Interface
File: `include/platform/platform_audio.h`
```c
#ifndef PLATFORM_AUDIO_H
#define PLATFORM_AUDIO_H

#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Audio System
//=============================================================================

typedef struct {
    int sample_rate;      // e.g., 22050
    int channels;         // 1 = mono, 2 = stereo
    int bits_per_sample;  // 8 or 16
    int buffer_size;      // In samples
} AudioConfig;

bool Platform_Audio_Init(const AudioConfig* config);
void Platform_Audio_Shutdown(void);
void Platform_Audio_SetMasterVolume(float volume);  // 0.0 - 1.0
float Platform_Audio_GetMasterVolume(void);

//=============================================================================
// Sound Handles
//=============================================================================

typedef struct PlatformSound PlatformSound;

// Load from memory (for ADPCM-decoded data)
PlatformSound* Platform_Sound_CreateFromMemory(const void* data, int size,
                                                int sample_rate, int channels,
                                                int bits_per_sample);
void Platform_Sound_Destroy(PlatformSound* sound);

//=============================================================================
// Sound Playback
//=============================================================================

typedef int SoundHandle;  // Playing instance handle
#define INVALID_SOUND_HANDLE (-1)

SoundHandle Platform_Sound_Play(PlatformSound* sound, float volume, float pan, bool loop);
void Platform_Sound_Stop(SoundHandle handle);
void Platform_Sound_StopAll(void);
bool Platform_Sound_IsPlaying(SoundHandle handle);
void Platform_Sound_SetVolume(SoundHandle handle, float volume);
void Platform_Sound_SetPan(SoundHandle handle, float pan);  // -1.0 (left) to 1.0 (right)

//=============================================================================
// Streaming Audio (for music and large files)
//=============================================================================

typedef struct PlatformStream PlatformStream;

// Callback to fill audio buffer
typedef int (*AudioStreamCallback)(void* userdata, uint8_t* buffer, int bytes_requested);

PlatformStream* Platform_Stream_Create(int sample_rate, int channels, int bits_per_sample,
                                       AudioStreamCallback callback, void* userdata);
void Platform_Stream_Destroy(PlatformStream* stream);
void Platform_Stream_Play(PlatformStream* stream);
void Platform_Stream_Pause(PlatformStream* stream);
void Platform_Stream_Stop(PlatformStream* stream);
void Platform_Stream_SetVolume(PlatformStream* stream, float volume);

#ifdef __cplusplus
}
#endif

#endif // PLATFORM_AUDIO_H
```

### 2.5 Timer Interface
File: `include/platform/platform_timer.h`
```c
#ifndef PLATFORM_TIMER_H
#define PLATFORM_TIMER_H

#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Time Functions
//=============================================================================

// Millisecond precision
uint32_t Platform_GetTicks(void);           // Milliseconds since init
void Platform_Delay(uint32_t milliseconds); // Sleep

// High-precision timing
uint64_t Platform_GetPerformanceCounter(void);
uint64_t Platform_GetPerformanceFrequency(void);

// Convenience: Get time in seconds (floating point)
double Platform_GetTime(void);

//=============================================================================
// Periodic Timers (replaces Windows multimedia timers)
//=============================================================================

typedef void (*TimerCallback)(void* userdata);
typedef int TimerHandle;
#define INVALID_TIMER_HANDLE (-1)

TimerHandle Platform_Timer_Create(uint32_t interval_ms, TimerCallback callback, void* userdata);
void Platform_Timer_Destroy(TimerHandle handle);
void Platform_Timer_SetInterval(TimerHandle handle, uint32_t interval_ms);

// Note: Callbacks may be called from a separate thread!
// Use appropriate synchronization.

#ifdef __cplusplus
}
#endif

#endif // PLATFORM_TIMER_H
```

### 2.6 File System Interface
File: `include/platform/platform_file.h`
```c
#ifndef PLATFORM_FILE_H
#define PLATFORM_FILE_H

#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Path Utilities
//=============================================================================

// Get platform-specific paths
const char* Platform_GetBasePath(void);      // Where the executable is
const char* Platform_GetPrefPath(void);      // Writable config/save location
const char* Platform_GetSeparator(void);     // "/" on Unix, "\" on Windows

// Path manipulation
void Platform_NormalizePath(char* path);     // Fix separators, resolve ..
bool Platform_PathExists(const char* path);
bool Platform_IsDirectory(const char* path);
bool Platform_CreateDirectory(const char* path);

//=============================================================================
// File Operations
//=============================================================================

typedef struct PlatformFile PlatformFile;

typedef enum {
    FILE_READ = 1,
    FILE_WRITE = 2,
    FILE_APPEND = 4
} FileMode;

typedef enum {
    SEEK_FROM_START,
    SEEK_FROM_CURRENT,
    SEEK_FROM_END
} SeekOrigin;

PlatformFile* Platform_File_Open(const char* path, FileMode mode);
void Platform_File_Close(PlatformFile* file);
int Platform_File_Read(PlatformFile* file, void* buffer, int size);
int Platform_File_Write(PlatformFile* file, const void* buffer, int size);
bool Platform_File_Seek(PlatformFile* file, int64_t offset, SeekOrigin origin);
int64_t Platform_File_Tell(PlatformFile* file);
int64_t Platform_File_Size(PlatformFile* file);
bool Platform_File_Eof(PlatformFile* file);

//=============================================================================
// Directory Enumeration
//=============================================================================

typedef struct PlatformDir PlatformDir;

typedef struct {
    char name[256];
    bool is_directory;
    int64_t size;
} DirEntry;

PlatformDir* Platform_Dir_Open(const char* path);
bool Platform_Dir_Read(PlatformDir* dir, DirEntry* entry);
void Platform_Dir_Close(PlatformDir* dir);

//=============================================================================
// Archive Support (for MIX files)
//=============================================================================

bool Platform_Archive_Mount(const char* archive_path, const char* mount_point);
void Platform_Archive_Unmount(const char* archive_path);

#ifdef __cplusplus
}
#endif

#endif // PLATFORM_FILE_H
```

### 2.7 Network Interface
File: `include/platform/platform_network.h`
```c
#ifndef PLATFORM_NETWORK_H
#define PLATFORM_NETWORK_H

#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Network Initialization
//=============================================================================

bool Platform_Network_Init(void);
void Platform_Network_Shutdown(void);

//=============================================================================
// Host/Server
//=============================================================================

typedef struct PlatformHost PlatformHost;
typedef struct PlatformPeer PlatformPeer;

typedef struct {
    uint16_t port;
    int max_clients;
    int channel_count;
} HostConfig;

PlatformHost* Platform_Host_Create(const HostConfig* config);
void Platform_Host_Destroy(PlatformHost* host);
void Platform_Host_Service(PlatformHost* host, uint32_t timeout_ms);

//=============================================================================
// Client Connection
//=============================================================================

typedef struct {
    const char* host_address;
    uint16_t port;
    int channel_count;
} ConnectConfig;

PlatformPeer* Platform_Connect(const ConnectConfig* config, uint32_t timeout_ms);
void Platform_Disconnect(PlatformPeer* peer);
bool Platform_IsConnected(PlatformPeer* peer);

//=============================================================================
// Sending/Receiving
//=============================================================================

typedef enum {
    PACKET_RELIABLE,     // TCP-like, ordered
    PACKET_UNRELIABLE,   // UDP-like, may be lost
    PACKET_UNSEQUENCED   // UDP-like, may arrive out of order
} PacketType;

bool Platform_Send(PlatformPeer* peer, int channel, const void* data, int size, PacketType type);

typedef struct {
    PlatformPeer* peer;
    int channel;
    void* data;
    int size;
} ReceivedPacket;

bool Platform_Receive(PlatformHost* host, ReceivedPacket* packet);
void Platform_FreePacket(ReceivedPacket* packet);

//=============================================================================
// Peer Information
//=============================================================================

void Platform_GetPeerAddress(PlatformPeer* peer, char* buffer, int buffer_size);
uint32_t Platform_GetPeerRTT(PlatformPeer* peer);  // Round-trip time in ms

#ifdef __cplusplus
}
#endif

#endif // PLATFORM_NETWORK_H
```

### 2.8 Threading Interface
File: `include/platform/platform_thread.h`
```c
#ifndef PLATFORM_THREAD_H
#define PLATFORM_THREAD_H

#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Mutexes (replaces CRITICAL_SECTION)
//=============================================================================

typedef struct PlatformMutex PlatformMutex;

PlatformMutex* Platform_Mutex_Create(void);
void Platform_Mutex_Destroy(PlatformMutex* mutex);
void Platform_Mutex_Lock(PlatformMutex* mutex);
bool Platform_Mutex_TryLock(PlatformMutex* mutex);
void Platform_Mutex_Unlock(PlatformMutex* mutex);

//=============================================================================
// Threads
//=============================================================================

typedef struct PlatformThread PlatformThread;
typedef int (*ThreadFunc)(void* userdata);

PlatformThread* Platform_Thread_Create(ThreadFunc func, void* userdata, const char* name);
void Platform_Thread_Join(PlatformThread* thread);
void Platform_Thread_Detach(PlatformThread* thread);
void Platform_Thread_SetPriority(PlatformThread* thread, int priority);  // -2 to +2

//=============================================================================
// Atomic Operations
//=============================================================================

int32_t Platform_AtomicAdd(volatile int32_t* ptr, int32_t value);
int32_t Platform_AtomicSub(volatile int32_t* ptr, int32_t value);
int32_t Platform_AtomicGet(volatile int32_t* ptr);
void Platform_AtomicSet(volatile int32_t* ptr, int32_t value);
bool Platform_AtomicCompareExchange(volatile int32_t* ptr, int32_t expected, int32_t desired);

#ifdef __cplusplus
}
#endif

#endif // PLATFORM_THREAD_H
```

## Implementation Tasks

### Task 1: Core Platform (2 days)
- Implement `platform.h` functions
- SDL2 initialization
- Event loop integration
- Error handling and logging

### Task 2: Create Stub Implementations (1 day)
- Create empty implementations of all interfaces
- Return sensible defaults (success, zero, etc.)
- Allows game to compile before implementations are complete

### Task 3: Mapping Layer (2 days)
- Create macros/functions that map Windows calls to Platform calls
- Example: `DirectDrawCreate()` → `Platform_Graphics_Init()`
- Placed in compatibility headers

### Task 4: Integration Testing (1 day)
- Verify game code compiles with new headers
- Fix any missing mappings
- Document any Windows features that can't be abstracted

## File Structure

```
include/
├── compat/
│   ├── windows.h           # Master Windows replacement
│   ├── windows_types.h     # Basic types (DWORD, HANDLE, etc.)
│   ├── ddraw_stub.h        # DirectDraw → Platform_Graphics
│   ├── dsound_stub.h       # DirectSound → Platform_Audio
│   └── winsock_stub.h      # Winsock → Platform_Network
└── platform/
    ├── platform.h          # Core platform functions
    ├── platform_graphics.h # Graphics abstraction
    ├── platform_input.h    # Input abstraction
    ├── platform_audio.h    # Audio abstraction
    ├── platform_timer.h    # Timing abstraction
    ├── platform_file.h     # File system abstraction
    ├── platform_network.h  # Network abstraction
    └── platform_thread.h   # Threading abstraction

src/platform/
├── CMakeLists.txt
├── platform_sdl.cpp        # Core SDL2 implementation
├── graphics_sdl.cpp        # Graphics implementation
├── input_sdl.cpp           # Input implementation
├── audio_sdl.cpp           # Audio implementation (or OpenAL)
├── timer_sdl.cpp           # Timer implementation
├── file_posix.cpp          # POSIX file implementation
├── network_enet.cpp        # enet networking implementation
└── thread_sdl.cpp          # SDL2 threading (or pthread)
```

## Acceptance Criteria

- [ ] All platform interfaces defined in headers
- [ ] Stub implementations compile
- [ ] Game code compiles using platform interfaces
- [ ] No direct Windows API calls remain in game code
- [ ] Clear separation between game and platform code

## Estimated Duration
**5-7 days**

## Dependencies
- Plan 01 (Build System) completed
- SDL2 available in build

## Next Plan
Once abstraction layer is in place, proceed to **Plan 03: Graphics System**
