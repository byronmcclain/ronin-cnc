# Plan 08: Audio System

## Objective
Replace DirectSound with SDL2_mixer or OpenAL, supporting the game's 8-bit/16-bit PCM audio and IMA ADPCM compressed audio formats.

## Current State Analysis

### DirectSound Usage in Codebase

**Primary Files:**
- `WIN32LIB/AUDIO/SOUNDIO.CPP` (2208 lines)
- `WIN32LIB/AUDIO/SOUNDINT.CPP`
- `WIN32LIB/AUDIO/SOUNDLCK.CPP`
- `WIN32LIB/INCLUDE/AUDIO.H`
- `WIN32LIB/INCLUDE/SOUNDINT.H`
- `CODE/AUDIO.CPP`, `CODE/AUDIO.H`
- `CODE/ADPCM.CPP` - ADPCM codec

**DirectSound Objects:**
```cpp
LPDIRECTSOUND SoundObject;
LPDIRECTSOUNDBUFFER PrimaryBufferPtr;
LPDIRECTSOUNDBUFFER DumpBuffer;  // Mixing buffer
CRITICAL_SECTION GlobalAudioCriticalSection;
```

**Audio Formats:**
- 8-bit unsigned PCM
- 16-bit signed PCM
- Mono and stereo
- Sample rates: 11025, 22050 Hz
- IMA ADPCM compressed (AUD files)

**Key Structures:**
```cpp
struct SampleTrackerType {
    LPDIRECTSOUNDBUFFER PlayBuffer;
    void* Original;          // Original sample data
    int OriginalSize;
    int Priority;
    int Volume;
    int Pan;
    bool IsLooping;
    bool Active;
};

#define MAX_SAMPLE_TRACKERS 16  // Concurrent sounds
```

**Audio Features:**
- 16 simultaneous sound channels
- Volume control (0-255)
- Panning (-128 to +127)
- Looping sounds
- Priority-based channel stealing
- Streaming for large files (music)

## SDL2_mixer Implementation

Using SDL2_mixer for simplicity. Can upgrade to OpenAL later for 3D audio.

### 8.1 Audio State

File: `src/platform/audio_sdl.cpp`
```cpp
#include "platform/platform_audio.h"
#include <SDL.h>
#include <SDL_mixer.h>
#include <vector>
#include <mutex>
#include <cstring>

namespace {

//=============================================================================
// Audio Configuration
//=============================================================================

AudioConfig g_audio_config = {22050, 2, 16, 1024};
float g_master_volume = 1.0f;
bool g_audio_initialized = false;

//=============================================================================
// Sound Management
//=============================================================================

struct SoundData {
    Mix_Chunk* chunk;
    int sample_rate;
    int channels;
    int bits_per_sample;
};

struct PlayingSound {
    int channel;
    SoundData* sound;
    bool active;
};

std::vector<SoundData*> g_sounds;
std::vector<PlayingSound> g_playing;
std::mutex g_audio_mutex;

//=============================================================================
// Streaming Audio
//=============================================================================

struct StreamData {
    SDL_AudioDeviceID device;
    AudioStreamCallback callback;
    void* userdata;
    int sample_rate;
    int channels;
    int bits_per_sample;
    float volume;
    bool playing;
};

std::vector<StreamData*> g_streams;

} // anonymous namespace
```

### 8.2 Initialization

```cpp
bool Platform_Audio_Init(const AudioConfig* config) {
    if (g_audio_initialized) return true;

    if (config) {
        g_audio_config = *config;
    }

    // Initialize SDL audio
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        Platform_SetError("SDL_InitSubSystem(AUDIO) failed: %s", SDL_GetError());
        return false;
    }

    // Determine format
    Uint16 format;
    if (g_audio_config.bits_per_sample == 8) {
        format = AUDIO_U8;
    } else {
        format = AUDIO_S16SYS;
    }

    // Open mixer
    if (Mix_OpenAudio(g_audio_config.sample_rate,
                      format,
                      g_audio_config.channels,
                      g_audio_config.buffer_size) < 0) {
        Platform_SetError("Mix_OpenAudio failed: %s", Mix_GetError());
        return false;
    }

    // Allocate mixing channels
    Mix_AllocateChannels(32);  // More than the game's 16

    g_audio_initialized = true;
    return true;
}

void Platform_Audio_Shutdown(void) {
    if (!g_audio_initialized) return;

    // Stop all sounds
    Mix_HaltChannel(-1);

    // Free all loaded sounds
    for (SoundData* sound : g_sounds) {
        if (sound->chunk) {
            Mix_FreeChunk(sound->chunk);
        }
        delete sound;
    }
    g_sounds.clear();

    // Stop all streams
    for (StreamData* stream : g_streams) {
        if (stream->device) {
            SDL_CloseAudioDevice(stream->device);
        }
        delete stream;
    }
    g_streams.clear();

    Mix_CloseAudio();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    g_audio_initialized = false;
}

void Platform_Audio_SetMasterVolume(float volume) {
    g_master_volume = (volume < 0.0f) ? 0.0f : (volume > 1.0f) ? 1.0f : volume;
    Mix_Volume(-1, (int)(g_master_volume * MIX_MAX_VOLUME));
}

float Platform_Audio_GetMasterVolume(void) {
    return g_master_volume;
}
```

