#include <cstdio>
#include <cstring>
#include <cassert>
#include "platform.h"

// Test helper macros
#define TEST(name) printf("  Testing %s... ", name)
#define PASS() printf("PASS\n")
#define FAIL(msg) do { printf("FAIL: %s\n", msg); return 1; } while(0)

int test_version() {
    TEST("Platform_GetVersion");
    int version = Platform_GetVersion();
    if (version < 1) FAIL("Version should be >= 1");
    printf("(v%d) ", version);
    PASS();
    return 0;
}

int test_init_shutdown() {
    TEST("Platform_Init/Shutdown cycle");

    // Should not be initialized yet (we reset in main)
    if (Platform_IsInitialized()) FAIL("Should not be initialized");

    // Init should succeed
    PlatformResult result = Platform_Init();
    if (result != PLATFORM_RESULT_SUCCESS) FAIL("Init failed");

    // Should be initialized
    if (!Platform_IsInitialized()) FAIL("Should be initialized after init");

    // Double init should fail
    result = Platform_Init();
    if (result != PLATFORM_RESULT_ALREADY_INITIALIZED) FAIL("Double init should fail");

    // Shutdown should succeed
    result = Platform_Shutdown();
    if (result != PLATFORM_RESULT_SUCCESS) FAIL("Shutdown failed");

    // Should not be initialized
    if (Platform_IsInitialized()) FAIL("Should not be initialized after shutdown");

    // Double shutdown should fail
    result = Platform_Shutdown();
    if (result != PLATFORM_RESULT_NOT_INITIALIZED) FAIL("Double shutdown should fail");

    PASS();
    return 0;
}

int test_error_handling() {
    TEST("Error handling");

    // Clear any existing error
    Platform_ClearError();

    // Get error should return empty/null
    const char* err = Platform_GetErrorString();
    // err may be null or empty string - both are valid

    // Set an error
    Platform_SetError("Test error message");

    // Get error should return our message
    err = Platform_GetErrorString();
    if (err == nullptr) FAIL("GetErrorString returned null after SetError");
    if (strstr(err, "Test error") == nullptr) FAIL("Error message not found");

    // Test buffer-based error retrieval
    char buffer[256];
    int len = Platform_GetLastError(reinterpret_cast<int8_t*>(buffer), sizeof(buffer));
    if (len < 0) FAIL("GetLastError failed");
    if (strstr(buffer, "Test error") == nullptr) FAIL("Buffer error message not found");

    // Clear error
    Platform_ClearError();

    PASS();
    return 0;
}

int test_logging() {
    TEST("Logging functions");

    // These should not crash - we can't easily verify output
    Platform_Log(LOG_LEVEL_DEBUG, "Debug test message");
    Platform_Log(LOG_LEVEL_INFO, "Info test message");
    Platform_Log(LOG_LEVEL_WARN, "Warn test message");
    Platform_Log(LOG_LEVEL_ERROR, "Error test message");

    Platform_LogDebug("Debug shorthand test");
    Platform_LogInfo("Info shorthand test");
    Platform_LogWarn("Warn shorthand test");
    Platform_LogError("Error shorthand test");

    // Test with null (should not crash)
    Platform_Log(LOG_LEVEL_INFO, nullptr);

    PASS();
    return 0;
}

