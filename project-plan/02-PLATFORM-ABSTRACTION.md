# Plan 02: Platform Abstraction Layer (Rust)

## Objective
Create a clean Rust abstraction layer with C FFI exports that isolates all platform-specific code, allowing the original C++ game logic to call into Rust for all platform services.

## Design Philosophy

The abstraction layer follows these principles:
1. **Rust traits** define internal abstractions
2. **C FFI exports** expose functionality to game code
3. **cbindgen** auto-generates headers - never hand-write them
4. **Panic safety** - catch panics at FFI boundary
5. **Opaque pointers** - C sees handles, Rust owns data

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     GAME CODE (Original C++)                 │
│                  (CODE/*.CPP, WIN32LIB/*.CPP)                │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼  extern "C" FFI
┌─────────────────────────────────────────────────────────────┐
│                    platform.h (AUTO-GENERATED)               │
│                        via cbindgen                          │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                   RUST PLATFORM CRATE                        │
│                  redalert-platform/src/                      │
├─────────────────────────────────────────────────────────────┤
│    ffi/     │   graphics/  │   audio/   │   input/          │
│  (exports)  │   (sdl2-rs)  │  (rodio)   │  (sdl2-rs)        │
├─────────────────────────────────────────────────────────────┤
│    timer/   │    files/    │  network/  │   memory/         │
│  (std::time)│  (std::fs)   │   (enet)   │  (allocator)      │
└─────────────────────────────────────────────────────────────┘
```

## Rust Crate Structure

```
platform/
├── Cargo.toml
├── cbindgen.toml
├── build.rs
└── src/
    ├── lib.rs              # Crate root, module declarations
    ├── error.rs            # Error types
    ├── ffi/
    │   ├── mod.rs          # FFI module root
    │   ├── graphics.rs     # Graphics FFI exports
    │   ├── audio.rs        # Audio FFI exports
    │   ├── input.rs        # Input FFI exports
    │   ├── timer.rs        # Timer FFI exports
    │   ├── files.rs        # File system FFI exports
    │   └── network.rs      # Network FFI exports
    ├── graphics/
    │   ├── mod.rs
    │   ├── surface.rs      # Surface implementation
    │   └── palette.rs      # Palette management
    ├── audio/
    │   ├── mod.rs
    │   ├── mixer.rs        # Audio mixing
    │   └── adpcm.rs        # ADPCM decoder
    ├── input/
    │   ├── mod.rs
    │   ├── keyboard.rs     # Keyboard handling
    │   └── mouse.rs        # Mouse handling
    ├── timer/
    │   └── mod.rs          # Timing functions
    ├── files/
    │   ├── mod.rs
    │   └── path.rs         # Path normalization
    └── network/
        ├── mod.rs
        └── host.rs         # Network host/peer
```

## Interface Definitions

### 2.1 Core Platform (Rust)

File: `platform/src/lib.rs`
```rust
//! Red Alert Platform Layer
//!
//! This crate provides platform abstraction for the Red Alert macOS port.
//! All public items are exported via C FFI for use by the original C++ game code.

#![allow(clippy::missing_safety_doc)]

pub mod error;
pub mod graphics;
pub mod audio;
pub mod input;
pub mod timer;
pub mod files;
pub mod network;
pub mod memory;

// FFI exports
pub mod ffi;

use std::sync::OnceLock;
use log::{info, error, warn};

/// Global platform state
static PLATFORM: OnceLock<Platform> = OnceLock::new();

/// Platform configuration passed from C
#[repr(C)]
pub struct PlatformConfig {
    pub window_title: *const std::ffi::c_char,
    pub window_width: i32,
    pub window_height: i32,
    pub fullscreen: bool,
    pub vsync: bool,
}

struct Platform {
    sdl_context: sdl2::Sdl,
    // Other subsystems initialized here
}

impl Platform {
    fn new(config: &PlatformConfig) -> Result<Self, error::PlatformError> {
        let sdl = sdl2::init().map_err(error::PlatformError::Sdl)?;

        info!("Platform initialized");

        Ok(Self {
            sdl_context: sdl,
        })
    }
}

/// Result type for platform operations
pub type PlatformResult<T> = Result<T, error::PlatformError>;
```

### 2.2 Error Handling (Rust)

File: `platform/src/error.rs`
```rust
//! Platform error types

use thiserror::Error;
use std::sync::Mutex;

/// Platform error enum
#[derive(Debug, Error)]
pub enum PlatformError {
    #[error("SDL error: {0}")]
    Sdl(String),

    #[error("Audio error: {0}")]
    Audio(String),

    #[error("Graphics error: {0}")]
    Graphics(String),

    #[error("IO error: {0}")]
    Io(#[from] std::io::Error),

    #[error("Network error: {0}")]
    Network(String),

    #[error("Already initialized")]
    AlreadyInitialized,

    #[error("Not initialized")]
    NotInitialized,

    #[error("Invalid parameter: {0}")]
    InvalidParameter(String),
}

/// Thread-local error message for FFI
static LAST_ERROR: Mutex<Option<String>> = Mutex::new(None);

/// Set the last error message (for FFI)
pub fn set_error(msg: impl Into<String>) {
    if let Ok(mut guard) = LAST_ERROR.lock() {
        *guard = Some(msg.into());
    }
}

/// Get the last error message (for FFI)
pub fn get_error() -> Option<String> {
    LAST_ERROR.lock().ok().and_then(|guard| guard.clone())
}

/// Clear the last error
pub fn clear_error() {
    if let Ok(mut guard) = LAST_ERROR.lock() {
        *guard = None;
    }
}

/// Convert Result to C error code (0 = success, negative = error)
pub fn result_to_code<T>(result: crate::PlatformResult<T>) -> i32 {
    match result {
        Ok(_) => 0,
        Err(e) => {
            set_error(e.to_string());
            -1
        }
    }
}

/// Catch panics at FFI boundary
pub fn catch_panic<F, T>(f: F) -> i32
where
    F: FnOnce() -> crate::PlatformResult<T> + std::panic::UnwindSafe,
{
    match std::panic::catch_unwind(f) {
        Ok(result) => result_to_code(result),
        Err(_) => {
            set_error("Panic caught at FFI boundary");
            -999
        }
    }
}

/// Catch panics and return Option
pub fn catch_panic_option<F, T>(f: F) -> Option<T>
where
    F: FnOnce() -> Option<T> + std::panic::UnwindSafe,
{
    match std::panic::catch_unwind(f) {
        Ok(result) => result,
        Err(_) => {
            set_error("Panic caught at FFI boundary");
            None
        }
    }
}
```

### 2.3 FFI Core Exports

File: `platform/src/ffi/mod.rs`
```rust
//! FFI exports for C/C++ game code
//!
//! All functions in this module are exported with C linkage.

pub mod graphics;
pub mod audio;
pub mod input;
pub mod timer;
pub mod files;
pub mod network;

use crate::error::{catch_panic, get_error, clear_error, set_error};
use crate::{Platform, PlatformConfig, PLATFORM};
use std::ffi::{c_char, CStr};

/// Log level enum for C
#[repr(C)]
pub enum LogLevel {
    Debug = 0,
    Info = 1,
    Warn = 2,
    Error = 3,
}

// =============================================================================
// Platform Initialization
// =============================================================================

/// Initialize the platform layer
///
/// # Safety
/// `config` must be a valid pointer to a PlatformConfig struct
#[no_mangle]
pub unsafe extern "C" fn Platform_Init(config: *const PlatformConfig) -> i32 {
    catch_panic(|| {
        if PLATFORM.get().is_some() {
            return Err(crate::error::PlatformError::AlreadyInitialized);
        }

        // Initialize logging
        env_logger::Builder::from_default_env()
            .filter_level(log::LevelFilter::Info)
            .init();

        let config = if config.is_null() {
            PlatformConfig {
                window_title: std::ptr::null(),
                window_width: 640,
                window_height: 400,
                fullscreen: false,
                vsync: true,
            }
        } else {
            std::ptr::read(config)
        };

        let platform = Platform::new(&config)?;
        PLATFORM.set(platform).map_err(|_| {
            crate::error::PlatformError::AlreadyInitialized
        })?;

        Ok(())
    })
}

/// Shutdown the platform layer
#[no_mangle]
pub extern "C" fn Platform_Shutdown() {
    // OnceLock doesn't support removal, so we just log
    log::info!("Platform shutdown requested");
}

/// Process OS events (call each frame)
#[no_mangle]
pub extern "C" fn Platform_PumpEvents() {
    // Implemented in graphics module with SDL event pump
    crate::input::pump_events();
}

/// Check if quit was requested
#[no_mangle]
pub extern "C" fn Platform_ShouldQuit() -> bool {
    crate::input::should_quit()
}

// =============================================================================
// Error Handling
// =============================================================================

/// Get the last error message
///
/// # Safety
/// Returns a pointer to a static string buffer. Valid until next error.
#[no_mangle]
pub extern "C" fn Platform_GetError() -> *const c_char {
    static mut ERROR_BUF: [u8; 512] = [0; 512];

    if let Some(msg) = get_error() {
        unsafe {
            let bytes = msg.as_bytes();
            let len = bytes.len().min(ERROR_BUF.len() - 1);
            ERROR_BUF[..len].copy_from_slice(&bytes[..len]);
            ERROR_BUF[len] = 0;
            ERROR_BUF.as_ptr() as *const c_char
        }
    } else {
        std::ptr::null()
    }
}

/// Set an error message
///
/// # Safety
/// `msg` must be a valid null-terminated C string
#[no_mangle]
pub unsafe extern "C" fn Platform_SetError(msg: *const c_char) {
    if !msg.is_null() {
        if let Ok(s) = CStr::from_ptr(msg).to_str() {
            set_error(s);
        }
    }
}

/// Clear the last error
#[no_mangle]
pub extern "C" fn Platform_ClearError() {
    clear_error();
}

// =============================================================================
// Logging
// =============================================================================

/// Log a message
///
/// # Safety
/// `msg` must be a valid null-terminated C string
#[no_mangle]
pub unsafe extern "C" fn Platform_Log(level: LogLevel, msg: *const c_char) {
    if msg.is_null() {
        return;
    }

    if let Ok(s) = CStr::from_ptr(msg).to_str() {
        match level {
            LogLevel::Debug => log::debug!("{}", s),
            LogLevel::Info => log::info!("{}", s),
            LogLevel::Warn => log::warn!("{}", s),
            LogLevel::Error => log::error!("{}", s),
        }
    }
}
```

### 2.4 Graphics Trait

File: `platform/src/graphics/mod.rs`
```rust
//! Graphics subsystem

pub mod surface;
pub mod palette;

use crate::PlatformResult;
use crate::error::PlatformError;
use sdl2::video::Window;
use sdl2::render::Canvas;
use std::sync::Mutex;
use once_cell::sync::Lazy;

/// Display mode configuration
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct DisplayMode {
    pub width: i32,
    pub height: i32,
    pub bits_per_pixel: i32,
}

/// Global graphics state
static GRAPHICS: Lazy<Mutex<Option<GraphicsState>>> = Lazy::new(|| Mutex::new(None));

pub struct GraphicsState {
    pub sdl_video: sdl2::VideoSubsystem,
    pub window: Window,
    pub canvas: Canvas<Window>,
    pub display_mode: DisplayMode,
    pub palette: [PaletteEntry; 256],
    pub palette_rgba: [u32; 256],
    pub back_buffer: Vec<u8>,
}

/// RGB palette entry
#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct PaletteEntry {
    pub r: u8,
    pub g: u8,
    pub b: u8,
}

impl GraphicsState {
    pub fn new(sdl: &sdl2::Sdl, mode: DisplayMode) -> PlatformResult<Self> {
        let video = sdl.video().map_err(PlatformError::Sdl)?;

        let window = video
            .window("Command & Conquer: Red Alert",
                    (mode.width * 2) as u32,
                    (mode.height * 2) as u32)
            .position_centered()
            .resizable()
            .build()
            .map_err(|e| PlatformError::Graphics(e.to_string()))?;

        let canvas = window
            .into_canvas()
            .accelerated()
            .present_vsync()
            .build()
            .map_err(|e| PlatformError::Graphics(e.to_string()))?;

        let buffer_size = (mode.width * mode.height) as usize;

        Ok(Self {
            sdl_video: video,
            window: canvas.window().clone(),
            canvas,
            display_mode: mode,
            palette: [PaletteEntry::default(); 256],
            palette_rgba: [0xFF000000; 256],
            back_buffer: vec![0; buffer_size],
        })
    }

    pub fn rebuild_palette_lut(&mut self) {
        for i in 0..256 {
            let p = &self.palette[i];
            self.palette_rgba[i] =
                0xFF000000 | ((p.r as u32) << 16) | ((p.g as u32) << 8) | (p.b as u32);
        }
    }
}

/// Initialize graphics subsystem
pub fn init(sdl: &sdl2::Sdl) -> PlatformResult<()> {
    let mode = DisplayMode {
        width: 640,
        height: 400,
        bits_per_pixel: 8,
    };

    let state = GraphicsState::new(sdl, mode)?;

    if let Ok(mut guard) = GRAPHICS.lock() {
        *guard = Some(state);
    }

    Ok(())
}

/// Access graphics state
pub fn with_graphics<F, R>(f: F) -> Option<R>
where
    F: FnOnce(&mut GraphicsState) -> R,
{
    GRAPHICS.lock().ok().and_then(|mut guard| {
        guard.as_mut().map(f)
    })
}
```

### 2.5 Input Trait

File: `platform/src/input/mod.rs`
```rust
//! Input subsystem

pub mod keyboard;
pub mod mouse;

use sdl2::event::Event;
use sdl2::keyboard::Keycode;
use std::collections::VecDeque;
use std::sync::Mutex;
use once_cell::sync::Lazy;

/// Key codes (matching Windows VK_* values for compatibility)
#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum KeyCode {
    None = 0,
    Escape = 0x1B,
    Return = 0x0D,
    Space = 0x20,
    Up = 0x26,
    Down = 0x28,
    Left = 0x25,
    Right = 0x27,
    Shift = 0x10,
    Control = 0x11,
    Alt = 0x12,
}

/// Mouse button
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub enum MouseButton {
    Left = 0,
    Right = 1,
    Middle = 2,
}

/// Key event for queue
#[derive(Clone, Copy)]
pub struct KeyEvent {
    pub key: KeyCode,
    pub released: bool,
}

/// Global input state
static INPUT: Lazy<Mutex<InputState>> = Lazy::new(|| Mutex::new(InputState::new()));

pub struct InputState {
    pub keys_current: [bool; 256],
    pub keys_previous: [bool; 256],
    pub key_queue: VecDeque<KeyEvent>,

    pub mouse_x: i32,
    pub mouse_y: i32,
    pub mouse_buttons: [bool; 3],
    pub mouse_buttons_prev: [bool; 3],
    pub mouse_wheel: i32,

    pub shift_down: bool,
    pub ctrl_down: bool,
    pub alt_down: bool,

    pub should_quit: bool,
}

impl InputState {
    pub fn new() -> Self {
        Self {
            keys_current: [false; 256],
            keys_previous: [false; 256],
            key_queue: VecDeque::new(),
            mouse_x: 0,
            mouse_y: 0,
            mouse_buttons: [false; 3],
            mouse_buttons_prev: [false; 3],
            mouse_wheel: 0,
            shift_down: false,
            ctrl_down: false,
            alt_down: false,
            should_quit: false,
        }
    }

    pub fn begin_frame(&mut self) {
        self.keys_previous = self.keys_current;
        self.mouse_buttons_prev = self.mouse_buttons;
        self.mouse_wheel = 0;
    }

    pub fn handle_event(&mut self, event: &Event) {
        match event {
            Event::Quit { .. } => {
                self.should_quit = true;
            }
            Event::KeyDown { keycode: Some(key), repeat: false, .. } => {
                if let Some(code) = sdl_to_keycode(*key) {
                    let idx = code as usize;
                    if idx < 256 {
                        self.keys_current[idx] = true;
                        self.key_queue.push_back(KeyEvent { key: code, released: false });
                    }
                }
            }
            Event::KeyUp { keycode: Some(key), .. } => {
                if let Some(code) = sdl_to_keycode(*key) {
                    let idx = code as usize;
                    if idx < 256 {
                        self.keys_current[idx] = false;
                        self.key_queue.push_back(KeyEvent { key: code, released: true });
                    }
                }
            }
            Event::MouseMotion { x, y, .. } => {
                self.mouse_x = *x;
                self.mouse_y = *y;
            }
            Event::MouseButtonDown { mouse_btn, .. } => {
                let btn = sdl_to_mousebutton(*mouse_btn);
                self.mouse_buttons[btn as usize] = true;
            }
            Event::MouseButtonUp { mouse_btn, .. } => {
                let btn = sdl_to_mousebutton(*mouse_btn);
                self.mouse_buttons[btn as usize] = false;
            }
            Event::MouseWheel { y, .. } => {
                self.mouse_wheel = *y;
            }
            _ => {}
        }
    }
}

fn sdl_to_keycode(key: Keycode) -> Option<KeyCode> {
    match key {
        Keycode::Escape => Some(KeyCode::Escape),
        Keycode::Return => Some(KeyCode::Return),
        Keycode::Space => Some(KeyCode::Space),
        Keycode::Up => Some(KeyCode::Up),
        Keycode::Down => Some(KeyCode::Down),
        Keycode::Left => Some(KeyCode::Left),
        Keycode::Right => Some(KeyCode::Right),
        Keycode::LShift | Keycode::RShift => Some(KeyCode::Shift),
        Keycode::LCtrl | Keycode::RCtrl => Some(KeyCode::Control),
        Keycode::LAlt | Keycode::RAlt => Some(KeyCode::Alt),
        _ => None,
    }
}

fn sdl_to_mousebutton(btn: sdl2::mouse::MouseButton) -> MouseButton {
    match btn {
        sdl2::mouse::MouseButton::Left => MouseButton::Left,
        sdl2::mouse::MouseButton::Right => MouseButton::Right,
        sdl2::mouse::MouseButton::Middle => MouseButton::Middle,
        _ => MouseButton::Left,
    }
}

/// Pump SDL events
pub fn pump_events() {
    // This would be called from the graphics module which owns the event pump
}

/// Check if quit was requested
pub fn should_quit() -> bool {
    INPUT.lock().map(|s| s.should_quit).unwrap_or(false)
}
```

### 2.6 Timer Trait

File: `platform/src/timer/mod.rs`
```rust
//! Timer subsystem using std::time

use std::time::{Duration, Instant};
use std::sync::Mutex;
use once_cell::sync::Lazy;

static START_TIME: Lazy<Instant> = Lazy::new(Instant::now);
static TIMERS: Lazy<Mutex<Vec<Option<PeriodicTimer>>>> = Lazy::new(|| Mutex::new(Vec::new()));

pub struct PeriodicTimer {
    pub interval: Duration,
    pub last_tick: Instant,
    pub callback: Box<dyn FnMut() + Send>,
    pub active: bool,
}

/// Get milliseconds since platform init
pub fn get_ticks() -> u32 {
    START_TIME.elapsed().as_millis() as u32
}

/// Get high-precision time in seconds
pub fn get_time() -> f64 {
    START_TIME.elapsed().as_secs_f64()
}

/// Sleep for milliseconds
pub fn delay(ms: u32) {
    std::thread::sleep(Duration::from_millis(ms as u64));
}

/// Get performance counter
pub fn get_performance_counter() -> u64 {
    START_TIME.elapsed().as_nanos() as u64
}

/// Get performance frequency (nanoseconds)
pub fn get_performance_frequency() -> u64 {
    1_000_000_000 // 1 GHz (nanosecond resolution)
}
```

### 2.7 File System Trait

File: `platform/src/files/mod.rs`
```rust
//! File system abstraction

pub mod path;

use std::fs::{self, File};
use std::io::{Read, Write, Seek, SeekFrom};
use std::path::{Path, PathBuf};
use crate::PlatformResult;
use crate::error::PlatformError;

/// File handle wrapper
pub struct PlatformFile {
    file: File,
    path: PathBuf,
    size: i64,
}

/// File open mode
#[repr(C)]
pub enum FileMode {
    Read = 1,
    Write = 2,
    Append = 4,
}

/// Seek origin
#[repr(C)]
pub enum SeekOrigin {
    Start = 0,
    Current = 1,
    End = 2,
}

impl PlatformFile {
    pub fn open(path: &str, mode: FileMode) -> PlatformResult<Self> {
        let normalized = path::normalize(path);
        let path_buf = PathBuf::from(&normalized);

        let file = match mode {
            FileMode::Read => File::open(&path_buf)?,
            FileMode::Write => File::create(&path_buf)?,
            FileMode::Append => File::options().append(true).open(&path_buf)?,
        };

        let size = file.metadata().map(|m| m.len() as i64).unwrap_or(0);

        Ok(Self {
            file,
            path: path_buf,
            size,
        })
    }

    pub fn read(&mut self, buf: &mut [u8]) -> PlatformResult<usize> {
        Ok(self.file.read(buf)?)
    }

    pub fn write(&mut self, buf: &[u8]) -> PlatformResult<usize> {
        Ok(self.file.write(buf)?)
    }

    pub fn seek(&mut self, offset: i64, origin: SeekOrigin) -> PlatformResult<u64> {
        let pos = match origin {
            SeekOrigin::Start => SeekFrom::Start(offset as u64),
            SeekOrigin::Current => SeekFrom::Current(offset),
            SeekOrigin::End => SeekFrom::End(offset),
        };
        Ok(self.file.seek(pos)?)
    }

    pub fn size(&self) -> i64 {
        self.size
    }
}

/// Get base path (executable directory)
pub fn get_base_path() -> String {
    std::env::current_exe()
        .ok()
        .and_then(|p| p.parent().map(|p| p.to_string_lossy().into_owned()))
        .unwrap_or_else(|| ".".to_string())
}

/// Get preferences path (for saves/config)
pub fn get_pref_path(app_name: &str) -> String {
    if let Some(home) = dirs::home_dir() {
        let path = home
            .join("Library")
            .join("Application Support")
            .join(app_name);

        // Create if doesn't exist
        let _ = fs::create_dir_all(&path);

        path.to_string_lossy().into_owned()
    } else {
        ".".to_string()
    }
}
```

## Generated Header Preview

The `include/platform.h` generated by cbindgen will look like:

```c
/*
 * Red Alert Platform Layer
 * Auto-generated by cbindgen - DO NOT EDIT
 */

#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque types */
typedef struct PlatformSurface PlatformSurface;
typedef struct PlatformSound PlatformSound;
typedef struct PlatformFile PlatformFile;
typedef struct PlatformHost PlatformHost;
typedef struct PlatformPeer PlatformPeer;

/* Enums */
typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARN = 2,
    LOG_ERROR = 3,
} LogLevel;

