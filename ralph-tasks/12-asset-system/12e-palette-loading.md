# Task 12e: Palette Loading

## Dependencies
- Task 12d (MIX Manager) must be complete
- Platform graphics palette functions available

## Context
Red Alert uses 256-color indexed palettes. All graphics in the game reference these palette indices rather than direct RGB values. This task implements palette loading from MIX files and integration with the platform graphics layer.

## Objective
Create a Palette class and PaletteManager that:
1. Loads .PAL files from MIX archives
2. Converts 6-bit VGA values (0-63) to 8-bit RGB (0-255)
3. Applies palettes to the platform graphics layer
4. Supports palette effects (fade, flash, cycling)

## Technical Background

### PAL File Format
```
Size: 768 bytes exactly (256 colors * 3 bytes RGB)
Format: R, G, B for each of 256 colors
Range: 0-63 (VGA 6-bit) - must multiply by 4 for 8-bit

Offset  Size  Description
------  ----  -----------
0x000   1     Color 0 Red (0-63)
0x001   1     Color 0 Green (0-63)
0x002   1     Color 0 Blue (0-63)
0x003   1     Color 1 Red
...
0x2FD   1     Color 255 Red
0x2FE   1     Color 255 Green
0x2FF   1     Color 255 Blue
```

### Known Palette Files
| Filename | Purpose |
|----------|---------|
| TEMPERAT.PAL | Temperate theater |
| SNOW.PAL | Snow theater |
| DESERT.PAL | Desert theater |
| INTERIOR.PAL | Interior/Base theater |
| JUNGLE.PAL | Jungle theater |
| PALETTE.PAL | Generic/fallback |

### Palette Color Indices (Red Alert conventions)
| Index | Purpose |
|-------|---------|
| 0 | Transparent (usually black) |
| 1-15 | House colors (remappable) |
| 16-175 | General colors |
| 176-191 | Cycling colors (animation) |
| 192-207 | Mouse cursor |
| 208-223 | Shadow colors |
| 224-239 | Gold/amber tints |
| 240-254 | Fire/explosion |
| 255 | White/flash |

## Deliverables
- [ ] `include/game/palette.h` - Palette structures and manager
- [ ] `src/game/palette.cpp` - Implementation
- [ ] Color remapping table support
- [ ] Palette effects (fade in/out)
- [ ] Test program with visual output
- [ ] Verification script

## Files to Create

