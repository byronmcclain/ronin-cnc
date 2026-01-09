# Task 14c: GScreenClass Port

## Dependencies
- Task 14a (Project Structure Setup) must be complete
- Task 14b (Coordinate Types) must be complete
- Platform layer graphics functions must be available

## Context
GScreenClass is the base class for all screen rendering in Red Alert. It sits at the top of the display class hierarchy and manages the graphics buffer, rendering loop, and UI gadget system. All display classes inherit from it.

## Objective
Port the essential GScreenClass functionality:
1. Back buffer management
2. Virtual rendering pipeline
3. Screen dimensions and pitch
4. Basic gadget system foundation
5. One_Time() and Init() lifecycle methods

## Technical Background

### Original Class Hierarchy (from CODE/FUNCTION.H)
```
GScreenClass          <- We're porting this
    |
    +-- MapClass      <- Task 14d
        |
        +-- DisplayClass  <- Task 14f
            |
            +-- RadarClass
                |
                +-- ... (more derived classes)
```

### Original GScreenClass (from CODE/GSCREEN.H)
The original manages:
- GraphicBufferClass pointer for rendering
- Gadget list for UI elements (buttons, sliders)
- Virtual functions for initialization and rendering

### Platform Layer Integration
Instead of DirectDraw, we use our platform layer:
- `Platform_Graphics_Init()` - Initialize graphics
- `Platform_Graphics_GetBackBuffer()` - Get render target
- `Platform_Graphics_Flip()` - Present frame

## Deliverables
- [ ] `include/game/gscreen.h` - GScreenClass header
- [ ] `src/game/display/gscreen.cpp` - GScreenClass implementation
- [ ] `include/game/gadget.h` - Basic gadget types (stub)
- [ ] Integration with platform layer graphics
- [ ] Unit test for basic rendering
- [ ] Verification script passes

## Files to Create

