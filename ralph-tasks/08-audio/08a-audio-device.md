# Task 08a: Audio Device Setup

## Dependencies
- Task 07d must be complete (File System FFI exports)
- Phase 02 Platform Abstraction complete
- rodio crate available

## Context
The audio system replaces DirectSound with Rust's rodio crate. This task establishes the audio device initialization, shutdown, and global audio state management. The original codebase uses DirectSound in `WIN32LIB/AUDIO/SOUNDIO.CPP`.

## Objective
Create the audio module with device initialization using rodio's `OutputStream`, global audio state management, and master volume control. Export basic init/shutdown via FFI.

## Deliverables
- [ ] `platform/src/audio/mod.rs` - Audio state and initialization
- [ ] `platform/Cargo.toml` - Add rodio dependency
- [ ] FFI exports for audio init/shutdown in `platform/src/ffi/mod.rs`
- [ ] C header updated with audio functions
- [ ] Verification passes

## Files to Create/Modify

### platform/Cargo.toml (MODIFY)
Add under `[dependencies]`:
```toml
rodio = { version = "0.19", default-features = false, features = ["symphonia-all"] }
```

### platform/src/audio/mod.rs (NEW)
```rust
//! Audio subsystem using rodio

pub mod adpcm;

use crate::error::PlatformError;
use crate::PlatformResult;
use rodio::{OutputStream, OutputStreamHandle, Sink};
use std::sync::{Arc, Mutex};
use std::collections::HashMap;
use once_cell::sync::Lazy;

/// Audio configuration
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct AudioConfig {
    pub sample_rate: i32,
    pub channels: i32,
    pub bits_per_sample: i32,
    pub buffer_size: i32,
}

impl Default for AudioConfig {
    fn default() -> Self {
        Self {
            sample_rate: 22050,
            channels: 2,
            bits_per_sample: 16,
            buffer_size: 1024,
        }
    }
}

/// Sound handle type
pub type SoundHandle = i32;
pub const INVALID_SOUND_HANDLE: SoundHandle = -1;

/// Playing sound handle
pub type PlayHandle = i32;
pub const INVALID_PLAY_HANDLE: PlayHandle = -1;

/// Sound data stored in memory
pub struct SoundData {
    pub samples: Vec<i16>,
    pub sample_rate: u32,
    pub channels: u16,
}

/// Global audio state
pub struct AudioState {
    #[allow(dead_code)]
    stream: OutputStream,
    pub stream_handle: OutputStreamHandle,
    pub sounds: HashMap<SoundHandle, Arc<SoundData>>,
    pub playing: HashMap<PlayHandle, Sink>,
    pub next_sound_handle: SoundHandle,
    pub next_play_handle: PlayHandle,
    pub master_volume: f32,
}

/// Global audio state singleton
pub static AUDIO: Lazy<Mutex<Option<AudioState>>> = Lazy::new(|| Mutex::new(None));

// =============================================================================
// Initialization
// =============================================================================

/// Initialize audio subsystem
pub fn init(config: &AudioConfig) -> PlatformResult<()> {
    let (stream, stream_handle) = OutputStream::try_default()
        .map_err(|e| PlatformError::Audio(e.to_string()))?;

    let state = AudioState {
        stream,
        stream_handle,
        sounds: HashMap::new(),
        playing: HashMap::new(),
        next_sound_handle: 1,
        next_play_handle: 1,
        master_volume: 1.0,
    };

    if let Ok(mut guard) = AUDIO.lock() {
        *guard = Some(state);
    }

    log::info!("Audio initialized: {}Hz, {} channels, {}-bit",
               config.sample_rate, config.channels, config.bits_per_sample);

    Ok(())
}

/// Shutdown audio subsystem
pub fn shutdown() {
    if let Ok(mut guard) = AUDIO.lock() {
        if let Some(state) = guard.as_mut() {
            // Stop all playing sounds
            for (_, sink) in state.playing.drain() {
                sink.stop();
            }
            // Clear all loaded sounds
            state.sounds.clear();
        }
        *guard = None;
    }
    log::info!("Audio shutdown");
}

/// Check if audio is initialized
pub fn is_initialized() -> bool {
    if let Ok(guard) = AUDIO.lock() {
        return guard.is_some();
    }
    false
}

// =============================================================================
// Master Volume
// =============================================================================

/// Set master volume (0.0 to 1.0)
pub fn set_master_volume(volume: f32) {
    if let Ok(mut guard) = AUDIO.lock() {
        if let Some(state) = guard.as_mut() {
            state.master_volume = volume.clamp(0.0, 1.0);
            // Update all playing sounds
            for (_, sink) in state.playing.iter() {
                sink.set_volume(state.master_volume);
            }
        }
    }
}

/// Get master volume
pub fn get_master_volume() -> f32 {
    if let Ok(guard) = AUDIO.lock() {
        if let Some(state) = guard.as_ref() {
            return state.master_volume;
        }
    }
    1.0
}
```

