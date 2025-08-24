#pragma once

#include <cstdint>
#include "ili9341_config.hpp"

namespace ili9341 {

// Forward declaration
class ILI9341;

// Graphics class for ILI9341 - handles all drawing operations
class Graphics {
private:
    ILI9341* _display;  // Pointer to parent display object
    
public:
    Graphics(ILI9341* display);
    virtual ~Graphics();
    
    // Basic drawing functions
    void drawPixel(int16_t x, int16_t y, uint16_t color);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
    void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
    
    // Text drawing functions
    void drawChar(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, uint8_t size);
    void drawString(int16_t x, int16_t y, const char* str, uint16_t color, uint16_t bg, uint8_t size);
    
    // Image drawing functions
    void drawImage(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data);
    
    // Screen clearing
    void clearScreen(uint16_t width, uint16_t height, uint16_t color);
    
    // Color conversion utility
    static uint16_t color565(uint8_t r, uint8_t g, uint8_t b);
    
    // Helper functions
    void swap(int16_t& a, int16_t& b);
};

} // namespace ili9341
