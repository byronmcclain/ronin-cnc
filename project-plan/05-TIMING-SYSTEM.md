# Plan 05: Timing System

## Objective
Replace Windows timing APIs (multimedia timers, QueryPerformanceCounter) with SDL2/POSIX timing, ensuring the game runs at the correct speed.

## Current State Analysis

### Windows Timing in Codebase

**Primary Files:**
- `WIN32LIB/TIMER/TIMER.CPP`
- `WIN32LIB/TIMER/TIMER.H`
- `CODE/STARTUP.CPP` (lines 351-361)
- `WIN32LIB/AUDIO/SOUNDIO.CPP` (audio timer callbacks)

**Windows APIs Used:**
```cpp
// High-resolution timing
QueryPerformanceCounter(&counter);
QueryPerformanceFrequency(&frequency);

// Multimedia timers
timeSetEvent(interval, resolution, callback, data, TIME_PERIODIC);
timeKillEvent(timerID);

// Basic timing
GetTickCount();
timeGetTime();
Sleep(milliseconds);
```

**Timer Usage:**
- **Game Timer**: 60 Hz (16.67ms) for game logic
- **Audio Timer**: 40 Hz (25ms) for sound maintenance
- **Frame Timer**: Variable, for FPS measurement

**Key Classes:**
```cpp
class TimerClass {
    int Started;        // When timer was started
    int Accumulated;    // Total elapsed time

    void Start();
    void Stop();
    int Time();         // Get elapsed ticks
};

class CountDownTimerClass {
    int InitialValue;
    int StartTime;

    void Set(int value);
    int Time();         // Time remaining
    bool Expired();
};
```

## SDL2 Implementation

### 5.1 Core Timing Functions

File: `src/platform/timer_sdl.cpp`
```cpp
#include "platform/platform_timer.h"
#include <SDL.h>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>

namespace {

// High-frequency counter baseline
Uint64 g_perf_frequency = 0;
Uint64 g_start_counter = 0;

// Timer callbacks
struct PeriodicTimer {
    SDL_TimerID sdl_id;
    TimerCallback callback;
    void* userdata;
    std::atomic<bool> active;
};

std::vector<PeriodicTimer*> g_timers;
std::mutex g_timer_mutex;
int g_next_timer_handle = 1;

} // anonymous namespace

//=============================================================================
// Basic Time Functions
//=============================================================================

static bool g_timer_initialized = false;

void Platform_Timer_Init(void) {
    if (g_timer_initialized) return;

    g_perf_frequency = SDL_GetPerformanceFrequency();
    g_start_counter = SDL_GetPerformanceCounter();
    g_timer_initialized = true;
}

uint32_t Platform_GetTicks(void) {
    return SDL_GetTicks();
}

void Platform_Delay(uint32_t milliseconds) {
    SDL_Delay(milliseconds);
}

uint64_t Platform_GetPerformanceCounter(void) {
    return SDL_GetPerformanceCounter();
}

uint64_t Platform_GetPerformanceFrequency(void) {
    return SDL_GetPerformanceFrequency();
}

double Platform_GetTime(void) {
    if (!g_timer_initialized) Platform_Timer_Init();

    Uint64 now = SDL_GetPerformanceCounter();
    return (double)(now - g_start_counter) / (double)g_perf_frequency;
}
```

### 5.2 Periodic Timer Implementation

