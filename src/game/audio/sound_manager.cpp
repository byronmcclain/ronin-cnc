// src/game/audio/sound_manager.cpp
// Sound Effect Manager implementation
// Task 17b - Sound Effect Manager

#include "game/audio/sound_manager.h"
#include "game/audio/aud_file.h"
#include "game/viewport.h"
#include "platform.h"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cmath>

//=============================================================================
// Sound Info Database
//=============================================================================

// Sound metadata table - indexed by SoundEffect enum
static const SoundInfo SOUND_INFO[] = {
    // NONE
    { nullptr, SoundCategory::UI, 0, 0, 0, 0.0f },

    // UI Sounds
    { "CLICK.AUD",    SoundCategory::UI, 200, 2, 50, 1.0f },   // UI_CLICK
    { "BEEP1.AUD",    SoundCategory::UI, 180, 2, 100, 0.8f },  // UI_BEEP
    { "CANCEL.AUD",   SoundCategory::UI, 180, 1, 100, 1.0f },  // UI_CANCEL
    { "BUZZY1.AUD",   SoundCategory::UI, 220, 1, 200, 0.9f },  // UI_ERROR
    { "TARGET1.AUD",  SoundCategory::UI, 150, 2, 100, 0.7f },  // UI_TARGET

    // Construction Sounds
    { "PLACBLDG.AUD", SoundCategory::SPECIAL, 180, 1, 200, 1.0f },  // BUILD_PLACE
    { "SOLD.AUD",     SoundCategory::SPECIAL, 180, 1, 200, 1.0f },  // BUILD_SOLD
    { "PRIMARY.AUD",  SoundCategory::SPECIAL, 180, 1, 200, 0.9f },  // BUILD_PRIMARY
    { "CLOCK.AUD",    SoundCategory::UI, 100, 1, 500, 0.5f },       // BUILD_CLOCK
    { "CRUMBLE.AUD",  SoundCategory::COMBAT, 200, 2, 100, 1.0f },   // BUILD_CRUMBLE

    // Explosions
    { "XPLOSML.AUD",  SoundCategory::COMBAT, 150, 4, 30, 0.8f },   // EXPLODE_TINY
    { "XPLOS.AUD",    SoundCategory::COMBAT, 160, 4, 30, 0.9f },   // EXPLODE_SMALL
    { "XPLOMED.AUD",  SoundCategory::COMBAT, 170, 3, 50, 1.0f },   // EXPLODE_MEDIUM
    { "XPLOBIG.AUD",  SoundCategory::COMBAT, 180, 3, 50, 1.0f },   // EXPLODE_LARGE
    { "XPLOMGE.AUD",  SoundCategory::COMBAT, 200, 2, 100, 1.0f },  // EXPLODE_HUGE
    { "FIRE1.AUD",    SoundCategory::COMBAT, 140, 4, 30, 0.7f },   // EXPLODE_FIRE
    { "SPLASH1.AUD",  SoundCategory::COMBAT, 130, 4, 30, 0.6f },   // EXPLODE_WATER
    { "NUKEXPLO.AUD", SoundCategory::COMBAT, 255, 1, 500, 1.0f },  // EXPLODE_NUKE

    // Weapons
    { "PISTOL.AUD",   SoundCategory::COMBAT, 120, 4, 30, 0.6f },   // WEAPON_PISTOL
    { "GUN1.AUD",     SoundCategory::COMBAT, 130, 4, 30, 0.7f },   // WEAPON_RIFLE
    { "MGUN1.AUD",    SoundCategory::COMBAT, 140, 3, 50, 0.8f },   // WEAPON_MGUN
    { "MGUN2.AUD",    SoundCategory::COMBAT, 140, 3, 50, 0.8f },   // WEAPON_MGUN_BURST
    { "CANNON.AUD",   SoundCategory::COMBAT, 160, 3, 50, 1.0f },   // WEAPON_CANNON
    { "ARTIL.AUD",    SoundCategory::COMBAT, 170, 2, 100, 1.0f },  // WEAPON_ARTILLERY
    { "ROCKET1.AUD",  SoundCategory::COMBAT, 150, 3, 50, 0.9f },   // WEAPON_ROCKET
    { "ROCKET2.AUD",  SoundCategory::COMBAT, 150, 3, 50, 0.9f },   // WEAPON_MISSILE
    { "GRENADE.AUD",  SoundCategory::COMBAT, 140, 4, 30, 0.8f },   // WEAPON_GRENADE
    { "FLAME.AUD",    SoundCategory::COMBAT, 140, 2, 100, 0.8f },  // WEAPON_FLAME
    { "TESLA1.AUD",   SoundCategory::COMBAT, 180, 2, 100, 1.0f },  // WEAPON_TESLA
    { "TELSCHG.AUD",  SoundCategory::COMBAT, 170, 1, 200, 0.9f },  // WEAPON_TESLA_CHARGE
    { "FLAK1.AUD",    SoundCategory::COMBAT, 150, 3, 50, 0.9f },   // WEAPON_AA
    { "DEPTHCH.AUD",  SoundCategory::COMBAT, 160, 2, 100, 1.0f },  // WEAPON_DEPTH_CHARGE
    { "TORPEDO.AUD",  SoundCategory::COMBAT, 150, 2, 100, 0.9f },  // WEAPON_TORPEDO

    // Impacts
    { "RICOCHET.AUD", SoundCategory::COMBAT, 100, 6, 20, 0.5f },   // IMPACT_BULLET
    { "SHELL.AUD",    SoundCategory::COMBAT, 120, 4, 30, 0.6f },   // IMPACT_SHELL
    { "METAL1.AUD",   SoundCategory::COMBAT, 110, 4, 30, 0.6f },   // IMPACT_METAL
    { "CONCRET1.AUD", SoundCategory::COMBAT, 110, 4, 30, 0.6f },   // IMPACT_CONCRETE
    { "DIRT1.AUD",    SoundCategory::COMBAT, 100, 4, 30, 0.5f },   // IMPACT_GROUND
    { "WATERSPL.AUD", SoundCategory::COMBAT, 100, 4, 30, 0.5f },   // IMPACT_WATER

    // Unit Movement
    { "VSTART.AUD",   SoundCategory::UNIT, 100, 3, 100, 0.6f },    // MOVE_VEHICLE_START
    { "VEHICLEA.AUD", SoundCategory::UNIT, 80, 2, 200, 0.5f },     // MOVE_VEHICLE
    { "VEHICLEB.AUD", SoundCategory::UNIT, 80, 2, 200, 0.5f },     // MOVE_VEHICLE_HEAVY
    { "TURRET.AUD",   SoundCategory::UNIT, 90, 3, 100, 0.4f },     // MOVE_TANK_TURRET
    { "INFANTRY.AUD", SoundCategory::UNIT, 60, 4, 50, 0.3f },      // MOVE_INFANTRY
    { "COPTER.AUD",   SoundCategory::UNIT, 100, 2, 200, 0.6f },    // MOVE_HELICOPTER
    { "PLANE.AUD",    SoundCategory::UNIT, 100, 2, 200, 0.6f },    // MOVE_PLANE
    { "SHIP.AUD",     SoundCategory::UNIT, 80, 2, 200, 0.5f },     // MOVE_SHIP
    { "SUB.AUD",      SoundCategory::UNIT, 70, 2, 200, 0.4f },     // MOVE_SUB

    // Unit Actions
    { "REPAIR.AUD",   SoundCategory::UNIT, 120, 2, 200, 0.7f },    // ACTION_REPAIR
    { "HARVEST.AUD",  SoundCategory::UNIT, 100, 2, 200, 0.6f },    // ACTION_HARVEST
    { "CRUSH.AUD",    SoundCategory::UNIT, 140, 3, 100, 0.8f },    // ACTION_CRUSH
    { "CLOAK.AUD",    SoundCategory::SPECIAL, 150, 2, 200, 0.8f }, // ACTION_CLOAK
    { "UNCLOAK.AUD",  SoundCategory::SPECIAL, 150, 2, 200, 0.8f }, // ACTION_UNCLOAK
    { "CHRONO.AUD",   SoundCategory::SPECIAL, 200, 1, 500, 1.0f }, // ACTION_CHRONO
    { "IRON.AUD",     SoundCategory::SPECIAL, 200, 1, 500, 1.0f }, // ACTION_IRON_CURTAIN
    { "PARABOMB.AUD", SoundCategory::SPECIAL, 180, 1, 500, 1.0f }, // ACTION_PARABOMB

    // Environment
    { "BURN.AUD",     SoundCategory::AMBIENT, 50, 4, 100, 0.4f },  // AMBIENT_FIRE
    { "ELECTRIC.AUD", SoundCategory::AMBIENT, 60, 2, 200, 0.3f },  // AMBIENT_ELECTRIC
    { "ALARM.AUD",    SoundCategory::AMBIENT, 150, 1, 1000, 0.8f },// AMBIENT_ALARM
    { "SIREN.AUD",    SoundCategory::AMBIENT, 160, 1, 2000, 0.9f },// AMBIENT_SIREN

    // Special
    { "RADARUP.AUD",  SoundCategory::SPECIAL, 180, 1, 500, 1.0f }, // SPECIAL_RADAR_ON
    { "RADARDN.AUD",  SoundCategory::SPECIAL, 180, 1, 500, 1.0f }, // SPECIAL_RADAR_OFF
    { "POWRUP1.AUD",  SoundCategory::SPECIAL, 180, 1, 500, 1.0f }, // SPECIAL_POWER_UP
    { "POWDOWN1.AUD", SoundCategory::SPECIAL, 180, 1, 500, 1.0f }, // SPECIAL_POWER_DOWN
    { "GPS.AUD",      SoundCategory::SPECIAL, 180, 1, 500, 0.9f }, // SPECIAL_GPS
};

