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
