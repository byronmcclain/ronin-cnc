# Phase 16: Graphics Integration

## Overview

This phase connects the ported game rendering code to the platform graphics layer, enabling visual display of game content.

---

## Platform Graphics API

### Available Functions (from platform.h)

```c
// Initialization
int Platform_Graphics_Init(void);
void Platform_Graphics_Shutdown(void);
bool Platform_Graphics_IsInitialized(void);

// Display modes
void Platform_Graphics_GetMode(DisplayMode* mode);
int Platform_Graphics_SetMode(int width, int height, int scale);
void Platform_ToggleFullscreen(void);

// Back buffer access
int Platform_Graphics_GetBackBuffer(uint8_t** pixels,
    int32_t* width, int32_t* height, int32_t* pitch);
void Platform_Graphics_LockBackBuffer(void);
void Platform_Graphics_UnlockBackBuffer(void);
void Platform_Graphics_ClearBackBuffer(uint8_t color);

// Palette
void Platform_Graphics_SetPalette(const uint8_t* palette_rgb);
void Platform_Graphics_SetPaletteEntry(int index, uint8_t r, uint8_t g, uint8_t b);
void Platform_Graphics_GetPalette(uint8_t* palette_rgb);

// Display
int Platform_Graphics_Flip(void);
```

### Display Configuration

- **Resolution:** 640x400 (original was 640x400 or 320x200)
- **Color depth:** 8-bit indexed (256 colors)
- **Window scale:** 2x by default (1280x800 actual window)
- **Frame rate:** ~60 FPS target

---

## Task 16.1: GraphicBuffer Replacement

Replace original `GraphicBufferClass` with platform layer access.

**Original pattern:**
```cpp
// WIN32LIB GraphicBufferClass
GraphicBufferClass buffer(640, 400);
buffer.Lock();
uint8_t* pixels = buffer.Get_Buffer();
// ... draw ...
buffer.Unlock();
buffer.Blit_To_Primary();
```

**Platform layer pattern:**
```cpp
// Using platform layer directly
uint8_t* pixels;
int32_t width, height, pitch;

Platform_Graphics_LockBackBuffer();
Platform_Graphics_GetBackBuffer(&pixels, &width, &height, &pitch);

// Draw at (x, y)
pixels[y * pitch + x] = color_index;

Platform_Graphics_UnlockBackBuffer();
Platform_Graphics_Flip();
```

**Wrapper class (optional):**
```cpp
// src/game/graphics_buffer.h
class GraphicsBuffer {
public:
    GraphicsBuffer() : pixels_(nullptr), width_(0), height_(0), pitch_(0) {}

    void Lock() {
        Platform_Graphics_LockBackBuffer();
        Platform_Graphics_GetBackBuffer(&pixels_, &width_, &height_, &pitch_);
    }

    void Unlock() {
        Platform_Graphics_UnlockBackBuffer();
    }

    uint8_t* Get_Buffer() { return pixels_; }
    int Get_Width() { return width_; }
    int Get_Height() { return height_; }
    int Get_Pitch() { return pitch_; }

    void Put_Pixel(int x, int y, uint8_t color) {
        if (x >= 0 && x < width_ && y >= 0 && y < height_) {
            pixels_[y * pitch_ + x] = color;
        }
    }

    void Clear(uint8_t color = 0) {
        Platform_Graphics_ClearBackBuffer(color);
    }

private:
    uint8_t* pixels_;
    int32_t width_, height_, pitch_;
};

// Global instance
extern GraphicsBuffer ScreenBuffer;
```

---

## Task 16.2: Shape Drawing

Implement shape/sprite rendering using platform buffer.

