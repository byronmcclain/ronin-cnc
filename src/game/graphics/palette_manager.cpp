/**
 * PaletteManager Implementation
 */

#include "game/graphics/palette_manager.h"
#include "game/graphics/tile_renderer.h"  // For TheaterType
#include "platform.h"
#include <cstring>
#include <algorithm>

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
        original_[i] = PaletteColor(static_cast<uint8_t>(i),
                                     static_cast<uint8_t>(i),
                                     static_cast<uint8_t>(i));
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
            r = static_cast<uint8_t>(r << 2);
            g = static_cast<uint8_t>(g << 2);
            b = static_cast<uint8_t>(b << 2);
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
    (void)house;
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
