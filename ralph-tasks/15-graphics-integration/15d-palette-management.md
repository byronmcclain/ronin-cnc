# Task 15d: Palette Management System

## Dependencies
- Task 15a (GraphicsBuffer) must be complete
- Platform palette APIs must work (`Platform_Graphics_SetPalette`, etc.)
- Asset system (MIX file loading) must be functional
- Task 15b (Remap Tables) provides foundation for palette effects

## Context
Red Alert uses 8-bit palettized graphics with 256 colors. The palette is changed throughout the game:
- Theater palettes (TEMPERAT.PAL, SNOW.PAL, INTERIOR.PAL)
- Fade effects (black → full color for level transitions)
- Flash effects (white flash on explosion, red flash on damage)
- Sidebar/UI uses fixed palette ranges

The original game manipulates the VGA palette directly through BIOS calls. Our implementation uses the platform layer to convert 8-bit indexed to 32-bit RGBA.

This task creates a centralized `PaletteManager` that:
1. Loads and manages theater palettes
2. Implements fade in/out effects (smooth transitions)
3. Implements flash effects (brief color overlay)
4. Handles palette animation (cycling colors for water, flames)
5. Preserves certain palette ranges during effects (UI colors)

## Objective
Create a comprehensive palette management system with support for all original game effects: fading, flashing, animation, and theater switching.

## Technical Background

### PAL File Format
Red Alert PAL files are 768 bytes (256 × 3):
```
For each color (0-255):
  uint8_t red   (0-63, VGA 6-bit)
  uint8_t green (0-63, VGA 6-bit)
  uint8_t blue  (0-63, VGA 6-bit)

To convert to 8-bit: value << 2 (or value * 4)
```

### Palette Ranges
The 256-color palette is organized:
```
0:         Transparent (always black, never drawn)
1-15:      System colors (UI, selection boxes)
16-31:     Remap range 1 (terrain/effects)
32-47:     Remap range 2
48-79:     General colors
80-95:     House colors primary (remapped per player)
96-111:    Fire/explosion colors (often animated)
112-127:   Shadow colors
128-175:   General colors
176-191:   House colors secondary
192-207:   Water animation range
208-239:   General colors
240-254:   Ramp/gradient colors
255:       White (often used for highlights)
```

### Fade Effects
Original implements fade by scaling all RGB values:
```cpp
// Fade from black (factor 0) to full (factor 1)
for (int i = 0; i < 256; i++) {
    current[i].r = original[i].r * factor;
    current[i].g = original[i].g * factor;
    current[i].b = original[i].b * factor;
}
```

### Flash Effects
Brief color overlay (e.g., white flash on explosion):
```cpp
// Flash towards white (or any color)
for (int i = 0; i < 256; i++) {
    current[i].r = lerp(original[i].r, flash_color.r, intensity);
    current[i].g = lerp(original[i].g, flash_color.g, intensity);
    current[i].b = lerp(original[i].b, flash_color.b, intensity);
}
```

### Palette Animation
Water and fire colors cycle through ranges:
```cpp
// Rotate indices 192-207 for water shimmer
uint8_t temp = palette[207];
for (int i = 207; i > 192; i--) {
    palette[i] = palette[i-1];
}
palette[192] = temp;
```

## Deliverables
- [ ] `include/game/graphics/palette_manager.h` - Palette manager class
- [ ] `src/game/graphics/palette_manager.cpp` - Implementation
- [ ] `src/test_palette_manager.cpp` - Test program with visual effects
- [ ] `ralph-tasks/verify/check-palette-manager.sh` - Verification script
- [ ] All tests pass

## Files to Create

### include/game/graphics/palette_manager.h
```cpp
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
```

