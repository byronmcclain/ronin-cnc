#include <cstdio>
#include <cstring>
#include "platform.h"

int main(int argc, char* argv[]) {
    // Handle --test-init flag for automated testing
    bool test_mode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--test-init") == 0) {
            test_mode = true;
        }
    }

    printf("Red Alert - Platform Test\n");
    printf("Platform version: %d\n", Platform_GetVersion());

    // Test init/shutdown cycle
    PlatformResult result = Platform_Init();
    if (result != PLATFORM_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to initialize platform: %d\n", result);
        return 1;
    }

    if (!Platform_IsInitialized()) {
        fprintf(stderr, "Platform reports not initialized after init!\n");
        return 1;
    }

    printf("Platform initialized successfully\n");

    // In test mode, just verify init works and exit
    if (test_mode) {
        Platform_Shutdown();
        printf("Test passed: init/shutdown cycle complete\n");
        return 0;
    }

    // Normal operation would continue here...

    // Cleanup
    Platform_Shutdown();
    printf("Platform shutdown complete\n");

    return 0;
}
