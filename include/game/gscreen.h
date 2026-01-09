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
