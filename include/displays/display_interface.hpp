#pragma once

#include <stdint.h>
#include "hardware/spi.h"

namespace display {

enum class Rotation : uint8_t {
    ROTATION_0   = 0,
    ROTATION_90  = 1,
    ROTATION_180 = 2,
    ROTATION_270 = 3
};

struct Config {
    spi_inst_t* spi = nullptr;
    uint8_t pin_cs = 0;
    uint8_t pin_dc = 0;
    uint8_t pin_rst = 0;
    uint8_t pin_bl = 0;
    uint16_t width = 240;
    uint16_t height = 320;
    uint32_t spi_speed = 62500000;
    Rotation rotation = Rotation::ROTATION_0;
};

class IDisplay {
public:
    virtual ~IDisplay() = default;
    
    virtual bool begin(const Config& config) = 0;
    virtual void setRotation(Rotation rotation) = 0;
    virtual void clearScreen(uint16_t color = 0x0000) = 0;
    virtual void setBrightness(uint8_t brightness) = 0;
    virtual void setBacklight(bool on) = 0;
    
    virtual bool drawImageDMA(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data) = 0;
    virtual bool isDmaBusy() const = 0;
    
    virtual uint16_t getWidth() const = 0;
    virtual uint16_t getHeight() const = 0;
    
    static constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
    
    static constexpr uint16_t BLACK = 0x0000;
    static constexpr uint16_t WHITE = 0xFFFF;
};

}
