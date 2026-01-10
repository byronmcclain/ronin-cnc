// src/game/audio/audio_events.cpp
// Game Audio Event Handlers
// Task 17f - Game Audio Events

#include "game/audio/audio_events.h"
#include "game/audio/event_rate_limiter.h"
#include "game/audio/audio_system.h"
#include "game/audio/sound_effect.h"
#include "game/audio/voice_event.h"
#include "game/audio/music_track.h"
#include "platform.h"
#include <cstdio>

//=============================================================================
// Statistics Tracking
//=============================================================================

static uint32_t s_total_triggered = 0;
static uint32_t s_total_rate_limited = 0;
static bool s_initialized = false;

//=============================================================================
// Helper Macros
//=============================================================================

#define CHECK_INITIALIZED() if (!s_initialized) return

#define RATE_LIMITED_EVENT(event_type) \
    s_total_triggered++; \
    if (!GetEventRateLimiter().CanFireGlobal(static_cast<int>(event_type))) { \
        s_total_rate_limited++; \
        return; \
    }

#define RATE_LIMITED_POSITION_EVENT(event_type, cx, cy) \
    s_total_triggered++; \
    if (!GetEventRateLimiter().CanFireGlobalAndPosition(static_cast<int>(event_type), cx, cy)) { \
        s_total_rate_limited++; \
        return; \
    }

#define RATE_LIMITED_OBJECT_EVENT(event_type, obj_id) \
    s_total_triggered++; \
    if (!GetEventRateLimiter().CanFireGlobalAndObject(static_cast<int>(event_type), obj_id)) { \
        s_total_rate_limited++; \
        return; \
    }

//=============================================================================
// Coordinate Conversion Helpers
//=============================================================================

// Convert world coordinates to cell coordinates
static inline int WorldToCell(int world_coord) {
    return world_coord / 24;  // 24 pixels per cell (ICON_PIXEL_W)
}

// Get object ID for rate limiting (use pointer as ID for now)
static inline ObjectKey GetObjectId(void* obj) {
    return static_cast<ObjectKey>(reinterpret_cast<uintptr_t>(obj));
}

// Check if house is the player's house
static inline bool IsPlayerHouse(HouseClass* house) {
    // In actual implementation, compare against PlayerPtr
    // For now, assume all houses are player for EVA events
    return house != nullptr;
}

//=============================================================================
// Initialization
//=============================================================================