```cpp
// src/game/shape_draw.cpp
#include "shape.h"
#include "graphics_buffer.h"

void ShapeData::Draw(GraphicsBuffer& buffer, int x, int y, int frame) {
    if (frame < 0 || frame >= frames.size()) return;

    const ShapeFrame& f = frames[frame];
    uint8_t* dest = buffer.Get_Buffer();
    int pitch = buffer.Get_Pitch();

    int draw_x = x + f.x_offset;
    int draw_y = y + f.y_offset;

    for (int row = 0; row < f.height; row++) {
        int screen_y = draw_y + row;
        if (screen_y < 0 || screen_y >= buffer.Get_Height()) continue;

        for (int col = 0; col < f.width; col++) {
            int screen_x = draw_x + col;
            if (screen_x < 0 || screen_x >= buffer.Get_Width()) continue;

            uint8_t pixel = f.pixels[row * f.width + col];
            if (pixel != 0) {  // 0 = transparent
                dest[screen_y * pitch + screen_x] = pixel;
            }
        }
    }
}

// Draw with color remapping (for house colors)
void ShapeData::DrawRemapped(GraphicsBuffer& buffer, int x, int y,
                             int frame, const uint8_t* remap) {
    if (frame < 0 || frame >= frames.size()) return;

    const ShapeFrame& f = frames[frame];
    uint8_t* dest = buffer.Get_Buffer();
    int pitch = buffer.Get_Pitch();

    int draw_x = x + f.x_offset;
    int draw_y = y + f.y_offset;

    for (int row = 0; row < f.height; row++) {
        int screen_y = draw_y + row;
        if (screen_y < 0 || screen_y >= buffer.Get_Height()) continue;

        for (int col = 0; col < f.width; col++) {
            int screen_x = draw_x + col;
            if (screen_x < 0 || screen_x >= buffer.Get_Width()) continue;

            uint8_t pixel = f.pixels[row * f.width + col];
            if (pixel != 0) {
                dest[screen_y * pitch + screen_x] = remap[pixel];
            }
        }
    }
}
```

---

## Task 16.3: Tile/Template Drawing

Draw terrain tiles from theater data.

```cpp
// src/game/tile_draw.cpp
#include "template.h"
#include "graphics_buffer.h"

// Tile size in pixels
const int TILE_WIDTH = 24;
const int TILE_HEIGHT = 24;

void Draw_Tile(GraphicsBuffer& buffer, int screen_x, int screen_y,
               const TileTemplate& tmpl, int icon_index) {

    if (icon_index < 0 || icon_index >= tmpl.tiles.size()) return;

    const uint8_t* tile = tmpl.tiles[icon_index];
    uint8_t* dest = buffer.Get_Buffer();
    int pitch = buffer.Get_Pitch();

    for (int row = 0; row < TILE_HEIGHT; row++) {
        int sy = screen_y + row;
        if (sy < 0 || sy >= buffer.Get_Height()) continue;

        for (int col = 0; col < TILE_WIDTH; col++) {
            int sx = screen_x + col;
            if (sx < 0 || sx >= buffer.Get_Width()) continue;

            uint8_t pixel = tile[row * TILE_WIDTH + col];
            dest[sy * pitch + sx] = pixel;
        }
    }
}

// Draw map cell
void CellClass::Draw(GraphicsBuffer& buffer, int screen_x, int screen_y) {
    // Get template
    const TileTemplate& tmpl = TheaterTemplates[template_type];

    // Draw base terrain
    Draw_Tile(buffer, screen_x, screen_y, tmpl, template_icon);

    // Draw overlay if present
    if (overlay_type != 0) {
        const TileTemplate& overlay = TheaterOverlays[overlay_type];
        Draw_Tile(buffer, screen_x, screen_y, overlay, overlay_data);
    }
}
```

---

## Task 16.4: Palette Management

```cpp
// src/game/palette_mgr.cpp
#include "platform.h"

class PaletteManager {
public:
    static PaletteManager& Instance();

    // Load palette from MIX file
    bool LoadPalette(const char* name) {
        uint32_t size;
        const uint8_t* data = MixManager::Instance().FindFile(name, &size);
        if (!data || size < 768) return false;

        // Copy and convert 6-bit to 8-bit
        for (int i = 0; i < 256; i++) {
            palette_[i * 3 + 0] = data[i * 3 + 0] << 2;  // R
            palette_[i * 3 + 1] = data[i * 3 + 1] << 2;  // G
            palette_[i * 3 + 2] = data[i * 3 + 2] << 2;  // B
        }

        return true;
    }

    // Apply to platform layer
    void Apply() {
        Platform_Graphics_SetPalette(palette_);
    }

    // Set single entry
    void SetEntry(int index, uint8_t r, uint8_t g, uint8_t b) {
        palette_[index * 3 + 0] = r;
        palette_[index * 3 + 1] = g;
        palette_[index * 3 + 2] = b;
        Platform_Graphics_SetPaletteEntry(index, r, g, b);
    }

    // Palette effects
    void FadeIn(int steps);
    void FadeOut(int steps);
    void Flash(uint8_t r, uint8_t g, uint8_t b, int duration);

private:
    uint8_t palette_[768];  // 256 * RGB
    uint8_t original_[768]; // For fade effects
};
```

