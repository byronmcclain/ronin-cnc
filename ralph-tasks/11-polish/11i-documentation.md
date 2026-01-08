# Task 11i: Documentation and README

## Dependencies
- All previous Phase 11 tasks should be complete
- Project builds and runs successfully

## Context
This task creates comprehensive documentation for the Red Alert macOS port. Good documentation enables:
- Users to install and run the game
- Developers to understand the architecture
- Contributors to build and test the project
- Future maintainers to understand decisions

Documentation includes:
- Updated README.md with macOS focus
- Build instructions
- Architecture overview
- Platform layer API reference
- Known issues and troubleshooting

## Objective
Create documentation that:
1. Updates README.md for the macOS port
2. Documents build requirements and process
3. Explains the platform layer architecture
4. Provides troubleshooting guidance
5. Documents the FFI interface

## Deliverables
- [ ] Updated `README.md` - Main project documentation
- [ ] `docs/BUILD.md` - Detailed build instructions
- [ ] `docs/ARCHITECTURE.md` - Platform layer architecture
- [ ] `docs/PLATFORM-API.md` - FFI reference
- [ ] `CHANGELOG.md` - Version history
- [ ] Verification passes

## Technical Background

### Documentation Structure
```
README.md              <- Quick start, overview
CHANGELOG.md           <- Version history
CLAUDE.md              <- AI assistant context (existing)
docs/
├── BUILD.md           <- Detailed build steps
├── ARCHITECTURE.md    <- System design
├── PLATFORM-API.md    <- FFI reference
└── TROUBLESHOOTING.md <- Common issues
```

### Audience
1. **End Users**: Want to run the game
2. **Developers**: Want to modify/build
3. **Contributors**: Want to add features
4. **AI Assistants**: Want context (CLAUDE.md)

## Files to Create/Modify

### README.md (REPLACE)
```markdown
# Command & Conquer: Red Alert - macOS Port

A modern macOS port of the classic 1996 real-time strategy game by Westwood Studios.

![macOS](https://img.shields.io/badge/macOS-10.15%2B-blue)
![Architecture](https://img.shields.io/badge/arch-Intel%20%7C%20Apple%20Silicon-green)
![License](https://img.shields.io/badge/license-GPL--3.0-blue)

## Features

- **Native macOS Support**: Runs natively on Intel and Apple Silicon Macs
- **Modern Platform Layer**: Built with Rust for safety and performance
- **SDL2 Graphics**: Hardware-accelerated rendering
- **Retina Display Support**: Crisp graphics on HiDPI displays
- **Cross-Platform Audio**: Modern audio using rodio
- **Reliable Networking**: UDP networking via enet

## Quick Start

### Requirements

- macOS 10.15 (Catalina) or later
- Original Red Alert game data files

### Installation

1. Download the latest release from [Releases](../../releases)
2. Drag `Red Alert.app` to your Applications folder
3. Copy your game data files to:
   ```
   ~/Library/Application Support/RedAlert/data/
   ```
4. Double-click to run

### Required Game Files

You'll need the following files from an original Red Alert installation:
- `REDALERT.MIX`
- `CONQUER.MIX`
- `LOCAL.MIX`
- `SCORES.MIX`
- `MAIN.MIX`

## Building from Source

See [docs/BUILD.md](docs/BUILD.md) for detailed build instructions.

### Quick Build

```bash
# Clone the repository
git clone https://github.com/your-org/redalert-macos.git
cd redalert-macos

# Install dependencies
brew install cmake ninja