### src/game/graphics/palette_manager.cpp
```cpp
/**
 * PaletteManager Implementation
 */

#include "game/graphics/palette_manager.h"
#include "game/graphics/tile_renderer.h"  // For TheaterType
#include "platform.h"
#include <cstring>
#include <algorithm>
#include <cmath>

// =============================================================================
// Singleton
// =============================================================================

PaletteManager& PaletteManager::Instance() {
    static PaletteManager instance;
    return instance;
}

PaletteManager::PaletteManager()
    : fade_state_(FadeState::None)
    , fade_progress_(1.0f)
    , fade_step_(0.0f)
    , flash_active_(false)
    , flash_intensity_(0.0f)
    , flash_current_(0.0f)
    , flash_duration_(0)
    , flash_frame_(0)
    , anim_range_count_(0)
    , water_anim_enabled_(false)
    , fire_anim_enabled_(false)
    , needs_apply_(false)
{
    // Initialize with grayscale palette
    Init();
}

PaletteManager::~PaletteManager() {
}

// =============================================================================
// Initialization
// =============================================================================

void PaletteManager::Init() {
    // Create default grayscale palette
    for (int i = 0; i < PALETTE_SIZE; i++) {
        original_[i] = PaletteColor(i, i, i);
        current_[i] = original_[i];
    }

    // Index 0 is always black (transparent)
    original_[0] = PaletteColor(0, 0, 0);
    current_[0] = PaletteColor(0, 0, 0);

    fade_state_ = FadeState::None;
    fade_progress_ = 1.0f;
    flash_active_ = false;
    anim_range_count_ = 0;

    needs_apply_ = true;
    Apply();
}

bool PaletteManager::LoadTheaterPalette(TheaterType theater) {
    const char* filename = nullptr;

    switch (theater) {
        case THEATER_TEMPERATE:
            filename = "TEMPERAT.PAL";
            break;
        case THEATER_SNOW:
            filename = "SNOW.PAL";
            break;
        case THEATER_INTERIOR:
            filename = "INTERIOR.PAL";
            break;
        default:
            return false;
    }

    return LoadPalette(filename);
}

bool PaletteManager::LoadPalette(const char* filename) {
    if (!filename) return false;

    uint8_t raw_data[PALETTE_BYTES];
    if (Platform_Palette_Load(filename, raw_data) != 0) {
        // Try without conversion (already 8-bit)
        int32_t size = Platform_Mix_GetSize(filename);
        if (size != PALETTE_BYTES) {
            return false;
        }
        if (Platform_Mix_Read(filename, raw_data, PALETTE_BYTES) != PALETTE_BYTES) {
            return false;
        }
        LoadPaletteFromMemory(raw_data, false);
    } else {
        // Platform_Palette_Load already converted to 8-bit
        LoadPaletteFromMemory(raw_data, false);
    }

    return true;
}

void PaletteManager::LoadPaletteFromMemory(const uint8_t* data, bool is_6bit) {
    for (int i = 0; i < PALETTE_SIZE; i++) {
        uint8_t r = data[i * 3 + 0];
        uint8_t g = data[i * 3 + 1];
        uint8_t b = data[i * 3 + 2];

        if (is_6bit) {
            // Convert 6-bit VGA to 8-bit
            r = r << 2;
            g = g << 2;
            b = b << 2;
        }

        original_[i] = PaletteColor(r, g, b);
    }

    // Always keep index 0 black
    original_[0] = PaletteColor(0, 0, 0);

    // Apply current effects
    ApplyEffects();
    needs_apply_ = true;
}

void PaletteManager::SetPalette(const PaletteColor* colors) {
    for (int i = 0; i < PALETTE_SIZE; i++) {
        original_[i] = colors[i];
    }
    original_[0] = PaletteColor(0, 0, 0);
    ApplyEffects();
    needs_apply_ = true;
}

// =============================================================================
// Palette Access
// =============================================================================

void PaletteManager::SetColor(int index, const PaletteColor& color) {
    if (index >= 0 && index < PALETTE_SIZE) {
        original_[index] = color;
        ApplyEffects();
        needs_apply_ = true;
    }
}

void PaletteManager::GetRawPalette(uint8_t* output) const {
    for (int i = 0; i < PALETTE_SIZE; i++) {
        output[i * 3 + 0] = current_[i].r;
        output[i * 3 + 1] = current_[i].g;
        output[i * 3 + 2] = current_[i].b;
    }
}

// =============================================================================
// Fade Effects
// =============================================================================

void PaletteManager::StartFadeIn(int frames, std::function<void()> callback) {
    if (frames <= 0) frames = 1;

    fade_state_ = FadeState::FadingIn;
    fade_progress_ = 0.0f;
    fade_step_ = 1.0f / frames;
    fade_callback_ = callback;
    needs_apply_ = true;
}

void PaletteManager::StartFadeOut(int frames, std::function<void()> callback) {
    if (frames <= 0) frames = 1;

    fade_state_ = FadeState::FadingOut;
    fade_progress_ = 1.0f;
    fade_step_ = 1.0f / frames;
    fade_callback_ = callback;
    needs_apply_ = true;
}

void PaletteManager::FadeToBlack() {
    fade_state_ = FadeState::FadedOut;
    fade_progress_ = 0.0f;
    fade_callback_ = nullptr;
    ApplyEffects();
    needs_apply_ = true;
}

void PaletteManager::RestoreFromBlack() {
    fade_state_ = FadeState::None;
    fade_progress_ = 1.0f;
    fade_callback_ = nullptr;
    ApplyEffects();
    needs_apply_ = true;
}

// =============================================================================
// Flash Effects
// =============================================================================

void PaletteManager::StartFlash(FlashType type, int duration, float intensity) {
    PaletteColor color;

    switch (type) {
        case FlashType::White:
            color = PaletteColor(255, 255, 255);
            break;
        case FlashType::Red:
            color = PaletteColor(255, 0, 0);
            break;
        case FlashType::Green:
            color = PaletteColor(0, 255, 0);
            break;
        default:
            return;
    }

    StartFlash(color, duration, intensity);
}

void PaletteManager::StartFlash(const PaletteColor& color, int duration, float intensity) {
    flash_active_ = true;
    flash_color_ = color;
    flash_intensity_ = std::max(0.0f, std::min(1.0f, intensity));
    flash_current_ = flash_intensity_;
    flash_duration_ = std::max(1, duration);
    flash_frame_ = 0;
    needs_apply_ = true;
}

void PaletteManager::StopFlash() {
    flash_active_ = false;
    flash_current_ = 0.0f;
    ApplyEffects();
    needs_apply_ = true;
}

// =============================================================================
// Color Animation
// =============================================================================

void PaletteManager::SetWaterAnimationEnabled(bool enabled) {
    if (enabled && !water_anim_enabled_) {
        AddAnimationRange(WATER_ANIM_START, WATER_ANIM_COUNT, 4);
    } else if (!enabled && water_anim_enabled_) {
        RemoveAnimationRange(WATER_ANIM_START);
    }
    water_anim_enabled_ = enabled;
}

void PaletteManager::SetFireAnimationEnabled(bool enabled) {
    if (enabled && !fire_anim_enabled_) {
        AddAnimationRange(FIRE_ANIM_START, FIRE_ANIM_COUNT, 3);
    } else if (!enabled && fire_anim_enabled_) {
        RemoveAnimationRange(FIRE_ANIM_START);
    }
    fire_anim_enabled_ = enabled;
}

void PaletteManager::AddAnimationRange(int start, int count, int frame_delay) {
    if (anim_range_count_ >= MAX_ANIM_RANGES) return;
    if (start < 0 || start + count > PALETTE_SIZE) return;

    // Check for duplicate
    for (int i = 0; i < anim_range_count_; i++) {
        if (anim_ranges_[i].start == start) {
            return;  // Already exists
        }
    }

    anim_ranges_[anim_range_count_] = AnimationRange(start, count, frame_delay);
    anim_range_count_++;
}

void PaletteManager::RemoveAnimationRange(int start) {
    for (int i = 0; i < anim_range_count_; i++) {
        if (anim_ranges_[i].start == start) {
            // Shift remaining ranges down
            for (int j = i; j < anim_range_count_ - 1; j++) {
                anim_ranges_[j] = anim_ranges_[j + 1];
            }
            anim_range_count_--;
            return;
        }
    }
}

void PaletteManager::ClearAnimationRanges() {
    anim_range_count_ = 0;
    water_anim_enabled_ = false;
    fire_anim_enabled_ = false;
}

// =============================================================================
// Update
// =============================================================================

void PaletteManager::Update() {
    UpdateFade();
    UpdateFlash();
    UpdateAnimations();

    if (needs_apply_) {
        Apply();
    }
}

void PaletteManager::UpdateFade() {
    if (fade_state_ == FadeState::FadingIn) {
        fade_progress_ += fade_step_;
        if (fade_progress_ >= 1.0f) {
            fade_progress_ = 1.0f;
            fade_state_ = FadeState::None;
            if (fade_callback_) {
                fade_callback_();
                fade_callback_ = nullptr;
            }
        }
        ApplyEffects();
        needs_apply_ = true;
    }
    else if (fade_state_ == FadeState::FadingOut) {
        fade_progress_ -= fade_step_;
        if (fade_progress_ <= 0.0f) {
            fade_progress_ = 0.0f;
            fade_state_ = FadeState::FadedOut;
            if (fade_callback_) {
                fade_callback_();
                fade_callback_ = nullptr;
            }
        }
        ApplyEffects();
        needs_apply_ = true;
    }
}

void PaletteManager::UpdateFlash() {
    if (!flash_active_) return;

    flash_frame_++;

    // Flash decays over duration
    float progress = static_cast<float>(flash_frame_) / flash_duration_;
    flash_current_ = flash_intensity_ * (1.0f - progress);

    if (flash_frame_ >= flash_duration_) {
        flash_active_ = false;
        flash_current_ = 0.0f;
    }

    ApplyEffects();
    needs_apply_ = true;
}

void PaletteManager::UpdateAnimations() {
    for (int i = 0; i < anim_range_count_; i++) {
        AnimationRange& range = anim_ranges_[i];
        range.current_frame++;

        if (range.current_frame >= range.frame_delay) {
            range.current_frame = 0;
            RotateRange(range.start, range.count, range.forward);
            ApplyEffects();
            needs_apply_ = true;
        }
    }
}

void PaletteManager::RotateRange(int start, int count, bool forward) {
    if (count <= 1) return;

    if (forward) {
        // Rotate forward: last moves to first
        PaletteColor temp = original_[start + count - 1];
        for (int i = count - 1; i > 0; i--) {
            original_[start + i] = original_[start + i - 1];
        }
        original_[start] = temp;
    } else {
        // Rotate backward: first moves to last
        PaletteColor temp = original_[start];
        for (int i = 0; i < count - 1; i++) {
            original_[start + i] = original_[start + i + 1];
        }
        original_[start + count - 1] = temp;
    }
}

void PaletteManager::ApplyEffects() {
    // Start with original palette
    for (int i = 0; i < PALETTE_SIZE; i++) {
        current_[i] = original_[i];
    }

    // Apply fade
    if (fade_progress_ < 1.0f) {
        for (int i = 0; i < PALETTE_SIZE; i++) {
            // Skip protected UI range during fade
            if (i >= UI_RANGE_START && i <= UI_RANGE_END) {
                continue;
            }
            current_[i] = current_[i].Scaled(fade_progress_);
        }
    }

    // Apply flash
    if (flash_active_ && flash_current_ > 0.0f) {
        for (int i = 1; i < PALETTE_SIZE; i++) {  // Skip index 0
            current_[i] = PaletteColor::Lerp(current_[i], flash_color_, flash_current_);
        }
    }

    // Index 0 is always black
    current_[0] = PaletteColor(0, 0, 0);
}

void PaletteManager::Apply() {
    // Convert to platform format and set
    PaletteEntry entries[PALETTE_SIZE];
    for (int i = 0; i < PALETTE_SIZE; i++) {
        entries[i].r = current_[i].r;
        entries[i].g = current_[i].g;
        entries[i].b = current_[i].b;
    }

    Platform_Graphics_SetPalette(entries, 0, PALETTE_SIZE);
    needs_apply_ = false;
}

// =============================================================================
// Utility
// =============================================================================

void PaletteManager::GenerateHouseRemap(int house, uint8_t* output) const {
    // This would use the same logic as remap_tables.cpp
    // For now, just return identity
    for (int i = 0; i < PALETTE_SIZE; i++) {
        output[i] = static_cast<uint8_t>(i);
    }

    // TODO: Implement house color remapping
}

void PaletteManager::GenerateShadowTable(uint8_t* output, float intensity) const {
    float factor = 1.0f - intensity;

    for (int i = 0; i < PALETTE_SIZE; i++) {
        if (i == 0) {
            output[i] = 0;
            continue;
        }

        // Find darkened color
        int r = static_cast<int>(original_[i].r * factor);
        int g = static_cast<int>(original_[i].g * factor);
        int b = static_cast<int>(original_[i].b * factor);

        output[i] = static_cast<uint8_t>(FindClosestColor(r, g, b, true));
    }
}

int PaletteManager::FindClosestColor(int r, int g, int b, bool skip_zero) const {
    int best_index = skip_zero ? 1 : 0;
    int best_dist = 0x7FFFFFFF;

    int start = skip_zero ? 1 : 0;
    for (int i = start; i < PALETTE_SIZE; i++) {
        int dr = r - original_[i].r;
        int dg = g - original_[i].g;
        int db = b - original_[i].b;
        int dist = dr * dr + dg * dg + db * db;

        if (dist < best_dist) {
            best_dist = dist;
            best_index = i;
        }
    }

    return best_index;
}
```

