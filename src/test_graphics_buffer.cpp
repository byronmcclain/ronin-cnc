/**
 * GraphicsBuffer Test Program
 *
 * Comprehensive tests for the GraphicsBuffer wrapper class.
 * Tests both functional correctness and visual output.
 */

#include "game/graphics/graphics_buffer.h"
#include "platform.h"
#include <cstdio>
#include <cstring>
#include <utility>

// =============================================================================
// Test Utilities
// =============================================================================

static int tests_run = 0;
static int tests_passed = 0;

#define TEST_START(name) do { \
    printf("  Testing %s... ", name); \
    tests_run++; \
} while(0)

#define TEST_PASS() do { \
    printf("PASS\n"); \
    tests_passed++; \
} while(0)

#define TEST_FAIL(msg) do { \
    printf("FAIL: %s\n", msg); \
    return false; \
} while(0)

#define ASSERT(cond, msg) do { \
    if (!(cond)) TEST_FAIL(msg); \
} while(0)

// =============================================================================
// Unit Tests
// =============================================================================

bool test_offscreen_buffer_creation() {
    TEST_START("off-screen buffer creation");

    // Create various sizes
    GraphicsBuffer buf1(100, 100);
    ASSERT(buf1.Get_Width() == 100, "Width should be 100");
    ASSERT(buf1.Get_Height() == 100, "Height should be 100");
    ASSERT(!buf1.IsScreenBuffer(), "Should not be screen buffer");

    // Lock should succeed
    ASSERT(buf1.Lock(), "Lock should succeed");
    ASSERT(buf1.IsLocked(), "Should be locked");
    ASSERT(buf1.Get_Buffer() != nullptr, "Buffer should not be null");

    buf1.Unlock();
    ASSERT(!buf1.IsLocked(), "Should be unlocked");

    // Zero-size buffer should have null pixels
    GraphicsBuffer buf_zero(0, 0);
    ASSERT(!buf_zero.Lock(), "Zero-size buffer lock should fail");

    TEST_PASS();
    return true;
}

bool test_pixel_operations() {
    TEST_START("pixel operations");

    GraphicsBuffer buf(64, 64);
    ASSERT(buf.Lock(), "Lock should succeed");

    // Clear and verify
    buf.Clear(0);
    ASSERT(buf.Get_Pixel(0, 0) == 0, "Pixel should be 0 after clear");

    // Put/Get pixel
    buf.Put_Pixel(10, 10, 123);
    ASSERT(buf.Get_Pixel(10, 10) == 123, "Pixel should be 123");

    // Out of bounds should be safe
    buf.Put_Pixel(-1, 10, 99);  // Should not crash
    buf.Put_Pixel(10, -1, 99);
    buf.Put_Pixel(100, 10, 99);  // Beyond buffer
    buf.Put_Pixel(10, 100, 99);
    ASSERT(buf.Get_Pixel(-1, 10) == 0, "Out of bounds should return 0");
    ASSERT(buf.Get_Pixel(100, 10) == 0, "Out of bounds should return 0");

    buf.Unlock();
    TEST_PASS();
    return true;
}

bool test_line_drawing() {
    TEST_START("line drawing");

    GraphicsBuffer buf(64, 64);
    ASSERT(buf.Lock(), "Lock should succeed");
    buf.Clear(0);

    // Horizontal line
    buf.Draw_HLine(10, 20, 30, 5);
    ASSERT(buf.Get_Pixel(10, 20) == 5, "HLine start pixel");
    ASSERT(buf.Get_Pixel(39, 20) == 5, "HLine end pixel");
    ASSERT(buf.Get_Pixel(40, 20) == 0, "HLine should not extend");
    ASSERT(buf.Get_Pixel(10, 19) == 0, "HLine should not affect other rows");

    // Vertical line
    buf.Clear(0);
    buf.Draw_VLine(30, 10, 20, 7);
    ASSERT(buf.Get_Pixel(30, 10) == 7, "VLine start pixel");
    ASSERT(buf.Get_Pixel(30, 29) == 7, "VLine end pixel");
    ASSERT(buf.Get_Pixel(30, 30) == 0, "VLine should not extend");

    // Clipped lines
    buf.Clear(0);
    buf.Draw_HLine(-5, 5, 20, 8);  // Starts off-screen
    ASSERT(buf.Get_Pixel(0, 5) == 8, "Clipped HLine should start at edge");
    ASSERT(buf.Get_Pixel(14, 5) == 8, "Clipped HLine should end correctly");

    buf.Unlock();
    TEST_PASS();
    return true;
}

