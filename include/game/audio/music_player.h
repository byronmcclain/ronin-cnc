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
