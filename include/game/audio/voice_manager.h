// include/game/audio/voice_manager.h
// Voice playback manager
// Task 17d - Voice System

#ifndef VOICE_MANAGER_H
#define VOICE_MANAGER_H

#include "game/audio/voice_event.h"
#include "platform.h"
#include <vector>
#include <queue>
#include <unordered_map>
#include <functional>
#include <cstdint>

//=============================================================================
// Voice Queue Entry
//=============================================================================

struct QueuedVoice {
    EvaVoice voice;
    uint8_t priority;
    uint64_t queue_time;
};

//=============================================================================
// Voice Manager Class
//=============================================================================

class VoiceManager {
public:
    /// Singleton instance
    static VoiceManager& Instance();

    //=========================================================================
    // Lifecycle
    //=========================================================================

    /// Initialize voice manager
    bool Initialize();

    /// Shutdown and release resources
    void Shutdown();

    /// Check if initialized
    bool IsInitialized() const { return initialized_; }

    /// Preload common voices for faster playback
    void PreloadCommonVoices();

    //=========================================================================
    // EVA Voice Playback
    //=========================================================================

    /// Play an EVA announcement
    /// @param voice The voice event to play
    /// @return true if voice started or queued
    bool PlayEva(EvaVoice voice);

    /// Queue an EVA voice (always queues, never interrupts)
    void QueueEva(EvaVoice voice);

    /// Stop current EVA voice and clear queue
    void StopEva();

    /// Check if EVA is currently speaking
    bool IsEvaSpeaking() const;

    /// Get current EVA voice
    EvaVoice GetCurrentEvaVoice() const { return current_eva_; }

    /// Clear EVA voice queue
    void ClearEvaQueue();

    //=========================================================================
    // Unit Voice Playback
    //=========================================================================

    /// Play a unit acknowledgment voice
    /// @param voice The voice event to play
    /// @param faction Faction for voice variant
    /// @return Play handle
    PlayHandle PlayUnit(UnitVoice voice, VoiceFaction faction = VoiceFaction::NEUTRAL);

    /// Play unit voice at world position
    PlayHandle PlayUnitAt(UnitVoice voice, int world_x, int world_y,
                          VoiceFaction faction = VoiceFaction::NEUTRAL);

    /// Stop current unit voice
    void StopUnit();

    /// Check if unit voice is playing
    bool IsUnitSpeaking() const;

    //=========================================================================
    // Volume Control
    //=========================================================================

    /// Set voice volume (0.0-1.0)
    void SetVolume(float volume);

    /// Get voice volume
    float GetVolume() const { return volume_; }

    /// Mute/unmute voices
    void SetMuted(bool muted);

    /// Check if muted
    bool IsMuted() const { return muted_; }

    //=========================================================================
    // Per-Frame Update
    //=========================================================================

    /// Update voice manager (process queue, check completion)
    void Update();

    //=========================================================================
    // Debug
    //=========================================================================

    /// Print current status
    void PrintStatus() const;

    /// Get queue size
    size_t GetQueueSize() const { return eva_queue_.size(); }

private:
    VoiceManager();
    ~VoiceManager();
    VoiceManager(const VoiceManager&) = delete;
    VoiceManager& operator=(const VoiceManager&) = delete;

    //=========================================================================
    // Internal Types
    //=========================================================================

    struct LoadedVoice {
        SoundHandle handle;
        bool loaded;
    };

    //=========================================================================
    // Internal Data
    //=========================================================================

    // EVA voice data
    std::unordered_map<int, LoadedVoice> eva_voices_;
    std::priority_queue<QueuedVoice,
                        std::vector<QueuedVoice>,
                        std::function<bool(const QueuedVoice&, const QueuedVoice&)>> eva_queue_;
    PlayHandle eva_play_handle_;
    EvaVoice current_eva_;
    uint64_t eva_last_play_time_;

    // Rate limiting for EVA voices
    std::unordered_map<int, uint64_t> eva_last_played_;

    // Unit voice data
    std::unordered_map<int, LoadedVoice> unit_voices_;
    PlayHandle unit_play_handle_;
    UnitVoice current_unit_;
    uint64_t unit_last_play_time_;

    // Rate limiting for unit voices
    std::unordered_map<int, uint64_t> unit_last_played_;

    // Settings
    float volume_;
    bool muted_;
    bool initialized_;
    uint64_t current_time_;

    //=========================================================================
    // Internal Methods
    //=========================================================================

    /// Load an EVA voice file
    bool LoadEvaVoice(EvaVoice voice);

    /// Load a unit voice file
    bool LoadUnitVoice(UnitVoice voice, VoiceFaction faction);

    /// Get sound handle for EVA voice (loading if needed)
    SoundHandle GetEvaSound(EvaVoice voice);

    /// Get sound handle for unit voice
    SoundHandle GetUnitSound(UnitVoice voice, VoiceFaction faction);

    /// Actually play an EVA voice
    bool DoPlayEva(EvaVoice voice);

    /// Process EVA queue
    void ProcessEvaQueue();

    /// Check if EVA voice can play (rate limiting)
    bool CanPlayEva(EvaVoice voice) const;

    /// Check if unit voice can play (rate limiting)
    bool CanPlayUnit(UnitVoice voice) const;

    /// Get unique key for voice+faction
    int GetUnitVoiceKey(UnitVoice voice, VoiceFaction faction) const;
};

//=============================================================================
// Global Convenience Functions
//=============================================================================

/// Initialize voice manager
bool Voice_Init();

/// Shutdown voice manager
void Voice_Shutdown();

/// Update voice manager (call each frame)
void Voice_Update();

/// Play EVA announcement
void Voice_PlayEva(EvaVoice voice);

/// Queue EVA announcement
void Voice_QueueEva(EvaVoice voice);

/// Play unit acknowledgment
PlayHandle Voice_PlayUnit(UnitVoice voice);

/// Set voice volume (0-255)
void Voice_SetVolume(int volume);

/// Get voice volume (0-255)
int Voice_GetVolume();

#endif // VOICE_MANAGER_H
