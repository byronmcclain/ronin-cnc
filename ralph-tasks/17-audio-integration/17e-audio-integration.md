# Task 17e: Audio Integration

| Field | Value |
|-------|-------|
| **Task ID** | 17e |
| **Task Name** | Audio Integration |
| **Phase** | 17 - Audio Integration |
| **Depends On** | Tasks 17a-17d |
| **Estimated Complexity** | Medium (~300 lines) |

---

## Dependencies

- Task 17a (AUD File Parser) - AudFile class
- Task 17b (Sound Manager) - SoundManager, SoundEffect
- Task 17c (Music System) - MusicPlayer, MusicTrack
- Task 17d (Voice System) - VoiceManager, EvaVoice, UnitVoice
- Platform audio API
- Viewport system (for listener position)

---

## Context

The Audio Integration task creates a unified interface that coordinates all audio subsystems:

1. **Unified Initialization/Shutdown**: Single entry point for all audio setup
2. **Coordinated Update Loop**: Single frame update that updates all subsystems
3. **Master Volume Control**: Global volume that affects all audio
4. **Listener Position Sync**: Automatically sync listener with viewport
5. **Convenience API**: Simple functions for common operations
6. **Debug/Stats**: Unified audio statistics and debugging

This is the public API that game code uses for all audio operations.

---

## Objective

Create an AudioSystem class that:
1. Initializes and coordinates all audio subsystems
2. Provides unified volume controls (master, SFX, music, voice)
3. Synchronizes listener position with camera each frame
4. Offers convenience functions for common audio operations
5. Provides audio statistics and debug information
6. Handles clean shutdown of all audio resources

---

## Deliverables

- [ ] `include/game/audio/audio_system.h` - Unified audio API
- [ ] `src/game/audio/audio_system.cpp` - Implementation
- [ ] `src/test_audio_integration.cpp` - Integration test
- [ ] CMakeLists.txt updated
- [ ] All tests pass

---

## Files to Create

### include/game/audio/audio_system.h