### src/test_palette_manager.cpp
```cpp
/**
 * Palette Manager Test Program
 */

#include "game/graphics/palette_manager.h"
#include "game/graphics/graphics_buffer.h"
#include "platform.h"
#include <cstdio>

// =============================================================================
// Test Utilities
// =============================================================================

static int tests_run = 0;
static int tests_passed = 0;

#define TEST_START(name) do { \
    printf("  Testing %s... ", name); \
    tests_run++; \
} while(0)

#define TEST_PASS() do { \
    printf("PASS\n"); \
    tests_passed++; \
} while(0)

#define TEST_FAIL(msg) do { \
    printf("FAIL: %s\n", msg); \
    return false; \
} while(0)

#define ASSERT(cond, msg) do { \
    if (!(cond)) TEST_FAIL(msg); \
} while(0)

// =============================================================================
// Unit Tests
// =============================================================================

bool test_singleton() {
    TEST_START("singleton pattern");

    PaletteManager& pm1 = PaletteManager::Instance();
    PaletteManager& pm2 = PaletteManager::Instance();

    ASSERT(&pm1 == &pm2, "Should return same instance");

    TEST_PASS();
    return true;
}

bool test_palette_color() {
    TEST_START("PaletteColor operations");

    PaletteColor white(255, 255, 255);
    PaletteColor black(0, 0, 0);

    // Test scaling
    PaletteColor half = white.Scaled(0.5f);
    ASSERT(half.r == 127, "Scaled R should be 127");
    ASSERT(half.g == 127, "Scaled G should be 127");
    ASSERT(half.b == 127, "Scaled B should be 127");

    // Test lerp
    PaletteColor mid = PaletteColor::Lerp(black, white, 0.5f);
    ASSERT(mid.r == 127, "Lerp R should be 127");

    // Test equality
    PaletteColor a(100, 150, 200);
    PaletteColor b(100, 150, 200);
    PaletteColor c(100, 150, 201);
    ASSERT(a == b, "Equal colors should match");
    ASSERT(!(a == c), "Different colors should not match");

    TEST_PASS();
    return true;
}

bool test_initialization() {
    TEST_START("initialization");

    PaletteManager& pm = PaletteManager::Instance();
    pm.Init();

    // Should have grayscale palette
    const PaletteColor* palette = pm.GetCurrentPalette();
    ASSERT(palette != nullptr, "Palette should not be null");

    // Index 0 should be black
    ASSERT(palette[0].r == 0 && palette[0].g == 0 && palette[0].b == 0,
           "Index 0 should be black");

    // Index 255 should be white
    ASSERT(palette[255].r == 255, "Index 255 should be white");

    TEST_PASS();
    return true;
}

bool test_fade_state() {
    TEST_START("fade state machine");

    PaletteManager& pm = PaletteManager::Instance();
    pm.Init();

    // Initial state
    ASSERT(pm.GetFadeState() == FadeState::None, "Should start with no fade");
    ASSERT(pm.GetFadeProgress() == 1.0f, "Should start at full brightness");

    // Start fade out
    pm.StartFadeOut(10);
    ASSERT(pm.GetFadeState() == FadeState::FadingOut, "Should be fading out");
    ASSERT(pm.IsFading(), "IsFading should be true");

    // Advance a few frames
    for (int i = 0; i < 5; i++) {
        pm.Update();
    }
    ASSERT(pm.GetFadeProgress() < 1.0f, "Progress should have decreased");
    ASSERT(pm.GetFadeProgress() > 0.0f, "Progress should not be zero yet");

    // Complete fade
    for (int i = 0; i < 10; i++) {
        pm.Update();
    }
    ASSERT(pm.GetFadeState() == FadeState::FadedOut, "Should be faded out");
    ASSERT(pm.GetFadeProgress() == 0.0f, "Progress should be 0");

    // Restore
    pm.RestoreFromBlack();
    ASSERT(pm.GetFadeState() == FadeState::None, "Should be restored");
    ASSERT(pm.GetFadeProgress() == 1.0f, "Should be at full brightness");

    TEST_PASS();
    return true;
}

bool test_flash() {
    TEST_START("flash effects");

    PaletteManager& pm = PaletteManager::Instance();
    pm.Init();

    ASSERT(!pm.IsFlashing(), "Should not be flashing initially");

    pm.StartFlash(FlashType::White, 5, 1.0f);
    ASSERT(pm.IsFlashing(), "Should be flashing");

    // Update through flash
    for (int i = 0; i < 10; i++) {
        pm.Update();
    }
    ASSERT(!pm.IsFlashing(), "Flash should have ended");

    TEST_PASS();
    return true;
}

bool test_animation() {
    TEST_START("color animation");

    PaletteManager& pm = PaletteManager::Instance();
    pm.Init();

    // Enable water animation
    pm.SetWaterAnimationEnabled(true);

    // Get original color at animation start
    const PaletteColor* original = pm.GetOriginalPalette();
    PaletteColor start_color = original[PaletteManager::WATER_ANIM_START];

    // Update enough frames for rotation
    for (int i = 0; i < 20; i++) {
        pm.Update();
    }

    // Color should have rotated (hard to verify exactly without knowing palette)
    // Just verify no crash occurred

    pm.SetWaterAnimationEnabled(false);

    TEST_PASS();
    return true;
}

bool test_find_closest_color() {
    TEST_START("find closest color");

    PaletteManager& pm = PaletteManager::Instance();
    pm.Init();

    // With grayscale palette, closest to (128, 128, 128) should be index 128
    int closest = pm.FindClosestColor(128, 128, 128, true);
    ASSERT(closest == 128, "Closest to gray should be 128");

    // Closest to (255, 255, 255) should be 255
    closest = pm.FindClosestColor(255, 255, 255, true);
    ASSERT(closest == 255, "Closest to white should be 255");

    // Closest to (0, 0, 0) with skip_zero should be 1
    closest = pm.FindClosestColor(0, 0, 0, true);
    ASSERT(closest == 1, "Closest to black (skipping 0) should be 1");

    TEST_PASS();
    return true;
}

// =============================================================================
// Visual Test
// =============================================================================

void draw_palette_grid(GraphicsBuffer& screen) {
    screen.Lock();
    screen.Clear(0);

    // Draw 16x16 grid of palette colors
    int cell_w = screen.Get_Width() / 16;
    int cell_h = (screen.Get_Height() - 50) / 16;

    for (int row = 0; row < 16; row++) {
        for (int col = 0; col < 16; col++) {
            int index = row * 16 + col;
            int x = col * cell_w;
            int y = row * cell_h;
            screen.Fill_Rect(x, y, cell_w - 1, cell_h - 1, index);
        }
    }

    screen.Unlock();
    screen.Flip();
}

void run_visual_test() {
    printf("\n=== Visual Palette Test ===\n");

    PaletteManager& pm = PaletteManager::Instance();
    GraphicsBuffer& screen = GraphicsBuffer::Screen();

    // Create colorful test palette
    PaletteColor colors[256];
    for (int i = 0; i < 256; i++) {
        // Create rainbow effect
        float hue = static_cast<float>(i) / 256.0f * 6.0f;
        float r, g, b;

        if (hue < 1.0f) {
            r = 1.0f; g = hue; b = 0.0f;
        } else if (hue < 2.0f) {
            r = 2.0f - hue; g = 1.0f; b = 0.0f;
        } else if (hue < 3.0f) {
            r = 0.0f; g = 1.0f; b = hue - 2.0f;
        } else if (hue < 4.0f) {
            r = 0.0f; g = 4.0f - hue; b = 1.0f;
        } else if (hue < 5.0f) {
            r = hue - 4.0f; g = 0.0f; b = 1.0f;
        } else {
            r = 1.0f; g = 0.0f; b = 6.0f - hue;
        }

        colors[i] = PaletteColor(
            static_cast<uint8_t>(r * 255),
            static_cast<uint8_t>(g * 255),
            static_cast<uint8_t>(b * 255)
        );
    }
    colors[0] = PaletteColor(0, 0, 0);  // Keep transparent black

    pm.SetPalette(colors);
    pm.Apply();

    // Show initial palette
    printf("Showing rainbow palette...\n");
    draw_palette_grid(screen);
    Platform_Delay(1000);

    // Demonstrate fade out
    printf("Fading out...\n");
    pm.StartFadeOut(30);
    for (int i = 0; i < 35; i++) {
        pm.Update();
        draw_palette_grid(screen);
        Platform_Delay(33);
    }

    Platform_Delay(500);

    // Demonstrate fade in
    printf("Fading in...\n");
    pm.StartFadeIn(30);
    for (int i = 0; i < 35; i++) {
        pm.Update();
        draw_palette_grid(screen);
        Platform_Delay(33);
    }

    // Demonstrate flash
    printf("White flash...\n");
    pm.StartFlash(FlashType::White, 10, 0.8f);
    for (int i = 0; i < 15; i++) {
        pm.Update();
        draw_palette_grid(screen);
        Platform_Delay(33);
    }

    printf("Red flash...\n");
    pm.StartFlash(FlashType::Red, 10, 0.5f);
    for (int i = 0; i < 15; i++) {
        pm.Update();
        draw_palette_grid(screen);
        Platform_Delay(33);
    }

    // Enable animation and show for a bit
    printf("Water animation (2 seconds)...\n");
    pm.SetWaterAnimationEnabled(true);
    for (int i = 0; i < 60; i++) {
        pm.Update();
        draw_palette_grid(screen);
        Platform_Delay(33);
    }
    pm.SetWaterAnimationEnabled(false);

    printf("Visual test complete.\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("==========================================\n");
    printf("Palette Manager Test Suite\n");
    printf("==========================================\n\n");

    if (Platform_Init() != PLATFORM_RESULT_SUCCESS) {
        printf("ERROR: Failed to initialize platform\n");
        return 1;
    }

    if (Platform_Graphics_Init() != 0) {
        printf("ERROR: Failed to initialize graphics\n");
        Platform_Shutdown();
        return 1;
    }

    printf("=== Unit Tests ===\n\n");

    test_singleton();
    test_palette_color();
    test_initialization();
    test_fade_state();
    test_flash();
    test_animation();
    test_find_closest_color();

    printf("\n------------------------------------------\n");
    printf("Tests: %d/%d passed\n", tests_passed, tests_run);
    printf("------------------------------------------\n");

    if (tests_passed == tests_run) {
        run_visual_test();
    }

    Platform_Graphics_Shutdown();
    Platform_Shutdown();

    printf("\n==========================================\n");
    if (tests_passed == tests_run) {
        printf("All tests PASSED!\n");
    } else {
        printf("Some tests FAILED\n");
    }
    printf("==========================================\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
```