### 8.3 Sound Loading

```cpp
PlatformSound* Platform_Sound_CreateFromMemory(const void* data, int size,
                                                int sample_rate, int channels,
                                                int bits_per_sample) {
    if (!g_audio_initialized) return nullptr;

    // Create SDL_RWops from memory
    SDL_RWops* rw = SDL_RWFromConstMem(data, size);
    if (!rw) {
        Platform_SetError("SDL_RWFromConstMem failed: %s", SDL_GetError());
        return nullptr;
    }

    // Determine format
    Uint16 format;
    if (bits_per_sample == 8) {
        format = AUDIO_U8;
    } else {
        format = AUDIO_S16LSB;  // Little-endian 16-bit
    }

    // Create audio spec
    SDL_AudioSpec src_spec;
    src_spec.freq = sample_rate;
    src_spec.format = format;
    src_spec.channels = channels;
    src_spec.samples = 4096;
    src_spec.callback = nullptr;

    // Convert to mixer format if needed
    SDL_AudioSpec dst_spec;
    dst_spec.freq = g_audio_config.sample_rate;
    dst_spec.format = (g_audio_config.bits_per_sample == 8) ? AUDIO_U8 : AUDIO_S16SYS;
    dst_spec.channels = g_audio_config.channels;
    dst_spec.samples = 4096;
    dst_spec.callback = nullptr;

    SDL_AudioCVT cvt;
    int cvt_needed = SDL_BuildAudioCVT(&cvt,
                                        src_spec.format, src_spec.channels, src_spec.freq,
                                        dst_spec.format, dst_spec.channels, dst_spec.freq);

    Uint8* converted_data;
    int converted_size;

    if (cvt_needed > 0) {
        // Need conversion
        cvt.len = size;
        cvt.buf = (Uint8*)SDL_malloc(size * cvt.len_mult);
        memcpy(cvt.buf, data, size);
        SDL_ConvertAudio(&cvt);
        converted_data = cvt.buf;
        converted_size = cvt.len_cvt;
    } else {
        // No conversion needed
        converted_data = (Uint8*)SDL_malloc(size);
        memcpy(converted_data, data, size);
        converted_size = size;
    }

    // Create Mix_Chunk
    Mix_Chunk* chunk = Mix_QuickLoad_RAW(converted_data, converted_size);
    if (!chunk) {
        SDL_free(converted_data);
        Platform_SetError("Mix_QuickLoad_RAW failed: %s", Mix_GetError());
        return nullptr;
    }

    // Create sound data
    SoundData* sound = new SoundData();
    sound->chunk = chunk;
    sound->sample_rate = sample_rate;
    sound->channels = channels;
    sound->bits_per_sample = bits_per_sample;

    std::lock_guard<std::mutex> lock(g_audio_mutex);
    g_sounds.push_back(sound);

    return (PlatformSound*)sound;
}

void Platform_Sound_Destroy(PlatformSound* sound) {
    if (!sound) return;

    SoundData* data = (SoundData*)sound;

    std::lock_guard<std::mutex> lock(g_audio_mutex);

    // Remove from list
    auto it = std::find(g_sounds.begin(), g_sounds.end(), data);
    if (it != g_sounds.end()) {
        g_sounds.erase(it);
    }

    // Free chunk (this also frees the buffer)
    if (data->chunk) {
        Mix_FreeChunk(data->chunk);
    }
    delete data;
}
```