### include/game/gscreen.h (NEW)
```cpp
/**
 * GScreenClass - Base Screen Rendering Class
 *
 * This is the root of the display class hierarchy. It manages the
 * graphics buffer, rendering lifecycle, and UI gadget system.
 *
 * Original location: CODE/GSCREEN.H, CODE/GSCREEN.CPP
 *
 * Class Hierarchy:
 *   GScreenClass        <- This class
 *       |
 *       +-- MapClass
 *           |
 *           +-- DisplayClass
 *               |
 *               +-- RadarClass
 *                   |
 *                   +-- ... (additional derived classes)
 */

#pragma once

#include <cstdint>
#include <cstddef>

// Forward declarations
class GadgetClass;

// =============================================================================
// Screen Constants
// =============================================================================

// Standard Red Alert screen resolution
constexpr int SCREEN_WIDTH = 640;
constexpr int SCREEN_HEIGHT = 400;

// High-res mode (used in later patches)
constexpr int SCREEN_WIDTH_HIRES = 640;
constexpr int SCREEN_HEIGHT_HIRES = 480;

// Buffer format
constexpr int SCREEN_BPP = 8;  // 8-bit palettized

// =============================================================================
// GScreenClass
// =============================================================================

/**
 * GScreenClass - Base class for all game screens
 *
 * Provides:
 * - Back buffer access for rendering
 * - Virtual render pipeline (One_Time, Init, Render)
 * - Gadget management for UI elements
 *
 * Usage:
 *   Derived classes override Render() to draw content,
 *   then call base class to flip buffers.
 */
class GScreenClass {
public:
    // -------------------------------------------------------------------------
    // Construction / Destruction
    // -------------------------------------------------------------------------

    GScreenClass();
    virtual ~GScreenClass();

    // Non-copyable
    GScreenClass(const GScreenClass&) = delete;
    GScreenClass& operator=(const GScreenClass&) = delete;

    // -------------------------------------------------------------------------
    // Lifecycle Methods (virtual)
    // -------------------------------------------------------------------------

    /**
     * One_Time - Called once at game startup
     *
     * Performs global initialization:
     * - Initialize graphics subsystem
     * - Allocate back buffer
     * - Load common resources
     *
     * Derived classes should call base first.
     */
    virtual void One_Time();

    /**
     * Init - Called at scenario/level start
     *
     * Performs per-level initialization:
     * - Load theater-specific data
     * - Reset screen state
     * - Clear gadgets
     *
     * @param theater Theater type (TEMPERATE, SNOW, INTERIOR)
     *
     * Derived classes should call base first.
     */
    virtual void Init(int theater = 0);

    /**
     * Clear - Clear the back buffer
     *
     * Fills the buffer with the specified color (default black).
     */
    virtual void Clear(uint8_t color = 0);

    /**
     * Render - Main render method
     *
     * Called each frame to draw screen contents.
     * Derived classes draw their content, then call base to flip.
     *
     * Base implementation:
     * 1. Process gadgets
     * 2. Flip buffer to display
     */
    virtual void Render();

    /**
     * Flip - Flip back buffer to screen
     *
     * Presents the rendered frame. Usually called by Render().
     */
    virtual void Flip();

    // -------------------------------------------------------------------------
    // Buffer Access
    // -------------------------------------------------------------------------

    /**
     * Lock - Lock back buffer for drawing
     *
     * Must be called before any drawing operations.
     * Returns pointer to buffer start.
     */
    uint8_t* Lock();

    /**
     * Unlock - Unlock back buffer after drawing
     *
     * Call when done with drawing operations.
     */
    void Unlock();

    /**
     * Is_Locked - Check if buffer is currently locked
     */
    bool Is_Locked() const { return is_locked_; }

    /**
     * Get_Buffer - Get raw buffer pointer
     *
     * Buffer must be locked first.
     */
    uint8_t* Get_Buffer() const { return back_buffer_; }

    /**
     * Get_Width - Get screen width in pixels
     */
    int Get_Width() const { return width_; }

    /**
     * Get_Height - Get screen height in pixels
     */
    int Get_Height() const { return height_; }

    /**
     * Get_Pitch - Get buffer pitch (bytes per row)
     *
     * This may be larger than width due to alignment.
     */
    int Get_Pitch() const { return pitch_; }

    // -------------------------------------------------------------------------
    // Buffer Drawing Helpers
    // -------------------------------------------------------------------------

    /**
     * Put_Pixel - Set a single pixel
     *
     * @param x X coordinate
     * @param y Y coordinate
     * @param color Palette index
     */
    void Put_Pixel(int x, int y, uint8_t color);

    /**
     * Get_Pixel - Get pixel color
     *
     * @param x X coordinate
     * @param y Y coordinate
     * @return Palette index
     */
    uint8_t Get_Pixel(int x, int y) const;

    /**
     * Draw_Rect - Draw filled rectangle
     *
     * @param x Left edge
     * @param y Top edge
     * @param w Width
     * @param h Height
     * @param color Palette index
     */
    void Draw_Rect(int x, int y, int w, int h, uint8_t color);

    /**
     * Draw_Line - Draw line (horizontal or vertical only)
     *
     * @param x1, y1 Start point
     * @param x2, y2 End point
     * @param color Palette index
     */
    void Draw_Line(int x1, int y1, int x2, int y2, uint8_t color);

    // -------------------------------------------------------------------------
    // Gadget Management
    // -------------------------------------------------------------------------

    /**
     * Add_Gadget - Add UI gadget to the screen
     *
     * @param gadget Pointer to gadget (screen does not own)
     */
    void Add_Gadget(GadgetClass* gadget);

    /**
     * Remove_Gadget - Remove gadget from screen
     *
     * @param gadget Gadget to remove
     */
    void Remove_Gadget(GadgetClass* gadget);

    /**
     * Remove_All_Gadgets - Clear all gadgets
     */
    void Remove_All_Gadgets();

    /**
     * Process_Gadgets - Process gadget input/updates
     *
     * Called each frame to handle gadget interaction.
     *
     * @param input Input flags (key/mouse state)
     * @return ID of activated gadget, or 0
     */
    int Process_Gadgets(int input);

    /**
     * Draw_Gadgets - Draw all gadgets
     *
     * Called during Render to draw UI elements.
     */
    void Draw_Gadgets();

    // -------------------------------------------------------------------------
    // Screen State
    // -------------------------------------------------------------------------

    /**
     * Is_Initialized - Check if screen is initialized
     */
    bool Is_Initialized() const { return is_initialized_; }

    /**
     * Get_Theater - Get current theater type
     */
    int Get_Theater() const { return theater_; }

protected:
    // -------------------------------------------------------------------------
    // Protected Members
    // -------------------------------------------------------------------------

    // Graphics buffer
    uint8_t* back_buffer_;     // Pointer to back buffer pixels
    int width_;                // Buffer width in pixels
    int height_;               // Buffer height in pixels
    int pitch_;                // Bytes per row (may include padding)
    bool is_locked_;           // Buffer lock state
    bool is_initialized_;      // One_Time() called flag

    // Theater (set by Init)
    int theater_;              // Current theater type

    // Gadget list (simple linked list)
    GadgetClass* gadget_head_; // First gadget in list

    // -------------------------------------------------------------------------
    // Protected Helpers
    // -------------------------------------------------------------------------

    /**
     * Clamp coordinates to screen bounds
     */
    void Clamp_Coords(int& x, int& y) const;

    /**
     * Check if coordinates are within screen bounds
     */
    bool In_Bounds(int x, int y) const;
};

// =============================================================================
// Global Screen Pointer
// =============================================================================

/**
 * TheScreen - Global pointer to current screen
 *
 * This points to the active screen class (usually MapEditClass or similar).
 * All rendering should go through this.
 */
extern GScreenClass* TheScreen;
```

