# Plan 11: Cleanup & Polish

## Objective
Finalize the macOS port with bug fixes, performance optimization, proper packaging, and quality-of-life improvements.

## Scope

This phase covers:
1. Bug fixes from previous phases
2. Performance optimization
3. macOS-specific integration
4. App bundle packaging
5. Testing and validation
6. Documentation

## 11.1 Bug Fixes

### Common Issues to Address

**Graphics:**
- [ ] Palette flashing during transitions
- [ ] Sprite clipping at screen edges
- [ ] Screen tearing (if VSync issues)
- [ ] Mouse cursor alignment

**Audio:**
- [ ] Audio pops/clicks between sounds
- [ ] Music loop points
- [ ] Volume balance

**Input:**
- [ ] Key repeat handling
- [ ] Mouse acceleration
- [ ] Modifier key state

**General:**
- [ ] Save/load game issues
- [ ] Memory leaks
- [ ] File path issues on case-sensitive systems

### Bug Tracking Process

1. Create `bugs.md` to track known issues
2. Prioritize by severity (crash > gameplay > cosmetic)
3. Fix systematically
4. Verify fix doesn't introduce regressions

## 11.2 Performance Optimization

### Profiling

Use Instruments (Xcode) to profile:
```bash
# CPU profiling
instruments -t "Time Profiler" ./RedAlert.app

# Memory profiling
instruments -t "Allocations" ./RedAlert.app
```

### Optimization Targets

**Rendering Pipeline:**
```cpp
// Batch palette conversions
void OptimizedFlip() {
    // Convert entire buffer in one pass
    // Use SIMD if available
    #ifdef __SSE2__
    // SSE2 optimized path
    #elif defined(__ARM_NEON)
    // NEON optimized path for Apple Silicon
    #else
    // Scalar fallback
    #endif
}
```

**Memory Allocation:**
- Pool frequently allocated objects
- Reduce allocation in game loop
- Pre-allocate audio buffers

**Frame Rate:**
- Target: 60 FPS at 1080p
- Baseline: 30 FPS acceptable
- Measure on M1 Mac Mini and older Intel Macs

### Performance Metrics

| Metric | Target | Minimum |
|--------|--------|---------|
| Frame Rate | 60 FPS | 30 FPS |
| Frame Time | 16ms | 33ms |
| Memory Usage | <256MB | <512MB |
| Load Time | <5s | <10s |

## 11.3 macOS Integration

### Menu Bar

```cpp
// Native macOS menu bar integration
void Setup_MacOS_Menu() {
    // Application menu (handled by SDL2)
    // File menu: New Game, Load, Save, Quit
    // Options menu: Sound, Video, etc.
}
```

### Retina Display Support

```cpp
bool Platform_IsRetinaDisplay() {
    // Check for high-DPI
    float dpi;
    SDL_GetDisplayDPI(0, &dpi, nullptr, nullptr);
    return dpi > 100.0f;
}

void Platform_GetRetinaScale(int* scale_x, int* scale_y) {
    int window_w, window_h;
    int render_w, render_h;

    SDL_GetWindowSize(g_window, &window_w, &window_h);
    SDL_GL_GetDrawableSize(g_window, &render_w, &render_h);

    *scale_x = render_w / window_w;
    *scale_y = render_h / window_h;
}
```

### Fullscreen Mode

```cpp
void Platform_ToggleFullscreen() {
    Uint32 flags = SDL_GetWindowFlags(g_window);

    if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
        SDL_SetWindowFullscreen(g_window, 0);
    } else {
        SDL_SetWindowFullscreen(g_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
}
```

### Application Lifecycle

```cpp
// Handle macOS application events
void Handle_MacOS_Events(SDL_Event* event) {
    switch (event->type) {
        case SDL_APP_WILLENTERBACKGROUND:
            // Pause game, mute audio
            Game_Pause();
            Platform_Audio_SetMasterVolume(0);
            break;

        case SDL_APP_DIDENTERFOREGROUND:
            // Resume
            Game_Resume();
            Platform_Audio_SetMasterVolume(g_saved_volume);
            break;

        case SDL_APP_TERMINATING:
            // Save state, cleanup
            Game_Save_State();
            break;
    }
}
```

## 11.4 App Bundle Packaging

### Bundle Structure

```
RedAlert.app/
├── Contents/
│   ├── Info.plist
│   ├── MacOS/
│   │   └── RedAlert          # Executable
│   ├── Resources/
│   │   ├── RedAlert.icns     # App icon
│   │   ├── data/             # Game data files
│   │   │   ├── REDALERT.MIX
│   │   │   ├── CONQUER.MIX
│   │   │   └── ...
│   │   └── en.lproj/
│   │       └── InfoPlist.strings
│   └── Frameworks/           # Bundled libraries
│       ├── SDL2.framework
│       └── ...
```

### Info.plist

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "...">
<plist version="1.0">
<dict>
    <key>CFBundleName</key>
    <string>Red Alert</string>
    <key>CFBundleDisplayName</key>
    <string>Command &amp; Conquer: Red Alert</string>
    <key>CFBundleIdentifier</key>
    <string>com.example.redalert</string>
    <key>CFBundleVersion</key>
    <string>1.0.0</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>CFBundleExecutable</key>
    <string>RedAlert</string>
    <key>CFBundleIconFile</key>
    <string>RedAlert</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.15</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>LSArchitecturePriority</key>
    <array>
        <string>arm64</string>
        <string>x86_64</string>
    </array>
