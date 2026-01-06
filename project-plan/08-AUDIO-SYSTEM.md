# Plan 08: Audio System (Rust + rodio)

## Objective
Replace DirectSound with Rust's rodio crate, supporting the game's 8-bit/16-bit PCM audio and IMA ADPCM compressed audio formats.

## Current State Analysis

### DirectSound Usage in Codebase

**Primary Files:**
- `WIN32LIB/AUDIO/SOUNDIO.CPP` (2208 lines)
- `WIN32LIB/AUDIO/SOUNDINT.CPP`
- `CODE/AUDIO.CPP`, `CODE/AUDIO.H`
- `CODE/ADPCM.CPP` - ADPCM codec

**Audio Formats:**
- 8-bit unsigned PCM
- 16-bit signed PCM
- Sample rates: 11025, 22050 Hz
- IMA ADPCM compressed (AUD files)

## Rust Implementation with rodio

### 8.1 Audio State

File: `platform/src/audio/mod.rs`
```rust
//! Audio subsystem using rodio

pub mod mixer;
pub mod adpcm;

use crate::error::PlatformError;
use crate::PlatformResult;
use rodio::{OutputStream, OutputStreamHandle, Sink, Source};
use std::sync::{Arc, Mutex};
use std::collections::HashMap;
use std::io::Cursor;
use once_cell::sync::Lazy;

/// Audio configuration
#[repr(C)]
#[derive(Clone, Copy)]
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

/// Global audio state
static AUDIO: Lazy<Mutex<Option<AudioState>>> = Lazy::new(|| Mutex::new(None));

struct AudioState {
    _stream: OutputStream,
    stream_handle: OutputStreamHandle,
    sounds: HashMap<SoundHandle, Arc<SoundData>>,
    playing: HashMap<PlayHandle, Sink>,
    next_sound_handle: SoundHandle,
    next_play_handle: PlayHandle,
    master_volume: f32,
}

/// Sound handle type
pub type SoundHandle = i32;
pub const INVALID_SOUND_HANDLE: SoundHandle = -1;

/// Playing sound handle
pub type PlayHandle = i32;
pub const INVALID_PLAY_HANDLE: PlayHandle = -1;

/// Sound data
struct SoundData {
    samples: Vec<i16>,
    sample_rate: u32,
    channels: u16,
}

// =============================================================================
// Initialization
// =============================================================================

/// Initialize audio subsystem
pub fn init(config: &AudioConfig) -> PlatformResult<()> {
    let (stream, stream_handle) = OutputStream::try_default()
        .map_err(|e| PlatformError::Audio(e.to_string()))?;

    let state = AudioState {
        _stream: stream,
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
        *guard = None;
    }
    log::info!("Audio shutdown");
}

// =============================================================================
// Sound Management
// =============================================================================

/// Create a sound from PCM data
pub fn create_sound(
    data: &[u8],
    sample_rate: u32,
    channels: u16,
    bits_per_sample: u16,
) -> SoundHandle {
    let samples = convert_to_i16(data, bits_per_sample);

    let sound = Arc::new(SoundData {
        samples,
        sample_rate,
        channels,
    });

    if let Ok(mut guard) = AUDIO.lock() {
        if let Some(state) = guard.as_mut() {
            let handle = state.next_sound_handle;
            state.next_sound_handle += 1;
            state.sounds.insert(handle, sound);
            return handle;
        }
    }

    INVALID_SOUND_HANDLE
}

/// Destroy a sound
pub fn destroy_sound(handle: SoundHandle) {
    if let Ok(mut guard) = AUDIO.lock() {
        if let Some(state) = guard.as_mut() {
            state.sounds.remove(&handle);
        }
    }
}

/// Convert raw audio data to i16 samples
fn convert_to_i16(data: &[u8], bits_per_sample: u16) -> Vec<i16> {
    match bits_per_sample {
        8 => {
            // 8-bit unsigned to 16-bit signed
            data.iter()
                .map(|&s| ((s as i16 - 128) * 256) as i16)
                .collect()
        }
        16 => {
            // 16-bit signed little-endian
            data.chunks_exact(2)
                .map(|c| i16::from_le_bytes([c[0], c[1]]))
                .collect()
        }
        _ => Vec::new(),
    }
}

// =============================================================================
// Playback
// =============================================================================

/// Play a sound
pub fn play(handle: SoundHandle, volume: f32, _pan: f32, looping: bool) -> PlayHandle {
    if let Ok(mut guard) = AUDIO.lock() {
        if let Some(state) = guard.as_mut() {
            if let Some(sound) = state.sounds.get(&handle) {
                let source = SoundSource {
                    samples: sound.samples.clone(),
                    sample_rate: sound.sample_rate,
                    channels: sound.channels,
                    position: 0,
                    looping,
                };

                let sink = Sink::try_new(&state.stream_handle)
                    .map_err(|e| log::error!("Failed to create sink: {}", e))
                    .ok();

                if let Some(sink) = sink {
                    sink.set_volume(volume * state.master_volume);
                    sink.append(source);

                    let play_handle = state.next_play_handle;
                    state.next_play_handle += 1;
                    state.playing.insert(play_handle, sink);
                    return play_handle;
                }
            }
        }
    }

    INVALID_PLAY_HANDLE
}

/// Stop a playing sound
pub fn stop(handle: PlayHandle) {
    if let Ok(mut guard) = AUDIO.lock() {
        if let Some(state) = guard.as_mut() {
            if let Some(sink) = state.playing.remove(&handle) {
                sink.stop();
            }
        }
    }
}

/// Stop all playing sounds
pub fn stop_all() {
    if let Ok(mut guard) = AUDIO.lock() {
        if let Some(state) = guard.as_mut() {
            for (_, sink) in state.playing.drain() {
                sink.stop();
            }
        }
    }
}

/// Check if a sound is playing
pub fn is_playing(handle: PlayHandle) -> bool {
    if let Ok(guard) = AUDIO.lock() {
        if let Some(state) = guard.as_ref() {
            if let Some(sink) = state.playing.get(&handle) {
                return !sink.empty();
            }
        }
    }
    false
}

/// Set volume of playing sound
pub fn set_volume(handle: PlayHandle, volume: f32) {
    if let Ok(guard) = AUDIO.lock() {
        if let Some(state) = guard.as_ref() {
            if let Some(sink) = state.playing.get(&handle) {
                sink.set_volume(volume * state.master_volume);
            }
        }
    }
}

/// Set master volume
pub fn set_master_volume(volume: f32) {
    if let Ok(mut guard) = AUDIO.lock() {
        if let Some(state) = guard.as_mut() {
            state.master_volume = volume.clamp(0.0, 1.0);
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

// =============================================================================
// Custom Sound Source
// =============================================================================

struct SoundSource {
    samples: Vec<i16>,
    sample_rate: u32,
    channels: u16,
    position: usize,
    looping: bool,
}

impl Iterator for SoundSource {
    type Item = i16;

    fn next(&mut self) -> Option<Self::Item> {
        if self.position >= self.samples.len() {
            if self.looping {
                self.position = 0;
            } else {
                return None;
            }
        }

        let sample = self.samples[self.position];
        self.position += 1;
        Some(sample)
    }
}

impl Source for SoundSource {
    fn current_frame_len(&self) -> Option<usize> {
        None
    }

    fn channels(&self) -> u16 {
        self.channels
    }

    fn sample_rate(&self) -> u32 {
        self.sample_rate
    }

    fn total_duration(&self) -> Option<std::time::Duration> {
        let total_samples = self.samples.len() / self.channels as usize;
        Some(std::time::Duration::from_secs_f64(
            total_samples as f64 / self.sample_rate as f64
        ))
    }
}
```

