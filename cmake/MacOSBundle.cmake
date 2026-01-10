# cmake/MacOSBundle.cmake
# macOS App Bundle Configuration
# Task 18i - macOS App Bundle

# Only apply on macOS
if(NOT APPLE)
    return()
endif()

#==============================================================================
# Bundle Configuration
#==============================================================================

set(MACOSX_BUNDLE_BUNDLE_NAME "Red Alert")
set(MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.westwood.redalert")
set(MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 1996 Westwood Studios / EA")
set(MACOSX_BUNDLE_ICON_FILE "icon.icns")
set(MACOSX_BUNDLE_INFO_STRING "Red Alert macOS Port")

# Minimum macOS version
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

# Universal binary (Intel + Apple Silicon)
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures")

#==============================================================================
# Bundle Target Function
#==============================================================================

function(configure_macos_bundle TARGET_NAME)
    # Enable bundle
    set_target_properties(${TARGET_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/resources/macos/Info.plist.in"

        # Output location
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"

        # Install RPATH for bundled libraries
        INSTALL_RPATH "@executable_path/../Frameworks"
        BUILD_WITH_INSTALL_RPATH TRUE
    )

    # Get bundle path
    set(BUNDLE_PATH "$<TARGET_BUNDLE_DIR:${TARGET_NAME}>")

    # Copy icon (if exists)
    if(EXISTS "${CMAKE_SOURCE_DIR}/resources/macos/icon.icns")
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "${BUNDLE_PATH}/Contents/Resources"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/resources/macos/icon.icns"
                "${BUNDLE_PATH}/Contents/Resources/"
            COMMENT "Copying app icon"
        )
    endif()

    # Copy PkgInfo
    if(EXISTS "${CMAKE_SOURCE_DIR}/resources/macos/PkgInfo")
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/resources/macos/PkgInfo"
                "${BUNDLE_PATH}/Contents/"
            COMMENT "Copying PkgInfo"
        )
    endif()

    # Copy game assets (if exists)
    if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "${BUNDLE_PATH}/Contents/Resources/Assets"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/assets"
                "${BUNDLE_PATH}/Contents/Resources/Assets"
            COMMENT "Copying game assets"
        )
    endif()

    # Copy embedded frameworks (if any)
    # add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    #     COMMAND ${CMAKE_COMMAND} -E make_directory
    #         "${BUNDLE_PATH}/Contents/Frameworks"
    #     COMMAND ${CMAKE_COMMAND} -E copy_if_different
    #         "${SDL2_LIBRARY}/SDL2.framework"
    #         "${BUNDLE_PATH}/Contents/Frameworks/"
    #     COMMENT "Copying frameworks"
    # )
endfunction()

#==============================================================================
# Icon Generation
#==============================================================================

function(generate_app_icon)
    # Generate .icns from .iconset directory
    set(ICONSET_DIR "${CMAKE_SOURCE_DIR}/resources/macos/icon.iconset")
    set(ICNS_FILE "${CMAKE_SOURCE_DIR}/resources/macos/icon.icns")

    # Only regenerate if iconset is newer
    if(EXISTS "${ICONSET_DIR}")
        add_custom_command(
            OUTPUT ${ICNS_FILE}
            COMMAND iconutil -c icns -o ${ICNS_FILE} ${ICONSET_DIR}
            DEPENDS ${ICONSET_DIR}
            COMMENT "Generating app icon"
        )

        add_custom_target(AppIcon DEPENDS ${ICNS_FILE})
    endif()
endfunction()

#==============================================================================
# Code Signing Target
#==============================================================================

function(add_codesign_target TARGET_NAME)
    set(BUNDLE_PATH "$<TARGET_BUNDLE_DIR:${TARGET_NAME}>")

    add_custom_target(codesign
        COMMAND codesign --force --deep --sign - "${BUNDLE_PATH}"
        DEPENDS ${TARGET_NAME}
        COMMENT "Ad-hoc code signing"
    )

    add_custom_target(codesign-release
        COMMAND "${CMAKE_SOURCE_DIR}/scripts/sign_bundle.sh" "${BUNDLE_PATH}"
        DEPENDS ${TARGET_NAME}
        COMMENT "Release code signing"
    )
endfunction()

#==============================================================================
# DMG Creation Target
#==============================================================================

function(add_dmg_target TARGET_NAME)
    set(BUNDLE_PATH "$<TARGET_BUNDLE_DIR:${TARGET_NAME}>")
    set(DMG_NAME "${TARGET_NAME}-${PROJECT_VERSION}.dmg")

    add_custom_target(dmg
        COMMAND "${CMAKE_SOURCE_DIR}/scripts/create_dmg.sh"
            "${BUNDLE_PATH}"
            "${CMAKE_BINARY_DIR}/${DMG_NAME}"
        DEPENDS ${TARGET_NAME}
        COMMENT "Creating DMG"
    )
endfunction()