### include/game/palette.h (NEW)
```cpp
#ifndef PALETTE_H
#define PALETTE_H

#include <cstdint>
#include <string>
#include "platform.h"

/// Number of colors in a palette
constexpr int PALETTE_SIZE = 256;

/// Size of palette data in bytes (256 * RGB)
constexpr int PALETTE_BYTES = 768;

/// Single palette color
struct PaletteColor {
    uint8_t r, g, b;

    PaletteColor() : r(0), g(0), b(0) {}
    PaletteColor(uint8_t red, uint8_t green, uint8_t blue)
        : r(red), g(green), b(blue) {}

    /// Interpolate between two colors
    static PaletteColor Lerp(const PaletteColor& a, const PaletteColor& b, float t);
};

/// 256-color palette
class Palette {
public:
    Palette();

    /// Load from raw 768-byte data (6-bit VGA values)
    /// @param data Pointer to 768 bytes of palette data
    /// @param convert_6bit If true, multiply values by 4 (VGA to RGB)
    void LoadFromData(const uint8_t* data, bool convert_6bit = true);

    /// Load from MIX file via manager
    /// @param filename Palette filename (e.g., "TEMPERAT.PAL")
    /// @return true if loaded successfully
    bool LoadFromMix(const char* filename);

    /// Get color at index
    const PaletteColor& GetColor(int index) const;

    /// Set color at index
    void SetColor(int index, const PaletteColor& color);
    void SetColor(int index, uint8_t r, uint8_t g, uint8_t b);

    /// Get raw palette data for platform layer (RGB888)
    /// @param out_data Buffer of at least 768 bytes
    void GetRawRGB(uint8_t* out_data) const;

    /// Apply this palette to the platform graphics layer
    void Apply() const;

    /// Clear palette to black
    void Clear();

    /// Create grayscale palette
    void MakeGrayscale();

    /// Copy from another palette
    void CopyFrom(const Palette& other);

    /// Interpolate between two palettes
    static void Lerp(const Palette& a, const Palette& b, float t, Palette& result);

private:
    PaletteColor colors_[PALETTE_SIZE];
};

/// Color remapping table (256 bytes)
/// Maps source color indices to destination color indices
class RemapTable {
public:
    RemapTable();

    /// Create identity remap (no change)
    void MakeIdentity();

    /// Create house color remap
    /// @param house_colors Array of 16 color indices for house colors
    void MakeHouseRemap(const uint8_t* house_colors);

    /// Create shadow remap (darken colors)
    void MakeShadow(const Palette& palette);

    /// Get remapped color index
    uint8_t operator[](int index) const { return table_[index & 0xFF]; }

    /// Set remap for specific index
    void Set(int index, uint8_t remap) { table_[index & 0xFF] = remap; }

    /// Get raw table data
    const uint8_t* GetData() const { return table_; }
    uint8_t* GetData() { return table_; }

private:
    uint8_t table_[256];
};

/// Global palette manager
class PaletteManager {
public:
    /// Get singleton instance
    static PaletteManager& Instance();

    /// Load and apply a palette from MIX
    /// @param filename Palette filename
    /// @return true if loaded and applied
    bool LoadPalette(const char* filename);

    /// Get current active palette
    const Palette& GetCurrent() const { return current_; }
    Palette& GetCurrent() { return current_; }

    /// Get original (unmodified) palette
    const Palette& GetOriginal() const { return original_; }

    /// Apply current palette to graphics
    void Apply();

    /// Save current palette as original (for effects)
    void SaveOriginal();

    /// Restore original palette
    void RestoreOriginal();

    // === Palette Effects ===

    /// Fade to black over specified frames
    /// Call once per frame until returns true (complete)
    bool FadeOut(int total_frames);

    /// Fade from black to current palette
    /// Call once per frame until returns true (complete)
    bool FadeIn(int total_frames);

    /// Flash to color and back
    /// @param r, g, b Flash color
    /// @param duration_frames Total duration
    void Flash(uint8_t r, uint8_t g, uint8_t b, int duration_frames);

    /// Update effects (call each frame)
    /// @return true if effect is active
    bool UpdateEffects();

    /// Check if effect is running
    bool IsEffectActive() const { return effect_active_; }

    /// Cancel any active effect
    void CancelEffect();

private:
    PaletteManager();

    Palette current_;        // Active palette
    Palette original_;       // Saved for effects
    Palette effect_start_;   // Effect start state
    Palette effect_end_;     // Effect end state

    bool effect_active_;
    int effect_frame_;
    int effect_total_frames_;
    enum EffectType { EFFECT_NONE, EFFECT_FADE_OUT, EFFECT_FADE_IN, EFFECT_FLASH };
    EffectType effect_type_;
};

/// Convenience macro
#define ThePaletteManager PaletteManager::Instance()

#endif // PALETTE_H
```