// Verify table size matches enum
static_assert(sizeof(SOUND_INFO) / sizeof(SOUND_INFO[0]) == static_cast<size_t>(SoundEffect::COUNT),
              "SOUND_INFO table size mismatch");

//=============================================================================
// Sound Info Accessors
//=============================================================================

const SoundInfo& GetSoundInfo(SoundEffect sfx) {
    int idx = static_cast<int>(sfx);
    if (idx < 0 || idx >= static_cast<int>(SoundEffect::COUNT)) {
        return SOUND_INFO[0];  // Return NONE info
    }
    return SOUND_INFO[idx];
}

const char* GetSoundFilename(SoundEffect sfx) {
    return GetSoundInfo(sfx).filename;
}

SoundCategory GetSoundCategory(SoundEffect sfx) {
    return GetSoundInfo(sfx).category;
}

//=============================================================================
// SoundManager Implementation
//=============================================================================

SoundManager& SoundManager::Instance() {
    static SoundManager instance;
    return instance;
}

SoundManager::SoundManager()
    : sfx_volume_(1.0f)
    , muted_(false)
    , listener_x_(0)
    , listener_y_(0)
    , max_distance_(1200)
    , initialized_(false)
    , current_time_(0) {

    // Initialize all sounds as unloaded
    for (auto& sound : sounds_) {
        sound.platform_handle = INVALID_SOUND_HANDLE;
        sound.loaded = false;
        sound.last_play_time = 0;
        sound.current_play_count = 0;
    }

    // Initialize category volumes
    category_volumes_.fill(1.0f);
}