int test_graphics() {
    TEST("Graphics system");

    // Initialize graphics (requires Platform_Init first)
    int result = Platform_Graphics_Init();
    if (result != 0) {
        char err[256];
        Platform_GetLastError(reinterpret_cast<int8_t*>(err), sizeof(err));
        printf("SKIP (init failed: %s)\n", err);
        return 0; // Don't fail test if graphics unavailable (CI environment)
    }

    if (!Platform_Graphics_IsInitialized()) {
        Platform_Graphics_Shutdown();
        FAIL("Graphics should be initialized");
    }

    // Check display mode
    DisplayMode mode;
    Platform_Graphics_GetMode(&mode);
    if (mode.width != 640 || mode.height != 400) {
        printf("(mode: %dx%d) ", mode.width, mode.height);
    }

    // Get back buffer
    uint8_t* pixels = nullptr;
    int32_t width, height, pitch;
    result = Platform_Graphics_GetBackBuffer(&pixels, &width, &height, &pitch);
    if (result != 0 || pixels == nullptr) {
        Platform_Graphics_Shutdown();
        FAIL("Failed to get back buffer");
    }

    // Draw test pattern (vertical color bars using grayscale palette)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            pixels[y * pitch + x] = (x * 256 / width) & 0xFF;
        }
    }

    // Flip to display
    result = Platform_Graphics_Flip();
    if (result != 0) {
        char err[256];
        Platform_GetLastError(reinterpret_cast<int8_t*>(err), sizeof(err));
        Platform_Graphics_Shutdown();
        printf("FAIL: Flip failed: %s\n", err);
        return 1;
    }

    // Test palette modification
    Platform_Graphics_SetPaletteEntry(128, 255, 0, 0);  // Set index 128 to red

    // Draw horizontal line at color 128 (should be red now)
    for (int x = 0; x < width; x++) {
        pixels[200 * pitch + x] = 128;
    }

    // Flip again
    result = Platform_Graphics_Flip();
    if (result != 0) {
        Platform_Graphics_Shutdown();
        FAIL("Second flip failed");
    }

    // Cleanup
    Platform_Graphics_Shutdown();

    if (Platform_Graphics_IsInitialized()) {
        FAIL("Graphics should be shut down");
    }

    printf("(buffer: %dx%d, flip OK) ", width, height);
    PASS();
    return 0;
}

int test_input() {
    TEST("Input system");

    // Input init should succeed (auto-initialized)
    int result = Platform_Input_Init();
    if (result != 0) {
        FAIL("Input init failed");
    }

    // Update should not crash
    Platform_Input_Update();

    // Key queries should return false (no keys pressed)
    if (Platform_Key_IsPressed(KEY_CODE_SPACE)) {
        // This might be true if user is holding space, so just warn
        printf("(space pressed?) ");
    }

    // Mouse position should be valid
    int32_t mx, my;
    Platform_Mouse_GetPosition(&mx, &my);
    // Position can be anything, just verify it doesn't crash

    // Modifier queries should work
    bool shift = Platform_Key_ShiftDown();
    bool ctrl = Platform_Key_CtrlDown();
    bool alt = Platform_Key_AltDown();
    (void)shift; (void)ctrl; (void)alt;  // Suppress unused warnings

    Platform_Input_Shutdown();

    PASS();
    return 0;
}

int test_memory() {
    TEST("Memory system");

    // Test basic allocation
    void* ptr1 = Platform_Alloc(1024, 0);  // MEM_NORMAL
    if (ptr1 == nullptr) FAIL("Allocation returned null");

    // Test zero-initialized allocation
    void* ptr2 = Platform_Alloc(256, 0x0004);  // MEM_CLEAR
    if (ptr2 == nullptr) FAIL("Clear allocation returned null");

    // Verify zero-initialized
    uint8_t* bytes = static_cast<uint8_t*>(ptr2);
    bool all_zero = true;
    for (int i = 0; i < 256; i++) {
        if (bytes[i] != 0) {
            all_zero = false;
            break;
        }
    }
    if (!all_zero) FAIL("MEM_CLEAR did not zero memory");

    // Test memory operations
    const char* test_str = "Hello, Memory!";
    size_t str_len = strlen(test_str) + 1;
    Platform_MemCopy(ptr1, test_str, str_len);

    if (Platform_MemCmp(ptr1, test_str, str_len) != 0) {
        FAIL("MemCopy/MemCmp failed");
    }

    // Test MemSet
    Platform_MemSet(ptr2, 0xAB, 256);
    bytes = static_cast<uint8_t*>(ptr2);
    bool all_ab = true;
    for (int i = 0; i < 256; i++) {
        if (bytes[i] != 0xAB) {
            all_ab = false;
            break;
        }
    }
    if (!all_ab) FAIL("MemSet failed");

    // Test ZeroMemory
    Platform_ZeroMemory(ptr2, 256);
    bytes = static_cast<uint8_t*>(ptr2);
    all_zero = true;
    for (int i = 0; i < 256; i++) {
        if (bytes[i] != 0) {
            all_zero = false;
            break;
        }
    }
    if (!all_zero) FAIL("ZeroMemory failed");

    // Test reallocation
    void* ptr3 = Platform_Alloc(64, 0);
    if (ptr3 == nullptr) FAIL("Small allocation failed");

    void* ptr3_new = Platform_Realloc(ptr3, 64, 128);
    if (ptr3_new == nullptr) FAIL("Realloc failed");

    // Check statistics
    size_t allocated = Platform_Mem_GetAllocated();
    size_t count = Platform_Mem_GetCount();
    size_t peak = Platform_Mem_GetPeak();

    if (allocated == 0) FAIL("Allocated should be > 0");
    if (count == 0) FAIL("Count should be > 0");
    if (peak == 0) FAIL("Peak should be > 0");

    printf("(alloc=%zu, count=%zu, peak=%zu) ", allocated, count, peak);

    // Free all allocations
    Platform_Free(ptr1, 1024);
    Platform_Free(ptr2, 256);
    Platform_Free(ptr3_new, 128);

    // Check RAM queries don't crash
    size_t free_ram = Platform_Ram_Free();
    size_t total_ram = Platform_Ram_Total();
    if (free_ram == 0 || total_ram == 0) {
        printf("(ram: free=%zu, total=%zu) ", free_ram, total_ram);
    }

    PASS();
    return 0;
}

