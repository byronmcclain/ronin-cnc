/**
 * Red Alert Facing/Direction System
 *
 * The game uses a 256-step direction system for smooth rotation.
 * Sprites typically have 8 or 32 rotational frames.
 *
 * Direction values:
 *   0 = North (up)
 *   32 = NE
 *   64 = East (right)
 *   96 = SE
 *   128 = South (down)
 *   160 = SW
 *   192 = West (left)
 *   224 = NW
 */

#pragma once

#include <cstdint>

// =============================================================================
// Direction Type
// =============================================================================

/**
 * DirType - 256-step direction (0-255)
 * Used for unit facing, turret rotation, projectile trajectory
 */
typedef uint8_t DirType;

// =============================================================================
// Direction Constants
// =============================================================================

constexpr DirType DIR_N   = 0;     // North (up)
constexpr DirType DIR_NE  = 32;    // Northeast
constexpr DirType DIR_E   = 64;    // East (right)
constexpr DirType DIR_SE  = 96;    // Southeast
constexpr DirType DIR_S   = 128;   // South (down)
constexpr DirType DIR_SW  = 160;   // Southwest
constexpr DirType DIR_W   = 192;   // West (left)
constexpr DirType DIR_NW  = 224;   // Northwest

// Special values
constexpr DirType DIR_NONE = 255;  // Invalid/unset direction

// Rotation amounts
constexpr DirType DIR_ROTATION_45 = 32;   // 45 degrees
constexpr DirType DIR_ROTATION_90 = 64;   // 90 degrees
constexpr DirType DIR_ROTATION_180 = 128; // 180 degrees

// =============================================================================
// FacingClass - Smooth rotation handler
// =============================================================================

/**
 * FacingClass handles smooth rotation between directions.
 * Used by units and turrets to turn gradually rather than snap.
 *
 * Original location: CODE/FACING.H, CODE/FACING.CPP
 */
class FacingClass {
public:
    FacingClass() : current_(DIR_N), desired_(DIR_N), rate_(0) {}
    explicit FacingClass(DirType initial) : current_(initial), desired_(initial), rate_(0) {}

    // Get current facing
    DirType Current() const { return current_; }

    // Get desired (target) facing
    DirType Desired() const { return desired_; }

    // Set new desired facing
    void Set_Desired(DirType dir) { desired_ = dir; }

    // Set rotation rate (steps per tick)
    void Set_Rate(uint8_t rate) { rate_ = rate; }

    // Snap to desired facing instantly
    void Snap() { current_ = desired_; }

    // Is at desired facing?
    bool Is_At_Target() const { return current_ == desired_; }

    // Get difference between current and desired
    int8_t Difference() const;

    // Update facing towards desired (call each tick)
    // Returns true if facing changed
    bool Rotate();

    // Get which way to rotate (+1 = clockwise, -1 = counter-clockwise)
    int Rotation_Direction() const;

    // Set both current and desired
    void Set(DirType dir) {
        current_ = dir;
        desired_ = dir;
    }

private:
    DirType current_;    // Current facing direction
    DirType desired_;    // Target facing direction
    uint8_t rate_;       // Rotation speed (0 = instant)
};

// =============================================================================
// Direction Utility Functions
// =============================================================================

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get opposite direction
 */
DirType Dir_Opposite(DirType dir);

/**
 * Normalize direction to 8-way facing (for sprite selection)
 * Returns 0-7 (N, NE, E, SE, S, SW, W, NW)
 */
int Dir_To_8Way(DirType dir);

/**
 * Normalize direction to 16-way facing
 * Returns 0-15
 */
int Dir_To_16Way(DirType dir);

/**
 * Normalize direction to 32-way facing
 * Returns 0-31
 */
int Dir_To_32Way(DirType dir);

/**
 * Convert 8-way facing back to DirType
 */
DirType Way8_To_Dir(int way);

/**
 * Shortest rotation direction from one dir to another
 * Returns positive for clockwise, negative for counter-clockwise
 */
int8_t Dir_Delta(DirType from, DirType to);

/**
 * Calculate direction from dx, dy deltas
 */
DirType Dir_From_XY(int dx, int dy);

/**
 * Get X component of direction vector (scaled by 256)
 */
int Dir_X_Factor(DirType dir);

/**
 * Get Y component of direction vector (scaled by 256)
 */
int Dir_Y_Factor(DirType dir);

#ifdef __cplusplus
}
#endif

// =============================================================================
// Precomputed Direction Tables
// =============================================================================

// Sine table for direction (256 entries, -127 to +127)
extern const int8_t DirSineTable[256];

// Cosine table for direction (256 entries, -127 to +127)
extern const int8_t DirCosineTable[256];

// 8-way facing for each 256 direction
extern const uint8_t Dir8Way[256];
