# Plan 11: Cleanup & Polish (Rust Implementation)

## Objective
Finalize the macOS port with bug fixes, performance optimization, proper packaging, and quality-of-life improvements.

## Scope

This phase covers:
1. Bug fixes from previous phases
2. Performance optimization
3. macOS-specific integration
4. App bundle packaging
5. Testing and validation
6. Documentation

## 11.1 Bug Fixes

### Common Issues to Address

**Graphics:**
- [ ] Palette flashing during transitions
- [ ] Sprite clipping at screen edges
- [ ] Screen tearing (if VSync issues)
- [ ] Mouse cursor alignment

**Audio:**
- [ ] Audio pops/clicks between sounds
- [ ] Music loop points
- [ ] Volume balance

**Input:**
- [ ] Key repeat handling
- [ ] Mouse acceleration
- [ ] Modifier key state

**General:**
- [ ] Save/load game issues
- [ ] Memory leaks (use Rust's memory safety!)
- [ ] File path issues on case-sensitive systems

### Bug Tracking Process

1. Create `bugs.md` to track known issues
2. Prioritize by severity (crash > gameplay > cosmetic)
3. Fix systematically
4. Verify fix doesn't introduce regressions

## 11.2 Performance Optimization

### Profiling

Use Instruments (Xcode) to profile:
```bash
# CPU profiling
instruments -t "Time Profiler" ./RedAlert.app

# Memory profiling
instruments -t "Allocations" ./RedAlert.app
```

For Rust-specific profiling:
```bash
# Using cargo-flamegraph
cargo install flamegraph
cargo flamegraph --bin redalert_platform

# Using perf on Linux (for cross-platform development)
perf record -g ./target/release/redalert_platform
perf report
```

### Optimization Targets

**Rendering Pipeline (Rust):**

File: `platform/src/graphics/optimize.rs`
```rust
//! Performance optimizations for graphics rendering

use std::arch::x86_64::*;

/// Optimized palette conversion using SIMD
#[cfg(target_arch = "x86_64")]
#[target_feature(enable = "sse2")]
pub unsafe fn convert_palette_simd(
    src: &[u8],
    dest: &mut [u32],
    palette: &[u32; 256],
) {
    // Process 16 pixels at a time when possible
    let chunks = src.len() / 16;

    for i in 0..chunks {
        let base = i * 16;

        // Load 16 indices
        for j in 0..16 {
            dest[base + j] = palette[src[base + j] as usize];
        }
    }

    // Handle remainder
    let remainder_start = chunks * 16;
    for i in remainder_start..src.len() {
        dest[i] = palette[src[i] as usize];
    }
}

/// ARM NEON optimized palette conversion
#[cfg(target_arch = "aarch64")]
pub fn convert_palette_neon(
    src: &[u8],
    dest: &mut [u32],
    palette: &[u32; 256],
) {
    // NEON implementation for Apple Silicon
    for (d, s) in dest.iter_mut().zip(src.iter()) {
        *d = palette[*s as usize];
    }
}

/// Auto-dispatch to best implementation
pub fn convert_palette_fast(
    src: &[u8],
    dest: &mut [u32],
    palette: &[u32; 256],
) {
    #[cfg(target_arch = "x86_64")]
    {
        if is_x86_feature_detected!("sse2") {
            unsafe { convert_palette_simd(src, dest, palette); }
            return;
        }
    }

    #[cfg(target_arch = "aarch64")]
    {
        convert_palette_neon(src, dest, palette);
        return;
    }

    // Scalar fallback
    for (d, s) in dest.iter_mut().zip(src.iter()) {
        *d = palette[*s as usize];
    }
}
```

**Memory Allocation:**

Rust's ownership system already helps with allocation efficiency, but we can add:
- Object pools for frequently allocated items
- Pre-allocated buffers for audio

```rust
//! Object pooling for performance

use std::sync::Mutex;
use std::collections::VecDeque;

/// Simple object pool for reusable allocations
pub struct ObjectPool<T> {
    available: Mutex<VecDeque<T>>,
    factory: fn() -> T,
}

impl<T> ObjectPool<T> {
    pub fn new(factory: fn() -> T, initial_size: usize) -> Self {
        let mut available = VecDeque::with_capacity(initial_size);
        for _ in 0..initial_size {
            available.push_back(factory());
        }
        Self {
            available: Mutex::new(available),
            factory,
        }
    }

    pub fn acquire(&self) -> T {
        let mut pool = self.available.lock().unwrap();
        pool.pop_front().unwrap_or_else(|| (self.factory)())
    }

    pub fn release(&self, item: T) {
        let mut pool = self.available.lock().unwrap();
        pool.push_back(item);
    }
}
```

### Performance Metrics

| Metric | Target | Minimum |
|--------|--------|---------|
| Frame Rate | 60 FPS | 30 FPS |
| Frame Time | 16ms | 33ms |
| Memory Usage | <256MB | <512MB |
| Load Time | <5s | <10s |

## 11.3 macOS Integration

### Retina Display Support

File: `platform/src/graphics/display.rs`
```rust
//! Display and DPI handling

use sdl2::video::Window;

/// Check if running on a Retina/HiDPI display
pub fn is_retina_display(window: &Window) -> bool {
    let (window_w, _) = window.size();
    let (drawable_w, _) = window.drawable_size();
    drawable_w > window_w
}

/// Get the scale factor for HiDPI displays
pub fn get_display_scale(window: &Window) -> (u32, u32) {
    let (window_w, window_h) = window.size();
    let (drawable_w, drawable_h) = window.drawable_size();

    (drawable_w / window_w, drawable_h / window_h)
}

// FFI exports
#[no_mangle]
pub extern "C" fn Platform_IsRetinaDisplay() -> i32 {
    crate::graphics::with_graphics(|state| {
        if is_retina_display(&state.window) { 1 } else { 0 }
    }).unwrap_or(0)
}

#[no_mangle]
pub unsafe extern "C" fn Platform_GetDisplayScale(
    scale_x: *mut i32,
    scale_y: *mut i32,
) {
    if let Ok((sx, sy)) = crate::graphics::with_graphics(|state| {
        get_display_scale(&state.window)
    }) {
        if !scale_x.is_null() { *scale_x = sx as i32; }
        if !scale_y.is_null() { *scale_y = sy as i32; }
    }
}
```

### Fullscreen Mode

```rust
//! Fullscreen handling

use sdl2::video::{Window, FullscreenType};

/// Toggle fullscreen mode
pub fn toggle_fullscreen(window: &mut Window) -> bool {
    let current = window.fullscreen_state();

    let new_state = match current {
        FullscreenType::Off => FullscreenType::Desktop,
        _ => FullscreenType::Off,
    };

    window.set_fullscreen(new_state).is_ok()
}

/// Set fullscreen mode explicitly
pub fn set_fullscreen(window: &mut Window, fullscreen: bool) -> bool {
    let state = if fullscreen {
        FullscreenType::Desktop
    } else {
        FullscreenType::Off
    };

    window.set_fullscreen(state).is_ok()
}

// FFI exports
#[no_mangle]
pub extern "C" fn Platform_ToggleFullscreen() -> i32 {
    crate::graphics::with_graphics_mut(|state| {
        if toggle_fullscreen(&mut state.window) { 0 } else { -1 }
    }).unwrap_or(-1)
}

#[no_mangle]
pub extern "C" fn Platform_SetFullscreen(fullscreen: i32) -> i32 {
    crate::graphics::with_graphics_mut(|state| {
        if set_fullscreen(&mut state.window, fullscreen != 0) { 0 } else { -1 }
    }).unwrap_or(-1)
}
```

### Application Lifecycle

File: `platform/src/app/lifecycle.rs`
```rust
//! macOS application lifecycle handling

use sdl2::event::Event;

/// Application state for lifecycle events
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum AppState {
    Active,
    Background,
    Terminating,
}

static APP_STATE: std::sync::atomic::AtomicU8 = std::sync::atomic::AtomicU8::new(0);

impl AppState {
    fn to_u8(self) -> u8 {
        match self {
            AppState::Active => 0,
            AppState::Background => 1,
            AppState::Terminating => 2,
        }
    }

    fn from_u8(v: u8) -> Self {
        match v {
            0 => AppState::Active,
            1 => AppState::Background,
            2 => AppState::Terminating,
            _ => AppState::Active,
        }
    }
}

/// Handle macOS application events
pub fn handle_app_event(event: &Event) -> Option<AppState> {
    use std::sync::atomic::Ordering;

    match event {
        Event::AppWillEnterBackground { .. } => {
            log::info!("App entering background");
            APP_STATE.store(AppState::Background.to_u8(), Ordering::SeqCst);
            Some(AppState::Background)
        }

        Event::AppDidEnterForeground { .. } => {
            log::info!("App entering foreground");
            APP_STATE.store(AppState::Active.to_u8(), Ordering::SeqCst);
            Some(AppState::Active)
        }

        Event::AppTerminating { .. } => {
            log::info!("App terminating");
            APP_STATE.store(AppState::Terminating.to_u8(), Ordering::SeqCst);
            Some(AppState::Terminating)
        }

        _ => None,
    }
}

/// Get current application state
pub fn get_app_state() -> AppState {
    use std::sync::atomic::Ordering;
    AppState::from_u8(APP_STATE.load(Ordering::SeqCst))
}

// FFI exports
#[no_mangle]
pub extern "C" fn Platform_GetAppState() -> i32 {
    get_app_state().to_u8() as i32
}

#[no_mangle]
pub extern "C" fn Platform_IsAppActive() -> i32 {
    if get_app_state() == AppState::Active { 1 } else { 0 }
}
```

## 11.4 App Bundle Packaging

### Bundle Structure

```
RedAlert.app/
├── Contents/
│   ├── Info.plist
│   ├── MacOS/
│   │   └── RedAlert          # Executable
│   ├── Resources/
│   │   ├── RedAlert.icns     # App icon
│   │   ├── data/             # Game data files
│   │   │   ├── REDALERT.MIX
│   │   │   ├── CONQUER.MIX
│   │   │   └── ...
│   │   └── en.lproj/
│   │       └── InfoPlist.strings
│   └── Frameworks/           # Bundled libraries
│       ├── libSDL2.dylib
│       └── ...
```

### Info.plist

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "...">
<plist version="1.0">
<dict>
    <key>CFBundleName</key>
    <string>Red Alert</string>
    <key>CFBundleDisplayName</key>
    <string>Command &amp; Conquer: Red Alert</string>
    <key>CFBundleIdentifier</key>
    <string>com.example.redalert</string>
    <key>CFBundleVersion</key>
    <string>1.0.0</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>CFBundleExecutable</key>
    <string>RedAlert</string>
    <key>CFBundleIconFile</key>
    <string>RedAlert</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.15</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>LSArchitecturePriority</key>
    <array>
        <string>arm64</string>
        <string>x86_64</string>
    </array>
</dict>
</plist>
```

### CMake Bundle Configuration

```cmake
# macOS app bundle settings
if(APPLE)
    set(MACOSX_BUNDLE TRUE)
    set(MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/platform/macos/Info.plist.in)
    set(MACOSX_BUNDLE_ICON_FILE RedAlert.icns)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.redalert")
    set(MACOSX_BUNDLE_BUNDLE_NAME "Red Alert")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0")

    # Copy resources
    set_source_files_properties(
        ${CMAKE_SOURCE_DIR}/resources/RedAlert.icns
        PROPERTIES MACOSX_PACKAGE_LOCATION "Resources"
    )

    # Copy game data
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/data/
            DESTINATION ${CMAKE_BINARY_DIR}/RedAlert.app/Contents/Resources/data)

    # Copy and fix library paths
    include(BundleUtilities)
    set(DIRS ${CMAKE_BINARY_DIR})
    fixup_bundle(${CMAKE_BINARY_DIR}/RedAlert.app "" "${DIRS}")
