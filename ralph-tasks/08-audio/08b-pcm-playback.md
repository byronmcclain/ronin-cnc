# Task 08b: PCM Sound Loading and Playback

## Dependencies
- Task 08a must be complete (Audio device setup)
- rodio crate available

## Context
This task implements PCM sound loading and playback functionality. The original codebase supports 8-bit unsigned and 16-bit signed PCM at 11025Hz and 22050Hz sample rates. We need to create sounds from raw PCM data, play them with volume/pan control, and manage playback handles.

## Objective
Implement sound creation from PCM data, basic playback functions (play, stop, is_playing), and volume control. Create a custom rodio Source implementation for sound playback.

## Deliverables
- [ ] Sound creation from PCM memory in `platform/src/audio/mod.rs`
- [ ] Custom SoundSource implementation for rodio
- [ ] Playback functions: play, stop, stop_all, is_playing
- [ ] Volume control for individual sounds
- [ ] FFI exports for sound operations
- [ ] Verification passes

## Files to Modify

### platform/src/audio/mod.rs (MODIFY)
Add after the existing code:
```rust
use rodio::Source;

// =============================================================================
// Sound Management
// =============================================================================

/// Create a sound from raw PCM data
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

/// Destroy a sound and free its resources
pub fn destroy_sound(handle: SoundHandle) {
    if let Ok(mut guard) = AUDIO.lock() {
        if let Some(state) = guard.as_mut() {
            state.sounds.remove(&handle);
        }
    }
}

/// Get the number of loaded sounds
pub fn get_sound_count() -> usize {
    if let Ok(guard) = AUDIO.lock() {
        if let Some(state) = guard.as_ref() {
            return state.sounds.len();
        }
    }
    0
}

/// Convert raw audio data to i16 samples
fn convert_to_i16(data: &[u8], bits_per_sample: u16) -> Vec<i16> {
    match bits_per_sample {
        8 => {
            // 8-bit unsigned PCM to 16-bit signed
            // Original: 0-255 with 128 as center
            // Target: -32768 to 32767 with 0 as center
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
        _ => {
            log::warn!("Unsupported bits_per_sample: {}", bits_per_sample);
            Vec::new()
        }
    }
}

// =============================================================================
// Playback
// =============================================================================

/// Play a sound with volume and pan control
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

                match Sink::try_new(&state.stream_handle) {
                    Ok(sink) => {
                        sink.set_volume(volume * state.master_volume);
                        sink.append(source);

                        let play_handle = state.next_play_handle;
                        state.next_play_handle += 1;
                        state.playing.insert(play_handle, sink);
                        return play_handle;
                    }
                    Err(e) => {
                        log::error!("Failed to create audio sink: {}", e);
                    }
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

/// Check if a sound is currently playing
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

/// Set volume of a playing sound (0.0 to 1.0)
pub fn set_volume(handle: PlayHandle, volume: f32) {
    if let Ok(guard) = AUDIO.lock() {
        if let Some(state) = guard.as_ref() {
            if let Some(sink) = state.playing.get(&handle) {
                sink.set_volume(volume.clamp(0.0, 1.0) * state.master_volume);
            }
        }
    }
}

/// Pause a playing sound
pub fn pause(handle: PlayHandle) {
    if let Ok(guard) = AUDIO.lock() {
        if let Some(state) = guard.as_ref() {
            if let Some(sink) = state.playing.get(&handle) {
                sink.pause();
            }
        }
    }
}

/// Resume a paused sound
pub fn resume(handle: PlayHandle) {
    if let Ok(guard) = AUDIO.lock() {
        if let Some(state) = guard.as_ref() {
            if let Some(sink) = state.playing.get(&handle) {
                sink.play();
            }
        }
    }
}

/// Get the number of currently playing sounds
pub fn get_playing_count() -> usize {
    if let Ok(guard) = AUDIO.lock() {
        if let Some(state) = guard.as_ref() {
            // Count only non-empty sinks
            return state.playing.values().filter(|s| !s.empty()).count();
        }
    }
    0
}

// =============================================================================
// Custom Sound Source for rodio
// =============================================================================

/// Custom sound source implementing rodio's Source trait
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

        if self.samples.is_empty() {
            return None;
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
        if self.looping {
            None
        } else {
            let total_samples = self.samples.len() / self.channels.max(1) as usize;
            Some(std::time::Duration::from_secs_f64(
                total_samples as f64 / self.sample_rate.max(1) as f64
            ))
        }
    }
}
```

