# Command & Conquer Red Alert - macOS Port Master Plan

## Project Goal
Port the 1996 Command & Conquer Red Alert source code from Windows (Win32/DirectX 5) to macOS, creating a native application that runs on both Intel and Apple Silicon Macs.

## Guiding Principles

1. **Minimal Code Changes** - Wrap platform differences rather than rewriting game logic
2. **Abstraction Layers** - Create clean interfaces that hide platform details
3. **Incremental Progress** - Get something running quickly, then iterate
4. **Test Early, Test Often** - Validate each component before moving on
5. **Cross-Platform First** - Choices should enable future Linux/other ports

## Technology Stack

| Layer | Technology | Rationale |
|-------|------------|-----------|
| Build System | CMake | Cross-platform, modern, well-supported |
| Graphics | SDL2 | Mature, handles window/input/audio/threading |
| Audio | SDL2_mixer + OpenAL | SDL2_mixer for simplicity, OpenAL for 3D audio |
| Networking | enet | Reliable UDP, NAT traversal, replaces IPX |
| File System | PhysicsFS | Virtual FS, handles MIX archives |
| Threading | C++17 std::thread | Modern, portable, no external deps |

## Component Execution Order

The plans are numbered in the order they should be executed:

| # | Component | Dependency | Milestone |
|---|-----------|------------|-----------|
| 01 | Build System | None | Project compiles (with stubs) |
| 02 | Platform Abstraction | Build System | Clean API layer exists |
| 03 | Graphics System | Platform Layer | Window opens, renders test pattern |
| 04 | Input System | Graphics | Can click and type |
| 05 | Timing System | Platform Layer | Game loop runs at correct speed |
| 06 | Memory Management | Build System | Memory allocations work |
| 07 | File System | Memory | Can load MIX files and assets |
| 08 | Audio System | File System, Timing | Sound effects and music play |
| 09 | Assembly Rewrites | All above | Performance-critical code works |
| 10 | Networking | Platform Layer | Multiplayer functional |
| 11 | Cleanup & Polish | All above | Release-ready build |

## Milestone Definitions

### M1: "It Compiles" (Plans 01-02)
- CMake builds the project on macOS
- All Windows headers stubbed or replaced
- Executable links (even if it crashes immediately)

### M2: "It Draws" (Plans 03-05)
- Window opens with correct resolution
- Game renders main menu
- Mouse cursor visible and responsive
- Keyboard input works

### M3: "It Runs" (Plans 06-08)
- Single-player campaign loads
- Units move and animate
- Sound effects and music play
- Game is playable start to finish

### M4: "It's Fast" (Plan 09)
- Assembly routines rewritten in optimized C++
- Frame rate matches original game
- No performance regressions

### M5: "It's Complete" (Plans 10-11)
- Multiplayer works over internet
- All settings persist correctly
- Polish and bug fixes
- Ready for distribution

## Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Assembly complexity | High | High | Start with C++ fallbacks, optimize later |
| Missing game assets | Medium | High | Use original game data files |
| Networking protocol issues | High | Medium | Focus on single-player first |
| Compiler incompatibilities | Medium | Medium | Use C++17 standard features only |
| Endianness issues | Low | Medium | Test on both Intel and ARM |

## Directory Structure (Proposed)

```
Ronin_CnC/
├── project-plan/          # These planning documents
├── src/
│   ├── platform/          # New: Platform abstraction layer
│   │   ├── graphics/      # SDL2 graphics wrapper
│   │   ├── audio/         # Audio abstraction
│   │   ├── input/         # Input handling
│   │   ├── network/       # Networking layer
│   │   └── system/        # Timing, memory, files
│   └── game/              # Adapted game code (from CODE/)
├── deps/                  # Third-party dependencies
│   ├── SDL2/
│   ├── enet/
│   └── physfs/
├── assets/                # Game data files (not in repo)
└── build/                 # CMake build output
```

## Success Criteria

1. **Functional**: Game plays identically to original Windows version
2. **Performant**: 60 FPS on any Mac from 2015 onwards
3. **Native**: Runs natively on Apple Silicon (no Rosetta)
4. **Distributable**: Single .app bundle, no installer needed
5. **Maintainable**: Clean code that others can contribute to

## Next Steps

1. Read Plan 01 (Build System) and set up CMake
2. Create stub headers for Windows types
3. Get the project to compile (with empty implementations)
4. Proceed through plans in order

---

*Total Estimated Timeline: 4-6 months (single developer, part-time)*
*See individual plan files for detailed breakdowns*
