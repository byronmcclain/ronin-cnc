# Phase 19: Testing & Polish

## Overview

Final phase focusing on comprehensive testing, bug fixes, performance optimization, and macOS polish for release quality.

---

## Testing Categories

### 1. Unit Tests

Test individual components in isolation.

```bash
# Platform layer tests
./build/TestPlatform
./build/TestPlatform --quick

# Asset system tests
./build/TestMix gamedata/REDALERT.MIX
./build/TestPalette
./build/TestShape

# Audio tests
./build/TestAudio
```

### 2. Integration Tests

Test component interactions.

```cpp
// test_integration.cpp

void Test_Full_Init_Shutdown() {
    // Initialize all systems
    Platform_Init();
    Audio_Init();
    MixManager::Instance().Initialize("gamedata");

    // Load essential assets
    MixManager::Instance().Mount("REDALERT.MIX");
    PaletteManager::Instance().LoadPalette("TEMPERAT.PAL");
    SoundManager::Instance().LoadSounds();

    // Verify state
    assert(Platform_IsInitialized());
    assert(Platform_Audio_IsInitialized());
    assert(Platform_Graphics_IsInitialized());

    // Shutdown in reverse order
    SoundManager::Instance().Shutdown();
    MixManager::Instance().Shutdown();
    Audio_Shutdown();
    Platform_Shutdown();

    printf("PASS: Full init/shutdown cycle\n");
}

void Test_Asset_Pipeline() {
    Platform_Init();
    MixManager::Instance().Initialize("gamedata");
    MixManager::Instance().Mount("REDALERT.MIX");

    // Load palette
    assert(PaletteManager::Instance().LoadPalette("TEMPERAT.PAL"));

    // Load shape
    ShapeData mouse;
    assert(mouse.LoadFromMix("MOUSE.SHP"));
    assert(mouse.GetFrameCount() > 0);

    // Load sound
    AudFile sound;
    assert(sound.LoadFromMix("CLICK.AUD"));
    assert(sound.GetPCMSize() > 0);

    printf("PASS: Asset pipeline\n");
}
```

### 3. Visual Tests

```cpp
void Test_Render_Pipeline() {
    Game_Init();

    // Render test frames
    for (int i = 0; i < 300; i++) {  // 5 seconds @ 60fps
        Platform_Frame_Begin();
        Platform_PollEvents();

        // Draw test pattern
        Render_Frame();

        Platform_Frame_End();
    }

    Game_Shutdown();
}

void Test_All_Palettes() {
    Platform_Init();
    Platform_Graphics_Init();

    const char* palettes[] = {
        "TEMPERAT.PAL", "SNOW.PAL", "DESERT.PAL",
        "INTERIOR.PAL", "JUNGLE.PAL"
    };

    for (const char* pal : palettes) {
        PaletteManager::Instance().LoadPalette(pal);
        PaletteManager::Instance().Apply();

        // Draw gradient
        uint8_t* pixels;
        int32_t w, h, p;
        Platform_Graphics_GetBackBuffer(&pixels, &w, &h, &p);
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                pixels[y * p + x] = x & 0xFF;
            }
        }
        Platform_Graphics_Flip();
        Platform_Timer_Delay(1000);
    }

    Platform_Graphics_Shutdown();
    Platform_Shutdown();
}
```

### 4. Gameplay Tests

```cpp
void Test_Unit_Movement() {
    // Create test map
    // Spawn unit
    // Issue move command
    // Verify unit reaches destination
}

void Test_Combat() {
    // Create two opposing units
    // Set to attack each other
    // Verify damage applied
    // Verify one is destroyed
}

void Test_Building_Construction() {
    // Place construction yard
    // Start building
    // Verify build time
    // Verify building placed
}
```

### 5. Performance Tests

```cpp
void Test_Frame_Rate() {
    Game_Init();

    int frame_count = 0;
    uint32_t start = Platform_Timer_GetTicks();

    while (frame_count < 1000) {
        Platform_Frame_Begin();
        Render_Frame();
        Platform_Frame_End();
        frame_count++;
    }

    uint32_t elapsed = Platform_Timer_GetTicks() - start;
    double fps = (frame_count * 1000.0) / elapsed;

    printf("Average FPS: %.1f\n", fps);
    assert(fps >= 55.0);  // Should maintain 60fps

    Game_Shutdown();
}

void Test_Many_Units() {
    Game_Init();

    // Spawn 500 units
    for (int i = 0; i < 500; i++) {
        SpawnUnit(UNIT_MEDIUM_TANK, rand() % MAP_WIDTH, rand() % MAP_HEIGHT);
    }

    // Render and check FPS
    int frame_count = 0;
    uint32_t start = Platform_Timer_GetTicks();

    while (frame_count < 300) {
        Platform_Frame_Begin();
        Game_Logic();
        Render_Frame();
        Platform_Frame_End();
        frame_count++;
    }

    uint32_t elapsed = Platform_Timer_GetTicks() - start;
    double fps = (frame_count * 1000.0) / elapsed;

    printf("500 units FPS: %.1f\n", fps);
    assert(fps >= 30.0);  // Should be playable

    Game_Shutdown();
}
```

