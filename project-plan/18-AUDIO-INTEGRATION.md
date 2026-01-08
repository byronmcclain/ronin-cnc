# Phase 18: Audio Integration

## Overview

This phase integrates sound effects and music playback using the platform audio system (rodio backend).

---

## Platform Audio API

```c
// Initialization
int Platform_Audio_Init(void);
void Platform_Audio_Shutdown(void);
bool Platform_Audio_IsInitialized(void);

// Sound playback
SoundHandle Platform_Sound_Play(const uint8_t* data, uint32_t size,
                                 int volume, bool looping);
void Platform_Sound_Stop(SoundHandle handle);
void Platform_Sound_StopAll(void);

// Volume control
void Platform_Sound_SetVolume(SoundHandle handle, int volume);
void Platform_Sound_SetMasterVolume(int volume);
int Platform_Sound_GetMasterVolume(void);

// Status
bool Platform_Sound_IsPlaying(SoundHandle handle);
int Platform_Sound_GetPlayingCount(void);

// PCM formats supported
#define AUDIO_FORMAT_PCM_U8     1   // 8-bit unsigned
#define AUDIO_FORMAT_PCM_S16    2   // 16-bit signed
#define AUDIO_FORMAT_ADPCM      3   // IMA ADPCM (compressed)
```

---

## Audio File Formats

### AUD Files (Primary Sound Format)

Red Alert uses Westwood's AUD format for sound effects and voice.

**Header (12 bytes):**
```
Offset  Size  Description
------  ----  -----------
0x0000  2     Sample rate (usually 22050)
0x0002  4     Uncompressed size
0x0004  4     Compressed size
0x0008  1     Flags (bit 0 = stereo, bit 1 = 16-bit)
0x0009  1     Compression type (1 = WW ADPCM, 99 = IMA ADPCM)
```

**Data:**
After header, either raw PCM or compressed ADPCM chunks.

### VQA Files (Video with Audio)

Movies use VQA format with embedded audio tracks. Audio portion is IMA ADPCM.

---

## Task 18.1: AUD File Loader

```cpp
// src/game/aud_file.h
#pragma once

#include <cstdint>
#include <vector>

struct AudHeader {
    uint16_t sample_rate;
    uint32_t uncompressed_size;
    uint32_t compressed_size;
    uint8_t flags;
    uint8_t compression;
};

class AudFile {
public:
    bool LoadFromData(const uint8_t* data, uint32_t size);
    bool LoadFromMix(const char* filename);

    // Get decoded PCM data
    const uint8_t* GetPCMData() const { return pcm_data_.data(); }
    uint32_t GetPCMSize() const { return pcm_data_.size(); }

    // Audio properties
    uint16_t GetSampleRate() const { return header_.sample_rate; }
    bool IsStereo() const { return header_.flags & 0x01; }
    bool Is16Bit() const { return header_.flags & 0x02; }

private:
    AudHeader header_;
    std::vector<uint8_t> pcm_data_;

    bool DecodeWWADPCM(const uint8_t* src, uint32_t src_size);
    bool DecodeIMADPCM(const uint8_t* src, uint32_t src_size);
};
```

**Implementation:**
```cpp
// src/game/aud_file.cpp
#include "aud_file.h"
#include "mix_manager.h"

bool AudFile::LoadFromData(const uint8_t* data, uint32_t size) {
    if (size < 12) return false;

    // Parse header
    header_.sample_rate = *(uint16_t*)data;
    header_.uncompressed_size = *(uint32_t*)(data + 2);
    header_.compressed_size = *(uint32_t*)(data + 6);
    header_.flags = data[10];
    header_.compression = data[11];

    const uint8_t* audio_data = data + 12;
    uint32_t audio_size = size - 12;

    // Decompress based on type
    if (header_.compression == 1) {
        return DecodeWWADPCM(audio_data, audio_size);
    } else if (header_.compression == 99) {
        // IMA ADPCM - use platform decoder
        pcm_data_.resize(header_.uncompressed_size);
        // Platform_Audio_DecodeADPCM available
        return true;
    } else {
        // Uncompressed - copy directly
        pcm_data_.assign(audio_data, audio_data + audio_size);
        return true;
    }
}

bool AudFile::LoadFromMix(const char* filename) {
    uint32_t size;
    const uint8_t* data = MixManager::Instance().FindFile(filename, &size);
    if (!data) return false;
    return LoadFromData(data, size);
}
```

---

## Task 18.2: Sound Effect Manager