void AudioEvents_Init() {
    if (s_initialized) {
        return;
    }

    Platform_LogInfo("AudioEvents: Initializing rate limiters...");

    EventRateLimiter& limiter = GetEventRateLimiter();

    // Combat events - position-based limiting to prevent explosion spam
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::DAMAGE_SMALL), 50);
    limiter.SetPositionCooldown(static_cast<int>(AudioEventType::DAMAGE_SMALL), 150);

    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::DAMAGE_LARGE), 100);
    limiter.SetPositionCooldown(static_cast<int>(AudioEventType::DAMAGE_LARGE), 200);

    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::EXPLOSION_SMALL), 50);
    limiter.SetPositionCooldown(static_cast<int>(AudioEventType::EXPLOSION_SMALL), 150);

    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::EXPLOSION_LARGE), 100);
    limiter.SetPositionCooldown(static_cast<int>(AudioEventType::EXPLOSION_LARGE), 300);

    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::WEAPON_FIRE), 30);
    limiter.SetPositionCooldown(static_cast<int>(AudioEventType::WEAPON_FIRE), 100);

    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::PROJECTILE_IMPACT), 50);
    limiter.SetPositionCooldown(static_cast<int>(AudioEventType::PROJECTILE_IMPACT), 100);

    // Unit events - object-based limiting
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::UNIT_SELECT), 100);
    limiter.SetObjectCooldown(static_cast<int>(AudioEventType::UNIT_SELECT), 500);

    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::UNIT_MOVE_ORDER), 150);
    limiter.SetObjectCooldown(static_cast<int>(AudioEventType::UNIT_MOVE_ORDER), 400);

    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::UNIT_ATTACK_ORDER), 150);
    limiter.SetObjectCooldown(static_cast<int>(AudioEventType::UNIT_ATTACK_ORDER), 400);

    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::UNIT_ACKNOWLEDGE), 200);
    limiter.SetObjectCooldown(static_cast<int>(AudioEventType::UNIT_ACKNOWLEDGE), 500);

    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::UNIT_DEATH), 100);
    limiter.SetPositionCooldown(static_cast<int>(AudioEventType::UNIT_DEATH), 200);

    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::UNIT_CREATED), 500);

    // Building events
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::BUILDING_PLACED), 200);
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::BUILDING_COMPLETE), 1000);
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::BUILDING_SOLD), 500);
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::BUILDING_DESTROYED), 300);
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::BUILDING_CAPTURED), 500);

    // EVA events - long cooldowns to prevent annoyance
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::EVA_BASE_ATTACK), 30000);      // 30 seconds
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::EVA_UNIT_LOST), 5000);         // 5 seconds
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::EVA_BUILDING_LOST), 5000);     // 5 seconds
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::EVA_LOW_POWER), 10000);        // 10 seconds
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::EVA_INSUFFICIENT_FUNDS), 5000); // 5 seconds
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::EVA_SILOS_NEEDED), 15000);     // 15 seconds
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::EVA_RADAR_ONLINE), 2000);      // 2 seconds
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::EVA_RADAR_OFFLINE), 2000);     // 2 seconds
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::EVA_UNIT_READY), 1500);        // 1.5 seconds
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::EVA_CONSTRUCTION_COMPLETE), 1500);

    // UI events - short cooldowns
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::UI_CLICK), 50);
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::UI_BUILD_CLICK), 100);
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::UI_TAB_CLICK), 100);
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::UI_SIDEBAR_UP), 100);
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::UI_SIDEBAR_DOWN), 100);
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::UI_CANNOT), 200);
    limiter.SetGlobalCooldown(static_cast<int>(AudioEventType::UI_PRIMARY_SET), 200);

    // Ambient - per-object
    limiter.SetObjectCooldown(static_cast<int>(AudioEventType::AMBIENT_MOVEMENT), 2000);

    s_total_triggered = 0;
    s_total_rate_limited = 0;
    s_initialized = true;

    Platform_LogInfo("AudioEvents: Initialized with rate limiters");
}

void AudioEvents_Shutdown() {
    if (!s_initialized) {
        return;
    }

    GetEventRateLimiter().Reset();
    s_initialized = false;

    printf("AudioEvents: Shutdown (triggered: %u, rate-limited: %u)\n",
           s_total_triggered, s_total_rate_limited);
}

//=============================================================================
// Combat Event Handlers
//=============================================================================

void AudioEvent_Damage(ObjectClass* obj, int damage, int source_x, int source_y) {
    CHECK_INITIALIZED();

    if (!obj) return;

    int cx = WorldToCell(source_x);
    int cy = WorldToCell(source_y);

    if (damage > 50) {
        RATE_LIMITED_POSITION_EVENT(AudioEventType::DAMAGE_LARGE, cx, cy);
        AudioSystem::Instance().PlaySoundAt(SoundEffect::EXPLODE_LARGE, source_x, source_y);
    } else if (damage > 10) {
        RATE_LIMITED_POSITION_EVENT(AudioEventType::DAMAGE_SMALL, cx, cy);
        AudioSystem::Instance().PlaySoundAt(SoundEffect::EXPLODE_SMALL, source_x, source_y);
    }
    // Very small damage doesn't play sound
}

