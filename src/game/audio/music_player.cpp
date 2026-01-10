// src/game/audio/music_player.cpp
// Music player implementation
// Task 17c - Music System

#include "game/audio/music_player.h"
#include "game/audio/aud_file.h"
#include "platform.h"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <random>

//=============================================================================
// Music Track Info Table
//=============================================================================

static const MusicTrackInfo MUSIC_TRACK_INFO[] = {
    // NONE
    { nullptr, "None", false, 0.0f },

    // Menu/Special Tracks
    { "INTRO.AUD",    "Title Theme",      true, 1.0f },   // TITLE
    { "MENU.AUD",     "Main Menu",        true, 1.0f },   // MENU
    { "MAP.AUD",      "Map Selection",    true, 0.9f },   // MAP_SELECT
    { "SCORE.AUD",    "Score Screen",     true, 1.0f },   // SCORE_SCREEN
    { "CREDITS.AUD",  "Credits",          true, 1.0f },   // CREDITS

    // In-Game Tracks
    { "BIGFOOT.AUD",  "Bigfoot",          false, 1.0f },  // BIGFOOT
    { "CRUSH.AUD",    "Crush",            false, 1.0f },  // CRUSH
    { "FACE.AUD",     "Face the Enemy",   false, 1.0f },  // FACE
    { "HELLMRCH.AUD", "Hell March",       false, 1.0f },  // HELL_MARCH
    { "RUN.AUD",      "Run for Your Life",false, 1.0f },  // RUN
    { "SMASH.AUD",    "Smash",            false, 1.0f },  // SMASH
    { "TWINMIX.AUD",  "Twin Cannon Mix",  false, 1.0f },  // TWINMIX
    { "WARFARE.AUD",  "Warfare",          false, 1.0f },  // WARFARE
    { "DENSE.AUD",    "Dense",            false, 1.0f },  // DENSE
    { "VECTOR.AUD",   "Vector",           false, 1.0f },  // VECTOR
    { "WORKMEN.AUD",  "Workmen",          false, 1.0f },  // WORKMEN
    { "AWAIT.AUD",    "Await",            false, 1.0f },  // AWAIT
    { "GRINDER.AUD",  "Grinder",          false, 1.0f },  // GRINDER
    { "SHUTIT.AUD",   "Shut It",          false, 1.0f },  // SHUT_IT
    { "SNAKE.AUD",    "Snake",            false, 1.0f },  // SNAKE
};

static_assert(sizeof(MUSIC_TRACK_INFO) / sizeof(MUSIC_TRACK_INFO[0]) ==
              static_cast<size_t>(MusicTrack::COUNT),
              "MUSIC_TRACK_INFO size mismatch");

//=============================================================================
// Track Info Accessors
//=============================================================================

const MusicTrackInfo& GetMusicTrackInfo(MusicTrack track) {
    int idx = static_cast<int>(track);
    if (idx < 0 || idx >= static_cast<int>(MusicTrack::COUNT)) {
        return MUSIC_TRACK_INFO[0];
    }
    return MUSIC_TRACK_INFO[idx];
}

const char* GetMusicTrackFilename(MusicTrack track) {
    return GetMusicTrackInfo(track).filename;
}

const char* GetMusicTrackDisplayName(MusicTrack track) {
    return GetMusicTrackInfo(track).display_name;
}

bool IsMusicTrackInGame(MusicTrack track) {
    int idx = static_cast<int>(track);
    int first = static_cast<int>(MusicTrack::FIRST_INGAME);
    int last = static_cast<int>(MusicTrack::LAST_INGAME);
    return idx >= first && idx <= last;
}

MusicTrack GetNextMusicTrack(MusicTrack current) {
    if (!IsMusicTrackInGame(current)) {
        return MusicTrack::FIRST_INGAME;
    }

    int idx = static_cast<int>(current);
    idx++;

    if (idx > static_cast<int>(MusicTrack::LAST_INGAME)) {
        idx = static_cast<int>(MusicTrack::FIRST_INGAME);
    }

    return static_cast<MusicTrack>(idx);
}

MusicTrack GetRandomInGameTrack() {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    int first = static_cast<int>(MusicTrack::FIRST_INGAME);
    int last = static_cast<int>(MusicTrack::LAST_INGAME);

    std::uniform_int_distribution<> distrib(first, last);
    return static_cast<MusicTrack>(distrib(gen));
}

//=============================================================================
// MusicPlayer Implementation
//=============================================================================

