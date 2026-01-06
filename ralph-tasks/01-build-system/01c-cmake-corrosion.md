# Task 01c: CMake Integration with Corrosion

## Dependencies
- Task 01a must be complete (Rust crate builds)
- Task 01b must be complete (cbindgen generates headers)
- `platform/` directory exists with working Cargo.toml
- `include/platform.h` can be generated

## Context
We need CMake to orchestrate building both the Rust platform library and C++ game code. Corrosion is a CMake module that integrates Cargo builds seamlessly, allowing us to treat Rust crates as CMake targets.

## Objective
Create a root CMakeLists.txt that uses Corrosion to build the Rust platform crate and configure header generation.

## Deliverables
- [ ] `CMakeLists.txt` - Root CMake configuration with Corrosion
- [ ] `cmake --build build` succeeds
- [ ] Rust library is built via CMake
- [ ] Verification passes

## Files to Create

### CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.20)
project(RedAlert VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# =============================================================================
# Platform Detection
# =============================================================================

if(APPLE)
    set(PLATFORM_MACOS TRUE)
    add_compile_definitions(PLATFORM_MACOS)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")
elseif(UNIX)
    set(PLATFORM_LINUX TRUE)
    add_compile_definitions(PLATFORM_LINUX)
endif()

# =============================================================================
# Rust Integration via Corrosion
# =============================================================================

include(FetchContent)

# Fetch corrosion for CMake-Cargo integration
FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG v0.5
)
FetchContent_MakeAvailable(Corrosion)

# Import the Rust platform crate
corrosion_import_crate(MANIFEST_PATH platform/Cargo.toml)

# For macOS deployment target
if(APPLE)
    corrosion_set_env_vars(redalert_platform
        MACOSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}
    )
endif()

# =============================================================================
# Header Generation (cbindgen)
# =============================================================================

set(PLATFORM_HEADER ${CMAKE_SOURCE_DIR}/include/platform.h)

add_custom_command(
    OUTPUT ${PLATFORM_HEADER}
    COMMAND cargo build --manifest-path ${CMAKE_SOURCE_DIR}/platform/Cargo.toml
            --features cbindgen-run
    DEPENDS ${CMAKE_SOURCE_DIR}/platform/src/lib.rs
            ${CMAKE_SOURCE_DIR}/platform/cbindgen.toml
    COMMENT "Generating platform.h from Rust sources"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/platform
)

add_custom_target(generate_headers DEPENDS ${PLATFORM_HEADER})

# =============================================================================
# Placeholder Executable (for testing build system)
# =============================================================================

# Create a minimal test executable to verify linking works
add_executable(RedAlert src/main.cpp)

target_include_directories(RedAlert PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Ensure headers are generated before compiling
add_dependencies(RedAlert generate_headers)

# Link against Rust platform library
target_link_libraries(RedAlert PRIVATE redalert_platform)

# macOS: Link required frameworks
if(APPLE)
    target_link_libraries(RedAlert PRIVATE
        "-framework CoreFoundation"
        "-framework Security"
    )
endif()
```

### src/main.cpp
```cpp
#include <cstdio>
#include "platform.h"

int main(int argc, char* argv[]) {
    printf("Red Alert - Platform Test\n");
    printf("Platform version: %d\n", Platform_GetVersion());

    // Test init/shutdown cycle
    PlatformResult result = Platform_Init();
    if (result != PLATFORM_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to initialize platform: %d\n", result);
        return 1;
    }

    printf("Platform initialized: %s\n", Platform_IsInitialized() ? "yes" : "no");

    // Cleanup
    Platform_Shutdown();
    printf("Platform shutdown complete\n");

    return 0;
}
```

## Verification Command
```bash
# Clean and configure
rm -rf build
cmake -B build -G Ninja 2>&1 && \
cmake --build build 2>&1 && \
test -f build/RedAlert && \
echo "VERIFY_SUCCESS"
```

## Implementation Steps
1. Create `CMakeLists.txt` at repository root
2. Create `src/` directory
3. Create `src/main.cpp` test file
4. Run `cmake -B build -G Ninja`
5. Run `cmake --build build`
6. Verify `build/RedAlert` executable exists
7. If any step fails, analyze error and fix
8. Repeat until verification passes

## Expected Output
After successful build:
```
$ ./build/RedAlert
Red Alert - Platform Test
Platform version: 1
Platform initialized: yes
Platform shutdown complete
```

## Common Issues

### Corrosion fetch fails
- Check internet connectivity
- Try: `cmake -B build -G Ninja --fresh`

### Rust library not found
- Ensure `platform/Cargo.toml` has correct `[lib]` section
- Check Corrosion output for cargo errors

### Linking errors on macOS
- Add required frameworks (CoreFoundation, Security)
- Check that Rust was built with same architecture

## Completion Promise
When verification passes (CMake configures and builds successfully), output:
<promise>TASK_01C_COMPLETE</promise>

## Escape Hatch
If stuck after 15 iterations:
- Document blocking issue in `ralph-tasks/blocked/01c.md`
- Include CMake and cargo error output
- Output: <promise>TASK_01C_BLOCKED</promise>

## Max Iterations
15
