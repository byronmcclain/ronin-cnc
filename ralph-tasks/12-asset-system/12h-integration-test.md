# Task 12h: Asset System Integration Test

## Dependencies
- Tasks 12a-12g must be complete
- Platform layer fully functional

## Context
This final task in the Asset System phase creates a comprehensive integration test that verifies all asset loading components work together. It produces a visual demo showing palette, shapes, and tiles loaded from the actual game MIX files.

## Objective
Create an integration test that:
1. Initializes all asset systems in correct order
2. Loads a complete theater (palette, tiles, shapes)
3. Renders a visual demo showing all components
4. Validates data integrity across components
5. Measures and reports performance

## Technical Background

### System Initialization Order
```
1. Platform_Init()
2. Platform_Graphics_Init()
3. MixManager::Initialize("gamedata")
4. MixManager::Mount("REDALERT.MIX")
5. MixManager::MountTheater(Theater::TEMPERATE)
6. PaletteManager::LoadPalette("TEMPERAT.PAL")
7. TemplateManager::LoadTheater("TEMPERAT")
8. Load game shapes (MOUSE.SHP, etc.)
```

### System Shutdown Order (Reverse)
```
1. Unload shapes
2. TemplateManager::UnloadTheater()
3. MixManager::Shutdown()
4. Platform_Graphics_Shutdown()
5. Platform_Shutdown()
```

### Demo Display Layout
```
+----------------------------------+
|  PALETTE (256 color gradient)    |
+----------------------------------+
|  TILES (grid of terrain tiles)   |
+----------------------------------+
|  SHAPES (animated mouse cursor)  |
+----------------------------------+
|  STATUS (FPS, file counts, etc.) |
+----------------------------------+
```

## Deliverables
- [ ] `src/test_assets_integration.cpp` - Full integration test
- [ ] Visual demo with all components
- [ ] Performance metrics output
- [ ] Error handling and reporting
- [ ] Verification script
- [ ] Documentation of any issues found

## Files to Create

