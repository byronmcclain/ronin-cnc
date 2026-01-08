# Platform API Reference

This document describes the C FFI interface exposed by the platform layer.

## Initialization

### Platform_Init
```c
PlatformResult Platform_Init(void);
```
Initialize the platform layer. Must be called before any other platform functions.

**Returns:**
- `Success (0)` - Initialization successful
- `AlreadyInitialized (1)` - Already initialized
- `InitFailed (3)` - Initialization failed

### Platform_Shutdown
```c
PlatformResult Platform_Shutdown(void);
```
Shutdown the platform layer. Releases all resources.

### Platform_IsInitialized
```c
bool Platform_IsInitialized(void);
```
Check if platform is initialized.

### Platform_GetVersion
```c
int32_t Platform_GetVersion(void);
```
Get platform layer version number.

## Configuration Paths

### Platform_GetConfigPath
```c
int32_t Platform_GetConfigPath(char* buffer, int32_t buffer_size);
```
Get the configuration directory path.

**Parameters:**
- `buffer` - Output buffer for path string
- `buffer_size` - Size of buffer in bytes

**Returns:** Number of bytes written, or -1 on error

**Example:** `/Users/name/Library/Application Support/RedAlert`

### Platform_GetSavesPath
```c
int32_t Platform_GetSavesPath(char* buffer, int32_t buffer_size);
```
Get the save games directory path.

### Platform_GetLogPath
```c
int32_t Platform_GetLogPath(char* buffer, int32_t buffer_size);
```
Get the log files directory path.

### Platform_GetCachePath
```c
int32_t Platform_GetCachePath(char* buffer, int32_t buffer_size);
```
Get the cache directory path.

### Platform_GetDataPath
```c
int32_t Platform_GetDataPath(char* buffer, int32_t buffer_size);
```
Get the game data directory path.

### Platform_EnsureDirectories
```c
int32_t Platform_EnsureDirectories(void);
```
Create all configuration directories if they don't exist.

**Returns:** 0 on success, -1 on error

## Logging

### Platform_Log_Init
```c
int32_t Platform_Log_Init(void);
```
Initialize the logging system.

### Platform_Log_Shutdown
```c
void Platform_Log_Shutdown(void);
```
Shutdown the logging system.

### Platform_Log
```c
void Platform_Log(LogLevel level, const char* message);
```
Log a message.

**Levels:**
- `LOG_LEVEL_DEBUG (0)` - Debug messages
- `LOG_LEVEL_INFO (1)` - Informational messages
- `LOG_LEVEL_WARN (2)` - Warning messages
- `LOG_LEVEL_ERROR (3)` - Error messages

### Platform_Log_SetLevel
```c
void Platform_Log_SetLevel(int32_t level);
```
Set the minimum log level.

### Platform_Log_GetLevel
```c
int32_t Platform_Log_GetLevel(void);
```
Get the current log level.

### Platform_Log_Flush
```c
void Platform_Log_Flush(void);
```
Flush log buffer to disk.

## Application Lifecycle

### Platform_GetAppState
```c
int32_t Platform_GetAppState(void);
```
Get current application state.

**Returns:**
- `0` - Active (foreground)
- `1` - Background
- `2` - Terminating

### Platform_IsAppActive
```c
int32_t Platform_IsAppActive(void);
```
Check if app is in foreground.

### Platform_IsAppBackground
```c
int32_t Platform_IsAppBackground(void);
```
Check if app is in background.

### Platform_RequestQuit
```c
void Platform_RequestQuit(void);
```
Request application quit. Game should exit gracefully.

### Platform_ShouldQuit
```c
int32_t Platform_ShouldQuit(void);
```
Check if quit has been requested.

### Platform_ClearQuitRequest
```c
void Platform_ClearQuitRequest(void);
```
Clear the quit request flag.

## Performance

### Platform_FrameStart
```c
void Platform_FrameStart(void);
```
Mark the start of a frame.

### Platform_FrameEnd
```c
void Platform_FrameEnd(void);
```
Mark the end of a frame.

### Platform_GetFPS
```c
int32_t Platform_GetFPS(void);
```
Get current frames per second.

### Platform_GetFrameTime
```c
int32_t Platform_GetFrameTime(void);
```
Get last frame time in milliseconds.

### Platform_GetFrameCount
```c
uint32_t Platform_GetFrameCount(void);
```
Get total frame count since start.

## Display

### Platform_IsRetinaDisplay
```c
int32_t Platform_IsRetinaDisplay(void);
```
Check if running on a Retina/HiDPI display.

### Platform_GetDisplayScale
```c
float Platform_GetDisplayScale(void);
```
Get the display scale factor (1.0 for standard, 2.0 for Retina).

### Platform_ToggleFullscreen
```c
int32_t Platform_ToggleFullscreen(void);
```
Toggle fullscreen mode.

### Platform_IsFullscreen
```c
int32_t Platform_IsFullscreen(void);
```
Check if in fullscreen mode.

## Networking

### Platform_Network_Init
```c
int32_t Platform_Network_Init(void);
```
Initialize networking subsystem.

### Platform_Network_Shutdown
```c
void Platform_Network_Shutdown(void);
```
Shutdown networking subsystem.

### Platform_Network_IsInitialized
```c
int32_t Platform_Network_IsInitialized(void);
```
Check if networking is initialized.

For full networking API, see the generated `platform.h` header.

## Error Codes

| Code | Name | Description |
|------|------|-------------|
| 0 | Success | Operation succeeded |
| 1 | AlreadyInitialized | Already initialized |
| 2 | NotInitialized | Not initialized |
| 3 | InitFailed | Initialization failed |
| -1 | Error | Generic error |

## Thread Safety

- All functions are safe to call from the main thread
- Audio functions may be called from any thread
- Network functions are internally synchronized
- Do not call graphics functions from background threads
