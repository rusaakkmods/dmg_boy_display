#pragma once

#include <cstdint>
#include "hardware/spi.h"
#include "hardware/dma.h"

namespace ili9341 {

// Color definitions (RGB565 format)
enum Color {
    BLACK     = 0x0000,
    WHITE     = 0xFFFF,
    RED       = 0xF800,
    GREEN     = 0x07E0,
    BLUE      = 0x001F,
    YELLOW    = 0xFFE0,
    CYAN      = 0x07FF,
    MAGENTA   = 0xF81F
};

// Rotation direction
enum Rotation {
    ROTATION_0   = 0,
    ROTATION_90  = 1,
    ROTATION_180 = 2,
    ROTATION_270 = 3
};

// DMA configuration
struct DmaConfig {
    bool enabled;
    uint dma_tx_channel;
    size_t buffer_size;
    
    DmaConfig() :
        enabled(true),
        dma_tx_channel(0),
        buffer_size(4096)
    {}
};

// Configuration structure for ILI9341
struct Config {
    spi_inst_t* spi_inst;
    uint32_t spi_speed_hz;
    
    uint8_t pin_din;
    uint8_t pin_sck;
    uint8_t pin_cs;
    uint8_t pin_dc;
    uint8_t pin_reset;
    uint8_t pin_bl;
    
    uint16_t width;
    uint16_t height;
    Rotation rotation;
    
    DmaConfig dma;
    
    Config() : 
        spi_inst(spi0),
        spi_speed_hz(25 * 1000 * 1000),  // 25MHz for ILI9341
        pin_din(19),
        pin_sck(18),
        pin_cs(17),
        pin_dc(20),
        pin_reset(15),
        pin_bl(10),
        width(240),
        height(320),
        rotation(ROTATION_0),
        dma() {}
};

} // namespace ili9341