---

## Verification Scripts

Create comprehensive verification suite:

```bash
#!/bin/bash
# verify/run_all_tests.sh

set -e

echo "=== Red Alert macOS Port - Full Test Suite ==="
echo ""

# Platform tests
echo "[1/8] Platform layer tests..."
./build/TestPlatform --quick || exit 1

# Asset tests
echo "[2/8] MIX file loading..."
./build/TestMix gamedata/REDALERT.MIX || exit 1
./build/TestMix gamedata/SETUP.MIX || exit 1

# Graphics tests
echo "[3/8] Graphics system..."
./scripts/test_graphics.sh || exit 1

# Audio tests
echo "[4/8] Audio system..."
./build/TestAudio || exit 1

# Input tests
echo "[5/8] Input system..."
./scripts/test_input.sh || exit 1

# Integration tests
echo "[6/8] Integration tests..."
./build/TestIntegration || exit 1

# Performance tests
echo "[7/8] Performance tests..."
./build/TestPerformance || exit 1

# Memory leak check
echo "[8/8] Memory leak check..."
leaks --atExit -- ./build/RedAlertGame --quick-test 2>&1 | grep -q "0 leaks" || {
    echo "WARN: Potential memory leaks detected"
}

echo ""
echo "=== All tests passed! ==="
```

---

## Bug Tracking

### Known Issues Template

```markdown
## Issue: [Title]

**Severity:** Critical / Major / Minor / Cosmetic
**Component:** Graphics / Audio / Input / Game Logic
**Status:** Open / In Progress / Fixed

**Description:**
[Detailed description]

**Steps to Reproduce:**
1. ...
2. ...
3. ...

**Expected Behavior:**
[What should happen]

**Actual Behavior:**
[What actually happens]

**Fix:**
[Solution when resolved]
```

### Common Issues to Check

- [ ] Memory leaks (use `leaks` tool on macOS)
- [ ] Crash on shutdown
- [ ] Audio continues after window close
- [ ] Input stuck after losing focus
- [ ] Fullscreen toggle issues
- [ ] Retina display scaling
- [ ] Mouse position offset
- [ ] Frame rate drops
- [ ] Asset loading failures
- [ ] Save/load corruption

---

## Performance Optimization

### Profiling

```bash
# CPU profiling with Instruments
instruments -t "Time Profiler" ./build/RedAlertGame

# Memory profiling
instruments -t "Allocations" ./build/RedAlertGame

# Frame analysis
instruments -t "Metal System Trace" ./build/RedAlertGame
```

### Optimization Checklist

- [ ] **Rendering:**
  - [ ] Minimize draw calls
  - [ ] Batch sprite rendering
  - [ ] Dirty rectangle optimization
  - [ ] Culling off-screen objects

- [ ] **Memory:**
  - [ ] Pool frequently allocated objects
  - [ ] Cache MIX file data
  - [ ] Unload unused assets

- [ ] **CPU:**
  - [ ] Profile hot paths
  - [ ] Use SIMD where applicable (platform layer)
  - [ ] Reduce allocations in game loop

- [ ] **I/O:**
  - [ ] Async asset loading
  - [ ] Cache file handles
  - [ ] Minimize disk access during gameplay

---

## macOS Polish

### App Bundle Structure

```
RedAlert.app/
├── Contents/
│   ├── Info.plist
│   ├── MacOS/
│   │   └── RedAlert          # Main executable
│   ├── Resources/
│   │   ├── icon.icns         # App icon
│   │   ├── gamedata/         # MIX files
│   │   └── en.lproj/
│   │       └── InfoPlist.strings
│   └── Frameworks/           # Bundled dylibs if needed
```

### Info.plist

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
    "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key>
    <string>Red Alert</string>
    <key>CFBundleDisplayName</key>
    <string>Command & Conquer: Red Alert</string>
    <key>CFBundleIdentifier</key>
    <string>com.westwood.redalert</string>
    <key>CFBundleVersion</key>
    <string>1.0.0</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>CFBundleExecutable</key>
    <string>RedAlert</string>
    <key>CFBundleIconFile</key>
    <string>icon</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.15</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>LSApplicationCategoryType</key>
    <string>public.app-category.games</string>
    <key>NSHumanReadableCopyright</key>
    <string>© 1996 Westwood Studios. GPL v3.</string>
</dict>
</plist>
```

### Build Script

```bash
#!/bin/bash
# scripts/build_app_bundle.sh

APP_NAME="RedAlert"
BUNDLE_DIR="build/${APP_NAME}.app"

