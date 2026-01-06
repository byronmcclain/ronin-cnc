# Command & Conquer Red Alert - macOS Port Master Plan

## Project Goal
Port the 1996 Command & Conquer Red Alert source code from Windows (Win32/DirectX 5) to macOS, creating a native application that runs on both Intel and Apple Silicon Macs.

## Language Strategy

**Hybrid C++/Rust Approach:**
- **Original Game Code (C/C++)**: Untouched - CODE/, WIN32LIB/, etc.
- **New Platform Layer (Rust)**: All new code written in Rust
- **FFI Boundary**: Clean C interface between game and platform

```
┌─────────────────────────────────────────────────────────────┐
│                  ORIGINAL GAME CODE (C/C++)                  │
│              (CODE/*.CPP, WIN32LIB/*.CPP - untouched)       │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼  extern "C" FFI boundary
┌─────────────────────────────────────────────────────────────┐
│                  PLATFORM LAYER (Rust)                       │
│              redalert-platform crate                         │
├─────────────────────────────────────────────────────────────┤
│  graphics  │  audio  │  input  │  network  │  files  │ time │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼  Rust crates
┌─────────────────────────────────────────────────────────────┐
│    sdl2    │    rodio    │    enet    │    std::*          │
└─────────────────────────────────────────────────────────────┘
```

## Why Rust?

| Benefit | Impact on This Project |
|---------|------------------------|
| **Memory Safety** | No buffer overflows in platform layer |
| **Fearless Concurrency** | Safe audio/network threading |
| **Modern Tooling** | Cargo > CMake for dependencies |
| **Excellent FFI** | cbindgen auto-generates C headers |
| **Cross-Platform** | First-class macOS/Linux/Windows |
| **Performance** | Zero-cost abstractions, SIMD support |

## Technology Stack

| Layer | Technology | Rationale |
|-------|------------|-----------|
| Build System | CMake + Cargo | CMake for C++, Cargo for Rust |
| FFI Generation | cbindgen | Auto-generate C headers from Rust |
| CMake-Cargo Bridge | corrosion | Seamless CMake/Cargo integration |
| Graphics | sdl2 crate | Mature, safe SDL2 bindings |
| Audio | rodio | Pure Rust, modern audio |
| Networking | enet crate | Reliable UDP, NAT-friendly |
| File System | std::fs, std::path | Excellent path handling |
| Timing | std::time | High-precision, portable |

## Rust Crates Used

```toml
[dependencies]
sdl2 = { version = "0.36", features = ["bundled"] }
rodio = "0.17"
enet = "0.3"
log = "0.4"
env_logger = "0.10"
thiserror = "1.0"
libc = "0.2"

[build-dependencies]
cbindgen = "0.26"
```

## Component Execution Order

| # | Component | Language | Milestone |
|---|-----------|----------|-----------|
| 01 | Build System | CMake+Cargo | Mixed build works |
| 02 | Platform Abstraction | Rust | FFI headers generated |
| 03 | Graphics System | Rust | Window opens, renders |
| 04 | Input System | Rust | Keyboard/mouse works |
| 05 | Timing System | Rust | Game loop runs correctly |
| 06 | Memory Management | Rust | Allocations work across FFI |
| 07 | File System | Rust | MIX files load |
| 08 | Audio System | Rust | Sound effects play |
| 09 | Assembly Rewrites | Rust | Blitting/scaling works |
| 10 | Networking | Rust | Multiplayer functional |
| 11 | Cleanup & Polish | Both | Release-ready |

## Milestone Definitions

### M1: "It Compiles" (Plans 01-02)
- CMake + Cargo build succeeds
- Rust platform crate compiles
- cbindgen generates C headers
- C++ game code links against Rust static library

### M2: "It Draws" (Plans 03-05)
- Window opens with SDL2 (via Rust)
- Game renders main menu
- Mouse cursor visible and responsive
- Keyboard input works

### M3: "It Runs" (Plans 06-08)
- Single-player campaign loads
- Units move and animate
- Sound effects and music play
- Game is playable start to finish

### M4: "It's Fast" (Plan 09)
- Assembly routines rewritten in Rust
- SIMD optimizations where beneficial
- Frame rate matches original game

### M5: "It's Complete" (Plans 10-11)
- Multiplayer works over internet
- All settings persist correctly
- Polish and bug fixes
- macOS app bundle ready

## Directory Structure

```
Ronin_CnC/
├── project-plan/           # Planning documents
├── platform/               # NEW: Rust platform crate
│   ├── Cargo.toml
│   ├── cbindgen.toml
│   ├── src/
│   │   ├── lib.rs          # Crate root, FFI exports
│   │   ├── graphics.rs     # SDL2 graphics
│   │   ├── audio.rs        # rodio audio
│   │   ├── input.rs        # Input handling
│   │   ├── timer.rs        # Timing
│   │   ├── files.rs        # File system
│   │   ├── network.rs      # enet networking
│   │   └── ffi/            # C FFI layer
│   │       ├── mod.rs
│   │       ├── graphics.rs
│   │       └── ...
│   └── build.rs            # cbindgen header generation
├── include/
│   └── platform.h          # AUTO-GENERATED by cbindgen
├── CODE/                   # Original game code (C++)
├── WIN32LIB/               # Original library (C++)
└── build/                  # CMake build output
```

## FFI Design Principles

1. **Rust Owns, C Borrows**: Rust allocates memory, provides explicit free functions
2. **Opaque Pointers**: C sees `typedef struct PlatformGraphics PlatformGraphics;`
3. **Error Codes**: Return integers, not Result<T, E> across FFI
4. **No Panics**: Catch panics at FFI boundary, convert to error codes
5. **cbindgen**: Auto-generate headers, never hand-write them

## Success Criteria

1. **Functional**: Game plays identically to original Windows version
2. **Performant**: 60 FPS on any Mac from 2015 onwards
3. **Native**: Runs natively on Apple Silicon (no Rosetta)
4. **Safe**: No undefined behavior in new Rust code
5. **Maintainable**: Clean separation, well-documented FFI

## Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| FFI complexity | Medium | Medium | Use cbindgen, test thoroughly |
| Rust learning curve | Low | Low | Rust is well-documented |
| Build system complexity | Medium | Medium | Use corrosion, proven approach |
| Performance regression | Low | Medium | Benchmark critical paths |
| SDL2 crate issues | Low | Low | Mature, well-maintained crate |

---

*Total Estimated Timeline: 4-6 months (single developer)*
*See individual plan files for detailed breakdowns*
