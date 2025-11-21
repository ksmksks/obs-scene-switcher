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
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs ixwebsocket)

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
    src/eventsub/eventsub_client.h
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
        src/eventsub/eventsub_client.h
        src/eventsub/twitch_event_types.h

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
)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

# Add data directory (locale files, etc.) ---
install(
    DIRECTORY data/
    DESTINATION ${OBS_DATA_DEST}/${_name}
)
