// include/game/audio/sound_manager.h
// Sound Effect Manager
// Task 17b - Sound Effect Manager

#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include "game/audio/sound_effect.h"
#include "platform.h"
#include <cstdint>
#include <vector>
#include <array>

//=============================================================================
// Forward Declarations
//=============================================================================

class AudFile;

//=============================================================================
// Sound Manager Configuration
//=============================================================================

struct SoundManagerConfig {
    int max_concurrent_sounds = 16;       // Max total simultaneous sounds
    int max_same_sound = 3;               // Max plays of same sound
    int default_max_distance = 1200;      // Max audible distance in pixels
    float min_audible_volume = 0.05f;     // Volume threshold for culling
};

//=============================================================================
// Playing Sound Info
//=============================================================================

struct PlayingSoundInfo {
    PlayHandle play_handle;      // Platform play handle
    SoundEffect sound_id;        // Which sound
    uint64_t start_time_ms;      // When playback started
    bool positional;             // Is position-based
    int world_x;                 // World position X
    int world_y;                 // World position Y
    float volume;                // Current volume
};

//=============================================================================
// Sound Manager Class
//=============================================================================

class SoundManager {
public:
    /// Singleton instance
    static SoundManager& Instance();

    //=========================================================================
    // Lifecycle
    //=========================================================================

    /// Initialize sound manager and load all sounds
    /// @param config Optional configuration
    /// @return true if successful
    bool Initialize(const SoundManagerConfig& config = SoundManagerConfig());

    /// Shutdown and release all resources
    void Shutdown();

    /// Check if initialized
    bool IsInitialized() const { return initialized_; }

    /// Load all sound effects from MIX files
    /// Called automatically by Initialize(), can be called again to reload
    void LoadAllSounds();

    /// Unload all sounds to free memory
    void UnloadAllSounds();

    //=========================================================================
    // Playback
    //=========================================================================

    /// Play a sound effect (non-positional, full volume)
    /// @param sfx Sound effect to play
    /// @return Play handle, or INVALID_PLAY_HANDLE if failed
    PlayHandle Play(SoundEffect sfx);

    /// Play a sound effect with volume
    /// @param sfx Sound effect to play
    /// @param volume Volume multiplier (0.0-1.0)
    /// @return Play handle
    PlayHandle Play(SoundEffect sfx, float volume);

    /// Play a sound effect at a world position
    /// Volume is attenuated based on distance from listener
    /// @param sfx Sound effect to play
    /// @param world_x World X coordinate
    /// @param world_y World Y coordinate
    /// @param volume Base volume before attenuation
    /// @return Play handle
    PlayHandle PlayAt(SoundEffect sfx, int world_x, int world_y, float volume = 1.0f);

    /// Play a sound at a cell coordinate
    /// @param sfx Sound effect to play
    /// @param cell_x Cell X coordinate
    /// @param cell_y Cell Y coordinate
    /// @param volume Base volume
    /// @return Play handle
    PlayHandle PlayAtCell(SoundEffect sfx, int cell_x, int cell_y, float volume = 1.0f);

    /// Stop a specific playing sound
    void Stop(PlayHandle handle);

    /// Stop all currently playing sound effects
    void StopAll();

    /// Check if a sound is currently playing
    bool IsPlaying(PlayHandle handle) const;

    //=========================================================================
    // Volume Control
    //=========================================================================

    /// Set SFX master volume (0.0-1.0)
    void SetVolume(float volume);

    /// Get SFX master volume
    float GetVolume() const { return sfx_volume_; }

    /// Set volume for a category
    void SetCategoryVolume(SoundCategory category, float volume);

    /// Get category volume
    float GetCategoryVolume(SoundCategory category) const;

    /// Mute/unmute all sound effects
    void SetMuted(bool muted);

    /// Check if muted
    bool IsMuted() const { return muted_; }

    //=========================================================================
    // Listener Position (for positional audio)
    //=========================================================================

    /// Set listener position (typically camera center)
    void SetListenerPosition(int world_x, int world_y);

    /// Get listener position
    void GetListenerPosition(int& world_x, int& world_y) const;

    /// Update listener position from viewport
    /// Call each frame to track camera movement
    void UpdateListenerFromViewport();

    //=========================================================================
    // Positional Audio Settings
    //=========================================================================

    /// Set maximum audible distance in world pixels
    void SetMaxDistance(int distance);

    /// Get maximum audible distance
    int GetMaxDistance() const { return max_distance_; }

    //=========================================================================
    // Per-Frame Update
    //=========================================================================

    /// Update sound manager state
    /// Cleans up finished sounds, updates positional audio
    void Update();

    //=========================================================================
    // Debug/Stats
    //=========================================================================

    /// Get number of loaded sounds
    int GetLoadedSoundCount() const;

    /// Get number of currently playing sounds
    int GetPlayingSoundCount() const;

    /// Check if a specific sound is loaded
    bool IsSoundLoaded(SoundEffect sfx) const;

    /// Print debug information
    void PrintStats() const;

private:
    SoundManager();
    ~SoundManager();
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    //=========================================================================
    // Internal Data
    //=========================================================================

    /// Sound data for each effect
    struct LoadedSound {
        SoundHandle platform_handle;    // Platform sound handle
        bool loaded;                    // Successfully loaded
        uint64_t last_play_time;        // Last time this sound was played
        int current_play_count;         // How many currently playing
    };

    std::array<LoadedSound, static_cast<size_t>(SoundEffect::COUNT)> sounds_;
    std::vector<PlayingSoundInfo> playing_sounds_;

    // Configuration
    SoundManagerConfig config_;

    // Volume settings
    float sfx_volume_;
    std::array<float, 5> category_volumes_;  // One per SoundCategory
    bool muted_;

    // Listener position
    int listener_x_;
    int listener_y_;
    int max_distance_;

    // State
    bool initialized_;
    uint64_t current_time_;

    //=========================================================================
    // Internal Methods
    //=========================================================================

    /// Load a single sound effect
    bool LoadSound(SoundEffect sfx);

    /// Calculate volume based on distance
    float CalculateDistanceVolume(int world_x, int world_y) const;

    /// Check if sound can play (rate limiting, concurrent limits)
    bool CanPlaySound(SoundEffect sfx) const;

    /// Clean up finished sounds from playing list
    void CleanupFinishedSounds();

    /// Get final volume for a sound
    float GetFinalVolume(SoundEffect sfx, float base_volume) const;
};

//=============================================================================
// Global Convenience Functions
//=============================================================================

/// Initialize sound manager
bool Sound_Init();

/// Shutdown sound manager
void Sound_Shutdown();

/// Update sound manager (call each frame)
void Sound_Update();

/// Play a sound effect
PlayHandle Sound_Play(SoundEffect sfx);

/// Play a sound effect at position
PlayHandle Sound_PlayAt(SoundEffect sfx, int world_x, int world_y);

/// Stop all sound effects
void Sound_StopAll();

/// Set SFX volume (0-255 for compatibility with original)
void Sound_SetVolume(int volume);

/// Get SFX volume (0-255)
int Sound_GetVolume();

#endif // SOUND_MANAGER_H
