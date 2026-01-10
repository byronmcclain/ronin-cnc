// src/game/audio/voice_manager.cpp
// Voice playback manager implementation
// Task 17d - Voice System

#include "game/audio/voice_manager.h"
#include "game/audio/aud_file.h"
#include "platform.h"
#include <algorithm>
#include <cstdio>

//=============================================================================
// Voice Info Tables
//=============================================================================

static const EvaVoiceInfo EVA_VOICE_INFO[] = {
    // NONE
    { nullptr, nullptr, 0, 0, "None" },

    // Construction Events
    { "BLDG1.AUD",     nullptr, 50,  2000, "Building" },
    { "CONSTRU1.AUD",  nullptr, 100, 3000, "Construction complete" },
    { "ONHOLD1.AUD",   nullptr, 80,  3000, "On hold" },
    { "CANCLD1.AUD",   nullptr, 80,  2000, "Cancelled" },
    { "NEWOPT1.AUD",   nullptr, 100, 5000, "New construction options" },

    // Unit Events
    { "TRAIN1.AUD",    nullptr, 50,  2000, "Training" },
    { "UNITREDY.AUD",  nullptr, 100, 2000, "Unit ready" },
    { "UNITLOST.AUD",  nullptr, 200, 5000, "Unit lost" },
    { "REINFOR1.AUD",  nullptr, 150, 5000, "Reinforcements" },

    // Base Status
    { "BASEATK1.AUD",  nullptr, 250, 30000, "Base under attack" },
    { "PRIBLDG1.AUD",  nullptr, 80,  3000, "Primary building" },
    { "BLDGCAP1.AUD",  nullptr, 180, 5000, "Building captured" },
    { "BLDGLST1.AUD",  nullptr, 180, 5000, "Building lost" },

    // Resources
    { "LOWPOWR1.AUD",  nullptr, 180, 10000, "Low power" },
    { "POWRRES1.AUD",  nullptr, 150, 5000, "Power restored" },
    { "INSUFUND.AUD",  nullptr, 150, 5000, "Insufficient funds" },
    { "ABORRONE.AUD",  nullptr, 120, 10000, "Ore depleted" },
    { "NEEDSILO.AUD",  nullptr, 150, 10000, "Silos needed" },

    // Radar
    { "RADARON1.AUD",  nullptr, 100, 5000, "Radar online" },
    { "RADAROFF.AUD",  nullptr, 120, 5000, "Radar offline" },

    // Superweapons
    { "IRONRDY1.AUD",  nullptr, 200, 30000, "Iron curtain ready" },
    { "IRONCHG1.AUD",  nullptr, 100, 10000, "Iron curtain charging" },
    { "CHRORDY1.AUD",  nullptr, 200, 30000, "Chronosphere ready" },
    { "CHROCHG1.AUD",  nullptr, 100, 10000, "Chronosphere charging" },
    { "NUKESRDY.AUD",  nullptr, 220, 30000, "Nuke ready" },
    { "NUKLNCH1.AUD",  nullptr, 255, 5000, "Nuke launched" },
    { "NUKEATK1.AUD",  nullptr, 255, 5000, "Nuclear attack imminent" },
    { "GPSRDY1.AUD",   nullptr, 180, 30000, "GPS ready" },
    { "PARABRDY.AUD",  nullptr, 180, 30000, "Parabombs ready" },
    { "SPYRDY1.AUD",   nullptr, 150, 30000, "Spy plane ready" },

    // Mission
    { "ACCOM1.AUD",    nullptr, 255, 0, "Mission accomplished" },
    { "FAIL1.AUD",     nullptr, 255, 0, "Mission failed" },
    { "BATCON1.AUD",   nullptr, 200, 0, "Battle control online" },
    { "BATCONT1.AUD",  nullptr, 200, 0, "Battle control terminated" },

    // Multiplayer
    { "PLYDEFT1.AUD",  nullptr, 200, 5000, "Player defeated" },
    { "ALLATK1.AUD",   nullptr, 220, 30000, "Ally under attack" },
};