MusicPlayer& MusicPlayer::Instance() {
    static MusicPlayer instance;
    return instance;
}

MusicPlayer::MusicPlayer()
    : state_(MusicState::STOPPED)
    , current_track_(MusicTrack::NONE)
    , pending_track_(MusicTrack::NONE)
    , sound_handle_(INVALID_SOUND_HANDLE)
    , play_handle_(INVALID_PLAY_HANDLE)
    , volume_(0.8f)
    , current_volume_(0.8f)
    , muted_(false)
    , initialized_(false)
    , fade_start_time_(0)
    , current_time_(0) {
}

MusicPlayer::~MusicPlayer() {
    Shutdown();
}

bool MusicPlayer::Initialize(const MusicPlayerConfig& config) {
    if (initialized_) {
        return true;
    }

    Platform_LogInfo("MusicPlayer: Initializing...");

    config_ = config;

    // Initialize shuffle playlist
    if (config_.shuffle_enabled) {
        InitShufflePlaylist();
    }

    initialized_ = true;
    Platform_LogInfo("MusicPlayer: Initialized");

    return true;
}

void MusicPlayer::Shutdown() {
    if (!initialized_) {
        return;
    }

    Platform_LogInfo("MusicPlayer: Shutting down...");

    Stop(false);
    UnloadTrack();

    shuffle_playlist_.clear();
    track_history_.clear();

    initialized_ = false;
}

//=============================================================================
// Track Loading
//=============================================================================

bool MusicPlayer::LoadTrack(MusicTrack track) {
    const char* filename = GetMusicTrackFilename(track);
    if (!filename) {
        return false;
    }

    // Unload previous track
    UnloadTrack();

    // Load AUD file using platform MIX API
    AudFile aud;
    if (!aud.LoadFromMix(filename)) {
        // Track not found - expected if game assets aren't present
        return false;
    }

    // Create platform sound
    sound_handle_ = Platform_Sound_CreateFromMemory(
        aud.GetPCMDataBytes(),
        static_cast<int32_t>(aud.GetPCMDataSize()),
        aud.GetSampleRate(),
        aud.GetChannels(),
        16
    );

    if (sound_handle_ == INVALID_SOUND_HANDLE) {
        return false;
    }

    return true;
}

void MusicPlayer::UnloadTrack() {
    if (play_handle_ != INVALID_PLAY_HANDLE) {
        Platform_Sound_Stop(play_handle_);
        play_handle_ = INVALID_PLAY_HANDLE;
    }

    if (sound_handle_ != INVALID_SOUND_HANDLE) {
        Platform_Sound_Destroy(sound_handle_);
        sound_handle_ = INVALID_SOUND_HANDLE;
    }
}

bool MusicPlayer::StartPlayback(bool loop) {
    if (sound_handle_ == INVALID_SOUND_HANDLE) {
        return false;
    }

    float vol = muted_ ? 0.0f : current_volume_;

    play_handle_ = Platform_Sound_Play(
        sound_handle_,
        vol,
        0.0f,  // No pan for music
        loop
    );

    return play_handle_ != INVALID_PLAY_HANDLE;
}

//=============================================================================
// Playback Control
//=============================================================================

bool MusicPlayer::Play(MusicTrack track, bool loop) {
    if (!initialized_) {
        return false;
    }

    // Same track already playing?
    if (track == current_track_ && IsPlaying()) {
        return true;
    }

    // Handle fade-out if configured
    if (config_.fade_duration_ms > 0 && IsPlaying()) {
        pending_track_ = track;
        state_ = MusicState::FADING_OUT;
        fade_start_time_ = current_time_;
        return true;
    }

    // Stop current track
    Stop(false);

    // Load and play new track
    if (!LoadTrack(track)) {
        return false;
    }

    current_track_ = track;
    config_.loop_enabled = loop;

    if (!StartPlayback(loop)) {
        UnloadTrack();
        current_track_ = MusicTrack::NONE;
        return false;
    }

    state_ = MusicState::PLAYING;
    current_volume_ = volume_;

    // Add to history
    track_history_.push_back(track);
    if (track_history_.size() > MAX_HISTORY) {
        track_history_.erase(track_history_.begin());
    }

    return true;
}

void MusicPlayer::Stop(bool fade) {
    if (!IsActive()) {
        return;
    }

    if (fade && config_.fade_duration_ms > 0) {
        pending_track_ = MusicTrack::NONE;
        state_ = MusicState::FADING_OUT;
        fade_start_time_ = current_time_;
        return;
    }

    if (play_handle_ != INVALID_PLAY_HANDLE) {
        Platform_Sound_Stop(play_handle_);
        play_handle_ = INVALID_PLAY_HANDLE;
    }

    state_ = MusicState::STOPPED;
}

