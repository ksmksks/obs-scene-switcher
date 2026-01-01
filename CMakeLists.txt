cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version} LANGUAGES C CXX)

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" ON)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs ixwebsocket bcrypt)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

# Enable TLS for IXWebSocket
set(USE_TLS ON CACHE BOOL "Enable TLS support" FORCE)
set(USE_MBED_TLS ON CACHE BOOL "Use mbed TLS" FORCE)

# IXWebSocket (vendor)
add_subdirectory(src/vendor/ixwebsocket)

# nlohmann/json (vendor)
add_subdirectory(src/vendor/json)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)

# Force C++ compile for .cpp plugin source
set_source_files_properties(
    src/plugin-main.cpp
    src/obs_scene_switcher.cpp
    src/obs_scene_switcher.hpp
    src/oauth/http_server.cpp
    src/oauth/http_server.hpp
    src/oauth/twitch_oauth.cpp
    src/oauth/twitch_oauth.hpp
    src/eventsub/eventsub_client.cpp
    src/eventsub/eventsub_client.hpp
    src/eventsub/twitch_event_types.h
    src/obs/scene_switcher.cpp
    src/obs/scene_switcher.hpp
    src/obs/config_manager.cpp
    src/obs/config_manager.hpp
    src/ui/plugin_dock.cpp
    src/ui/plugin_dock.hpp
    src/ui/plugin_properties.cpp
    src/ui/plugin_properties.h
    src/ui/login_widget.cpp
    src/ui/login_widget.hpp
    src/ui/auth_settings_dialog.cpp
    src/ui/auth_settings_dialog.hpp
    src/ui/dock_main_widget.cpp
    src/ui/dock_main_widget.hpp
    src/ui/settings_window.cpp
    src/ui/settings_window.hpp
    src/ui/rule_row.cpp
    src/ui/rule_row.hpp
    src/core/reward_rule.hpp
    src/update/update_checker.cpp
    src/update/update_checker.hpp
    src/i18n/locale_manager.cpp
    src/i18n/locale_manager.hpp
    PROPERTIES LANGUAGE CXX
)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        src/plugin-main.cpp
        src/obs_scene_switcher.cpp
        src/obs_scene_switcher.hpp

        # OAuth
        src/oauth/http_server.cpp
        src/oauth/http_server.hpp
        src/oauth/twitch_oauth.cpp
        src/oauth/twitch_oauth.hpp

        # EventSub
        src/eventsub/eventsub_client.cpp
        src/eventsub/eventsub_client.hpp

        # OBS
        src/obs/scene_switcher.cpp
        src/obs/scene_switcher.hpp
        src/obs/config_manager.cpp
        src/obs/config_manager.hpp

        # UI
        src/ui/plugin_dock.cpp
        src/ui/plugin_dock.hpp
        src/ui/plugin_properties.cpp
        src/ui/plugin_properties.h
        src/ui/login_widget.cpp
        src/ui/login_widget.hpp
        src/ui/auth_settings_dialog.cpp
        src/ui/auth_settings_dialog.hpp
        src/ui/dock_main_widget.cpp
        src/ui/dock_main_widget.hpp
        src/ui/settings_window.cpp
        src/ui/settings_window.hpp
        src/ui/rule_row.cpp
        src/ui/rule_row.hpp

        # Core
        src/core/reward_rule.hpp
        
        # Update
        src/update/update_checker.cpp
        src/update/update_checker.hpp
        
        # I18n
        src/i18n/locale_manager.cpp
        src/i18n/locale_manager.hpp
)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

# Add data directory (locale files, etc.) ---
install(
    DIRECTORY data/
    DESTINATION ${OBS_DATA_DEST}/${_name}
)

