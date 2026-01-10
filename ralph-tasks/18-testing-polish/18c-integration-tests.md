# Task 18c: Integration Tests

| Field | Value |
|-------|-------|
| **Task ID** | 18c |
| **Task Name** | Integration Tests |
| **Phase** | 18 - Testing & Polish |
| **Depends On** | Tasks 18a, 18b (Test Framework, Unit Tests) |
| **Estimated Complexity** | High (~600 lines) |

---

## Dependencies

- Task 18a (Test Framework) - Test infrastructure
- Task 18b (Unit Tests) - Component-level verification
- All platform subsystems working
- Asset system functioning
- Audio system functioning

---

## Context

Integration tests verify that multiple components work correctly together. Unlike unit tests which test components in isolation, integration tests verify:

1. **Init/Shutdown Cycles**: Full system initialization and cleanup
2. **Asset Pipeline**: Loading assets through all system layers
3. **Graphics + Assets**: Rendering loaded shapes with palettes
4. **Audio + Assets**: Playing loaded sound files
5. **Input + Graphics**: Input handling with visual feedback
6. **Full Game Loop**: All systems working together

Integration tests may be slower than unit tests and may require actual game data.

---

## Technical Background

### Integration Test Categories

| Category | Description | Systems Involved |
|----------|-------------|------------------|
| Lifecycle | Init/shutdown sequences | Platform, all subsystems |
| Asset Pipeline | Load and use game assets | MIX, assets, graphics |
| Audio Pipeline | Load and play audio | MIX, AUD, audio |
| Render Pipeline | Full frame rendering | Graphics, assets, palette |
| Input Pipeline | Input through to game | Input, event handling |
| Game State | State transitions | All game systems |

### Test Data Requirements

Integration tests may need actual game assets. The test should:
1. Check if assets exist before running
2. Skip gracefully if assets unavailable
3. Document which assets are required

### Fixture Lifecycle for Integration

```
SetUp:
  1. Platform_Init()
  2. Graphics_Init()
  3. Audio_Init()
  4. MixManager.Initialize()
  5. Load required assets

Test:
  - Test multi-component interaction

TearDown:
  1. Unload assets
  2. MixManager.Shutdown()
  3. Audio_Shutdown()
  4. Graphics_Shutdown()
  5. Platform_Shutdown()
```

---

## Objective

Create integration tests that:
1. Verify full system init/shutdown cycles
2. Test complete asset loading pipeline
3. Test graphics + asset rendering
4. Test audio + asset playback
5. Test multi-frame game loops
6. Verify clean resource cleanup

---

## Deliverables

- [ ] `src/tests/integration/test_lifecycle.cpp` - Init/shutdown tests
- [ ] `src/tests/integration/test_asset_pipeline.cpp` - Asset loading tests
- [ ] `src/tests/integration/test_render_pipeline.cpp` - Rendering tests
- [ ] `src/tests/integration/test_audio_pipeline.cpp` - Audio tests
- [ ] `src/tests/integration/test_game_loop.cpp` - Game loop tests
- [ ] `src/tests/integration_tests_main.cpp` - Main entry point
- [ ] CMakeLists.txt updated
- [ ] All tests pass

---

## Files to Create

### src/tests/integration/test_lifecycle.cpp

