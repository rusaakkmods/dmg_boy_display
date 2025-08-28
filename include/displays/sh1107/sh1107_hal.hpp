#pragma once

#include "sh1107_config.hpp"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include <cstdint>

namespace sh1107 {

class HAL {
private:
    Config _config;
    bool _initialized;

    void spi_init_hw();
    void send_command(uint8_t cmd);
    void send_data(const uint8_t* data, size_t len);

public:
    HAL();
    ~HAL();

    bool init(const Config& config);
    void reset();
    void setContrast(uint8_t v);
    void setRotation(Rotation r);

    // Basic writes (8-bit data)
    void writeCommand(uint8_t cmd);
    void writeData(uint8_t data);
    void writeDataBuffer(const uint8_t* buffer, size_t length);

    void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

    const Config& getConfig() const { return _config; }
    bool isInitialized() const { return _initialized; }
};

} // namespace sh1107