```cpp
// src/game/sound_manager.h
#pragma once

#include "platform.h"
#include <string>
#include <unordered_map>

// Sound effect IDs (matching original game)
enum SoundEffect {
    SFX_NONE = 0,

    // UI sounds
    SFX_CLICK,
    SFX_BUILD_COMPLETE,
    SFX_CANNOT,
    SFX_PRIMARY,

    // Combat
    SFX_EXPLOSION_SMALL,
    SFX_EXPLOSION_LARGE,
    SFX_GUN_SHOT,
    SFX_CANNON,
    SFX_MISSILE_LAUNCH,
    SFX_MISSILE_HIT,

    // Units
    SFX_TANK_MOVE,
    SFX_TRUCK_MOVE,
    SFX_INFANTRY_MOVE,
    SFX_HELICOPTER,

    // Voice
    SFX_UNIT_REPORTING,
    SFX_UNIT_MOVING,
    SFX_UNIT_ATTACKING,
    SFX_BUILDING,
    SFX_CONSTRUCTION_COMPLETE,
    SFX_UNIT_READY,
    SFX_UNIT_LOST,
    SFX_LOW_POWER,
    SFX_INSUFFICIENT_FUNDS,

    SFX_COUNT
};

class SoundManager {
public:
    static SoundManager& Instance();

    bool Initialize();
    void Shutdown();

    // Load all sound effects
    void LoadSounds();

    // Play sound effect
    SoundHandle Play(SoundEffect sfx, int volume = 255);
    SoundHandle PlayAt(SoundEffect sfx, int world_x, int world_y, int volume = 255);

    // Positional audio
    void SetListenerPosition(int x, int y);

    // Volume control
    void SetSFXVolume(int volume);
    void SetVoiceVolume(int volume);

    // Stop sounds
    void StopAll();

private:
    struct SoundData {
        std::vector<uint8_t> pcm;
        uint16_t sample_rate;
        bool stereo;
        bool loaded;
    };

    SoundData sounds_[SFX_COUNT];
    int sfx_volume_ = 255;
    int voice_volume_ = 255;
    int listener_x_ = 0;
    int listener_y_ = 0;

    const char* GetSoundFilename(SoundEffect sfx);
    int CalculateVolume(int base_vol, int x, int y);
};
```

**Implementation:**
```cpp
// src/game/sound_manager.cpp
#include "sound_manager.h"
#include "aud_file.h"

// Sound filename mapping
static const char* SOUND_FILES[] = {
    nullptr,           // SFX_NONE
    "CLICK.AUD",
    "COMPLET1.AUD",
    "CANNOT.AUD",
    "PRIMARY.AUD",
    "XPLOS.AUD",
    "XPLOBIG.AUD",
    "GUNSHOT.AUD",
    "CANNON.AUD",
    // ... etc
};

void SoundManager::LoadSounds() {
    for (int i = 1; i < SFX_COUNT; i++) {
        const char* filename = SOUND_FILES[i];
        if (!filename) continue;

        AudFile aud;
        if (aud.LoadFromMix(filename)) {
            sounds_[i].pcm.assign(
                aud.GetPCMData(),
                aud.GetPCMData() + aud.GetPCMSize()
            );
            sounds_[i].sample_rate = aud.GetSampleRate();
            sounds_[i].stereo = aud.IsStereo();
            sounds_[i].loaded = true;
        }
    }
}

SoundHandle SoundManager::Play(SoundEffect sfx, int volume) {
    if (sfx <= SFX_NONE || sfx >= SFX_COUNT) return 0;
    if (!sounds_[sfx].loaded) return 0;

    int adjusted_vol = (volume * sfx_volume_) / 255;

    return Platform_Sound_Play(
        sounds_[sfx].pcm.data(),
        sounds_[sfx].pcm.size(),
        adjusted_vol,
        false  // not looping
    );
}

SoundHandle SoundManager::PlayAt(SoundEffect sfx, int world_x, int world_y, int vol) {
    int adjusted_vol = CalculateVolume(vol, world_x, world_y);
    if (adjusted_vol <= 0) return 0;  // Too far to hear
    return Play(sfx, adjusted_vol);
}

int SoundManager::CalculateVolume(int base_vol, int x, int y) {
    // Distance-based attenuation
    int dx = x - listener_x_;
    int dy = y - listener_y_;
    int dist = (int)sqrt(dx*dx + dy*dy);

    const int MAX_DIST = 1000;  // In world pixels
    if (dist >= MAX_DIST) return 0;

    return base_vol * (MAX_DIST - dist) / MAX_DIST;
}
```

---

## Task 18.3: Music System