static_assert(sizeof(EVA_VOICE_INFO) / sizeof(EVA_VOICE_INFO[0]) ==
              static_cast<size_t>(EvaVoice::COUNT),
              "EVA_VOICE_INFO size mismatch");

static const UnitVoiceInfo UNIT_VOICE_INFO[] = {
    // NONE
    { nullptr, nullptr, 0 },

    // Selection
    { "REPORTN1.AUD", nullptr, 800 },     // REPORTING
    { "YESSIR1.AUD",  nullptr, 800 },     // YES_SIR
    { "READY1.AUD",   nullptr, 800 },     // READY
    { "AWARONE1.AUD", nullptr, 800 },     // AWAITING_ORDERS
    { "ATSERV1.AUD",  nullptr, 800 },     // AT_YOUR_SERVICE
    { "ACKNO1.AUD",   nullptr, 800 },     // ACKNOWLEDGED
    { "AFFIRM1.AUD",  nullptr, 800 },     // AFFIRMATIVE

    // Movement
    { "MOVOUT1.AUD",  nullptr, 600 },     // MOVING_OUT
    { "ONWAY1.AUD",   nullptr, 600 },     // ON_MY_WAY
    { "DOUBLE1.AUD",  nullptr, 600 },     // DOUBLE_TIME
    { "UGOTIT1.AUD",  nullptr, 600 },     // YOU_GOT_IT
    { "NODEST1.AUD",  nullptr, 600 },     // NO_PROBLEM
    { "ROGER1.AUD",   nullptr, 600 },     // ROGER

    // Attack
    { "ATACKNG1.AUD", nullptr, 500 },     // ATTACKING
    { "FIREONE1.AUD", nullptr, 500 },     // FIRING
    { "LETEM1.AUD",   nullptr, 500 },     // LET_EM_HAVE_IT
    { "FORMR1.AUD",   "FORMR1.AUD", 500 },  // FOR_MOTHER_RUSSIA (Soviet)
    { "FORKAC1.AUD",  nullptr, 500 },     // FOR_KING_COUNTRY (Allied)

    // Special units
    { "ENGR1.AUD",    nullptr, 800 },     // ENGINEER
    { "MEDIC1.AUD",   nullptr, 800 },     // MEDIC
    { "SPY1.AUD",     nullptr, 800 },     // SPY
    { "TANYA1.AUD",   nullptr, 800 },     // TANYA
    { "THIEF1.AUD",   nullptr, 800 },     // THIEF

    // Vehicles
    { "VEHIC1.AUD",   nullptr, 600 },     // VEHICLE
    { "TANK1.AUD",    nullptr, 600 },     // TANK
    { "COPTER1.AUD",  nullptr, 600 },     // HELICOPTER
    { "BOAT1.AUD",    nullptr, 600 },     // BOAT
};

static_assert(sizeof(UNIT_VOICE_INFO) / sizeof(UNIT_VOICE_INFO[0]) ==
              static_cast<size_t>(UnitVoice::COUNT),
              "UNIT_VOICE_INFO size mismatch");

//=============================================================================
// Voice Info Accessors
//=============================================================================

const EvaVoiceInfo& GetEvaVoiceInfo(EvaVoice voice) {
    int idx = static_cast<int>(voice);
    if (idx < 0 || idx >= static_cast<int>(EvaVoice::COUNT)) {
        return EVA_VOICE_INFO[0];
    }
    return EVA_VOICE_INFO[idx];
}

const UnitVoiceInfo& GetUnitVoiceInfo(UnitVoice voice) {
    int idx = static_cast<int>(voice);
    if (idx < 0 || idx >= static_cast<int>(UnitVoice::COUNT)) {
        return UNIT_VOICE_INFO[0];
    }
    return UNIT_VOICE_INFO[idx];
}

const char* GetEvaVoiceFilename(EvaVoice voice) {
    return GetEvaVoiceInfo(voice).filename;
}

