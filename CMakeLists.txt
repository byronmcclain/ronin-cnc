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

    # Graphics (Phase 15)
    src/game/graphics/graphics_buffer.cpp
    src/game/graphics/shape_renderer.cpp
    src/game/graphics/remap_tables.cpp
    src/game/graphics/tile_renderer.cpp
    src/game/graphics/palette_manager.cpp
    src/game/graphics/mouse_cursor.cpp
    src/game/graphics/render_pipeline.cpp
    src/game/graphics/sidebar_render.cpp
    src/game/graphics/radar_render.cpp

    # Viewport (Phase 15g)
    src/game/viewport.cpp
    src/game/scroll_manager.cpp

    # Input System (Phase 16)
    src/game/input/input_state.cpp
    src/game/input/input_mapper.cpp
    src/game/input/keyboard_handler.cpp
    src/game/input/mouse_handler.cpp
    src/game/input/selection_manager.cpp
    src/game/input/command_system.cpp
    src/game/input/scroll_processor.cpp
    src/game/input/input_integration.cpp

    # Audio System (Phase 17)
    src/game/audio/aud_file.cpp
    src/game/audio/sound_manager.cpp
    src/game/audio/music_player.cpp
    src/game/audio/voice_manager.cpp
    src/game/audio/audio_system.cpp
    src/game/audio/event_rate_limiter.cpp
    src/game/audio/audio_events.cpp
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
    include/game/graphics/graphics_buffer.h
    include/game/graphics/shape_renderer.h
    include/game/graphics/remap_tables.h
    include/game/graphics/tile_renderer.h
    include/game/graphics/palette_manager.h
    include/game/graphics/mouse_cursor.h
    include/game/graphics/render_layer.h
    include/game/graphics/render_pipeline.h
    include/game/graphics/sidebar_render.h
    include/game/graphics/radar_render.h
    include/game/viewport.h
    include/game/scroll_manager.h
    include/game/input/input_defs.h
    include/game/input/input_state.h
    include/game/input/game_action.h
    include/game/input/input_mapper.h
    include/game/input/keyboard_handler.h
    include/game/input/cursor_context.h
    include/game/input/mouse_handler.h
    include/game/input/selection_manager.h
    include/game/input/command_types.h
    include/game/input/command_system.h
    include/game/input/scroll_processor.h
    include/game/input/input_integration.h
    include/game/audio/aud_file.h
    include/game/audio/sound_effect.h
    include/game/audio/sound_manager.h
    include/game/audio/music_track.h
    include/game/audio/music_player.h
    include/game/audio/voice_event.h
    include/game/audio/voice_manager.h
    include/game/audio/audio_system.h
    include/game/audio/event_rate_limiter.h
    include/game/audio/audio_events.h
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

# =============================================================================
# GraphicsBuffer Test (Phase 15a)
# =============================================================================

add_executable(TestGraphicsBuffer src/test_graphics_buffer.cpp)

target_include_directories(TestGraphicsBuffer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/game/graphics
)

target_link_libraries(TestGraphicsBuffer PRIVATE
    game_core
    redalert_platform
)

add_dependencies(TestGraphicsBuffer generate_headers)

