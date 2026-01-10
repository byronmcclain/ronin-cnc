# Task 17c: Music System

| Field | Value |
|-------|-------|
| **Task ID** | 17c |
| **Task Name** | Music System |
| **Phase** | 17 - Audio Integration |
| **Depends On** | Task 17a (AUD File Parser) |
| **Estimated Complexity** | Medium (~350 lines) |

---

## Dependencies

- Task 17a (AUD File Parser) must be complete:
  - `AudFile` class for loading music tracks
- MixManager for locating music in MIX archives:
  - Music typically stored in `SCORES.MIX`
- Platform audio API:
  - Looping playback support
  - Pause/resume functionality

---

## Context

The Music System manages background music playback for Red Alert. Unlike sound effects, music requires:

1. **Streaming/Long Playback**: Music tracks are several minutes long
2. **Looping**: Tracks typically loop until changed
3. **Track Management**: Title, menu, and in-game music are different
4. **Shuffle Mode**: In-game music can shuffle through available tracks
5. **Crossfade** (optional): Smooth transitions between tracks
6. **Single Active Track**: Only one music track plays at a time

Red Alert has iconic music including "Hell March" and other memorable tracks stored as AUD files in SCORES.MIX.

---

## Objective

Create a Music System that:
1. Defines all music tracks as an enum
2. Loads and plays music tracks from MIX archives
3. Supports play, stop, pause, resume operations
4. Implements looping and auto-advance to next track
5. Provides shuffle mode for in-game music variety
6. Has independent volume control from SFX
7. Handles track completion callbacks

---

## Technical Background

### Music Tracks in Red Alert

Music files are stored in `SCORES.MIX`:

**Menu/Special Tracks:**
- `INTRO.AUD` - Title screen theme
- `MENU.AUD` - Main menu
- `MAP.AUD` - Map selection
- `SCORE.AUD` - Score screen

**In-Game Tracks:**
- `BIGFOOT.AUD` - Bigfoot
- `CRUSH.AUD` - Crush
- `FACE.AUD` - Face the Enemy
- `HELLMRCH.AUD` - Hell March (iconic!)
- `RUN.AUD` - Run for Your Life
- `SMASH.AUD` - Smash
- `TWINMIX.AUD` - Twin Cannon Mix
- `WARFARE.AUD` - Warfare

### Music Playback Flow

```
User calls Play(MUSIC_HELL_MARCH)
    ↓
Check if already playing → if same track, do nothing
    ↓
Stop current track (with optional fade)
    ↓
Load new track from MIX
    ↓
Start playback with looping
    ↓
On track end:
    If looping: restart
    If shuffle: pick random track
    If sequential: play next track
```

### Shuffle Algorithm

For shuffle mode, maintain a "playlist" of unplayed tracks. When a track finishes:
1. Remove current track from playlist
2. If playlist empty, refill with all in-game tracks
3. Pick random track from remaining playlist
4. Play that track

This ensures all tracks play before repeating.

---

## Deliverables

- [ ] `include/game/audio/music_track.h` - Music track enumeration
- [ ] `include/game/audio/music_player.h` - MusicPlayer class declaration
- [ ] `src/game/audio/music_player.cpp` - Implementation
- [ ] `src/test_music_player.cpp` - Unit tests and interactive test
- [ ] CMakeLists.txt updated
- [ ] All tests pass

---

## Files to Create

### include/game/audio/music_track.h