void AudioEvent_Explosion(int world_x, int world_y, int size) {
    CHECK_INITIALIZED();

    int cx = WorldToCell(world_x);
    int cy = WorldToCell(world_y);

    if (size >= 2) {
        RATE_LIMITED_POSITION_EVENT(AudioEventType::EXPLOSION_LARGE, cx, cy);
        AudioSystem::Instance().PlaySoundAt(SoundEffect::EXPLODE_LARGE, world_x, world_y);
    } else if (size >= 1) {
        RATE_LIMITED_POSITION_EVENT(AudioEventType::EXPLOSION_SMALL, cx, cy);
        AudioSystem::Instance().PlaySoundAt(SoundEffect::EXPLODE_MEDIUM, world_x, world_y);
    } else {
        RATE_LIMITED_POSITION_EVENT(AudioEventType::EXPLOSION_SMALL, cx, cy);
        AudioSystem::Instance().PlaySoundAt(SoundEffect::EXPLODE_SMALL, world_x, world_y);
    }
}

void AudioEvent_WeaponFire(WeaponTypeClass* weapon, int world_x, int world_y) {
    CHECK_INITIALIZED();

    int cx = WorldToCell(world_x);
    int cy = WorldToCell(world_y);

    RATE_LIMITED_POSITION_EVENT(AudioEventType::WEAPON_FIRE, cx, cy);

    // In real implementation, get weapon sound from WeaponTypeClass
    // For now, play generic weapon sound
    AudioSystem::Instance().PlaySoundAt(SoundEffect::WEAPON_MGUN, world_x, world_y);
}

void AudioEvent_ProjectileImpact(BulletClass* bullet, int world_x, int world_y) {
    CHECK_INITIALIZED();

    int cx = WorldToCell(world_x);
    int cy = WorldToCell(world_y);

    RATE_LIMITED_POSITION_EVENT(AudioEventType::PROJECTILE_IMPACT, cx, cy);

    // In real implementation, get impact sound from bullet type
    AudioSystem::Instance().PlaySoundAt(SoundEffect::IMPACT_SHELL, world_x, world_y);
}

//=============================================================================
// Unit Event Handlers
//=============================================================================

void AudioEvent_UnitSelected(ObjectClass* obj) {
    CHECK_INITIALIZED();

    if (!obj) return;

    ObjectKey id = GetObjectId(obj);
    RATE_LIMITED_OBJECT_EVENT(AudioEventType::UNIT_SELECT, id);

    // Play unit acknowledgment voice
    // In real implementation, determine unit type and play appropriate voice
    AudioSystem::Instance().PlayUnit(UnitVoice::REPORTING);
}

void AudioEvent_UnitMoveOrder(ObjectClass* obj) {
    CHECK_INITIALIZED();

    if (!obj) return;

    ObjectKey id = GetObjectId(obj);
    RATE_LIMITED_OBJECT_EVENT(AudioEventType::UNIT_MOVE_ORDER, id);

    // Play acknowledgment
    AudioSystem::Instance().PlayUnit(UnitVoice::MOVING_OUT);
}

void AudioEvent_UnitAttackOrder(ObjectClass* obj) {
    CHECK_INITIALIZED();

    if (!obj) return;

    ObjectKey id = GetObjectId(obj);
    RATE_LIMITED_OBJECT_EVENT(AudioEventType::UNIT_ATTACK_ORDER, id);

    // Play attack acknowledgment
    AudioSystem::Instance().PlayUnit(UnitVoice::ATTACKING);
}

void AudioEvent_UnitDeath(ObjectClass* obj) {
    CHECK_INITIALIZED();

    if (!obj) return;

    // Use position-based limiting for death sounds
    // In real implementation: int wx = Coord_X(obj->Coord);
    // For now, use 0,0 as placeholder
    int wx = 0;
    int wy = 0;
    int cx = WorldToCell(wx);
    int cy = WorldToCell(wy);

    RATE_LIMITED_POSITION_EVENT(AudioEventType::UNIT_DEATH, cx, cy);

    // Play death sound - in real implementation, based on unit type
    AudioSystem::Instance().PlaySoundAt(SoundEffect::EXPLODE_SMALL, wx, wy);
}

