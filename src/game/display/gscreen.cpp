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
    int32_t result = Platform_Graphics_Init();
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

    Platform_LogInfo("GScreenClass::One_Time: Screen initialized");
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

    Platform_LogInfo("GScreenClass::Init complete");
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

    // Platform layer doesn't require explicit locking - buffer is always accessible
    // Just track lock state for compatibility with original code patterns
    is_locked_ = true;

    return back_buffer_;
}

void GScreenClass::Unlock() {
    if (!is_initialized_ || !is_locked_) {
        return;
    }

    // Platform layer doesn't require explicit unlocking
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

    // Use available platform functions
    // WasClicked = press+release in single frame
    // IsPressed = currently held down
    gi.left_press = Platform_Mouse_WasClicked(MOUSE_BUTTON_LEFT);
    gi.left_release = false; // Platform doesn't track release separately
    gi.left_held = Platform_Mouse_IsPressed(MOUSE_BUTTON_LEFT);
    gi.right_press = Platform_Mouse_WasClicked(MOUSE_BUTTON_RIGHT);
    gi.right_release = false;
    gi.right_held = Platform_Mouse_IsPressed(MOUSE_BUTTON_RIGHT);
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