---

## Task 16.5: Mouse Cursor Rendering

```cpp
// src/game/mouse_draw.cpp
#include "shape.h"
#include "graphics_buffer.h"

class MouseCursor {
public:
    bool Load(const char* shape_name) {
        return shape_.LoadFromMix(shape_name);
    }

    void SetFrame(int frame) {
        current_frame_ = frame;
    }

    void Draw(GraphicsBuffer& buffer, int mouse_x, int mouse_y) {
        shape_.Draw(buffer, mouse_x, mouse_y, current_frame_);
    }

    // Standard cursor frames
    enum CursorType {
        CURSOR_NORMAL = 0,
        CURSOR_MOVE = 1,
        CURSOR_ATTACK = 2,
        CURSOR_SELECT = 3,
        CURSOR_NO_MOVE = 4,
        CURSOR_SELL = 5,
        CURSOR_REPAIR = 6,
        // ... etc
    };

private:
    ShapeData shape_;
    int current_frame_ = 0;
};

extern MouseCursor TheMouse;
```

---

## Task 16.6: Screen Rendering Pipeline

```cpp
// src/game/render.cpp

void Render_Frame() {
    // Lock buffer
    ScreenBuffer.Lock();
    ScreenBuffer.Clear(0);

    // 1. Draw terrain
    Render_Terrain();

    // 2. Draw shadows
    Render_Shadows();

    // 3. Draw ground objects (infantry, units)
    Render_Ground_Layer();

    // 4. Draw buildings
    Render_Buildings();

    // 5. Draw air units
    Render_Air_Layer();

    // 6. Draw effects (explosions, smoke)
    Render_Effects();

    // 7. Draw UI elements
    Render_Sidebar();
    Render_Radar();

    // 8. Draw mouse cursor (last)
    int mx, my;
    Platform_Mouse_GetPosition(&mx, &my);
    mx /= 2;  // Account for 2x scale
    my /= 2;
    TheMouse.Draw(ScreenBuffer, mx, my);

    // Unlock and flip
    ScreenBuffer.Unlock();
    Platform_Graphics_Flip();
}

void Render_Terrain() {
    int start_cell_x = viewport_x / TILE_WIDTH;
    int start_cell_y = viewport_y / TILE_HEIGHT;
    int end_cell_x = start_cell_x + (SCREEN_WIDTH / TILE_WIDTH) + 1;
    int end_cell_y = start_cell_y + (SCREEN_HEIGHT / TILE_HEIGHT) + 1;

    for (int cy = start_cell_y; cy <= end_cell_y; cy++) {
        for (int cx = start_cell_x; cx <= end_cell_x; cx++) {
            if (!Map.Is_Valid_Cell(cx, cy)) continue;

            CellClass& cell = Map.Cell_At(cx, cy);
            int screen_x = cx * TILE_WIDTH - viewport_x;
            int screen_y = cy * TILE_HEIGHT - viewport_y;

            cell.Draw(ScreenBuffer, screen_x, screen_y);
        }
    }
}

void Render_Ground_Layer() {
    for (auto* obj : Map.Get_Layer(LAYER_GROUND)) {
        if (!Is_Visible(obj)) continue;

        int screen_x, screen_y;
        World_To_Screen(obj->Coord, screen_x, screen_y);
        obj->Draw(screen_x, screen_y);
    }
}
```

---

## Task 16.7: Viewport/Scrolling