void AudioEvent_UnitCreated(ObjectClass* obj, HouseClass* house) {
    CHECK_INITIALIZED();

    if (!IsPlayerHouse(house)) return;

    RATE_LIMITED_EVENT(AudioEventType::UNIT_CREATED);

    // EVA announcement handled separately by AudioEvent_UnitReady
}

//=============================================================================
// Building Event Handlers
//=============================================================================

void AudioEvent_BuildingPlaced(BuildingClass* building) {
    CHECK_INITIALIZED();

    RATE_LIMITED_EVENT(AudioEventType::BUILDING_PLACED);

    AudioSystem::Instance().PlaySound(SoundEffect::BUILD_PLACE);
}

void AudioEvent_BuildingComplete(BuildingClass* building, HouseClass* house) {
    CHECK_INITIALIZED();

    RATE_LIMITED_EVENT(AudioEventType::BUILDING_COMPLETE);

    AudioSystem::Instance().PlaySound(SoundEffect::BUILD_CLOCK);

    // EVA announcement for player
    if (IsPlayerHouse(house)) {
        // Queue instead of play to prevent overlap
        AudioSystem::Instance().QueueEva(EvaVoice::CONSTRUCTION_COMPLETE);
    }
}

void AudioEvent_BuildingSold(BuildingClass* building) {
    CHECK_INITIALIZED();

    RATE_LIMITED_EVENT(AudioEventType::BUILDING_SOLD);

    AudioSystem::Instance().PlaySound(SoundEffect::BUILD_SOLD);
}

void AudioEvent_BuildingDestroyed(BuildingClass* building, HouseClass* house) {
    CHECK_INITIALIZED();

    RATE_LIMITED_EVENT(AudioEventType::BUILDING_DESTROYED);

    // Play large explosion for building destruction
    AudioSystem::Instance().PlaySound(SoundEffect::EXPLODE_HUGE);

    // EVA building lost
    if (IsPlayerHouse(house)) {
        AudioEvent_BuildingLost(house);
    }
}

void AudioEvent_BuildingCaptured(BuildingClass* building, HouseClass* old_owner, HouseClass* new_owner) {
    CHECK_INITIALIZED();

    RATE_LIMITED_EVENT(AudioEventType::BUILDING_CAPTURED);

    AudioSystem::Instance().PlaySound(SoundEffect::UI_TARGET);
}

//=============================================================================
// EVA Announcement Handlers
//=============================================================================

void AudioEvent_BaseUnderAttack(HouseClass* house) {
    CHECK_INITIALIZED();

    if (!IsPlayerHouse(house)) return;

    RATE_LIMITED_EVENT(AudioEventType::EVA_BASE_ATTACK);

    AudioSystem::Instance().PlayEva(EvaVoice::BASE_UNDER_ATTACK);
}

void AudioEvent_UnitLost(HouseClass* house) {
    CHECK_INITIALIZED();

    if (!IsPlayerHouse(house)) return;

    RATE_LIMITED_EVENT(AudioEventType::EVA_UNIT_LOST);

    AudioSystem::Instance().QueueEva(EvaVoice::UNIT_LOST);
}

void AudioEvent_BuildingLost(HouseClass* house) {
    CHECK_INITIALIZED();

    if (!IsPlayerHouse(house)) return;

    RATE_LIMITED_EVENT(AudioEventType::EVA_BUILDING_LOST);

    AudioSystem::Instance().QueueEva(EvaVoice::BUILDING_LOST);
}

void AudioEvent_LowPower(HouseClass* house) {
    CHECK_INITIALIZED();

    if (!IsPlayerHouse(house)) return;

    RATE_LIMITED_EVENT(AudioEventType::EVA_LOW_POWER);

    AudioSystem::Instance().PlayEva(EvaVoice::LOW_POWER);
}