### include/game/gadget.h (NEW)
```cpp
/**
 * GadgetClass - UI Element Base Class
 *
 * Gadgets are UI elements like buttons, sliders, list boxes, etc.
 * They form a linked list attached to the screen and receive
 * input events.
 *
 * This is a simplified version for initial porting.
 * Full gadget system will be expanded later.
 *
 * Original location: CODE/GADGET.H, CODE/GADGET.CPP
 */

#pragma once

#include <cstdint>

// Forward declarations
class GScreenClass;

// =============================================================================
// Gadget Flags
// =============================================================================

/**
 * GadgetFlags - Control gadget behavior
 */
enum GadgetFlags : uint16_t {
    GADGET_NONE = 0,
    GADGET_DISABLED = (1 << 0),    // Gadget is disabled (greyed out)
    GADGET_HIDDEN = (1 << 1),      // Gadget is hidden
    GADGET_STICKY = (1 << 2),      // Mouse capture on click
    GADGET_TOGGLE = (1 << 3),      // Toggle state on click
    GADGET_LEFTPRESS = (1 << 4),   // Trigger on left button press
    GADGET_LEFTRELEASE = (1 << 5), // Trigger on left button release
    GADGET_RIGHTPRESS = (1 << 6),  // Trigger on right button press
    GADGET_KEYBOARD = (1 << 7),    // Accept keyboard input
};

// =============================================================================
// Gadget Input
// =============================================================================

/**
 * GadgetInput - Input state passed to gadgets
 */
struct GadgetInput {
    int mouse_x;         // Mouse X position
    int mouse_y;         // Mouse Y position
    bool left_press;     // Left button pressed this frame
    bool left_release;   // Left button released this frame
    bool left_held;      // Left button held
    bool right_press;    // Right button pressed
    bool right_release;  // Right button released
    bool right_held;     // Right button held
    uint16_t key_code;   // Key pressed (0 if none)
};

// =============================================================================
// GadgetClass
// =============================================================================

/**
 * GadgetClass - Base class for all UI elements
 *
 * Gadgets form a linked list and receive input events from the screen.
 * When activated, they return their ID which the game can handle.
 */
class GadgetClass {
public:
    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------

    /**
     * Create gadget at position with size
     *
     * @param x Left edge
     * @param y Top edge
     * @param w Width
     * @param h Height
     * @param flags Control flags
     * @param id Unique identifier
     */
    GadgetClass(int x, int y, int w, int h, uint16_t flags = GADGET_NONE, int id = 0);

    virtual ~GadgetClass();

    // Non-copyable
    GadgetClass(const GadgetClass&) = delete;
    GadgetClass& operator=(const GadgetClass&) = delete;

    // -------------------------------------------------------------------------
    // Input Processing
    // -------------------------------------------------------------------------

    /**
     * Process_Input - Handle input for this gadget
     *
     * @param input Current input state
     * @return Gadget ID if activated, 0 otherwise
     */
    virtual int Process_Input(const GadgetInput& input);

    /**
     * Is_Point_In - Check if point is within gadget bounds
     */
    bool Is_Point_In(int px, int py) const;

    // -------------------------------------------------------------------------
    // Drawing
    // -------------------------------------------------------------------------

    /**
     * Draw - Render gadget to screen
     *
     * @param screen Target screen
     * @param forced Force redraw even if not dirty
     */
    virtual void Draw(GScreenClass* screen, bool forced = false);

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------

    /**
     * Enable/Disable gadget
     */
    void Enable() { flags_ &= ~GADGET_DISABLED; Set_Dirty(); }
    void Disable() { flags_ |= GADGET_DISABLED; Set_Dirty(); }
    bool Is_Enabled() const { return !(flags_ & GADGET_DISABLED); }

    /**
     * Show/Hide gadget
     */
    void Show() { flags_ &= ~GADGET_HIDDEN; Set_Dirty(); }
    void Hide() { flags_ |= GADGET_HIDDEN; Set_Dirty(); }
    bool Is_Visible() const { return !(flags_ & GADGET_HIDDEN); }

    /**
     * Mark gadget as needing redraw
     */
    void Set_Dirty() { is_dirty_ = true; }
    bool Is_Dirty() const { return is_dirty_; }
    void Clear_Dirty() { is_dirty_ = false; }

    // -------------------------------------------------------------------------
    // Position/Size
    // -------------------------------------------------------------------------

    int Get_X() const { return x_; }
    int Get_Y() const { return y_; }
    int Get_Width() const { return width_; }
    int Get_Height() const { return height_; }
    int Get_ID() const { return id_; }

    void Set_Position(int x, int y) { x_ = x; y_ = y; Set_Dirty(); }
    void Set_Size(int w, int h) { width_ = w; height_ = h; Set_Dirty(); }

    // -------------------------------------------------------------------------
    // List Management
    // -------------------------------------------------------------------------

    GadgetClass* Get_Next() const { return next_; }
    void Set_Next(GadgetClass* next) { next_ = next; }

protected:
    // Position and size
    int x_;
    int y_;
    int width_;
    int height_;

    // State
    uint16_t flags_;
    int id_;
    bool is_dirty_;
    bool is_pressed_;  // Currently pressed (for sticky gadgets)

    // Linked list
    GadgetClass* next_;
};

// =============================================================================
// ButtonClass - Simple clickable button
// =============================================================================

/**
 * ButtonClass - A simple button gadget
 *
 * Displays text and triggers when clicked.
 */
class ButtonClass : public GadgetClass {
public:
    ButtonClass(int x, int y, int w, int h, const char* text, int id);
    virtual ~ButtonClass();

    virtual void Draw(GScreenClass* screen, bool forced = false) override;
    virtual int Process_Input(const GadgetInput& input) override;

    void Set_Text(const char* text);
    const char* Get_Text() const { return text_; }

protected:
    char text_[64];
};

// =============================================================================
// TextClass - Static text display
// =============================================================================

/**
 * TextClass - Non-interactive text display
 */
class TextClass : public GadgetClass {
public:
    TextClass(int x, int y, const char* text, uint8_t color = 15);
    virtual ~TextClass();

    virtual void Draw(GScreenClass* screen, bool forced = false) override;

    void Set_Text(const char* text);
    void Set_Color(uint8_t color) { color_ = color; Set_Dirty(); }

protected:
    char text_[256];
    uint8_t color_;
};
```