</dict>
</plist>
```

### CMake Bundle Configuration

```cmake
# macOS app bundle settings
if(APPLE)
    set(MACOSX_BUNDLE TRUE)
    set(MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/platform/macos/Info.plist.in)
    set(MACOSX_BUNDLE_ICON_FILE RedAlert.icns)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.redalert")
    set(MACOSX_BUNDLE_BUNDLE_NAME "Red Alert")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0")

    # Copy resources
    set_source_files_properties(
        ${CMAKE_SOURCE_DIR}/resources/RedAlert.icns
        PROPERTIES MACOSX_PACKAGE_LOCATION "Resources"
    )

    # Copy game data
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/data/
            DESTINATION ${CMAKE_BINARY_DIR}/RedAlert.app/Contents/Resources/data)

    # Copy frameworks
    include(BundleUtilities)
    set(DIRS ${CMAKE_BINARY_DIR})
    fixup_bundle(${CMAKE_BINARY_DIR}/RedAlert.app "" "${DIRS}")
endif()
```

### Universal Binary (Intel + Apple Silicon)

```cmake
# Build universal binary
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")
endif()
```

### Code Signing (Optional)

```bash
# Sign the app bundle
codesign --force --deep --sign "Developer ID Application: Your Name" RedAlert.app

# Verify signature
codesign --verify --verbose RedAlert.app

# Notarize for distribution
xcrun altool --notarize-app --primary-bundle-id "com.example.redalert" \
    --username "your@email.com" --password "@keychain:AC_PASSWORD" \
    --file RedAlert.zip
```

## 11.5 Configuration & Saves

### Settings File Location

```cpp
std::string Get_Config_Path() {
    const char* home = getenv("HOME");
    if (!home) return "./";

    std::string path = home;
    path += "/Library/Application Support/RedAlert/";

    // Create directory if needed
    mkdir(path.c_str(), 0755);

    return path;
}

// Settings file: ~/Library/Application Support/RedAlert/settings.ini
// Save games: ~/Library/Application Support/RedAlert/saves/
```

### Settings Migration

If user has Windows saves (from Steam/GOG), offer to import:
```cpp
bool Import_Windows_Saves(const char* source_path) {
    // Look for save files in standard Windows locations
    // Convert paths and copy to macOS location
    return true;
}
```

## 11.6 Testing Checklist

### Functionality Tests

**Single Player:**
- [ ] Allied campaign plays start to finish
- [ ] Soviet campaign plays start to finish
- [ ] Skirmish mode works
- [ ] Save/Load works at all points
- [ ] All difficulty levels

**Multiplayer:**
- [ ] LAN game discovery
- [ ] Host/Join works
- [ ] Game synchronization
- [ ] All maps playable
- [ ] Various player counts

**Options:**
- [ ] Video settings apply
- [ ] Audio settings apply
- [ ] Control settings work
- [ ] Settings persist between sessions

### Compatibility Tests

**Hardware:**
- [ ] M1/M2 Mac (Apple Silicon)
- [ ] Intel Mac (2015+)
- [ ] Various display resolutions
- [ ] External monitors

**macOS Versions:**
- [ ] macOS 14 (Sonoma)
- [ ] macOS 13 (Ventura)
- [ ] macOS 12 (Monterey)
- [ ] macOS 11 (Big Sur)
- [ ] macOS 10.15 (Catalina) - minimum target

## 11.7 Documentation

### README.md Updates

Add macOS-specific information:
```markdown
## macOS Installation

1. Download RedAlert.dmg
2. Drag Red Alert to Applications folder
3. Double-click to run
4. If prompted about unidentified developer:
   - Right-click the app, select Open
   - Click Open in the dialog

### Game Data

Place your original game data files in:
`~/Library/Application Support/RedAlert/data/`

Required files:
- REDALERT.MIX
- CONQUER.MIX
- LOCAL.MIX
- (etc.)
```

### Build Instructions

```markdown
## Building on macOS

### Requirements
- Xcode Command Line Tools
- CMake 3.20+
- SDL2 (installed via Homebrew or bundled)

### Build Steps
```bash
mkdir build && cd build
cmake .. -G "Xcode"
cmake --build . --config Release
```
```

## Tasks Breakdown

### Phase 1: Bug Fixes (5 days)
- [ ] Collect all known bugs
- [ ] Prioritize by severity
- [ ] Fix critical bugs
- [ ] Fix major bugs
- [ ] Fix minor bugs

### Phase 2: Performance (3 days)
- [ ] Profile with Instruments
- [ ] Identify bottlenecks
- [ ] Optimize critical paths
- [ ] Verify improvements

### Phase 3: macOS Integration (2 days)
- [ ] Retina display support
- [ ] Fullscreen handling
- [ ] Menu bar integration
- [ ] Application lifecycle

### Phase 4: Packaging (2 days)
- [ ] Create app bundle
- [ ] Universal binary
- [ ] Include resources
- [ ] Test installation

### Phase 5: Testing (3 days)
- [ ] Full playthrough testing
- [ ] Compatibility testing
- [ ] Edge case testing
- [ ] Final bug fixes

### Phase 6: Documentation (1 day)
- [ ] Update README
- [ ] Build instructions
- [ ] Known issues

## Acceptance Criteria

- [ ] Game fully playable single-player
- [ ] Game playable multiplayer
- [ ] No critical or major bugs
- [ ] Performance targets met
- [ ] Proper macOS app bundle
- [ ] Documentation complete

## Estimated Duration
**12-16 days**

## Dependencies
- All previous plans completed
- Game functional end-to-end

## Project Completion

Upon completing this phase:
1. Tag release in Git
2. Create release notes
3. Build final distribution
4. Celebrate! 🎉