```cpp
// include/game/audio/audio_system.h
// Unified Audio System Interface
// Task 17e - Audio Integration

#ifndef AUDIO_SYSTEM_H
#define AUDIO_SYSTEM_H

#include "game/audio/sound_effect.h"
#include "game/audio/music_track.h"
#include "game/audio/voice_event.h"
#include "platform.h"

//=============================================================================
// Audio System Configuration
//=============================================================================

struct AudioSystemConfig {
    // Master settings
    float master_volume = 1.0f;

    // Subsystem volumes (0.0 - 1.0)
    float sfx_volume = 1.0f;
    float music_volume = 0.8f;
    float voice_volume = 1.0f;

    // Music settings
    bool music_shuffle = false;
    bool music_loop = true;

    // SFX settings
    int max_concurrent_sounds = 16;
    int max_audible_distance = 1200;

    // Platform audio settings
    int sample_rate = 22050;
    int channels = 2;
    int buffer_size = 1024;
};

//=============================================================================
// Audio Statistics
//=============================================================================

struct AudioStats {
    // Sounds
    int loaded_sounds;
    int playing_sounds;

    // Music
    bool music_playing;
    const char* current_track_name;

    // Voice
    bool eva_speaking;
    bool unit_speaking;
    int voice_queue_size;

    // System
    float master_volume;
    float sfx_volume;
    float music_volume;
    float voice_volume;
    bool muted;
};

//=============================================================================
// Audio System Class
//=============================================================================

class AudioSystem {
public:
    /// Singleton instance
    static AudioSystem& Instance();

    //=========================================================================
    // Lifecycle
    //=========================================================================

    /// Initialize entire audio system
    /// @param config Configuration options
    /// @return true if successful
    bool Initialize(const AudioSystemConfig& config = AudioSystemConfig());

    /// Shutdown entire audio system
    void Shutdown();

    /// Check if initialized
    bool IsInitialized() const { return initialized_; }

    //=========================================================================
    // Per-Frame Update
    //=========================================================================

    /// Update all audio subsystems (call once per frame)
    /// - Updates listener position from viewport
    /// - Processes voice queue
    /// - Handles music transitions
    /// - Cleans up finished sounds
    void Update();

    //=========================================================================
    // Master Volume Control
    //=========================================================================

    /// Set master volume (affects all audio)
    void SetMasterVolume(float volume);

    /// Get master volume
    float GetMasterVolume() const { return config_.master_volume; }

    /// Mute/unmute all audio
    void SetMasterMuted(bool muted);

    /// Check if master muted
    bool IsMasterMuted() const { return master_muted_; }

    /// Toggle master mute
    void ToggleMute();

    //=========================================================================
    // Category Volume Control
    //=========================================================================

    /// Set sound effects volume
    void SetSFXVolume(float volume);
    float GetSFXVolume() const { return config_.sfx_volume; }

    /// Set music volume
    void SetMusicVolume(float volume);
    float GetMusicVolume() const { return config_.music_volume; }

    /// Set voice volume
    void SetVoiceVolume(float volume);
    float GetVoiceVolume() const { return config_.voice_volume; }

    //=========================================================================
    // Sound Effects (convenience wrappers)
    //=========================================================================

    /// Play a sound effect
    PlayHandle PlaySound(SoundEffect sfx);

    /// Play a sound effect at world position
    PlayHandle PlaySoundAt(SoundEffect sfx, int world_x, int world_y);

    /// Play a sound effect at cell position
    PlayHandle PlaySoundAtCell(SoundEffect sfx, int cell_x, int cell_y);

    /// Stop all sound effects
    void StopAllSounds();

    //=========================================================================
    // Music (convenience wrappers)
    //=========================================================================

    /// Play a music track
    void PlayMusic(MusicTrack track);

    /// Stop music
    void StopMusic();

    /// Pause music
    void PauseMusic();

    /// Resume music
    void ResumeMusic();

    /// Toggle music pause
    void ToggleMusicPause();

    /// Play next music track
    void NextTrack();

    /// Check if music is playing
    bool IsMusicPlaying() const;

    /// Get current track name
    const char* GetCurrentTrackName() const;

    /// Enable/disable shuffle
    void SetMusicShuffle(bool enabled);

    //=========================================================================
    // Voice (convenience wrappers)
    //=========================================================================

    /// Play EVA announcement
    void PlayEva(EvaVoice voice);

    /// Queue EVA announcement
    void QueueEva(EvaVoice voice);

    /// Play unit acknowledgment
    void PlayUnit(UnitVoice voice, VoiceFaction faction = VoiceFaction::NEUTRAL);

    /// Stop all voices
    void StopAllVoices();

    //=========================================================================
    // Stop All Audio
    //=========================================================================

    /// Stop all audio (sounds, music, voices)
    void StopAll();

    //=========================================================================
    // Statistics
    //=========================================================================

    /// Get audio statistics
    AudioStats GetStats() const;

    /// Print debug information
    void PrintDebugInfo() const;

    //=========================================================================
    // Configuration
    //=========================================================================

    /// Get current configuration
    const AudioSystemConfig& GetConfig() const { return config_; }

    /// Update configuration (limited - some settings require reinit)
    void UpdateConfig(const AudioSystemConfig& config);

private:
    AudioSystem();
    ~AudioSystem();
    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;

    AudioSystemConfig config_;
    bool initialized_;
    bool master_muted_;

    void ApplyMasterVolume();
};

//=============================================================================
// Global Audio Functions (C-style API for game code)
//=============================================================================

// Lifecycle
bool Audio_Init();
void Audio_Shutdown();
void Audio_Update();

// Master control
void Audio_SetMasterVolume(int volume);  // 0-255
int Audio_GetMasterVolume();
void Audio_SetMuted(bool muted);
bool Audio_IsMuted();
void Audio_ToggleMute();

// Category volumes (0-255)
void Audio_SetSFXVolume(int volume);
int Audio_GetSFXVolume();
void Audio_SetMusicVolume(int volume);
int Audio_GetMusicVolume();
void Audio_SetVoiceVolume(int volume);
int Audio_GetVoiceVolume();

// Sound effects
PlayHandle Audio_PlaySound(SoundEffect sfx);
PlayHandle Audio_PlaySoundAt(SoundEffect sfx, int world_x, int world_y);
void Audio_StopAllSounds();

// Music
void Audio_PlayMusic(MusicTrack track);
void Audio_StopMusic();
void Audio_PauseMusic();
void Audio_ResumeMusic();
bool Audio_IsMusicPlaying();

// Voice
void Audio_PlayEva(EvaVoice voice);
void Audio_PlayUnit(UnitVoice voice);

// Stop everything
void Audio_StopAll();

#endif // AUDIO_SYSTEM_H
```