### src/game/palette.cpp (NEW)
```cpp
#include "game/palette.h"
#include "game/mix_manager.h"
#include <cstring>
#include <algorithm>
#include <cstdio>

// === PaletteColor ===

PaletteColor PaletteColor::Lerp(const PaletteColor& a, const PaletteColor& b, float t) {
    t = std::max(0.0f, std::min(1.0f, t));
    return PaletteColor(
        static_cast<uint8_t>(a.r + (b.r - a.r) * t),
        static_cast<uint8_t>(a.g + (b.g - a.g) * t),
        static_cast<uint8_t>(a.b + (b.b - a.b) * t)
    );
}

// === Palette ===

Palette::Palette() {
    Clear();
}

void Palette::LoadFromData(const uint8_t* data, bool convert_6bit) {
    if (!data) return;

    for (int i = 0; i < PALETTE_SIZE; i++) {
        uint8_t r = data[i * 3 + 0];
        uint8_t g = data[i * 3 + 1];
        uint8_t b = data[i * 3 + 2];

        if (convert_6bit) {
            // VGA 6-bit (0-63) to 8-bit (0-255)
            // Multiply by 4, but also copy top bits to bottom for full range
            r = (r << 2) | (r >> 4);
            g = (g << 2) | (g >> 4);
            b = (b << 2) | (b >> 4);
        }

        colors_[i] = PaletteColor(r, g, b);
    }
}

bool Palette::LoadFromMix(const char* filename) {
    uint32_t size;
    uint8_t* data = MixManager::Instance().LoadFile(filename, &size);

    if (!data) {
        fprintf(stderr, "Palette: Failed to load '%s'\n", filename);
        return false;
    }

    if (size < PALETTE_BYTES) {
        fprintf(stderr, "Palette: '%s' too small (%u bytes, need %d)\n",
                filename, size, PALETTE_BYTES);
        delete[] data;
        return false;
    }

    LoadFromData(data, true);  // Always convert from 6-bit
    delete[] data;

    printf("Palette: Loaded '%s'\n", filename);
    return true;
}

const PaletteColor& Palette::GetColor(int index) const {
    return colors_[index & 0xFF];
}

void Palette::SetColor(int index, const PaletteColor& color) {
    colors_[index & 0xFF] = color;
}

void Palette::SetColor(int index, uint8_t r, uint8_t g, uint8_t b) {
    colors_[index & 0xFF] = PaletteColor(r, g, b);
}

void Palette::GetRawRGB(uint8_t* out_data) const {
    for (int i = 0; i < PALETTE_SIZE; i++) {
        out_data[i * 3 + 0] = colors_[i].r;
        out_data[i * 3 + 1] = colors_[i].g;
        out_data[i * 3 + 2] = colors_[i].b;
    }
}

void Palette::Apply() const {
    uint8_t raw[PALETTE_BYTES];
    GetRawRGB(raw);
    Platform_Graphics_SetPalette(raw);
}

void Palette::Clear() {
    for (int i = 0; i < PALETTE_SIZE; i++) {
        colors_[i] = PaletteColor(0, 0, 0);
    }
}

void Palette::MakeGrayscale() {
    for (int i = 0; i < PALETTE_SIZE; i++) {
        uint8_t gray = static_cast<uint8_t>(i);
        colors_[i] = PaletteColor(gray, gray, gray);
    }
}

void Palette::CopyFrom(const Palette& other) {
    memcpy(colors_, other.colors_, sizeof(colors_));
}

void Palette::Lerp(const Palette& a, const Palette& b, float t, Palette& result) {
    for (int i = 0; i < PALETTE_SIZE; i++) {
        result.colors_[i] = PaletteColor::Lerp(a.colors_[i], b.colors_[i], t);
    }
}

// === RemapTable ===

RemapTable::RemapTable() {
    MakeIdentity();
}

void RemapTable::MakeIdentity() {
    for (int i = 0; i < 256; i++) {
        table_[i] = static_cast<uint8_t>(i);
    }
}

void RemapTable::MakeHouseRemap(const uint8_t* house_colors) {
    MakeIdentity();
    if (!house_colors) return;

    // Remap indices 1-15 (house colors)
    for (int i = 0; i < 16; i++) {
        table_[i] = house_colors[i];
    }
}

void RemapTable::MakeShadow(const Palette& palette) {
    // Map each color to a darker version
    // Find closest color at ~50% brightness
    for (int i = 0; i < 256; i++) {
        const PaletteColor& src = palette.GetColor(i);

        // Target brightness (halved)
        int target_r = src.r / 2;
        int target_g = src.g / 2;
        int target_b = src.b / 2;

        // Find closest color in palette
        int best_idx = 0;
        int best_dist = INT32_MAX;

        for (int j = 0; j < 256; j++) {
            const PaletteColor& dst = palette.GetColor(j);
            int dr = dst.r - target_r;
            int dg = dst.g - target_g;
            int db = dst.b - target_b;
            int dist = dr*dr + dg*dg + db*db;

            if (dist < best_dist) {
                best_dist = dist;
                best_idx = j;
            }
        }

        table_[i] = static_cast<uint8_t>(best_idx);
    }
}

// === PaletteManager ===

PaletteManager& PaletteManager::Instance() {
    static PaletteManager instance;
    return instance;
}

PaletteManager::PaletteManager()
    : effect_active_(false)
    , effect_frame_(0)
    , effect_total_frames_(0)
    , effect_type_(EFFECT_NONE)
{
}

bool PaletteManager::LoadPalette(const char* filename) {
    if (!current_.LoadFromMix(filename)) {
        return false;
    }
    SaveOriginal();
    Apply();
    return true;
}

void PaletteManager::Apply() {
    current_.Apply();
}

void PaletteManager::SaveOriginal() {
    original_.CopyFrom(current_);
}

void PaletteManager::RestoreOriginal() {
    current_.CopyFrom(original_);
    Apply();
}

bool PaletteManager::FadeOut(int total_frames) {
    if (!effect_active_) {
        effect_type_ = EFFECT_FADE_OUT;
        effect_start_.CopyFrom(current_);
        effect_end_.Clear();  // Black
        effect_frame_ = 0;
        effect_total_frames_ = total_frames;
        effect_active_ = true;
    }
    return UpdateEffects();
}

bool PaletteManager::FadeIn(int total_frames) {
    if (!effect_active_) {
        effect_type_ = EFFECT_FADE_IN;
        effect_start_.Clear();  // Start from black
        effect_end_.CopyFrom(original_);
        effect_frame_ = 0;
        effect_total_frames_ = total_frames;
        effect_active_ = true;
        current_.Clear();
        Apply();
    }
    return UpdateEffects();
}

void PaletteManager::Flash(uint8_t r, uint8_t g, uint8_t b, int duration_frames) {
    effect_type_ = EFFECT_FLASH;
    effect_start_.CopyFrom(current_);

    // Flash palette is all one color
    for (int i = 0; i < PALETTE_SIZE; i++) {
        effect_end_.SetColor(i, r, g, b);
    }

    effect_frame_ = 0;
    effect_total_frames_ = duration_frames;
    effect_active_ = true;
}

bool PaletteManager::UpdateEffects() {
    if (!effect_active_) {
        return false;
    }

    effect_frame_++;

    float t = static_cast<float>(effect_frame_) / effect_total_frames_;

    if (effect_type_ == EFFECT_FLASH) {
        // Flash: go to color then back
        if (t < 0.5f) {
            t = t * 2.0f;  // 0 to 1 for first half
        } else {
            t = 2.0f - t * 2.0f;  // 1 to 0 for second half
        }
        Palette::Lerp(effect_start_, effect_end_, t, current_);
    } else {
        // Linear interpolation for fade
        Palette::Lerp(effect_start_, effect_end_, t, current_);
    }

    Apply();

    if (effect_frame_ >= effect_total_frames_) {
        effect_active_ = false;
        effect_type_ = EFFECT_NONE;

        // Ensure final state
        if (effect_type_ == EFFECT_FADE_IN) {
            current_.CopyFrom(original_);
            Apply();
        }
        return true;  // Effect complete
    }

    return false;  // Effect still running
}

void PaletteManager::CancelEffect() {
    effect_active_ = false;
    effect_type_ = EFFECT_NONE;
    RestoreOriginal();
}
```