### src/game/display/gscreen.cpp (NEW)
```cpp
/**
 * GScreenClass Implementation
 *
 * Base screen rendering class that interfaces with the platform layer.
 */

#include "game/gscreen.h"
#include "game/gadget.h"
#include "platform.h"
#include <cstring>
#include <cstdio>

// =============================================================================
// Global Screen Pointer
// =============================================================================

GScreenClass* TheScreen = nullptr;

// =============================================================================
// Construction / Destruction
// =============================================================================

GScreenClass::GScreenClass()
    : back_buffer_(nullptr)
    , width_(0)
    , height_(0)
    , pitch_(0)
    , is_locked_(false)
    , is_initialized_(false)
    , theater_(0)
    , gadget_head_(nullptr)
{
}

GScreenClass::~GScreenClass() {
    // Remove all gadgets (we don't own them, just unlink)
    Remove_All_Gadgets();

    // Note: back_buffer_ is owned by platform layer, don't free
    back_buffer_ = nullptr;
}

// =============================================================================
// Lifecycle Methods
// =============================================================================

void GScreenClass::One_Time() {
    if (is_initialized_) {
        return; // Already initialized
    }

    // Initialize platform graphics
    PlatformResult result = Platform_Graphics_Init();
    if (result != PLATFORM_RESULT_SUCCESS) {
        Platform_LogError("GScreenClass::One_Time: Failed to initialize graphics");
        return;
    }

    // Get back buffer from platform
    // Note: The platform layer manages the actual buffer
    int32_t w, h, p;
    result = Platform_Graphics_GetBackBuffer(&back_buffer_, &w, &h, &p);
    if (result != PLATFORM_RESULT_SUCCESS || back_buffer_ == nullptr) {
        Platform_LogError("GScreenClass::One_Time: Failed to get back buffer");
        return;
    }

    width_ = w;
    height_ = h;
    pitch_ = p;
    is_initialized_ = true;

    Platform_LogInfo("GScreenClass::One_Time: Initialized %dx%d buffer (pitch=%d)",
                     width_, height_, pitch_);
}

void GScreenClass::Init(int theater) {
    if (!is_initialized_) {
        One_Time();
    }

    theater_ = theater;

    // Clear the screen
    Clear(0);

    // Clear any existing gadgets
    Remove_All_Gadgets();

    Platform_LogInfo("GScreenClass::Init: Theater %d", theater_);
}

void GScreenClass::Clear(uint8_t color) {
    if (!is_initialized_ || back_buffer_ == nullptr) {
        return;
    }

    bool was_locked = is_locked_;
    if (!was_locked) {
        Lock();
    }

    // Fill entire buffer with color
    for (int y = 0; y < height_; y++) {
        uint8_t* row = back_buffer_ + (y * pitch_);
        memset(row, color, width_);
    }

    if (!was_locked) {
        Unlock();
    }
}

void GScreenClass::Render() {
    if (!is_initialized_) {
        return;
    }

    // Draw all gadgets
    Draw_Gadgets();

    // Flip to display
    Flip();
}

void GScreenClass::Flip() {
    if (!is_initialized_) {
        return;
    }

    // Make sure buffer is unlocked before flip
    if (is_locked_) {
        Unlock();
    }

    Platform_Graphics_Flip();
}

// =============================================================================
// Buffer Access
// =============================================================================

uint8_t* GScreenClass::Lock() {
    if (!is_initialized_ || is_locked_) {
        return back_buffer_;
    }

    Platform_Graphics_LockBackBuffer();
    is_locked_ = true;

    return back_buffer_;
}

void GScreenClass::Unlock() {
    if (!is_initialized_ || !is_locked_) {
        return;
    }

    Platform_Graphics_UnlockBackBuffer();
    is_locked_ = false;
}

// =============================================================================
// Buffer Drawing Helpers
// =============================================================================

void GScreenClass::Clamp_Coords(int& x, int& y) const {
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= width_) x = width_ - 1;
    if (y >= height_) y = height_ - 1;
}

bool GScreenClass::In_Bounds(int x, int y) const {
    return (x >= 0 && x < width_ && y >= 0 && y < height_);
}

void GScreenClass::Put_Pixel(int x, int y, uint8_t color) {
    if (!In_Bounds(x, y) || back_buffer_ == nullptr) {
        return;
    }

    bool was_locked = is_locked_;
    if (!was_locked) Lock();

    back_buffer_[y * pitch_ + x] = color;

    if (!was_locked) Unlock();
}

uint8_t GScreenClass::Get_Pixel(int x, int y) const {
    if (!In_Bounds(x, y) || back_buffer_ == nullptr) {
        return 0;
    }

    return back_buffer_[y * pitch_ + x];
}

void GScreenClass::Draw_Rect(int x, int y, int w, int h, uint8_t color) {
    if (back_buffer_ == nullptr || w <= 0 || h <= 0) {
        return;
    }

    // Clamp to screen bounds
    int x1 = x;
    int y1 = y;
    int x2 = x + w;
    int y2 = y + h;

    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 > width_) x2 = width_;
    if (y2 > height_) y2 = height_;

    if (x1 >= x2 || y1 >= y2) {
        return; // Nothing to draw
    }

    bool was_locked = is_locked_;
    if (!was_locked) Lock();

    int rect_width = x2 - x1;
    for (int row = y1; row < y2; row++) {
        uint8_t* ptr = back_buffer_ + (row * pitch_) + x1;
        memset(ptr, color, rect_width);
    }

    if (!was_locked) Unlock();
}

void GScreenClass::Draw_Line(int x1, int y1, int x2, int y2, uint8_t color) {
    if (back_buffer_ == nullptr) {
        return;
    }

    bool was_locked = is_locked_;
    if (!was_locked) Lock();

    // Simple horizontal/vertical line implementation
    if (y1 == y2) {
        // Horizontal line
        if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
        if (y1 < 0 || y1 >= height_) goto done;
        if (x1 < 0) x1 = 0;
        if (x2 >= width_) x2 = width_ - 1;
        if (x1 > x2) goto done;

        uint8_t* ptr = back_buffer_ + (y1 * pitch_) + x1;
        memset(ptr, color, x2 - x1 + 1);
    }
    else if (x1 == x2) {
        // Vertical line
        if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }
        if (x1 < 0 || x1 >= width_) goto done;
        if (y1 < 0) y1 = 0;
        if (y2 >= height_) y2 = height_ - 1;
        if (y1 > y2) goto done;

        for (int y = y1; y <= y2; y++) {
            back_buffer_[y * pitch_ + x1] = color;
        }
    }
    else {
        // Diagonal line - use Bresenham's algorithm
        int dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);
        int dy = (y2 > y1) ? (y2 - y1) : (y1 - y2);
        int sx = (x1 < x2) ? 1 : -1;
        int sy = (y1 < y2) ? 1 : -1;
        int err = dx - dy;

        while (true) {
            if (In_Bounds(x1, y1)) {
                back_buffer_[y1 * pitch_ + x1] = color;
            }

            if (x1 == x2 && y1 == y2) break;

            int e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x1 += sx;
            }
            if (e2 < dx) {
                err += dx;
                y1 += sy;
            }
        }
    }

done:
    if (!was_locked) Unlock();
}

// =============================================================================
// Gadget Management
// =============================================================================

void GScreenClass::Add_Gadget(GadgetClass* gadget) {
    if (gadget == nullptr) {
        return;
    }

    // Add to front of list
    gadget->Set_Next(gadget_head_);
    gadget_head_ = gadget;
}

void GScreenClass::Remove_Gadget(GadgetClass* gadget) {
    if (gadget == nullptr || gadget_head_ == nullptr) {
        return;
    }

    // Remove from list
    if (gadget_head_ == gadget) {
        gadget_head_ = gadget->Get_Next();
        gadget->Set_Next(nullptr);
        return;
    }

    GadgetClass* prev = gadget_head_;
    while (prev->Get_Next() != nullptr) {
        if (prev->Get_Next() == gadget) {
            prev->Set_Next(gadget->Get_Next());
            gadget->Set_Next(nullptr);
            return;
        }
        prev = prev->Get_Next();
    }
}

void GScreenClass::Remove_All_Gadgets() {
    // Just unlink all - we don't own the gadgets
    while (gadget_head_ != nullptr) {
        GadgetClass* next = gadget_head_->Get_Next();
        gadget_head_->Set_Next(nullptr);
        gadget_head_ = next;
    }
}

int GScreenClass::Process_Gadgets(int input) {
    (void)input; // Will be expanded later

    // Get mouse state from platform
    GadgetInput gi;
    Platform_Input_Update();

    int32_t mx, my;
    Platform_Mouse_GetPosition(&mx, &my);
    gi.mouse_x = mx;
    gi.mouse_y = my;
    gi.left_press = Platform_Mouse_ButtonJustPressed(0);
    gi.left_release = Platform_Mouse_ButtonJustReleased(0);
    gi.left_held = Platform_Mouse_ButtonHeld(0);
    gi.right_press = Platform_Mouse_ButtonJustPressed(1);
    gi.right_release = Platform_Mouse_ButtonJustReleased(1);
    gi.right_held = Platform_Mouse_ButtonHeld(1);
    gi.key_code = 0; // TODO: Get from keyboard

    // Process each gadget
    GadgetClass* gadget = gadget_head_;
    while (gadget != nullptr) {
        int result = gadget->Process_Input(gi);
        if (result != 0) {
            return result; // Gadget was activated
        }
        gadget = gadget->Get_Next();
    }

    return 0;
}

void GScreenClass::Draw_Gadgets() {
    bool was_locked = is_locked_;
    if (!was_locked) Lock();

    // Draw each gadget (in reverse order so first added is on top)
    GadgetClass* gadget = gadget_head_;
    while (gadget != nullptr) {
        if (gadget->Is_Visible()) {
            gadget->Draw(this, false);
        }
        gadget = gadget->Get_Next();
    }

    if (!was_locked) Unlock();
}
```