endif()
```

### Universal Binary Build Script

File: `scripts/build-universal.sh`
```bash
#!/bin/bash
set -e

# Build for both architectures
echo "Building for x86_64..."
cargo build --release --target x86_64-apple-darwin

echo "Building for arm64..."
cargo build --release --target aarch64-apple-darwin

# Create universal binary with lipo
echo "Creating universal binary..."
lipo -create \
    target/x86_64-apple-darwin/release/libredalert_platform.a \
    target/aarch64-apple-darwin/release/libredalert_platform.a \
    -output target/universal/libredalert_platform.a

echo "Universal binary created at target/universal/libredalert_platform.a"
```

### Code Signing (Optional)

```bash
# Sign the app bundle
codesign --force --deep --sign "Developer ID Application: Your Name" RedAlert.app

# Verify signature
codesign --verify --verbose RedAlert.app

# Notarize for distribution
xcrun notarytool submit RedAlert.zip \
    --apple-id "your@email.com" \
    --team-id "TEAMID" \
    --password "@keychain:AC_PASSWORD" \
    --wait

# Staple the notarization ticket
xcrun stapler staple RedAlert.app
```

## 11.5 Configuration & Saves

### Settings File Location

File: `platform/src/config/paths.rs`
```rust
//! Configuration and save file paths