const char* GetUnitVoiceFilename(UnitVoice voice, VoiceFaction faction) {
    const UnitVoiceInfo& info = GetUnitVoiceInfo(voice);
    if (faction == VoiceFaction::SOVIET && info.filename_soviet) {
        return info.filename_soviet;
    }
    return info.filename;
}

//=============================================================================
// VoiceManager Implementation
//=============================================================================

// Priority queue comparator (higher priority first)
static bool QueueComparator(const QueuedVoice& a, const QueuedVoice& b) {
    if (a.priority != b.priority) {
        return a.priority < b.priority;  // Lower priority comes first (min-heap)
    }
    return a.queue_time > b.queue_time;  // Earlier time has priority
}

VoiceManager& VoiceManager::Instance() {
    static VoiceManager instance;
    return instance;
}

VoiceManager::VoiceManager()
    : eva_queue_(QueueComparator)
    , eva_play_handle_(INVALID_PLAY_HANDLE)
    , current_eva_(EvaVoice::NONE)
    , eva_last_play_time_(0)
    , unit_play_handle_(INVALID_PLAY_HANDLE)
    , current_unit_(UnitVoice::NONE)
    , unit_last_play_time_(0)
    , volume_(1.0f)
    , muted_(false)
    , initialized_(false)
    , current_time_(0) {
}

VoiceManager::~VoiceManager() {
    Shutdown();
}

bool VoiceManager::Initialize() {
    if (initialized_) {
        return true;
    }

    Platform_LogInfo("VoiceManager: Initializing...");

    // Clear any existing data
    eva_voices_.clear();
    unit_voices_.clear();
    eva_last_played_.clear();
    unit_last_played_.clear();

    initialized_ = true;
    Platform_LogInfo("VoiceManager: Initialized");

    return true;
}

void VoiceManager::Shutdown() {
    if (!initialized_) {
        return;
    }

    Platform_LogInfo("VoiceManager: Shutting down...");

    StopEva();
    StopUnit();

    // Destroy all loaded voices
    for (auto& pair : eva_voices_) {
        if (pair.second.loaded && pair.second.handle != INVALID_SOUND_HANDLE) {
            Platform_Sound_Destroy(pair.second.handle);
        }
    }
    eva_voices_.clear();

    for (auto& pair : unit_voices_) {
        if (pair.second.loaded && pair.second.handle != INVALID_SOUND_HANDLE) {
            Platform_Sound_Destroy(pair.second.handle);
        }
    }
    unit_voices_.clear();

    // Clear queue
    while (!eva_queue_.empty()) {
        eva_queue_.pop();
    }

    initialized_ = false;
}

void VoiceManager::PreloadCommonVoices() {
    Platform_LogInfo("VoiceManager: Preloading common voices...");

    // Preload frequently used EVA voices
    static const EvaVoice common_eva[] = {
        EvaVoice::CONSTRUCTION_COMPLETE,
        EvaVoice::UNIT_READY,
        EvaVoice::BASE_UNDER_ATTACK,
        EvaVoice::LOW_POWER,
        EvaVoice::INSUFFICIENT_FUNDS,
    };

    for (EvaVoice v : common_eva) {
        LoadEvaVoice(v);
    }

    // Preload common unit voices
    static const UnitVoice common_unit[] = {
        UnitVoice::REPORTING,
        UnitVoice::ACKNOWLEDGED,
        UnitVoice::AFFIRMATIVE,
        UnitVoice::MOVING_OUT,
        UnitVoice::ATTACKING,
    };

    for (UnitVoice v : common_unit) {
        LoadUnitVoice(v, VoiceFaction::NEUTRAL);
    }
}

//=============================================================================
// Voice Loading
//=============================================================================

