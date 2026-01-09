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
