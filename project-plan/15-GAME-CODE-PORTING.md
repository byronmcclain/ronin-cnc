# Phase 15: Game Code Porting

## Overview

This phase identifies and ports the minimum viable subset of original C++ code required to run the game. Rather than porting all 277 files (~250K lines), we focus on critical systems.

---

## Porting Strategy

### Approach: Incremental Integration

1. **Identify critical files** - Core loop, rendering, assets
2. **Port in isolation** - One system at a time
3. **Replace, don't wrap** - Use platform layer directly
4. **Test continuously** - Verify each addition works

### What NOT to Port

- DOS-specific code (IPX/, LAUNCH/)
- Windows launcher (LAUNCHER/)
- DirectDraw/DirectSound implementation
- Modem/serial networking (use enet instead)
- Win32 event loop (use platform layer)

---

## Critical File Analysis

### Tier 1: Must Port (Core Game)

| File | Lines | Purpose | Dependencies |
|------|-------|---------|--------------|
| CONQUER.CPP | 5,558 | Main game loop | Everything |
| INIT.CPP | ~1,500 | Game initialization | MIX, palette |
| GSCREEN.CPP | ~200 | Base screen class | Display |
| DISPLAY.CPP | 4,509 | Tactical map | Map, Cell |
| MAP.CPP | ~2,000 | Map management | Cell |
| CELL.CPP | ~3,000 | Map cell handling | Template |
| OBJECT.CPP | ~1,500 | Base game object | Type classes |
| HOUSE.CPP | ~3,000 | Player/faction | Many |

### Tier 2: Required for Rendering

| File | Lines | Purpose | Dependencies |
|------|-------|---------|--------------|
| PALETTE.CPP | ~500 | Palette management | MIX |
| SHAPE.CPP | ~800 | Shape rendering | Buffer |
| BUFFER.CPP | ~600 | Graphics buffers | Platform |
| MOUSE.CPP | ~700 | Mouse cursor | Input, Shape |
| FONT.CPP | ~400 | Font rendering | Shape |

### Tier 3: Required for Gameplay

| File | Lines | Purpose | Dependencies |
|------|-------|---------|--------------|
| UNIT.CPP | 5,064 | Unit class | Techno, Drive |
| BUILDING.CPP | 5,712 | Building class | Techno |
| INFANTRY.CPP | 4,150 | Infantry class | Techno, Foot |
| AIRCRAFT.CPP | 4,375 | Aircraft class | Techno, Foot |
| TECHNO.CPP | ~2,500 | Base tech class | Object |
| MISSION.CPP | ~1,500 | AI missions | Object |

### Tier 4: Supporting Systems

| File | Lines | Purpose |
|------|-------|---------|
| INI.CPP | ~800 | Config parsing |
| RULES.CPP | ~1,200 | Game rules |
| SCENARIO.CPP | ~2,000 | Level loading |
| SIDEBAR.CPP | ~1,500 | Build menu |
| RADAR.CPP | ~800 | Minimap |

---

## Task 15.1: Port GSCREEN.CPP

The base class for all screen rendering.

**Original structure:**
```cpp
class GScreenClass {
protected:
    // Graphics buffer for rendering
    GraphicBufferClass* ShadowBuff;

public:
    GScreenClass();
    virtual ~GScreenClass();

    virtual void One_Time();     // One-time initialization
    virtual void Init(...)  ;    // Per-level init
    virtual void Render();       // Main render call

    // Gadget system (UI elements)
    void Add_Gadget(GadgetClass*);
    void Remove_Gadget(GadgetClass*);
};
```