```cpp
// include/game/audio/music_track.h
// Music track enumeration
// Task 17c - Music System

#ifndef MUSIC_TRACK_H
#define MUSIC_TRACK_H

#include <cstdint>

//=============================================================================
// Music Track Enumeration
//=============================================================================

enum class MusicTrack : int16_t {
    NONE = 0,

    //=========================================================================
    // Menu/Special Tracks
    //=========================================================================
    TITLE,          // INTRO.AUD - Title screen theme
    MENU,           // MENU.AUD - Main menu music
    MAP_SELECT,     // MAP.AUD - Map selection screen
    SCORE_SCREEN,   // SCORE.AUD - End mission score screen
    CREDITS,        // CREDITS.AUD - Credits music (if exists)

    //=========================================================================
    // In-Game Tracks (for shuffle play)
    //=========================================================================
    BIGFOOT,        // BIGFOOT.AUD
    CRUSH,          // CRUSH.AUD
    FACE,           // FACE.AUD - Face the Enemy
    HELL_MARCH,     // HELLMRCH.AUD - Iconic track!
    RUN,            // RUN.AUD - Run for Your Life
    SMASH,          // SMASH.AUD
    TWINMIX,        // TWINMIX.AUD - Twin Cannon Mix
    WARFARE,        // WARFARE.AUD

    // Additional tracks (expansion packs, etc.)
    DENSE,          // DENSE.AUD
    VECTOR,         // VECTOR.AUD
    WORKMEN,        // WORKMEN.AUD
    AWAIT,          // AWAIT.AUD
    GRINDER,        // GRINDER.AUD
    SHUT_IT,        // SHUTIT.AUD
    SNAKE,          // SNAKE.AUD

    // Sentinel
    COUNT,

    // Special values for range checking
    FIRST_INGAME = BIGFOOT,
    LAST_INGAME = SNAKE
};

//=============================================================================
// Music Track Info
//=============================================================================

struct MusicTrackInfo {
    const char* filename;       // AUD filename in SCORES.MIX
    const char* display_name;   // Human-readable name
    bool is_menu_track;         // True for menu/special tracks
    float default_volume;       // Default volume multiplier
};

/// Get info for a music track
const MusicTrackInfo& GetMusicTrackInfo(MusicTrack track);

/// Get filename for a music track
const char* GetMusicTrackFilename(MusicTrack track);

/// Get display name for a music track
const char* GetMusicTrackDisplayName(MusicTrack track);

/// Check if track is an in-game track (for shuffle)
bool IsMusicTrackInGame(MusicTrack track);

/// Get the next track in sequence
MusicTrack GetNextMusicTrack(MusicTrack current);

/// Get a random in-game track
MusicTrack GetRandomInGameTrack();

#endif // MUSIC_TRACK_H
```

### include/game/audio/music_player.h

