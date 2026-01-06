# Plan 03: Graphics System

## Objective
Replace DirectDraw with SDL2-based rendering, supporting the original 8-bit palettized graphics modes (320x200, 640x400, 640x480) with proper scaling to modern displays.

## Current State Analysis

### DirectDraw Usage in Codebase

**Primary Files:**
- `WIN32LIB/MISC/DDRAW.CPP` (1001 lines) - Core DirectDraw wrapper
- `WIN32LIB/DRAWBUFF/GBUFFER.CPP` - Graphics buffer class
- `WIN32LIB/INCLUDE/GBUFFER.H` - Buffer definitions
- `CODE/STARTUP.CPP` - Display initialization

**Key DirectDraw Objects:**
```cpp
LPDIRECTDRAW DirectDrawObject;
LPDIRECTDRAW2 DirectDraw2Interface;
LPDIRECTDRAWSURFACE PaletteSurface;
LPDIRECTDRAWPALETTE PalettePtr;
PALETTEENTRY PaletteEntries[256];
```

**Display Modes Used:**
- 320x200x8 (ModeX-style, 2x scaled to 640x400)
- 640x400x8 (primary hi-res mode)
- 640x480x8 (alternate hi-res)

**Key Operations:**
1. Surface creation (primary, back buffer, off-screen)
2. Palette management (256 color palette)
3. Lock/Unlock for direct pixel access
4. Blit operations (copy, stretch, transparent)
5. Page flipping (double buffering)

## SDL2 Implementation Strategy

### Architecture

```
┌────────────────────────────────────────────────────────────────┐
│                        Game Code                                │
│              (uses GraphicBufferClass, etc.)                    │
└────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌────────────────────────────────────────────────────────────────┐
│                    Platform_Graphics_*                          │
│                 (platform/platform_graphics.h)                  │
└────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌────────────────────────────────────────────────────────────────┐
│                     SDL2 Rendering                              │
├────────────────────────────────────────────────────────────────┤
│  SDL_Window   │  SDL_Renderer  │  SDL_Texture  │  SDL_Surface  │
└────────────────────────────────────────────────────────────────┘
```

### Rendering Pipeline

For 8-bit palettized rendering on modern hardware:

```
1. Game writes to 8-bit buffer (index values 0-255)
                    │
                    ▼
2. On flip: Convert 8-bit → 32-bit RGBA using palette lookup
                    │
                    ▼
3. Upload to SDL_Texture (GPU)
                    │
                    ▼
4. SDL_RenderCopy with scaling to window size
                    │
                    ▼
5. SDL_RenderPresent (display)
```

## Implementation

### 3.1 Core Graphics State

File: `src/platform/graphics_sdl.cpp`
```cpp
#include "platform/platform_graphics.h"
#include <SDL.h>
#include <vector>
#include <cstring>

namespace {

// SDL objects
SDL_Window* g_window = nullptr;
SDL_Renderer* g_renderer = nullptr;
SDL_Texture* g_screen_texture = nullptr;

// Current display mode
DisplayMode g_display_mode = {640, 400, 8};

// 8-bit palette
PaletteEntry g_palette[256];
uint32_t g_palette_rgba[256];  // Pre-computed RGBA values

// Back buffer (8-bit indexed)
struct SoftwareSurface {
    std::vector<uint8_t> pixels;
    int width;
    int height;
    int pitch;
};

SoftwareSurface g_back_buffer;
SoftwareSurface g_front_buffer;

// Convert palette entry to RGBA
uint32_t PaletteToRGBA(const PaletteEntry& entry) {
    return (255 << 24) | (entry.r << 16) | (entry.g << 8) | entry.b;
}

// Rebuild RGBA palette lookup table
void RebuildPaletteLUT() {
    for (int i = 0; i < 256; i++) {
        g_palette_rgba[i] = PaletteToRGBA(g_palette[i]);
    }
}

} // anonymous namespace
```

### 3.2 Initialization