bool test_rectangle_operations() {
    TEST_START("rectangle operations");

    GraphicsBuffer buf(64, 64);
    ASSERT(buf.Lock(), "Lock should succeed");
    buf.Clear(0);

    // Fill rectangle
    buf.Fill_Rect(10, 10, 20, 20, 42);
    ASSERT(buf.Get_Pixel(10, 10) == 42, "Fill_Rect corner");
    ASSERT(buf.Get_Pixel(29, 29) == 42, "Fill_Rect opposite corner");
    ASSERT(buf.Get_Pixel(9, 10) == 0, "Fill_Rect should not extend left");
    ASSERT(buf.Get_Pixel(30, 10) == 0, "Fill_Rect should not extend right");

    // Draw rectangle (outline)
    buf.Clear(0);
    buf.Draw_Rect(10, 10, 10, 10, 55);
    ASSERT(buf.Get_Pixel(10, 10) == 55, "Draw_Rect corner");
    ASSERT(buf.Get_Pixel(19, 10) == 55, "Draw_Rect top edge");
    ASSERT(buf.Get_Pixel(10, 19) == 55, "Draw_Rect left edge");
    ASSERT(buf.Get_Pixel(15, 15) == 0, "Draw_Rect interior should be empty");

    // Clipped rectangle
    buf.Clear(0);
    buf.Fill_Rect(-5, -5, 20, 20, 99);
    ASSERT(buf.Get_Pixel(0, 0) == 99, "Clipped Fill_Rect at origin");
    ASSERT(buf.Get_Pixel(14, 14) == 99, "Clipped Fill_Rect visible portion");

    buf.Unlock();
    TEST_PASS();
    return true;
}

bool test_blitting() {
    TEST_START("blitting operations");

    // Create source buffer with pattern
    GraphicsBuffer src(32, 32);
    ASSERT(src.Lock(), "Src lock should succeed");
    src.Clear(0);
    src.Fill_Rect(8, 8, 16, 16, 100);  // Center square

    // Create destination buffer
    GraphicsBuffer dst(64, 64);
    ASSERT(dst.Lock(), "Dst lock should succeed");
    dst.Clear(0);

    // Opaque blit
    dst.Blit_From(src, 0, 0, 10, 10, 32, 32);
    ASSERT(dst.Get_Pixel(10, 10) == 0, "Blit corner (from src transparent)");
    ASSERT(dst.Get_Pixel(18, 18) == 100, "Blit center square");

    // Transparent blit
    dst.Clear(50);  // Fill with different color
    dst.Blit_From_Trans(src, 0, 0, 0, 0, 32, 32);
    ASSERT(dst.Get_Pixel(0, 0) == 50, "Trans blit should preserve background");
    ASSERT(dst.Get_Pixel(8, 8) == 100, "Trans blit should copy non-transparent");

    src.Unlock();
    dst.Unlock();
    TEST_PASS();
    return true;
}

bool test_color_remapping() {
    TEST_START("color remapping");

    GraphicsBuffer buf(32, 32);
    ASSERT(buf.Lock(), "Lock should succeed");

    // Create remap table that adds 10 to each color
    uint8_t remap[256];
    for (int i = 0; i < 256; i++) {
        remap[i] = (i + 10) & 0xFF;
    }

    // Fill with known values
    buf.Clear(0);
    buf.Fill_Rect(5, 5, 10, 10, 20);  // Value 20

    // Apply remap to region
    buf.Remap(5, 5, 10, 10, remap);
    ASSERT(buf.Get_Pixel(5, 5) == 30, "Remap should add 10 (20 -> 30)");
    ASSERT(buf.Get_Pixel(0, 0) == 0, "Remap should not affect outside region");

    buf.Unlock();
    TEST_PASS();
    return true;
}

bool test_screen_buffer() {
    TEST_START("screen buffer singleton");

    // Get screen buffer (this creates the singleton if needed)
    GraphicsBuffer& screen = GraphicsBuffer::Screen();

    // Check initialization status (after singleton created)
    ASSERT(GraphicsBuffer::IsScreenInitialized(), "Screen should be initialized");
    ASSERT(screen.IsScreenBuffer(), "Should be screen buffer");
    ASSERT(screen.Get_Width() == 640, "Screen width should be 640");
    ASSERT(screen.Get_Height() == 400, "Screen height should be 400");

    // Lock and draw
    ASSERT(screen.Lock(), "Screen lock should succeed");
    screen.Clear(0);

    // Draw a test pattern
    for (int y = 0; y < screen.Get_Height(); y++) {
        for (int x = 0; x < screen.Get_Width(); x++) {
            screen.Put_Pixel(x, y, (x + y) & 0xFF);
        }
    }

    screen.Unlock();

    // Flip should succeed
    ASSERT(screen.Flip(), "Flip should succeed");

    // Get same instance again
    GraphicsBuffer& screen2 = GraphicsBuffer::Screen();
    ASSERT(&screen == &screen2, "Singleton should return same instance");

    TEST_PASS();
    return true;
}