```cpp
//=============================================================================
// SDL Timer Callback Wrapper
//=============================================================================

static Uint32 SDL_Timer_Callback(Uint32 interval, void* param) {
    PeriodicTimer* timer = static_cast<PeriodicTimer*>(param);

    if (timer->active && timer->callback) {
        timer->callback(timer->userdata);
    }

    return timer->active ? interval : 0;  // Return 0 to stop timer
}

//=============================================================================
// Periodic Timer Functions
//=============================================================================

TimerHandle Platform_Timer_Create(uint32_t interval_ms, TimerCallback callback, void* userdata) {
    std::lock_guard<std::mutex> lock(g_timer_mutex);

    PeriodicTimer* timer = new PeriodicTimer();
    timer->callback = callback;
    timer->userdata = userdata;
    timer->active = true;

    timer->sdl_id = SDL_AddTimer(interval_ms, SDL_Timer_Callback, timer);

    if (timer->sdl_id == 0) {
        delete timer;
        return INVALID_TIMER_HANDLE;
    }

    g_timers.push_back(timer);
    return g_next_timer_handle++;
}

void Platform_Timer_Destroy(TimerHandle handle) {
    std::lock_guard<std::mutex> lock(g_timer_mutex);

    if (handle <= 0 || handle > (int)g_timers.size()) return;

    PeriodicTimer* timer = g_timers[handle - 1];
    if (timer) {
        timer->active = false;
        SDL_RemoveTimer(timer->sdl_id);
        delete timer;
        g_timers[handle - 1] = nullptr;
    }
}

void Platform_Timer_SetInterval(TimerHandle handle, uint32_t interval_ms) {
    // SDL doesn't support changing interval of existing timer
    // Would need to destroy and recreate
    // For now, this is a no-op (original code rarely changes intervals)
}
```

### 5.3 Frame Rate Limiter

```cpp
//=============================================================================
// Frame Rate Control (for game loop)
//=============================================================================

namespace {
    Uint64 g_frame_start = 0;
    Uint64 g_frame_target_ticks = 0;
    double g_frame_time = 0.0;
    double g_fps = 0.0;

    // FPS averaging
    double g_fps_samples[60] = {0};
    int g_fps_sample_index = 0;
}

void Platform_Frame_Start(void) {
    g_frame_start = SDL_GetPerformanceCounter();
}

void Platform_Frame_End(int target_fps) {
    Uint64 frame_end = SDL_GetPerformanceCounter();
    Uint64 freq = SDL_GetPerformanceFrequency();

    // Calculate frame time
    g_frame_time = (double)(frame_end - g_frame_start) / (double)freq;

    // Calculate target frame time
    double target_time = 1.0 / (double)target_fps;

    // Sleep if we're running too fast
    double sleep_time = target_time - g_frame_time;
    if (sleep_time > 0.001) {  // > 1ms
        SDL_Delay((Uint32)(sleep_time * 1000.0));
    }

    // Recalculate actual frame time including sleep
    frame_end = SDL_GetPerformanceCounter();
    g_frame_time = (double)(frame_end - g_frame_start) / (double)freq;

    // Update FPS counter
    g_fps_samples[g_fps_sample_index] = 1.0 / g_frame_time;
    g_fps_sample_index = (g_fps_sample_index + 1) % 60;

    // Calculate average FPS
    double sum = 0.0;
    for (int i = 0; i < 60; i++) {
        sum += g_fps_samples[i];
    }
    g_fps = sum / 60.0;
}

double Platform_GetFrameTime(void) {
    return g_frame_time;
}

double Platform_GetFPS(void) {
    return g_fps;
}
```

### 5.4 Windows Compatibility Wrappers

