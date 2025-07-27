cmake_minimum_required(VERSION 3.13)

# initialize pico-sdk from submodule
# note: this must happen before project()
include(../pico-sdk/pico_sdk_init.cmake)

project(burroughs)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()


add_executable(main main.c)

target_link_libraries(main pico_stdlib)

pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)

pico_add_extra_outputs(main)
