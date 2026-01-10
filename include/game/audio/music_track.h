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