int test_files() {
    TEST("File system");

    // Test path normalization
    char path[256] = "DATA\\CONQUER.MIX";
    Platform_NormalizePath(path);
    if (strcmp(path, "DATA/CONQUER.MIX") != 0) {
        FAIL("Path normalization failed");
    }

    // Test file write
    const char* test_file = "/tmp/platform_test_file.txt";
    const char* test_data = "Hello from C++!";

    PlatformFile* file = Platform_File_Open(test_file, FILE_MODE_WRITE);
    if (file == nullptr) {
        FAIL("Failed to open file for writing");
    }

    int written = Platform_File_Write(file, test_data, strlen(test_data));
    if (written != (int)strlen(test_data)) {
        Platform_File_Close(file);
        FAIL("Write returned wrong byte count");
    }
    Platform_File_Close(file);

    // Test file exists
    if (!Platform_File_Exists(test_file)) {
        FAIL("File should exist after writing");
    }

    // Test file size
    int64_t size = Platform_File_GetSize(test_file);
    if (size != (int64_t)strlen(test_data)) {
        FAIL("File size mismatch");
    }

    // Test file read
    file = Platform_File_Open(test_file, FILE_MODE_READ);
    if (file == nullptr) {
        FAIL("Failed to open file for reading");
    }

    if (Platform_File_Size(file) != (int64_t)strlen(test_data)) {
        Platform_File_Close(file);
        FAIL("File size from handle mismatch");
    }

    char buffer[256] = {0};
    int bytes_read = Platform_File_Read(file, buffer, sizeof(buffer) - 1);
    if (bytes_read != (int)strlen(test_data)) {
        Platform_File_Close(file);
        FAIL("Read returned wrong byte count");
    }

    if (strcmp(buffer, test_data) != 0) {
        Platform_File_Close(file);
        FAIL("Read data mismatch");
    }

    // Test seek and tell
    Platform_File_Seek(file, 0, SEEK_ORIGIN_START);
    if (Platform_File_Tell(file) != 0) {
        Platform_File_Close(file);
        FAIL("Tell after seek to start should be 0");
    }

    Platform_File_Seek(file, 6, SEEK_ORIGIN_START);
    if (Platform_File_Tell(file) != 6) {
        Platform_File_Close(file);
        FAIL("Tell after seek to 6 should be 6");
    }

    // Test eof
    Platform_File_Seek(file, 0, SEEK_ORIGIN_END);
    if (!Platform_File_Eof(file)) {
        Platform_File_Close(file);
        FAIL("Should be at EOF after seeking to end");
    }

    Platform_File_Close(file);

    // Test directory operations
    const char* test_dir = "/tmp/platform_test_dir";
    if (!Platform_CreateDirectory(test_dir)) {
        // May already exist, that's ok
    }

    if (!Platform_IsDirectory(test_dir)) {
        FAIL("Created path should be a directory");
    }

    // Test directory iteration
    PlatformDir* dir = Platform_Dir_Open("/tmp");
    if (dir == nullptr) {
        FAIL("Failed to open /tmp directory");
    }

    DirEntry entry;
    int entry_count = 0;
    while (Platform_Dir_Read(dir, &entry)) {
        entry_count++;
    }
    Platform_Dir_Close(dir);

    if (entry_count == 0) {
        FAIL("/tmp should have some entries");
    }

    // Cleanup
    Platform_File_Delete(test_file);
    Platform_DeleteDirectory(test_dir);

    PASS();
    return 0;
}

