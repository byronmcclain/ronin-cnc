# Phase 2 Overview: Game Integration

## Executive Summary

**Platform layer is 100% complete.** Phases 01-11 implemented all core systems:
- Graphics (SDL2), Input, Audio (rodio), Networking (enet)
- Timer, Memory, File I/O, Configuration, Logging
- ~16,400 lines of production-quality Rust code

**Remaining work: Bridge original game code to platform layer.**

This document outlines Phases 12-19, the path from working platform to playable game.

---

## Current State

### What's Working
| Component | Status | Notes |
|-----------|--------|-------|
| Window/Display | 640x400 @ 60fps | SDL2, 8-bit indexed color |
| Palette System | Full 256-entry | RGB→ARGB conversion |
| Keyboard Input | Complete | Windows VK_* compatible |
| Mouse Input | Complete | Position, buttons, wheel |
| Audio Playback | PCM + ADPCM | rodio backend |
| Networking | Host + Client | enet UDP reliable/unreliable |
| File I/O | Cross-platform | Path normalization |
| Timer System | High-precision | Performance counters |

### What's Missing
| Component | Status | Blocker |
|-----------|--------|---------|
| MIX File Loading | 0% | Need parser implementation |
| Game Code Compile | 0% | Watcom/Win32 dependencies |
| Main Game Loop | 0% | Needs game code |
| Asset Rendering | 0% | Needs MIX + shapes |
| UI System | 0% | Needs gadget porting |

---

## Phase Roadmap

```
Phase 12: Asset System (MIX files, palettes, shapes)
    ↓
Phase 13: Compatibility Layer (Win32/DirectX stubs)
    ↓
Phase 14: Game Code Porting (critical C++ modules)
    ↓
Phase 15: Game Loop Integration (main loop, init)
    ↓
Phase 16: Graphics Integration (rendering pipeline)
    ↓
Phase 17: Input Integration (keyboard/mouse handlers)
    ↓
Phase 18: Audio Integration (sound effects, music)
    ↓
Phase 19: Testing & Polish (full game verification)
```

---

## Priority Matrix

### Critical Path (Must Complete First)
1. **MIX File Parser** - All assets are in MIX archives
2. **Palette Loading** - Required for any rendering
3. **Shape/Frame Loading** - Unit/building graphics
4. **Minimal Game Loop** - Something that renders

### Secondary Priority
5. **Map/Cell System** - Terrain rendering
6. **UI Gadgets** - Buttons, dialogs
7. **Sound Effects** - Audio feedback
8. **Network Integration** - Multiplayer

### Polish Phase
9. **Music Playback** - Background scores
10. **Save/Load** - Game persistence
11. **Performance** - Optimization
12. **macOS Integration** - App bundle, icons

---

## Effort Estimates

| Phase | Estimated Hours | Complexity |
|-------|-----------------|------------|
| 12 - Asset System | 20-30 | Medium |
| 13 - Compatibility | 15-25 | Medium |
| 14 - Game Porting | 30-50 | High |
| 15 - Game Loop | 15-25 | Medium |
| 16 - Graphics | 20-30 | Medium |
| 17 - Input | 10-15 | Low |
| 18 - Audio | 10-15 | Low |
| 19 - Testing | 20-30 | Medium |
| **Total** | **140-220** | |

---

## Success Criteria

### Milestone 1: Static Rendering
- [ ] Load REDALERT.MIX successfully
- [ ] Display title screen palette
- [ ] Render a single shape/sprite

### Milestone 2: Map Display
- [ ] Load theater MIX (TEMPERAT.MIX)
- [ ] Parse map file
- [ ] Render terrain tiles

### Milestone 3: Interactive Demo
- [ ] Mouse cursor visible
- [ ] Click detection working
- [ ] Basic UI responsive

### Milestone 4: Playable Game
- [ ] Full game loop running
- [ ] Units selectable and movable
- [ ] Buildings constructible
- [ ] Win/lose conditions

---

## File Organization

New files to create:
```
src/
├── game/
│   ├── mix_file.cpp      # MIX archive parser
│   ├── palette.cpp       # Palette management
│   ├── shape.cpp         # Shape/sprite loading
│   ├── map.cpp           # Map/cell system
│   └── game_loop.cpp     # Main game loop
├── compat/
│   ├── win32_stubs.h     # Windows API stubs
│   ├── directx_stubs.h   # DirectX stubs
│   └── watcom_compat.h   # Watcom compiler compat
└── main.cpp              # Entry point (replace test)
```

---

## Dependencies

### Game Data Required
Located in `gamedata/`:
- `REDALERT.MIX` - Core game data (24MB)
- `CD1/MAIN.MIX` - Allied campaign (434MB)
- `CD2/MAIN.MIX` - Soviet campaign (477MB)
- Theater files: `barren.mix`, `desert.mix`, etc.

### Build Requirements
- Xcode Command Line Tools (clang)
- CMake 3.20+
- Rust toolchain (for platform layer)

---

## Risk Assessment

| Risk | Impact | Mitigation |
|------|--------|------------|
| MIX format undocumented | High | Use existing open-source parsers (OpenRA) |
| C++ code too entangled | High | Start with minimal subset, expand |
| Assembly code critical | Medium | Already replaced in Phase 09 |
| Missing assets | Low | Have complete CD images |
| Performance issues | Low | Platform layer already optimized |

---

## Next Steps

1. **Read Phase 13** - Start with MIX file system
2. **Review CODE/MIXFILE.H** - Understand original format
3. **Test asset extraction** - Verify MIX files are valid
4. **Implement minimal loader** - Get first asset displayed