```cpp
// src/game/music.h
#pragma once

#include "platform.h"

// Music tracks
enum MusicTrack {
    MUSIC_NONE = 0,
    MUSIC_TITLE,
    MUSIC_MENU,
    MUSIC_SCORE,
    MUSIC_MAP,

    // In-game tracks
    MUSIC_BIGFOOT,
    MUSIC_CRUSH,
    MUSIC_FACE,
    MUSIC_HELL_MARCH,
    MUSIC_RUN,
    MUSIC_SMASH,
    MUSIC_TWINMIX,
    MUSIC_WARFARE,

    MUSIC_COUNT
};

class MusicPlayer {
public:
    static MusicPlayer& Instance();

    bool Initialize();
    void Shutdown();

    void Play(MusicTrack track, bool loop = true);
    void Stop();
    void Pause();
    void Resume();

    void SetVolume(int volume);
    bool IsPlaying() const;

    // Shuffle mode
    void EnableShuffle(bool enable);
    void PlayNext();

private:
    SoundHandle current_handle_;
    MusicTrack current_track_;
    int volume_ = 200;
    bool shuffle_ = false;
    bool looping_ = true;

    std::vector<uint8_t> LoadTrack(MusicTrack track);
};
```

**Track loading:**
```cpp
// Music is typically in SCORES.MIX or on CD as AUDIO*.AUD
static const char* MUSIC_FILES[] = {
    nullptr,          // MUSIC_NONE
    "INTRO.AUD",      // MUSIC_TITLE
    "MENU.AUD",       // MUSIC_MENU
    "SCORE.AUD",      // MUSIC_SCORE
    "MAP.AUD",        // MUSIC_MAP
    "BIGFOOT.AUD",
    "CRUSH.AUD",
    "FACE.AUD",
    "HELLMRCH.AUD",   // Hell March!
    "RUN.AUD",
    "SMASH.AUD",
    "TWINMIX.AUD",
    "WARFARE.AUD",
};

void MusicPlayer::Play(MusicTrack track, bool loop) {
    if (track == current_track_ && IsPlaying()) return;

    Stop();

    std::vector<uint8_t> data = LoadTrack(track);
    if (data.empty()) return;

    current_handle_ = Platform_Sound_Play(
        data.data(), data.size(),
        volume_, loop
    );
    current_track_ = track;
    looping_ = loop;
}
```

---

## Task 18.4: Voice System

```cpp
// src/game/voice.h
#pragma once

// Voice samples for different factions
enum VoiceEvent {
    VOICE_UNIT_REPORTING,
    VOICE_UNIT_MOVING,
    VOICE_UNIT_ATTACKING,
    VOICE_ACKNOWLEDGED,
    VOICE_AFFIRMATIVE,
    VOICE_YES_SIR,

    // EVA voice
    VOICE_BUILDING,
    VOICE_CONSTRUCTION_COMPLETE,
    VOICE_UNIT_READY,
    VOICE_UNIT_LOST,
    VOICE_OUR_BASE_UNDER_ATTACK,
    VOICE_LOW_POWER,
    VOICE_INSUFFICIENT_FUNDS,
    VOICE_SILOS_NEEDED,
    VOICE_RADAR_ONLINE,
    VOICE_RADAR_OFFLINE,
    VOICE_MISSION_ACCOMPLISHED,
    VOICE_MISSION_FAILED,

    VOICE_COUNT
};

class VoiceManager {
public:
    static VoiceManager& Instance();

    void PlayVoice(VoiceEvent event, int faction = 0);
    void PlayEVA(VoiceEvent event);

    void SetVolume(int volume);
    void StopVoice();

private:
    SoundHandle current_voice_;
    int volume_ = 255;
    uint32_t last_voice_time_;

    // Don't spam voices
    bool CanPlayVoice();
};

// Voice files are typically in SPEECH.MIX
static const char* VOICE_FILES[] = {
    "REPORTN1.AUD",  // VOICE_UNIT_REPORTING
    "MOVIG1.AUD",    // VOICE_UNIT_MOVING
    "ATACKNG1.AUD",  // VOICE_UNIT_ATTACKING
    "ACKNO1.AUD",
    "AFFIRM1.AUD",
    "YESSIR1.AUD",
    // EVA voices
    "BLDG1.AUD",
    "CONSTRU1.AUD",
    "UNITREDY.AUD",
    "UNITLOST.AUD",
    "BASEATK1.AUD",
    "LOWPOWR1.AUD",
    "INSUFUND.AUD",
    "NEEDSILO.AUD",
    "RADARON1.AUD",
    "RADAROFF.AUD",
    "ACCOM1.AUD",
    "FAIL1.AUD",
};
```

---

## Task 18.5: Audio Integration