```cpp
bool Platform_Graphics_Init(void) {
    // Create window
    g_window = SDL_CreateWindow(
        "Command & Conquer: Red Alert",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        g_display_mode.width * 2,  // 2x scale
        g_display_mode.height * 2,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!g_window) {
        Platform_SetError("SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }

    // Create renderer
    g_renderer = SDL_CreateRenderer(
        g_window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!g_renderer) {
        Platform_SetError("SDL_CreateRenderer failed: %s", SDL_GetError());
        return false;
    }

    // Set scaling quality
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");  // Pixel-perfect scaling

    // Create streaming texture for screen
    g_screen_texture = SDL_CreateTexture(
        g_renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        g_display_mode.width,
        g_display_mode.height
    );

    if (!g_screen_texture) {
        Platform_SetError("SDL_CreateTexture failed: %s", SDL_GetError());
        return false;
    }

    // Initialize buffers
    int buffer_size = g_display_mode.width * g_display_mode.height;
    g_back_buffer.pixels.resize(buffer_size);
    g_back_buffer.width = g_display_mode.width;
    g_back_buffer.height = g_display_mode.height;
    g_back_buffer.pitch = g_display_mode.width;

    g_front_buffer = g_back_buffer;

    // Initialize palette to grayscale
    for (int i = 0; i < 256; i++) {
        g_palette[i] = {(uint8_t)i, (uint8_t)i, (uint8_t)i};
    }
    RebuildPaletteLUT();

    return true;
}

void Platform_Graphics_Shutdown(void) {
    if (g_screen_texture) {
        SDL_DestroyTexture(g_screen_texture);
        g_screen_texture = nullptr;
    }
    if (g_renderer) {
        SDL_DestroyRenderer(g_renderer);
        g_renderer = nullptr;
    }
    if (g_window) {
        SDL_DestroyWindow(g_window);
        g_window = nullptr;
    }
}
```

### 3.3 Display Mode Switching

```cpp
bool Platform_Graphics_SetMode(const DisplayMode* mode) {
    g_display_mode = *mode;

    // Recreate texture at new size
    if (g_screen_texture) {
        SDL_DestroyTexture(g_screen_texture);
    }

    g_screen_texture = SDL_CreateTexture(
        g_renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        mode->width,
        mode->height
    );

    // Resize buffers
    int buffer_size = mode->width * mode->height;
    g_back_buffer.pixels.resize(buffer_size);
    g_back_buffer.width = mode->width;
    g_back_buffer.height = mode->height;
    g_back_buffer.pitch = mode->width;

    g_front_buffer = g_back_buffer;

    // Resize window to maintain aspect ratio with 2x scale
    SDL_SetWindowSize(g_window, mode->width * 2, mode->height * 2);

    return true;
}

void Platform_Graphics_GetMode(DisplayMode* mode) {
    *mode = g_display_mode;
}
```

### 3.4 Surface Implementation

```cpp
struct PlatformSurface {
    std::vector<uint8_t> pixels;
    int width;
    int height;
    int pitch;
    int bpp;
    bool locked;

    PlatformSurface(int w, int h, int bits)
        : width(w), height(h), bpp(bits), locked(false) {
        pitch = w * (bits / 8);
        pixels.resize(pitch * h);
    }
};

PlatformSurface* Platform_Surface_Create(int width, int height, int bpp) {
    return new PlatformSurface(width, height, bpp);
}

void Platform_Surface_Destroy(PlatformSurface* surface) {
    delete surface;
}

PlatformSurface* Platform_Graphics_GetBackBuffer(void) {
    // Return a wrapper around our back buffer
    static PlatformSurface back_buffer_wrapper(0, 0, 8);
    back_buffer_wrapper.width = g_back_buffer.width;
    back_buffer_wrapper.height = g_back_buffer.height;
    back_buffer_wrapper.pitch = g_back_buffer.pitch;
    back_buffer_wrapper.bpp = 8;
    back_buffer_wrapper.pixels.assign(
        g_back_buffer.pixels.begin(),
        g_back_buffer.pixels.end()
    );
    return &back_buffer_wrapper;
}

bool Platform_Surface_Lock(PlatformSurface* surface, void** pixels, int* pitch) {
    if (surface->locked) return false;

    *pixels = surface->pixels.data();
    *pitch = surface->pitch;
    surface->locked = true;
    return true;
}

void Platform_Surface_Unlock(PlatformSurface* surface) {
    surface->locked = false;
}
```