int test_timing() {
    TEST("Timing system");

    // Test get_ticks returns increasing values
    uint32_t t1 = Platform_Timer_GetTicks();
    Platform_Timer_Delay(10);
    uint32_t t2 = Platform_Timer_GetTicks();

    if (t2 <= t1) FAIL("Ticks should increase over time");

    // Check delay was approximately correct (allow 5-50ms tolerance)
    uint32_t elapsed = t2 - t1;
    if (elapsed < 5 || elapsed > 50) {
        printf("(delay was %ums, expected ~10ms) ", elapsed);
        // Don't fail - timing can vary on different systems
    }

    // Test performance counter
    uint64_t freq = Platform_Timer_GetPerformanceFrequency();
    if (freq != 1000000000) {
        printf("(freq=%llu, expected 1GHz) ", (unsigned long long)freq);
    }

    uint64_t pc1 = Platform_Timer_GetPerformanceCounter();
    Platform_Timer_Delay(1);
    uint64_t pc2 = Platform_Timer_GetPerformanceCounter();

    if (pc2 <= pc1) FAIL("Performance counter should increase");

    // Test frame timing functions exist and don't crash
    Platform_Frame_Begin();
    Platform_Timer_Delay(16); // Simulate ~60fps frame
    Platform_Frame_End();

    double fps = Platform_Frame_GetFPS();
    double frame_time = Platform_Frame_GetTime();

    printf("(fps=%.1f, frame=%.3fs) ", fps, frame_time);

    PASS();
    return 0;
}

// =============================================================================
// Demo Mode - Interactive graphics demonstration
// =============================================================================

void setup_rainbow_palette() {
    // Create a smooth rainbow palette
    for (int i = 0; i < 256; i++) {
        uint8_t r, g, b;
        if (i < 43) {
            // Red to Yellow
            r = 255;
            g = i * 6;
            b = 0;
        } else if (i < 85) {
            // Yellow to Green
            r = 255 - (i - 43) * 6;
            g = 255;
            b = 0;
        } else if (i < 128) {
            // Green to Cyan
            r = 0;
            g = 255;
            b = (i - 85) * 6;
        } else if (i < 170) {
            // Cyan to Blue
            r = 0;
            g = 255 - (i - 128) * 6;
            b = 255;
        } else if (i < 213) {
            // Blue to Magenta
            r = (i - 170) * 6;
            g = 0;
            b = 255;
        } else {
            // Magenta to Red
            r = 255;
            g = 0;
            b = 255 - (i - 213) * 6;
        }
        Platform_Graphics_SetPaletteEntry(i, r, g, b);
    }
}