void MusicPlayer::Pause() {
    if (state_ != MusicState::PLAYING) {
        return;
    }

    if (play_handle_ != INVALID_PLAY_HANDLE) {
        Platform_Sound_Pause(play_handle_);
    }

    state_ = MusicState::PAUSED;
}

void MusicPlayer::Resume() {
    if (state_ != MusicState::PAUSED) {
        return;
    }

    if (play_handle_ != INVALID_PLAY_HANDLE) {
        Platform_Sound_Resume(play_handle_);
    }

    state_ = MusicState::PLAYING;
}

void MusicPlayer::TogglePause() {
    if (IsPlaying()) {
        Pause();
    } else if (IsPaused()) {
        Resume();
    }
}

void MusicPlayer::PlayNext() {
    MusicTrack next = GetNextTrackToPlay();
    if (next != MusicTrack::NONE) {
        Play(next, config_.loop_enabled);
    }
}

void MusicPlayer::PlayPrevious() {
    if (track_history_.size() < 2) {
        return;  // No previous track
    }

    // Get second-to-last track (last is current)
    MusicTrack prev = track_history_[track_history_.size() - 2];
    track_history_.pop_back();  // Remove current
    track_history_.pop_back();  // Remove previous (will be re-added by Play)

    Play(prev, config_.loop_enabled);
}

void MusicPlayer::PlayRandom() {
    MusicTrack track = GetRandomInGameTrack();
    Play(track, config_.loop_enabled);
}

//=============================================================================
// State Queries
//=============================================================================

const char* MusicPlayer::GetCurrentTrackName() const {
    return GetMusicTrackDisplayName(current_track_);
}

//=============================================================================
// Volume Control
//=============================================================================

void MusicPlayer::SetVolume(float volume) {
    volume_ = std::clamp(volume, 0.0f, 1.0f);

    if (state_ == MusicState::PLAYING) {
        current_volume_ = volume_;
        ApplyVolume();
    }
}

void MusicPlayer::SetMuted(bool muted) {
    muted_ = muted;
    ApplyVolume();
}

void MusicPlayer::ApplyVolume() {
    if (play_handle_ != INVALID_PLAY_HANDLE) {
        float vol = muted_ ? 0.0f : current_volume_;
        Platform_Sound_SetVolume(play_handle_, vol);
    }
}

//=============================================================================
// Shuffle Mode
//=============================================================================

void MusicPlayer::SetShuffleEnabled(bool enabled) {
    config_.shuffle_enabled = enabled;
    if (enabled) {
        InitShufflePlaylist();
    }
}

void MusicPlayer::ResetShufflePlaylist() {
    InitShufflePlaylist();
}

void MusicPlayer::InitShufflePlaylist() {
    shuffle_playlist_.clear();

    for (int i = static_cast<int>(MusicTrack::FIRST_INGAME);
         i <= static_cast<int>(MusicTrack::LAST_INGAME);
         i++) {
        shuffle_playlist_.push_back(static_cast<MusicTrack>(i));
    }

    // Remove current track if playing
    if (current_track_ != MusicTrack::NONE) {
        auto it = std::find(shuffle_playlist_.begin(), shuffle_playlist_.end(), current_track_);
        if (it != shuffle_playlist_.end()) {
            shuffle_playlist_.erase(it);
        }
    }
}

MusicTrack MusicPlayer::PopRandomFromPlaylist() {
    if (shuffle_playlist_.empty()) {
        InitShufflePlaylist();
    }

    if (shuffle_playlist_.empty()) {
        return MusicTrack::NONE;
    }

    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::uniform_int_distribution<size_t> distrib(0, shuffle_playlist_.size() - 1);
    size_t idx = distrib(gen);

    MusicTrack track = shuffle_playlist_[idx];
    shuffle_playlist_.erase(shuffle_playlist_.begin() + static_cast<std::ptrdiff_t>(idx));

    return track;
}

//=============================================================================
// Loop/Auto-Advance
//=============================================================================

void MusicPlayer::SetLoopEnabled(bool enabled) {
    config_.loop_enabled = enabled;
}

void MusicPlayer::SetAutoAdvance(bool enabled) {
    config_.auto_advance = enabled;
}

//=============================================================================
// Update
//=============================================================================

