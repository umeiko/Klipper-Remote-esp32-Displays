cmake_minimum_required(VERSION 3.16)
project(klipper_remote_desktop C)

# desktop 后端（SDL2，Windows/Linux 同构）：与 ESP32 共享 src/ui 全部界面代码
set(CMAKE_C_STANDARD 99)

set(SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(REPO_ROOT ${SRC_ROOT}/..)

# LVGL v9；lv_conf.h 通过 LV_CONF_INCLUDE_SIMPLE 从本目录解析
add_subdirectory(${REPO_ROOT}/third_party/lvgl lvgl)
target_compile_definitions(lvgl PUBLIC LV_CONF_INCLUDE_SIMPLE)
target_include_directories(lvgl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_package(SDL2 REQUIRED)

file(GLOB UI_SRCS
    ${SRC_ROOT}/ui/*.c
    ${SRC_ROOT}/ui/panels/*.c
    ${SRC_ROOT}/ui/widgets/*.c
    ${SRC_ROOT}/ui/assets/*.c
)

add_executable(klipper_remote_desktop
    main.c
    ${SRC_ROOT}/bsp/desktop/bsp_sdl2.c
    ${SRC_ROOT}/bsp/desktop/bsp_wifi_windows.c
    ${SRC_ROOT}/bsp/desktop/bsp_wifi_linux.c
    ${SRC_ROOT}/bsp/desktop/bsp_conf_file.c
    ${SRC_ROOT}/core/app_settings.c
    ${SRC_ROOT}/core/printer_mock.c
    ${SRC_ROOT}/core/moonraker_client_stub.c
    ${UI_SRCS}
)

target_include_directories(klipper_remote_desktop PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SRC_ROOT}/bsp
    ${SRC_ROOT}/core
    ${SRC_ROOT}/ui
    ${SRC_ROOT}/ui/panels
    ${SRC_ROOT}/ui/widgets
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(klipper_remote_desktop PRIVATE lvgl ${SDL2_LIBRARIES})

if(WIN32)
    target_link_libraries(klipper_remote_desktop PRIVATE mingw32 SDL2::SDL2main)
    set_target_properties(klipper_remote_desktop PROPERTIES WIN32_EXECUTABLE FALSE)
endif()