use std::path::PathBuf;

/// Get the application support directory for Red Alert
pub fn get_config_path() -> PathBuf {
    let home = std::env::var("HOME").unwrap_or_else(|_| ".".to_string());
    let path = PathBuf::from(home)
        .join("Library")
        .join("Application Support")
        .join("RedAlert");

    // Create directory if needed
    std::fs::create_dir_all(&path).ok();

    path
}

/// Get the saves directory
pub fn get_saves_path() -> PathBuf {
    get_config_path().join("saves")
}

/// Get the settings file path
pub fn get_settings_path() -> PathBuf {
    get_config_path().join("settings.ini")
}

/// Get the log file path
pub fn get_log_path() -> PathBuf {
    get_config_path().join("game.log")
}

// FFI exports
#[no_mangle]
pub unsafe extern "C" fn Platform_GetConfigPath(
    buffer: *mut i8,
    buffer_size: i32,
) -> i32 {
    if buffer.is_null() || buffer_size <= 0 {
        return -1;
    }

    let path = get_config_path();
    let path_str = path.to_string_lossy();

    let bytes = path_str.as_bytes();
    let copy_len = std::cmp::min(bytes.len(), (buffer_size - 1) as usize);

    std::ptr::copy_nonoverlapping(
        bytes.as_ptr() as *const i8,
        buffer,
        copy_len,
    );

    // Null terminate
    *buffer.add(copy_len) = 0;

    copy_len as i32
}