SoundManager::~SoundManager() {
    Shutdown();
}

bool SoundManager::Initialize(const SoundManagerConfig& config) {
    if (initialized_) {
        return true;
    }

    Platform_LogInfo("SoundManager: Initializing...");

    config_ = config;
    max_distance_ = config.default_max_distance;

    // Load all sounds
    LoadAllSounds();

    initialized_ = true;
    Platform_LogInfo("SoundManager: Initialization complete");

    return true;
}

void SoundManager::Shutdown() {
    if (!initialized_) {
        return;
    }

    Platform_LogInfo("SoundManager: Shutting down...");

    StopAll();
    UnloadAllSounds();

    initialized_ = false;
}

void SoundManager::LoadAllSounds() {
    Platform_LogInfo("SoundManager: Loading sound effects...");

    int loaded = 0;
    int failed = 0;

    for (int i = 1; i < static_cast<int>(SoundEffect::COUNT); i++) {
        SoundEffect sfx = static_cast<SoundEffect>(i);
        if (LoadSound(sfx)) {
            loaded++;
        } else {
            failed++;
        }
    }

    // Log results (simple string version - counts available via GetLoadedSoundCount)
    Platform_LogInfo("SoundManager: Sound loading complete");
}

void SoundManager::UnloadAllSounds() {
    for (auto& sound : sounds_) {
        if (sound.loaded && sound.platform_handle != INVALID_SOUND_HANDLE) {
            Platform_Sound_Destroy(sound.platform_handle);
        }
        sound.platform_handle = INVALID_SOUND_HANDLE;
        sound.loaded = false;
    }
}

