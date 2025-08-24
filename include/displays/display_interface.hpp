#pragma once

#include <cstdint>

namespace display {

// Forward declarations to avoid including hardware headers
struct Config;
enum Rotation;
enum Color;

// Common display interface that all displays must implement
class DisplayInterface {
public:
    virtual ~DisplayInterface() = default;
    
    // Display initialization
    virtual bool begin(const Config& config) = 0;
    virtual bool begin(void* spi, uint8_t cs_pin, uint8_t dc_pin, 
                      uint8_t rst_pin, uint8_t bl_pin = 10,
                      uint16_t width = 240, uint16_t height = 320) = 0;
    
    // Display control
    virtual void setRotation(int rotation) = 0;
    virtual int getRotation() = 0;
    virtual void invertDisplay(bool invert) = 0;
    virtual void fillScreen(uint16_t color) = 0;
    virtual void sleepDisplay(bool sleep) = 0;
    
    // Screen clearing
    virtual void clearScreen(uint16_t color = 0x0000) = 0;
    
    // DMA related functions
    virtual bool isDmaEnabled() const = 0;
    virtual bool isDmaBusy() const = 0;
    
    // Efficient drawing functions using DMA
    virtual bool drawImageDMA(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data) = 0;
    virtual bool fillRectDMA(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) = 0;
    
    // Hardware control
    virtual void setBacklight(bool on) = 0;
    virtual void setBrightness(uint8_t brightness) = 0;
    virtual void reset() = 0;
    
    // Drawing functions
    virtual void drawPixel(int16_t x, int16_t y, uint16_t color) = 0;
    virtual void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) = 0;
    virtual void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) = 0;
    virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) = 0;
    virtual void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) = 0;
    virtual void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) = 0;
    virtual void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) = 0;
    virtual void drawChar(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, uint8_t size) = 0;
    virtual void drawString(int16_t x, int16_t y, const char* str, uint16_t color, uint16_t bg, uint8_t size) = 0;
    virtual void drawImage(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data) = 0;
    
    // Static helper functions
    static uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
};

} // namespace display