#[no_mangle]
pub unsafe extern "C" fn Platform_GetSavesPath(
    buffer: *mut i8,
    buffer_size: i32,
) -> i32 {
    if buffer.is_null() || buffer_size <= 0 {
        return -1;
    }

    let path = get_saves_path();

    // Create saves directory if needed
    std::fs::create_dir_all(&path).ok();

    let path_str = path.to_string_lossy();
    let bytes = path_str.as_bytes();
    let copy_len = std::cmp::min(bytes.len(), (buffer_size - 1) as usize);

    std::ptr::copy_nonoverlapping(
        bytes.as_ptr() as *const i8,
        buffer,
        copy_len,
    );

    *buffer.add(copy_len) = 0;

    copy_len as i32
}
```

## 11.6 Logging

File: `platform/src/logging.rs`
```rust
//! Logging infrastructure

use std::fs::File;
use std::io::Write;
use std::sync::Mutex;
use once_cell::sync::Lazy;
use log::{Log, Metadata, Record, Level, LevelFilter};

static LOG_FILE: Lazy<Mutex<Option<File>>> = Lazy::new(|| Mutex::new(None));

struct GameLogger;

impl Log for GameLogger {
    fn enabled(&self, metadata: &Metadata) -> bool {
        metadata.level() <= Level::Debug
    }

    fn log(&self, record: &Record) {
        if self.enabled(record.metadata()) {
            let msg = format!(
                "[{}] {} - {}\n",
                record.level(),
                record.target(),
                record.args()
            );

            // Print to stderr
            eprint!("{}", msg);

            // Write to log file
            if let Ok(mut guard) = LOG_FILE.lock() {
                if let Some(ref mut file) = *guard {
                    let _ = file.write_all(msg.as_bytes());
                }
            }
        }
    }

    fn flush(&self) {
        if let Ok(mut guard) = LOG_FILE.lock() {
            if let Some(ref mut file) = *guard {
                let _ = file.flush();
            }
        }
    }
}

static LOGGER: GameLogger = GameLogger;

/// Initialize the logging system
pub fn init() {
    // Set up log file
    let log_path = crate::config::paths::get_log_path();
    if let Ok(file) = File::create(&log_path) {
        *LOG_FILE.lock().unwrap() = Some(file);
    }

    // Set up logger
    log::set_logger(&LOGGER)
        .map(|()| log::set_max_level(LevelFilter::Debug))
        .ok();

    log::info!("Logging initialized to {:?}", log_path);
}

// FFI exports
#[no_mangle]
pub extern "C" fn Platform_Log_Init() {
    init();
}