void AudioEvent_InsufficientFunds(HouseClass* house) {
    CHECK_INITIALIZED();

    if (!IsPlayerHouse(house)) return;

    RATE_LIMITED_EVENT(AudioEventType::EVA_INSUFFICIENT_FUNDS);

    AudioSystem::Instance().PlayEva(EvaVoice::INSUFFICIENT_FUNDS);
}

void AudioEvent_SilosNeeded(HouseClass* house) {
    CHECK_INITIALIZED();

    if (!IsPlayerHouse(house)) return;

    RATE_LIMITED_EVENT(AudioEventType::EVA_SILOS_NEEDED);

    AudioSystem::Instance().PlayEva(EvaVoice::SILOS_NEEDED);
}

void AudioEvent_RadarOnline(HouseClass* house) {
    CHECK_INITIALIZED();

    if (!IsPlayerHouse(house)) return;

    RATE_LIMITED_EVENT(AudioEventType::EVA_RADAR_ONLINE);

    AudioSystem::Instance().PlayEva(EvaVoice::RADAR_ONLINE);
}

void AudioEvent_RadarOffline(HouseClass* house) {
    CHECK_INITIALIZED();

    if (!IsPlayerHouse(house)) return;

    RATE_LIMITED_EVENT(AudioEventType::EVA_RADAR_OFFLINE);

    AudioSystem::Instance().PlayEva(EvaVoice::RADAR_OFFLINE);
}

void AudioEvent_UnitReady(HouseClass* house) {
    CHECK_INITIALIZED();

    if (!IsPlayerHouse(house)) return;

    RATE_LIMITED_EVENT(AudioEventType::EVA_UNIT_READY);

    AudioSystem::Instance().QueueEva(EvaVoice::UNIT_READY);
}

void AudioEvent_ConstructionComplete(HouseClass* house) {
    CHECK_INITIALIZED();

    if (!IsPlayerHouse(house)) return;

    RATE_LIMITED_EVENT(AudioEventType::EVA_CONSTRUCTION_COMPLETE);

    AudioSystem::Instance().PlayEva(EvaVoice::CONSTRUCTION_COMPLETE);
}

void AudioEvent_MissionAccomplished() {
    CHECK_INITIALIZED();

    // No rate limiting for mission events
    s_total_triggered++;
    AudioSystem::Instance().PlayEva(EvaVoice::MISSION_ACCOMPLISHED);
}

void AudioEvent_MissionFailed() {
    CHECK_INITIALIZED();

    s_total_triggered++;
    AudioSystem::Instance().PlayEva(EvaVoice::MISSION_FAILED);
}

//=============================================================================
// UI Event Handlers
//=============================================================================

void AudioEvent_UIClick() {
    CHECK_INITIALIZED();

    RATE_LIMITED_EVENT(AudioEventType::UI_CLICK);

    AudioSystem::Instance().PlaySound(SoundEffect::UI_CLICK);
}

void AudioEvent_UIBuildClick() {
    CHECK_INITIALIZED();

    RATE_LIMITED_EVENT(AudioEventType::UI_BUILD_CLICK);

    AudioSystem::Instance().PlaySound(SoundEffect::UI_CLICK);
}

void AudioEvent_UITabClick() {
    CHECK_INITIALIZED();

    RATE_LIMITED_EVENT(AudioEventType::UI_TAB_CLICK);

    AudioSystem::Instance().PlaySound(SoundEffect::UI_BEEP);
}

void AudioEvent_UISidebarUp() {
    CHECK_INITIALIZED();

    RATE_LIMITED_EVENT(AudioEventType::UI_SIDEBAR_UP);

    AudioSystem::Instance().PlaySound(SoundEffect::UI_BEEP);
}

void AudioEvent_UISidebarDown() {
    CHECK_INITIALIZED();

    RATE_LIMITED_EVENT(AudioEventType::UI_SIDEBAR_DOWN);

    AudioSystem::Instance().PlaySound(SoundEffect::UI_BEEP);
}