### platform/src/audio/adpcm.rs (NEW - Placeholder)
```rust
//! IMA ADPCM decoder - placeholder for task 08c

/// Placeholder ADPCM decode function
pub fn decode_adpcm(_input: &[u8]) -> Vec<i16> {
    Vec::new()
}
```

### platform/src/lib.rs (MODIFY)
Add after existing module declarations:
```rust
pub mod audio;
```

### platform/src/ffi/mod.rs (MODIFY)
Add audio FFI exports:
```rust
use crate::audio::{self, AudioConfig};

// =============================================================================
// Audio FFI
// =============================================================================

/// Initialize audio subsystem
#[no_mangle]
pub extern "C" fn Platform_Audio_Init(config: *const AudioConfig) -> i32 {
    let config = if config.is_null() {
        AudioConfig::default()
    } else {
        unsafe { *config }
    };

    match audio::init(&config) {
        Ok(()) => 0,
        Err(e) => {
            log::error!("Audio init failed: {}", e);
            -1
        }
    }
}

/// Shutdown audio subsystem
#[no_mangle]
pub extern "C" fn Platform_Audio_Shutdown() {
    audio::shutdown();
}

/// Check if audio is initialized
#[no_mangle]
pub extern "C" fn Platform_Audio_IsInitialized() -> bool {
    audio::is_initialized()
}

/// Set master volume (0.0 to 1.0)
#[no_mangle]
pub extern "C" fn Platform_Audio_SetMasterVolume(volume: f32) {
    audio::set_master_volume(volume);
}

/// Get master volume
#[no_mangle]
pub extern "C" fn Platform_Audio_GetMasterVolume() -> f32 {
    audio::get_master_volume()
}
```

### platform/src/error.rs (MODIFY)
Add Audio variant to PlatformError if not present:
```rust
// In the PlatformError enum, ensure this variant exists:
Audio(String),
```

## Verification Command
```bash
cd /Users/ooartist/Src/Ronin_CnC && \
cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1 && \
grep -q "Platform_Audio_Init" include/platform.h && \
grep -q "Platform_Audio_Shutdown" include/platform.h && \
grep -q "Platform_Audio_SetMasterVolume" include/platform.h && \
echo "VERIFY_SUCCESS"
```

## Implementation Steps
1. Add rodio dependency to `platform/Cargo.toml`
2. Create `platform/src/audio/mod.rs` with AudioState and init/shutdown
3. Create placeholder `platform/src/audio/adpcm.rs`
4. Add `pub mod audio;` to `platform/src/lib.rs`
5. Add Audio variant to PlatformError if needed
6. Add audio FFI exports to `platform/src/ffi/mod.rs`
7. Build with cbindgen to generate header
8. Verify header contains audio functions
9. Run verification command

## Completion Promise
When verification passes, output:
```
<promise>TASK_08A_COMPLETE</promise>
```

## Escape Hatch
If stuck after 12 iterations:
- Document what's blocking in `ralph-tasks/blocked/08a.md`
- List attempted approaches
- Output: `<promise>TASK_08A_BLOCKED</promise>`

## Max Iterations
12
