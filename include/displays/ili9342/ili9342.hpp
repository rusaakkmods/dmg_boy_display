#pragma once

#include "ili9342_config.hpp"
#include "ili9342_hal.hpp"
#include "ili9342_gfx.hpp"

namespace ili9342 {

// Main ILI9342 LCD control class
class ILI9342 {
private:
    HAL _hal;
    Graphics _gfx;
    bool _initialized;

    void initializeDisplay();

public:
    ILI9342();
    virtual ~ILI9342();

    bool begin(const Config& config = Config());
    bool begin(spi_inst_t* spi, uint8_t cs_pin, uint8_t dc_pin,
              uint8_t rst_pin, uint8_t bl_pin = 10,
              uint16_t width = 240, uint16_t height = 320);

    void setRotation(Rotation rotation);
    Rotation getRotation() { return _hal.getConfig().rotation; }
    void invertDisplay(bool invert);
    void fillScreen(uint16_t color);
    void sleepDisplay(bool sleep);
    void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

    void clearScreen(uint16_t color = BLACK) { _gfx.clearScreen(_hal.getConfig().width, _hal.getConfig().height, color); }

    bool isDmaEnabled() const { return _hal.isDmaEnabled(); }
    bool isDmaBusy() const { return _hal.isDmaBusy(); }

    bool drawImageDMA(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data);
    bool fillRectDMA(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

    void setBacklight(bool on);
    void setBrightness(uint8_t brightness);
    void reset();

    static uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }

    static uint16_t correctColor(uint16_t color) {
        return color;
    }

    Graphics& graphics() { return _gfx; }
    HAL& hal() { return _hal; }

    void drawPixel(int16_t x, int16_t y, uint16_t color) { _gfx.drawPixel(x, y, color); }
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) { _gfx.drawLine(x0, y0, x1, y1, color); }
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) { _gfx.drawRect(x, y, w, h, color); }
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) { _gfx.fillRect(x, y, w, h, color); }
    void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) { _gfx.drawCircle(x0, y0, r, color); }
    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) { _gfx.fillCircle(x0, y0, r, color); }
    void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) { _gfx.drawTriangle(x0, y0, x1, y1, x2, y2, color); }
    void drawChar(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, uint8_t size) { _gfx.drawChar(x, y, c, color, bg, size); }
    void drawString(int16_t x, int16_t y, const char* str, uint16_t color, uint16_t bg, uint8_t size) { _gfx.drawString(x, y, str, color, bg, size); }
    void drawImage(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data) { _gfx.drawImage(x, y, w, h, data); }

    friend class Graphics;
};

} // namespace ili9342
