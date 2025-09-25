#include "displays/st7796/st7796.hpp"
#include "pico/stdlib.h"
#include <cstdio>

namespace st7796 {

ST7796::ST7796() : _initialized(false) {
}

ST7796::~ST7796() {
}

bool ST7796::begin(const Config& config) {
    // Initialize HAL
    if (!_hal.init(config)) {
        printf("ST7796: HAL initialization failed\n");
        return false;
    }
    
    // Initialize graphics with HAL
    _gfx.init(&_hal);
    
    // Initialize display
    initializeDisplay();
    
    // Turn on backlight
    setBacklight(true);
    
    _initialized = true;
    printf("ST7796: Display initialized (%dx%d)\n", config.width, config.height);
    return true;
}

bool ST7796::begin(spi_inst_t* spi, uint8_t cs_pin, uint8_t dc_pin, 
                  uint8_t rst_pin, uint8_t bl_pin, 
                  uint16_t width, uint16_t height) {
    Config config;
    config.spi_inst = spi;
    config.pin_cs = cs_pin;
    config.pin_dc = dc_pin;
    config.pin_reset = rst_pin;
    config.pin_bl = bl_pin;
    config.width = width;
    config.height = height;
    
    return begin(config);
}

void ST7796::initializeDisplay() {
    // ST7796 initialization sequence
    _hal.reset();
    sleep_ms(120);
    
    // Sleep out
    _hal.writeCommand(ST7796_SLPOUT);
    sleep_ms(120);
    
    // Command Set Control
    _hal.writeCommand(ST7796_CSCON);
    _hal.writeData(0xC3);  // Enable extension command 2 part I
    
    // Command Set Control
    _hal.writeCommand(ST7796_CSCON);
    _hal.writeData(0x96);  // Enable extension command 2 part II
    
    // Memory Access Control
    _hal.writeCommand(ST7796_MADCTL);
    _hal.writeData(0x40);  // RGB color filter, Normal mode - removed BGR flag
    
    // Pixel Format Set
    _hal.writeCommand(ST7796_COLMOD);
    _hal.writeData(0x55);  // 16-bit RGB565
    
    // Display Function Control
    _hal.writeCommand(ST7796_DFUNCTR);
    _hal.writeData(0x80);  // Bypass
    _hal.writeData(0x02);  // Source Output Scan from S1 to S960, Gate Output scan from G1 to G480, scan cycle=2
    _hal.writeData(0x3B);  // LCD Drive Line=8*(59+1)
    
    // Display Function Control
    _hal.writeCommand(ST7796_DFUNCTR);
    _hal.writeData(0x80);  // Bypass
    _hal.writeData(0x02);  // Source Output Scan from S1 to S960, Gate Output scan from G1 to G480, scan cycle=2
    _hal.writeData(0x3B);  // LCD Drive Line=8*(59+1)
    
    // Power Control 1
    _hal.writeCommand(ST7796_PWCTR1);
    _hal.writeData(0x80);  // VRH=4.8V
    
    // Power Control 2
    _hal.writeCommand(ST7796_PWCTR2);
    _hal.writeData(0x13);  // VDV=1.2V
    
    // Power Control 3
    _hal.writeCommand(ST7796_PWCTR3);
    _hal.writeData(0xA7);  // VDS=-1V
    
    // VCOM Control
    _hal.writeCommand(ST7796_VMCTR1);
    _hal.writeData(0x09);  // VCOMH=3.2V
    _hal.writeData(0x09);  // VCOML=-0.9V
    
    // VCOM Offset Control
    _hal.writeCommand(ST7796_VMOFCTR);
    _hal.writeData(0x22);  // VCOM offset
    
    // Positive Gamma Control
    _hal.writeCommand(ST7796_GMCTRP1);
    _hal.writeData(0xF0);
    _hal.writeData(0x09);
    _hal.writeData(0x0B);
    _hal.writeData(0x06);
    _hal.writeData(0x04);
    _hal.writeData(0x15);
    _hal.writeData(0x2F);
    _hal.writeData(0x54);
    _hal.writeData(0x42);
    _hal.writeData(0x3C);
    _hal.writeData(0x17);
    _hal.writeData(0x14);
    _hal.writeData(0x18);
    _hal.writeData(0x1B);
    
    // Negative Gamma Control
    _hal.writeCommand(ST7796_GMCTRN1);
    _hal.writeData(0xE0);
    _hal.writeData(0x09);
    _hal.writeData(0x0B);
    _hal.writeData(0x06);
    _hal.writeData(0x04);
    _hal.writeData(0x03);
    _hal.writeData(0x2B);
    _hal.writeData(0x43);
    _hal.writeData(0x42);
    _hal.writeData(0x3B);
    _hal.writeData(0x16);
    _hal.writeData(0x14);
    _hal.writeData(0x17);
    _hal.writeData(0x1B);
    
    // Command Set Control
    _hal.writeCommand(ST7796_CSCON);
    _hal.writeData(0x3C);  // Disable extension command 2 part I
    
    // Command Set Control  
    _hal.writeCommand(ST7796_CSCON);
    _hal.writeData(0x69);  // Disable extension command 2 part II
    
    // Inversion off
    _hal.writeCommand(ST7796_INVOFF);
    
    // Normal display mode on
    _hal.writeCommand(ST7796_NORON);
    sleep_ms(20);
    
    // Display on
    _hal.writeCommand(ST7796_DISPON);
    sleep_ms(120);
}

void ST7796::setRotation(Rotation rotation) {
    _hal.writeCommand(ST7796_MADCTL);
    
    switch (rotation) {
        case ROTATION_0:
            _hal.writeData(MADCTL_MX);  // Removed MADCTL_BGR to fix red/blue swap
            break;
        case ROTATION_90:
            _hal.writeData(MADCTL_MV);  // Removed MADCTL_BGR to fix red/blue swap
            break;
        case ROTATION_180:
            _hal.writeData(MADCTL_MY);  // Removed MADCTL_BGR to fix red/blue swap
            break;
        case ROTATION_270:
            _hal.writeData(MADCTL_MX | MADCTL_MY | MADCTL_MV);  // Removed MADCTL_BGR to fix red/blue swap
            break;
    }
}

void ST7796::invertDisplay(bool invert) {
    _hal.writeCommand(invert ? ST7796_INVON : ST7796_INVOFF);
}

void ST7796::fillScreen(uint16_t color) {
    _gfx.fillScreen(color);
}

void ST7796::sleepDisplay(bool sleep) {
    _hal.writeCommand(sleep ? ST7796_SLPIN : ST7796_SLPOUT);
    sleep_ms(120);
}

void ST7796::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    _hal.setAddrWindow(x0, y0, x1, y1);
}

bool ST7796::drawImageDMA(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data) {
    if (!_hal.isDmaEnabled() || !data) {
        return false;
    }
    
    setAddrWindow(x, y, x + w - 1, y + h - 1);
    return _hal.writeDataDMA(data, w * h);
}

bool ST7796::fillRectDMA(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (!_hal.isDmaEnabled()) {
        return false;
    }
    
    // Create buffer with repeated color
    uint32_t total = w * h;
    uint16_t* buffer = new uint16_t[total];
    for (uint32_t i = 0; i < total; i++) {
        buffer[i] = color;
    }
    
    setAddrWindow(x, y, x + w - 1, y + h - 1);
    bool result = _hal.writeDataDMA(buffer, total);
    
    delete[] buffer;
    return result;
}

void ST7796::setBacklight(bool on) {
    _hal.setBacklight(on);
}

void ST7796::setBrightness(uint8_t brightness) {
    _hal.setBrightness(brightness);
}

void ST7796::reset() {
    _hal.reset();
    if (_initialized) {
        initializeDisplay();
    }
}

} // namespace st7796