# Build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run
open build/RedAlert.app
```

### Build Requirements

- Xcode Command Line Tools
- CMake 3.20+
- Ninja build system
- Rust toolchain (installed automatically via Corrosion)

## Project Structure

```
├── CODE/           # Original C++ game source
├── WIN32LIB/       # Original Westwood library
├── platform/       # Modern Rust platform layer
│   └── src/
│       ├── graphics/   # SDL2 rendering
│       ├── audio/      # Audio playback
│       ├── input/      # Keyboard/mouse input
│       ├── network/    # enet networking
│       └── ...
├── include/        # Generated C headers
├── scripts/        # Build and utility scripts
└── docs/           # Documentation
```

## Documentation

- [Build Instructions](docs/BUILD.md)
- [Architecture Overview](docs/ARCHITECTURE.md)
- [Platform API Reference](docs/PLATFORM-API.md)
- [Troubleshooting](docs/TROUBLESHOOTING.md)

## Controls

| Key | Action |
|-----|--------|
| Arrow Keys | Scroll map |
| Mouse | Select units, issue commands |
| Ctrl+# | Assign control group |
| # | Select control group |
| S | Stop selected units |
| G | Guard mode |
| Esc | Cancel/Menu |

## Known Limitations

- Multiplayer requires all players to use this port
- Some original music formats may not play
- Map editor not yet ported

## Contributing

This is a preservation project. Bug fixes and compatibility improvements are welcome.

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## License

The source code is released under the GPL-3.0 license. See [LICENSE](LICENSE) for details.

Original game content and assets remain property of Electronic Arts.

## Acknowledgments

- **Westwood Studios**: Original game development
- **Electronic Arts**: Open-source release of the code
- The OpenRA and CnCNet communities for keeping C&C alive

## History

Red Alert was released in 1996 and became one of the most beloved RTS games ever made. In 2020, EA released the source code under GPL-3.0 to support the Steam Workshop release of the Command & Conquer Remastered Collection.

This port adapts the original DOS/Windows code to run on modern macOS, replacing obsolete APIs (DirectDraw, Winsock, IPX) with cross-platform alternatives.
```

### docs/BUILD.md (NEW)
```markdown
# Build Instructions

This document provides detailed instructions for building Red Alert on macOS.

## Prerequisites

### System Requirements

- macOS 10.15 (Catalina) or later
- At least 4GB RAM
- 500MB free disk space for build

### Required Tools

1. **Xcode Command Line Tools**
   ```bash
   xcode-select --install
   ```

2. **CMake 3.20+**
   ```bash
   brew install cmake
   ```

3. **Ninja** (optional but recommended)
   ```bash
   brew install ninja
   ```

4. **Rust toolchain** (installed automatically, or manually):
   ```bash
   curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
   ```

## Build Steps

### Standard Build

```bash
# Configure
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build

# Output is at build/RedAlert.app
```

### Debug Build

```bash
cmake -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
```

### Universal Binary (Intel + Apple Silicon)

```bash
# Ensure both Rust targets are installed
rustup target add x86_64-apple-darwin aarch64-apple-darwin

# Build universal binary
./scripts/build-universal.sh
```

## Build Targets

| Target | Description |
|--------|-------------|
| RedAlert | Main game executable |
| TestPlatform | Integration tests |
| TestAudio | Audio subsystem tests |
| generate_headers | Generate platform.h from Rust |

## Build Options

| Option | Default | Description |
|--------|---------|-------------|
| CMAKE_BUILD_TYPE | Release | Build type (Debug/Release) |
| CMAKE_OSX_ARCHITECTURES | native | Target architectures |
| CMAKE_OSX_DEPLOYMENT_TARGET | 10.15 | Minimum macOS version |

## Troubleshooting Build Issues

### "xcrun: error: invalid active developer path"
```bash
xcode-select --install
```

### "cargo not found"
```bash
source "$HOME/.cargo/env"
```

### Rust compilation fails
```bash
# Update Rust
rustup update stable
```

### CMake can't find Ninja
```bash
brew install ninja
```

## Running Tests

```bash
# All tests
./scripts/run-tests.sh

# Quick smoke tests
./scripts/run-tests.sh --quick

# Rust tests only
cargo test --manifest-path platform/Cargo.toml --release
```

## Development Workflow

1. Make changes to Rust code in `platform/src/`
2. Run `cargo build --manifest-path platform/Cargo.toml`
3. Regenerate headers: `cargo build --manifest-path platform/Cargo.toml --features cbindgen-run`
4. Rebuild C++ code: `cmake --build build`
5. Test: `./build/RedAlert.app/Contents/MacOS/RedAlert`

## Cleaning Build

```bash
# Clean CMake build
rm -rf build

# Clean Rust build
cargo clean --manifest-path platform/Cargo.toml

# Clean everything
rm -rf build target
```
```

### docs/ARCHITECTURE.md (NEW)
```markdown
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
```

