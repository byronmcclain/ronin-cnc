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

# =============================================================================
# Asset System Integration Test (Phase 12)
# =============================================================================

add_executable(TestAssets src/test_assets.cpp)

target_include_directories(TestAssets PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

add_dependencies(TestAssets generate_headers)

target_link_libraries(TestAssets PRIVATE redalert_platform)

if(APPLE)
    target_link_libraries(TestAssets PRIVATE
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
# MIX Decryption Test (Phase 12i)
# =============================================================================

add_executable(TestMixDecrypt src/test_mix_decrypt.cpp)

target_include_directories(TestMixDecrypt PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

add_dependencies(TestMixDecrypt generate_headers)

target_link_libraries(TestMixDecrypt PRIVATE redalert_platform)

if(APPLE)
    target_link_libraries(TestMixDecrypt PRIVATE
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
# Compatibility Layer Library
# =============================================================================
# Provides Windows/DOS API compatibility for original Red Alert code

# Compatibility library source
add_library(compat STATIC
    src/compat/compat.cpp
)

# Include directories
target_include_directories(compat PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/compat
)

# Link against platform layer
target_link_libraries(compat PUBLIC
    redalert_platform
)

# Compile definitions
target_compile_definitions(compat PUBLIC
    # These prevent real Windows headers from being included
    _WINDOWS_
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

# C++ standard
target_compile_features(compat PUBLIC cxx_std_17)

# Ensure headers are generated before compiling
add_dependencies(compat generate_headers)

# Suppress warnings for legacy compatibility code
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(compat PRIVATE
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-sign-compare
    )
endif()

# =============================================================================
# Compatibility Layer Test
# =============================================================================

add_executable(TestCompat src/test_compat.cpp)

target_link_libraries(TestCompat PRIVATE
    compat
    redalert_platform
)

add_dependencies(TestCompat generate_headers)

if(APPLE)
    target_link_libraries(TestCompat PRIVATE
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
# Original Code Compilation Test
# =============================================================================

add_executable(TestOriginalCompile src/test_original_compile.cpp)

target_link_libraries(TestOriginalCompile PRIVATE
    compat
    redalert_platform
)

add_dependencies(TestOriginalCompile generate_headers)

if(APPLE)
    target_link_libraries(TestOriginalCompile PRIVATE
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
# Game Code Library (Phase 14)
# =============================================================================
# Ported Red Alert game code

# Game source files
set(GAME_SOURCES
    # Core types
    src/game/core/coord.cpp
    src/game/core/house.cpp

    # Display system
    src/game/display/gscreen.cpp
    src/game/display/gadget.cpp
    src/game/display/map.cpp
    src/game/display/display.cpp

    # Map cells
    src/game/map/cell.cpp

    # Object system
    src/game/object/object.cpp

    # Type classes
    src/game/types/types.cpp

    # Main loop
    src/game/game_loop.cpp
    src/game/init.cpp
)

# Game header files (for IDE)
set(GAME_HEADERS
    include/game/game.h
    include/game/coord.h
    include/game/facing.h
    include/game/house.h
    include/game/weapon.h
    include/game/gscreen.h
    include/game/gadget.h
    include/game/map.h
    include/game/cell.h
    include/game/display.h
    include/game/abstract.h
    include/game/object.h
    include/game/techno.h
    include/game/mission.h
    include/game/core/rtti.h
    include/game/types/abstracttype.h
    include/game/types/objecttype.h
    include/game/types/technotype.h
    include/game/types/unittype.h
    include/game/types/buildingtype.h
)

# Create game library
add_library(game_core STATIC
    ${GAME_SOURCES}
    ${GAME_HEADERS}
)

target_include_directories(game_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/compat
)

target_link_libraries(game_core PUBLIC
    redalert_platform
)

target_compile_features(game_core PUBLIC cxx_std_17)

add_dependencies(game_core generate_headers)

# Suppress warnings for legacy code patterns
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(game_core PRIVATE
        -Wno-unused-parameter
        -Wno-unused-variable
    )
endif()

if(APPLE)
    target_link_libraries(game_core PUBLIC
        "-framework CoreFoundation"
        "-framework Security"
    )
endif()

# =============================================================================
# RedAlertGame Executable
# =============================================================================

add_executable(RedAlertGame
    src/main_game.cpp
)

target_include_directories(RedAlertGame PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(RedAlertGame PRIVATE
    game_core
    redalert_platform
)

target_compile_features(RedAlertGame PRIVATE cxx_std_17)

add_dependencies(RedAlertGame generate_headers)

if(APPLE)
    target_link_libraries(RedAlertGame PRIVATE
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
# Game Structure Test
# =============================================================================

add_executable(TestGameStructure src/test_game_structure.cpp)

target_link_libraries(TestGameStructure PRIVATE
    game_core
    redalert_platform
)

add_dependencies(TestGameStructure generate_headers)

if(APPLE)
    target_link_libraries(TestGameStructure PRIVATE
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
add_test(NAME AssetTests COMMAND TestAssets)
add_test(NAME MixDecryptTests COMMAND TestMixDecrypt)
add_test(NAME CompatTests COMMAND TestCompat)
add_test(NAME OriginalCompileTests COMMAND TestOriginalCompile)
add_test(NAME GameStructureTests COMMAND TestGameStructure)
