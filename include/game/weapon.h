/**
 * Red Alert Weapon and Armor Definitions
 *
 * Defines the combat system types used throughout the game.
 */

#pragma once

#include <cstdint>

// =============================================================================
// Armor Types
// =============================================================================

/**
 * ArmorType - Damage resistance categories
 *
 * Different weapons have different effectiveness against armor types.
 * This creates rock-paper-scissors style combat balance.
 */
enum ArmorType : int8_t {
    ARMOR_NONE = 0,        // No armor (infantry, aircraft)
    ARMOR_WOOD = 1,        // Light structures
    ARMOR_LIGHT = 2,       // Light vehicles (jeeps, APCs)
    ARMOR_HEAVY = 3,       // Heavy tanks
    ARMOR_CONCRETE = 4,    // Buildings

    ARMOR_COUNT = 5
};

// =============================================================================
// Weapon Types
// =============================================================================

/**
 * WeaponType - All weapon systems in the game
 *
 * Each weapon has:
 * - Damage amount
 * - Range
 * - Rate of fire
 * - Projectile type
 * - Warhead type (determines armor effectiveness)
 */
enum WeaponType : int8_t {
    WEAPON_NONE = -1,

    // Infantry weapons
    WEAPON_COLT45 = 0,         // Pistol
    WEAPON_ZSU23 = 1,          // Anti-aircraft gun
    WEAPON_VULCAN = 2,         // Chaingun
    WEAPON_MAVERICK = 3,       // Missile
    WEAPON_CAMERA = 4,         // Spy camera (no damage)
    WEAPON_FIREBALL = 5,       // Flamethrower
    WEAPON_SNIPER = 6,         // Sniper rifle
    WEAPON_CHAINGUN = 7,       // Machine gun
    WEAPON_PISTOL = 8,         // Light pistol
    WEAPON_M1_CARBINE = 9,     // Rifle
    WEAPON_DRAGON = 10,        // Anti-tank missile
    WEAPON_HELLFIRE = 11,      // Heavy missile
    WEAPON_GRENADE = 12,       // Hand grenade
    WEAPON_75MM = 13,          // Light cannon
    WEAPON_90MM = 14,          // Medium cannon
    WEAPON_105MM = 15,         // Heavy cannon
    WEAPON_120MM = 16,         // Main battle tank gun
    WEAPON_TURRET_GUN = 17,    // Defense turret
    WEAPON_MAMMOTH_TUSK = 18,  // Mammoth tank missiles
    WEAPON_155MM = 19,         // Artillery
    WEAPON_M60MG = 20,         // Machine gun
    WEAPON_NAPALM = 21,        // Air-dropped napalm
    WEAPON_TESLA_ZAP = 22,     // Tesla coil
    WEAPON_NIKE = 23,          // SAM site
    WEAPON_8INCH = 24,         // Naval gun
    WEAPON_STINGER = 25,       // AA missile
    WEAPON_TORPEDO = 26,       // Sub torpedo
    WEAPON_2INCH = 27,         // Gunboat
    WEAPON_DEPTH_CHARGE = 28,  // Anti-sub
    WEAPON_PARABOMB = 29,      // Parachute bomb
    WEAPON_DOGJAW = 30,        // Attack dog
    WEAPON_HEAL = 31,          // Medic heal (friendly fire)
    WEAPON_SCUD = 32,          // V2 rocket
    WEAPON_FLAK = 33,          // Anti-air flak
    WEAPON_APHE = 34,          // High explosive

    WEAPON_COUNT = 35
};

// =============================================================================
// Warhead Types
// =============================================================================

/**
 * WarheadType - Damage delivery types
 *
 * Determines:
 * - Armor penetration vs different armor types
 * - Spread/splash damage
 * - Special effects (fire, EMP, etc.)
 */
enum WarheadType : int8_t {
    WARHEAD_SA = 0,        // Small arms (bullets)
    WARHEAD_HE = 1,        // High explosive
    WARHEAD_AP = 2,        // Armor piercing
    WARHEAD_FIRE = 3,      // Incendiary
    WARHEAD_HOLLOW_POINT = 4,  // Anti-infantry
    WARHEAD_TESLA = 5,     // Tesla (electric)
    WARHEAD_NUKE = 6,      // Nuclear
    WARHEAD_MECHANICAL = 7, // Crush damage

    WARHEAD_COUNT = 8
};

// =============================================================================
// Bullet/Projectile Types
// =============================================================================

/**
 * BulletType - Visual projectile types
 */
enum BulletType : int8_t {
    BULLET_INVISIBLE = 0,  // Instant hit (bullets)
    BULLET_CANNON = 1,     // Shell
    BULLET_ACK = 2,        // Anti-air tracer
    BULLET_TORPEDO = 3,    // Water torpedo
    BULLET_FROG = 4,       // Frog (unused)
    BULLET_HEATSEEKER = 5, // Guided missile
    BULLET_LASERGUIDED = 6,// Laser guided bomb
    BULLET_LOBBED = 7,     // Arcing projectile
    BULLET_BOMBLET = 8,    // Cluster bomb
    BULLET_BALLISTIC = 9,  // V2 rocket
    BULLET_PARACHUTE = 10, // Parachute bomb
    BULLET_FIREBALL = 11,  // Flame
    BULLET_DOG = 12,       // Dog bite (instant)
    BULLET_CATAPULT = 13,  // Catapult (unused)
    BULLET_AAMISSILE = 14, // AA missile

    BULLET_COUNT = 15
};

// =============================================================================
// Weapon Data Structures
// =============================================================================

/**
 * Warhead damage vs armor table entry
 */
struct WarheadVsArmor {
    int16_t versus[ARMOR_COUNT];  // Damage multiplier per armor type (%)
};

/**
 * Weapon definition structure
 */
struct WeaponData {
    const char* Name;           // Internal name
    int Damage;                 // Base damage
    int Range;                  // Range in leptons
    int ROF;                    // Rate of fire (frames between shots)
    BulletType Projectile;      // Projectile graphic
    WarheadType Warhead;        // Damage type
    int Speed;                  // Projectile speed
    int Sound;                  // Sound effect ID
    bool TwoShots;              // Fire twice per attack
    bool Anim;                  // Has muzzle flash
};

// =============================================================================
// Weapon Data Tables (defined in weapon.cpp)
// =============================================================================

extern const WeaponData WeaponTable[WEAPON_COUNT];
extern const WarheadVsArmor WarheadTable[WARHEAD_COUNT];

// =============================================================================
// Weapon Utility Functions
// =============================================================================

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Calculate damage after armor reduction
 */
int Calculate_Damage(int base_damage, WarheadType warhead, ArmorType armor);

/**
 * Get weapon name string
 */
const char* Weapon_Name(WeaponType weapon);

/**
 * Get armor name string
 */
const char* Armor_Name(ArmorType armor);

/**
 * Check if weapon can damage armor type
 */
int Weapon_Can_Damage(WeaponType weapon, ArmorType armor);

#ifdef __cplusplus
}
#endif
