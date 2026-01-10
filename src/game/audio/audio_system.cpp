// src/game/audio/audio_system.cpp
// Unified Audio System Implementation
// Task 17e - Audio Integration

#include "game/audio/audio_system.h"
#include "game/audio/sound_manager.h"
#include "game/audio/music_player.h"
#include "game/audio/voice_manager.h"
#include "platform.h"
#include <algorithm>
#include <cstdio>

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
        Platform_LogInfo("AudioSystem: Already initialized");
        return true;
    }

    Platform_LogInfo("AudioSystem: Initializing...");
    config_ = config;

    // Initialize platform audio
    AudioConfig platform_config;
    platform_config.sample_rate = config.sample_rate;
    platform_config.channels = config.channels;
    platform_config.bits_per_sample = 16;
    platform_config.buffer_size = config.buffer_size;

    if (Platform_Audio_Init(&platform_config) != 0) {
        Platform_LogInfo("AudioSystem: Failed to initialize platform audio");
        return false;
    }

    // Initialize sound manager
    SoundManagerConfig sfx_config;
    sfx_config.max_concurrent_sounds = config.max_concurrent_sounds;
    sfx_config.default_max_distance = config.max_audible_distance;

    if (!SoundManager::Instance().Initialize(sfx_config)) {
        Platform_LogInfo("AudioSystem: Failed to initialize SoundManager");
        Platform_Audio_Shutdown();
        return false;
    }

    // Initialize music player
    MusicPlayerConfig music_config;
    music_config.shuffle_enabled = config.music_shuffle;
    music_config.loop_enabled = config.music_loop;

    if (!MusicPlayer::Instance().Initialize(music_config)) {
        Platform_LogInfo("AudioSystem: Failed to initialize MusicPlayer");
        SoundManager::Instance().Shutdown();
        Platform_Audio_Shutdown();
        return false;
    }

    // Initialize voice manager
    if (!VoiceManager::Instance().Initialize()) {
        Platform_LogInfo("AudioSystem: Failed to initialize VoiceManager");
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
    Platform_LogInfo("AudioSystem: Initialized successfully");
    printf("  SFX Volume: %.0f%%\n", config.sfx_volume * 100);
    printf("  Music Volume: %.0f%%\n", config.music_volume * 100);
    printf("  Voice Volume: %.0f%%\n", config.voice_volume * 100);

    return true;
}

void AudioSystem::Shutdown() {
    if (!initialized_) {
        return;
    }

    Platform_LogInfo("AudioSystem: Shutting down...");

    StopAll();

    VoiceManager::Instance().Shutdown();
    MusicPlayer::Instance().Shutdown();
    SoundManager::Instance().Shutdown();
    Platform_Audio_Shutdown();

    initialized_ = false;
    Platform_LogInfo("AudioSystem: Shutdown complete");
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
    printf("=== Audio System Status ===\n");

    if (!initialized_) {
        printf("  Not initialized\n");
        return;
    }

    AudioStats stats = GetStats();

    printf("Master: %.0f%% %s\n",
                 stats.master_volume * 100,
                 stats.muted ? "(MUTED)" : "");

    printf("Volumes: SFX=%.0f%% Music=%.0f%% Voice=%.0f%%\n",
                 stats.sfx_volume * 100,
                 stats.music_volume * 100,
                 stats.voice_volume * 100);

    printf("Sounds: %d loaded, %d playing\n",
                 stats.loaded_sounds, stats.playing_sounds);

    printf("Music: %s - %s\n",
                 stats.music_playing ? "Playing" : "Stopped",
                 stats.current_track_name);

    printf("Voice: EVA=%s Unit=%s Queue=%d\n",
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