```cpp
// src/tests/integration/test_lifecycle.cpp
// System Lifecycle Integration Tests
// Task 18c - Integration Tests

#include "test/test_framework.h"
#include "platform.h"
#include "game/mix_manager.h"

//=============================================================================
// Platform Lifecycle Tests
//=============================================================================

TEST_CASE(Lifecycle_Platform_InitShutdown, "Lifecycle") {
    // Clean init
    TEST_ASSERT_EQ(Platform_Init(), 0);
    TEST_ASSERT(Platform_IsInitialized());

    // Clean shutdown
    Platform_Shutdown();
    TEST_ASSERT(!Platform_IsInitialized());
}

TEST_CASE(Lifecycle_Platform_MultipleInitShutdown, "Lifecycle") {
    // Multiple cycles should work
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT_EQ(Platform_Init(), 0);
        TEST_ASSERT(Platform_IsInitialized());
        Platform_Shutdown();
        TEST_ASSERT(!Platform_IsInitialized());
    }
}

//=============================================================================
// Graphics Lifecycle Tests
//=============================================================================

TEST_CASE(Lifecycle_Graphics_InitShutdown, "Lifecycle") {
    TEST_ASSERT_EQ(Platform_Init(), 0);

    GraphicsConfig config;
    config.width = 640;
    config.height = 480;
    config.fullscreen = false;
    config.vsync = false;

    TEST_ASSERT_EQ(Platform_Graphics_Init(&config), 0);
    TEST_ASSERT(Platform_Graphics_IsInitialized());

    Platform_Graphics_Shutdown();
    TEST_ASSERT(!Platform_Graphics_IsInitialized());

    Platform_Shutdown();
}

TEST_CASE(Lifecycle_Graphics_BackBufferAvailable, "Lifecycle") {
    Platform_Init();

    GraphicsConfig config = {640, 480, false, false};
    Platform_Graphics_Init(&config);

    uint8_t* buffer;
    int32_t w, h, p;
    TEST_ASSERT_EQ(Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p), 0);
    TEST_ASSERT_NOT_NULL(buffer);
    TEST_ASSERT_EQ(w, 640);
    TEST_ASSERT_EQ(h, 480);

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Audio Lifecycle Tests
//=============================================================================

TEST_CASE(Lifecycle_Audio_InitShutdown, "Lifecycle") {
    TEST_ASSERT_EQ(Platform_Init(), 0);

    AudioConfig config;
    config.sample_rate = 22050;
    config.channels = 2;
    config.bits_per_sample = 16;
    config.buffer_size = 1024;

    TEST_ASSERT_EQ(Platform_Audio_Init(&config), 0);
    TEST_ASSERT(Platform_Audio_IsInitialized());

    Platform_Audio_Shutdown();
    TEST_ASSERT(!Platform_Audio_IsInitialized());

    Platform_Shutdown();
}

//=============================================================================
// Full System Lifecycle Tests
//=============================================================================

TEST_CASE(Lifecycle_FullSystem_InitShutdown, "Lifecycle") {
    // Initialize in correct order
    TEST_ASSERT_EQ(Platform_Init(), 0);

    GraphicsConfig gfx_config = {640, 480, false, false};
    TEST_ASSERT_EQ(Platform_Graphics_Init(&gfx_config), 0);

    AudioConfig audio_config = {22050, 2, 16, 1024};
    TEST_ASSERT_EQ(Platform_Audio_Init(&audio_config), 0);

    // Verify all initialized
    TEST_ASSERT(Platform_IsInitialized());
    TEST_ASSERT(Platform_Graphics_IsInitialized());
    TEST_ASSERT(Platform_Audio_IsInitialized());

    // Shutdown in reverse order
    Platform_Audio_Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();

    // Verify all shut down
    TEST_ASSERT(!Platform_Audio_IsInitialized());
    TEST_ASSERT(!Platform_Graphics_IsInitialized());
    TEST_ASSERT(!Platform_IsInitialized());
}

TEST_CASE(Lifecycle_FullSystem_MultipleCycles, "Lifecycle") {
    for (int cycle = 0; cycle < 3; cycle++) {
        Platform_Init();

        GraphicsConfig gfx_config = {640, 480, false, false};
        Platform_Graphics_Init(&gfx_config);

        AudioConfig audio_config = {22050, 2, 16, 1024};
        Platform_Audio_Init(&audio_config);

        // Run a few frames
        for (int frame = 0; frame < 10; frame++) {
            Platform_PumpEvents();
            Platform_Graphics_Flip();
            Platform_Timer_Delay(16);
        }

        Platform_Audio_Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
    }

    TEST_ASSERT(true);  // If we got here, no crashes
}

//=============================================================================
// Error Recovery Tests
//=============================================================================

TEST_CASE(Lifecycle_ShutdownWithoutInit, "Lifecycle") {
    // Shutdown without init should not crash
    Platform_Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Audio_Shutdown();

    TEST_ASSERT(true);
}

TEST_CASE(Lifecycle_DoubleInit, "Lifecycle") {
    Platform_Init();
    int result = Platform_Init();  // Double init

    // Should either succeed or return "already initialized"
    TEST_ASSERT(result == 0 || result == 1);

    Platform_Shutdown();
}

TEST_CASE(Lifecycle_DoubleShutdown, "Lifecycle") {
    Platform_Init();
    Platform_Shutdown();
    Platform_Shutdown();  // Double shutdown - should not crash

    TEST_ASSERT(true);
}

//=============================================================================
// MixManager Lifecycle Tests
//=============================================================================

TEST_CASE(Lifecycle_MixManager_InitShutdown, "Lifecycle") {
    Platform_Init();

    MixManager& mix = MixManager::Instance();
    TEST_ASSERT(mix.Initialize());

    mix.Shutdown();

    Platform_Shutdown();
}

TEST_CASE(Lifecycle_MixManager_ReInitialize, "Lifecycle") {
    Platform_Init();

    MixManager& mix = MixManager::Instance();

    // Multiple init/shutdown cycles
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT(mix.Initialize());
        mix.Shutdown();
    }

    Platform_Shutdown();
}
```

### src/tests/integration/test_asset_pipeline.cpp