# Clean previous
rm -rf "$BUNDLE_DIR"

# Create structure
mkdir -p "$BUNDLE_DIR/Contents/MacOS"
mkdir -p "$BUNDLE_DIR/Contents/Resources"
mkdir -p "$BUNDLE_DIR/Contents/Resources/gamedata"

# Copy executable
cp build/RedAlertGame "$BUNDLE_DIR/Contents/MacOS/RedAlert"

# Copy Info.plist
cp resources/Info.plist "$BUNDLE_DIR/Contents/"

# Copy icon
cp resources/icon.icns "$BUNDLE_DIR/Contents/Resources/"

# Copy game data
cp -r gamedata/*.MIX "$BUNDLE_DIR/Contents/Resources/gamedata/"
cp -r gamedata/*.mix "$BUNDLE_DIR/Contents/Resources/gamedata/"
cp -r gamedata/CD1 "$BUNDLE_DIR/Contents/Resources/gamedata/"
cp -r gamedata/CD2 "$BUNDLE_DIR/Contents/Resources/gamedata/"

# Sign (optional, for distribution)
# codesign --deep --force --sign - "$BUNDLE_DIR"

echo "Built: $BUNDLE_DIR"
```

---

## Release Checklist

### Pre-Release

- [ ] All tests passing
- [ ] No known critical bugs
- [ ] Performance acceptable (60fps target)
- [ ] Memory usage reasonable
- [ ] No memory leaks
- [ ] Clean shutdown
- [ ] Save/load working
- [ ] Multiplayer tested (if implemented)

### Build

- [ ] Release build (optimizations enabled)
- [ ] Universal binary (x86_64 + arm64)
- [ ] App bundle created
- [ ] Code signed (if distributing)
- [ ] Notarized (if distributing outside App Store)

### Documentation

- [ ] README.md updated
- [ ] BUILD.md accurate
- [ ] CHANGELOG.md current
- [ ] License files included
- [ ] Credits/attribution complete

### Distribution

- [ ] Tested on macOS 10.15 (minimum)
- [ ] Tested on latest macOS
- [ ] Tested on Intel Mac
- [ ] Tested on Apple Silicon Mac
- [ ] DMG or ZIP created
- [ ] SHA256 checksum generated

---

## Final Verification

```bash
#!/bin/bash
# verify/final_release_check.sh

echo "=== Release Verification ==="

# Check app bundle
APP="build/RedAlert.app"
if [ ! -d "$APP" ]; then
    echo "FAIL: App bundle not found"
    exit 1
fi

# Check executable
if [ ! -x "$APP/Contents/MacOS/RedAlert" ]; then
    echo "FAIL: Executable not found or not executable"
    exit 1
fi

# Check game data
if [ ! -f "$APP/Contents/Resources/gamedata/REDALERT.MIX" ]; then
    echo "FAIL: Game data not found"
    exit 1
fi

# Check Info.plist
if [ ! -f "$APP/Contents/Info.plist" ]; then
    echo "FAIL: Info.plist not found"
    exit 1
fi

# Test launch
echo "Testing app launch..."
"$APP/Contents/MacOS/RedAlert" --test-only &
PID=$!
sleep 3
if ps -p $PID > /dev/null; then
    echo "PASS: App launched successfully"
    kill $PID 2>/dev/null
else
    echo "FAIL: App crashed on launch"
    exit 1
fi

# Check architecture
echo "Checking architectures..."
lipo -info "$APP/Contents/MacOS/RedAlert"

# Check signing (optional)
echo "Checking code signature..."
codesign -v "$APP" 2>&1 || echo "WARN: Not signed"

echo ""
echo "=== Release verification complete ==="
```

---

## Success Criteria

### Minimum Viable Product

- [ ] Game launches without crash
- [ ] Title screen displays
- [ ] Main menu navigable
- [ ] Skirmish game starts
- [ ] Units can be selected and moved
- [ ] Buildings can be constructed
- [ ] Combat functions
- [ ] Sound effects play
- [ ] Music plays
- [ ] Game can be quit cleanly

### Full Release

All MVP criteria plus:
- [ ] Campaign missions playable
- [ ] All unit types functional
- [ ] All building types functional
- [ ] Fog of war works
- [ ] AI opponents work
- [ ] Multiplayer functional
- [ ] Save/load works
- [ ] All sound/music present
- [ ] 60 FPS maintained
- [ ] No significant bugs

---

## Estimated Effort

| Task | Hours |
|------|-------|
| Unit tests | 4-6 |
| Integration tests | 4-6 |
| Visual tests | 3-4 |
| Gameplay tests | 6-8 |
| Performance tests | 3-4 |
| Bug fixes | 10-20 |
| Optimization | 5-10 |
| App bundle | 3-4 |
| Documentation | 3-4 |
| Final verification | 2-3 |
| **Total** | **43-69** |
