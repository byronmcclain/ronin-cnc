// include/game/audio/audio_events.h
// Game Audio Event Handlers
// Task 17f - Game Audio Events

#ifndef AUDIO_EVENTS_H
#define AUDIO_EVENTS_H

#include <cstdint>

//=============================================================================
// Forward Declarations (Game Types)
//=============================================================================

// These would normally be in game headers
// For now, use forward declarations as placeholders

struct ObjectClass;
struct BuildingClass;
struct UnitClass;
struct InfantryClass;
struct AircraftClass;
struct VesselClass;
struct HouseClass;
struct WeaponTypeClass;
struct BulletClass;

//=============================================================================
// Audio Event Types (for rate limiting)
//=============================================================================

enum class AudioEventType : int {
    // Combat events
    DAMAGE_SMALL = 0,
    DAMAGE_LARGE,
    EXPLOSION_SMALL,
    EXPLOSION_LARGE,
    WEAPON_FIRE,
    PROJECTILE_IMPACT,

    // Unit events
    UNIT_SELECT,
    UNIT_MOVE_ORDER,
    UNIT_ATTACK_ORDER,
    UNIT_ACKNOWLEDGE,
    UNIT_DEATH,
    UNIT_CREATED,

    // Building events
    BUILDING_PLACED,
    BUILDING_COMPLETE,
    BUILDING_SOLD,
    BUILDING_DESTROYED,
    BUILDING_CAPTURED,

    // EVA events
    EVA_BASE_ATTACK,
    EVA_UNIT_LOST,
    EVA_BUILDING_LOST,
    EVA_LOW_POWER,
    EVA_INSUFFICIENT_FUNDS,
    EVA_SILOS_NEEDED,
    EVA_RADAR_ONLINE,
    EVA_RADAR_OFFLINE,
    EVA_UNIT_READY,
    EVA_CONSTRUCTION_COMPLETE,
    EVA_MISSION_COMPLETE,
    EVA_MISSION_FAILED,

    // UI events
    UI_CLICK,
    UI_BUILD_CLICK,
    UI_TAB_CLICK,
    UI_SIDEBAR_UP,
    UI_SIDEBAR_DOWN,
    UI_CANNOT,
    UI_PRIMARY_SET,

    // Ambient events
    AMBIENT_MOVEMENT,

    EVENT_TYPE_COUNT
};

//=============================================================================
// Audio Events Configuration
//=============================================================================

/// Initialize audio events system
/// Sets up rate limiters with default cooldowns
void AudioEvents_Init();

/// Shutdown audio events system
void AudioEvents_Shutdown();

//=============================================================================
// Combat Event Handlers
//=============================================================================

/// Called when an object takes damage
/// @param obj The damaged object
/// @param damage Amount of damage taken
/// @param source_x World X of damage source (for directional audio)
/// @param source_y World Y of damage source
void AudioEvent_Damage(ObjectClass* obj, int damage, int source_x, int source_y);

/// Called when an explosion occurs
/// @param world_x World X coordinate
/// @param world_y World Y coordinate
/// @param size Explosion size (0 = small, 1 = medium, 2 = large)
void AudioEvent_Explosion(int world_x, int world_y, int size);

/// Called when a weapon fires
/// @param weapon The weapon type being fired
/// @param world_x World X coordinate
/// @param world_y World Y coordinate
void AudioEvent_WeaponFire(WeaponTypeClass* weapon, int world_x, int world_y);

/// Called when a projectile impacts
/// @param bullet The impacting projectile
/// @param world_x Impact world X
/// @param world_y Impact world Y
void AudioEvent_ProjectileImpact(BulletClass* bullet, int world_x, int world_y);

//=============================================================================
// Unit Event Handlers
//=============================================================================

/// Called when a unit is selected by the player
/// @param obj The selected object
void AudioEvent_UnitSelected(ObjectClass* obj);

/// Called when a unit is given a move order
/// @param obj The ordered unit
void AudioEvent_UnitMoveOrder(ObjectClass* obj);

/// Called when a unit is given an attack order
/// @param obj The ordered unit
void AudioEvent_UnitAttackOrder(ObjectClass* obj);

/// Called when a unit dies
/// @param obj The dying unit
void AudioEvent_UnitDeath(ObjectClass* obj);