File: `include/compat/timer_wrapper.h`
```cpp
#ifndef TIMER_WRAPPER_H
#define TIMER_WRAPPER_H

#include "platform/platform_timer.h"

//=============================================================================
// QueryPerformanceCounter replacement
//=============================================================================

typedef struct {
    int64_t QuadPart;
} LARGE_INTEGER;

inline BOOL QueryPerformanceCounter(LARGE_INTEGER* counter) {
    counter->QuadPart = Platform_GetPerformanceCounter();
    return TRUE;
}

inline BOOL QueryPerformanceFrequency(LARGE_INTEGER* frequency) {
    frequency->QuadPart = Platform_GetPerformanceFrequency();
    return TRUE;
}

//=============================================================================
// GetTickCount / timeGetTime replacement
//=============================================================================

inline DWORD GetTickCount(void) {
    return Platform_GetTicks();
}

inline DWORD timeGetTime(void) {
    return Platform_GetTicks();
}

//=============================================================================
// Sleep replacement
//=============================================================================

inline void Sleep(DWORD milliseconds) {
    Platform_Delay(milliseconds);
}

//=============================================================================
// Multimedia Timer replacement
//=============================================================================

// Timer callback type
typedef void (CALLBACK *LPTIMECALLBACK)(UINT uTimerID, UINT uMsg,
                                         DWORD_PTR dwUser, DWORD_PTR dw1,
                                         DWORD_PTR dw2);

// Timer flags
#define TIME_ONESHOT    0
#define TIME_PERIODIC   1

// Wrapper to convert callback format
struct MMTimerWrapper {
    LPTIMECALLBACK callback;
    DWORD_PTR user_data;
    TimerHandle platform_handle;
};

static void MMTimer_Callback_Wrapper(void* userdata) {
    MMTimerWrapper* wrapper = static_cast<MMTimerWrapper*>(userdata);
    if (wrapper->callback) {
        wrapper->callback(0, 0, wrapper->user_data, 0, 0);
    }
}

inline MMRESULT timeSetEvent(UINT uDelay, UINT uResolution,
                              LPTIMECALLBACK fptc, DWORD_PTR dwUser,
                              UINT fuEvent) {
    MMTimerWrapper* wrapper = new MMTimerWrapper();
    wrapper->callback = fptc;
    wrapper->user_data = dwUser;

    wrapper->platform_handle = Platform_Timer_Create(
        uDelay,
        MMTimer_Callback_Wrapper,
        wrapper
    );

    // Return wrapper pointer as timer ID (hacky but works)
    return (MMRESULT)(intptr_t)wrapper;
}

inline MMRESULT timeKillEvent(UINT uTimerID) {
    MMTimerWrapper* wrapper = (MMTimerWrapper*)(intptr_t)uTimerID;
    if (wrapper) {
        Platform_Timer_Destroy(wrapper->platform_handle);
        delete wrapper;
    }
    return 0;
}

#endif // TIMER_WRAPPER_H
```

### 5.5 Game Timer Classes

File: `include/compat/gametimer_wrapper.h`
```cpp
#ifndef GAMETIMER_WRAPPER_H
#define GAMETIMER_WRAPPER_H

#include "platform/platform_timer.h"

//=============================================================================
// TimerClass - Measures elapsed time
//=============================================================================

class TimerClass {
private:
    uint32_t started_time;
    uint32_t accumulated;
    bool running;

public:
    TimerClass() : started_time(0), accumulated(0), running(false) {}

    void Start() {
        if (!running) {
            started_time = Platform_GetTicks();
            running = true;
        }
    }

    void Stop() {
        if (running) {
            accumulated += Platform_GetTicks() - started_time;
            running = false;
        }
    }

    void Reset() {
        accumulated = 0;
        if (running) {
            started_time = Platform_GetTicks();
        }
    }

    uint32_t Time() const {
        if (running) {
            return accumulated + (Platform_GetTicks() - started_time);
        }
        return accumulated;
    }

    bool Is_Running() const { return running; }
};

//=============================================================================
// CountDownTimerClass - Counts down from a value
//=============================================================================

class CountDownTimerClass {
private:
    uint32_t start_time;
    uint32_t duration;
    bool active;

public:
    CountDownTimerClass() : start_time(0), duration(0), active(false) {}

    void Set(uint32_t milliseconds) {
        start_time = Platform_GetTicks();
        duration = milliseconds;
        active = true;
    }

    void Clear() {
        active = false;
        duration = 0;
    }

    uint32_t Time() const {
        if (!active) return 0;

        uint32_t elapsed = Platform_GetTicks() - start_time;
        if (elapsed >= duration) return 0;
        return duration - elapsed;
    }

    bool Expired() const {
        if (!active) return true;
        return (Platform_GetTicks() - start_time) >= duration;
    }

    bool Is_Active() const { return active && !Expired(); }
};

//=============================================================================
// Game Frame Timing
//=============================================================================

// Ticks per second (game logic runs at 15 FPS in original)
#define TICKS_PER_SECOND    15
#define TIMER_SECOND        1000

// Convert real time to game ticks
inline int Time_To_Ticks(uint32_t milliseconds) {
    return (milliseconds * TICKS_PER_SECOND) / TIMER_SECOND;
}

// Convert game ticks to real time
inline uint32_t Ticks_To_Time(int ticks) {
    return (ticks * TIMER_SECOND) / TICKS_PER_SECOND;
}

#endif // GAMETIMER_WRAPPER_H
```