#[no_mangle]
pub unsafe extern "C" fn Platform_Log(level: i32, message: *const i8) {
    if message.is_null() {
        return;
    }

    let msg = std::ffi::CStr::from_ptr(message)
        .to_string_lossy();

    match level {
        0 => log::error!("{}", msg),
        1 => log::warn!("{}", msg),
        2 => log::info!("{}", msg),
        3 => log::debug!("{}", msg),
        _ => log::trace!("{}", msg),
    }
}
```

## 11.7 Testing Checklist

### Functionality Tests

**Single Player:**
- [ ] Allied campaign plays start to finish
- [ ] Soviet campaign plays start to finish
- [ ] Skirmish mode works
- [ ] Save/Load works at all points
- [ ] All difficulty levels

**Multiplayer:**
- [ ] LAN game discovery
- [ ] Host/Join works
- [ ] Game synchronization
- [ ] All maps playable
- [ ] Various player counts

**Options:**
- [ ] Video settings apply
- [ ] Audio settings apply
- [ ] Control settings work
- [ ] Settings persist between sessions

### Compatibility Tests

**Hardware:**
- [ ] M1/M2/M3 Mac (Apple Silicon)
- [ ] Intel Mac (2015+)
- [ ] Various display resolutions
- [ ] External monitors

**macOS Versions:**
- [ ] macOS 14 (Sonoma)
- [ ] macOS 13 (Ventura)
- [ ] macOS 12 (Monterey)
- [ ] macOS 11 (Big Sur)
- [ ] macOS 10.15 (Catalina) - minimum target

## 11.8 Documentation

### README.md Updates

Add macOS-specific information:
```markdown
## macOS Installation

1. Download RedAlert.dmg
2. Drag Red Alert to Applications folder
3. Double-click to run
4. If prompted about unidentified developer:
   - Right-click the app, select Open
   - Click Open in the dialog

### Game Data

Place your original game data files in:
`~/Library/Application Support/RedAlert/data/`

Required files:
- REDALERT.MIX
- CONQUER.MIX
- LOCAL.MIX
- (etc.)
```

### Build Instructions

```markdown
## Building on macOS

### Requirements
- Xcode Command Line Tools
- CMake 3.20+
- Rust toolchain (via rustup)

### Install Dependencies
```bash
# Install Rust
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh

# Add targets for universal binary
rustup target add x86_64-apple-darwin aarch64-apple-darwin

# Install CMake
brew install cmake ninja
```

### Build Steps
```bash
# Configure
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build

# Run
./build/RedAlert.app/Contents/MacOS/RedAlert
```
```

## Tasks Breakdown

### Phase 1: Bug Fixes (5 days)
- [ ] Collect all known bugs
- [ ] Prioritize by severity
- [ ] Fix critical bugs
- [ ] Fix major bugs
- [ ] Fix minor bugs

### Phase 2: Performance (3 days)
- [ ] Profile with Instruments
- [ ] Identify bottlenecks
- [ ] Optimize critical paths (SIMD where applicable)
- [ ] Verify improvements

### Phase 3: macOS Integration (2 days)
- [ ] Retina display support
- [ ] Fullscreen handling
- [ ] Application lifecycle events
- [ ] Proper path handling

### Phase 4: Packaging (2 days)
- [ ] Create app bundle
- [ ] Universal binary (Intel + Apple Silicon)
- [ ] Include resources
- [ ] Test installation

### Phase 5: Testing (3 days)
- [ ] Full playthrough testing
- [ ] Compatibility testing
- [ ] Edge case testing
- [ ] Final bug fixes

### Phase 6: Documentation (1 day)
- [ ] Update README
- [ ] Build instructions
- [ ] Known issues

## Acceptance Criteria

- [ ] Game fully playable single-player
- [ ] Game playable multiplayer
- [ ] No critical or major bugs
- [ ] Performance targets met (60 FPS)
- [ ] Proper macOS app bundle
- [ ] Works on Intel and Apple Silicon
- [ ] Documentation complete

## Estimated Duration
**12-16 days**

## Dependencies
- All previous plans completed
- Game functional end-to-end

## Project Completion

Upon completing this phase:
1. Tag release in Git: `git tag -a v1.0.0 -m "Initial macOS release"`
2. Create release notes
3. Build final distribution DMG
4. Test on fresh macOS installation