bool SoundManager::LoadSound(SoundEffect sfx) {
    int idx = static_cast<int>(sfx);
    if (idx <= 0 || idx >= static_cast<int>(SoundEffect::COUNT)) {
        return false;
    }

    const SoundInfo& info = SOUND_INFO[idx];
    if (!info.filename) {
        return false;
    }

    // Load AUD file using platform MIX API
    AudFile aud;
    if (!aud.LoadFromMix(info.filename)) {
        // Sound file not found - this is expected if game assets aren't present
        return false;
    }

    // Create platform sound
    SoundHandle handle = Platform_Sound_CreateFromMemory(
        aud.GetPCMDataBytes(),
        static_cast<int32_t>(aud.GetPCMDataSize()),
        aud.GetSampleRate(),
        aud.GetChannels(),
        16  // Always 16-bit after decode
    );

    if (handle == INVALID_SOUND_HANDLE) {
        // Sound creation failed (logged internally by platform)
        return false;
    }

    sounds_[idx].platform_handle = handle;
    sounds_[idx].loaded = true;

    return true;
}

//=============================================================================
// Playback
//=============================================================================

PlayHandle SoundManager::Play(SoundEffect sfx) {
    return Play(sfx, 1.0f);
}

PlayHandle SoundManager::Play(SoundEffect sfx, float volume) {
    if (!initialized_ || muted_) {
        return INVALID_PLAY_HANDLE;
    }

    int idx = static_cast<int>(sfx);
    if (idx <= 0 || idx >= static_cast<int>(SoundEffect::COUNT)) {
        return INVALID_PLAY_HANDLE;
    }

    if (!sounds_[idx].loaded) {
        return INVALID_PLAY_HANDLE;
    }

    if (!CanPlaySound(sfx)) {
        return INVALID_PLAY_HANDLE;
    }

    float final_volume = GetFinalVolume(sfx, volume);
    if (final_volume < config_.min_audible_volume) {
        return INVALID_PLAY_HANDLE;
    }

    PlayHandle play_handle = Platform_Sound_Play(
        sounds_[idx].platform_handle,
        final_volume,
        0.0f,   // No pan
        false   // Not looping
    );

    if (play_handle != INVALID_PLAY_HANDLE) {
        // Track playing sound
        PlayingSoundInfo info;
        info.play_handle = play_handle;
        info.sound_id = sfx;
        info.start_time_ms = current_time_;
        info.positional = false;
        info.world_x = 0;
        info.world_y = 0;
        info.volume = final_volume;
        playing_sounds_.push_back(info);

        // Update tracking
        sounds_[idx].last_play_time = current_time_;
        sounds_[idx].current_play_count++;
    }

    return play_handle;
}

PlayHandle SoundManager::PlayAt(SoundEffect sfx, int world_x, int world_y, float volume) {
    if (!initialized_ || muted_) {
        return INVALID_PLAY_HANDLE;
    }

    int idx = static_cast<int>(sfx);
    if (idx <= 0 || idx >= static_cast<int>(SoundEffect::COUNT)) {
        return INVALID_PLAY_HANDLE;
    }

    if (!sounds_[idx].loaded) {
        return INVALID_PLAY_HANDLE;
    }

    if (!CanPlaySound(sfx)) {
        return INVALID_PLAY_HANDLE;
    }

    // Calculate distance-based volume
    float distance_factor = CalculateDistanceVolume(world_x, world_y);
    if (distance_factor <= 0.0f) {
        return INVALID_PLAY_HANDLE;  // Too far to hear
    }

    float final_volume = GetFinalVolume(sfx, volume * distance_factor);
    if (final_volume < config_.min_audible_volume) {
        return INVALID_PLAY_HANDLE;
    }

    // Calculate stereo pan based on position
    float pan = 0.0f;
    int dx = world_x - listener_x_;
    if (max_distance_ > 0) {
        pan = static_cast<float>(dx) / static_cast<float>(max_distance_);
        pan = std::clamp(pan, -1.0f, 1.0f);
    }

    PlayHandle play_handle = Platform_Sound_Play(
        sounds_[idx].platform_handle,
        final_volume,
        pan,
        false
    );

    if (play_handle != INVALID_PLAY_HANDLE) {
        PlayingSoundInfo info;
        info.play_handle = play_handle;
        info.sound_id = sfx;
        info.start_time_ms = current_time_;
        info.positional = true;
        info.world_x = world_x;
        info.world_y = world_y;
        info.volume = final_volume;
        playing_sounds_.push_back(info);

        sounds_[idx].last_play_time = current_time_;
        sounds_[idx].current_play_count++;
    }

    return play_handle;
}

