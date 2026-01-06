# Task 01d: Mixed C++/Rust Link Test

## Dependencies
- Task 01c must be complete (CMake builds successfully)
- `build/RedAlert` executable exists
- Platform library linked correctly

## Context
This is the final validation task for Phase 01. We verify that the mixed C++/Rust binary actually runs correctly, calling into Rust code from C++ and receiving proper return values. This confirms the entire build system is working.

## Objective
Execute the built binary and verify it successfully initializes and shuts down the Rust platform layer, proving FFI works correctly.

## Deliverables
- [ ] `build/RedAlert` runs without crash
- [ ] Platform_Init() returns success
- [ ] Platform_IsInitialized() returns true after init
- [ ] Platform_Shutdown() completes
- [ ] Exit code is 0
- [ ] Verification script passes

## Files to Modify

### src/main.cpp (if needed)
The main.cpp from task 01c should already be correct. If not, ensure it contains:
```cpp
#include <cstdio>
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
```

## Verification Command
```bash
cd /path/to/repo
cmake --build build 2>&1 && \
./build/RedAlert --test-init 2>&1 | grep -q "Test passed" && \
echo "VERIFY_SUCCESS"
```

## Implementation Steps
1. Rebuild with `cmake --build build`
2. Run `./build/RedAlert --test-init`
3. Check output contains "Platform initialized successfully"
4. Check exit code is 0
5. If executable crashes, check for missing symbols with `nm`
6. If init fails, check Rust code for issues
7. Run verification script
8. If verification fails, analyze and fix

## Expected Output
```
$ ./build/RedAlert --test-init
Red Alert - Platform Test
Platform version: 1
Platform initialized successfully
Test passed: init/shutdown cycle complete
```

## Debugging

### Check symbols are exported
```bash
nm build/libredalert_platform.a | grep Platform_
```
Should show:
```
T _Platform_Init
T _Platform_Shutdown
T _Platform_IsInitialized
T _Platform_GetVersion
T _Platform_GetLastError
```

### Check linking
```bash
otool -L build/RedAlert
```

### Run with debug output
```bash
RUST_BACKTRACE=1 ./build/RedAlert --test-init
```

## Common Issues

### "dyld: Library not loaded"
- Static library not linked correctly
- Check `target_link_libraries` in CMakeLists.txt

### "undefined symbol: Platform_*"
- cbindgen didn't export the function
- Check `#[no_mangle] pub extern "C"` on Rust functions

### Segmentation fault
- Panic in Rust code crossing FFI boundary
- Add `RUST_BACKTRACE=1` to see panic location
- Check catch_panic wrapper is working

### "Platform reports not initialized"
- State management issue in Rust
- Check PLATFORM_STATE mutex logic

## Completion Promise
When verification passes (executable runs and exits with code 0), output:
<promise>TASK_01D_COMPLETE</promise>

## Escape Hatch
If stuck after 10 iterations:
- Document blocking issue in `ralph-tasks/blocked/01d.md`
- Include crash output or error messages
- Output: <promise>TASK_01D_BLOCKED</promise>

## Max Iterations
10

## Phase 01 Complete
When this task completes, the entire build system is working:
- Rust crate builds ✓
- cbindgen generates headers ✓
- CMake orchestrates build ✓
- Mixed C++/Rust linking works ✓
- FFI calls succeed ✓

Proceed to Phase 02: Platform Abstraction.