void AudioEvent_UICannot() {
    CHECK_INITIALIZED();

    RATE_LIMITED_EVENT(AudioEventType::UI_CANNOT);

    AudioSystem::Instance().PlaySound(SoundEffect::UI_ERROR);
}

void AudioEvent_UIPrimarySet() {
    CHECK_INITIALIZED();

    RATE_LIMITED_EVENT(AudioEventType::UI_PRIMARY_SET);

    AudioSystem::Instance().PlaySound(SoundEffect::BUILD_PRIMARY);
}

//=============================================================================
// Game State Audio
//=============================================================================

void AudioEvent_EnterMainMenu() {
    CHECK_INITIALIZED();

    s_total_triggered++;
    AudioSystem::Instance().PlayMusic(MusicTrack::MENU);
}

void AudioEvent_EnterBriefing(int mission_number, bool is_allied) {
    CHECK_INITIALIZED();

    s_total_triggered++;

    // Play briefing/score music
    AudioSystem::Instance().PlayMusic(MusicTrack::SCORE_SCREEN);
}

void AudioEvent_MissionStart() {
    CHECK_INITIALIZED();

    s_total_triggered++;

    // Play random in-game track
    AudioSystem::Instance().PlayMusic(MusicTrack::BIGFOOT);
    AudioSystem::Instance().SetMusicShuffle(true);
}

void AudioEvent_CombatStart() {
    CHECK_INITIALIZED();

    s_total_triggered++;

    // Could switch to more intense combat music
    // For now, music continues unchanged
}

void AudioEvent_CombatEnd() {
    CHECK_INITIALIZED();

    s_total_triggered++;

    // Could switch to calmer exploration music
}

void AudioEvent_Victory() {
    CHECK_INITIALIZED();

    s_total_triggered++;

    AudioSystem::Instance().StopMusic();
    AudioSystem::Instance().PlayEva(EvaVoice::MISSION_ACCOMPLISHED);

    // Could play victory theme
    AudioSystem::Instance().PlayMusic(MusicTrack::SCORE_SCREEN);
}

void AudioEvent_Defeat() {
    CHECK_INITIALIZED();

    s_total_triggered++;

    AudioSystem::Instance().StopMusic();
    AudioSystem::Instance().PlayEva(EvaVoice::MISSION_FAILED);
}

//=============================================================================
// Movement/Ambient Sounds
//=============================================================================

void AudioEvent_UnitMovement(ObjectClass* obj, bool is_moving) {
    CHECK_INITIALIZED();

    if (!obj) return;

    ObjectKey id = GetObjectId(obj);

    if (is_moving) {
        // Only rate limit starting movement sound
        if (!GetEventRateLimiter().CanFireForObject(static_cast<int>(AudioEventType::AMBIENT_MOVEMENT), id)) {
            return;
        }
        s_total_triggered++;

        // In real implementation, determine unit type for movement sound
        AudioSystem::Instance().PlaySound(SoundEffect::MOVE_VEHICLE);
    } else {
        // Stop movement sound for this unit
        // Would need to track sound handle per unit
    }
}

//=============================================================================
// Debug/Stats
//=============================================================================

void AudioEvents_PrintStats() {
    printf("=== Audio Events Statistics ===\n");
    printf("  Initialized: %s\n", s_initialized ? "yes" : "no");
    printf("  Total Triggered: %u\n", s_total_triggered);
    printf("  Total Rate-Limited: %u\n", s_total_rate_limited);

    if (s_total_triggered > 0) {
        float rate = (float)s_total_rate_limited / (float)s_total_triggered * 100.0f;
        printf("  Rate-Limit Ratio: %.1f%%\n", rate);
    }

    printf("  Tracked Entries: %zu\n", GetEventRateLimiter().GetTrackedCount());
}

uint32_t AudioEvents_GetTotalTriggered() {
    return s_total_triggered;
}

uint32_t AudioEvents_GetTotalRateLimited() {
    return s_total_rate_limited;
}
