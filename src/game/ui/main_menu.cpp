/**
 * MainMenu Implementation
 *
 * Implements the main menu screen with title background and buttons.
 */

#include "game/ui/main_menu.h"
#include "game/graphics/graphics_buffer.h"
#include "platform.h"
#include <cstring>
#include <cstdio>

// =============================================================================
// Screen dimensions
// =============================================================================

static constexpr int SCREEN_WIDTH = 640;
static constexpr int SCREEN_HEIGHT = 400;

// =============================================================================
// Construction/Destruction
// =============================================================================

MainMenu::MainMenu()
    : initialized_(false)
    , finished_(false)
    , selection_(MenuResult::NONE)
    , highlighted_index_(0)
    , background_(nullptr)
    , has_background_(false)
{
    std::memset(palette_, 0, sizeof(palette_));
    std::memset(buttons_, 0, sizeof(buttons_));
}

MainMenu::~MainMenu() {
    Shutdown();
}

// =============================================================================
// Lifecycle
// =============================================================================

bool MainMenu::Initialize() {
    if (initialized_) {
        return true;
    }

    Platform_LogInfo("MainMenu::Initialize: Starting...");

    // Set up button positions
    SetupButtons();

    // Try to load title screen
    if (!LoadTitleScreen()) {
        Platform_LogWarn("MainMenu: Could not load TITLE.PCX, using fallback");
        // Continue without background - buttons will still work
    }

    // Apply palette if we have one
    if (has_background_) {
        PaletteEntry entries[256];
        for (int i = 0; i < 256; i++) {
            entries[i].r = palette_[i * 3 + 0];
            entries[i].g = palette_[i * 3 + 1];
            entries[i].b = palette_[i * 3 + 2];
        }
        Platform_Graphics_SetPalette(entries, 0, 256);
        Platform_LogInfo("MainMenu: Title screen palette applied");
    }

    initialized_ = true;
    finished_ = false;
    selection_ = MenuResult::NONE;

    Platform_LogInfo("MainMenu::Initialize: Complete");
    return true;
}

void MainMenu::Shutdown() {
    if (!initialized_) {
        return;
    }

    background_.reset();
    has_background_ = false;
    initialized_ = false;

    Platform_LogInfo("MainMenu::Shutdown: Complete");
}

void MainMenu::Reset() {
    finished_ = false;
    selection_ = MenuResult::NONE;
    highlighted_index_ = 0;
}

// =============================================================================
// Title Screen Loading
// =============================================================================

bool MainMenu::LoadTitleScreen() {
    fprintf(stderr, "[MainMenu] Loading TITLE.PCX...\n");

    // Check if file exists first
    int exists = Platform_Mix_Exists("TITLE.PCX");
    fprintf(stderr, "[MainMenu] TITLE.PCX exists in MIX: %d\n", exists);

    // Try to load PCX from MIX archives
    PlatformPcx* pcx = Platform_PCX_Load("TITLE.PCX");
    if (!pcx) {
        fprintf(stderr, "[MainMenu] Platform_PCX_Load failed for TITLE.PCX\n");
        return false;
    }

    // Get dimensions
    int32_t width = 0, height = 0;
    Platform_PCX_GetSize(pcx, &width, &height);

    char msg[128];
    snprintf(msg, sizeof(msg), "MainMenu: TITLE.PCX loaded: %dx%d", width, height);
    Platform_LogInfo(msg);

    // Create background buffer
    background_ = std::make_unique<GraphicsBuffer>(width, height);
    if (!background_) {
        Platform_PCX_Free(pcx);
        return false;
    }

    // Get pixel data
    background_->Lock();
    int32_t pixels_size = width * height;
    uint8_t* pixels = background_->Get_Buffer();
    if (pixels) {
        Platform_PCX_GetPixels(pcx, pixels, pixels_size);
    }
    background_->Unlock();

    // Get palette
    Platform_PCX_GetPalette(pcx, palette_);

    // Clean up
    Platform_PCX_Free(pcx);

    has_background_ = true;
    return true;
}

// =============================================================================
// Button Setup
// =============================================================================

void MainMenu::SetupButtons() {
    // Button labels matching original Red Alert menu
    static const char* labels[BUTTON_COUNT] = {
        "New Missions",
        "Start New Game",
        "Internet Game",
        "Load Mission",
        "Multiplayer Game",
        "Intro & Sneak Peek",
        "Exit Game"
    };

    // Use predefined start position from header constants
    int start_x = BUTTON_START_X;
    int start_y = BUTTON_START_Y;

    // Set up all 7 buttons
    for (int i = 0; i < BUTTON_COUNT; i++) {
        buttons_[i].x = start_x;
        buttons_[i].y = start_y + i * (BUTTON_HEIGHT + BUTTON_SPACING);
        buttons_[i].width = BUTTON_WIDTH;
        buttons_[i].height = BUTTON_HEIGHT;
        buttons_[i].label = labels[i];
        buttons_[i].highlighted = (i == 0);  // First button highlighted by default
    }
}

