cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(pico_joybus C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

pico_sdk_init()

add_executable(pico_joybus
    src/main.cpp
    src/joybus.cpp
    src/touchpad.cpp
)

target_include_directories(pico_joybus PRIVATE include)

pico_generate_pio_header(pico_joybus ${CMAKE_CURRENT_LIST_DIR}/pio/joybus.pio)

target_link_libraries(pico_joybus
    pico_stdlib
    pico_platform
    hardware_pio
    hardware_i2c
    pico_multicore
    pico_stdio_usb
)

target_compile_definitions(pico_joybus PRIVATE
    PICO_STDIO_USB=1
    PICO_STDIO_UART=0
)

pico_add_extra_outputs(pico_joybus)