```cpp
// src/game/audio.cpp
#include "sound_manager.h"
#include "music.h"
#include "voice.h"
#include "platform.h"

void Audio_Init() {
    Platform_Audio_Init();

    SoundManager::Instance().Initialize();
    SoundManager::Instance().LoadSounds();

    MusicPlayer::Instance().Initialize();
    VoiceManager::Instance().SetVolume(255);
}

void Audio_Shutdown() {
    MusicPlayer::Instance().Shutdown();
    SoundManager::Instance().Shutdown();
    Platform_Audio_Shutdown();
}

void Audio_Update() {
    // Update listener position to camera center
    int cx = TheViewport.x + TheViewport.width / 2;
    int cy = TheViewport.y + TheViewport.height / 2;
    SoundManager::Instance().SetListenerPosition(cx, cy);
}

// Convenience functions for game code
void Play_Sound(SoundEffect sfx) {
    SoundManager::Instance().Play(sfx);
}

void Play_Sound_At(SoundEffect sfx, COORDINATE coord) {
    SoundManager::Instance().PlayAt(sfx, Coord_X(coord), Coord_Y(coord));
}

void Play_Voice(VoiceEvent voice) {
    VoiceManager::Instance().PlayVoice(voice);
}

void Play_EVA(VoiceEvent voice) {
    VoiceManager::Instance().PlayEVA(voice);
}

void Play_Music(MusicTrack track) {
    MusicPlayer::Instance().Play(track);
}

void Stop_Music() {
    MusicPlayer::Instance().Stop();
}
```

---

## Task 18.6: Game Audio Events

```cpp
// Hook audio into game events
// src/game/audio_events.cpp

// Called when unit takes damage
void On_Unit_Damaged(ObjectClass* obj, int damage) {
    if (damage > 50) {
        Play_Sound_At(SFX_EXPLOSION_LARGE, obj->Coord);
    } else {
        Play_Sound_At(SFX_EXPLOSION_SMALL, obj->Coord);
    }
}

// Called when unit is selected
void On_Unit_Selected(ObjectClass* obj) {
    Play_Voice(VOICE_UNIT_REPORTING);
}

// Called when unit is given move order
void On_Unit_Move_Order(ObjectClass* obj) {
    Play_Voice(VOICE_UNIT_MOVING);
}

// Called when construction completes
void On_Building_Complete(BuildingClass* bld) {
    Play_EVA(VOICE_CONSTRUCTION_COMPLETE);
}

// Called when unit is produced
void On_Unit_Ready(UnitClass* unit) {
    Play_EVA(VOICE_UNIT_READY);
}

// Called when credits are insufficient
void On_Insufficient_Funds() {
    // Don't spam - rate limit
    static uint32_t last_time = 0;
    uint32_t now = Platform_Timer_GetTicks();
    if (now - last_time > 5000) {  // 5 seconds
        Play_EVA(VOICE_INSUFFICIENT_FUNDS);
        last_time = now;
    }
}

// Called when base is attacked
void On_Base_Under_Attack() {
    static uint32_t last_time = 0;
    uint32_t now = Platform_Timer_GetTicks();
    if (now - last_time > 30000) {  // 30 seconds
        Play_EVA(VOICE_OUR_BASE_UNDER_ATTACK);
        last_time = now;
    }
}
```

---

## Testing

```cpp
// test_audio.cpp

void Test_Sound_Effects() {
    Audio_Init();

    printf("Playing test sounds...\n");

    Play_Sound(SFX_CLICK);
    Platform_Timer_Delay(500);

    Play_Sound(SFX_EXPLOSION_SMALL);
    Platform_Timer_Delay(1000);

    Play_Sound(SFX_EXPLOSION_LARGE);
    Platform_Timer_Delay(1500);

    Audio_Shutdown();
}

void Test_Music() {
    Audio_Init();

    printf("Playing Hell March...\n");
    Play_Music(MUSIC_HELL_MARCH);

    // Wait or until key press
    while (!Platform_Key_JustPressed(KEY_CODE_ESCAPE)) {
        Platform_Input_Update();
        Platform_Timer_Delay(16);
    }

    Stop_Music();
    Audio_Shutdown();
}

void Test_Voice() {
    Audio_Init();

    Play_EVA(VOICE_CONSTRUCTION_COMPLETE);
    Platform_Timer_Delay(2000);

    Play_Voice(VOICE_UNIT_REPORTING);
    Platform_Timer_Delay(1500);

    Play_EVA(VOICE_UNIT_READY);
    Platform_Timer_Delay(2000);

    Audio_Shutdown();
}
```

---

## Success Criteria

- [ ] AUD files load and decode
- [ ] Sound effects play correctly
- [ ] Positional audio attenuates with distance
- [ ] Music tracks loop properly
- [ ] Voice lines play without overlap
- [ ] EVA announcements trigger
- [ ] Volume controls work
- [ ] No audio glitches or pops
- [ ] Clean shutdown (no playing sounds)

---

## Estimated Effort

| Task | Hours |
|------|-------|
| 18.1 AUD Loader | 3-4 |
| 18.2 Sound Manager | 3-4 |
| 18.3 Music System | 2-3 |
| 18.4 Voice System | 2-3 |
| 18.5 Integration | 2-3 |
| 18.6 Game Events | 2-3 |
| Testing | 2-3 |
| **Total** | **16-23** |