// =============================================================================
// Update
// =============================================================================

void MainMenu::Update() {
    if (!initialized_ || finished_) {
        return;
    }

    HandleKeyboard();
    HandleMouse();
}

void MainMenu::HandleKeyboard() {
    // Up arrow - move highlight up
    if (Platform_Key_WasPressed(KEY_CODE_UP)) {
        buttons_[highlighted_index_].highlighted = false;
        highlighted_index_--;
        if (highlighted_index_ < 0) {
            highlighted_index_ = BUTTON_COUNT - 1;
        }
        buttons_[highlighted_index_].highlighted = true;
    }

    // Down arrow - move highlight down
    if (Platform_Key_WasPressed(KEY_CODE_DOWN)) {
        buttons_[highlighted_index_].highlighted = false;
        highlighted_index_++;
        if (highlighted_index_ >= BUTTON_COUNT) {
            highlighted_index_ = 0;
        }
        buttons_[highlighted_index_].highlighted = true;
    }

    // Enter/Return - select highlighted button
    if (Platform_Key_WasPressed(KEY_CODE_RETURN)) {
        selection_ = static_cast<MenuResult>(highlighted_index_);
        finished_ = true;
    }

    // Escape - exit
    if (Platform_Key_WasPressed(KEY_CODE_ESCAPE)) {
        selection_ = MenuResult::EXIT_GAME;
        finished_ = true;
    }
}

void MainMenu::HandleMouse() {
    int32_t mx, my;
    Platform_Mouse_GetPosition(&mx, &my);

    // Check if mouse is over any button
    for (int i = 0; i < BUTTON_COUNT; i++) {
        if (IsPointInButton(mx, my, i)) {
            // Update highlight
            if (highlighted_index_ != i) {
                buttons_[highlighted_index_].highlighted = false;
                highlighted_index_ = i;
                buttons_[highlighted_index_].highlighted = true;
            }

            // Check for click
            if (Platform_Mouse_WasClicked(MOUSE_BUTTON_LEFT)) {
                selection_ = static_cast<MenuResult>(i);
                finished_ = true;
            }
            break;
        }
    }
}

bool MainMenu::IsPointInButton(int x, int y, int button_index) const {
    if (button_index < 0 || button_index >= BUTTON_COUNT) {
        return false;
    }

    const MenuButton& btn = buttons_[button_index];
    return (x >= btn.x && x < btn.x + btn.width &&
            y >= btn.y && y < btn.y + btn.height);
}

// =============================================================================
// Render
// =============================================================================

void MainMenu::Render() {
    if (!initialized_) {
        return;
    }

    GraphicsBuffer& screen = GraphicsBuffer::Screen();
    screen.Lock();

    // Draw background
    if (has_background_ && background_) {
        // Blit title screen to display
        int src_w = background_->Get_Width();
        int src_h = background_->Get_Height();

        // Center if smaller than screen
        int dst_x = (SCREEN_WIDTH - src_w) / 2;
        int dst_y = (SCREEN_HEIGHT - src_h) / 2;
        if (dst_x < 0) dst_x = 0;
        if (dst_y < 0) dst_y = 0;

        // Lock the source buffer before blitting (Blit_From requires both buffers locked)
        background_->Lock();
        // Parameter order: src_x, src_y, dst_x, dst_y, width, height
        screen.Blit_From(*background_, 0, 0, dst_x, dst_y, src_w, src_h);
        background_->Unlock();
    } else {
        // No background - just clear to black
        screen.Clear(0);

        // Draw "TITLE.PCX NOT FOUND" message
        // (Simple visual indicator using rectangles)
        screen.Draw_Rect(200, 50, 240, 30, 255);
    }

    // Draw buttons
    for (int i = 0; i < BUTTON_COUNT; i++) {
        DrawButton(i);
    }

    screen.Unlock();
    screen.Flip();
}

