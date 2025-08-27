#pragma once

#include <cstdint>
#include "ili9342_config.hpp"

namespace ili9342 {

class ILI9342;

class Graphics {
private:
    ILI9342* _display;

public:
    Graphics(ILI9342* display);
    virtual ~Graphics();

    void drawPixel(int16_t x, int16_t y, uint16_t color);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
    void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
    void drawChar(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, uint8_t size);
    void drawString(int16_t x, int16_t y, const char* str, uint16_t color, uint16_t bg, uint8_t size);
    void drawImage(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data);
    void clearScreen(uint16_t width, uint16_t height, uint16_t color);
    static uint16_t color565(uint8_t r, uint8_t g, uint8_t b);
    void swap(int16_t& a, int16_t& b);
};

} // namespace ili9342