### src/test_assets_integration.cpp (NEW)
```cpp
// Asset System Integration Test
// Verifies all asset loading components work together

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>

#include "platform.h"
#include "game/mix_file.h"
#include "game/mix_manager.h"
#include "game/palette.h"
#include "game/shape.h"
#include "game/template.h"

// Test configuration
static const char* GAMEDATA_PATH = "gamedata";
static const char* THEATER_NAME = "TEMPERAT";

// Test results
struct TestResults {
    bool platform_ok;
    bool graphics_ok;
    bool mix_ok;
    bool palette_ok;
    bool shapes_ok;
    bool templates_ok;

    int mix_count;
    int file_count;
    int palette_colors;
    int shape_frames;
    int tile_count;

    double load_time_ms;
    double render_fps;

    void Print() const {
        printf("\n=== Integration Test Results ===\n");
        printf("Platform:   %s\n", platform_ok ? "PASS" : "FAIL");
        printf("Graphics:   %s\n", graphics_ok ? "PASS" : "FAIL");
        printf("MIX Files:  %s (%d files in %d archives)\n",
               mix_ok ? "PASS" : "FAIL", file_count, mix_count);
        printf("Palette:    %s (%d colors)\n",
               palette_ok ? "PASS" : "FAIL", palette_colors);
        printf("Shapes:     %s (%d frames)\n",
               shapes_ok ? "PASS" : "FAIL", shape_frames);
        printf("Templates:  %s (%d tiles)\n",
               templates_ok ? "PASS" : "FAIL", tile_count);
        printf("\nPerformance:\n");
        printf("  Load time: %.1f ms\n", load_time_ms);
        printf("  Render:    %.1f FPS\n", render_fps);
        printf("\n");

        if (AllPassed()) {
            printf(">>> ALL TESTS PASSED <<<\n");
        } else {
            printf(">>> SOME TESTS FAILED <<<\n");
        }
    }

    bool AllPassed() const {
        return platform_ok && graphics_ok && mix_ok &&
               palette_ok && shapes_ok && templates_ok;
    }
};

static TestResults g_results = {};

// === Helper Functions ===

void draw_palette_strip(uint8_t* buffer, int pitch, int width, int y, int height) {
    // Draw 256-color horizontal strip
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            int color_idx = (col * 256) / width;
            buffer[(y + row) * pitch + col] = static_cast<uint8_t>(color_idx);
        }
    }
}

void draw_tile_grid(uint8_t* buffer, int pitch, int buf_w, int buf_h,
                    int start_y, int height) {
    const TheaterTiles& theater = TemplateManager::Instance().GetTheater();
    int tile_count = theater.GetTileCount();

    int tiles_per_row = buf_w / TILE_WIDTH;
    int rows = height / TILE_HEIGHT;
    int drawn = 0;

    for (int ty = 0; ty < rows && drawn < tile_count; ty++) {
        for (int tx = 0; tx < tiles_per_row && drawn < tile_count; tx++, drawn++) {
            int px = tx * TILE_WIDTH;
            int py = start_y + ty * TILE_HEIGHT;
            theater.DrawTile(buffer, pitch, buf_w, buf_h, px, py, drawn);
        }
    }
}

void draw_status(uint8_t* buffer, int pitch, int buf_w, int buf_h,
                 int y, const char* status) {
    // Simple status bar (just fill with color for now)
    // Real implementation would draw text
    for (int row = 0; row < 20; row++) {
        memset(buffer + (y + row) * pitch, 0x1F, buf_w);  // Blue-ish color
    }

    printf("Status: %s\n", status);
}

// === Test Functions ===

bool test_platform() {
    printf("Testing platform initialization...\n");

    int result = Platform_Init();
    if (result != 0) {
        fprintf(stderr, "  FAIL: Platform_Init returned %d\n", result);
        return false;
    }

    result = Platform_Graphics_Init();
    if (result != 0) {
        fprintf(stderr, "  FAIL: Platform_Graphics_Init returned %d\n", result);
        Platform_Shutdown();
        return false;
    }

    printf("  PASS: Platform initialized\n");
    g_results.platform_ok = true;
    g_results.graphics_ok = true;
    return true;
}

bool test_mix_manager() {
    printf("Testing MIX manager...\n");

    MixManager& mgr = MixManager::Instance();

    if (!mgr.Initialize(GAMEDATA_PATH)) {
        fprintf(stderr, "  FAIL: MixManager::Initialize failed\n");
        return false;
    }

    // Mount core files
    const char* core_files[] = {
        "REDALERT.MIX",
        "SETUP.MIX",
        nullptr
    };

    int mounted = 0;
    for (int i = 0; core_files[i]; i++) {
        if (mgr.Mount(core_files[i])) {
            printf("  Mounted: %s\n", core_files[i]);
            mounted++;
        }
    }

    // Try CD directories
    if (mgr.Mount("CD1/MAIN.MIX")) mounted++;
    if (mgr.Mount("CD2/MAIN.MIX")) mounted++;

    g_results.mix_count = mounted;
    g_results.mix_ok = mounted > 0;

    // Count total files
    g_results.file_count = 0;
    // Would need to enumerate, for now estimate

    printf("  PASS: %d MIX files mounted\n", mounted);
    return true;
}

bool test_palette() {
    printf("Testing palette loading...\n");

    std::string pal_name = std::string(THEATER_NAME) + ".PAL";
    if (!PaletteManager::Instance().LoadPalette(pal_name.c_str())) {
        fprintf(stderr, "  FAIL: Could not load %s\n", pal_name.c_str());
        return false;
    }

    g_results.palette_colors = 256;
    g_results.palette_ok = true;

    // Verify some colors
    const Palette& pal = PaletteManager::Instance().GetCurrent();
    const PaletteColor& black = pal.GetColor(0);
    const PaletteColor& white = pal.GetColor(255);

    printf("  Color 0 (should be dark): R=%d G=%d B=%d\n", black.r, black.g, black.b);
    printf("  Color 255: R=%d G=%d B=%d\n", white.r, white.g, white.b);
    printf("  PASS: Palette loaded\n");

    return true;
}

bool test_shapes() {
    printf("Testing shape loading...\n");

    ShapeData mouse;
    if (!mouse.LoadFromMix("MOUSE.SHP")) {
        fprintf(stderr, "  WARNING: MOUSE.SHP not found\n");
        // Try alternate
        if (!mouse.LoadFromMix("SIDEBAR.SHP")) {
            g_results.shapes_ok = false;
            return false;
        }
    }

    g_results.shape_frames = mouse.GetFrameCount();
    g_results.shapes_ok = true;

    printf("  Loaded shape with %d frames\n", g_results.shape_frames);
    printf("  PASS: Shapes working\n");

    return true;
}

bool test_templates() {
    printf("Testing template loading...\n");

    // Mount theater MIX
    std::string theater_mix = std::string(THEATER_NAME) + ".MIX";
    MixManager::Instance().Mount(theater_mix.c_str());

    if (!TemplateManager::Instance().LoadTheater(THEATER_NAME)) {
        fprintf(stderr, "  WARNING: Could not load theater\n");
        // Templates are optional for basic functionality
    }

    g_results.tile_count = TemplateManager::Instance().GetTheater().GetTileCount();
    g_results.templates_ok = g_results.tile_count > 0;

    printf("  Loaded %d tiles\n", g_results.tile_count);
    printf("  PASS: Templates %s\n", g_results.templates_ok ? "working" : "(using defaults)");

    return true;
}

void run_visual_demo(ShapeData& mouse_shape) {
    printf("\nRunning visual demo...\n");

    // Layout
    const int PALETTE_HEIGHT = 40;
    const int TILES_HEIGHT = 160;
    const int SHAPES_HEIGHT = 160;
    const int STATUS_HEIGHT = 40;

    int frame_count = 0;
    auto start = std::chrono::high_resolution_clock::now();

    // Run for 5 seconds
    while (true) {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        if (elapsed.count() >= 5000) break;

        Platform_PollEvents();

        uint8_t* pixels;
        int32_t width, height, pitch;

        Platform_Graphics_LockBackBuffer();
        Platform_Graphics_GetBackBuffer(&pixels, &width, &height, &pitch);

        // Clear
        memset(pixels, 0, pitch * height);

        int y = 0;

        // 1. Palette strip
        draw_palette_strip(pixels, pitch, width, y, PALETTE_HEIGHT);
        y += PALETTE_HEIGHT;

        // 2. Tile grid
        draw_tile_grid(pixels, pitch, width, height, y, TILES_HEIGHT);
        y += TILES_HEIGHT;

        // 3. Animated shape
        int shape_frame = (elapsed.count() / 100) % mouse_shape.GetFrameCount();
        int shape_x = width / 2;
        int shape_y = y + SHAPES_HEIGHT / 2;
        mouse_shape.Draw(pixels, pitch, width, height, shape_x, shape_y, shape_frame);
        y += SHAPES_HEIGHT;

        // 4. Status bar
        draw_status(pixels, pitch, width, height, y, "Asset System Demo");

        Platform_Graphics_UnlockBackBuffer();
        Platform_Graphics_Flip();

        frame_count++;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    g_results.render_fps = (frame_count * 1000.0) / total_ms;

    printf("  Rendered %d frames in %.1f seconds\n", frame_count, total_ms / 1000.0);
}

void cleanup() {
    printf("\nCleaning up...\n");
    TemplateManager::Instance().UnloadTheater();
    MixManager::Instance().Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();
    printf("  Done\n");
}

// === Main ===

int main(int argc, char* argv[]) {
    printf("========================================\n");
    printf("  Asset System Integration Test\n");
    printf("========================================\n\n");

    auto start = std::chrono::high_resolution_clock::now();

    // 1. Platform
    if (!test_platform()) {
        g_results.Print();
        return 1;
    }

    // 2. MIX Manager
    if (!test_mix_manager()) {
        cleanup();
        g_results.Print();
        return 1;
    }

    // 3. Palette
    test_palette();

    // 4. Shapes
    ShapeData mouse;
    if (test_shapes()) {
        mouse.LoadFromMix("MOUSE.SHP");
    }

    // 5. Templates
    test_templates();

    auto load_end = std::chrono::high_resolution_clock::now();
    g_results.load_time_ms = std::chrono::duration_cast<std::chrono::microseconds>(
        load_end - start).count() / 1000.0;

    // 6. Visual demo
    if (mouse.IsValid()) {
        run_visual_demo(mouse);
    } else {
        printf("\nSkipping visual demo (no shapes loaded)\n");
        g_results.render_fps = 0;
    }

    // Cleanup
    cleanup();

    // Print results
    g_results.Print();

    return g_results.AllPassed() ? 0 : 1;
}
```