### docs/PLATFORM-API.md (NEW)
```markdown
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

### Platform_Log
```c
void Platform_Log(int32_t level, const char* message);
```
Log a message.

**Levels:** 0=Error, 1=Warn, 2=Info, 3=Debug, 4=Trace

### Platform_Log_SetLevel
```c
void Platform_Log_SetLevel(int32_t level);
```
Set the minimum log level.

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

**Returns:** 0=Active, 1=Background, 2=Terminating

### Platform_IsAppActive
```c
int32_t Platform_IsAppActive(void);
```
Check if app is in foreground.

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
```

### CHANGELOG.md (NEW)
```markdown
# Changelog

All notable changes to the Red Alert macOS port.

## [1.0.0] - 2024-XX-XX

### Added
- Initial macOS release
- Native support for Intel and Apple Silicon Macs
- SDL2-based graphics rendering
- Modern audio playback via rodio
- enet-based UDP networking
- Retina display support
- Fullscreen mode
- macOS app bundle

### Changed
- Replaced DirectDraw with SDL2
- Replaced Winsock with enet
- Replaced Windows timers with platform timers
- Replaced IPX networking (obsolete) with modern UDP

### Fixed
- Various path handling for case-sensitive filesystems
- Audio format compatibility

### Known Issues
- Some original music files may not play
- Map editor not yet ported
- Multiplayer requires matching port versions

## [Unreleased]

### Planned
- Linux support
- Improved music format support
- Save game cloud sync
```

## Verification Command
```bash
ralph-tasks/verify/check-documentation.sh
```

## Verification Script
Create `ralph-tasks/verify/check-documentation.sh`:
```bash
#!/bin/bash
# Verification script for documentation

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Documentation ==="

cd "$ROOT_DIR"

# Step 1: Check main README exists and has content
echo "Step 1: Checking README.md..."
if [ ! -f "README.md" ]; then
    echo "VERIFY_FAILED: README.md not found"
    exit 1
fi
if [ $(wc -l < README.md) -lt 50 ]; then
    echo "VERIFY_FAILED: README.md too short"
    exit 1
fi
echo "  README.md exists and has content"

# Step 2: Check docs directory
echo "Step 2: Checking docs directory..."
mkdir -p docs
for doc in BUILD.md ARCHITECTURE.md PLATFORM-API.md; do
    if [ ! -f "docs/$doc" ]; then
        echo "VERIFY_FAILED: docs/$doc not found"
        exit 1
    fi
done
echo "  All documentation files exist"

# Step 3: Check CHANGELOG
echo "Step 3: Checking CHANGELOG.md..."
if [ ! -f "CHANGELOG.md" ]; then
    echo "VERIFY_FAILED: CHANGELOG.md not found"
    exit 1
fi
echo "  CHANGELOG.md exists"

# Step 4: Verify README has key sections
echo "Step 4: Verifying README content..."
for section in "Quick Start" "Building" "Requirements" "License"; do
    if ! grep -qi "$section" README.md; then
        echo "VERIFY_FAILED: README missing '$section' section"
        exit 1
    fi
    echo "  Found '$section' section"
done

# Step 5: Check no broken internal links (basic)
echo "Step 5: Checking documentation links..."
for doc in README.md docs/BUILD.md; do
    if grep -o '\[.*\](docs/[^)]*\.md)' "$doc" | while read link; do
        target=$(echo "$link" | grep -o 'docs/[^)]*\.md')
        if [ ! -f "$target" ]; then
            echo "  WARNING: Broken link to $target in $doc"
        fi
    done; then
        true  # Continue even with warnings
    fi
done
echo "  Link check complete"

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Update README.md with macOS-focused content
2. Create docs/ directory
3. Create docs/BUILD.md
4. Create docs/ARCHITECTURE.md
5. Create docs/PLATFORM-API.md
6. Create CHANGELOG.md
7. Run verification script

## Success Criteria
- README.md is comprehensive and accurate
- All docs/*.md files exist
- Documentation covers build, architecture, and API
- No broken internal links
- Verification script exits with 0

## Documentation Guidelines

1. **Be Concise**: Users want answers, not essays
2. **Use Examples**: Show, don't just tell
3. **Keep Updated**: Documentation must match code
4. **Target Audience**: Write for your users, not yourself

## Completion Promise
When verification passes, output:
```
<promise>TASK_11I_COMPLETE</promise>
```

## Escape Hatch
If stuck after 8 iterations:
- Document what's blocking in `ralph-tasks/blocked/11i.md`
- List attempted approaches
- Output: `<promise>TASK_11I_BLOCKED</promise>`

## Max Iterations
8