PlayHandle SoundManager::PlayAtCell(SoundEffect sfx, int cell_x, int cell_y, float volume) {
    // Convert cell to world coordinates (center of cell)
    int world_x = cell_x * TILE_PIXEL_WIDTH + TILE_PIXEL_WIDTH / 2;
    int world_y = cell_y * TILE_PIXEL_HEIGHT + TILE_PIXEL_HEIGHT / 2;
    return PlayAt(sfx, world_x, world_y, volume);
}

void SoundManager::Stop(PlayHandle handle) {
    if (handle == INVALID_PLAY_HANDLE) {
        return;
    }

    Platform_Sound_Stop(handle);

    // Remove from playing list
    auto it = std::find_if(playing_sounds_.begin(), playing_sounds_.end(),
        [handle](const PlayingSoundInfo& info) {
            return info.play_handle == handle;
        });

    if (it != playing_sounds_.end()) {
        int idx = static_cast<int>(it->sound_id);
        if (idx > 0 && idx < static_cast<int>(SoundEffect::COUNT)) {
            if (sounds_[idx].current_play_count > 0) {
                sounds_[idx].current_play_count--;
            }
        }
        playing_sounds_.erase(it);
    }
}

void SoundManager::StopAll() {
    Platform_Sound_StopAll();

    // Reset all play counts
    for (auto& sound : sounds_) {
        sound.current_play_count = 0;
    }

    playing_sounds_.clear();
}

bool SoundManager::IsPlaying(PlayHandle handle) const {
    if (handle == INVALID_PLAY_HANDLE) {
        return false;
    }
    return Platform_Sound_IsPlaying(handle);
}

//=============================================================================
// Volume Control
//=============================================================================

void SoundManager::SetVolume(float volume) {
    sfx_volume_ = std::clamp(volume, 0.0f, 1.0f);
}

void SoundManager::SetCategoryVolume(SoundCategory category, float volume) {
    int idx = static_cast<int>(category);
    if (idx >= 0 && idx < static_cast<int>(category_volumes_.size())) {
        category_volumes_[idx] = std::clamp(volume, 0.0f, 1.0f);
    }
}

float SoundManager::GetCategoryVolume(SoundCategory category) const {
    int idx = static_cast<int>(category);
    if (idx >= 0 && idx < static_cast<int>(category_volumes_.size())) {
        return category_volumes_[idx];
    }
    return 1.0f;
}

void SoundManager::SetMuted(bool muted) {
    muted_ = muted;
    if (muted) {
        StopAll();
    }
}

//=============================================================================
// Listener Position
//=============================================================================

void SoundManager::SetListenerPosition(int world_x, int world_y) {
    listener_x_ = world_x;
    listener_y_ = world_y;
}

void SoundManager::GetListenerPosition(int& world_x, int& world_y) const {
    world_x = listener_x_;
    world_y = listener_y_;
}

void SoundManager::UpdateListenerFromViewport() {
    GameViewport& vp = GameViewport::Instance();

    // Listener is at center of viewport
    int center_x = vp.x + vp.width / 2;
    int center_y = vp.y + vp.height / 2;

    SetListenerPosition(center_x, center_y);
}

void SoundManager::SetMaxDistance(int distance) {
    max_distance_ = std::max(1, distance);
}

//=============================================================================
// Update
//=============================================================================

void SoundManager::Update() {
    if (!initialized_) {
        return;
    }

    current_time_ = Platform_GetTicks();

    // Update listener from viewport
    UpdateListenerFromViewport();

    // Clean up finished sounds
    CleanupFinishedSounds();
}

//=============================================================================
// Internal Methods
//=============================================================================

