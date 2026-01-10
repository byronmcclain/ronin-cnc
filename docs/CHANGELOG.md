# Changelog

All notable changes to Red Alert macOS Port will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Initial macOS port from original source code

### Changed
- Graphics system ported to Metal
- Audio system ported to Core Audio
- Input handling ported to AppKit

### Fixed
- (List bug fixes here)

## [1.0.0] - YYYY-MM-DD

### Added
- Native macOS application bundle
- Universal binary (Intel + Apple Silicon)
- Retina display support
- Full keyboard and mouse support
- Original game assets support (requires original game files)
- Comprehensive test suite (unit, integration, visual, gameplay, performance)
- Performance profiling and optimization tools
- Bug tracking and regression testing system

### Changed
- DirectX graphics replaced with Metal
- DirectSound audio replaced with Core Audio
- Win32 window handling replaced with AppKit/SDL
- Sprite batching for efficient rendering
- Dirty rectangle tracking for optimized updates

### Technical Details
- Modern C++17 codebase
- Rust platform layer for cross-platform support
- CMake-based build system
- Comprehensive test framework

### Known Issues
- See [KNOWN_ISSUES.md](KNOWN_ISSUES.md)

---

## Version History

- 1.0.0 - Initial macOS release