```cpp
// src/tests/integration/test_asset_pipeline.cpp
// Asset Pipeline Integration Tests
// Task 18c - Integration Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "platform.h"
#include "game/mix_manager.h"
#include "game/palette.h"
#include "game/shape.h"
#include "game/audio/aud_file.h"
#include <filesystem>

//=============================================================================
// Helper to check for game data
//=============================================================================

static bool HasGameData() {
    // Check if essential game files exist
    return std::filesystem::exists("gamedata/REDALERT.MIX") ||
           std::filesystem::exists("REDALERT.MIX");
}

//=============================================================================
// MIX File Loading Tests
//=============================================================================

TEST_CASE(AssetPipeline_MixLoad_RedalertMix, "AssetPipeline") {
    if (!HasGameData()) {
        TEST_SKIP("Game data not found");
    }

    Platform_Init();

    MixManager& mix = MixManager::Instance();
    mix.Initialize();

    bool loaded = mix.LoadMix("REDALERT.MIX") ||
                  mix.LoadMix("gamedata/REDALERT.MIX");
    TEST_ASSERT_MSG(loaded, "Failed to load REDALERT.MIX");

    mix.Shutdown();
    Platform_Shutdown();
}

TEST_CASE(AssetPipeline_MixLoad_MultipleMix, "AssetPipeline") {
    if (!HasGameData()) {
        TEST_SKIP("Game data not found");
    }

    Platform_Init();

    MixManager& mix = MixManager::Instance();
    mix.Initialize();

    // Load multiple MIX files
    mix.LoadMix("REDALERT.MIX");
    mix.LoadMix("LOCAL.MIX");
    mix.LoadMix("CONQUER.MIX");

    // At least one should have loaded
    TEST_ASSERT_GT(mix.GetLoadedMixCount(), 0);

    mix.Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Palette Loading Tests
//=============================================================================

TEST_CASE(AssetPipeline_Palette_LoadFromMix, "AssetPipeline") {
    if (!HasGameData()) {
        TEST_SKIP("Game data not found");
    }

    Platform_Init();

    MixManager& mix = MixManager::Instance();
    mix.Initialize();
    mix.LoadMix("REDALERT.MIX");

    // Load a palette
    PaletteData palette;
    bool loaded = palette.LoadFromMix("TEMPERAT.PAL");

    if (loaded) {
        TEST_ASSERT_EQ(palette.GetColorCount(), 256u);

        // Check that colors are valid (not all zero)
        bool has_nonzero = false;
        for (int i = 0; i < 256; i++) {
            uint8_t r, g, b;
            palette.GetColor(i, &r, &g, &b);
            if (r != 0 || g != 0 || b != 0) {
                has_nonzero = true;
                break;
            }
        }
        TEST_ASSERT(has_nonzero);
    } else {
        TEST_SKIP("Palette not found in MIX");
    }

    mix.Shutdown();
    Platform_Shutdown();
}

TEST_CASE(AssetPipeline_Palette_AllTheaters, "AssetPipeline") {
    if (!HasGameData()) {
        TEST_SKIP("Game data not found");
    }

    Platform_Init();

    MixManager& mix = MixManager::Instance();
    mix.Initialize();
    mix.LoadMix("REDALERT.MIX");

    const char* palettes[] = {
        "TEMPERAT.PAL",
        "SNOW.PAL",
        "INTERIOR.PAL"
    };

    int loaded_count = 0;
    for (const char* pal_name : palettes) {
        PaletteData palette;
        if (palette.LoadFromMix(pal_name)) {
            loaded_count++;
            TEST_ASSERT_EQ(palette.GetColorCount(), 256u);
        }
    }

    // At least one palette should load
    TEST_ASSERT_GT(loaded_count, 0);

    mix.Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Shape Loading Tests
//=============================================================================

TEST_CASE(AssetPipeline_Shape_LoadMouse, "AssetPipeline") {
    if (!HasGameData()) {
        TEST_SKIP("Game data not found");
    }

    Platform_Init();

    MixManager& mix = MixManager::Instance();
    mix.Initialize();
    mix.LoadMix("REDALERT.MIX");

    ShapeData shape;
    bool loaded = shape.LoadFromMix("MOUSE.SHP");

    if (loaded) {
        TEST_ASSERT_GT(shape.GetFrameCount(), 0u);
        TEST_ASSERT_GT(shape.GetWidth(), 0);
        TEST_ASSERT_GT(shape.GetHeight(), 0);
    } else {
        TEST_SKIP("MOUSE.SHP not found");
    }

    mix.Shutdown();
    Platform_Shutdown();
}

TEST_CASE(AssetPipeline_Shape_LoadUnit, "AssetPipeline") {
    if (!HasGameData()) {
        TEST_SKIP("Game data not found");
    }

    Platform_Init();

    MixManager& mix = MixManager::Instance();
    mix.Initialize();
    mix.LoadMix("CONQUER.MIX");

    // Try to load a unit shape
    const char* unit_shapes[] = {"MTNK.SHP", "JEEP.SHP", "E1.SHP"};

    bool any_loaded = false;
    for (const char* shape_name : unit_shapes) {
        ShapeData shape;
        if (shape.LoadFromMix(shape_name)) {
            any_loaded = true;
            TEST_ASSERT_GT(shape.GetFrameCount(), 0u);
            break;
        }
    }

    if (!any_loaded) {
        TEST_SKIP("No unit shapes found");
    }

    mix.Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// AUD File Loading Tests
//=============================================================================

TEST_CASE(AssetPipeline_Audio_LoadClick, "AssetPipeline") {
    if (!HasGameData()) {
        TEST_SKIP("Game data not found");
    }

    Platform_Init();

    MixManager& mix = MixManager::Instance();
    mix.Initialize();
    mix.LoadMix("SOUNDS.MIX");

    AudFile aud;
    bool loaded = aud.LoadFromMix("CLICK.AUD");

    if (loaded) {
        TEST_ASSERT_GT(aud.GetSampleRate(), 0u);
        TEST_ASSERT_GT(aud.GetPCMSize(), 0u);
    } else {
        TEST_SKIP("CLICK.AUD not found");
    }

    mix.Shutdown();
    Platform_Shutdown();
}

TEST_CASE(AssetPipeline_Audio_DecodeWWADPCM, "AssetPipeline") {
    if (!HasGameData()) {
        TEST_SKIP("Game data not found");
    }

    Platform_Init();

    MixManager& mix = MixManager::Instance();
    mix.Initialize();
    mix.LoadMix("SOUNDS.MIX");

    // Find an ADPCM compressed file
    AudFile aud;
    if (aud.LoadFromMix("CLICK.AUD")) {
        // Verify decoded PCM data
        const uint8_t* pcm = aud.GetPCMData();
        uint32_t size = aud.GetPCMSize();

        TEST_ASSERT_NOT_NULL(pcm);
        TEST_ASSERT_GT(size, 0u);

        // PCM data should have some variation (not all zeros or same value)
        bool has_variation = false;
        uint8_t first = pcm[0];
        for (uint32_t i = 1; i < std::min(size, 1000u); i++) {
            if (pcm[i] != first) {
                has_variation = true;
                break;
            }
        }
        TEST_ASSERT(has_variation);
    }

    mix.Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Full Asset Pipeline Tests
//=============================================================================

TEST_CASE(AssetPipeline_Full_LoadAllAssetTypes, "AssetPipeline") {
    if (!HasGameData()) {
        TEST_SKIP("Game data not found");
    }

    Platform_Init();

    MixManager& mix = MixManager::Instance();
    mix.Initialize();

    // Load multiple MIX files
    mix.LoadMix("REDALERT.MIX");
    mix.LoadMix("CONQUER.MIX");
    mix.LoadMix("SOUNDS.MIX");

    int loaded_types = 0;

    // Try palette
    PaletteData palette;
    if (palette.LoadFromMix("TEMPERAT.PAL")) {
        loaded_types++;
    }

    // Try shape
    ShapeData shape;
    if (shape.LoadFromMix("MOUSE.SHP")) {
        loaded_types++;
    }

    // Try sound
    AudFile aud;
    if (aud.LoadFromMix("CLICK.AUD")) {
        loaded_types++;
    }

    TEST_ASSERT_GT(loaded_types, 0);

    mix.Shutdown();
    Platform_Shutdown();
}

TEST_CASE(AssetPipeline_CaseSensitivity, "AssetPipeline") {
    if (!HasGameData()) {
        TEST_SKIP("Game data not found");
    }

    Platform_Init();

    MixManager& mix = MixManager::Instance();
    mix.Initialize();
    mix.LoadMix("REDALERT.MIX");

    // Same file with different cases should all work
    ShapeData shape1, shape2, shape3;

    bool loaded1 = shape1.LoadFromMix("MOUSE.SHP");
    bool loaded2 = shape2.LoadFromMix("mouse.shp");
    bool loaded3 = shape3.LoadFromMix("Mouse.Shp");

    // At least the uppercase version should work
    if (loaded1) {
        // If lowercase/mixed case also work, they should match
        if (loaded2) {
            TEST_ASSERT_EQ(shape1.GetFrameCount(), shape2.GetFrameCount());
        }
        if (loaded3) {
            TEST_ASSERT_EQ(shape1.GetFrameCount(), shape3.GetFrameCount());
        }
    }

    mix.Shutdown();
    Platform_Shutdown();
}
```