### src/game/display/gadget.cpp (NEW)
```cpp
/**
 * GadgetClass Implementation
 *
 * Basic UI gadget system.
 */

#include "game/gadget.h"
#include "game/gscreen.h"
#include <cstring>

// =============================================================================
// GadgetClass Implementation
// =============================================================================

GadgetClass::GadgetClass(int x, int y, int w, int h, uint16_t flags, int id)
    : x_(x)
    , y_(y)
    , width_(w)
    , height_(h)
    , flags_(flags)
    , id_(id)
    , is_dirty_(true)
    , is_pressed_(false)
    , next_(nullptr)
{
}

GadgetClass::~GadgetClass() {
    // Nothing to clean up
}

bool GadgetClass::Is_Point_In(int px, int py) const {
    return (px >= x_ && px < x_ + width_ &&
            py >= y_ && py < y_ + height_);
}

int GadgetClass::Process_Input(const GadgetInput& input) {
    if (!Is_Enabled() || !Is_Visible()) {
        return 0;
    }

    bool in_bounds = Is_Point_In(input.mouse_x, input.mouse_y);

    // Handle sticky gadgets (mouse capture)
    if (flags_ & GADGET_STICKY) {
        if (input.left_press && in_bounds) {
            is_pressed_ = true;
            Set_Dirty();
        }
        if (input.left_release && is_pressed_) {
            is_pressed_ = false;
            Set_Dirty();
            if (in_bounds) {
                return id_; // Activated
            }
        }
        return 0;
    }

    // Handle left press trigger
    if ((flags_ & GADGET_LEFTPRESS) && input.left_press && in_bounds) {
        return id_;
    }

    // Handle left release trigger
    if ((flags_ & GADGET_LEFTRELEASE) && input.left_release && in_bounds) {
        return id_;
    }

    return 0;
}

void GadgetClass::Draw(GScreenClass* screen, bool forced) {
    if (!Is_Visible()) {
        return;
    }

    if (!is_dirty_ && !forced) {
        return;
    }

    // Base gadget draws a simple box (derived classes override)
    screen->Draw_Rect(x_, y_, width_, height_, 8); // Grey box
    screen->Draw_Line(x_, y_, x_ + width_ - 1, y_, 15); // Top edge (white)
    screen->Draw_Line(x_, y_, x_, y_ + height_ - 1, 15); // Left edge
    screen->Draw_Line(x_ + width_ - 1, y_, x_ + width_ - 1, y_ + height_ - 1, 0); // Right edge (black)
    screen->Draw_Line(x_, y_ + height_ - 1, x_ + width_ - 1, y_ + height_ - 1, 0); // Bottom edge

    is_dirty_ = false;
}

// =============================================================================
// ButtonClass Implementation
// =============================================================================

ButtonClass::ButtonClass(int x, int y, int w, int h, const char* text, int id)
    : GadgetClass(x, y, w, h, GADGET_STICKY | GADGET_LEFTRELEASE, id)
{
    Set_Text(text);
}

ButtonClass::~ButtonClass() {
}

void ButtonClass::Set_Text(const char* text) {
    if (text) {
        strncpy(text_, text, sizeof(text_) - 1);
        text_[sizeof(text_) - 1] = '\0';
    } else {
        text_[0] = '\0';
    }
    Set_Dirty();
}

void ButtonClass::Draw(GScreenClass* screen, bool forced) {
    if (!Is_Visible()) {
        return;
    }

    if (!is_dirty_ && !forced) {
        return;
    }

    // Button colors
    uint8_t face_color = Is_Enabled() ? 8 : 7;  // Light grey or darker
    uint8_t text_color = Is_Enabled() ? 0 : 8;  // Black or grey

    // Draw button face
    screen->Draw_Rect(x_, y_, width_, height_, face_color);

    // Draw 3D border (pressed or normal)
    if (is_pressed_) {
        // Pressed - inverted border
        screen->Draw_Line(x_, y_, x_ + width_ - 1, y_, 0);
        screen->Draw_Line(x_, y_, x_, y_ + height_ - 1, 0);
        screen->Draw_Line(x_ + width_ - 1, y_, x_ + width_ - 1, y_ + height_ - 1, 15);
        screen->Draw_Line(x_, y_ + height_ - 1, x_ + width_ - 1, y_ + height_ - 1, 15);
    } else {
        // Normal - raised border
        screen->Draw_Line(x_, y_, x_ + width_ - 1, y_, 15);
        screen->Draw_Line(x_, y_, x_, y_ + height_ - 1, 15);
        screen->Draw_Line(x_ + width_ - 1, y_, x_ + width_ - 1, y_ + height_ - 1, 0);
        screen->Draw_Line(x_, y_ + height_ - 1, x_ + width_ - 1, y_ + height_ - 1, 0);
    }

    // TODO: Draw text centered (requires font system)
    // For now, just draw a dot in the center as placeholder
    int cx = x_ + width_ / 2;
    int cy = y_ + height_ / 2;
    screen->Put_Pixel(cx, cy, text_color);

    is_dirty_ = false;
}

int ButtonClass::Process_Input(const GadgetInput& input) {
    bool was_pressed = is_pressed_;
    int result = GadgetClass::Process_Input(input);

    // Redraw if press state changed
    if (was_pressed != is_pressed_) {
        Set_Dirty();
    }

    return result;
}

// =============================================================================
// TextClass Implementation
// =============================================================================

TextClass::TextClass(int x, int y, const char* text, uint8_t color)
    : GadgetClass(x, y, 0, 0, GADGET_NONE, 0)
    , color_(color)
{
    Set_Text(text);
}

TextClass::~TextClass() {
}

void TextClass::Set_Text(const char* text) {
    if (text) {
        strncpy(text_, text, sizeof(text_) - 1);
        text_[sizeof(text_) - 1] = '\0';
        // Update size based on text length (placeholder - 8 pixels per char)
        width_ = (int)strlen(text_) * 8;
        height_ = 8;
    } else {
        text_[0] = '\0';
        width_ = 0;
        height_ = 0;
    }
    Set_Dirty();
}

void TextClass::Draw(GScreenClass* screen, bool forced) {
    if (!Is_Visible()) {
        return;
    }

    if (!is_dirty_ && !forced) {
        return;
    }

    // TODO: Draw text (requires font system)
    // For now, draw a horizontal line as placeholder
    if (width_ > 0) {
        screen->Draw_Line(x_, y_ + 4, x_ + width_ - 1, y_ + 4, color_);
    }

    is_dirty_ = false;
}
```