```cpp
// include/game/audio/music_player.h
// Music player for background music
// Task 17c - Music System

#ifndef MUSIC_PLAYER_H
#define MUSIC_PLAYER_H

#include "game/audio/music_track.h"
#include "platform.h"
#include <vector>
#include <cstdint>

//=============================================================================
// Music Player Configuration
//=============================================================================

struct MusicPlayerConfig {
    bool shuffle_enabled = false;   // Random track selection
    bool loop_enabled = true;       // Loop tracks
    bool auto_advance = true;       // Auto-play next track when done
    float fade_duration_ms = 0.0f;  // Crossfade duration (0 = instant)
};

//=============================================================================
// Music Player State
//=============================================================================

enum class MusicState : uint8_t {
    STOPPED,        // No music playing
    PLAYING,        // Music is playing
    PAUSED,         // Music is paused
    FADING_OUT,     // Fading out before stop/switch
    FADING_IN       // Fading in after switch
};

//=============================================================================
// Music Player Class
//=============================================================================

class MusicPlayer {
public:
    /// Singleton instance
    static MusicPlayer& Instance();

    //=========================================================================
    // Lifecycle
    //=========================================================================

    /// Initialize music player
    bool Initialize(const MusicPlayerConfig& config = MusicPlayerConfig());

    /// Shutdown and release resources
    void Shutdown();

    /// Check if initialized
    bool IsInitialized() const { return initialized_; }

    //=========================================================================
    // Playback Control
    //=========================================================================

    /// Play a music track
    /// @param track Track to play
    /// @param loop Whether to loop (default true)
    /// @return true if track started successfully
    bool Play(MusicTrack track, bool loop = true);

    /// Stop current music
    /// @param fade If true and fade_duration > 0, fade out first
    void Stop(bool fade = false);

    /// Pause current music
    void Pause();

    /// Resume paused music
    void Resume();

    /// Toggle pause state
    void TogglePause();

    /// Play next track (based on shuffle/sequential mode)
    void PlayNext();

    /// Play previous track
    void PlayPrevious();

    /// Play a random in-game track
    void PlayRandom();

    //=========================================================================
    // State Queries
    //=========================================================================

    /// Get current playback state
    MusicState GetState() const { return state_; }

    /// Check if music is playing
    bool IsPlaying() const { return state_ == MusicState::PLAYING; }

    /// Check if music is paused
    bool IsPaused() const { return state_ == MusicState::PAUSED; }

    /// Check if any track is active (playing or paused)
    bool IsActive() const { return state_ != MusicState::STOPPED; }

    /// Get currently playing/paused track
    MusicTrack GetCurrentTrack() const { return current_track_; }

    /// Get display name of current track
    const char* GetCurrentTrackName() const;

    //=========================================================================
    // Volume Control
    //=========================================================================

    /// Set music volume (0.0 - 1.0)
    void SetVolume(float volume);

    /// Get music volume
    float GetVolume() const { return volume_; }

    /// Mute/unmute music
    void SetMuted(bool muted);

    /// Check if muted
    bool IsMuted() const { return muted_; }

    //=========================================================================
    // Shuffle Mode
    //=========================================================================

    /// Enable/disable shuffle mode
    void SetShuffleEnabled(bool enabled);

    /// Check if shuffle is enabled
    bool IsShuffleEnabled() const { return config_.shuffle_enabled; }

    /// Reset shuffle playlist (start fresh)
    void ResetShufflePlaylist();

    //=========================================================================
    // Loop Control
    //=========================================================================

    /// Enable/disable looping
    void SetLoopEnabled(bool enabled);

    /// Check if looping is enabled
    bool IsLoopEnabled() const { return config_.loop_enabled; }

    //=========================================================================
    // Auto-Advance
    //=========================================================================

    /// Enable/disable auto-advance to next track
    void SetAutoAdvance(bool enabled);

    /// Check if auto-advance is enabled
    bool IsAutoAdvanceEnabled() const { return config_.auto_advance; }

    //=========================================================================
    // Per-Frame Update
    //=========================================================================

    /// Update music player (call each frame)
    /// Handles fade effects and track completion
    void Update();

    //=========================================================================
    // Debug
    //=========================================================================

    /// Print current status
    void PrintStatus() const;

private:
    MusicPlayer();
    ~MusicPlayer();
    MusicPlayer(const MusicPlayer&) = delete;
    MusicPlayer& operator=(const MusicPlayer&) = delete;

    //=========================================================================
    // Internal Data
    //=========================================================================

    MusicPlayerConfig config_;
    MusicState state_;
    MusicTrack current_track_;
    MusicTrack pending_track_;  // Track to play after fade-out

    SoundHandle sound_handle_;  // Loaded sound data
    PlayHandle play_handle_;    // Currently playing instance

    float volume_;
    float current_volume_;  // For fading
    bool muted_;
    bool initialized_;

    // Shuffle playlist (tracks not yet played in this cycle)
    std::vector<MusicTrack> shuffle_playlist_;

    // Track history for previous track function
    std::vector<MusicTrack> track_history_;
    static constexpr int MAX_HISTORY = 10;

    // Timing
    uint64_t fade_start_time_;
    uint64_t current_time_;

    //=========================================================================
    // Internal Methods
    //=========================================================================

    /// Load a track into memory
    bool LoadTrack(MusicTrack track);

    /// Unload current track
    void UnloadTrack();

    /// Actually start playback of loaded track
    bool StartPlayback(bool loop);

    /// Handle track completion
    void OnTrackComplete();

    /// Get next track based on mode
    MusicTrack GetNextTrackToPlay();

    /// Initialize shuffle playlist with all in-game tracks
    void InitShufflePlaylist();

    /// Pop a random track from shuffle playlist
    MusicTrack PopRandomFromPlaylist();

    /// Update fade effect
    void UpdateFade();

    /// Apply current volume (including fade)
    void ApplyVolume();
};

//=============================================================================
// Global Convenience Functions
//=============================================================================

/// Initialize music player
bool Music_Init();

/// Shutdown music player
void Music_Shutdown();

/// Update music player (call each frame)
void Music_Update();

/// Play a music track
void Music_Play(MusicTrack track);

/// Stop music
void Music_Stop();

/// Pause music
void Music_Pause();

/// Resume music
void Music_Resume();

/// Check if music is playing
bool Music_IsPlaying();

/// Set music volume (0-255 for compatibility)
void Music_SetVolume(int volume);

/// Get music volume (0-255)
int Music_GetVolume();

/// Enable/disable shuffle
void Music_SetShuffle(bool enabled);

#endif // MUSIC_PLAYER_H
```