### 8.2 ADPCM Decoder

File: `platform/src/audio/adpcm.rs`
```rust
//! IMA ADPCM decoder

/// IMA ADPCM index table
static INDEX_TABLE: [i32; 16] = [
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8,
];

/// IMA ADPCM step table
static STEP_TABLE: [i32; 89] = [
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767,
];

/// ADPCM decoder state
pub struct AdpcmDecoder {
    predictor: i32,
    step_index: i32,
}

impl AdpcmDecoder {
    pub fn new() -> Self {
        Self {
            predictor: 0,
            step_index: 0,
        }
    }

    /// Decode a single nibble
    fn decode_nibble(&mut self, nibble: u8) -> i16 {
        let step = STEP_TABLE[self.step_index as usize];

        // Calculate difference
        let mut diff = step >> 3;
        if nibble & 1 != 0 { diff += step >> 2; }
        if nibble & 2 != 0 { diff += step >> 1; }
        if nibble & 4 != 0 { diff += step; }
        if nibble & 8 != 0 { diff = -diff; }

        // Update predictor
        self.predictor += diff;
        self.predictor = self.predictor.clamp(-32768, 32767);

        // Update step index
        self.step_index += INDEX_TABLE[(nibble & 0x0F) as usize];
        self.step_index = self.step_index.clamp(0, 88);

        self.predictor as i16
    }

    /// Decode ADPCM data to PCM samples
    pub fn decode(&mut self, input: &[u8]) -> Vec<i16> {
        let mut output = Vec::with_capacity(input.len() * 2);

        for &byte in input {
            // Decode low nibble
            output.push(self.decode_nibble(byte & 0x0F));
            // Decode high nibble
            output.push(self.decode_nibble((byte >> 4) & 0x0F));
        }

        output
    }

    /// Reset decoder state
    pub fn reset(&mut self) {
        self.predictor = 0;
        self.step_index = 0;
    }
}

/// Decode ADPCM audio data
pub fn decode_adpcm(input: &[u8]) -> Vec<i16> {
    let mut decoder = AdpcmDecoder::new();
    decoder.decode(input)
}
```

### 8.3 FFI Audio Exports

