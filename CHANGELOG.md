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
- Configuration paths in ~/Library/Application Support/RedAlert/
- Comprehensive logging system
- Application lifecycle handling
- Performance monitoring (FPS, frame timing)
- Integration test infrastructure

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