**Ported version:**
```cpp
// src/game/gscreen.cpp
#include "compat/compat.h"
#include "platform.h"

class GScreenClass {
protected:
    uint8_t* back_buffer_;
    int32_t width_;
    int32_t height_;
    int32_t pitch_;

public:
    GScreenClass() : back_buffer_(nullptr), width_(0), height_(0), pitch_(0) {}

    virtual void One_Time() {
        // Initialize graphics
        Platform_Graphics_Init();
        Platform_Graphics_GetBackBuffer(&back_buffer_, &width_, &height_, &pitch_);
    }

    virtual void Init(int theater) {
        // Load theater-specific data
        // ... palette, tiles, etc.
    }

    virtual void Render() {
        // Lock buffer for drawing
        Platform_Graphics_LockBackBuffer();

        // Derived classes draw here
        // ...

        // Unlock and present
        Platform_Graphics_UnlockBackBuffer();
        Platform_Graphics_Flip();
    }

    // ... rest of class
};
```

---

## Task 15.2: Port Display Classes

The class hierarchy for map display:

```
GScreenClass
    ↓
MapClass
    ↓
DisplayClass
    ↓
RadarClass
    ↓
... etc
```

**Simplified approach:** Start with just GScreen + Display, add others incrementally.

```cpp
// src/game/display.cpp
#include "gscreen.h"

class MapClass : public GScreenClass {
protected:
    // Map data
    CellClass* cells_;
    int map_width_;
    int map_height_;

public:
    virtual void Init(int theater) override {
        GScreenClass::Init(theater);
        // Load map cells
    }
};

class DisplayClass : public MapClass {
protected:
    int tactical_x_;  // Viewport position
    int tactical_y_;
    int tactical_w_;
    int tactical_h_;

public:
    virtual void Render() override {
        // Draw visible map area
        Draw_Tactical();

        // Call base to flip
        GScreenClass::Render();
    }

    void Draw_Tactical() {
        // Draw terrain
        for (int y = tactical_y_; y < tactical_y_ + tactical_h_; y++) {
            for (int x = tactical_x_; x < tactical_x_ + tactical_w_; x++) {
                Draw_Cell(x, y);
            }
        }
    }

    void Draw_Cell(int x, int y) {
        CellClass* cell = Get_Cell(x, y);
        // Draw terrain tile
        // Draw overlay
        // Draw objects
    }
};
```

---

## Task 15.3: Port Object System

Minimal object class for game entities:

```cpp
// src/game/object.h
#pragma once

#include <cstdint>

// Coordinate type
typedef int32_t COORDINATE;
typedef int16_t CELL;

// Object types
enum RTTIType {
    RTTI_NONE,
    RTTI_UNIT,
    RTTI_INFANTRY,
    RTTI_BUILDING,
    RTTI_AIRCRAFT,
    RTTI_VESSEL,
    RTTI_BULLET,
    RTTI_ANIM,
    RTTI_TERRAIN,
    RTTI_OVERLAY,
    RTTI_SMUDGE,
};

class ObjectClass {
public:
    COORDINATE Coord;       // World position
    int Strength;           // Health
    bool IsActive;
    bool IsSelected;

    RTTIType What_Am_I() const { return rtti_type_; }

    virtual void Draw(int x, int y) = 0;
    virtual void AI() = 0;
    virtual bool Take_Damage(int damage, int source) = 0;

protected:
    RTTIType rtti_type_;
};
```

---

## Task 15.4: Port Main Game Loop

**Original CONQUER.CPP structure:**
```cpp
void Main_Loop() {
    while (!GameActive) {
        // Process Windows messages (REMOVE - use platform layer)
        // ...

        // Process input
        Keyboard_Process();
        Mouse_Process();

        // Game logic
        if (frame_timer) {
            AI();                // All objects think
            Logic.Process();     // Game state updates
        }

        // Render
        if (render_needed) {
            Map.Render();        // Draw everything
            Display.Render();
        }

        // Frame timing
        Frame_Limiter();
    }
}
```