## Game Loop Integration

### Fixed Time Step Game Loop

```cpp
// Constants
const double GAME_TICK_RATE = 15.0;  // Game logic at 15 Hz
const double TICK_DURATION = 1.0 / GAME_TICK_RATE;
const int MAX_RENDER_FPS = 60;

// State
double accumulator = 0.0;
double previous_time = Platform_GetTime();

void Game_Loop() {
    while (!Platform_ShouldQuit()) {
        Platform_Frame_Start();

        // Calculate delta time
        double current_time = Platform_GetTime();
        double frame_time = current_time - previous_time;
        previous_time = current_time;

        // Clamp frame time to avoid spiral of death
        if (frame_time > 0.25) frame_time = 0.25;

        accumulator += frame_time;

        // Process input
        Platform_PumpEvents();

        // Update game logic at fixed rate
        while (accumulator >= TICK_DURATION) {
            Game_Logic_Tick();
            accumulator -= TICK_DURATION;
        }

        // Render (interpolate for smooth animation)
        double alpha = accumulator / TICK_DURATION;
        Game_Render(alpha);

        // Frame rate limiting
        Platform_Frame_End(MAX_RENDER_FPS);
    }
}
```

## Tasks Breakdown

### Phase 1: Basic Timing (1 day)
- [ ] Implement `Platform_GetTicks()`, `Platform_Delay()`
- [ ] Implement high-precision counter functions
- [ ] Implement `Platform_GetTime()`
- [ ] Test timing accuracy

### Phase 2: Periodic Timers (1 day)
- [ ] Implement `Platform_Timer_Create()`
- [ ] Implement timer callback system
- [ ] Handle timer destruction safely
- [ ] Test with audio maintenance timer

### Phase 3: Compatibility Layer (1 day)
- [ ] Create `timer_wrapper.h`
- [ ] Map `QueryPerformanceCounter`
- [ ] Map `timeSetEvent`/`timeKillEvent`
- [ ] Map `GetTickCount`/`Sleep`

### Phase 4: Game Timer Classes (1 day)
- [ ] Implement `TimerClass`
- [ ] Implement `CountDownTimerClass`
- [ ] Verify tick rate conversion
- [ ] Test with game logic

## Testing Strategy

### Test 1: Accuracy Test
Measure 1 second with various timing methods, compare to wall clock.

### Test 2: Timer Callback Test
Create periodic timer, verify callback frequency.

### Test 3: Frame Rate Test
Run empty game loop, verify consistent FPS.

### Test 4: Game Speed Test
Run game logic, verify units move at correct speed.

## Acceptance Criteria

- [ ] `Platform_GetTicks()` accurate to ±1ms
- [ ] High-precision timer accurate to microseconds
- [ ] Periodic timers fire at correct rate
- [ ] Game loop runs at target FPS
- [ ] Game logic runs at 15 ticks/second
- [ ] No timer-related crashes or leaks

## Estimated Duration
**3-4 days**

## Dependencies
- Plan 02 (Platform Abstraction) headers
- SDL2 timer subsystem

## Next Plan
Once timing is working, proceed to **Plan 06: Memory Management**
