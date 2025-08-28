#pragma once

#include "sh1107_config.hpp"
#include "sh1107_hal.hpp"
#include "sh1107_gfx.hpp"

namespace sh1107 {

class SH1107 {
private:
    HAL _hal;
    Graphics _gfx;
    bool _initialized;

public:
    SH1107();
    virtual ~SH1107();

    bool begin(const Config& config = Config());
    void setRotation(Rotation rotation);
    void clearScreen(uint16_t color = 0x0000) { _gfx.clearScreen(_hal.getConfig().width, _hal.getConfig().height, color); }
    void drawImage(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data);
    void setBrightness(uint8_t v) { _hal.setContrast(v); }
};

} // namespace sh1107