bool test_nested_locks() {
    TEST_START("nested locks");

    GraphicsBuffer buf(32, 32);

    ASSERT(!buf.IsLocked(), "Should start unlocked");

    ASSERT(buf.Lock(), "First lock should succeed");
    ASSERT(buf.IsLocked(), "Should be locked");

    ASSERT(buf.Lock(), "Second lock should succeed");
    ASSERT(buf.IsLocked(), "Should still be locked");

    buf.Unlock();
    ASSERT(buf.IsLocked(), "Should still be locked after one unlock");

    buf.Unlock();
    ASSERT(!buf.IsLocked(), "Should be unlocked after matching unlocks");

    // Extra unlock should be safe
    buf.Unlock();
    ASSERT(!buf.IsLocked(), "Extra unlock should be safe");

    TEST_PASS();
    return true;
}

bool test_move_semantics() {
    TEST_START("move semantics");

    GraphicsBuffer buf1(32, 32);
    ASSERT(buf1.Lock(), "Lock should succeed");
    buf1.Fill_Rect(0, 0, 32, 32, 42);
    buf1.Unlock();

    // Move construct
    GraphicsBuffer buf2(std::move(buf1));
    ASSERT(buf2.Get_Width() == 32, "Moved buffer should have width");
    ASSERT(buf2.Lock(), "Moved buffer should be lockable");
    ASSERT(buf2.Get_Pixel(0, 0) == 42, "Moved buffer should have data");
    buf2.Unlock();

    // Original should be empty
    ASSERT(!buf1.Lock(), "Original should not be lockable after move");

    TEST_PASS();
    return true;
}

// =============================================================================
// Visual Test
// =============================================================================

void run_visual_test() {
    printf("\n=== Visual Test ===\n");
    printf("Drawing test pattern to screen...\n");

    GraphicsBuffer& screen = GraphicsBuffer::Screen();

    // Set up a simple grayscale palette
    PaletteEntry palette[256];
    for (int i = 0; i < 256; i++) {
        palette[i].r = static_cast<uint8_t>(i);
        palette[i].g = static_cast<uint8_t>(i);
        palette[i].b = static_cast<uint8_t>(i);
    }
    Platform_Graphics_SetPalette(palette, 0, 256);

    screen.Lock();
    screen.Clear(0);

    // Draw color bands
    for (int y = 0; y < 100; y++) {
        for (int x = 0; x < 640; x++) {
            screen.Put_Pixel(x, y, (x * 256 / 640) & 0xFF);
        }
    }

    // Draw rectangles
    for (int i = 0; i < 8; i++) {
        screen.Fill_Rect(50 + i * 70, 120, 50, 50, 32 * (i + 1));
    }

    // Draw outlined rectangles
    for (int i = 0; i < 8; i++) {
        screen.Draw_Rect(50 + i * 70, 200, 50, 50, 255);
    }

    // Draw lines
    for (int i = 0; i < 10; i++) {
        screen.Draw_HLine(50, 280 + i * 10, 540, 128 + i * 10);
    }

    screen.Unlock();
    screen.Flip();

    printf("Test pattern displayed. Waiting 2 seconds...\n");
    Platform_Delay(2000);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[]) {
    printf("==========================================\n");
    printf("GraphicsBuffer Test Suite\n");
    printf("==========================================\n\n");

    // Check for quick mode (no visual test)
    bool quick_mode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--quick") == 0) {
            quick_mode = true;
        }
    }

    // Initialize platform
    if (Platform_Init() != PLATFORM_RESULT_SUCCESS) {
        printf("ERROR: Failed to initialize platform\n");
        return 1;
    }

    // Initialize graphics
    if (Platform_Graphics_Init() != 0) {
        printf("ERROR: Failed to initialize graphics\n");
        Platform_Shutdown();
        return 1;
    }

    printf("=== Unit Tests ===\n\n");

    // Run unit tests
    test_offscreen_buffer_creation();
    test_pixel_operations();
    test_line_drawing();
    test_rectangle_operations();
    test_blitting();
    test_color_remapping();
    test_screen_buffer();
    test_nested_locks();
    test_move_semantics();

    printf("\n------------------------------------------\n");
    printf("Tests: %d/%d passed\n", tests_passed, tests_run);
    printf("------------------------------------------\n");

    // Run visual test if all unit tests passed and not in quick mode
    if (tests_passed == tests_run && !quick_mode) {
        run_visual_test();
    }

    // Cleanup
    Platform_Graphics_Shutdown();
    Platform_Shutdown();

    printf("\n==========================================\n");
    if (tests_passed == tests_run) {
        printf("All tests PASSED!\n");
    } else {
        printf("Some tests FAILED\n");
    }
    printf("==========================================\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