bool VoiceManager::LoadEvaVoice(EvaVoice voice) {
    int key = static_cast<int>(voice);

    // Already loaded?
    auto it = eva_voices_.find(key);
    if (it != eva_voices_.end() && it->second.loaded) {
        return true;
    }

    const char* filename = GetEvaVoiceFilename(voice);
    if (!filename) {
        return false;
    }

    AudFile aud;
    if (!aud.LoadFromMix(filename)) {
        eva_voices_[key] = {INVALID_SOUND_HANDLE, false};
        return false;
    }

    SoundHandle handle = Platform_Sound_CreateFromMemory(
        aud.GetPCMDataBytes(),
        static_cast<int32_t>(aud.GetPCMDataSize()),
        aud.GetSampleRate(),
        aud.GetChannels(),
        16
    );

    if (handle == INVALID_SOUND_HANDLE) {
        eva_voices_[key] = {INVALID_SOUND_HANDLE, false};
        return false;
    }

    eva_voices_[key] = {handle, true};
    return true;
}

bool VoiceManager::LoadUnitVoice(UnitVoice voice, VoiceFaction faction) {
    int key = GetUnitVoiceKey(voice, faction);

    auto it = unit_voices_.find(key);
    if (it != unit_voices_.end() && it->second.loaded) {
        return true;
    }

    const char* filename = GetUnitVoiceFilename(voice, faction);
    if (!filename) {
        return false;
    }

    AudFile aud;
    if (!aud.LoadFromMix(filename)) {
        unit_voices_[key] = {INVALID_SOUND_HANDLE, false};
        return false;
    }

    SoundHandle handle = Platform_Sound_CreateFromMemory(
        aud.GetPCMDataBytes(),
        static_cast<int32_t>(aud.GetPCMDataSize()),
        aud.GetSampleRate(),
        aud.GetChannels(),
        16
    );

    if (handle == INVALID_SOUND_HANDLE) {
        unit_voices_[key] = {INVALID_SOUND_HANDLE, false};
        return false;
    }

    unit_voices_[key] = {handle, true};
    return true;
}

SoundHandle VoiceManager::GetEvaSound(EvaVoice voice) {
    int key = static_cast<int>(voice);

    auto it = eva_voices_.find(key);
    if (it != eva_voices_.end() && it->second.loaded) {
        return it->second.handle;
    }

    // Try to load
    if (LoadEvaVoice(voice)) {
        return eva_voices_[key].handle;
    }

    return INVALID_SOUND_HANDLE;
}

SoundHandle VoiceManager::GetUnitSound(UnitVoice voice, VoiceFaction faction) {
    int key = GetUnitVoiceKey(voice, faction);

    auto it = unit_voices_.find(key);
    if (it != unit_voices_.end() && it->second.loaded) {
        return it->second.handle;
    }

    if (LoadUnitVoice(voice, faction)) {
        return unit_voices_[key].handle;
    }

    return INVALID_SOUND_HANDLE;
}

int VoiceManager::GetUnitVoiceKey(UnitVoice voice, VoiceFaction faction) const {
    return static_cast<int>(voice) * 10 + static_cast<int>(faction);
}

//=============================================================================
// EVA Voice Playback
//=============================================================================

bool VoiceManager::PlayEva(EvaVoice voice) {
    if (!initialized_ || muted_ || voice == EvaVoice::NONE) {
        return false;
    }

    // Rate limiting check
    if (!CanPlayEva(voice)) {
        return false;
    }

    const EvaVoiceInfo& info = GetEvaVoiceInfo(voice);

    // If nothing playing, play immediately
    if (!IsEvaSpeaking()) {
        return DoPlayEva(voice);
    }

    // Check if new voice has higher priority
    const EvaVoiceInfo& current_info = GetEvaVoiceInfo(current_eva_);
    if (info.priority > current_info.priority) {
        // Interrupt current voice
        StopEva();
        return DoPlayEva(voice);
    }

    // Queue the voice
    QueueEva(voice);
    return true;
}

void VoiceManager::QueueEva(EvaVoice voice) {
    if (voice == EvaVoice::NONE) return;

    const EvaVoiceInfo& info = GetEvaVoiceInfo(voice);

    QueuedVoice qv;
    qv.voice = voice;
    qv.priority = info.priority;
    qv.queue_time = current_time_;

    eva_queue_.push(qv);
}