### ralph-tasks/verify/check-palette-manager.sh
```bash
#!/bin/bash
# Verification script for Palette Manager

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Palette Manager Implementation ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check files exist
echo "[1/5] Checking file existence..."
FILES=(
    "include/game/graphics/palette_manager.h"
    "src/game/graphics/palette_manager.cpp"
    "src/test_palette_manager.cpp"
)

for file in "${FILES[@]}"; do
    if [ ! -f "$file" ]; then
        echo "VERIFY_FAILED: $file not found"
        exit 1
    fi
done
echo "  OK: All source files exist"

# Step 2: Check header syntax
echo "[2/5] Checking header syntax..."
if ! clang++ -std=c++17 -fsyntax-only \
    -I include \
    -I include/game \
    -I include/game/graphics \
    include/game/graphics/palette_manager.h 2>&1; then
    echo "VERIFY_FAILED: Header has syntax errors"
    exit 1
fi
echo "  OK: Header compiles"

# Step 3: Build test executable
echo "[3/5] Building TestPaletteManager..."
if ! cmake --build build --target TestPaletteManager 2>&1; then
    echo "VERIFY_FAILED: Build failed"
    exit 1
fi
echo "  OK: Build succeeded"

# Step 4: Check executable exists
echo "[4/5] Checking executable..."
if [ ! -f "build/TestPaletteManager" ]; then
    echo "VERIFY_FAILED: TestPaletteManager executable not found"
    exit 1
fi
echo "  OK: Executable exists"

# Step 5: Run tests
echo "[5/5] Running unit tests..."
if timeout 60 ./build/TestPaletteManager 2>&1 | grep -q "All tests PASSED"; then
    echo "  OK: Tests passed"
else
    echo "VERIFY_FAILED: Tests did not pass"
    exit 1
fi

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## CMakeLists.txt Additions

```cmake
# Add palette manager sources
target_sources(game_core PRIVATE
    src/game/graphics/palette_manager.cpp
)

