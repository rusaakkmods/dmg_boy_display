#pragma once

#include "st7796_config.hpp"
#include "st7796_hal.hpp"
#include <cstdint>

namespace st7796 {

class Graphics {
private:
    HAL* _hal;
    uint16_t _width;
    uint16_t _height;
    
    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
    
public:
    Graphics();
    ~Graphics();
    
    void init(HAL* hal);
    
    // Basic drawing functions
    void drawPixel(int16_t x, int16_t y, uint16_t color);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
    void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
    
    // Text functions
    void drawChar(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, uint8_t size);
    void drawString(int16_t x, int16_t y, const char* str, uint16_t color, uint16_t bg, uint8_t size);
    
    // Image functions
    void drawImage(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data);
    
    // Screen functions
    void clearScreen(uint16_t width, uint16_t height, uint16_t color);
    void fillScreen(uint16_t color);
    
    // Utility functions
    static uint16_t color565(uint8_t r, uint8_t g, uint8_t b);
    
    // Bounds checking
    bool isValidCoord(int16_t x, int16_t y) const;
    void clipCoords(int16_t& x, int16_t& y, int16_t& w, int16_t& h) const;
};

} // namespace st7796
