/**
 * PaletteManager - Centralized Palette Control
 *
 * Manages the 256-color palette with support for:
 * - Theater palette loading
 * - Fade in/out effects
 * - Flash effects
 * - Color animation (water, fire)
 * - Protected palette ranges
 *
 * Usage:
 *   PaletteManager& pm = PaletteManager::Instance();
 *   pm.LoadTheaterPalette(THEATER_TEMPERATE);
 *   pm.StartFadeIn(30);  // 30-frame fade
 *   pm.Update();         // Call each frame
 *
 * Original: CODE/PALETTE.CPP, WIN32LIB/PALETTE.CPP
 */

#ifndef GAME_GRAPHICS_PALETTE_MANAGER_H
#define GAME_GRAPHICS_PALETTE_MANAGER_H

#include <cstdint>
#include <functional>

// Forward declarations
enum TheaterType : int8_t;

// =============================================================================
// Palette Entry
// =============================================================================

/**
 * RGB color entry (8-bit per channel)
 */
struct PaletteColor {
    uint8_t r, g, b;

    PaletteColor() : r(0), g(0), b(0) {}
    PaletteColor(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}

    // Interpolate between two colors
    static PaletteColor Lerp(const PaletteColor& a, const PaletteColor& b, float t) {
        return PaletteColor(
            static_cast<uint8_t>(a.r + (b.r - a.r) * t),
            static_cast<uint8_t>(a.g + (b.g - a.g) * t),
            static_cast<uint8_t>(a.b + (b.b - a.b) * t)
        );
    }

    // Scale color by factor (0.0 = black, 1.0 = unchanged)
    PaletteColor Scaled(float factor) const {
        return PaletteColor(
            static_cast<uint8_t>(r * factor),
            static_cast<uint8_t>(g * factor),
            static_cast<uint8_t>(b * factor)
        );
    }

    bool operator==(const PaletteColor& other) const {
        return r == other.r && g == other.g && b == other.b;
    }
};

// =============================================================================
// Fade State
// =============================================================================

enum class FadeState {
    None,       // No fade in progress
    FadingIn,   // Fading from black to full
    FadingOut,  // Fading from full to black
    FadedOut    // Fully faded to black
};

// =============================================================================
// Flash Type
// =============================================================================

enum class FlashType {
    None,
    White,      // Explosion, nuke
    Red,        // Damage, fire
    Green,      // Heal, powerup
    Custom      // User-defined color
};

// =============================================================================
// Animation Range
// =============================================================================

struct AnimationRange {
    int start;          // First palette index
    int count;          // Number of entries
    int frame_delay;    // Frames between rotations
    int current_frame;  // Current frame counter
    bool forward;       // Direction of rotation

    AnimationRange() : start(0), count(0), frame_delay(4), current_frame(0), forward(true) {}
    AnimationRange(int s, int c, int d = 4) : start(s), count(c), frame_delay(d), current_frame(0), forward(true) {}
};

// =============================================================================
// PaletteManager Class
// =============================================================================

class PaletteManager {
public:
    // =========================================================================
    // Constants
    // =========================================================================

    static constexpr int PALETTE_SIZE = 256;
    static constexpr int PALETTE_BYTES = PALETTE_SIZE * 3;

    // Standard animation ranges
    static constexpr int WATER_ANIM_START = 192;
    static constexpr int WATER_ANIM_COUNT = 16;
    static constexpr int FIRE_ANIM_START = 96;
    static constexpr int FIRE_ANIM_COUNT = 16;

    // Protected ranges (don't modify during effects)
    static constexpr int UI_RANGE_START = 1;
    static constexpr int UI_RANGE_END = 15;

    // =========================================================================
    // Singleton
    // =========================================================================

    static PaletteManager& Instance();

    // Non-copyable
    PaletteManager(const PaletteManager&) = delete;
    PaletteManager& operator=(const PaletteManager&) = delete;

    // =========================================================================
    // Initialization
    // =========================================================================

    /**
     * Initialize with default palette (grayscale)
     */
    void Init();

    /**
     * Load a theater palette
     *
     * @param theater Theater type
     * @return true if loaded successfully
     */
    bool LoadTheaterPalette(TheaterType theater);

    /**
     * Load palette from file
     *
     * @param filename PAL filename (in MIX)
     * @return true if loaded successfully
     */
    bool LoadPalette(const char* filename);

    /**
     * Load palette from raw data
     *
     * @param data   768 bytes of RGB data (6-bit or 8-bit)
     * @param is_6bit True if data uses 6-bit VGA values
     */
    void LoadPaletteFromMemory(const uint8_t* data, bool is_6bit = true);

    /**
     * Set entire palette directly
     *
     * @param colors Array of 256 PaletteColor entries
     */
    void SetPalette(const PaletteColor* colors);

    // =========================================================================
    // Palette Access
    // =========================================================================

    /**
     * Get the current (effected) palette
     */
    const PaletteColor* GetCurrentPalette() const { return current_; }

    /**
     * Get the original (uneffected) palette
     */
    const PaletteColor* GetOriginalPalette() const { return original_; }

    /**
     * Get a specific color from current palette
     */
    const PaletteColor& GetColor(int index) const {
        return current_[index & 0xFF];
    }

    /**
     * Set a specific color in original palette
     *
     * The change is reflected in current palette after effects are applied.
     */
    void SetColor(int index, const PaletteColor& color);

    /**
     * Get raw palette data (for platform layer)
     *
     * @param output Buffer for 768 bytes (256 RGB triplets)
     */
    void GetRawPalette(uint8_t* output) const;

    // =========================================================================
    // Fade Effects
    // =========================================================================