void MainMenu::DrawButton(int index) {
    if (index < 0 || index >= BUTTON_COUNT) {
        return;
    }

    const MenuButton& btn = buttons_[index];
    GraphicsBuffer& screen = GraphicsBuffer::Screen();

    // Colors for 3D beveled look using red palette entries
    // Normal: dark red background (196), highlighted: medium red (194)
    uint8_t bg_color = btn.highlighted ? COLOR_BUTTON_HIGHLIGHT : COLOR_BUTTON_NORMAL;
    // Light edge: brighter red for 3D effect (192=bright red, 193=slightly less)
    uint8_t light_edge = btn.highlighted ? 192 : 193;  // Bright red highlight
    // Dark edge: very dark red for shadow (198=very dark, 199=almost black)
    uint8_t dark_edge = btn.highlighted ? 198 : 199;   // Dark red shadow
    // Text: white (15) when highlighted, normal color otherwise
    uint8_t text_color = btn.highlighted ? 15 : COLOR_BUTTON_TEXT;

    // Draw button background
    screen.Fill_Rect(btn.x, btn.y, btn.width, btn.height, bg_color);

    // Draw 3D beveled edges (top and left are light, bottom and right are dark)
    // Top edge (light)
    screen.Draw_HLine(btn.x, btn.y, btn.width, light_edge);
    // Left edge (light)
    screen.Draw_VLine(btn.x, btn.y, btn.height, light_edge);
    // Bottom edge (dark)
    screen.Draw_HLine(btn.x, btn.y + btn.height - 1, btn.width, dark_edge);
    // Right edge (dark)
    screen.Draw_VLine(btn.x + btn.width - 1, btn.y, btn.height, dark_edge);

    // Draw button text using simple bitmap rendering
    if (btn.label) {
        DrawText(btn.label, btn.x + btn.width / 2, btn.y + btn.height / 2, text_color, true);
    }
}

// =============================================================================
// Simple Bitmap Text Rendering
// =============================================================================

// 5x7 bitmap font data for uppercase letters, numbers, and common symbols
// Each character is 5 pixels wide, stored as 7 rows of 5 bits
static const uint8_t FONT_DATA[][7] = {
    // Space (32)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // ! (33)
    {0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04},
    // " (34) - skip for now
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // # - skip
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // $ - skip
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // % - skip
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // & (38)
    {0x08, 0x14, 0x14, 0x08, 0x15, 0x12, 0x0D},
    // ' - skip
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // ( - skip
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // ) - skip
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // * - skip
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // + - skip
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // , - skip
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // - (45)
    {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00},
    // . (46)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04},
    // / - skip
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // 0-9 (48-57)
    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}, // 0
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}, // 1
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}, // 2
    {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E}, // 3
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}, // 4
    {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}, // 5
    {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}, // 6
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}, // 7
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}, // 8
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}, // 9
    // : - skip
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // ; - skip
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // < - skip
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // = - skip
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // > - skip
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // ? - skip
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // @ - skip
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // A-Z (65-90)
    {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, // A
    {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}, // B
    {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}, // C
    {0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C}, // D
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}, // E
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}, // F
    {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F}, // G
    {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, // H
    {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}, // I
    {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C}, // J
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}, // K
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}, // L
    {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}, // M
    {0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11}, // N
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, // O
    {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}, // P
    {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}, // Q
    {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}, // R
    {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}, // S
    {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}, // T
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, // U
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}, // V
    {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A}, // W
    {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}, // X
    {0x11, 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04}, // Y
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}, // Z
};

// Lowercase letters (map to uppercase for simplicity)
static const uint8_t FONT_LOWERCASE_OFFSET = 'a' - 'A';

void MainMenu::DrawText(const char* text, int center_x, int center_y, uint8_t color, bool centered) {
    if (!text) return;

    GraphicsBuffer& screen = GraphicsBuffer::Screen();

    // Calculate text width for centering
    int text_width = 0;
    for (const char* p = text; *p; p++) {
        text_width += 6;  // 5 pixels + 1 spacing
    }
    text_width -= 1;  // Remove trailing space

    int start_x = centered ? (center_x - text_width / 2) : center_x;
    int start_y = center_y - 3;  // Center vertically (7 pixel height / 2)

    int x = start_x;
    for (const char* p = text; *p; p++) {
        char c = *p;

        // Convert lowercase to uppercase
        if (c >= 'a' && c <= 'z') {
            c = c - 'a' + 'A';
        }

        // Get font data index
        int font_index = -1;
        if (c == ' ') font_index = 0;
        else if (c == '!') font_index = 1;
        else if (c == '&') font_index = 6;
        else if (c >= '0' && c <= '9') font_index = 16 + (c - '0');
        else if (c >= 'A' && c <= 'Z') font_index = 33 + (c - 'A');

        if (font_index >= 0 && font_index < 59) {
            // Draw character
            for (int row = 0; row < 7; row++) {
                uint8_t row_data = FONT_DATA[font_index][row];
                for (int col = 0; col < 5; col++) {
                    if (row_data & (0x10 >> col)) {
                        screen.Put_Pixel(x + col, start_y + row, color);
                    }
                }
            }
        }

        x += 6;  // Move to next character position
    }
}
