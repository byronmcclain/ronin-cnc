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

# For macOS deployment target - only set if we have a valid value
if(APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET)
    corrosion_set_env_vars(redalert_platform
        "MACOSX_DEPLOYMENT_TARGET=10.15"
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
# Test Executable
# =============================================================================

add_executable(RedAlert src/main.cpp)

target_include_directories(RedAlert PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Ensure headers are generated before compiling
add_dependencies(RedAlert generate_headers)

# Link against Rust platform library
target_link_libraries(RedAlert PRIVATE redalert_platform)

# macOS: Link required frameworks (including SDL2 dependencies)
if(APPLE)
    target_link_libraries(RedAlert PRIVATE
        "-framework CoreFoundation"
        "-framework Security"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework Carbon"
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework ForceFeedback"
        "-framework CoreVideo"
        "-framework Metal"
        "-framework QuartzCore"
        "-framework GameController"
        "-framework CoreHaptics"
        iconv
    )
endif()

# =============================================================================
# Audio Integration Test
# =============================================================================

add_executable(TestAudio src/test_audio.cpp)

target_include_directories(TestAudio PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

add_dependencies(TestAudio generate_headers)

target_link_libraries(TestAudio PRIVATE redalert_platform)

if(APPLE)
    target_link_libraries(TestAudio PRIVATE
        "-framework CoreFoundation"
        "-framework Security"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework Carbon"
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework ForceFeedback"
        "-framework CoreVideo"
        "-framework Metal"
        "-framework QuartzCore"
        "-framework GameController"
        "-framework CoreHaptics"
        iconv
    )
endif()

# =============================================================================
# Assembly Replacement Integration Test
# =============================================================================

add_executable(TestAsm src/test_asm.cpp)

target_include_directories(TestAsm PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

add_dependencies(TestAsm generate_headers)

target_link_libraries(TestAsm PRIVATE redalert_platform)

if(APPLE)
    target_link_libraries(TestAsm PRIVATE
        "-framework CoreFoundation"
        "-framework Security"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework Carbon"
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework ForceFeedback"
        "-framework CoreVideo"
        "-framework Metal"
        "-framework QuartzCore"
        "-framework GameController"
        "-framework CoreHaptics"
        iconv
    )
endif()

# =============================================================================
# Platform Integration Test
# =============================================================================

add_executable(TestPlatform src/test_platform.cpp)

target_include_directories(TestPlatform PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

add_dependencies(TestPlatform generate_headers)

target_link_libraries(TestPlatform PRIVATE redalert_platform)

if(APPLE)
    target_link_libraries(TestPlatform PRIVATE
        "-framework CoreFoundation"
        "-framework Security"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework Carbon"
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework ForceFeedback"
        "-framework CoreVideo"
        "-framework Metal"
        "-framework QuartzCore"
        "-framework GameController"
        "-framework CoreHaptics"
        iconv
    )
endif()

# Enable testing
enable_testing()
add_test(NAME PlatformTests COMMAND TestPlatform)
add_test(NAME PlatformQuickTests COMMAND TestPlatform --quick)