void MusicPlayer::Update() {
    if (!initialized_) {
        return;
    }

    current_time_ = Platform_GetTicks();

    // Update fade effect
    if (state_ == MusicState::FADING_OUT || state_ == MusicState::FADING_IN) {
        UpdateFade();
    }

    // Check for track completion
    if (state_ == MusicState::PLAYING) {
        if (play_handle_ != INVALID_PLAY_HANDLE &&
            !Platform_Sound_IsPlaying(play_handle_)) {
            OnTrackComplete();
        }
    }
}

void MusicPlayer::UpdateFade() {
    if (config_.fade_duration_ms <= 0) {
        return;
    }

    float elapsed = static_cast<float>(current_time_ - fade_start_time_);
    float progress = elapsed / config_.fade_duration_ms;
    progress = std::clamp(progress, 0.0f, 1.0f);

    if (state_ == MusicState::FADING_OUT) {
        current_volume_ = volume_ * (1.0f - progress);
        ApplyVolume();

        if (progress >= 1.0f) {
            // Fade-out complete
            Stop(false);

            if (pending_track_ != MusicTrack::NONE) {
                // Start next track with fade-in
                MusicTrack next = pending_track_;
                pending_track_ = MusicTrack::NONE;

                if (LoadTrack(next)) {
                    current_track_ = next;
                    current_volume_ = 0.0f;
                    StartPlayback(config_.loop_enabled);
                    state_ = MusicState::FADING_IN;
                    fade_start_time_ = current_time_;
                }
            }
        }
    }
    else if (state_ == MusicState::FADING_IN) {
        current_volume_ = volume_ * progress;
        ApplyVolume();

        if (progress >= 1.0f) {
            current_volume_ = volume_;
            state_ = MusicState::PLAYING;
        }
    }
}

void MusicPlayer::OnTrackComplete() {
    if (config_.auto_advance && IsMusicTrackInGame(current_track_)) {
        PlayNext();
    } else {
        state_ = MusicState::STOPPED;
    }
}

MusicTrack MusicPlayer::GetNextTrackToPlay() {
    if (config_.shuffle_enabled) {
        return PopRandomFromPlaylist();
    } else {
        return GetNextMusicTrack(current_track_);
    }
}

//=============================================================================
// Debug
//=============================================================================

void MusicPlayer::PrintStatus() const {
    const char* state_str = "Unknown";
    switch (state_) {
        case MusicState::STOPPED:    state_str = "Stopped"; break;
        case MusicState::PLAYING:    state_str = "Playing"; break;
        case MusicState::PAUSED:     state_str = "Paused"; break;
        case MusicState::FADING_OUT: state_str = "Fading Out"; break;
        case MusicState::FADING_IN:  state_str = "Fading In"; break;
    }

    printf("MusicPlayer Status:\n");
    printf("  State: %s\n", state_str);
    printf("  Track: %s\n", GetCurrentTrackName());
    printf("  Volume: %.0f%%\n", volume_ * 100);
    printf("  Muted: %s\n", muted_ ? "yes" : "no");
    printf("  Shuffle: %s\n", config_.shuffle_enabled ? "on" : "off");
    printf("  Loop: %s\n", config_.loop_enabled ? "on" : "off");
    printf("  Auto-Advance: %s\n", config_.auto_advance ? "on" : "off");
}

//=============================================================================
// Global Convenience Functions
//=============================================================================

bool Music_Init() {
    return MusicPlayer::Instance().Initialize();
}

void Music_Shutdown() {
    MusicPlayer::Instance().Shutdown();
}

void Music_Update() {
    MusicPlayer::Instance().Update();
}

void Music_Play(MusicTrack track) {
    MusicPlayer::Instance().Play(track);
}

void Music_Stop() {
    MusicPlayer::Instance().Stop();
}

void Music_Pause() {
    MusicPlayer::Instance().Pause();
}

void Music_Resume() {
    MusicPlayer::Instance().Resume();
}

bool Music_IsPlaying() {
    return MusicPlayer::Instance().IsPlaying();
}

void Music_SetVolume(int volume) {
    float vol = std::clamp(volume, 0, 255) / 255.0f;
    MusicPlayer::Instance().SetVolume(vol);
}

int Music_GetVolume() {
    return static_cast<int>(MusicPlayer::Instance().GetVolume() * 255);
}

void Music_SetShuffle(bool enabled) {
    MusicPlayer::Instance().SetShuffleEnabled(enabled);
}
