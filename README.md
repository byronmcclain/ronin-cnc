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
brew install cmake

# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run
open build/RedAlert.app
```

### Build Requirements

- Xcode Command Line Tools
- CMake 3.20+
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

The source code is released under the GPL-3.0 license. See [LICENSE.md](LICENSE.md) for details.

Original game content and assets remain property of Electronic Arts.

## Acknowledgments

- **Westwood Studios**: Original game development
- **Electronic Arts**: Open-source release of the code
- The OpenRA and CnCNet communities for keeping C&C alive

## History

Red Alert was released in 1996 and became one of the most beloved RTS games ever made. In 2020, EA released the source code under GPL-3.0 to support the Steam Workshop release of the Command & Conquer Remastered Collection.

This port adapts the original DOS/Windows code to run on modern macOS, replacing obsolete APIs (DirectDraw, Winsock, IPX) with cross-platform alternatives.
