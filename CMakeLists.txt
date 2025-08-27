# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(dmg_boy_display C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(dmg_boy_display
    main.cpp
    src/displays/st7789/st7789.cpp
    src/displays/st7789/st7789_hal.cpp
    src/displays/st7789/st7789_gfx.cpp
    src/displays/st7789/st7789_font.cpp
    src/displays/ili9341/ili9341.cpp
    src/displays/ili9341/ili9341_hal.cpp
    src/displays/ili9341/ili9341_gfx.cpp
    src/displays/st7796/st7796.cpp
    src/displays/st7796/st7796_hal.cpp
    src/displays/st7796/st7796_gfx.cpp
    src/dither.cpp
)

# Generate PIO header
pico_generate_pio_header(dmg_boy_display ${CMAKE_CURRENT_LIST_DIR}/pio/gblcd/gblcd.pio)

pico_set_program_name(dmg_boy_display "dmg_boy_display")
pico_set_program_version(dmg_boy_display "1.0.0")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(dmg_boy_display 0)
pico_enable_stdio_usb(dmg_boy_display 0)

# Include directories
target_include_directories(dmg_boy_display PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

# Link libraries
target_link_libraries(dmg_boy_display
    pico_stdlib
    hardware_spi
    hardware_gpio
    hardware_dma
    hardware_pio
    hardware_pwm
    hardware_i2c
    hardware_adc
)

pico_add_extra_outputs(dmg_boy_display)

