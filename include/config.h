#pragma once

#include <stdint.h>
#include "hardware/spi.h"
#include "displays/ili9341/ili9341.hpp"
#include "palettes.hpp"

// Version will be defined by CMake build system:
// -DVERSION_V1_1a for v1.1a variant
// -DVERSION_V1_0 for v1.0 variant

// Configuration options are now set via CMake build system:
// -DENABLE_DISPLAY_TEST=ON/OFF
// -DENABLE_BW_DITHER=ON/OFF  
// -DDISABLE_PALETTE_SELECTION=ON/OFF
// -DDITHER_MODE=FAST/BEST (when BW_DITHER is enabled)
// -DSPI_SPEED=40/62.5

// Hardware Pin Definitions - v1.1a
#ifdef VERSION_V1_1a
    #define SPI_CHANNEL     spi0
    #define PIN_MOSI        3
    #define PIN_SCK         6
    #define PIN_CS          5
    #define PIN_DC          4
    #define PIN_RESET       7
    #define PIN_BL          8
    #define PIN_PALETTE_ADC 29  // ADC3 - 10K potentiometer
    #define PIN_MODE_SWITCH 2   // Mode switch: LOW=brightness, HIGH=palette
    #define GB_PIN_BASE     9
#else // VERSION_V1_0
    #define SPI_CHANNEL spi1
    #define PIN_MOSI 11
    #define PIN_SCK 10
    #define PIN_CS 9
    #define PIN_DC 12
    #define PIN_RESET 13
    #define PIN_BL 8
    #define PIN_PALETTE_ADC 29  // ADC3 - 10K trimmer for palette selection
    #define GB_PIN_BASE 2
#endif

// Game Boy LCD Specifications
#define DMG_W 160
#define DMG_H 144

// Display Configuration - ILI9341
#define LCD_W           320      
#define LCD_H           240      
#define DISPLAY_SCALE   1.6
#define DISPLAY_ROTATION ili9341::ROTATION_270
#define FILL_COLOR      ili9341::BLACK
#define X_OFF 49
#define Y_OFF 7
#define SCALED_W (int)(DMG_W * DISPLAY_SCALE + 0.5f)
#define SCALED_H (int)(DMG_H * DISPLAY_SCALE + 0.5f)

// LCD SPI Configuration
#ifdef SPI_SPEED_40MHZ
    #define LCD_SPI_SPEED   (40 * 1000 * 1000)
#else
    #define LCD_SPI_SPEED   (62.5 * 1000 * 1000)  // Default to 62.5MHz
#endif
#define LCD_DMA_BUFFER  2560
#define LCD_BRIGHTNESS  128  // 50% brightness

// Palette Configuration
extern const uint16_t* const PALETTE_LIST[];
extern const size_t NUM_PALETTES;

// Default Palette Selection - use palette name directly
// Change this to any palette name defined in palettes.hpp
#define DEFAULT_PALETTE_NAME GRAYSCALE

// Dither palette setup
#ifdef ENABLE_BW_DITHER
    extern const uint16_t BW_BLACK;
    extern const uint16_t BW_WHITE;
    
    #ifdef DITHER_BEST
        extern const uint16_t gb_colors[4];
    #else
        extern const uint16_t gb_colors[4];
    #endif
#else
    extern const uint16_t* gb_colors;
#endif

// Test Pattern Configuration
#define TEST_BLINK_IO_V1_1A 2
#define TEST_BLINK_IO_V1_0  7
#define TEST_PATTERN_DELAY_MS 1000
#define NUM_TEST_PATTERNS 6

// ADC Configuration
#define ADC_OVERSAMPLE_COUNT 8
#define ADC_DELAY_US 10
#define ADC_MAX_VALUE 4096
#define BRIGHTNESS_MIN 5
#define BRIGHTNESS_MAX 128
#define BRIGHTNESS_RANGE (BRIGHTNESS_MAX - BRIGHTNESS_MIN)
#define BRIGHTNESS_THRESHOLD_V1_0 64

// Logo Display Configuration
#define LOGO_DISPLAY_DELAY_MS 100
#define LOGO_BRIGHTNESS_DELAY_MS 50
#define LOGO_TOTAL_DELAY_MS 900

// Scaling optimization
#define SCALE_UNROLL_FACTOR 8