**Ported version:**
```cpp
// src/game/game_loop.cpp
#include "compat/compat.h"
#include "platform.h"
#include "display.h"

static DisplayClass* TheDisplay = nullptr;
static bool GameRunning = true;

void Game_Init() {
    Platform_Init();

    // Mount MIX files
    MixManager::Instance().Initialize("gamedata");
    MixManager::Instance().Mount("REDALERT.MIX");

    // Create display
    TheDisplay = new DisplayClass();
    TheDisplay->One_Time();
}

void Game_Shutdown() {
    delete TheDisplay;
    MixManager::Instance().Shutdown();
    Platform_Shutdown();
}

void Game_Main_Loop() {
    while (GameRunning) {
        Platform_Frame_Begin();

        // Check for quit
        if (Platform_PollEvents()) {
            GameRunning = false;
            break;
        }

        // Process input
        Platform_Input_Update();

        // Handle keyboard
        if (Platform_Key_JustPressed(KEY_CODE_ESCAPE)) {
            GameRunning = false;
        }

        // Game logic (frame-limited)
        static uint32_t last_logic = 0;
        uint32_t now = Platform_Timer_GetTicks();
        if (now - last_logic >= 16) {  // ~60 FPS logic
            Game_Logic();
            last_logic = now;
        }

        // Render
        TheDisplay->Render();

        Platform_Frame_End();
    }
}

void Game_Logic() {
    // Update all game objects
    for (auto* obj : AllObjects) {
        obj->AI();
    }

    // Process construction
    // Process combat
    // etc.
}

int main(int argc, char* argv[]) {
    Game_Init();
    Game_Main_Loop();
    Game_Shutdown();
    return 0;
}
```

---

## Task 15.5: Port Cell/Map System

```cpp
// src/game/cell.h
#pragma once

#include <cstdint>

// Cell coordinates
typedef int16_t CELL;

// Land types
enum LandType {
    LAND_CLEAR,
    LAND_ROAD,
    LAND_WATER,
    LAND_ROCK,
    LAND_WALL,
    LAND_TIBERIUM,
    LAND_BEACH,
};

class CellClass {
public:
    uint8_t template_type;    // Terrain template
    uint8_t template_icon;    // Icon within template
    uint8_t overlay_type;     // Overlay (tiberium, walls)
    uint8_t overlay_data;

    LandType Land;
    bool IsVisible;           // Fog of war
    bool IsExplored;

    // Objects in this cell
    ObjectClass* CellObjects[4];

    void Draw(int screen_x, int screen_y, uint8_t* buffer, int pitch);
    void Recalc();
};

class MapClass {
public:
    static const int MAP_WIDTH = 128;
    static const int MAP_HEIGHT = 128;

    CellClass Cells[MAP_WIDTH * MAP_HEIGHT];

    CellClass& operator[](CELL cell) {
        return Cells[cell];
    }

    CellClass& Cell_At(int x, int y) {
        return Cells[y * MAP_WIDTH + x];
    }
};
```

---

## Task 15.6: Port Type Classes

Type classes define unit/building properties:

```cpp
// src/game/unit_type.h
#pragma once

struct UnitTypeClass {
    const char* Name;
    const char* FullName;

    int Speed;
    int Armor;
    int Strength;
    int Cost;
    int BuildTime;

    int SightRange;
    int PrimaryWeapon;

    // Shape data reference
    const char* ShapeName;
    int ShapeFrames;

    // Construction requirements
    int Prerequisites;
    int TechLevel;

    static UnitTypeClass Types[];
    static int TypeCount;
};

// Define in .cpp file
UnitTypeClass UnitTypeClass::Types[] = {
    // Name, Full, Speed, Armor, HP, Cost, Time, Sight, Weapon, Shape, Frames, Prereq, Tech
    {"MTNK", "Medium Tank", 10, 2, 400, 800, 100, 5, 1, "MTNK.SHP", 32, 0, 2},
    {"HTNK", "Heavy Tank", 8, 3, 600, 1500, 150, 4, 2, "HTNK.SHP", 32, 1, 5},
    {"LTNK", "Light Tank", 12, 1, 300, 600, 80, 6, 0, "LTNK.SHP", 32, 0, 1},
    // ... etc
};
```

