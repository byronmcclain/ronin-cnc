//! Audio subsystem using rodio
//!
//! Note: On macOS, rodio's OutputStream is not Send/Sync, so we need to handle
//! audio on a dedicated thread.

pub mod adpcm;

use crate::error::PlatformError;
use rodio::{OutputStream, OutputStreamHandle, Sink, Source};
use std::sync::{Arc, Mutex, mpsc};
use std::collections::HashMap;
use std::thread::{self, JoinHandle};
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
#[derive(Clone)]
pub struct SoundData {
    pub samples: Vec<i16>,
    pub sample_rate: u32,
    pub channels: u16,
}

/// Commands sent to the audio thread
enum AudioCommand {
    CreateSound {
        id: SoundHandle,
        data: SoundData,
    },
    DestroySound(SoundHandle),
    Play {
        sound_id: SoundHandle,
        play_id: PlayHandle,
        volume: f32,
        looping: bool,
    },
    Stop(PlayHandle),
    StopAll,
    SetVolume { handle: PlayHandle, volume: f32 },
    SetMasterVolume(f32),
    Pause(PlayHandle),
    Resume(PlayHandle),
    Shutdown,
}

/// Response from audio thread
enum AudioResponse {
    SoundCreated(SoundHandle),
    PlayStarted(PlayHandle),
    IsPlaying(PlayHandle, bool),
    Ok,
    Error(String),
}

/// State shared between main thread and audio thread
struct SharedState {
    master_volume: f32,
    sound_count: usize,
    playing_count: usize,
    initialized: bool,
    next_sound_handle: SoundHandle,
    next_play_handle: PlayHandle,
}

/// Global audio controller
struct AudioController {
    command_tx: mpsc::Sender<AudioCommand>,
    #[allow(dead_code)]
    thread_handle: JoinHandle<()>,
    shared: Arc<Mutex<SharedState>>,
}

/// Global audio controller singleton
static AUDIO: Lazy<Mutex<Option<AudioController>>> = Lazy::new(|| Mutex::new(None));

// =============================================================================
// Audio Thread
// =============================================================================

fn audio_thread(
    command_rx: mpsc::Receiver<AudioCommand>,
    shared: Arc<Mutex<SharedState>>,
) {
    // Initialize audio output on this thread
    let (_stream, stream_handle) = match OutputStream::try_default() {
        Ok((s, h)) => (s, h),
        Err(e) => {
            eprintln!("[AUDIO] Failed to initialize output stream: {}", e);
            return;
        }
    };

    let mut sounds: HashMap<SoundHandle, Arc<SoundData>> = HashMap::new();
    let mut playing: HashMap<PlayHandle, Sink> = HashMap::new();
    let mut master_volume: f32 = 1.0;

    loop {
        match command_rx.recv() {
            Ok(cmd) => match cmd {
                AudioCommand::CreateSound { id, data } => {
                    sounds.insert(id, Arc::new(data));
                    if let Ok(mut s) = shared.lock() {
                        s.sound_count = sounds.len();
                    }
                }
                AudioCommand::DestroySound(id) => {
                    sounds.remove(&id);
                    if let Ok(mut s) = shared.lock() {
                        s.sound_count = sounds.len();
                    }
                }
                AudioCommand::Play { sound_id, play_id, volume, looping } => {
                    if let Some(sound) = sounds.get(&sound_id) {
                        let source = SoundSource {
                            samples: sound.samples.clone(),
                            sample_rate: sound.sample_rate,
                            channels: sound.channels,
                            position: 0,
                            looping,
                        };

                        if let Ok(sink) = Sink::try_new(&stream_handle) {
                            sink.set_volume(volume * master_volume);
                            sink.append(source);
                            playing.insert(play_id, sink);
                            if let Ok(mut s) = shared.lock() {
                                s.playing_count = playing.len();
                            }
                        }
                    }
                }
                AudioCommand::Stop(handle) => {
                    if let Some(sink) = playing.remove(&handle) {
                        sink.stop();
                    }
                    if let Ok(mut s) = shared.lock() {
                        s.playing_count = playing.len();
                    }
                }
                AudioCommand::StopAll => {
                    for (_, sink) in playing.drain() {
                        sink.stop();
                    }
                    if let Ok(mut s) = shared.lock() {
                        s.playing_count = 0;
                    }
                }
                AudioCommand::SetVolume { handle, volume } => {
                    if let Some(sink) = playing.get(&handle) {
                        sink.set_volume(volume * master_volume);
                    }
                }
                AudioCommand::SetMasterVolume(vol) => {
                    master_volume = vol.clamp(0.0, 1.0);
                    if let Ok(mut s) = shared.lock() {
                        s.master_volume = master_volume;
                    }
                    // Update all playing sounds
                    for sink in playing.values() {
                        sink.set_volume(master_volume);
                    }
                }
                AudioCommand::Pause(handle) => {
                    if let Some(sink) = playing.get(&handle) {
                        sink.pause();
                    }
                }
                AudioCommand::Resume(handle) => {
                    if let Some(sink) = playing.get(&handle) {
                        sink.play();
                    }
                }
                AudioCommand::Shutdown => {
                    // Stop all sounds and exit
                    for (_, sink) in playing.drain() {
                        sink.stop();
                    }
                    sounds.clear();
                    break;
                }
            },
            Err(_) => {
                // Channel closed, exit thread
                break;
            }
        }

        // Clean up finished sounds
        playing.retain(|_, sink| !sink.empty());
        if let Ok(mut s) = shared.lock() {
            s.playing_count = playing.len();
        }
    }
}