### src/tests/integration/test_render_pipeline.cpp

```cpp
// src/tests/integration/test_render_pipeline.cpp
// Render Pipeline Integration Tests
// Task 18c - Integration Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "platform.h"
#include "game/mix_manager.h"
#include "game/palette.h"
#include "game/shape.h"
#include <cstring>

//=============================================================================
// Basic Render Tests
//=============================================================================

TEST_WITH_FIXTURE(GraphicsFixture, RenderPipeline_ClearScreen, "Render") {
    if (!fixture.IsInitialized()) {
        TEST_SKIP("Graphics not initialized");
    }

    // Clear to a specific color
    fixture.ClearBackBuffer(100);

    // Verify
    uint8_t* buffer = fixture.GetBackBuffer();
    TEST_ASSERT_EQ(buffer[0], 100);

    fixture.RenderFrame();
}

TEST_WITH_FIXTURE(GraphicsFixture, RenderPipeline_DrawPattern, "Render") {
    if (!fixture.IsInitialized()) {
        TEST_SKIP("Graphics not initialized");
    }

    uint8_t* buffer = fixture.GetBackBuffer();
    int width = fixture.GetWidth();
    int height = fixture.GetHeight();
    int pitch = fixture.GetPitch();

    // Draw a gradient pattern
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            buffer[y * pitch + x] = (x + y) & 0xFF;
        }
    }

    fixture.RenderFrame();

    // Verify pattern
    TEST_ASSERT_EQ(buffer[0], 0);
    TEST_ASSERT_EQ(buffer[100], 100);
}

//=============================================================================
// Palette Integration Tests
//=============================================================================

TEST_CASE(RenderPipeline_Palette_Apply, "Render") {
    Platform_Init();

    GraphicsConfig config = {640, 480, false, false};
    Platform_Graphics_Init(&config);

    // Create a grayscale palette
    uint32_t palette[256];
    for (int i = 0; i < 256; i++) {
        uint8_t gray = i;
        palette[i] = (gray << 16) | (gray << 8) | gray;  // RGB
    }

    Platform_Graphics_SetPalette(palette, 256);

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

TEST_CASE(RenderPipeline_Palette_LoadAndApply, "Render") {
    Platform_Init();

    GraphicsConfig config = {640, 480, false, false};
    Platform_Graphics_Init(&config);

    MixManager& mix = MixManager::Instance();
    mix.Initialize();

    bool has_data = mix.LoadMix("REDALERT.MIX");
    if (!has_data) {
        mix.Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        TEST_SKIP("Game data not found");
    }

    PaletteData palette;
    if (palette.LoadFromMix("TEMPERAT.PAL")) {
        // Apply palette to graphics
        Platform_Graphics_SetPalette(palette.GetRGBData(), 256);

        // Render a frame with the palette
        Platform_Graphics_Flip();
    }

    mix.Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Shape Rendering Tests
//=============================================================================

TEST_CASE(RenderPipeline_Shape_DrawToBuffer, "Render") {
    Platform_Init();

    GraphicsConfig config = {640, 480, false, false};
    Platform_Graphics_Init(&config);

    MixManager& mix = MixManager::Instance();
    mix.Initialize();

    bool has_data = mix.LoadMix("REDALERT.MIX");
    if (!has_data) {
        mix.Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        TEST_SKIP("Game data not found");
    }

    ShapeData shape;
    if (shape.LoadFromMix("MOUSE.SHP")) {
        uint8_t* buffer;
        int32_t w, h, p;
        Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);

        // Clear buffer
        memset(buffer, 0, h * p);

        // Draw shape frame 0 at center of screen
        int draw_x = (w - shape.GetWidth()) / 2;
        int draw_y = (h - shape.GetHeight()) / 2;

        shape.DrawFrame(buffer, p, w, h, 0, draw_x, draw_y);

        Platform_Graphics_Flip();

        // Verify something was drawn (not all zeros in draw area)
        bool has_content = false;
        for (int y = draw_y; y < draw_y + shape.GetHeight() && y < h; y++) {
            for (int x = draw_x; x < draw_x + shape.GetWidth() && x < w; x++) {
                if (buffer[y * p + x] != 0) {
                    has_content = true;
                    break;
                }
            }
            if (has_content) break;
        }
        TEST_ASSERT(has_content);
    }

    mix.Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Multi-Frame Render Tests
//=============================================================================

TEST_CASE(RenderPipeline_Animation_MultipleFrames, "Render") {
    Platform_Init();

    GraphicsConfig config = {640, 480, false, false};
    Platform_Graphics_Init(&config);

    // Render 60 frames (1 second at 60fps)
    for (int frame = 0; frame < 60; frame++) {
        uint8_t* buffer;
        int32_t w, h, p;
        Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);

        // Animate - change clear color each frame
        uint8_t color = (frame * 4) & 0xFF;
        for (int y = 0; y < h; y++) {
            memset(buffer + y * p, color, w);
        }

        Platform_Graphics_Flip();
        Platform_Timer_Delay(16);
    }

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

TEST_CASE(RenderPipeline_Shape_AllFrames, "Render") {
    Platform_Init();

    GraphicsConfig config = {640, 480, false, false};
    Platform_Graphics_Init(&config);

    MixManager& mix = MixManager::Instance();
    mix.Initialize();

    bool has_data = mix.LoadMix("REDALERT.MIX");
    if (!has_data) {
        mix.Shutdown();
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        TEST_SKIP("Game data not found");
    }

    ShapeData shape;
    if (shape.LoadFromMix("MOUSE.SHP")) {
        uint8_t* buffer;
        int32_t w, h, p;
        Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);

        // Draw each frame
        int frame_count = shape.GetFrameCount();
        for (int i = 0; i < frame_count; i++) {
            memset(buffer, 0, h * p);

            int draw_x = (w - shape.GetWidth()) / 2;
            int draw_y = (h - shape.GetHeight()) / 2;

            shape.DrawFrame(buffer, p, w, h, i, draw_x, draw_y);

            Platform_Graphics_Flip();
            Platform_Timer_Delay(50);  // 50ms per frame for visibility
        }
    }

    mix.Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}
```