---

## Build Integration

Update `CMakeLists.txt`:

```cmake
# Ported game code
set(GAME_SOURCES
    src/game/gscreen.cpp
    src/game/map.cpp
    src/game/display.cpp
    src/game/cell.cpp
    src/game/object.cpp
    src/game/unit.cpp
    src/game/building.cpp
    src/game/infantry.cpp
    src/game/game_loop.cpp
    src/game/init.cpp
)

add_executable(RedAlertGame
    src/main_game.cpp
    ${GAME_SOURCES}
)

target_include_directories(RedAlertGame PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/compat
    ${CMAKE_SOURCE_DIR}/src/game
)

target_link_libraries(RedAlertGame PRIVATE
    game_assets    # MIX file system
    compat         # Win32 stubs
    redalert_platform
)
```

---

## Verification Milestones

### Milestone 1: Empty Window
```bash
./build/RedAlertGame
# Opens window, shows black screen, responds to ESC
```

### Milestone 2: Palette Display
```bash
./build/RedAlertGame --test-palette
# Loads TEMPERAT.PAL, shows color gradient
```

### Milestone 3: Shape Display
```bash
./build/RedAlertGame --test-shape MTNK.SHP
# Loads and displays unit sprite
```

### Milestone 4: Map Display
```bash
./build/RedAlertGame --test-map SCG01EA.INI
# Loads scenario, renders terrain
```

### Milestone 5: Interactive Demo
```bash
./build/RedAlertGame --demo
# Mouse works, can scroll map, see cursor
```

---

## Common Porting Issues

### Issue 1: Coordinate Systems

Original uses fixed-point coordinates:
```cpp
// Original: COORD = pixel << 8
COORDINATE coord = Cell_To_Coord(cell);

// Ported: Use integer pixels or keep fixed-point
int pixel_x = coord >> 8;
int pixel_y = coord >> 8;
```

### Issue 2: Global Variables

Original uses many globals in EXTERNS.H:
```cpp
// Original
extern MapClass Map;
extern PlayerPtr House;
extern int Frame;

// Ported: Consider game state object
struct GameState {
    MapClass* Map;
    House* Player;
    int Frame;
};
extern GameState* Game;
```

### Issue 3: Virtual Functions

Original uses heavy virtual dispatch. Keep it but ensure vtables work:
```cpp
// Ensure virtual destructors
virtual ~ObjectClass() = default;
```

---

## File Mapping

| Original File | Ported File | Notes |
|---------------|-------------|-------|
| CODE/GSCREEN.CPP | src/game/gscreen.cpp | Simplified |
| CODE/DISPLAY.CPP | src/game/display.cpp | Platform calls |
| CODE/MAP.CPP | src/game/map.cpp | Minimal |
| CODE/CELL.CPP | src/game/cell.cpp | Core logic |
| CODE/CONQUER.CPP | src/game/game_loop.cpp | Platform loop |
| CODE/INIT.CPP | src/game/init.cpp | Adapted |
| CODE/OBJECT.CPP | src/game/object.cpp | Base class |
| (Type files) | src/game/*_type.cpp | Data tables |

---

## Success Criteria

- [ ] GScreenClass compiles and initializes
- [ ] MapClass can store cell data
- [ ] DisplayClass renders terrain
- [ ] Main loop runs at 60 FPS
- [ ] Input is processed correctly
- [ ] Clean shutdown without leaks
- [ ] At least one sprite renders

---

## Estimated Effort

| Task | Hours |
|------|-------|
| 15.1 GScreen | 4-6 |
| 15.2 Display Classes | 8-12 |
| 15.3 Object System | 6-8 |
| 15.4 Main Loop | 4-6 |
| 15.5 Cell/Map | 6-8 |
| 15.6 Type Classes | 4-6 |
| Integration | 4-6 |
| **Total** | **36-52** |