### 3.5 Blitting Operations

```cpp
bool Platform_Surface_Blit(PlatformSurface* dest, int dx, int dy,
                           PlatformSurface* src, int sx, int sy, int sw, int sh) {
    // Bounds checking
    if (dx < 0 || dy < 0 || dx + sw > dest->width || dy + sh > dest->height) {
        // Clip the blit
        if (dx < 0) { sx -= dx; sw += dx; dx = 0; }
        if (dy < 0) { sy -= dy; sh += dy; dy = 0; }
        if (dx + sw > dest->width) sw = dest->width - dx;
        if (dy + sh > dest->height) sh = dest->height - dy;
        if (sw <= 0 || sh <= 0) return true;
    }

    // Copy pixels
    uint8_t* dst_ptr = dest->pixels.data() + dy * dest->pitch + dx;
    uint8_t* src_ptr = src->pixels.data() + sy * src->pitch + sx;

    for (int y = 0; y < sh; y++) {
        memcpy(dst_ptr, src_ptr, sw);
        dst_ptr += dest->pitch;
        src_ptr += src->pitch;
    }

    return true;
}

void Platform_Surface_Fill(PlatformSurface* surface, int x, int y, int w, int h, uint8_t color) {
    // Clip to surface bounds
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > surface->width) w = surface->width - x;
    if (y + h > surface->height) h = surface->height - y;
    if (w <= 0 || h <= 0) return;

    uint8_t* ptr = surface->pixels.data() + y * surface->pitch + x;
    for (int row = 0; row < h; row++) {
        memset(ptr, color, w);
        ptr += surface->pitch;
    }
}

void Platform_Surface_Clear(PlatformSurface* surface, uint8_t color) {
    memset(surface->pixels.data(), color, surface->pixels.size());
}
```

### 3.6 Palette Management

```cpp
void Platform_Graphics_SetPalette(const PaletteEntry* palette, int start, int count) {
    if (start < 0 || start + count > 256) return;

    memcpy(&g_palette[start], palette, count * sizeof(PaletteEntry));

    // Update RGBA lookup table
    for (int i = start; i < start + count; i++) {
        g_palette_rgba[i] = PaletteToRGBA(g_palette[i]);
    }
}

void Platform_Graphics_GetPalette(PaletteEntry* palette, int start, int count) {
    if (start < 0 || start + count > 256) return;
    memcpy(palette, &g_palette[start], count * sizeof(PaletteEntry));
}
```

### 3.7 Page Flipping

```cpp
void Platform_Graphics_Flip(void) {
    // Convert 8-bit back buffer to 32-bit RGBA
    void* texture_pixels;
    int texture_pitch;

    SDL_LockTexture(g_screen_texture, nullptr, &texture_pixels, &texture_pitch);

    uint32_t* dst = static_cast<uint32_t*>(texture_pixels);
    uint8_t* src = g_back_buffer.pixels.data();

    for (int y = 0; y < g_display_mode.height; y++) {
        for (int x = 0; x < g_display_mode.width; x++) {
            dst[x] = g_palette_rgba[src[x]];
        }
        dst = reinterpret_cast<uint32_t*>(
            reinterpret_cast<uint8_t*>(dst) + texture_pitch
        );
        src += g_back_buffer.pitch;
    }

    SDL_UnlockTexture(g_screen_texture);

    // Clear and render
    SDL_RenderClear(g_renderer);
    SDL_RenderCopy(g_renderer, g_screen_texture, nullptr, nullptr);
    SDL_RenderPresent(g_renderer);

    // Swap buffers
    std::swap(g_back_buffer, g_front_buffer);
}

void Platform_Graphics_WaitVSync(void) {
    // VSync is handled by SDL_RENDERER_PRESENTVSYNC flag
    // This is a no-op when using SDL renderer
}
```

### 3.8 DirectDraw Compatibility Wrapper