### src/tests/integration/test_audio_pipeline.cpp

```cpp
// src/tests/integration/test_audio_pipeline.cpp
// Audio Pipeline Integration Tests
// Task 18c - Integration Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "platform.h"
#include "game/mix_manager.h"
#include "game/audio/aud_file.h"

//=============================================================================
// Audio Playback Tests
//=============================================================================

TEST_WITH_FIXTURE(AudioFixture, AudioPipeline_PlaySilence, "Audio") {
    if (!fixture.IsAudioInitialized()) {
        TEST_SKIP("Audio not initialized");
    }

    // Create silent audio buffer
    std::vector<int16_t> silence(22050, 0);  // 1 second of silence

    SoundHandle handle = Platform_Sound_CreateFromMemory(
        reinterpret_cast<uint8_t*>(silence.data()),
        silence.size() * sizeof(int16_t),
        22050, 1, 16
    );

    TEST_ASSERT_NE(handle, 0u);

    PlayHandle play = Platform_Sound_Play(handle, 1.0f, false);
    TEST_ASSERT_NE(play, 0u);

    // Wait briefly
    fixture.WaitMs(100);

    Platform_Sound_Stop(play);
    Platform_Sound_Destroy(handle);
}

TEST_CASE(AudioPipeline_LoadAndPlayAUD, "Audio") {
    Platform_Init();

    AudioConfig config = {22050, 2, 16, 1024};
    Platform_Audio_Init(&config);

    MixManager& mix = MixManager::Instance();
    mix.Initialize();

    bool has_data = mix.LoadMix("SOUNDS.MIX");
    if (!has_data) {
        mix.Shutdown();
        Platform_Audio_Shutdown();
        Platform_Shutdown();
        TEST_SKIP("SOUNDS.MIX not found");
    }

    AudFile aud;
    if (aud.LoadFromMix("CLICK.AUD")) {
        // Create sound from AUD data
        SoundHandle handle = Platform_Sound_CreateFromMemory(
            aud.GetPCMData(),
            aud.GetPCMSize(),
            aud.GetSampleRate(),
            aud.IsStereo() ? 2 : 1,
            aud.Is16Bit() ? 16 : 8
        );

        if (handle != 0) {
            PlayHandle play = Platform_Sound_Play(handle, 1.0f, false);

            // Wait for sound to finish
            Platform_Timer_Delay(500);

            Platform_Sound_Stop(play);
            Platform_Sound_Destroy(handle);
        }
    }

    mix.Shutdown();
    Platform_Audio_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Volume Control Tests
//=============================================================================

TEST_WITH_FIXTURE(AudioFixture, AudioPipeline_VolumeRamp, "Audio") {
    if (!fixture.IsAudioInitialized()) {
        TEST_SKIP("Audio not initialized");
    }

    // Generate a tone
    std::vector<int16_t> tone(22050);
    for (size_t i = 0; i < tone.size(); i++) {
        // 440Hz sine wave
        double t = i / 22050.0;
        tone[i] = static_cast<int16_t>(16000 * std::sin(2 * 3.14159 * 440 * t));
    }

    SoundHandle handle = Platform_Sound_CreateFromMemory(
        reinterpret_cast<uint8_t*>(tone.data()),
        tone.size() * sizeof(int16_t),
        22050, 1, 16
    );

    if (handle != 0) {
        PlayHandle play = Platform_Sound_Play(handle, 0.0f, true);

        // Ramp volume up
        for (float vol = 0.0f; vol <= 1.0f; vol += 0.1f) {
            Platform_Sound_SetVolume(play, vol);
            fixture.WaitMs(50);
        }

        // Ramp volume down
        for (float vol = 1.0f; vol >= 0.0f; vol -= 0.1f) {
            Platform_Sound_SetVolume(play, vol);
            fixture.WaitMs(50);
        }

        Platform_Sound_Stop(play);
        Platform_Sound_Destroy(handle);
    }
}

//=============================================================================
// Multi-Sound Tests
//=============================================================================

TEST_WITH_FIXTURE(AudioFixture, AudioPipeline_MultipleSounds, "Audio") {
    if (!fixture.IsAudioInitialized()) {
        TEST_SKIP("Audio not initialized");
    }

    std::vector<SoundHandle> handles;
    std::vector<PlayHandle> plays;

    // Create and play multiple sounds
    for (int i = 0; i < 5; i++) {
        std::vector<int16_t> tone(11025);  // 0.5 seconds
        int freq = 220 * (i + 1);  // Different frequencies

        for (size_t j = 0; j < tone.size(); j++) {
            double t = j / 22050.0;
            tone[j] = static_cast<int16_t>(10000 * std::sin(2 * 3.14159 * freq * t));
        }

        SoundHandle h = Platform_Sound_CreateFromMemory(
            reinterpret_cast<uint8_t*>(tone.data()),
            tone.size() * sizeof(int16_t),
            22050, 1, 16
        );

        if (h != 0) {
            handles.push_back(h);
            plays.push_back(Platform_Sound_Play(h, 0.5f, false));
        }
    }

    TEST_ASSERT_GT(handles.size(), 0u);

    // Wait for sounds
    fixture.WaitMs(600);

    // Cleanup
    for (auto play : plays) {
        Platform_Sound_Stop(play);
    }
    for (auto handle : handles) {
        Platform_Sound_Destroy(handle);
    }
}

//=============================================================================
// Music Playback Tests
//=============================================================================

TEST_CASE(AudioPipeline_MusicLoop, "Audio") {
    Platform_Init();

    AudioConfig config = {22050, 2, 16, 1024};
    Platform_Audio_Init(&config);

    MixManager& mix = MixManager::Instance();
    mix.Initialize();

    bool has_data = mix.LoadMix("SCORES.MIX");
    if (!has_data) {
        mix.Shutdown();
        Platform_Audio_Shutdown();
        Platform_Shutdown();
        TEST_SKIP("SCORES.MIX not found");
    }

    // Try to load a music track
    AudFile music;
    const char* tracks[] = {"HELLMRCH.AUD", "BIGFOOT.AUD", "CRUSH.AUD"};

    bool loaded = false;
    for (const char* track : tracks) {
        if (music.LoadFromMix(track)) {
            loaded = true;
            break;
        }
    }

    if (loaded) {
        SoundHandle handle = Platform_Sound_CreateFromMemory(
            music.GetPCMData(),
            music.GetPCMSize(),
            music.GetSampleRate(),
            music.IsStereo() ? 2 : 1,
            music.Is16Bit() ? 16 : 8
        );

        if (handle != 0) {
            // Play with looping
            PlayHandle play = Platform_Sound_Play(handle, 0.5f, true);

            // Let it play briefly
            Platform_Timer_Delay(1000);

            // Verify still playing (looping)
            TEST_ASSERT(Platform_Sound_IsPlaying(play));

            Platform_Sound_Stop(play);
            Platform_Sound_Destroy(handle);
        }
    }

    mix.Shutdown();
    Platform_Audio_Shutdown();
    Platform_Shutdown();
}
```