### src/game/audio/audio_system.cpp

```cpp
// src/game/audio/audio_system.cpp
// Unified Audio System Implementation
// Task 17e - Audio Integration

#include "game/audio/audio_system.h"
#include "game/audio/sound_manager.h"
#include "game/audio/music_player.h"
#include "game/audio/voice_manager.h"
#include "game/viewport.h"
#include "platform.h"
#include <algorithm>

//=============================================================================
// AudioSystem Implementation
//=============================================================================

AudioSystem& AudioSystem::Instance() {
    static AudioSystem instance;
    return instance;
}

AudioSystem::AudioSystem()
    : initialized_(false)
    , master_muted_(false) {
}

AudioSystem::~AudioSystem() {
    Shutdown();
}

bool AudioSystem::Initialize(const AudioSystemConfig& config) {
    if (initialized_) {
        Platform_Log("AudioSystem: Already initialized");
        return true;
    }

    Platform_Log("AudioSystem: Initializing...");
    config_ = config;

    // Initialize platform audio
    AudioConfig platform_config;
    platform_config.sample_rate = config.sample_rate;
    platform_config.channels = config.channels;
    platform_config.bits_per_sample = 16;
    platform_config.buffer_size = config.buffer_size;

    if (Platform_Audio_Init(&platform_config) != 0) {
        Platform_Log("AudioSystem: Failed to initialize platform audio");
        return false;
    }

    // Initialize sound manager
    SoundManagerConfig sfx_config;
    sfx_config.max_concurrent_sounds = config.max_concurrent_sounds;
    sfx_config.default_max_distance = config.max_audible_distance;

    if (!SoundManager::Instance().Initialize(sfx_config)) {
        Platform_Log("AudioSystem: Failed to initialize SoundManager");
        Platform_Audio_Shutdown();
        return false;
    }

    // Initialize music player
    MusicPlayerConfig music_config;
    music_config.shuffle_enabled = config.music_shuffle;
    music_config.loop_enabled = config.music_loop;

    if (!MusicPlayer::Instance().Initialize(music_config)) {
        Platform_Log("AudioSystem: Failed to initialize MusicPlayer");
        SoundManager::Instance().Shutdown();
        Platform_Audio_Shutdown();
        return false;
    }

    // Initialize voice manager
    if (!VoiceManager::Instance().Initialize()) {
        Platform_Log("AudioSystem: Failed to initialize VoiceManager");
        MusicPlayer::Instance().Shutdown();
        SoundManager::Instance().Shutdown();
        Platform_Audio_Shutdown();
        return false;
    }

    // Apply initial volumes
    SetSFXVolume(config.sfx_volume);
    SetMusicVolume(config.music_volume);
    SetVoiceVolume(config.voice_volume);

    initialized_ = true;
    Platform_Log("AudioSystem: Initialized successfully");
    Platform_Log("  SFX Volume: %.0f%%", config.sfx_volume * 100);
    Platform_Log("  Music Volume: %.0f%%", config.music_volume * 100);
    Platform_Log("  Voice Volume: %.0f%%", config.voice_volume * 100);

    return true;
}

void AudioSystem::Shutdown() {
    if (!initialized_) {
        return;
    }

    Platform_Log("AudioSystem: Shutting down...");

    StopAll();

    VoiceManager::Instance().Shutdown();
    MusicPlayer::Instance().Shutdown();
    SoundManager::Instance().Shutdown();
    Platform_Audio_Shutdown();

    initialized_ = false;
    Platform_Log("AudioSystem: Shutdown complete");
}

void AudioSystem::Update() {
    if (!initialized_) {
        return;
    }

    // Update listener position from viewport
    SoundManager::Instance().UpdateListenerFromViewport();

    // Update all subsystems
    SoundManager::Instance().Update();
    MusicPlayer::Instance().Update();
    VoiceManager::Instance().Update();
}

//=============================================================================
// Master Volume
//=============================================================================

void AudioSystem::SetMasterVolume(float volume) {
    config_.master_volume = std::clamp(volume, 0.0f, 1.0f);
    ApplyMasterVolume();
}

void AudioSystem::SetMasterMuted(bool muted) {
    master_muted_ = muted;
    ApplyMasterVolume();
}

void AudioSystem::ToggleMute() {
    SetMasterMuted(!master_muted_);
}

void AudioSystem::ApplyMasterVolume() {
    float effective_master = master_muted_ ? 0.0f : config_.master_volume;

    // Apply to platform
    Platform_Audio_SetMasterVolume(effective_master);

    // Update subsystem volumes (they already have their own volume settings)
    // The platform master volume will affect all sounds
}

//=============================================================================
// Category Volumes
//=============================================================================

void AudioSystem::SetSFXVolume(float volume) {
    config_.sfx_volume = std::clamp(volume, 0.0f, 1.0f);
    SoundManager::Instance().SetVolume(config_.sfx_volume);
}

void AudioSystem::SetMusicVolume(float volume) {
    config_.music_volume = std::clamp(volume, 0.0f, 1.0f);
    MusicPlayer::Instance().SetVolume(config_.music_volume);
}

void AudioSystem::SetVoiceVolume(float volume) {
    config_.voice_volume = std::clamp(volume, 0.0f, 1.0f);
    VoiceManager::Instance().SetVolume(config_.voice_volume);
}

//=============================================================================
// Sound Effects
//=============================================================================

PlayHandle AudioSystem::PlaySound(SoundEffect sfx) {
    if (!initialized_ || master_muted_) return INVALID_PLAY_HANDLE;
    return SoundManager::Instance().Play(sfx);
}

PlayHandle AudioSystem::PlaySoundAt(SoundEffect sfx, int world_x, int world_y) {
    if (!initialized_ || master_muted_) return INVALID_PLAY_HANDLE;
    return SoundManager::Instance().PlayAt(sfx, world_x, world_y);
}

PlayHandle AudioSystem::PlaySoundAtCell(SoundEffect sfx, int cell_x, int cell_y) {
    if (!initialized_ || master_muted_) return INVALID_PLAY_HANDLE;
    return SoundManager::Instance().PlayAtCell(sfx, cell_x, cell_y);
}

void AudioSystem::StopAllSounds() {
    if (initialized_) {
        SoundManager::Instance().StopAll();
    }
}

//=============================================================================
// Music
//=============================================================================

void AudioSystem::PlayMusic(MusicTrack track) {
    if (!initialized_) return;
    MusicPlayer::Instance().Play(track);
}

void AudioSystem::StopMusic() {
    if (initialized_) {
        MusicPlayer::Instance().Stop();
    }
}

void AudioSystem::PauseMusic() {
    if (initialized_) {
        MusicPlayer::Instance().Pause();
    }
}

void AudioSystem::ResumeMusic() {
    if (initialized_) {
        MusicPlayer::Instance().Resume();
    }
}

void AudioSystem::ToggleMusicPause() {
    if (initialized_) {
        MusicPlayer::Instance().TogglePause();
    }
}

void AudioSystem::NextTrack() {
    if (initialized_) {
        MusicPlayer::Instance().PlayNext();
    }
}

bool AudioSystem::IsMusicPlaying() const {
    return initialized_ && MusicPlayer::Instance().IsPlaying();
}

const char* AudioSystem::GetCurrentTrackName() const {
    if (!initialized_) return "None";
    return MusicPlayer::Instance().GetCurrentTrackName();
}

void AudioSystem::SetMusicShuffle(bool enabled) {
    if (initialized_) {
        MusicPlayer::Instance().SetShuffleEnabled(enabled);
    }
}

//=============================================================================
// Voice
//=============================================================================

void AudioSystem::PlayEva(EvaVoice voice) {
    if (!initialized_ || master_muted_) return;
    VoiceManager::Instance().PlayEva(voice);
}

void AudioSystem::QueueEva(EvaVoice voice) {
    if (!initialized_) return;
    VoiceManager::Instance().QueueEva(voice);
}

void AudioSystem::PlayUnit(UnitVoice voice, VoiceFaction faction) {
    if (!initialized_ || master_muted_) return;
    VoiceManager::Instance().PlayUnit(voice, faction);
}

void AudioSystem::StopAllVoices() {
    if (initialized_) {
        VoiceManager::Instance().StopEva();
        VoiceManager::Instance().StopUnit();
    }
}

//=============================================================================
// Stop All
//=============================================================================

void AudioSystem::StopAll() {
    if (!initialized_) return;

    StopAllSounds();
    StopMusic();
    StopAllVoices();
}

//=============================================================================
// Statistics
//=============================================================================

AudioStats AudioSystem::GetStats() const {
    AudioStats stats = {};

    if (!initialized_) {
        return stats;
    }

    // Sound stats
    stats.loaded_sounds = SoundManager::Instance().GetLoadedSoundCount();
    stats.playing_sounds = SoundManager::Instance().GetPlayingSoundCount();

    // Music stats
    stats.music_playing = MusicPlayer::Instance().IsPlaying();
    stats.current_track_name = MusicPlayer::Instance().GetCurrentTrackName();

    // Voice stats
    stats.eva_speaking = VoiceManager::Instance().IsEvaSpeaking();
    stats.unit_speaking = VoiceManager::Instance().IsUnitSpeaking();
    stats.voice_queue_size = static_cast<int>(VoiceManager::Instance().GetQueueSize());

    // Volume stats
    stats.master_volume = config_.master_volume;
    stats.sfx_volume = config_.sfx_volume;
    stats.music_volume = config_.music_volume;
    stats.voice_volume = config_.voice_volume;
    stats.muted = master_muted_;

    return stats;
}

void AudioSystem::PrintDebugInfo() const {
    Platform_Log("=== Audio System Status ===");

    if (!initialized_) {
        Platform_Log("  Not initialized");
        return;
    }

    AudioStats stats = GetStats();

    Platform_Log("Master: %.0f%% %s",
                 stats.master_volume * 100,
                 stats.muted ? "(MUTED)" : "");

    Platform_Log("Volumes: SFX=%.0f%% Music=%.0f%% Voice=%.0f%%",
                 stats.sfx_volume * 100,
                 stats.music_volume * 100,
                 stats.voice_volume * 100);

    Platform_Log("Sounds: %d loaded, %d playing",
                 stats.loaded_sounds, stats.playing_sounds);

    Platform_Log("Music: %s - %s",
                 stats.music_playing ? "Playing" : "Stopped",
                 stats.current_track_name);

    Platform_Log("Voice: EVA=%s Unit=%s Queue=%d",
                 stats.eva_speaking ? "Speaking" : "Silent",
                 stats.unit_speaking ? "Speaking" : "Silent",
                 stats.voice_queue_size);
}

void AudioSystem::UpdateConfig(const AudioSystemConfig& config) {
    // Update volumes
    SetSFXVolume(config.sfx_volume);
    SetMusicVolume(config.music_volume);
    SetVoiceVolume(config.voice_volume);
    SetMasterVolume(config.master_volume);

    // Update music settings
    MusicPlayer::Instance().SetShuffleEnabled(config.music_shuffle);
    MusicPlayer::Instance().SetLoopEnabled(config.music_loop);

    // Store new config
    config_ = config;
}

//=============================================================================
// Global Functions
//=============================================================================

bool Audio_Init() {
    return AudioSystem::Instance().Initialize();
}

void Audio_Shutdown() {
    AudioSystem::Instance().Shutdown();
}

void Audio_Update() {
    AudioSystem::Instance().Update();
}

void Audio_SetMasterVolume(int volume) {
    AudioSystem::Instance().SetMasterVolume(std::clamp(volume, 0, 255) / 255.0f);
}

int Audio_GetMasterVolume() {
    return static_cast<int>(AudioSystem::Instance().GetMasterVolume() * 255);
}

void Audio_SetMuted(bool muted) {
    AudioSystem::Instance().SetMasterMuted(muted);
}

bool Audio_IsMuted() {
    return AudioSystem::Instance().IsMasterMuted();
}

void Audio_ToggleMute() {
    AudioSystem::Instance().ToggleMute();
}

void Audio_SetSFXVolume(int volume) {
    AudioSystem::Instance().SetSFXVolume(std::clamp(volume, 0, 255) / 255.0f);
}

int Audio_GetSFXVolume() {
    return static_cast<int>(AudioSystem::Instance().GetSFXVolume() * 255);
}

void Audio_SetMusicVolume(int volume) {
    AudioSystem::Instance().SetMusicVolume(std::clamp(volume, 0, 255) / 255.0f);
}

int Audio_GetMusicVolume() {
    return static_cast<int>(AudioSystem::Instance().GetMusicVolume() * 255);
}

void Audio_SetVoiceVolume(int volume) {
    AudioSystem::Instance().SetVoiceVolume(std::clamp(volume, 0, 255) / 255.0f);
}

int Audio_GetVoiceVolume() {
    return static_cast<int>(AudioSystem::Instance().GetVoiceVolume() * 255);
}

PlayHandle Audio_PlaySound(SoundEffect sfx) {
    return AudioSystem::Instance().PlaySound(sfx);
}

PlayHandle Audio_PlaySoundAt(SoundEffect sfx, int world_x, int world_y) {
    return AudioSystem::Instance().PlaySoundAt(sfx, world_x, world_y);
}

void Audio_StopAllSounds() {
    AudioSystem::Instance().StopAllSounds();
}

void Audio_PlayMusic(MusicTrack track) {
    AudioSystem::Instance().PlayMusic(track);
}

void Audio_StopMusic() {
    AudioSystem::Instance().StopMusic();
}

void Audio_PauseMusic() {
    AudioSystem::Instance().PauseMusic();
}

void Audio_ResumeMusic() {
    AudioSystem::Instance().ResumeMusic();
}

bool Audio_IsMusicPlaying() {
    return AudioSystem::Instance().IsMusicPlaying();
}

void Audio_PlayEva(EvaVoice voice) {
    AudioSystem::Instance().PlayEva(voice);
}

void Audio_PlayUnit(UnitVoice voice) {
    AudioSystem::Instance().PlayUnit(voice);
}

void Audio_StopAll() {
    AudioSystem::Instance().StopAll();
}
```