### src/test_palette.cpp (NEW)
```cpp
// Test palette loading and display
#include <cstdio>
#include "game/palette.h"
#include "game/mix_manager.h"
#include "platform.h"

void draw_palette_test(const Palette& palette) {
    // Draw color bars to screen
    uint8_t* pixels;
    int32_t width, height, pitch;

    Platform_Graphics_LockBackBuffer();
    Platform_Graphics_GetBackBuffer(&pixels, &width, &height, &pitch);

    // Draw 16x16 grid of colors (16 columns, 16 rows = 256 colors)
    int cell_w = width / 16;
    int cell_h = height / 16;

    for (int cy = 0; cy < 16; cy++) {
        for (int cx = 0; cx < 16; cx++) {
            int color_idx = cy * 16 + cx;
            int px = cx * cell_w;
            int py = cy * cell_h;

            // Fill cell with color index
            for (int y = 0; y < cell_h; y++) {
                for (int x = 0; x < cell_w; x++) {
                    if (py + y < height && px + x < width) {
                        pixels[(py + y) * pitch + (px + x)] = static_cast<uint8_t>(color_idx);
                    }
                }
            }
        }
    }

    Platform_Graphics_UnlockBackBuffer();
    Platform_Graphics_Flip();
}

int main(int argc, char* argv[]) {
    const char* palette_name = "TEMPERAT.PAL";
    if (argc > 1) {
        palette_name = argv[1];
    }

    printf("=== Palette Loading Test ===\n\n");

    // Initialize platform
    if (Platform_Init() != 0) {
        fprintf(stderr, "Platform init failed\n");
        return 1;
    }

    if (Platform_Graphics_Init() != 0) {
        fprintf(stderr, "Graphics init failed\n");
        Platform_Shutdown();
        return 1;
    }

    // Initialize MIX manager
    MixManager& mgr = MixManager::Instance();
    if (!mgr.Initialize("gamedata")) {
        fprintf(stderr, "MixManager init failed\n");
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return 1;
    }

    // Mount REDALERT.MIX (contains palettes)
    mgr.Mount("REDALERT.MIX");

    // Test 1: Load palette
    printf("Test 1: Load palette '%s'...\n", palette_name);
    PaletteManager& pm = PaletteManager::Instance();
    if (!pm.LoadPalette(palette_name)) {
        fprintf(stderr, "  FAIL: Could not load palette\n");
        mgr.Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return 1;
    }
    printf("  PASS: Palette loaded\n");

    // Test 2: Apply and draw
    printf("Test 2: Draw palette test pattern...\n");
    draw_palette_test(pm.GetCurrent());
    printf("  PASS: Pattern drawn\n");

    // Wait a moment
    Platform_Timer_Delay(1000);

    // Test 3: Color inspection
    printf("\nTest 3: Sample colors:\n");
    const Palette& pal = pm.GetCurrent();
    const int sample_indices[] = {0, 1, 16, 128, 200, 255};
    for (int idx : sample_indices) {
        const PaletteColor& c = pal.GetColor(idx);
        printf("  Color %3d: R=%3d G=%3d B=%3d\n", idx, c.r, c.g, c.b);
    }

    // Test 4: Fade out
    printf("\nTest 4: Fade out effect...\n");
    while (!pm.FadeOut(30)) {
        Platform_Timer_Delay(33);  // ~30 FPS
    }
    printf("  PASS: Fade out complete\n");

    Platform_Timer_Delay(500);

    // Test 5: Fade in
    printf("Test 5: Fade in effect...\n");
    while (!pm.FadeIn(30)) {
        Platform_Timer_Delay(33);
    }
    printf("  PASS: Fade in complete\n");

    Platform_Timer_Delay(1000);

    // Cleanup
    mgr.Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();

    printf("\n=== Palette Test Complete ===\n");
    return 0;
}
```