### 8.4 Sound Playback

```cpp
SoundHandle Platform_Sound_Play(PlatformSound* sound, float volume, float pan, bool loop) {
    if (!sound || !g_audio_initialized) return INVALID_SOUND_HANDLE;

    SoundData* data = (SoundData*)sound;

    // Find free channel
    int channel = Mix_PlayChannel(-1, data->chunk, loop ? -1 : 0);
    if (channel < 0) {
        // No free channels
        return INVALID_SOUND_HANDLE;
    }

    // Set volume (0-128)
    int vol = (int)(volume * g_master_volume * MIX_MAX_VOLUME);
    Mix_Volume(channel, vol);

    // Set panning
    // pan: -1.0 (left) to 1.0 (right)
    Uint8 left = (pan <= 0) ? 255 : (Uint8)(255 * (1.0f - pan));
    Uint8 right = (pan >= 0) ? 255 : (Uint8)(255 * (1.0f + pan));
    Mix_SetPanning(channel, left, right);

    return channel;
}

void Platform_Sound_Stop(SoundHandle handle) {
    if (handle < 0) return;
    Mix_HaltChannel(handle);
}

void Platform_Sound_StopAll(void) {
    Mix_HaltChannel(-1);
}

bool Platform_Sound_IsPlaying(SoundHandle handle) {
    if (handle < 0) return false;
    return Mix_Playing(handle) != 0;
}

void Platform_Sound_SetVolume(SoundHandle handle, float volume) {
    if (handle < 0) return;
    int vol = (int)(volume * g_master_volume * MIX_MAX_VOLUME);
    Mix_Volume(handle, vol);
}

void Platform_Sound_SetPan(SoundHandle handle, float pan) {
    if (handle < 0) return;
    Uint8 left = (pan <= 0) ? 255 : (Uint8)(255 * (1.0f - pan));
    Uint8 right = (pan >= 0) ? 255 : (Uint8)(255 * (1.0f + pan));
    Mix_SetPanning(handle, left, right);
}
```

### 8.5 ADPCM Decoder

The existing ADPCM decoder in `CODE/ADPCM.CPP` can be kept. We just need to call it before passing to our audio system:

File: `src/platform/adpcm.cpp`
```cpp
// IMA ADPCM decoder (adapted from CODE/ADPCM.CPP)

#include <cstdint>

static const int IMA_INDEX_TABLE[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

static const int IMA_STEP_TABLE[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

// Decode IMA ADPCM to 16-bit PCM
int ADPCM_Decode(const uint8_t* input, int input_size,
                 int16_t* output, int output_size,
                 int* predictor, int* step_index) {
    int out_samples = 0;
    int pred = *predictor;
    int index = *step_index;

    for (int i = 0; i < input_size && out_samples < output_size; i++) {
        uint8_t byte = input[i];

        // Decode low nibble
        int nibble = byte & 0x0F;
        int step = IMA_STEP_TABLE[index];
        int diff = step >> 3;
        if (nibble & 1) diff += step >> 2;
        if (nibble & 2) diff += step >> 1;
        if (nibble & 4) diff += step;
        if (nibble & 8) diff = -diff;

        pred += diff;
        if (pred > 32767) pred = 32767;
        if (pred < -32768) pred = -32768;

        output[out_samples++] = (int16_t)pred;
        index += IMA_INDEX_TABLE[nibble];
        if (index < 0) index = 0;
        if (index > 88) index = 88;

        if (out_samples >= output_size) break;

        // Decode high nibble
        nibble = (byte >> 4) & 0x0F;
        step = IMA_STEP_TABLE[index];
        diff = step >> 3;
        if (nibble & 1) diff += step >> 2;
        if (nibble & 2) diff += step >> 1;
        if (nibble & 4) diff += step;
        if (nibble & 8) diff = -diff;

        pred += diff;
        if (pred > 32767) pred = 32767;
        if (pred < -32768) pred = -32768;

        output[out_samples++] = (int16_t)pred;
        index += IMA_INDEX_TABLE[nibble];
        if (index < 0) index = 0;
        if (index > 88) index = 88;
    }

    *predictor = pred;
    *step_index = index;
    return out_samples;
}
```

### 8.6 Streaming Audio