### src/tests/integration/test_game_loop.cpp

```cpp
// src/tests/integration/test_game_loop.cpp
// Game Loop Integration Tests
// Task 18c - Integration Tests

#include "test/test_framework.h"
#include "test/test_fixtures.h"
#include "platform.h"
#include "game/mix_manager.h"

//=============================================================================
// Basic Game Loop Tests
//=============================================================================

TEST_CASE(GameLoop_BasicFrameLoop, "GameLoop") {
    Platform_Init();

    GraphicsConfig gfx = {640, 480, false, false};
    Platform_Graphics_Init(&gfx);

    // Run 100 frames
    for (int frame = 0; frame < 100; frame++) {
        Platform_PumpEvents();
        Platform_Input_Update();

        // Check for quit
        if (Platform_Key_IsDown(KEY_CODE_ESCAPE)) {
            break;
        }

        // Simple render
        uint8_t* buffer;
        int32_t w, h, p;
        Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);

        uint8_t color = frame & 0xFF;
        for (int y = 0; y < h; y++) {
            memset(buffer + y * p, color, w);
        }

        Platform_Graphics_Flip();
        Platform_Timer_Delay(16);  // ~60fps
    }

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

TEST_CASE(GameLoop_TimingConsistency, "GameLoop") {
    Platform_Init();

    GraphicsConfig gfx = {640, 480, false, false};
    Platform_Graphics_Init(&gfx);

    const int FRAMES = 60;
    uint32_t frame_times[FRAMES];

    uint32_t start = Platform_Timer_GetTicks();
    for (int frame = 0; frame < FRAMES; frame++) {
        uint32_t frame_start = Platform_Timer_GetTicks();

        Platform_PumpEvents();

        uint8_t* buffer;
        int32_t w, h, p;
        Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);
        memset(buffer, frame * 4, h * p);
        Platform_Graphics_Flip();

        // Target 16ms per frame
        uint32_t elapsed = Platform_Timer_GetTicks() - frame_start;
        if (elapsed < 16) {
            Platform_Timer_Delay(16 - elapsed);
        }

        frame_times[frame] = Platform_Timer_GetTicks() - frame_start;
    }
    uint32_t total_time = Platform_Timer_GetTicks() - start;

    // Should take approximately 1 second (60 frames * 16ms)
    TEST_ASSERT_GE(total_time, 900u);   // At least 900ms
    TEST_ASSERT_LE(total_time, 1200u);  // At most 1200ms

    // Calculate average frame time
    uint32_t total_frame_time = 0;
    for (int i = 0; i < FRAMES; i++) {
        total_frame_time += frame_times[i];
    }
    uint32_t avg_frame_time = total_frame_time / FRAMES;

    // Average should be close to 16ms
    TEST_ASSERT_GE(avg_frame_time, 14u);
    TEST_ASSERT_LE(avg_frame_time, 20u);

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Input in Game Loop Tests
//=============================================================================

TEST_CASE(GameLoop_InputHandling, "GameLoop") {
    Platform_Init();

    GraphicsConfig gfx = {640, 480, false, false};
    Platform_Graphics_Init(&gfx);

    bool mouse_moved = false;
    int frames_run = 0;

    for (int frame = 0; frame < 30; frame++) {
        Platform_PumpEvents();
        Platform_Input_Update();

        // Check mouse position
        int32_t mx, my;
        Platform_Mouse_GetPosition(&mx, &my);

        if (mx != 0 || my != 0) {
            mouse_moved = true;
        }

        // Simple render
        uint8_t* buffer;
        int32_t w, h, p;
        Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);
        memset(buffer, 50, h * p);

        // Draw cursor indicator
        if (mx >= 0 && mx < w && my >= 0 && my < h) {
            buffer[my * p + mx] = 255;
        }

        Platform_Graphics_Flip();
        Platform_Timer_Delay(16);
        frames_run++;
    }

    TEST_ASSERT_EQ(frames_run, 30);

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}

//=============================================================================
// Full System Game Loop Tests
//=============================================================================

TEST_CASE(GameLoop_AllSystems, "GameLoop") {
    // Initialize all systems
    Platform_Init();

    GraphicsConfig gfx = {640, 480, false, false};
    Platform_Graphics_Init(&gfx);

    AudioConfig audio = {22050, 2, 16, 1024};
    Platform_Audio_Init(&audio);

    MixManager& mix = MixManager::Instance();
    mix.Initialize();

    // Run game loop
    for (int frame = 0; frame < 30; frame++) {
        // Input
        Platform_PumpEvents();
        Platform_Input_Update();

        // Update (placeholder)
        // Game_Update();

        // Render
        uint8_t* buffer;
        int32_t w, h, p;
        Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);
        memset(buffer, 30 + frame, h * p);
        Platform_Graphics_Flip();

        // Frame timing
        Platform_Timer_Delay(16);
    }

    // Clean shutdown
    mix.Shutdown();
    Platform_Audio_Shutdown();
    Platform_Graphics_Shutdown();
    Platform_Shutdown();

    // Verify clean state
    TEST_ASSERT(!Platform_IsInitialized());
}

//=============================================================================
// State Transition Tests
//=============================================================================

TEST_CASE(GameLoop_StateTransition, "GameLoop") {
    Platform_Init();

    GraphicsConfig gfx = {640, 480, false, false};
    Platform_Graphics_Init(&gfx);

    enum GameState { STATE_MENU, STATE_GAME, STATE_PAUSED };
    GameState state = STATE_MENU;

    for (int frame = 0; frame < 90; frame++) {
        Platform_PumpEvents();
        Platform_Input_Update();

        // State transitions
        if (frame == 30) {
            state = STATE_GAME;
        } else if (frame == 60) {
            state = STATE_PAUSED;
        }

        // Render based on state
        uint8_t* buffer;
        int32_t w, h, p;
        Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p);

        uint8_t color;
        switch (state) {
            case STATE_MENU:   color = 50;  break;
            case STATE_GAME:   color = 100; break;
            case STATE_PAUSED: color = 150; break;
        }

        memset(buffer, color, h * p);
        Platform_Graphics_Flip();
        Platform_Timer_Delay(16);
    }

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}
```