/* Structs */
typedef struct {
    const char *window_title;
    int32_t window_width;
    int32_t window_height;
    bool fullscreen;
    bool vsync;
} PlatformConfig;

/* Core functions */
int32_t Platform_Init(const PlatformConfig *config);
void Platform_Shutdown(void);
void Platform_PumpEvents(void);
bool Platform_ShouldQuit(void);
const char *Platform_GetError(void);
void Platform_ClearError(void);
void Platform_Log(LogLevel level, const char *msg);

/* Graphics functions */
int32_t Platform_Graphics_Init(void);
void Platform_Graphics_Shutdown(void);
/* ... more functions ... */

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_H */
```

## Implementation Tasks

### Task 1: Core Platform Module (2 days)
- [ ] Create `platform/Cargo.toml` with all dependencies
- [ ] Create `platform/cbindgen.toml` configuration
- [ ] Create `platform/build.rs` for header generation
- [ ] Implement `lib.rs` with initialization
- [ ] Implement `error.rs` with FFI error handling
- [ ] Verify `cargo build` succeeds

### Task 2: FFI Foundation (2 days)
- [ ] Create `ffi/mod.rs` with core exports
- [ ] Implement panic catching wrappers
- [ ] Generate initial `platform.h`
- [ ] Test calling Rust from C++

### Task 3: Module Stubs (1 day)
- [ ] Create stub implementations for all modules
- [ ] Ensure all FFI functions are exported
- [ ] Verify header generation includes all exports
- [ ] Test linking with game code

### Task 4: Integration Testing (1 day)
- [ ] Compile game with platform.h
- [ ] Fix any missing mappings
- [ ] Document FFI patterns

## File Structure

```
platform/
├── Cargo.toml
├── cbindgen.toml
├── build.rs
└── src/
    ├── lib.rs
    ├── error.rs
    ├── ffi/
    │   ├── mod.rs
    │   ├── graphics.rs
    │   ├── audio.rs
    │   ├── input.rs
    │   ├── timer.rs
    │   ├── files.rs
    │   └── network.rs
    ├── graphics/
    │   ├── mod.rs
    │   ├── surface.rs
    │   └── palette.rs
    ├── audio/
    │   ├── mod.rs
    │   ├── mixer.rs
    │   └── adpcm.rs
    ├── input/
    │   ├── mod.rs
    │   ├── keyboard.rs
    │   └── mouse.rs
    ├── timer/
    │   └── mod.rs
    ├── files/
    │   ├── mod.rs
    │   └── path.rs
    ├── memory/
    │   └── mod.rs
    └── network/
        ├── mod.rs
        └── host.rs

include/
└── platform.h              # AUTO-GENERATED by cbindgen

include/compat/
├── windows.h               # Windows type stubs
├── ddraw_wrapper.h         # DirectDraw → Platform mapping
├── dsound_wrapper.h        # DirectSound → Platform mapping
└── winsock_wrapper.h       # Winsock → Platform mapping
```

## Acceptance Criteria

- [ ] All Rust modules compile
- [ ] cbindgen generates complete `platform.h`
- [ ] FFI functions callable from C++
- [ ] Panic safety verified
- [ ] No undefined behavior at FFI boundary
- [ ] Clear separation between Rust internals and C interface

## Estimated Duration
**5-7 days**

## Dependencies
- Plan 01 (Build System) completed
- Rust toolchain installed
- SDL2 development libraries

## Next Plan
Once abstraction layer is in place, proceed to **Plan 03: Graphics System**