```cpp
// src/game/viewport.cpp

class Viewport {
public:
    int x, y;        // Top-left corner in world pixels
    int width;       // Viewport width (usually 640 - sidebar)
    int height;      // Viewport height (usually 400 - top bar)

    void Init() {
        x = y = 0;
        width = 480;   // 640 - 160 (sidebar)
        height = 368;  // 400 - 32 (top bar)
    }

    void Scroll(int dx, int dy) {
        x += dx;
        y += dy;

        // Clamp to map bounds
        int max_x = (Map.Width() * TILE_WIDTH) - width;
        int max_y = (Map.Height() * TILE_HEIGHT) - height;

        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x > max_x) x = max_x;
        if (y > max_y) y = max_y;
    }

    void Center_On(COORDINATE coord) {
        int world_x = Coord_X(coord);
        int world_y = Coord_Y(coord);

        x = world_x - width / 2;
        y = world_y - height / 2;

        // Clamp
        Scroll(0, 0);
    }

    bool Is_Visible(COORDINATE coord) {
        int wx = Coord_X(coord);
        int wy = Coord_Y(coord);
        return (wx >= x && wx < x + width &&
                wy >= y && wy < y + height);
    }
};

extern Viewport TheViewport;
```

---

## Platform Blitting Functions

The platform layer includes optimized blitting (from Phase 09):

```c
// Available in platform.h

// Basic buffer operations
void Platform_Buffer_Clear(uint8_t* buffer, int size, uint8_t color);
void Platform_Buffer_Copy(uint8_t* dest, const uint8_t* src, int size);

// Blitting with features
void Platform_Blit_Copy(uint8_t* dest, int dest_pitch,
                        const uint8_t* src, int src_pitch,
                        int width, int height);

void Platform_Blit_Remap(uint8_t* dest, int dest_pitch,
                         const uint8_t* src, int src_pitch,
                         int width, int height,
                         const uint8_t* remap_table);

void Platform_Blit_Trans(uint8_t* dest, int dest_pitch,
                         const uint8_t* src, int src_pitch,
                         int width, int height,
                         uint8_t trans_color);

void Platform_Blit_Scale(uint8_t* dest, int dest_w, int dest_h, int dest_pitch,
                         const uint8_t* src, int src_w, int src_h, int src_pitch);
```

Use these for performance-critical rendering.

---

## Testing

### Visual Tests

```cpp
// test_graphics.cpp

void Test_Clear_Screen() {
    Platform_Graphics_Init();
    for (int i = 0; i < 256; i++) {
        Platform_Graphics_ClearBackBuffer(i);
        Platform_Graphics_Flip();
        Platform_Timer_Delay(10);
    }
    Platform_Graphics_Shutdown();
}

void Test_Draw_Pattern() {
    Platform_Graphics_Init();

    uint8_t* pixels;
    int32_t w, h, pitch;
    Platform_Graphics_GetBackBuffer(&pixels, &w, &h, &pitch);

    // Draw color gradient
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            pixels[y * pitch + x] = (x + y) & 0xFF;
        }
    }

    Platform_Graphics_Flip();
    Platform_Timer_Delay(2000);
    Platform_Graphics_Shutdown();
}

void Test_Shape_Draw() {
    // Load and display a shape file
    ShapeData shape;
    shape.LoadFromMix("MOUSE.SHP");

    for (int frame = 0; frame < shape.GetFrameCount(); frame++) {
        ScreenBuffer.Lock();
        ScreenBuffer.Clear(0);
        shape.Draw(ScreenBuffer, 320, 200, frame);
        ScreenBuffer.Unlock();
        Platform_Graphics_Flip();
        Platform_Timer_Delay(100);
    }
}
```

---

## Success Criteria

- [ ] Screen clears to solid color
- [ ] Palette loads and displays correctly
- [ ] Single shape/sprite renders
- [ ] Terrain tiles render
- [ ] Map scrolls smoothly
- [ ] Mouse cursor follows pointer
- [ ] 60 FPS maintained
- [ ] No visual artifacts

---

## Estimated Effort

| Task | Hours |
|------|-------|
| 16.1 Buffer Wrapper | 2-3 |
| 16.2 Shape Drawing | 4-6 |
| 16.3 Tile Drawing | 3-4 |
| 16.4 Palette Manager | 2-3 |
| 16.5 Mouse Cursor | 2-3 |
| 16.6 Render Pipeline | 6-8 |
| 16.7 Viewport | 3-4 |
| Testing | 3-4 |
| **Total** | **25-35** |