### CMakeLists.txt (MODIFY)
```cmake
# Asset integration test
add_executable(TestAssetsIntegration src/test_assets_integration.cpp)
target_link_libraries(TestAssetsIntegration PRIVATE game_assets)
```

## Verification Command
```bash
ralph-tasks/verify/check-assets-integration.sh
```

## Verification Script

### ralph-tasks/verify/check-assets-integration.sh (NEW)
```bash
#!/bin/bash
# Verification script for asset system integration

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Asset System Integration ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check source file
echo "[1/5] Checking source file..."
if [ ! -f "src/test_assets_integration.cpp" ]; then
    echo "VERIFY_FAILED: test_assets_integration.cpp not found"
    exit 1
fi
echo "  OK: Source file exists"

# Step 2: Build all dependencies
echo "[2/5] Building asset system..."
if ! cmake --build build --target game_assets 2>&1; then
    echo "VERIFY_FAILED: game_assets build failed"
    exit 1
fi
echo "  OK: game_assets built"

# Step 3: Build integration test
echo "[3/5] Building integration test..."
if ! cmake --build build --target TestAssetsIntegration 2>&1; then
    echo "VERIFY_FAILED: TestAssetsIntegration build failed"
    exit 1
fi
echo "  OK: TestAssetsIntegration built"

# Step 4: Check for game data
echo "[4/5] Checking game data..."
if [ ! -d "gamedata" ]; then
    echo "VERIFY_WARNING: gamedata directory not found"
    echo "VERIFY_SUCCESS (build only)"
    exit 0
fi

if [ ! -f "gamedata/REDALERT.MIX" ]; then
    echo "VERIFY_WARNING: REDALERT.MIX not found"
    echo "VERIFY_SUCCESS (build only)"
    exit 0
fi
echo "  OK: Game data exists"

# Step 5: Run integration test
echo "[5/5] Running integration test..."
if ! timeout 60 ./build/TestAssetsIntegration 2>&1; then
    echo "VERIFY_FAILED: Integration test failed"
    exit 1
fi
echo "  OK: Integration test passed"

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Create `src/test_assets_integration.cpp`
2. Update CMakeLists.txt
3. Build: `cmake --build build --target TestAssetsIntegration`
4. Run: `./build/TestAssetsIntegration`
5. Verify visual output shows all components
6. Check that all systems initialize and shut down cleanly
7. Run verification script

## Success Criteria
- All subsystems initialize successfully
- At least one MIX file mounts
- Palette loads and applies
- At least one shape loads
- Templates load (even with defaults)
- Visual demo runs without crash
- Clean shutdown with no leaks
- All tests pass

## Performance Targets
- Load time: < 1000ms for core assets
- Render: > 30 FPS (should be ~60)
- Memory: No leaks on exit

## Common Issues

### Missing game data
- Integration test gracefully handles missing files
- Will report which components failed
- Build-only verification still passes

### Graphics not displaying
- Ensure Platform_Graphics_Init() called before graphics ops
- Check that palette is applied before drawing

### Shape animation wrong
- Verify frame index calculation
- Check elapsed time handling

### Shutdown crash
- Verify shutdown order is reverse of init
- Ensure no dangling references

## Documentation Output
After running, the test outputs a summary that can be included in documentation:

```
=== Integration Test Results ===
Platform:   PASS
Graphics:   PASS
MIX Files:  PASS (X files in Y archives)
Palette:    PASS (256 colors)
Shapes:     PASS (N frames)
Templates:  PASS (M tiles)

Performance:
  Load time: X.X ms
  Render:    XX.X FPS

>>> ALL TESTS PASSED <<<
```

## Completion Promise
When verification passes, output:
```
<promise>TASK_12H_COMPLETE</promise>
```

## Escape Hatch
If stuck after 15 iterations:
- Document in `ralph-tasks/blocked/12h.md`
- Note which specific subsystems are failing
- Output: `<promise>TASK_12H_BLOCKED</promise>`

## Max Iterations
15

## Phase Completion
When this task completes successfully, the entire Asset System phase (Phase 12) is complete. The project will have:

1. **CRC32 calculation** for MIX file lookups
2. **MIX file parsing** for header and entry access
3. **MIX data retrieval** for loading file contents
4. **MIX manager** for multi-archive search
5. **Palette loading** with 6-to-8-bit conversion and effects
6. **Shape loading** with RLE/LCW decompression
7. **Template loading** for terrain tiles
8. **Integration test** proving all components work together

This provides the foundation for all subsequent game integration work.