int run_demo() {
    printf("=== Red Alert Platform Demo ===\n\n");
    printf("Controls:\n");
    printf("  Mouse: Move to see position, click to see buttons\n");
    printf("  Keys:  Press any key to see it detected\n");
    printf("  Shift/Ctrl/Alt: Hold to see modifier state\n");
    printf("  ESC: Exit demo\n\n");

    // Initialize platform
    if (Platform_Init() != PLATFORM_RESULT_SUCCESS) {
        printf("Failed to initialize platform!\n");
        return 1;
    }

    // Initialize graphics
    if (Platform_Graphics_Init() != 0) {
        char err[256];
        Platform_GetLastError(reinterpret_cast<int8_t*>(err), sizeof(err));
        printf("Failed to initialize graphics: %s\n", err);
        Platform_Shutdown();
        return 1;
    }

    // Get back buffer
    uint8_t* pixels = nullptr;
    int32_t width, height, pitch;
    if (Platform_Graphics_GetBackBuffer(&pixels, &width, &height, &pitch) != 0) {
        printf("Failed to get back buffer!\n");
        Platform_Graphics_Shutdown();
        Platform_Shutdown();
        return 1;
    }

    printf("Display: %dx%d\n", width, height);
    printf("Running input demo...\n\n");

    // Setup grayscale palette (default is fine)

    // Main loop
    uint32_t frame = 0;
    uint32_t last_fps_time = Platform_GetTicks();

    while (!Platform_PollEvents()) {
        // Begin frame timing
        Platform_Frame_Begin();

        uint32_t time = Platform_GetTicks();

        // Begin input frame
        Platform_Input_Update();

        // Clear back buffer
        Platform_Graphics_ClearBackBuffer(0);

        // Get mouse position (scale from window to buffer coords)
        int32_t mouse_x, mouse_y;
        Platform_Mouse_GetPosition(&mouse_x, &mouse_y);
        // Window is 2x scaled, so divide by 2
        mouse_x /= 2;
        mouse_y /= 2;

        // Clamp to buffer bounds
        if (mouse_x < 0) mouse_x = 0;
        if (mouse_y < 0) mouse_y = 0;
        if (mouse_x >= width) mouse_x = width - 1;
        if (mouse_y >= height) mouse_y = height - 1;

        // Draw crosshair at mouse position
        for (int i = -10; i <= 10; i++) {
            int x = mouse_x + i;
            int y = mouse_y;
            if (x >= 0 && x < width) {
                pixels[y * pitch + x] = 255;
            }
            x = mouse_x;
            y = mouse_y + i;
            if (y >= 0 && y < height) {
                pixels[y * pitch + x] = 255;
            }
        }

        // Draw mouse button indicators (top left)
        // Left button - bright if pressed
        uint8_t left_color = Platform_Mouse_IsPressed(MOUSE_BUTTON_LEFT) ? 200 : 50;
        for (int y = 10; y < 30; y++) {
            for (int x = 10; x < 30; x++) {
                pixels[y * pitch + x] = left_color;
            }
        }

        // Right button
        uint8_t right_color = Platform_Mouse_IsPressed(MOUSE_BUTTON_RIGHT) ? 150 : 50;
        for (int y = 10; y < 30; y++) {
            for (int x = 35; x < 55; x++) {
                pixels[y * pitch + x] = right_color;
            }
        }

        // Middle button
        uint8_t middle_color = Platform_Mouse_IsPressed(MOUSE_BUTTON_MIDDLE) ? 100 : 50;
        for (int y = 10; y < 30; y++) {
            for (int x = 60; x < 80; x++) {
                pixels[y * pitch + x] = middle_color;
            }
        }

        // Draw modifier key indicators (top right area)
        // Shift indicator
        uint8_t shift_color = Platform_Key_ShiftDown() ? 255 : 30;
        for (int y = 10; y < 25; y++) {
            for (int x = width - 90; x < width - 70; x++) {
                pixels[y * pitch + x] = shift_color;
            }
        }

        // Ctrl indicator
        uint8_t ctrl_color = Platform_Key_CtrlDown() ? 255 : 30;
        for (int y = 10; y < 25; y++) {
            for (int x = width - 65; x < width - 45; x++) {
                pixels[y * pitch + x] = ctrl_color;
            }
        }

        // Alt indicator
        uint8_t alt_color = Platform_Key_AltDown() ? 255 : 30;
        for (int y = 10; y < 25; y++) {
            for (int x = width - 40; x < width - 20; x++) {
                pixels[y * pitch + x] = alt_color;
            }
        }

        // Draw border
        for (int x = 0; x < width; x++) {
            pixels[x] = 128;
            pixels[(height - 1) * pitch + x] = 128;
        }
        for (int y = 0; y < height; y++) {
            pixels[y * pitch] = 128;
            pixels[y * pitch + width - 1] = 128;
        }

        // Check for double-click (flash screen)
        if (Platform_Mouse_WasDoubleClicked(MOUSE_BUTTON_LEFT)) {
            // Flash the border bright
            for (int x = 0; x < width; x++) {
                pixels[x] = 255;
                pixels[(height - 1) * pitch + x] = 255;
            }
            for (int y = 0; y < height; y++) {
                pixels[y * pitch] = 255;
                pixels[y * pitch + width - 1] = 255;
            }
        }

        // Flip to screen
        Platform_Graphics_Flip();
        frame++;

        // End frame timing (handles FPS limiting)
        Platform_Frame_End();

        // Print status every second
        if (time - last_fps_time >= 1000) {
            double fps = Platform_Frame_GetFPS();
            printf("\rFPS: %.1f | Mouse: (%d,%d) | Shift:%d Ctrl:%d Alt:%d    ",
                   fps, mouse_x, mouse_y,
                   Platform_Key_ShiftDown(),
                   Platform_Key_CtrlDown(),
                   Platform_Key_AltDown());
            fflush(stdout);
            last_fps_time = time;
        }
    }

    printf("\n\nDemo ended. Total frames: %u\n", frame);

    // Cleanup
    Platform_Graphics_Shutdown();
    Platform_Shutdown();

    return 0;
}