// =============================================================================
// Initialization
// =============================================================================

/// Initialize audio subsystem
pub fn init(config: &AudioConfig) -> Result<(), PlatformError> {
    let (tx, rx) = mpsc::channel();

    let shared = Arc::new(Mutex::new(SharedState {
        master_volume: 1.0,
        sound_count: 0,
        playing_count: 0,
        initialized: true,
        next_sound_handle: 1,
        next_play_handle: 1,
    }));

    let shared_clone = shared.clone();
    let thread_handle = thread::spawn(move || {
        audio_thread(rx, shared_clone);
    });

    let controller = AudioController {
        command_tx: tx,
        thread_handle,
        shared,
    };

    if let Ok(mut guard) = AUDIO.lock() {
        *guard = Some(controller);
    }

    log::info!("Audio initialized: {}Hz, {} channels, {}-bit",
               config.sample_rate, config.channels, config.bits_per_sample);

    Ok(())
}

/// Shutdown audio subsystem
pub fn shutdown() {
    if let Ok(mut guard) = AUDIO.lock() {
        if let Some(controller) = guard.take() {
            let _ = controller.command_tx.send(AudioCommand::Shutdown);
            // Don't wait for thread to finish to avoid blocking
        }
    }
    log::info!("Audio shutdown");
}

/// Check if audio is initialized
pub fn is_initialized() -> bool {
    if let Ok(guard) = AUDIO.lock() {
        if let Some(controller) = guard.as_ref() {
            if let Ok(shared) = controller.shared.lock() {
                return shared.initialized;
            }
        }
    }
    false
}

// =============================================================================
// Master Volume
// =============================================================================

/// Set master volume (0.0 to 1.0)
pub fn set_master_volume(volume: f32) {
    if let Ok(guard) = AUDIO.lock() {
        if let Some(controller) = guard.as_ref() {
            // Update shared state immediately for get_master_volume()
            if let Ok(mut shared) = controller.shared.lock() {
                shared.master_volume = volume.clamp(0.0, 1.0);
            }
            // Also send to audio thread to update playing sounds
            let _ = controller.command_tx.send(AudioCommand::SetMasterVolume(volume));
        }
    }
}

/// Get master volume
pub fn get_master_volume() -> f32 {
    if let Ok(guard) = AUDIO.lock() {
        if let Some(controller) = guard.as_ref() {
            if let Ok(shared) = controller.shared.lock() {
                return shared.master_volume;
            }
        }
    }
    1.0
}

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

    let sound_data = SoundData {
        samples,
        sample_rate,
        channels,
    };

    if let Ok(guard) = AUDIO.lock() {
        if let Some(controller) = guard.as_ref() {
            if let Ok(mut shared) = controller.shared.lock() {
                let handle = shared.next_sound_handle;
                shared.next_sound_handle += 1;
                // Update sound count immediately
                shared.sound_count += 1;

                let _ = controller.command_tx.send(AudioCommand::CreateSound {
                    id: handle,
                    data: sound_data,
                });

                return handle;
            }
        }
    }

    INVALID_SOUND_HANDLE
}