### src/test_audio_integration.cpp

```cpp
// src/test_audio_integration.cpp
// Integration test for unified audio system
// Task 17e - Audio Integration

#include "game/audio/audio_system.h"
#include "game/mix_manager.h"
#include "game/viewport.h"
#include "platform.h"
#include <cstdio>
#include <cstring>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { printf("  FAILED: %s\n", msg); return false; } } while(0)

#define RUN_TEST(name) \
    do { printf("Test: %s... ", #name); \
         if (Test_##name()) { printf("PASSED\n"); tests_passed++; } \
         else { tests_failed++; } } while(0)

bool Test_Initialize() {
    AudioSystem& audio = AudioSystem::Instance();

    // Should start uninitialized
    audio.Shutdown();
    TEST_ASSERT(!audio.IsInitialized(), "Should not be initialized");

    TEST_ASSERT(Audio_Init(), "Audio_Init should succeed");
    TEST_ASSERT(audio.IsInitialized(), "Should be initialized");

    return true;
}

bool Test_MasterVolume() {
    Audio_SetMasterVolume(128);
    int vol = Audio_GetMasterVolume();
    TEST_ASSERT(vol >= 120 && vol <= 136, "Master volume ~128");

    Audio_SetMasterVolume(255);
    TEST_ASSERT(Audio_GetMasterVolume() >= 250, "Master volume 255");

    return true;
}

bool Test_CategoryVolumes() {
    Audio_SetSFXVolume(200);
    TEST_ASSERT(Audio_GetSFXVolume() >= 195, "SFX volume");

    Audio_SetMusicVolume(150);
    TEST_ASSERT(Audio_GetMusicVolume() >= 145, "Music volume");

    Audio_SetVoiceVolume(180);
    TEST_ASSERT(Audio_GetVoiceVolume() >= 175, "Voice volume");

    // Reset
    Audio_SetSFXVolume(255);
    Audio_SetMusicVolume(200);
    Audio_SetVoiceVolume(255);

    return true;
}

bool Test_MuteControl() {
    TEST_ASSERT(!Audio_IsMuted(), "Should not be muted");

    Audio_SetMuted(true);
    TEST_ASSERT(Audio_IsMuted(), "Should be muted");

    Audio_ToggleMute();
    TEST_ASSERT(!Audio_IsMuted(), "Should not be muted after toggle");

    return true;
}

bool Test_Stats() {
    AudioStats stats = AudioSystem::Instance().GetStats();

    TEST_ASSERT(stats.master_volume >= 0 && stats.master_volume <= 1.0f, "Valid master volume");
    TEST_ASSERT(stats.sfx_volume >= 0 && stats.sfx_volume <= 1.0f, "Valid SFX volume");

    return true;
}

bool Test_StopAll() {
    // Should not crash
    Audio_StopAll();

    TEST_ASSERT(!Audio_IsMusicPlaying(), "Music should be stopped");

    return true;
}

void Interactive_Test() {
    printf("\n=== Interactive Audio Integration Test ===\n");
    printf("Controls:\n");
    printf("  1-3: Play sound effects\n");
    printf("  4: Play Hell March\n");
    printf("  5: Play Menu Music\n");
    printf("  6-8: Play EVA voices\n");
    printf("  9: Play Unit voice\n");
    printf("  P: Pause/Resume music\n");
    printf("  N: Next track\n");
    printf("  M: Toggle mute\n");
    printf("  +/-: Master volume\n");
    printf("  I: Print info\n");
    printf("  S: Stop all\n");
    printf("  ESC: Exit\n\n");

    Platform_Init();

    MixManager::Instance().Initialize();
    MixManager::Instance().LoadMix("SOUNDS.MIX");
    MixManager::Instance().LoadMix("SCORES.MIX");
    MixManager::Instance().LoadMix("SPEECH.MIX");

    Viewport::Instance().Initialize();
    Viewport::Instance().SetMapSize(64, 64);

    Audio_Init();
    AudioSystem::Instance().PrintDebugInfo();

    bool running = true;
    while (running) {
        Platform_PumpEvents();
        Platform_Input_Update();

        if (Platform_Key_JustPressed(KEY_CODE_ESCAPE)) {
            running = false;
            continue;
        }

        // Sound effects
        if (Platform_Key_JustPressed(KEY_CODE_1)) {
            printf("Playing: UI Click\n");
            Audio_PlaySound(SoundEffect::UI_CLICK);
        }
        if (Platform_Key_JustPressed(KEY_CODE_2)) {
            printf("Playing: Explosion\n");
            Audio_PlaySound(SoundEffect::EXPLODE_LARGE);
        }
        if (Platform_Key_JustPressed(KEY_CODE_3)) {
            printf("Playing: Cannon\n");
            Audio_PlaySound(SoundEffect::WEAPON_CANNON);
        }

        // Music
        if (Platform_Key_JustPressed(KEY_CODE_4)) {
            printf("Playing: Hell March\n");
            Audio_PlayMusic(MusicTrack::HELL_MARCH);
        }
        if (Platform_Key_JustPressed(KEY_CODE_5)) {
            printf("Playing: Menu\n");
            Audio_PlayMusic(MusicTrack::MENU);
        }

        // Voice
        if (Platform_Key_JustPressed(KEY_CODE_6)) {
            printf("EVA: Construction Complete\n");
            Audio_PlayEva(EvaVoice::CONSTRUCTION_COMPLETE);
        }
        if (Platform_Key_JustPressed(KEY_CODE_7)) {
            printf("EVA: Unit Ready\n");
            Audio_PlayEva(EvaVoice::UNIT_READY);
        }
        if (Platform_Key_JustPressed(KEY_CODE_8)) {
            printf("EVA: Base Under Attack\n");
            Audio_PlayEva(EvaVoice::BASE_UNDER_ATTACK);
        }
        if (Platform_Key_JustPressed(KEY_CODE_9)) {
            printf("Unit: Reporting\n");
            Audio_PlayUnit(UnitVoice::REPORTING);
        }

        // Controls
        if (Platform_Key_JustPressed(KEY_CODE_P)) {
            if (Audio_IsMusicPlaying()) {
                Audio_PauseMusic();
                printf("Music paused\n");
            } else {
                Audio_ResumeMusic();
                printf("Music resumed\n");
            }
        }
        if (Platform_Key_JustPressed(KEY_CODE_N)) {
            printf("Next track\n");
            AudioSystem::Instance().NextTrack();
        }

        if (Platform_Key_JustPressed(KEY_CODE_M)) {
            Audio_ToggleMute();
            printf("Muted: %s\n", Audio_IsMuted() ? "yes" : "no");
        }

        if (Platform_Key_JustPressed(KEY_CODE_EQUALS) || Platform_Key_JustPressed(KEY_CODE_PLUS)) {
            int vol = Audio_GetMasterVolume() + 25;
            Audio_SetMasterVolume(vol);
            printf("Master Volume: %d\n", Audio_GetMasterVolume());
        }
        if (Platform_Key_JustPressed(KEY_CODE_MINUS)) {
            int vol = Audio_GetMasterVolume() - 25;
            Audio_SetMasterVolume(vol);
            printf("Master Volume: %d\n", Audio_GetMasterVolume());
        }

        if (Platform_Key_JustPressed(KEY_CODE_I)) {
            AudioSystem::Instance().PrintDebugInfo();
        }

        if (Platform_Key_JustPressed(KEY_CODE_S)) {
            printf("Stopping all audio\n");
            Audio_StopAll();
        }

        Audio_Update();
        Platform_Timer_Delay(16);
    }

    Audio_Shutdown();
    MixManager::Instance().Shutdown();
    Platform_Shutdown();
}

int main(int argc, char* argv[]) {
    printf("=== Audio Integration Tests (Task 17e) ===\n\n");

    Platform_Init();

    AudioConfig cfg = {22050, 2, 16, 1024};
    Platform_Audio_Init(&cfg);

    MixManager::Instance().Initialize();
    Viewport::Instance().Initialize();

    RUN_TEST(Initialize);
    RUN_TEST(MasterVolume);
    RUN_TEST(CategoryVolumes);
    RUN_TEST(MuteControl);
    RUN_TEST(Stats);
    RUN_TEST(StopAll);

    Audio_Shutdown();
    MixManager::Instance().Shutdown();
    Platform_Audio_Shutdown();
    Platform_Shutdown();

    printf("\n=== Results: %d passed, %d failed ===\n", tests_passed, tests_failed);

    if (argc > 1 && strcmp(argv[1], "-i") == 0) {
        Interactive_Test();
    } else {
        printf("\nRun with -i for interactive test\n");
    }

    return (tests_failed == 0) ? 0 : 1;
}
```