### platform/src/ffi/mod.rs (MODIFY)
Add sound FFI exports after audio init functions:
```rust
use crate::audio::{SoundHandle, PlayHandle, INVALID_SOUND_HANDLE, INVALID_PLAY_HANDLE};
use std::ffi::c_void;

// =============================================================================
// Sound FFI
// =============================================================================

/// Create a sound from raw PCM data in memory
#[no_mangle]
pub extern "C" fn Platform_Sound_CreateFromMemory(
    data: *const c_void,
    size: i32,
    sample_rate: i32,
    channels: i32,
    bits_per_sample: i32,
) -> SoundHandle {
    if data.is_null() || size <= 0 {
        return INVALID_SOUND_HANDLE;
    }

    let slice = unsafe { std::slice::from_raw_parts(data as *const u8, size as usize) };

    audio::create_sound(
        slice,
        sample_rate as u32,
        channels as u16,
        bits_per_sample as u16,
    )
}

/// Destroy a sound
#[no_mangle]
pub extern "C" fn Platform_Sound_Destroy(handle: SoundHandle) {
    audio::destroy_sound(handle);
}

/// Get number of loaded sounds
#[no_mangle]
pub extern "C" fn Platform_Sound_GetCount() -> i32 {
    audio::get_sound_count() as i32
}

/// Play a sound
#[no_mangle]
pub extern "C" fn Platform_Sound_Play(
    handle: SoundHandle,
    volume: f32,
    pan: f32,
    looping: bool,
) -> PlayHandle {
    audio::play(handle, volume, pan, looping)
}

/// Stop a playing sound
#[no_mangle]
pub extern "C" fn Platform_Sound_Stop(handle: PlayHandle) {
    audio::stop(handle);
}

/// Stop all playing sounds
#[no_mangle]
pub extern "C" fn Platform_Sound_StopAll() {
    audio::stop_all();
}

/// Check if a sound is playing
#[no_mangle]
pub extern "C" fn Platform_Sound_IsPlaying(handle: PlayHandle) -> bool {
    audio::is_playing(handle)
}

/// Set volume of a playing sound
#[no_mangle]
pub extern "C" fn Platform_Sound_SetVolume(handle: PlayHandle, volume: f32) {
    audio::set_volume(handle, volume);
}

/// Pause a playing sound
#[no_mangle]
pub extern "C" fn Platform_Sound_Pause(handle: PlayHandle) {
    audio::pause(handle);
}

/// Resume a paused sound
#[no_mangle]
pub extern "C" fn Platform_Sound_Resume(handle: PlayHandle) {
    audio::resume(handle);
}

/// Get number of currently playing sounds
#[no_mangle]
pub extern "C" fn Platform_Sound_GetPlayingCount() -> i32 {
    audio::get_playing_count() as i32
}
```

## Verification Command
```bash
cd /Users/ooartist/Src/Ronin_CnC && \
cargo build --manifest-path platform/Cargo.toml --features cbindgen-run 2>&1 && \
grep -q "Platform_Sound_CreateFromMemory" include/platform.h && \
grep -q "Platform_Sound_Play" include/platform.h && \
grep -q "Platform_Sound_Stop" include/platform.h && \
grep -q "Platform_Sound_IsPlaying" include/platform.h && \
echo "VERIFY_SUCCESS"
```

## Implementation Steps
1. Add SoundSource implementation to `platform/src/audio/mod.rs`
2. Add sound creation and management functions
3. Add playback control functions
4. Add sound FFI exports to `platform/src/ffi/mod.rs`
5. Build with cbindgen
6. Verify header contains all sound functions
7. Run verification command

## Completion Promise
When verification passes, output:
```
<promise>TASK_08B_COMPLETE</promise>
```

## Escape Hatch
If stuck after 12 iterations:
- Document what's blocking in `ralph-tasks/blocked/08b.md`
- List attempted approaches
- Output: `<promise>TASK_08B_BLOCKED</promise>`

## Max Iterations
12