### CMakeLists.txt (MODIFY)
```cmake
# Add palette to game_mix library (or create new game_assets library)
add_library(game_assets STATIC
    src/game/palette.cpp
)
target_include_directories(game_assets PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(game_assets PUBLIC game_mix)

# Palette test
add_executable(TestPalette src/test_palette.cpp)
target_link_libraries(TestPalette PRIVATE game_assets)
```

## Verification Command
```bash
ralph-tasks/verify/check-palette.sh
```

## Verification Script

### ralph-tasks/verify/check-palette.sh (NEW)
```bash
#!/bin/bash
# Verification script for palette loading

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Palette Loading ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check source files
echo "[1/4] Checking source files..."
if [ ! -f "include/game/palette.h" ]; then
    echo "VERIFY_FAILED: palette.h not found"
    exit 1
fi
if [ ! -f "src/game/palette.cpp" ]; then
    echo "VERIFY_FAILED: palette.cpp not found"
    exit 1
fi
echo "  OK: Source files exist"

# Step 2: Build
echo "[2/4] Building..."
if ! cmake --build build --target TestPalette 2>&1; then
    echo "VERIFY_FAILED: Build failed"
    exit 1
fi
echo "  OK: Build successful"

# Step 3: Check for palette file
echo "[3/4] Checking for palette files..."
if [ ! -f "gamedata/REDALERT.MIX" ]; then
    echo "VERIFY_WARNING: REDALERT.MIX not found"
    echo "VERIFY_SUCCESS (build only)"
    exit 0
fi
echo "  OK: REDALERT.MIX exists"

# Step 4: Run test (with timeout for visual test)
echo "[4/4] Running palette test..."
# Use timeout to prevent hanging on visual display
if ! timeout 30 ./build/TestPalette TEMPERAT.PAL 2>&1; then
    echo "VERIFY_FAILED: Palette test failed"
    exit 1
fi
echo "  OK: Palette test passed"

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Create `include/game/palette.h`
2. Create `src/game/palette.cpp`
3. Update CMakeLists.txt with game_assets library
4. Create `src/test_palette.cpp`
5. Build: `cmake --build build --target TestPalette`
6. Test: `./build/TestPalette TEMPERAT.PAL`
7. Verify visual output shows color grid
8. Run verification script

## Success Criteria
- Palette loads from MIX file
- 6-bit to 8-bit conversion works correctly
- Color index 0 is black (transparent)
- Platform palette is set correctly
- Visual test shows 16x16 color grid
- Fade in/out effects work smoothly

## Common Issues

### Colors look wrong/washed out
- 6-bit conversion: multiply by 4 AND copy top bits to bottom
- `(value << 2) | (value >> 4)` gives full range

### Palette not applied
- Call `Apply()` after loading
- Verify `Platform_Graphics_SetPalette` is called

### Fade effects are jerky
- Ensure delay between frames
- Use floating point for interpolation

## Completion Promise
When verification passes, output:
```
<promise>TASK_12E_COMPLETE</promise>
```

## Escape Hatch
If stuck after 15 iterations:
- Document in `ralph-tasks/blocked/12e.md`
- Output: `<promise>TASK_12E_BLOCKED</promise>`

## Max Iterations
15