### src/tests/integration_tests_main.cpp

```cpp
// src/tests/integration_tests_main.cpp
// Integration Tests Main Entry Point
// Task 18c - Integration Tests

#include "test/test_framework.h"

TEST_MAIN()
```

---

## CMakeLists.txt Additions

```cmake
# Task 18c: Integration Tests

set(INTEGRATION_TEST_SOURCES
    ${CMAKE_SOURCE_DIR}/src/tests/integration/test_lifecycle.cpp
    ${CMAKE_SOURCE_DIR}/src/tests/integration/test_asset_pipeline.cpp
    ${CMAKE_SOURCE_DIR}/src/tests/integration/test_render_pipeline.cpp
    ${CMAKE_SOURCE_DIR}/src/tests/integration/test_audio_pipeline.cpp
    ${CMAKE_SOURCE_DIR}/src/tests/integration/test_game_loop.cpp
    ${CMAKE_SOURCE_DIR}/src/tests/integration_tests_main.cpp
)

add_executable(IntegrationTests ${INTEGRATION_TEST_SOURCES})
target_link_libraries(IntegrationTests PRIVATE
    test_framework
    redalert_platform
    redalert_game
)
target_include_directories(IntegrationTests PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(NAME IntegrationTests COMMAND IntegrationTests)

# Quick integration tests (lifecycle only)
add_test(NAME IntegrationTests_Quick COMMAND IntegrationTests --category Lifecycle)
```