if(APPLE)
    target_link_libraries(TestGraphicsBuffer PRIVATE
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
add_test(NAME GraphicsBufferTests COMMAND TestGraphicsBuffer --quick)

# =============================================================================
# Shape Drawing Test (Phase 15b)
# =============================================================================

add_executable(TestShapeDrawing src/test_shape_drawing.cpp)

target_include_directories(TestShapeDrawing PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/game/graphics
)

target_link_libraries(TestShapeDrawing PRIVATE
    game_core
    redalert_platform
)

add_dependencies(TestShapeDrawing generate_headers)

if(APPLE)
    target_link_libraries(TestShapeDrawing PRIVATE
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

add_test(NAME ShapeDrawingTests COMMAND TestShapeDrawing --quick)

# =============================================================================
# Tile Rendering Test (Phase 15c)
# =============================================================================

add_executable(TestTileRendering src/test_tile_rendering.cpp)

target_include_directories(TestTileRendering PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/game/graphics
)

target_link_libraries(TestTileRendering PRIVATE
    game_core
    redalert_platform
)

add_dependencies(TestTileRendering generate_headers)

if(APPLE)
    target_link_libraries(TestTileRendering PRIVATE
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

add_test(NAME TileRenderingTests COMMAND TestTileRendering --quick)

# =============================================================================
# Palette Manager Test (Phase 15d)
# =============================================================================

add_executable(TestPaletteManager src/test_palette_manager.cpp)

target_include_directories(TestPaletteManager PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/game/graphics
)

target_link_libraries(TestPaletteManager PRIVATE
    game_core
    redalert_platform
)

add_dependencies(TestPaletteManager generate_headers)

if(APPLE)
    target_link_libraries(TestPaletteManager PRIVATE
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

add_test(NAME PaletteManagerTests COMMAND TestPaletteManager --quick)

# =============================================================================
# Mouse Cursor Test (Phase 15e)
# =============================================================================

add_executable(TestMouseCursor src/test_mouse_cursor.cpp)

target_include_directories(TestMouseCursor PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/game/graphics
)

target_link_libraries(TestMouseCursor PRIVATE
    game_core
    redalert_platform
)

add_dependencies(TestMouseCursor generate_headers)

if(APPLE)
    target_link_libraries(TestMouseCursor PRIVATE
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

add_test(NAME MouseCursorTests COMMAND TestMouseCursor --quick)

# =============================================================================
# Render Pipeline Test (Phase 15f)
# =============================================================================

add_executable(TestRenderPipeline src/test_render_pipeline.cpp)

target_include_directories(TestRenderPipeline PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/game/graphics
)

target_link_libraries(TestRenderPipeline PRIVATE
    game_core
    redalert_platform
)

add_dependencies(TestRenderPipeline generate_headers)

if(APPLE)
    target_link_libraries(TestRenderPipeline PRIVATE
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

add_test(NAME RenderPipelineTests COMMAND TestRenderPipeline --quick)

# =============================================================================
# Viewport Test (Phase 15g)
# =============================================================================

add_executable(TestViewport src/test_viewport.cpp)

target_include_directories(TestViewport PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/game/graphics
)

target_link_libraries(TestViewport PRIVATE
    game_core
    redalert_platform
)

add_dependencies(TestViewport generate_headers)

if(APPLE)
    target_link_libraries(TestViewport PRIVATE
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

add_test(NAME ViewportTests COMMAND TestViewport --quick)

# =============================================================================
# Graphics Integration Test (Phase 15h)
# =============================================================================

add_executable(TestGraphicsIntegration src/test_graphics_integration.cpp)

target_include_directories(TestGraphicsIntegration PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/game/graphics
)

target_link_libraries(TestGraphicsIntegration PRIVATE
    game_core
    redalert_platform
)

add_dependencies(TestGraphicsIntegration generate_headers)

if(APPLE)
    target_link_libraries(TestGraphicsIntegration PRIVATE
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

add_test(NAME GraphicsIntegrationTests COMMAND TestGraphicsIntegration --quick)

# =============================================================================
# Input Foundation Test (Phase 16a)
# =============================================================================

add_executable(TestInputFoundation src/test_input_foundation.cpp)

target_include_directories(TestInputFoundation PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/game/input
)

target_link_libraries(TestInputFoundation PRIVATE
    game_core
    redalert_platform
)

add_dependencies(TestInputFoundation generate_headers)

if(APPLE)
    target_link_libraries(TestInputFoundation PRIVATE
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

add_test(NAME InputFoundationTests COMMAND TestInputFoundation --quick)

# =============================================================================
# Input Mapper Test (Phase 16b)
# =============================================================================

add_executable(TestInputMapper src/test_input_mapper.cpp)

target_include_directories(TestInputMapper PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/game/input
)

target_link_libraries(TestInputMapper PRIVATE
    game_core
    redalert_platform
)

add_dependencies(TestInputMapper generate_headers)

if(APPLE)
    target_link_libraries(TestInputMapper PRIVATE
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

add_test(NAME InputMapperTests COMMAND TestInputMapper --quick)

# =============================================================================
# Keyboard Handler Test (Phase 16c)
# =============================================================================

add_executable(TestKeyboardHandler src/test_keyboard_handler.cpp)

target_include_directories(TestKeyboardHandler PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/game/input
)

target_link_libraries(TestKeyboardHandler PRIVATE
    game_core
    redalert_platform
)

add_dependencies(TestKeyboardHandler generate_headers)

if(APPLE)
    target_link_libraries(TestKeyboardHandler PRIVATE
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

add_test(NAME KeyboardHandlerTests COMMAND TestKeyboardHandler --quick)

# =============================================================================
# Mouse Handler Test (Phase 16d)
# =============================================================================

add_executable(TestMouseHandler src/test_mouse_handler.cpp)

target_include_directories(TestMouseHandler PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/game/input
)

target_link_libraries(TestMouseHandler PRIVATE
    game_core
    redalert_platform
)

add_dependencies(TestMouseHandler generate_headers)

if(APPLE)
    target_link_libraries(TestMouseHandler PRIVATE
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

add_test(NAME MouseHandlerTests COMMAND TestMouseHandler --quick)

# =============================================================================
# Selection System Test (Phase 16e)
# =============================================================================

add_executable(TestSelection src/test_selection.cpp)

target_include_directories(TestSelection PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/game/input
)

target_link_libraries(TestSelection PRIVATE
    game_core
    redalert_platform
)

add_dependencies(TestSelection generate_headers)

if(APPLE)
    target_link_libraries(TestSelection PRIVATE
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

add_test(NAME SelectionTests COMMAND TestSelection --quick)

# =============================================================================
# Command System Test (Phase 16f)
# =============================================================================

add_executable(TestCommandSystem src/test_command_system.cpp)

target_include_directories(TestCommandSystem PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/game/input
)

target_link_libraries(TestCommandSystem PRIVATE
    game_core
    redalert_platform
)

add_dependencies(TestCommandSystem generate_headers)

if(APPLE)
    target_link_libraries(TestCommandSystem PRIVATE
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

add_test(NAME CommandSystemTests COMMAND TestCommandSystem --quick)

# =============================================================================
# Scroll Processor Test (Phase 16g)
# =============================================================================

add_executable(TestScrollProcessor src/test_scroll_processor.cpp)

target_include_directories(TestScrollProcessor PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/game/input
)

target_link_libraries(TestScrollProcessor PRIVATE
    game_core
    redalert_platform
)

add_dependencies(TestScrollProcessor generate_headers)

if(APPLE)
    target_link_libraries(TestScrollProcessor PRIVATE
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

add_test(NAME ScrollProcessorTests COMMAND TestScrollProcessor --quick)

# =============================================================================
# Input Integration Test (Phase 16h)
# =============================================================================

add_executable(TestInputIntegration src/test_input_integration.cpp)

target_include_directories(TestInputIntegration PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/game/input
)

target_link_libraries(TestInputIntegration PRIVATE
    game_core
    redalert_platform
)

add_dependencies(TestInputIntegration generate_headers)

if(APPLE)
    target_link_libraries(TestInputIntegration PRIVATE
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

add_test(NAME InputIntegrationTests COMMAND TestInputIntegration --quick)

# =============================================================================
# AUD File Loader Test (Phase 17a)
# =============================================================================

add_executable(TestAudLoader src/test_aud_loader.cpp)

target_include_directories(TestAudLoader PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/game/audio
)

target_link_libraries(TestAudLoader PRIVATE
    game_core
    redalert_platform
)

add_dependencies(TestAudLoader generate_headers)

if(APPLE)
    target_link_libraries(TestAudLoader PRIVATE
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

add_test(NAME AudLoaderTests COMMAND TestAudLoader --quick)

# =============================================================================
# Sound Manager Test (Phase 17b)
# =============================================================================

add_executable(TestSoundManager src/test_sound_manager.cpp)

target_include_directories(TestSoundManager PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/game/audio
)

target_link_libraries(TestSoundManager PRIVATE
    game_core
    redalert_platform
)

add_dependencies(TestSoundManager generate_headers)

if(APPLE)
    target_link_libraries(TestSoundManager PRIVATE
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

add_test(NAME SoundManagerTests COMMAND TestSoundManager --quick)

# =============================================================================
# Music Player Test (Phase 17c)
# =============================================================================

add_executable(TestMusicPlayer src/test_music_player.cpp)

target_include_directories(TestMusicPlayer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/game/audio
)

target_link_libraries(TestMusicPlayer PRIVATE
    game_core
    redalert_platform
)

add_dependencies(TestMusicPlayer generate_headers)

if(APPLE)
    target_link_libraries(TestMusicPlayer PRIVATE
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

add_test(NAME MusicPlayerTests COMMAND TestMusicPlayer --quick)

# =============================================================================
# Voice Manager Test (Phase 17d)
# =============================================================================

add_executable(TestVoiceManager src/test_voice_manager.cpp)

target_include_directories(TestVoiceManager PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/game/audio
)

target_link_libraries(TestVoiceManager PRIVATE
    game_core
    redalert_platform
)

add_dependencies(TestVoiceManager generate_headers)

if(APPLE)
    target_link_libraries(TestVoiceManager PRIVATE
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

add_test(NAME VoiceManagerTests COMMAND TestVoiceManager --quick)

# =============================================================================
# Audio Integration Test (Phase 17e)
# =============================================================================

add_executable(TestAudioIntegration src/test_audio_integration.cpp)

target_include_directories(TestAudioIntegration PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/game/audio
)

target_link_libraries(TestAudioIntegration PRIVATE
    game_core
    redalert_platform
)

add_dependencies(TestAudioIntegration generate_headers)

if(APPLE)
    target_link_libraries(TestAudioIntegration PRIVATE
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

add_test(NAME AudioIntegrationTests COMMAND TestAudioIntegration --quick)

# =============================================================================
# Audio Events Test (Phase 17f)
# =============================================================================

add_executable(TestAudioEvents src/test_audio_events.cpp)

target_include_directories(TestAudioEvents PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/game
    ${CMAKE_SOURCE_DIR}/include/game/audio
)

target_link_libraries(TestAudioEvents PRIVATE
    game_core
    redalert_platform
)

add_dependencies(TestAudioEvents generate_headers)

if(APPLE)
    target_link_libraries(TestAudioEvents PRIVATE
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

add_test(NAME AudioEventsTests COMMAND TestAudioEvents --quick)