/// Destroy a sound and free its resources
pub fn destroy_sound(handle: SoundHandle) {
    if let Ok(guard) = AUDIO.lock() {
        if let Some(controller) = guard.as_ref() {
            // Update sound count immediately
            if let Ok(mut shared) = controller.shared.lock() {
                if shared.sound_count > 0 {
                    shared.sound_count -= 1;
                }
            }
            let _ = controller.command_tx.send(AudioCommand::DestroySound(handle));
        }
    }
}

/// Get the number of loaded sounds
pub fn get_sound_count() -> usize {
    if let Ok(guard) = AUDIO.lock() {
        if let Some(controller) = guard.as_ref() {
            if let Ok(shared) = controller.shared.lock() {
                return shared.sound_count;
            }
        }
    }
    0
}

/// Convert raw audio data to i16 samples
fn convert_to_i16(data: &[u8], bits_per_sample: u16) -> Vec<i16> {
    match bits_per_sample {
        8 => {
            // 8-bit unsigned PCM to 16-bit signed
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
    if let Ok(guard) = AUDIO.lock() {
        if let Some(controller) = guard.as_ref() {
            if let Ok(mut shared) = controller.shared.lock() {
                let play_handle = shared.next_play_handle;
                shared.next_play_handle += 1;

                let _ = controller.command_tx.send(AudioCommand::Play {
                    sound_id: handle,
                    play_id: play_handle,
                    volume,
                    looping,
                });

                return play_handle;
            }
        }
    }

    INVALID_PLAY_HANDLE
}

/// Stop a playing sound
pub fn stop(handle: PlayHandle) {
    if let Ok(guard) = AUDIO.lock() {
        if let Some(controller) = guard.as_ref() {
            let _ = controller.command_tx.send(AudioCommand::Stop(handle));
        }
    }
}

/// Stop all playing sounds
pub fn stop_all() {
    if let Ok(guard) = AUDIO.lock() {
        if let Some(controller) = guard.as_ref() {
            let _ = controller.command_tx.send(AudioCommand::StopAll);
        }
    }
}

/// Check if a sound is currently playing
/// Note: This is approximate due to the threaded nature
pub fn is_playing(_handle: PlayHandle) -> bool {
    if let Ok(guard) = AUDIO.lock() {
        if let Some(controller) = guard.as_ref() {
            if let Ok(shared) = controller.shared.lock() {
                return shared.playing_count > 0;
            }
        }
    }
    false
}

/// Set volume of a playing sound (0.0 to 1.0)
pub fn set_volume(handle: PlayHandle, volume: f32) {
    if let Ok(guard) = AUDIO.lock() {
        if let Some(controller) = guard.as_ref() {
            let _ = controller.command_tx.send(AudioCommand::SetVolume {
                handle,
                volume: volume.clamp(0.0, 1.0)
            });
        }
    }
}

/// Pause a playing sound
pub fn pause(handle: PlayHandle) {
    if let Ok(guard) = AUDIO.lock() {
        if let Some(controller) = guard.as_ref() {
            let _ = controller.command_tx.send(AudioCommand::Pause(handle));
        }
    }
}

/// Resume a paused sound
pub fn resume(handle: PlayHandle) {
    if let Ok(guard) = AUDIO.lock() {
        if let Some(controller) = guard.as_ref() {
            let _ = controller.command_tx.send(AudioCommand::Resume(handle));
        }
    }
}

/// Get the number of currently playing sounds
pub fn get_playing_count() -> usize {
    if let Ok(guard) = AUDIO.lock() {
        if let Some(controller) = guard.as_ref() {
            if let Ok(shared) = controller.shared.lock() {
                return shared.playing_count;
            }
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