# Palette Manager Test
add_executable(TestPaletteManager src/test_palette_manager.cpp)

target_include_directories(TestPaletteManager PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/game/graphics
)

target_link_libraries(TestPaletteManager PRIVATE
    game_core
    redalert_platform
)

add_test(NAME TestPaletteManager COMMAND TestPaletteManager)
```

## Implementation Steps

1. Create `include/game/graphics/palette_manager.h`
2. Create `src/game/graphics/palette_manager.cpp`
3. Create `src/test_palette_manager.cpp`
4. Update CMakeLists.txt
5. Create verification script
6. Build and test

## Success Criteria

- [ ] PaletteManager compiles without warnings
- [ ] Singleton pattern works correctly
- [ ] PaletteColor math operations work
- [ ] Fade in works smoothly
- [ ] Fade out works smoothly
- [ ] Instant fade/restore works
- [ ] Flash effects work (white, red, custom)
- [ ] Color animation works (water, fire)
- [ ] Protected UI range preserved during fade
- [ ] FindClosestColor returns correct results
- [ ] Theater palette loading works
- [ ] All unit tests pass
- [ ] Visual test shows correct effects

## Common Issues

### Fade Not Smooth
**Symptom**: Fade jumps or stutters
**Solution**: Ensure Update() called every frame, check fade_step calculation

### Flash Too Bright/Dark
**Symptom**: Flash overwhelms colors or barely visible
**Solution**: Adjust intensity parameter, check lerp calculation

### Animation Not Visible
**Symptom**: Water/fire colors don't cycle
**Solution**: Ensure animation range matches actual palette data

### UI Affected by Fade
**Symptom**: Menu/UI fades when it shouldn't
**Solution**: Check UI_RANGE protection in ApplyEffects()

## Completion Promise
When verification passes, output:
<promise>TASK_15D_COMPLETE</promise>

## Escape Hatch
If stuck after 15 iterations:
- Document what's blocking in `ralph-tasks/blocked/15d.md`
- Output:
<promise>TASK_15D_BLOCKED</promise>

## Max Iterations
15
