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
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build

# Output is at build/RedAlert.app
```

### Debug Build

```bash
cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug
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

## Creating App Bundle

```bash
# Build the project first
cmake --build build

# Create the bundle
./scripts/create-bundle.sh build .

# Bundle is at ./RedAlert.app
```
