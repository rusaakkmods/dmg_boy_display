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

# Build configuration options
option(ENABLE_DISPLAY_TEST "Enable display test mode instead of Game Boy capture" OFF)
option(ENABLE_BW_DITHER "Enable black & white dithering" OFF)
option(DISABLE_PALETTE_SELECTION "Disable palette selection (saves GPIO 29)" OFF)

# Dither mode selection (only used if ENABLE_BW_DITHER is ON)
set(DITHER_MODE "FAST" CACHE STRING "Dithering algorithm: FAST or BEST")
set_property(CACHE DITHER_MODE PROPERTY STRINGS "FAST" "BEST")

# SPI speed selection
set(SPI_SPEED "62.5" CACHE STRING "SPI clock speed in MHz: 40 or 62.5")
set_property(CACHE SPI_SPEED PROPERTY STRINGS "40" "62.5")

# Display offset adjustments (relative to base values: X_BASE=47, Y_BASE=2)
set(OFFSET_X_ADJUST "0" CACHE STRING "X offset adjustment from base (47): -47 to +273")
set(OFFSET_Y_ADJUST "0" CACHE STRING "Y offset adjustment from base (2): -2 to +238")

# Common source files for both variants
set(COMMON_SOURCES
    main.cpp
    src/displays/st7789/st7789.cpp
    src/displays/st7789/st7789_hal.cpp
    src/displays/st7789/st7789_gfx.cpp
    src/displays/st7789/st7789_font.cpp
    src/displays/ili9341/ili9341.cpp
    src/displays/ili9341/ili9341_hal.cpp
    src/displays/ili9341/ili9341_gfx.cpp
    src/displays/ili9342/ili9342.cpp
    src/displays/ili9342/ili9342_hal.cpp
    src/displays/ili9342/ili9342_gfx.cpp
    src/displays/st7796/st7796.cpp
    src/displays/st7796/st7796_hal.cpp
    src/displays/st7796/st7796_gfx.cpp
    src/displays/sh1107/sh1107.cpp
    src/displays/sh1107/sh1107_hal.cpp
    src/displays/sh1107/sh1107_gfx.cpp
    src/dither.cpp
    src/scaler.cpp
    src/helpers.cpp
    src/palettes.cpp
    src/eeprom.cpp
)

# Function to apply common build options to a target
function(apply_build_options target_name)
    # Apply optional feature flags
    if(ENABLE_DISPLAY_TEST)
        target_compile_definitions(${target_name} PRIVATE ENABLE_DISPLAY_TEST)
    endif()
    
    if(ENABLE_BW_DITHER)
        target_compile_definitions(${target_name} PRIVATE ENABLE_BW_DITHER)
        if(DITHER_MODE STREQUAL "BEST")
            target_compile_definitions(${target_name} PRIVATE DITHER_BEST)
        else()
            target_compile_definitions(${target_name} PRIVATE DITHER_FAST)
        endif()
    endif()
    
    if(DISABLE_PALETTE_SELECTION)
        target_compile_definitions(${target_name} PRIVATE DISABLE_PALETTE_SELECTION)
    endif()
    
    # SPI speed configuration
    if(SPI_SPEED STREQUAL "40")
        target_compile_definitions(${target_name} PRIVATE SPI_SPEED_40MHZ)
    else()
        target_compile_definitions(${target_name} PRIVATE SPI_SPEED_62_5MHZ)
    endif()
    
    # Display offset configuration
    target_compile_definitions(${target_name} PRIVATE OFFSET_X_ADJUST=${OFFSET_X_ADJUST})
    target_compile_definitions(${target_name} PRIVATE OFFSET_Y_ADJUST=${OFFSET_Y_ADJUST})
endfunction()

# Build v1.1 variant
add_executable(dmg_boy_display_v1_1 ${COMMON_SOURCES})
target_compile_definitions(dmg_boy_display_v1_1 PRIVATE VERSION_V1_1)
apply_build_options(dmg_boy_display_v1_1)

# Build v1.0 variant  
add_executable(dmg_boy_display_v1_0 ${COMMON_SOURCES})
target_compile_definitions(dmg_boy_display_v1_0 PRIVATE VERSION_V1_0)
apply_build_options(dmg_boy_display_v1_0)

# Configure v1.1 variant
pico_generate_pio_header(dmg_boy_display_v1_1 ${CMAKE_CURRENT_LIST_DIR}/pio/gblcd/gblcd.pio)
pico_set_program_name(dmg_boy_display_v1_1 "dmg_boy_display_v1.1")
pico_set_program_version(dmg_boy_display_v1_1 "1.1.0")

# Enable USB stdio in test mode for debugging
if(ENABLE_DISPLAY_TEST)
    pico_enable_stdio_uart(dmg_boy_display_v1_1 0)
    pico_enable_stdio_usb(dmg_boy_display_v1_1 1)
else()
    pico_enable_stdio_uart(dmg_boy_display_v1_1 0)
    pico_enable_stdio_usb(dmg_boy_display_v1_1 0)
endif()

target_include_directories(dmg_boy_display_v1_1 PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(dmg_boy_display_v1_1
    pico_stdlib hardware_spi hardware_gpio hardware_dma hardware_pio 
    hardware_pwm hardware_i2c hardware_adc hardware_watchdog
)
pico_add_extra_outputs(dmg_boy_display_v1_1)

# Configure v1.0 variant
pico_generate_pio_header(dmg_boy_display_v1_0 ${CMAKE_CURRENT_LIST_DIR}/pio/gblcd/gblcd.pio)
pico_set_program_name(dmg_boy_display_v1_0 "dmg_boy_display_v1.0")
pico_set_program_version(dmg_boy_display_v1_0 "1.0.0")
pico_enable_stdio_uart(dmg_boy_display_v1_0 0)
pico_enable_stdio_usb(dmg_boy_display_v1_0 0)
target_include_directories(dmg_boy_display_v1_0 PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(dmg_boy_display_v1_0
    pico_stdlib hardware_spi hardware_gpio hardware_dma hardware_pio 
    hardware_pwm hardware_i2c hardware_adc hardware_watchdog
)
pico_add_extra_outputs(dmg_boy_display_v1_0)

