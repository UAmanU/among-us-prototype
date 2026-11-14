cmake_minimum_required(VERSION 3.20)

project(among_us_prototype LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(RAYLIB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/lib/raylib")

if(NOT EXISTS "${RAYLIB_ROOT}/raylib.h" OR NOT EXISTS "${RAYLIB_ROOT}/libraylib.a")
    message(FATAL_ERROR "Raylib files were not found in ${RAYLIB_ROOT}")
endif()

add_library(raylib STATIC IMPORTED GLOBAL)
set_target_properties(raylib PROPERTIES
    IMPORTED_LOCATION "${RAYLIB_ROOT}/libraylib.a"
    INTERFACE_INCLUDE_DIRECTORIES "${RAYLIB_ROOT}"
)

add_executable(among_us_prototype
    src/main.cpp
    src/Game/game.cpp
    src/Map/map.cpp
    src/Task/task_game.cpp
    src/Core/programm_launcher.cpp
    src/Game/game_launcher.cpp
    src/ConfigManager/config_manager.cpp
    src/Menu/menu.cpp
    src/GUI/menu_renderer.cpp
)

target_include_directories(among_us_prototype PRIVATE include "${RAYLIB_ROOT}")
target_link_libraries(among_us_prototype PRIVATE raylib)

add_custom_command(TARGET among_us_prototype POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/images"
        "$<TARGET_FILE_DIR:among_us_prototype>/images"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Config"
        "$<TARGET_FILE_DIR:among_us_prototype>/Config"
    COMMENT "Copying game images beside the executable"
)

if(MSVC)
    target_compile_options(among_us_prototype PRIVATE /W4)
else()
    target_compile_options(among_us_prototype PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(WIN32)
    target_link_libraries(among_us_prototype PRIVATE opengl32 gdi32 winmm)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(among_us_prototype PRIVATE m pthread dl rt X11)
endif()
