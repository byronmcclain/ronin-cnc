# Platform Layer Architecture

This document describes the architecture of the Red Alert macOS platform layer.

## Overview

The platform layer provides a modern, cross-platform abstraction over OS services:

```
┌─────────────────────────────────────────────────┐
│              Original C++ Game Code             │
│    (CODE/, WIN32LIB/ - largely unchanged)       │
├─────────────────────────────────────────────────┤
│                FFI Interface                    │
│        (include/platform.h - generated)         │
├─────────────────────────────────────────────────┤
│           Rust Platform Layer                   │
│              (platform/src/)                    │
├─────────────────────────────────────────────────┤
│              SDL2 / enet / rodio                │
├─────────────────────────────────────────────────┤
│                  macOS                          │
└─────────────────────────────────────────────────┘
```

## Module Structure

```
platform/src/
├── lib.rs              # Crate root, FFI exports
├── error.rs            # Error types
├── graphics/           # SDL2 rendering
│   ├── mod.rs          # Window, canvas management
│   ├── surface.rs      # Off-screen surfaces
│   ├── palette.rs      # 8-bit palette handling
│   └── display.rs      # Retina, fullscreen
├── input/              # Keyboard, mouse input
│   └── mod.rs
├── audio/              # Sound and music
│   ├── mod.rs          # Audio device, playback
│   └── adpcm.rs        # IMA ADPCM decoder
├── timer/              # Timing and delays
│   └── mod.rs
├── files/              # File I/O
│   └── mod.rs
├── memory/             # Allocators
│   └── mod.rs
├── network/            # Multiplayer networking
│   ├── mod.rs          # Init, shutdown
│   ├── host.rs         # Server functionality
│   ├── client.rs       # Client functionality
│   ├── packet.rs       # Packet types
│   └── manager.rs      # High-level API
├── blit/               # Blitting operations
│   └── mod.rs
├── compression/        # LCW compression
│   └── mod.rs
├── config/             # Configuration, paths
│   ├── mod.rs
│   └── paths.rs
├── logging/            # Debug logging
│   └── mod.rs
├── app/                # Application lifecycle
│   ├── mod.rs
│   └── lifecycle.rs
└── perf/               # Performance utilities
    ├── mod.rs
    ├── simd.rs         # SIMD optimization
    ├── pool.rs         # Object pooling
    └── timing.rs       # Frame timing
```

## Key Design Decisions

### 1. Rust for Platform Layer

**Why Rust?**
- Memory safety without garbage collection
- Zero-cost abstractions
- Excellent FFI support
- Cross-platform by default

### 2. FFI Design

All platform functions follow this pattern:
- `extern "C"` for C ABI compatibility
- `#[no_mangle]` to prevent name mangling
- Return codes for errors (not exceptions)
- Pointers for output parameters
- Documentation in Rust generates header comments

### 3. Global State Management

Some modules use global state (SDL context, network state):
- Protected by `Mutex` for thread safety
- `OnceCell` for one-time initialization
- Atomic types for lock-free access where possible

### 4. Error Handling

- Rust code uses `Result<T, E>` internally
- FFI boundary converts to integer error codes
- Detailed errors logged, simple codes returned

## Data Flow

### Graphics Frame
```
Game Logic
    ↓ writes to back buffer
Platform_Surface_Lock()
    ↓ returns pointer to buffer
Game draws at 8-bit indexed
    ↓
Platform_Surface_Unlock()
    ↓ converts palette
SDL texture update
    ↓
Platform_Present()
    ↓
SDL render
    ↓
Display
```

### Input Events
```
SDL Event Queue
    ↓
Platform_ProcessEvents()
    ↓ translates to game events
Platform_Keyboard_Check()
    ↓
Game logic
```

### Audio Playback
```
Game calls Platform_Sound_Play()
    ↓
Decode audio (if ADPCM)
    ↓
Queue to rodio source
    ↓
Audio thread plays
```

## Threading Model

- **Main Thread**: Game loop, graphics, input
- **Audio Thread**: Sound playback (rodio manages)
- **Network Thread**: enet background processing

The game code is not thread-safe. All game logic runs on main thread.

## Memory Model

- Game allocates via `Platform_Alloc` / `Platform_Free`
- These map to Rust's global allocator
- No manual memory management in Rust code

## Platform-Specific Code

macOS-specific functionality:
- `~/Library/Application Support/RedAlert/` for config/saves
- `~/Library/Caches/RedAlert/` for cache files
- Retina display detection via SDL2
- App bundle support via Info.plist

## Dependencies

| Crate | Purpose |
|-------|---------|
| sdl2 | Graphics, input, window management |
| rodio | Audio playback |
| enet | UDP networking |
| log | Logging facade |
| cbindgen | C header generation |