    /**
     * Start fade-in from black
     *
     * @param frames Number of frames for fade (15 = 1 second at 15fps)
     * @param callback Optional callback when fade completes
     */
    void StartFadeIn(int frames, std::function<void()> callback = nullptr);

    /**
     * Start fade-out to black
     *
     * @param frames Number of frames for fade
     * @param callback Optional callback when fade completes
     */
    void StartFadeOut(int frames, std::function<void()> callback = nullptr);

    /**
     * Instant fade to black (no animation)
     */
    void FadeToBlack();

    /**
     * Instant restore from black
     */
    void RestoreFromBlack();

    /**
     * Get current fade state
     */
    FadeState GetFadeState() const { return fade_state_; }

    /**
     * Get fade progress (0.0 = black, 1.0 = full)
     */
    float GetFadeProgress() const { return fade_progress_; }

    /**
     * Check if fade is in progress
     */
    bool IsFading() const {
        return fade_state_ == FadeState::FadingIn || fade_state_ == FadeState::FadingOut;
    }

    // =========================================================================
    // Flash Effects
    // =========================================================================

    /**
     * Start a flash effect
     *
     * @param type     Flash color type
     * @param duration Frames to hold flash
     * @param intensity Maximum intensity (0.0 to 1.0)
     */
    void StartFlash(FlashType type, int duration, float intensity = 1.0f);

    /**
     * Start custom color flash
     *
     * @param color    Flash color
     * @param duration Frames to hold
     * @param intensity Maximum intensity
     */
    void StartFlash(const PaletteColor& color, int duration, float intensity = 1.0f);

    /**
     * Stop current flash immediately
     */
    void StopFlash();

    /**
     * Check if flash is active
     */
    bool IsFlashing() const { return flash_active_; }

    // =========================================================================
    // Color Animation
    // =========================================================================

    /**
     * Enable/disable water animation
     */
    void SetWaterAnimationEnabled(bool enabled);

    /**
     * Enable/disable fire animation
     */
    void SetFireAnimationEnabled(bool enabled);

    /**
     * Add custom animation range
     *
     * @param start First palette index
     * @param count Number of entries to cycle
     * @param frame_delay Frames between rotations
     */
    void AddAnimationRange(int start, int count, int frame_delay = 4);

    /**
     * Remove custom animation range
     */
    void RemoveAnimationRange(int start);

    /**
     * Clear all animation ranges
     */
    void ClearAnimationRanges();

    // =========================================================================
    // Update
    // =========================================================================

    /**
     * Update palette effects
     *
     * Call once per frame to advance fades, flashes, and animations.
     * Automatically applies changes to platform layer.
     */
    void Update();

    /**
     * Force palette update to platform layer
     *
     * Normally called automatically by Update().
     */
    void Apply();

    // =========================================================================
    // Utility
    // =========================================================================

    /**
     * Generate remap table for house colors
     *
     * @param house House index (0-7)
     * @param output 256-byte remap table
     */
    void GenerateHouseRemap(int house, uint8_t* output) const;

    /**
     * Generate shadow table from current palette
     *
     * @param output 256-byte shadow table
     * @param intensity Shadow darkness (0.0-1.0)
     */
    void GenerateShadowTable(uint8_t* output, float intensity = 0.5f) const;

    /**
     * Find closest palette index to RGB color
     *
     * @param r, g, b Target color (8-bit)
     * @param skip_zero If true, never returns 0 (transparent)
     */
    int FindClosestColor(int r, int g, int b, bool skip_zero = true) const;

private:
    PaletteManager();
    ~PaletteManager();

    // =========================================================================
    // Private Data
    // =========================================================================

    // Palette storage
    PaletteColor original_[PALETTE_SIZE];  // Base palette (unmodified)
    PaletteColor current_[PALETTE_SIZE];   // Current with effects applied

    // Fade state
    FadeState fade_state_;
    float fade_progress_;         // 0.0 (black) to 1.0 (full)
    float fade_step_;             // Progress per frame
    std::function<void()> fade_callback_;

    // Flash state
    bool flash_active_;
    PaletteColor flash_color_;
    float flash_intensity_;
    float flash_current_;         // Current intensity (decays)
    int flash_duration_;
    int flash_frame_;

    // Animation ranges
    static constexpr int MAX_ANIM_RANGES = 8;
    AnimationRange anim_ranges_[MAX_ANIM_RANGES];
    int anim_range_count_;
    bool water_anim_enabled_;
    bool fire_anim_enabled_;

    // Dirty flag
    bool needs_apply_;

    // =========================================================================
    // Private Methods
    // =========================================================================

    void UpdateFade();
    void UpdateFlash();
    void UpdateAnimations();
    void ApplyEffects();
    void RotateRange(int start, int count, bool forward);
};

// =============================================================================
// Convenience Functions
// =============================================================================

/**
 * Fade screen to black over specified frames
 */
inline void Fade_Out(int frames = 15) {
    PaletteManager::Instance().StartFadeOut(frames);
}

/**
 * Fade screen from black over specified frames
 */
inline void Fade_In(int frames = 15) {
    PaletteManager::Instance().StartFadeIn(frames);
}

/**
 * Flash screen white (explosion effect)
 */
inline void Flash_White(int duration = 2) {
    PaletteManager::Instance().StartFlash(FlashType::White, duration);
}

/**
 * Flash screen red (damage effect)
 */
inline void Flash_Red(int duration = 2) {
    PaletteManager::Instance().StartFlash(FlashType::Red, duration);
}

#endif // GAME_GRAPHICS_PALETTE_MANAGER_H