File: `platform/src/ffi/audio.rs`
```rust
//! Audio FFI exports

use crate::audio::{self, AudioConfig, SoundHandle, PlayHandle, INVALID_SOUND_HANDLE, INVALID_PLAY_HANDLE};
use crate::error::catch_panic;
use std::ffi::c_void;

/// Opaque sound type for C
pub struct PlatformSound(SoundHandle);

// =============================================================================
// Initialization
// =============================================================================

/// Initialize audio subsystem
#[no_mangle]
pub extern "C" fn Platform_Audio_Init(config: *const AudioConfig) -> i32 {
    catch_panic(|| {
        let config = if config.is_null() {
            AudioConfig::default()
        } else {
            unsafe { *config }
        };

        audio::init(&config)?;
        Ok(())
    })
}

/// Shutdown audio
#[no_mangle]
pub extern "C" fn Platform_Audio_Shutdown() {
    audio::shutdown();
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

// =============================================================================
// Sound Management
// =============================================================================

/// Create sound from PCM memory
#[no_mangle]
pub extern "C" fn Platform_Sound_CreateFromMemory(
    data: *const c_void,
    size: i32,
    sample_rate: i32,
    channels: i32,
    bits_per_sample: i32,
) -> *mut PlatformSound {
    if data.is_null() || size <= 0 {
        return std::ptr::null_mut();
    }

    let slice = unsafe { std::slice::from_raw_parts(data as *const u8, size as usize) };

    let handle = audio::create_sound(
        slice,
        sample_rate as u32,
        channels as u16,
        bits_per_sample as u16,
    );

    if handle == INVALID_SOUND_HANDLE {
        std::ptr::null_mut()
    } else {
        Box::into_raw(Box::new(PlatformSound(handle)))
    }
}

/// Create sound from ADPCM data
#[no_mangle]
pub extern "C" fn Platform_Sound_CreateFromADPCM(
    data: *const c_void,
    size: i32,
    sample_rate: i32,
    channels: i32,
) -> *mut PlatformSound {
    if data.is_null() || size <= 0 {
        return std::ptr::null_mut();
    }

    let slice = unsafe { std::slice::from_raw_parts(data as *const u8, size as usize) };

    // Decode ADPCM to PCM
    let samples = crate::audio::adpcm::decode_adpcm(slice);

    // Convert to bytes
    let bytes: Vec<u8> = samples.iter()
        .flat_map(|&s| s.to_le_bytes())
        .collect();

    let handle = audio::create_sound(
        &bytes,
        sample_rate as u32,
        channels as u16,
        16,
    );

    if handle == INVALID_SOUND_HANDLE {
        std::ptr::null_mut()
    } else {
        Box::into_raw(Box::new(PlatformSound(handle)))
    }
}

/// Destroy a sound
#[no_mangle]
pub extern "C" fn Platform_Sound_Destroy(sound: *mut PlatformSound) {
    if !sound.is_null() {
        unsafe {
            let sound = Box::from_raw(sound);
            audio::destroy_sound(sound.0);
        }
    }
}

// =============================================================================
// Playback
// =============================================================================

/// Play a sound
#[no_mangle]
pub extern "C" fn Platform_Sound_Play(
    sound: *const PlatformSound,
    volume: f32,
    pan: f32,
    looping: bool,
) -> PlayHandle {
    if sound.is_null() {
        return INVALID_PLAY_HANDLE;
    }

    unsafe {
        audio::play((*sound).0, volume, pan, looping)
    }
}

/// Stop a playing sound
#[no_mangle]
pub extern "C" fn Platform_Sound_Stop(handle: PlayHandle) {
    audio::stop(handle);
}

/// Stop all sounds
#[no_mangle]
pub extern "C" fn Platform_Sound_StopAll() {
    audio::stop_all();
}

/// Check if sound is playing
#[no_mangle]
pub extern "C" fn Platform_Sound_IsPlaying(handle: PlayHandle) -> bool {
    audio::is_playing(handle)
}

/// Set volume of playing sound
#[no_mangle]
pub extern "C" fn Platform_Sound_SetVolume(handle: PlayHandle, volume: f32) {
    audio::set_volume(handle, volume);
}

/// Set pan of playing sound (not fully implemented)
#[no_mangle]
pub extern "C" fn Platform_Sound_SetPan(_handle: PlayHandle, _pan: f32) {
    // Panning would require custom mixer implementation
}
```

## Tasks Breakdown

### Phase 1: Basic Audio (2 days)
- [ ] Initialize rodio output
- [ ] Implement sound loading
- [ ] Implement basic playback

### Phase 2: Volume/Pan (1 day)
- [ ] Implement volume control
- [ ] Implement basic panning

### Phase 3: ADPCM (1 day)
- [ ] Port ADPCM decoder
- [ ] Test with game audio

### Phase 4: FFI Exports (1 day)
- [ ] Create FFI functions
- [ ] Verify cbindgen output
- [ ] Test from C++

### Phase 5: Integration (1.5 days)
- [ ] Hook into game audio code
- [ ] Test all sounds
- [ ] Debug issues

## Acceptance Criteria

- [ ] Sound effects play correctly
- [ ] Volume control works
- [ ] ADPCM audio decodes correctly
- [ ] No audio glitches
- [ ] All FFI functions work

## Estimated Duration
**5-7 days**

## Dependencies
- Plan 07 (File System) for loading audio
- rodio crate

## Next Plan
Once audio is working, proceed to **Plan 09: Assembly Rewrites**