// =============================================================================
// Test Mode
// =============================================================================

int run_tests() {
    printf("=== Platform Layer Compatibility Test ===\n\n");
    printf("Platform version: %d\n\n", Platform_GetVersion());

    int failures = 0;

    // Run tests
    failures += test_version();
    failures += test_init_shutdown();
    failures += test_error_handling();
    failures += test_logging();

    // Graphics test (requires Platform_Init for SDL context)
    {
        // Re-init for graphics test
        Platform_Init();
        failures += test_graphics();
        Platform_Shutdown();
    }

    // Input test
    {
        Platform_Init();
        failures += test_input();
        Platform_Shutdown();
    }

    // Timing test
    {
        Platform_Init();
        failures += test_timing();
        Platform_Shutdown();
    }

    // Memory test
    {
        Platform_Init();
        failures += test_memory();
        Platform_Mem_DumpLeaks();  // Should show no leaks
        Platform_Shutdown();
    }

    // File system test
    {
        Platform_Init();
        failures += test_files();
        Platform_Shutdown();
    }

    // Final init/shutdown for normal operation test
    printf("\n  Final init/shutdown cycle... ");
    if (Platform_Init() == PLATFORM_RESULT_SUCCESS) {
        Platform_Shutdown();
        printf("PASS\n");
    } else {
        printf("FAIL\n");
        failures++;
    }

    printf("\n=== Results ===\n");
    if (failures == 0) {
        printf("All tests passed!\n");
        printf("Test passed: init/shutdown cycle complete\n");
        return 0;
    } else {
        printf("%d test(s) failed\n", failures);
        return 1;
    }
}

// =============================================================================
// Main Entry Point
// =============================================================================

void print_usage(const char* program) {
    printf("Red Alert Platform Layer\n\n");
    printf("Usage: %s [options]\n\n", program);
    printf("Options:\n");
    printf("  --demo     Run interactive graphics demo\n");
    printf("  --test     Run platform compatibility tests\n");
    printf("  --help     Show this help message\n");
}

int main(int argc, char* argv[]) {
    bool demo_mode = false;
    bool test_mode = false;
    bool help_mode = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--demo") == 0) {
            demo_mode = true;
        } else if (strcmp(argv[i], "--test-init") == 0 ||
                   strcmp(argv[i], "--test") == 0) {
            test_mode = true;
        } else if (strcmp(argv[i], "--help") == 0 ||
                   strcmp(argv[i], "-h") == 0) {
            help_mode = true;
        }
    }

    if (help_mode) {
        print_usage(argv[0]);
        return 0;
    }

    if (demo_mode) {
        return run_demo();
    }

    // Default to test mode
    return run_tests();
}