## Verification Command
```bash
ralph-tasks/verify/check-gscreen.sh
```

## Verification Script

### ralph-tasks/verify/check-gscreen.sh (NEW)
```bash
#!/bin/bash
# Verification script for GScreenClass port

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking GScreenClass Port ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check header files exist
echo "[1/5] Checking header files..."
check_file() {
    if [ ! -f "$1" ]; then
        echo "VERIFY_FAILED: $1 not found"
        exit 1
    fi
    echo "  OK: $1"
}

check_file "include/game/gscreen.h"
check_file "include/game/gadget.h"
check_file "src/game/display/gscreen.cpp"
check_file "src/game/display/gadget.cpp"

# Step 2: Syntax check headers
echo "[2/5] Checking header syntax..."
for header in include/game/gscreen.h include/game/gadget.h; do
    if ! clang++ -std=c++17 -fsyntax-only -I include "$header" 2>&1; then
        echo "VERIFY_FAILED: $header has syntax errors"
        exit 1
    fi
done
echo "  OK: Headers valid"

# Step 3: Compile implementations (syntax only - needs platform)
echo "[3/5] Checking implementation syntax..."
# Create a mock platform.h for syntax checking
cat > /tmp/mock_platform.h << 'EOF'
#pragma once
#include <cstdint>
enum PlatformResult { PLATFORM_RESULT_SUCCESS = 0 };
inline PlatformResult Platform_Graphics_Init() { return PLATFORM_RESULT_SUCCESS; }
inline PlatformResult Platform_Graphics_GetBackBuffer(uint8_t** buf, int32_t* w, int32_t* h, int32_t* p) {
    static uint8_t buffer[640*400];
    *buf = buffer; *w = 640; *h = 400; *p = 640;
    return PLATFORM_RESULT_SUCCESS;
}
inline void Platform_Graphics_Flip() {}
inline void Platform_Graphics_LockBackBuffer() {}
inline void Platform_Graphics_UnlockBackBuffer() {}
inline void Platform_Input_Update() {}
inline void Platform_Mouse_GetPosition(int32_t* x, int32_t* y) { *x = 0; *y = 0; }
inline bool Platform_Mouse_ButtonJustPressed(int) { return false; }
inline bool Platform_Mouse_ButtonJustReleased(int) { return false; }
inline bool Platform_Mouse_ButtonHeld(int) { return false; }
inline void Platform_LogInfo(const char*, ...) {}
inline void Platform_LogError(const char*, ...) {}
EOF

# Syntax check with mock platform
if ! clang++ -std=c++17 -fsyntax-only -I include -include /tmp/mock_platform.h \
    src/game/display/gscreen.cpp 2>&1; then
    echo "VERIFY_FAILED: gscreen.cpp has syntax errors"
    rm -f /tmp/mock_platform.h
    exit 1
fi

if ! clang++ -std=c++17 -fsyntax-only -I include -include /tmp/mock_platform.h \
    src/game/display/gadget.cpp 2>&1; then
    echo "VERIFY_FAILED: gadget.cpp has syntax errors"
    rm -f /tmp/mock_platform.h
    exit 1
fi
echo "  OK: Implementations valid"

# Step 4: Build test program with mock platform
echo "[4/5] Building GScreen test..."
cat > /tmp/test_gscreen.cpp << 'EOF'
#include "game/gscreen.h"
#include "game/gadget.h"
#include <cstdio>

// Mock platform implementation
static uint8_t mock_buffer[640 * 400];

#define TEST(name, cond) do { \
    if (!(cond)) { printf("FAIL: %s\n", name); return 1; } \
    printf("PASS: %s\n", name); \
} while(0)

int main() {
    printf("=== GScreenClass Tests ===\n\n");

    // Create screen
    GScreenClass screen;
    TEST("Screen created", true);

    // Check initial state
    TEST("Not initialized", !screen.Is_Initialized());
    TEST("Not locked", !screen.Is_Locked());

    // Test One_Time (with mock platform)
    screen.One_Time();
    TEST("Initialized after One_Time", screen.Is_Initialized());
    TEST("Width set", screen.Get_Width() == 640);
    TEST("Height set", screen.Get_Height() == 400);

    // Test buffer access
    uint8_t* buf = screen.Lock();
    TEST("Buffer locked", screen.Is_Locked());
    TEST("Buffer not null", buf != nullptr);

    screen.Unlock();
    TEST("Buffer unlocked", !screen.Is_Locked());

    // Test pixel operations
    screen.Put_Pixel(100, 100, 42);
    TEST("Pixel written", screen.Get_Pixel(100, 100) == 42);

    // Test rect draw
    screen.Draw_Rect(50, 50, 10, 10, 99);
    TEST("Rect drawn", screen.Get_Pixel(55, 55) == 99);

    // Test Clear
    screen.Clear(0);
    TEST("Clear works", screen.Get_Pixel(55, 55) == 0);

    // Test gadget system
    ButtonClass button(100, 100, 80, 20, "Test", 1);
    screen.Add_Gadget(&button);
    TEST("Gadget added", true);

    screen.Remove_Gadget(&button);
    TEST("Gadget removed", true);

    // Test global pointer
    TheScreen = &screen;
    TEST("TheScreen set", TheScreen != nullptr);

    printf("\n=== All GScreen tests passed! ===\n");
    return 0;
}
EOF

# Compile test with mock platform
if ! clang++ -std=c++17 -I include -include /tmp/mock_platform.h \
    /tmp/test_gscreen.cpp \
    src/game/display/gscreen.cpp \
    src/game/display/gadget.cpp \
    -o /tmp/test_gscreen 2>&1; then
    echo "VERIFY_FAILED: Test program compilation failed"
    rm -f /tmp/mock_platform.h /tmp/test_gscreen.cpp
    exit 1
fi
echo "  OK: Test program built"

# Step 5: Run tests
echo "[5/5] Running GScreen tests..."
if ! /tmp/test_gscreen; then
    echo "VERIFY_FAILED: GScreen tests failed"
    rm -f /tmp/mock_platform.h /tmp/test_gscreen.cpp /tmp/test_gscreen
    exit 1
fi

# Cleanup
rm -f /tmp/mock_platform.h /tmp/test_gscreen.cpp /tmp/test_gscreen

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Create directories: `mkdir -p include/game src/game/display`
2. Create header files (gscreen.h, gadget.h)
3. Create implementation files (gscreen.cpp, gadget.cpp)
4. Make verification script executable
5. Run verification script

## Success Criteria
- GScreenClass initializes with platform layer
- Buffer lock/unlock works correctly
- Pixel and rect drawing works
- Gadget add/remove works
- Basic button gadget functions

## Common Issues

### "Platform_* not declared"
- Platform layer must be built first
- Use mock platform for initial testing

### Buffer access violations
- Always check Is_Initialized() before access
- Always Lock() before drawing

### Gadget list corruption
- Don't delete gadgets while iterating
- Clear gadget list on shutdown

## Completion Promise
When verification passes, output:
```
<promise>TASK_14C_COMPLETE</promise>
```

## Escape Hatch
If stuck after 12 iterations:
- Document what's blocking in `ralph-tasks/blocked/14c.md`
- Output: `<promise>TASK_14C_BLOCKED</promise>`

## Max Iterations
12
