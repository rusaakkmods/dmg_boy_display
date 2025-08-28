#pragma once

#include "sh1107_config.hpp"
#include <cstdint>

namespace sh1107 {

class Graphics {
public:
    Graphics();
    ~Graphics();

    // Clear and draw image functions. For compatibility with main.cpp, drawImage accepts
    // a 16-bit RGB565 buffer; this will be converted to monochrome for the OLED.
    void clearScreen(uint16_t width, uint16_t height, uint16_t color);
    void drawImage(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data);

    static uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
};

} // namespace sh1107