### src/game/audio/music_player.cpp

```cpp
// src/game/audio/music_player.cpp
// Music player implementation
// Task 17c - Music System

#include "game/audio/music_player.h"
#include "game/audio/aud_file.h"
#include "game/mix_manager.h"
#include "platform.h"
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

    Platform_Log("MusicPlayer: Initializing...");

    config_ = config;

    // Initialize shuffle playlist
    if (config_.shuffle_enabled) {
        InitShufflePlaylist();
    }

    initialized_ = true;
    Platform_Log("MusicPlayer: Initialized");

    return true;
}

void MusicPlayer::Shutdown() {
    if (!initialized_) {
        return;
    }

    Platform_Log("MusicPlayer: Shutting down...");

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
        Platform_Log("MusicPlayer: Invalid track");
        return false;
    }

    // Unload previous track
    UnloadTrack();

    // Load AUD file
    AudFile aud;
    if (!aud.LoadFromMix(filename)) {
        Platform_Log("MusicPlayer: Failed to load %s", filename);
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
        Platform_Log("MusicPlayer: Failed to create sound for %s", filename);
        return false;
    }

    Platform_Log("MusicPlayer: Loaded %s (%.1f seconds)",
                 GetMusicTrackDisplayName(track), aud.GetDurationSeconds());

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

    Platform_Log("MusicPlayer: Now playing: %s", GetMusicTrackDisplayName(track));

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
    Platform_Log("MusicPlayer: Stopped");
}

void MusicPlayer::Pause() {
    if (state_ != MusicState::PLAYING) {
        return;
    }

    if (play_handle_ != INVALID_PLAY_HANDLE) {
        Platform_Sound_Pause(play_handle_);
    }

    state_ = MusicState::PAUSED;
    Platform_Log("MusicPlayer: Paused");
}

void MusicPlayer::Resume() {
    if (state_ != MusicState::PAUSED) {
        return;
    }

    if (play_handle_ != INVALID_PLAY_HANDLE) {
        Platform_Sound_Resume(play_handle_);
    }

    state_ = MusicState::PLAYING;
    Platform_Log("MusicPlayer: Resumed");
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
    shuffle_playlist_.erase(shuffle_playlist_.begin() + idx);

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
    Platform_Log("MusicPlayer: Track complete: %s", GetMusicTrackDisplayName(current_track_));

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

    Platform_Log("MusicPlayer Status:");
    Platform_Log("  State: %s", state_str);
    Platform_Log("  Track: %s", GetCurrentTrackName());
    Platform_Log("  Volume: %.0f%%", volume_ * 100);
    Platform_Log("  Muted: %s", muted_ ? "yes" : "no");
    Platform_Log("  Shuffle: %s", config_.shuffle_enabled ? "on" : "off");
    Platform_Log("  Loop: %s", config_.loop_enabled ? "on" : "off");
    Platform_Log("  Auto-Advance: %s", config_.auto_advance ? "on" : "off");
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
```

### src/test_music_player.cpp