---

## Verification Script

```bash
#!/bin/bash
set -e
echo "=== Task 18c Verification: Integration Tests ==="

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"

echo "Checking files..."
for f in src/tests/integration/test_lifecycle.cpp \
         src/tests/integration/test_asset_pipeline.cpp \
         src/tests/integration/test_render_pipeline.cpp \
         src/tests/integration/test_audio_pipeline.cpp \
         src/tests/integration/test_game_loop.cpp; do
    [ -f "$ROOT_DIR/$f" ] || { echo "Missing: $f"; exit 1; }
    echo "  $f"
done

echo "Building..."
cd "$BUILD_DIR" && cmake --build . --target IntegrationTests
echo "  Built"

echo "Running integration tests..."
"$BUILD_DIR/IntegrationTests" --verbose || { echo "Integration tests failed"; exit 1; }

echo "=== Task 18c Verification PASSED ==="
```

---

## Success Criteria

- [ ] Lifecycle tests verify init/shutdown sequences
- [ ] Asset pipeline tests load various asset types
- [ ] Render pipeline tests verify graphics + assets
- [ ] Audio pipeline tests verify sound playback
- [ ] Game loop tests verify multi-system coordination
- [ ] Tests skip gracefully when assets unavailable
- [ ] All tests pass
- [ ] No resource leaks between tests

---

## Completion Promise

When verification passes, output:
<promise>TASK_18C_COMPLETE</promise>

## Escape Hatch

If stuck after 15 iterations, document in `ralph-tasks/blocked/18c.md` and output:
<promise>TASK_18C_BLOCKED</promise>

## Max Iterations
15