---

## CMakeLists.txt Additions

```cmake
# Task 17e: Audio Integration
add_executable(TestAudioIntegration
    ${CMAKE_SOURCE_DIR}/src/test_audio_integration.cpp
    ${CMAKE_SOURCE_DIR}/src/game/audio/audio_system.cpp
    ${CMAKE_SOURCE_DIR}/src/game/audio/sound_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/game/audio/music_player.cpp
    ${CMAKE_SOURCE_DIR}/src/game/audio/voice_manager.cpp
    ${AUDIO_AUD_SOURCES}
)
target_include_directories(TestAudioIntegration PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(TestAudioIntegration PRIVATE redalert_platform)
add_test(NAME AudioIntegrationTests COMMAND TestAudioIntegration)
```

---

## Verification Script

```bash
#!/bin/bash
set -e
echo "=== Task 17e Verification: Audio Integration ==="

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"

echo "Checking files..."
for f in include/game/audio/audio_system.h src/game/audio/audio_system.cpp; do
    [ -f "$ROOT_DIR/$f" ] || { echo "Missing: $f"; exit 1; }
    echo "  ✓ $f"
done

echo "Building..."
cd "$BUILD_DIR" && cmake --build . --target TestAudioIntegration
echo "  ✓ Built"

echo "Running tests..."
"$BUILD_DIR/TestAudioIntegration" || { echo "Tests failed"; exit 1; }

echo "=== Task 17e Verification PASSED ==="
```

---

## Success Criteria

- [ ] AudioSystem singleton initializes all subsystems
- [ ] Master volume affects all audio
- [ ] Category volumes (SFX, Music, Voice) work independently
- [ ] Mute/unmute works correctly
- [ ] All convenience wrappers call correct subsystems
- [ ] StopAll stops everything
- [ ] Stats reporting works
- [ ] Interactive test demonstrates full functionality

---

## Completion Promise

When verification passes, output:
<promise>TASK_17E_COMPLETE</promise>

## Escape Hatch

If stuck after 15 iterations, document in `ralph-tasks/blocked/17e.md` and output:
<promise>TASK_17E_BLOCKED</promise>

## Max Iterations
15
