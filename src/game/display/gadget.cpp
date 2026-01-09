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