```cpp
static void Stream_Callback(void* userdata, Uint8* stream, int len) {
    StreamData* data = (StreamData*)userdata;

    if (!data->playing || !data->callback) {
        memset(stream, 0, len);
        return;
    }

    int filled = data->callback(data->userdata, stream, len);

    // Apply volume
    if (data->volume < 1.0f) {
        if (data->bits_per_sample == 16) {
            int16_t* samples = (int16_t*)stream;
            int count = filled / 2;
            for (int i = 0; i < count; i++) {
                samples[i] = (int16_t)(samples[i] * data->volume);
            }
        } else {
            for (int i = 0; i < filled; i++) {
                int sample = stream[i] - 128;
                sample = (int)(sample * data->volume);
                stream[i] = (Uint8)(sample + 128);
            }
        }
    }

    // Clear unfilled portion
    if (filled < len) {
        memset(stream + filled, 0, len - filled);
    }
}

PlatformStream* Platform_Stream_Create(int sample_rate, int channels, int bits_per_sample,
                                       AudioStreamCallback callback, void* userdata) {
    SDL_AudioSpec desired, obtained;
    desired.freq = sample_rate;
    desired.format = (bits_per_sample == 8) ? AUDIO_U8 : AUDIO_S16SYS;
    desired.channels = channels;
    desired.samples = 4096;
    desired.callback = Stream_Callback;

    StreamData* stream = new StreamData();
    stream->callback = callback;
    stream->userdata = userdata;
    stream->sample_rate = sample_rate;
    stream->channels = channels;
    stream->bits_per_sample = bits_per_sample;
    stream->volume = 1.0f;
    stream->playing = false;

    desired.userdata = stream;

    stream->device = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (stream->device == 0) {
        delete stream;
        Platform_SetError("SDL_OpenAudioDevice failed: %s", SDL_GetError());
        return nullptr;
    }

    g_streams.push_back(stream);
    return (PlatformStream*)stream;
}

void Platform_Stream_Play(PlatformStream* stream) {
    if (!stream) return;
    StreamData* data = (StreamData*)stream;
    data->playing = true;
    SDL_PauseAudioDevice(data->device, 0);
}

void Platform_Stream_Pause(PlatformStream* stream) {
    if (!stream) return;
    StreamData* data = (StreamData*)stream;
    SDL_PauseAudioDevice(data->device, 1);
}

void Platform_Stream_Stop(PlatformStream* stream) {
    if (!stream) return;
    StreamData* data = (StreamData*)stream;
    data->playing = false;
    SDL_PauseAudioDevice(data->device, 1);
}

void Platform_Stream_SetVolume(PlatformStream* stream, float volume) {
    if (!stream) return;
    StreamData* data = (StreamData*)stream;
    data->volume = (volume < 0) ? 0 : (volume > 1) ? 1 : volume;
}

void Platform_Stream_Destroy(PlatformStream* stream) {
    if (!stream) return;
    StreamData* data = (StreamData*)stream;

    SDL_CloseAudioDevice(data->device);

    auto it = std::find(g_streams.begin(), g_streams.end(), data);
    if (it != g_streams.end()) {
        g_streams.erase(it);
    }

    delete data;
}
```

## Tasks Breakdown

### Phase 1: Basic Audio (2 days)
- [ ] Initialize SDL2_mixer
- [ ] Implement sound loading from memory
- [ ] Implement basic playback
- [ ] Test with simple WAV data

### Phase 2: Volume/Panning (1 day)
- [ ] Implement volume control
- [ ] Implement stereo panning
- [ ] Test spatial audio

### Phase 3: ADPCM Support (1 day)
- [ ] Port ADPCM decoder
- [ ] Integrate with sound loading
- [ ] Test with AUD files

### Phase 4: Streaming (1.5 days)
- [ ] Implement streaming callback
- [ ] Implement stream playback
- [ ] Test with music files

### Phase 5: Integration (1.5 days)
- [ ] Create DirectSound wrappers
- [ ] Hook up to game audio code
- [ ] Test all game sounds

## Acceptance Criteria

- [ ] Sound effects play correctly
- [ ] Volume control works
- [ ] Panning works
- [ ] Music streams without skipping
- [ ] ADPCM audio decodes correctly
- [ ] No audio glitches or pops

## Estimated Duration
**6-8 days**

## Dependencies
- Plan 07 (File System) for loading audio files
- SDL2_mixer library

## Next Plan
Once audio is working, proceed to **Plan 09: Assembly Rewrites**