# ========== NSIS Installer Target (Windows only) ==========
if(WIN32)
    # Find makensis.exe
    find_program(MAKENSIS_EXE makensis.exe
        PATHS
            "C:/Program Files (x86)/NSIS"
            "C:/Program Files/NSIS"
            "$ENV{ProgramFiles}/NSIS"
        PATH_SUFFIXES bin
    )
    
    if(MAKENSIS_EXE)
        message(STATUS "Found NSIS: ${MAKENSIS_EXE}")
        
        # Create a script to build installer
        file(WRITE "${CMAKE_BINARY_DIR}/build_installer.cmake" "
# Copy DLL
file(MAKE_DIRECTORY \"${CMAKE_SOURCE_DIR}/installer/files/obs-studio/obs-plugins/64bit\")
file(COPY_FILE 
    \"\${DLL_PATH}\"
    \"${CMAKE_SOURCE_DIR}/installer/files/obs-studio/obs-plugins/64bit/obs-scene-switcher.dll\"
)

# Copy data files
file(MAKE_DIRECTORY \"${CMAKE_SOURCE_DIR}/installer/files/obs-studio/data/obs-plugins/obs-scene-switcher\")
file(COPY \"${CMAKE_SOURCE_DIR}/data/\"
    DESTINATION \"${CMAKE_SOURCE_DIR}/installer/files/obs-studio/data/obs-plugins/obs-scene-switcher\"
)

# Run NSIS
execute_process(
    COMMAND \"${MAKENSIS_EXE}\" /DPRODUCT_VERSION=${CMAKE_PROJECT_VERSION} \"${CMAKE_SOURCE_DIR}/installer/setup.nsi\"
    WORKING_DIRECTORY \"${CMAKE_SOURCE_DIR}/installer\"
    RESULT_VARIABLE result
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR \"NSIS installer build failed\")
endif()

message(STATUS \"Installer created: ${CMAKE_SOURCE_DIR}/installer/obs-scene-switcher-${CMAKE_PROJECT_VERSION}-installer.exe\")
")
        
        # Installer target (for manual invocation)
        add_custom_target(installer
            DEPENDS ${CMAKE_PROJECT_NAME}
            COMMAND ${CMAKE_COMMAND} 
                -DDLL_PATH=$<TARGET_FILE:${CMAKE_PROJECT_NAME}>
                -P "${CMAKE_BINARY_DIR}/build_installer.cmake"
            COMMENT "Building NSIS installer for ${CMAKE_PROJECT_NAME} v${CMAKE_PROJECT_VERSION}"
            VERBATIM
        )
        
        message(STATUS "NSIS installer target created.")
        message(STATUS "  Build installer: cmake --build build_x64 --target installer --config Release")
        message(STATUS "  Or in Visual Studio: Build → installer project")
    else()
        message(WARNING "NSIS not found. Installer target will not be available.")
        message(WARNING "  Install NSIS from https://nsis.sourceforge.io/")
    endif()
    
    # Manual installation ZIP target
    add_custom_target(package-zip
        DEPENDS ${CMAKE_PROJECT_NAME}
        
        # Create directory structure
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_SOURCE_DIR}/release/obs-scene-switcher-${CMAKE_PROJECT_VERSION}"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_SOURCE_DIR}/release/obs-scene-switcher-${CMAKE_PROJECT_VERSION}/obs-plugins/64bit"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_SOURCE_DIR}/release/obs-scene-switcher-${CMAKE_PROJECT_VERSION}/data/obs-plugins/obs-scene-switcher"
        
        # Copy DLL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
            "${CMAKE_SOURCE_DIR}/release/obs-scene-switcher-${CMAKE_PROJECT_VERSION}/obs-plugins/64bit/obs-scene-switcher.dll"
        
        # Copy data files
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/data"
            "${CMAKE_SOURCE_DIR}/release/obs-scene-switcher-${CMAKE_PROJECT_VERSION}/data/obs-plugins/obs-scene-switcher"
        
        # Create INSTALL.txt
        COMMAND ${CMAKE_COMMAND} -E echo "OBS Scene Switcher v${CMAKE_PROJECT_VERSION} - Manual Installation" > "${CMAKE_SOURCE_DIR}/release/obs-scene-switcher-${CMAKE_PROJECT_VERSION}/INSTALL.txt"
        COMMAND ${CMAKE_COMMAND} -E echo "" >> "${CMAKE_SOURCE_DIR}/release/obs-scene-switcher-${CMAKE_PROJECT_VERSION}/INSTALL.txt"
        COMMAND ${CMAKE_COMMAND} -E echo "Installation Instructions:" >> "${CMAKE_SOURCE_DIR}/release/obs-scene-switcher-${CMAKE_PROJECT_VERSION}/INSTALL.txt"
        COMMAND ${CMAKE_COMMAND} -E echo "1. Close OBS Studio if it is running" >> "${CMAKE_SOURCE_DIR}/release/obs-scene-switcher-${CMAKE_PROJECT_VERSION}/INSTALL.txt"
        COMMAND ${CMAKE_COMMAND} -E echo "2. Copy 'obs-plugins' folder to your OBS Studio installation directory" >> "${CMAKE_SOURCE_DIR}/release/obs-scene-switcher-${CMAKE_PROJECT_VERSION}/INSTALL.txt"
        COMMAND ${CMAKE_COMMAND} -E echo "   Default: C:\\Program Files\\obs-studio\\" >> "${CMAKE_SOURCE_DIR}/release/obs-scene-switcher-${CMAKE_PROJECT_VERSION}/INSTALL.txt"
        COMMAND ${CMAKE_COMMAND} -E echo "3. Copy 'data' folder to your OBS Studio installation directory" >> "${CMAKE_SOURCE_DIR}/release/obs-scene-switcher-${CMAKE_PROJECT_VERSION}/INSTALL.txt"
        COMMAND ${CMAKE_COMMAND} -E echo "4. Restart OBS Studio" >> "${CMAKE_SOURCE_DIR}/release/obs-scene-switcher-${CMAKE_PROJECT_VERSION}/INSTALL.txt"
        COMMAND ${CMAKE_COMMAND} -E echo "" >> "${CMAKE_SOURCE_DIR}/release/obs-scene-switcher-${CMAKE_PROJECT_VERSION}/INSTALL.txt"
        COMMAND ${CMAKE_COMMAND} -E echo "For more information:" >> "${CMAKE_SOURCE_DIR}/release/obs-scene-switcher-${CMAKE_PROJECT_VERSION}/INSTALL.txt"
        COMMAND ${CMAKE_COMMAND} -E echo "https://github.com/ksmksks/obs-scene-switcher" >> "${CMAKE_SOURCE_DIR}/release/obs-scene-switcher-${CMAKE_PROJECT_VERSION}/INSTALL.txt"
        
        # Create ZIP
        COMMAND ${CMAKE_COMMAND} -E tar "cf" 
            "${CMAKE_SOURCE_DIR}/release/obs-scene-switcher-${CMAKE_PROJECT_VERSION}-manual.zip" 
            --format=zip
            "${CMAKE_SOURCE_DIR}/release/obs-scene-switcher-${CMAKE_PROJECT_VERSION}"
        
        # Cleanup temporary directory
        COMMAND ${CMAKE_COMMAND} -E rm -rf
            "${CMAKE_SOURCE_DIR}/release/obs-scene-switcher-${CMAKE_PROJECT_VERSION}"
        
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/release"
        COMMENT "Creating manual installation ZIP for ${CMAKE_PROJECT_NAME} v${CMAKE_PROJECT_VERSION}"
        VERBATIM
    )
    
    message(STATUS "Manual installation ZIP target created.")
    message(STATUS "  Build ZIP: cmake --build build_x64 --target package-zip --config Release")
endif()