float SoundManager::CalculateDistanceVolume(int world_x, int world_y) const {
    int dx = world_x - listener_x_;
    int dy = world_y - listener_y_;
    float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));

    if (distance >= static_cast<float>(max_distance_)) {
        return 0.0f;
    }

    return 1.0f - (distance / static_cast<float>(max_distance_));
}

bool SoundManager::CanPlaySound(SoundEffect sfx) const {
    int idx = static_cast<int>(sfx);
    if (idx <= 0 || idx >= static_cast<int>(SoundEffect::COUNT)) {
        return false;
    }

    const LoadedSound& sound = sounds_[idx];
    const SoundInfo& info = SOUND_INFO[idx];

    // Check max concurrent for this sound
    if (info.max_concurrent > 0 && sound.current_play_count >= info.max_concurrent) {
        return false;
    }

    // Check rate limiting
    if (info.min_interval_ms > 0) {
        if (current_time_ - sound.last_play_time < info.min_interval_ms) {
            return false;
        }
    }

    // Check total concurrent sounds
    if (static_cast<int>(playing_sounds_.size()) >= config_.max_concurrent_sounds) {
        return false;
    }

    return true;
}

void SoundManager::CleanupFinishedSounds() {
    auto it = playing_sounds_.begin();
    while (it != playing_sounds_.end()) {
        if (!Platform_Sound_IsPlaying(it->play_handle)) {
            // Update play count
            int idx = static_cast<int>(it->sound_id);
            if (idx > 0 && idx < static_cast<int>(SoundEffect::COUNT)) {
                if (sounds_[idx].current_play_count > 0) {
                    sounds_[idx].current_play_count--;
                }
            }
            it = playing_sounds_.erase(it);
        } else {
            ++it;
        }
    }
}

float SoundManager::GetFinalVolume(SoundEffect sfx, float base_volume) const {
    const SoundInfo& info = GetSoundInfo(sfx);

    float category_vol = GetCategoryVolume(info.category);
    float default_vol = info.default_volume;

    return base_volume * default_vol * category_vol * sfx_volume_;
}

//=============================================================================
// Debug
//=============================================================================

int SoundManager::GetLoadedSoundCount() const {
    int count = 0;
    for (const auto& sound : sounds_) {
        if (sound.loaded) count++;
    }
    return count;
}

int SoundManager::GetPlayingSoundCount() const {
    return static_cast<int>(playing_sounds_.size());
}

bool SoundManager::IsSoundLoaded(SoundEffect sfx) const {
    int idx = static_cast<int>(sfx);
    if (idx <= 0 || idx >= static_cast<int>(SoundEffect::COUNT)) {
        return false;
    }
    return sounds_[idx].loaded;
}

void SoundManager::PrintStats() const {
    printf("SoundManager Stats:\n");
    printf("  Loaded sounds: %d / %d\n",
           GetLoadedSoundCount(), static_cast<int>(SoundEffect::COUNT) - 1);
    printf("  Playing sounds: %d / %d\n",
           GetPlayingSoundCount(), config_.max_concurrent_sounds);
    printf("  SFX Volume: %.0f%%\n", sfx_volume_ * 100.0f);
    printf("  Listener: (%d, %d)\n", listener_x_, listener_y_);
    printf("  Muted: %s\n", muted_ ? "yes" : "no");
}

//=============================================================================
// Global Convenience Functions
//=============================================================================

bool Sound_Init() {
    return SoundManager::Instance().Initialize();
}

void Sound_Shutdown() {
    SoundManager::Instance().Shutdown();
}

void Sound_Update() {
    SoundManager::Instance().Update();
}

PlayHandle Sound_Play(SoundEffect sfx) {
    return SoundManager::Instance().Play(sfx);
}

PlayHandle Sound_PlayAt(SoundEffect sfx, int world_x, int world_y) {
    return SoundManager::Instance().PlayAt(sfx, world_x, world_y);
}

void Sound_StopAll() {
    SoundManager::Instance().StopAll();
}

void Sound_SetVolume(int volume) {
    // Convert 0-255 to 0.0-1.0
    float vol = std::clamp(volume, 0, 255) / 255.0f;
    SoundManager::Instance().SetVolume(vol);
}

int Sound_GetVolume() {
    // Convert 0.0-1.0 to 0-255
    return static_cast<int>(SoundManager::Instance().GetVolume() * 255.0f);
}