File: `include/compat/ddraw_wrapper.h`
```cpp
#ifndef DDRAW_WRAPPER_H
#define DDRAW_WRAPPER_H

#include "platform/platform_graphics.h"

// Map DirectDraw calls to Platform calls
inline HRESULT DirectDrawCreate(void* guid, LPDIRECTDRAW* dd, void* outer) {
    if (Platform_Graphics_Init()) {
        *dd = (LPDIRECTDRAW)1;  // Non-null dummy pointer
        return S_OK;
    }
    return E_FAIL;
}

// GraphicBufferClass wrapper
class GraphicBufferClass {
private:
    PlatformSurface* surface;

public:
    GraphicBufferClass(int width, int height, int bpp = 8) {
        surface = Platform_Surface_Create(width, height, bpp);
    }

    ~GraphicBufferClass() {
        Platform_Surface_Destroy(surface);
    }

    void* Lock() {
        void* pixels;
        int pitch;
        Platform_Surface_Lock(surface, &pixels, &pitch);
        return pixels;
    }

    void Unlock() {
        Platform_Surface_Unlock(surface);
    }

    int Get_Width() const { return surface->width; }
    int Get_Height() const { return surface->height; }
    int Get_Pitch() const { return surface->pitch; }

    // Blit operations
    void Blit(GraphicBufferClass& dest, int x, int y) {
        Platform_Surface_Blit(dest.surface, x, y,
                              surface, 0, 0, surface->width, surface->height);
    }
};

#endif // DDRAW_WRAPPER_H
```

## Tasks Breakdown

### Phase 1: Basic Window (2 days)
- [ ] Create `graphics_sdl.cpp` with initialization
- [ ] Open window at 640x400 (2x scaled to 1280x800)
- [ ] Display solid color to verify rendering works
- [ ] Handle window close event

### Phase 2: 8-bit Rendering (3 days)
- [ ] Implement 8-bit buffer management
- [ ] Implement palette-to-RGBA conversion
- [ ] Test with gradient pattern
- [ ] Implement `Platform_Graphics_Flip()`

### Phase 3: Surface Operations (2 days)
- [ ] Implement `PlatformSurface` structure
- [ ] Implement Lock/Unlock
- [ ] Implement Blit operations
- [ ] Implement Fill/Clear

### Phase 4: DirectDraw Wrapper (2 days)
- [ ] Create compatibility wrapper in `ddraw_wrapper.h`
- [ ] Map DirectDraw function calls
- [ ] Map `GraphicBufferClass` methods
- [ ] Test with actual game code

### Phase 5: Integration & Debug (2 days)
- [ ] Hook up to game's display initialization
- [ ] Debug palette loading from game assets
- [ ] Verify main menu renders correctly
- [ ] Test resolution switching

## Testing Strategy

### Test 1: Color Bars
Display 256 vertical color bars using current palette.

### Test 2: Palette Animation
Cycle palette entries to verify palette updates work.

### Test 3: Blit Test
Draw checkerboard pattern using blit operations.

### Test 4: Game Integration
Load and display the main menu background.

## Performance Considerations

1. **Palette Lookup Table**: Pre-compute RGBA values on palette change
2. **Batch Updates**: Only update texture on flip, not every draw
3. **SIMD Optimization**: Consider using SSE for palette conversion (future)
4. **Dirty Rectangles**: Track changed regions (optional optimization)

## Known Issues to Address

1. **Aspect Ratio**: Original 320x200 was displayed on 4:3 CRTs
   - Consider adding letterboxing option for correct aspect ratio

2. **Fullscreen**: Need to implement proper fullscreen toggle (Alt+Enter)

3. **HiDPI**: macOS Retina displays need proper scaling

## Acceptance Criteria

- [ ] Window opens at correct resolution
- [ ] 8-bit palettized graphics render correctly
- [ ] Palette changes are reflected immediately
- [ ] Page flipping works (no tearing)
- [ ] Basic blit operations work
- [ ] Game main menu is visible

## Estimated Duration
**7-9 days**

## Dependencies
- Plan 01 (Build System) completed
- Plan 02 (Platform Abstraction) headers in place
- SDL2 linked and available

## Next Plan
Once graphics are working, proceed to **Plan 04: Input System**
