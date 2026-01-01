#pragma once

#include <stdint.h>
#include "hardware/spi.h"
#include "displays/ili9341/ili9341.hpp"
#include "palettes.hpp"

#ifdef VERSION_V1_1
    #define SPI_CHANNEL     spi0
    #define PIN_MOSI        3
    #define PIN_SCK         6
    #define PIN_CS          5
    #define PIN_DC          4
    #define PIN_RESET       7
    #define PIN_BL          8
    #define PIN_PALETTE_ADC 29
    #define PIN_MODE_SWITCH 2
    #define GB_PIN_BASE     9
    #define I2C_CHANNEL     i2c1
    #define PIN_I2C_SDA     14
    #define PIN_I2C_SCL     15
    #define EEPROM_ADDR     0x50
#else
    #define SPI_CHANNEL     spi1
    #define PIN_MOSI        11
    #define PIN_SCK         10
    #define PIN_CS          9
    #define PIN_DC          12
    #define PIN_RESET       13
    #define PIN_BL          8
    #define PIN_PALETTE_ADC 29
    #define GB_PIN_BASE     2
#endif

constexpr int DMG_W = 160;
constexpr int DMG_H = 144;

constexpr int LCD_W = 320;
constexpr int LCD_H = 240;
constexpr float DISPLAY_SCALE = 1.6f;
#define DISPLAY_ROTATION    ili9341::ROTATION_270
#define FILL_COLOR          ili9341::BLACK

constexpr int16_t X_OFF_BASE = 47;
constexpr int16_t Y_OFF_BASE = 2;
constexpr int SCALED_W = static_cast<int>(DMG_W * DISPLAY_SCALE + 0.5f);
constexpr int SCALED_H = static_cast<int>(DMG_H * DISPLAY_SCALE + 0.5f);
constexpr int16_t X_OFF_DEFAULT = X_OFF_BASE;
constexpr int16_t Y_OFF_DEFAULT = Y_OFF_BASE;

extern int16_t X_OFF;
extern int16_t Y_OFF;

constexpr int16_t OFFSET_X_MIN = X_OFF_BASE - 5;
constexpr int16_t OFFSET_X_MAX = X_OFF_BASE + 5;
constexpr int16_t OFFSET_Y_MIN = Y_OFF_BASE - 5;
constexpr int16_t OFFSET_Y_MAX = Y_OFF_BASE + 5;

#ifdef SPI_SPEED_40MHZ
    constexpr uint32_t LCD_SPI_SPEED = 40 * 1000 * 1000;
#else
    constexpr uint32_t LCD_SPI_SPEED = static_cast<uint32_t>(62.5 * 1000 * 1000);
#endif
constexpr int LCD_DMA_BUFFER = 2560;
constexpr uint8_t LCD_BRIGHTNESS = 128;

extern const uint16_t* const PALETTE_LIST[];
extern const size_t NUM_PALETTES;
#define DEFAULT_PALETTE_NAME GRAYSCALE

#ifdef ENABLE_BW_DITHER
    extern const uint16_t BW_BLACK;
    extern const uint16_t BW_WHITE;
    extern const uint16_t gb_colors[4];
#else
    extern const uint16_t* gb_colors;
#endif

constexpr uint8_t TEST_BLINK_IO_V1_1 = 2;
constexpr uint8_t TEST_BLINK_IO_V1_0 = 7;
constexpr uint32_t TEST_PATTERN_DELAY_MS = 1000;
constexpr int NUM_TEST_PATTERNS = 6;

constexpr int ADC_OVERSAMPLE_COUNT = 8;
constexpr uint32_t ADC_DELAY_US = 10;
constexpr int ADC_MAX_VALUE = 4096;
constexpr uint8_t BRIGHTNESS_MIN = 5;
constexpr uint8_t BRIGHTNESS_MAX = 128;
constexpr uint8_t BRIGHTNESS_RANGE = BRIGHTNESS_MAX - BRIGHTNESS_MIN;
constexpr uint8_t BRIGHTNESS_THRESHOLD_V1_0 = 64;

constexpr uint32_t LOGO_DISPLAY_DELAY_MS = 100;
constexpr uint32_t LOGO_BRIGHTNESS_DELAY_MS = 50;
constexpr uint32_t LOGO_TOTAL_DELAY_MS = 900;

constexpr int SCALE_UNROLL_FACTOR = 8;

constexpr uint8_t SCANLINE_INTENSITY_MIN = 64;
constexpr uint8_t SCANLINE_INTENSITY_MAX = 192;
constexpr uint8_t SCANLINE_INTENSITY_DEFAULT = 128;

#ifndef GIT_HASH
    #define GIT_HASH "unknown"
#endif
#ifndef GIT_TAG
    #define GIT_TAG "dev"
#endif