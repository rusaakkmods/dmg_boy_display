#pragma once

#include <cstdint>
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "ili9342_config.hpp"

namespace ili9342 {

class HAL {
private:
    Config _config;
    bool _initialized;
    int _dma_tx_channel;
    uint16_t* _dma_buffer;
    size_t _dma_buffer_size;
    bool _dma_enabled;
    bool _dma_busy;

    void initDma();
    void cleanupDma();
    bool waitForDmaComplete(uint32_t timeout_ms = 1000);

public:
    HAL();
    virtual ~HAL();

    bool init(const Config& config);

    void writeCommand(uint8_t cmd);
    void writeData(uint8_t data);
    void writeDataBulk(const uint8_t* data, size_t len);

    bool writeDataDma(const uint16_t* data, size_t len);
    bool isDmaBusy() const { return _dma_busy; }
    bool isDmaEnabled() const { return _dma_enabled; }
    void abortDma();

    void reset();
    void setBacklight(bool on);
    void setBrightness(uint8_t brightness);

    void delay(uint32_t ms);

    const Config& getConfig() const { return _config; }

    void setWidth(uint16_t width) { _config.width = width; }
    void setHeight(uint16_t height) { _config.height = height; }
    void setRotation(Rotation rotation) { _config.rotation = rotation; }

    friend void ili9342_dma_complete_handler();
};

} // namespace ili9342