```cpp
// src/test_music_player.cpp
// Unit tests for Music Player
// Task 17c - Music System

#include "game/audio/music_player.h"
#include "game/audio/aud_file.h"
#include "game/mix_manager.h"
#include "platform.h"
#include <cstdio>
#include <cstring>
#include <cmath>

//=============================================================================
// Test Utilities
//=============================================================================

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("  FAILED: %s\n", msg); \
            return false; \
        } \
    } while(0)

#define RUN_TEST(name) \
    do { \
        printf("Test: %s... ", #name); \
        if (Test_##name()) { \
            printf("PASSED\n"); \
            tests_passed++; \
        } else { \
            tests_failed++; \
        } \
    } while(0)

//=============================================================================
// Unit Tests
//=============================================================================

bool Test_MusicTrackEnum() {
    TEST_ASSERT(static_cast<int>(MusicTrack::NONE) == 0, "NONE should be 0");
    TEST_ASSERT(static_cast<int>(MusicTrack::COUNT) > 10, "Should have 10+ tracks");
    TEST_ASSERT(static_cast<int>(MusicTrack::HELL_MARCH) > 0, "HELL_MARCH should exist");

    return true;
}

bool Test_MusicTrackInfo() {
    const MusicTrackInfo& info = GetMusicTrackInfo(MusicTrack::HELL_MARCH);
    TEST_ASSERT(info.filename != nullptr, "HELL_MARCH should have filename");
    TEST_ASSERT(strstr(info.filename, "HELLMRCH") != nullptr, "Should contain HELLMRCH");
    TEST_ASSERT(!info.is_menu_track, "HELL_MARCH is not a menu track");

    const MusicTrackInfo& menu_info = GetMusicTrackInfo(MusicTrack::MENU);
    TEST_ASSERT(menu_info.is_menu_track, "MENU should be a menu track");

    return true;
}

bool Test_InGameTrackCheck() {
    TEST_ASSERT(IsMusicTrackInGame(MusicTrack::HELL_MARCH), "HELL_MARCH is in-game");
    TEST_ASSERT(IsMusicTrackInGame(MusicTrack::BIGFOOT), "BIGFOOT is in-game");
    TEST_ASSERT(!IsMusicTrackInGame(MusicTrack::MENU), "MENU is not in-game");
    TEST_ASSERT(!IsMusicTrackInGame(MusicTrack::TITLE), "TITLE is not in-game");

    return true;
}

bool Test_NextTrack() {
    MusicTrack next = GetNextMusicTrack(MusicTrack::BIGFOOT);
    TEST_ASSERT(next != MusicTrack::BIGFOOT, "Next should be different");
    TEST_ASSERT(IsMusicTrackInGame(next), "Next should be in-game track");

    // Test wrap-around
    MusicTrack last = MusicTrack::LAST_INGAME;
    MusicTrack wrapped = GetNextMusicTrack(last);
    TEST_ASSERT(wrapped == MusicTrack::FIRST_INGAME, "Should wrap to first");

    return true;
}

bool Test_RandomTrack() {
    // Test that random tracks are valid in-game tracks
    for (int i = 0; i < 10; i++) {
        MusicTrack random = GetRandomInGameTrack();
        TEST_ASSERT(IsMusicTrackInGame(random), "Random should be in-game track");
    }

    return true;
}

bool Test_VolumeControl() {
    MusicPlayer& player = MusicPlayer::Instance();

    if (!player.IsInitialized()) {
        player.Initialize();
    }

    player.SetVolume(0.5f);
    TEST_ASSERT(std::abs(player.GetVolume() - 0.5f) < 0.01f, "Volume should be 0.5");

    player.SetVolume(1.0f);
    TEST_ASSERT(std::abs(player.GetVolume() - 1.0f) < 0.01f, "Volume should be 1.0");

    // Test clamping
    player.SetVolume(2.0f);
    TEST_ASSERT(player.GetVolume() <= 1.0f, "Should clamp to 1.0");

    player.SetVolume(0.8f);  // Reset

    return true;
}

bool Test_MuteControl() {
    MusicPlayer& player = MusicPlayer::Instance();

    if (!player.IsInitialized()) {
        player.Initialize();
    }

    TEST_ASSERT(!player.IsMuted(), "Should not be muted initially");

    player.SetMuted(true);
    TEST_ASSERT(player.IsMuted(), "Should be muted");

    player.SetMuted(false);
    TEST_ASSERT(!player.IsMuted(), "Should not be muted");

    return true;
}

bool Test_ShuffleMode() {
    MusicPlayer& player = MusicPlayer::Instance();

    if (!player.IsInitialized()) {
        player.Initialize();
    }

    player.SetShuffleEnabled(true);
    TEST_ASSERT(player.IsShuffleEnabled(), "Shuffle should be enabled");

    player.SetShuffleEnabled(false);
    TEST_ASSERT(!player.IsShuffleEnabled(), "Shuffle should be disabled");

    return true;
}

bool Test_LoopMode() {
    MusicPlayer& player = MusicPlayer::Instance();

    if (!player.IsInitialized()) {
        player.Initialize();
    }

    player.SetLoopEnabled(false);
    TEST_ASSERT(!player.IsLoopEnabled(), "Loop should be disabled");

    player.SetLoopEnabled(true);
    TEST_ASSERT(player.IsLoopEnabled(), "Loop should be enabled");

    return true;
}

bool Test_InitialState() {
    MusicPlayer& player = MusicPlayer::Instance();

    // Reinitialize for clean state
    player.Shutdown();
    player.Initialize();

    TEST_ASSERT(player.GetState() == MusicState::STOPPED, "Should start stopped");
    TEST_ASSERT(player.GetCurrentTrack() == MusicTrack::NONE, "Should have no track");
    TEST_ASSERT(!player.IsPlaying(), "Should not be playing");

    return true;
}

bool Test_GlobalFunctions() {
    Music_SetVolume(128);
    int vol = Music_GetVolume();
    TEST_ASSERT(vol >= 120 && vol <= 136, "Volume should be ~128");

    Music_SetShuffle(true);
    Music_SetShuffle(false);

    return true;
}

//=============================================================================
// Integration Tests (require game assets)
//=============================================================================

bool Test_LoadAndPlayTrack() {
    printf("\n--- Integration Test: Load and Play Track ---\n");

    MixManager& mix = MixManager::Instance();
    if (!mix.HasAnyMixLoaded()) {
        mix.LoadMix("SCORES.MIX");
    }

    MusicPlayer& player = MusicPlayer::Instance();
    player.Shutdown();
    player.Initialize();

    // Try to play Hell March
    bool success = player.Play(MusicTrack::HELL_MARCH);

    if (success) {
        printf("  ✓ Successfully started: %s\n", player.GetCurrentTrackName());
        TEST_ASSERT(player.IsPlaying(), "Should be playing");
        TEST_ASSERT(player.GetCurrentTrack() == MusicTrack::HELL_MARCH, "Track mismatch");

        // Stop after brief test
        player.Stop();
        TEST_ASSERT(!player.IsPlaying(), "Should be stopped");
    } else {
        printf("  SKIPPED: Could not load track (game assets may not be present)\n");
    }

    return true;
}

//=============================================================================
// Interactive Test
//=============================================================================

void Interactive_Test() {
    printf("\n=== Interactive Music Player Test ===\n");
    printf("Controls:\n");
    printf("  1: Play Hell March\n");
    printf("  2: Play Menu Music\n");
    printf("  3: Play Random Track\n");
    printf("  N: Next Track\n");
    printf("  P: Previous Track\n");
    printf("  SPACE: Pause/Resume\n");
    printf("  S: Toggle Shuffle\n");
    printf("  +/-: Volume\n");
    printf("  M: Mute\n");
    printf("  I: Print Status\n");
    printf("  ESC: Exit\n\n");

    Platform_Init();

    AudioConfig audio_config = {};
    audio_config.sample_rate = 22050;
    audio_config.channels = 2;
    audio_config.bits_per_sample = 16;
    audio_config.buffer_size = 2048;

    if (Platform_Audio_Init(&audio_config) != 0) {
        printf("Failed to initialize audio\n");
        Platform_Shutdown();
        return;
    }

    MixManager::Instance().Initialize();
    MixManager::Instance().LoadMix("SCORES.MIX");

    MusicPlayerConfig config;
    config.shuffle_enabled = false;
    config.loop_enabled = true;
    config.auto_advance = true;
    Music_Init();

    MusicPlayer& player = MusicPlayer::Instance();
    player.PrintStatus();

    bool running = true;
    while (running) {
        Platform_PumpEvents();
        Platform_Input_Update();

        if (Platform_Key_JustPressed(KEY_CODE_ESCAPE)) {
            running = false;
            continue;
        }

        if (Platform_Key_JustPressed(KEY_CODE_1)) {
            printf("Playing Hell March...\n");
            player.Play(MusicTrack::HELL_MARCH);
        }
        if (Platform_Key_JustPressed(KEY_CODE_2)) {
            printf("Playing Menu Music...\n");
            player.Play(MusicTrack::MENU);
        }
        if (Platform_Key_JustPressed(KEY_CODE_3)) {
            printf("Playing Random...\n");
            player.PlayRandom();
        }

        if (Platform_Key_JustPressed(KEY_CODE_N)) {
            printf("Next track...\n");
            player.PlayNext();
        }
        if (Platform_Key_JustPressed(KEY_CODE_P)) {
            printf("Previous track...\n");
            player.PlayPrevious();
        }

        if (Platform_Key_JustPressed(KEY_CODE_SPACE)) {
            player.TogglePause();
            printf("%s\n", player.IsPaused() ? "Paused" : "Resumed");
        }

        if (Platform_Key_JustPressed(KEY_CODE_S)) {
            player.SetShuffleEnabled(!player.IsShuffleEnabled());
            printf("Shuffle: %s\n", player.IsShuffleEnabled() ? "ON" : "OFF");
        }

        if (Platform_Key_JustPressed(KEY_CODE_EQUALS) || Platform_Key_JustPressed(KEY_CODE_PLUS)) {
            float vol = player.GetVolume() + 0.1f;
            player.SetVolume(vol);
            printf("Volume: %.0f%%\n", player.GetVolume() * 100);
        }
        if (Platform_Key_JustPressed(KEY_CODE_MINUS)) {
            float vol = player.GetVolume() - 0.1f;
            player.SetVolume(vol);
            printf("Volume: %.0f%%\n", player.GetVolume() * 100);
        }

        if (Platform_Key_JustPressed(KEY_CODE_M)) {
            player.SetMuted(!player.IsMuted());
            printf("Muted: %s\n", player.IsMuted() ? "yes" : "no");
        }

        if (Platform_Key_JustPressed(KEY_CODE_I)) {
            player.PrintStatus();
        }

        Music_Update();
        Platform_Timer_Delay(16);
    }

    Music_Shutdown();
    MixManager::Instance().Shutdown();
    Platform_Audio_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Main
//=============================================================================

int main(int argc, char* argv[]) {
    printf("=== Music Player Tests (Task 17c) ===\n\n");

    Platform_Init();

    AudioConfig audio_config = {};
    audio_config.sample_rate = 22050;
    audio_config.channels = 2;
    audio_config.bits_per_sample = 16;
    audio_config.buffer_size = 2048;
    Platform_Audio_Init(&audio_config);

    MixManager::Instance().Initialize();

    // Run unit tests
    RUN_TEST(MusicTrackEnum);
    RUN_TEST(MusicTrackInfo);
    RUN_TEST(InGameTrackCheck);
    RUN_TEST(NextTrack);
    RUN_TEST(RandomTrack);
    RUN_TEST(VolumeControl);
    RUN_TEST(MuteControl);
    RUN_TEST(ShuffleMode);
    RUN_TEST(LoopMode);
    RUN_TEST(InitialState);
    RUN_TEST(GlobalFunctions);

    // Integration test
    RUN_TEST(LoadAndPlayTrack);

    // Cleanup
    MusicPlayer::Instance().Shutdown();
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
# =============================================================================
# Audio Integration - Task 17c: Music Player
# =============================================================================

set(AUDIO_MUSIC_PLAYER_SOURCES
    ${CMAKE_SOURCE_DIR}/src/game/audio/music_player.cpp
)

set(AUDIO_MUSIC_PLAYER_HEADERS
    ${CMAKE_SOURCE_DIR}/include/game/audio/music_track.h
    ${CMAKE_SOURCE_DIR}/include/game/audio/music_player.h
)

add_executable(TestMusicPlayer
    ${CMAKE_SOURCE_DIR}/src/test_music_player.cpp
    ${AUDIO_AUD_SOURCES}
    ${AUDIO_MUSIC_PLAYER_SOURCES}
)

target_include_directories(TestMusicPlayer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(TestMusicPlayer PRIVATE
    redalert_platform
)

add_test(NAME MusicPlayerTests COMMAND TestMusicPlayer)
```