/// Called when a new unit is created
/// @param obj The new unit
/// @param house The owning house
void AudioEvent_UnitCreated(ObjectClass* obj, HouseClass* house);

//=============================================================================
// Building Event Handlers
//=============================================================================

/// Called when a building placement begins
/// @param building The building being placed
void AudioEvent_BuildingPlaced(BuildingClass* building);

/// Called when building construction completes
/// @param building The completed building
/// @param house The owning house
void AudioEvent_BuildingComplete(BuildingClass* building, HouseClass* house);

/// Called when a building is sold
/// @param building The sold building
void AudioEvent_BuildingSold(BuildingClass* building);

/// Called when a building is destroyed
/// @param building The destroyed building
/// @param house The owning house
void AudioEvent_BuildingDestroyed(BuildingClass* building, HouseClass* house);

/// Called when a building is captured
/// @param building The captured building
/// @param old_owner Previous owner
/// @param new_owner New owner
void AudioEvent_BuildingCaptured(BuildingClass* building, HouseClass* old_owner, HouseClass* new_owner);

//=============================================================================
// EVA Announcement Handlers
//=============================================================================

/// Called when player's base is under attack
/// @param house The attacked house
void AudioEvent_BaseUnderAttack(HouseClass* house);

/// Called when player loses a unit
/// @param house The owning house
void AudioEvent_UnitLost(HouseClass* house);

/// Called when player loses a building
/// @param house The owning house
void AudioEvent_BuildingLost(HouseClass* house);

/// Called when power drops to low level
/// @param house The affected house
void AudioEvent_LowPower(HouseClass* house);

/// Called when player has insufficient funds for an action
/// @param house The affected house
void AudioEvent_InsufficientFunds(HouseClass* house);

/// Called when ore silos are needed
/// @param house The affected house
void AudioEvent_SilosNeeded(HouseClass* house);

/// Called when radar comes online
/// @param house The affected house
void AudioEvent_RadarOnline(HouseClass* house);

/// Called when radar goes offline
/// @param house The affected house
void AudioEvent_RadarOffline(HouseClass* house);

/// Called when unit training completes
/// @param house The owning house
void AudioEvent_UnitReady(HouseClass* house);

/// Called when building construction completes (EVA)
/// @param house The owning house
void AudioEvent_ConstructionComplete(HouseClass* house);

/// Called when mission is accomplished
void AudioEvent_MissionAccomplished();

/// Called when mission fails
void AudioEvent_MissionFailed();

//=============================================================================
// UI Event Handlers
//=============================================================================

/// Called when UI element is clicked
void AudioEvent_UIClick();

/// Called when build button is clicked
void AudioEvent_UIBuildClick();

/// Called when sidebar tab is clicked
void AudioEvent_UITabClick();

/// Called when sidebar scrolls up
void AudioEvent_UISidebarUp();

/// Called when sidebar scrolls down
void AudioEvent_UISidebarDown();

/// Called when action is not allowed
void AudioEvent_UICannot();

/// Called when primary building is set
void AudioEvent_UIPrimarySet();

//=============================================================================
// Game State Audio
//=============================================================================

/// Called when entering main menu
void AudioEvent_EnterMainMenu();

/// Called when entering mission briefing
/// @param mission_number Mission number (1-14 typically)
/// @param is_allied true for Allied campaign, false for Soviet
void AudioEvent_EnterBriefing(int mission_number, bool is_allied);

/// Called when starting a mission
void AudioEvent_MissionStart();

/// Called when combat begins (enemies spotted)
void AudioEvent_CombatStart();

/// Called when combat ends (no enemies in range for a while)
void AudioEvent_CombatEnd();

/// Called when game is won
void AudioEvent_Victory();

/// Called when game is lost
void AudioEvent_Defeat();

//=============================================================================
// Movement/Ambient Sounds
//=============================================================================

/// Called when a unit moves (for movement sound loop)
/// @param obj The moving unit
/// @param is_moving true if unit is currently moving
void AudioEvent_UnitMovement(ObjectClass* obj, bool is_moving);

//=============================================================================
// Debug/Stats
//=============================================================================

/// Print audio event statistics
void AudioEvents_PrintStats();

/// Get total events triggered this session
uint32_t AudioEvents_GetTotalTriggered();

/// Get total events rate-limited this session
uint32_t AudioEvents_GetTotalRateLimited();

#endif // AUDIO_EVENTS_H