bool VoiceManager::DoPlayEva(EvaVoice voice) {
    SoundHandle sound = GetEvaSound(voice);
    if (sound == INVALID_SOUND_HANDLE) {
        return false;
    }

    float vol = muted_ ? 0.0f : volume_;
    eva_play_handle_ = Platform_Sound_Play(sound, vol, 0.0f, false);

    if (eva_play_handle_ != INVALID_PLAY_HANDLE) {
        current_eva_ = voice;
        eva_last_play_time_ = current_time_;
        eva_last_played_[static_cast<int>(voice)] = current_time_;
        return true;
    }

    return false;
}

void VoiceManager::StopEva() {
    if (eva_play_handle_ != INVALID_PLAY_HANDLE) {
        Platform_Sound_Stop(eva_play_handle_);
        eva_play_handle_ = INVALID_PLAY_HANDLE;
    }
    current_eva_ = EvaVoice::NONE;
}

bool VoiceManager::IsEvaSpeaking() const {
    if (eva_play_handle_ == INVALID_PLAY_HANDLE) {
        return false;
    }
    return Platform_Sound_IsPlaying(eva_play_handle_);
}

void VoiceManager::ClearEvaQueue() {
    while (!eva_queue_.empty()) {
        eva_queue_.pop();
    }
}

bool VoiceManager::CanPlayEva(EvaVoice voice) const {
    int key = static_cast<int>(voice);
    auto it = eva_last_played_.find(key);
    if (it == eva_last_played_.end()) {
        return true;
    }

    const EvaVoiceInfo& info = GetEvaVoiceInfo(voice);
    return (current_time_ - it->second) >= info.min_interval_ms;
}

void VoiceManager::ProcessEvaQueue() {
    if (IsEvaSpeaking() || eva_queue_.empty()) {
        return;
    }

    // Get highest priority voice
    QueuedVoice qv = eva_queue_.top();
    eva_queue_.pop();

    // Check if still rate-limited
    if (CanPlayEva(qv.voice)) {
        DoPlayEva(qv.voice);
    } else if (!eva_queue_.empty()) {
        // Try next in queue
        ProcessEvaQueue();
    }
}

//=============================================================================
// Unit Voice Playback
//=============================================================================

PlayHandle VoiceManager::PlayUnit(UnitVoice voice, VoiceFaction faction) {
    if (!initialized_ || muted_ || voice == UnitVoice::NONE) {
        return INVALID_PLAY_HANDLE;
    }

    if (!CanPlayUnit(voice)) {
        return INVALID_PLAY_HANDLE;
    }

    SoundHandle sound = GetUnitSound(voice, faction);
    if (sound == INVALID_SOUND_HANDLE) {
        return INVALID_PLAY_HANDLE;
    }

    // Stop current unit voice
    StopUnit();

    float vol = muted_ ? 0.0f : volume_;
    unit_play_handle_ = Platform_Sound_Play(sound, vol, 0.0f, false);

    if (unit_play_handle_ != INVALID_PLAY_HANDLE) {
        current_unit_ = voice;
        unit_last_play_time_ = current_time_;
        unit_last_played_[static_cast<int>(voice)] = current_time_;
    }

    return unit_play_handle_;
}

PlayHandle VoiceManager::PlayUnitAt(UnitVoice voice, int world_x, int world_y,
                                     VoiceFaction faction) {
    // For now, unit voices are not positional (could add later)
    (void)world_x;
    (void)world_y;
    return PlayUnit(voice, faction);
}

void VoiceManager::StopUnit() {
    if (unit_play_handle_ != INVALID_PLAY_HANDLE) {
        Platform_Sound_Stop(unit_play_handle_);
        unit_play_handle_ = INVALID_PLAY_HANDLE;
    }
    current_unit_ = UnitVoice::NONE;
}

bool VoiceManager::IsUnitSpeaking() const {
    if (unit_play_handle_ == INVALID_PLAY_HANDLE) {
        return false;
    }
    return Platform_Sound_IsPlaying(unit_play_handle_);
}