---

## Verification Script

```bash
#!/bin/bash
set -e

echo "=== Task 17c Verification: Music Player ==="
echo ""

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"

echo "Step 1: Checking source files..."

FILES=(
    "include/game/audio/music_track.h"
    "include/game/audio/music_player.h"
    "src/game/audio/music_player.cpp"
    "src/test_music_player.cpp"
)

for file in "${FILES[@]}"; do
    if [ ! -f "$ROOT_DIR/$file" ]; then
        echo "VERIFY_FAILED: Missing $file"
        exit 1
    fi
    echo "  ✓ $file"
done

echo ""
echo "Step 2: Building..."

cd "$BUILD_DIR"
cmake --build . --target TestMusicPlayer

echo "  ✓ Built successfully"

echo ""
echo "Step 3: Running tests..."

if ! "$BUILD_DIR/TestMusicPlayer"; then
    echo "VERIFY_FAILED: Tests failed"
    exit 1
fi

echo "  ✓ All tests passed"

echo ""
echo "Step 4: Verifying track enum..."

TRACK_COUNT=$(grep -c "MusicTrack::" "$ROOT_DIR/include/game/audio/music_track.h" || echo "0")
if [ "$TRACK_COUNT" -lt 15 ]; then
    echo "VERIFY_FAILED: MusicTrack enum seems incomplete"
    exit 1
fi

echo "  ✓ $TRACK_COUNT tracks defined"

echo ""
echo "=== Task 17c Verification PASSED ==="
```

---

## Implementation Steps

1. **Create music_track.h** - Define MusicTrack enum with all game tracks
2. **Create music_player.h** - MusicPlayer class declaration
3. **Create music_player.cpp** - Full implementation with shuffle, fade, etc.
4. **Create test_music_player.cpp** - Unit and integration tests
5. **Update CMakeLists.txt**
6. **Build and run tests**
7. **Test interactive playback** with actual game music

---

## Success Criteria

- [ ] MusicTrack enum defines all 15+ tracks
- [ ] MUSIC_TRACK_INFO maps tracks to filenames
- [ ] Play/Stop/Pause/Resume work correctly
- [ ] Looping works
- [ ] Shuffle mode plays all tracks before repeating
- [ ] Volume control works
- [ ] Muting stops sound but maintains state
- [ ] Auto-advance plays next track
- [ ] Previous track function works
- [ ] All unit tests pass
- [ ] Interactive test plays music correctly

---

## Completion Promise

When verification passes, output:
<promise>TASK_17C_COMPLETE</promise>

## Escape Hatch

If stuck after 15 iterations:
- Document blocking issue in `ralph-tasks/blocked/17c.md`
- Output:
<promise>TASK_17C_BLOCKED</promise>

## Max Iterations
15
