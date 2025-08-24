#pragma once

#include "st7796_config.hpp"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include <cstdint>

namespace st7796 {

// ST7796 command definitions
enum Command {
    ST7796_NOP         = 0x00,
    ST7796_SWRESET     = 0x01,
    ST7796_RDDID       = 0x04,
    ST7796_RDDST       = 0x09,
    ST7796_SLPIN       = 0x10,
    ST7796_SLPOUT      = 0x11,
    ST7796_PTLON       = 0x12,
    ST7796_NORON       = 0x13,
    ST7796_INVOFF      = 0x20,
    ST7796_INVON       = 0x21,
    ST7796_DISPOFF     = 0x28,
    ST7796_DISPON      = 0x29,
    ST7796_CASET       = 0x2A,
    ST7796_RASET       = 0x2B,
    ST7796_RAMWR       = 0x2C,
    ST7796_RAMRD       = 0x2E,
    ST7796_PTLAR       = 0x30,
    ST7796_MADCTL      = 0x36,
    ST7796_COLMOD      = 0x3A,
    ST7796_DFUNCTR     = 0xB6,
    ST7796_PWCTR1      = 0xC0,
    ST7796_PWCTR2      = 0xC1,
    ST7796_PWCTR3      = 0xC2,
    ST7796_VMCTR1      = 0xC5,
    ST7796_VMOFCTR     = 0xC6,
    ST7796_GMCTRP1     = 0xE0,
    ST7796_GMCTRN1     = 0xE1,
    ST7796_CSCON       = 0xF0
};

// Memory Access Control (MADCTL) bits
enum MADCTLBits {
    MADCTL_MY  = 0x80,  // Page Address Order
    MADCTL_MX  = 0x40,  // Column Address Order
    MADCTL_MV  = 0x20,  // Page/Column Order
    MADCTL_ML  = 0x10,  // Line Address Order
    MADCTL_BGR = 0x08,  // RGB/BGR Order
    MADCTL_MH  = 0x04   // Display Data Latch Order
};

class HAL {
private:
    Config _config;
    bool _initialized;
    
    // DMA related
    int _dma_tx_channel;
    uint16_t* _dma_buffer;
    size_t _dma_buffer_size;
    bool _dma_enabled;
    volatile bool _dma_busy;
    
    void initDma();
    void cleanupDma();
    
public:
    HAL();
    ~HAL();
    
    bool init(const Config& config);
    
    // Basic communication
    void writeCommand(uint8_t cmd);
    void writeData(uint8_t data);
    void writeData16(uint16_t data);
    void writeDataBuffer(const uint8_t* buffer, size_t length);
    void writeDataBuffer16(const uint16_t* buffer, size_t length);
    
    // Display control
    void reset();
    void setBacklight(bool on);
    void setBrightness(uint8_t brightness);
    void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
    
    // DMA functions
    bool isDmaEnabled() const { return _dma_enabled; }
    bool isDmaBusy() const { return _dma_busy; }
    bool writeDataDMA(const uint16_t* data, size_t length);
    void waitForDmaComplete();
    bool waitForDmaComplete(uint32_t timeout_ms);
    void abortDma();
    
    // Configuration access
    const Config& getConfig() const { return _config; }
    bool isInitialized() const { return _initialized; }
    
    // Friend declarations for interrupt handler
    friend void dma_complete_handler();
};

} // namespace st7796