bool VoiceManager::CanPlayUnit(UnitVoice voice) const {
    int key = static_cast<int>(voice);
    auto it = unit_last_played_.find(key);
    if (it == unit_last_played_.end()) {
        return true;
    }

    const UnitVoiceInfo& info = GetUnitVoiceInfo(voice);
    return (current_time_ - it->second) >= info.min_interval_ms;
}

//=============================================================================
// Volume Control
//=============================================================================

void VoiceManager::SetVolume(float volume) {
    volume_ = std::clamp(volume, 0.0f, 1.0f);

    // Update playing voices
    if (eva_play_handle_ != INVALID_PLAY_HANDLE) {
        Platform_Sound_SetVolume(eva_play_handle_, muted_ ? 0.0f : volume_);
    }
    if (unit_play_handle_ != INVALID_PLAY_HANDLE) {
        Platform_Sound_SetVolume(unit_play_handle_, muted_ ? 0.0f : volume_);
    }
}

void VoiceManager::SetMuted(bool muted) {
    muted_ = muted;

    float vol = muted ? 0.0f : volume_;
    if (eva_play_handle_ != INVALID_PLAY_HANDLE) {
        Platform_Sound_SetVolume(eva_play_handle_, vol);
    }
    if (unit_play_handle_ != INVALID_PLAY_HANDLE) {
        Platform_Sound_SetVolume(unit_play_handle_, vol);
    }
}

//=============================================================================
// Update
//=============================================================================

void VoiceManager::Update() {
    if (!initialized_) {
        return;
    }

    current_time_ = Platform_GetTicks();

    // Check if EVA voice finished
    if (eva_play_handle_ != INVALID_PLAY_HANDLE &&
        !Platform_Sound_IsPlaying(eva_play_handle_)) {
        eva_play_handle_ = INVALID_PLAY_HANDLE;
        current_eva_ = EvaVoice::NONE;
    }

    // Check if unit voice finished
    if (unit_play_handle_ != INVALID_PLAY_HANDLE &&
        !Platform_Sound_IsPlaying(unit_play_handle_)) {
        unit_play_handle_ = INVALID_PLAY_HANDLE;
        current_unit_ = UnitVoice::NONE;
    }

    // Process EVA queue
    ProcessEvaQueue();
}

//=============================================================================
// Debug
//=============================================================================

void VoiceManager::PrintStatus() const {
    printf("VoiceManager Status:\n");
    printf("  EVA Speaking: %s\n", IsEvaSpeaking() ? "yes" : "no");
    if (current_eva_ != EvaVoice::NONE) {
        printf("  Current EVA: %s\n", GetEvaVoiceInfo(current_eva_).description);
    }
    printf("  EVA Queue: %zu\n", eva_queue_.size());
    printf("  Unit Speaking: %s\n", IsUnitSpeaking() ? "yes" : "no");
    printf("  Volume: %.0f%%\n", volume_ * 100);
    printf("  Muted: %s\n", muted_ ? "yes" : "no");
}

//=============================================================================
// Global Functions
//=============================================================================

bool Voice_Init() {
    return VoiceManager::Instance().Initialize();
}

void Voice_Shutdown() {
    VoiceManager::Instance().Shutdown();
}

void Voice_Update() {
    VoiceManager::Instance().Update();
}

void Voice_PlayEva(EvaVoice voice) {
    VoiceManager::Instance().PlayEva(voice);
}

void Voice_QueueEva(EvaVoice voice) {
    VoiceManager::Instance().QueueEva(voice);
}

PlayHandle Voice_PlayUnit(UnitVoice voice) {
    return VoiceManager::Instance().PlayUnit(voice);
}

void Voice_SetVolume(int volume) {
    float vol = std::clamp(volume, 0, 255) / 255.0f;
    VoiceManager::Instance().SetVolume(vol);
}

int Voice_GetVolume() {
    return static_cast<int>(VoiceManager::Instance().GetVolume() * 255);
}
