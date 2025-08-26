#pragma once

#include <cstdint>
#include "hardware/spi.h"
#include "hardware/dma.h"

namespace st7796 {

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
    bool enabled;           // Whether DMA is enabled
    uint dma_tx_channel;    // DMA transmit channel
    size_t buffer_size;     // DMA buffer size
    
    // Constructor with default values
    DmaConfig() :
        enabled(true),      // Enable DMA by default
        dma_tx_channel(0),  // Use channel 0, will be automatically assigned during initialization
        buffer_size(4096)   // Default 4KB buffer
    {}
};

// Configuration structure for ST7796
struct Config {
    spi_inst_t* spi_inst;     // SPI instance
    uint32_t spi_speed_hz;    // SPI speed
    
    uint8_t pin_din;          // MOSI
    uint8_t pin_sck;          // SCK
    uint8_t pin_cs;           // Chip Select
    uint8_t pin_dc;           // Data/Command
    uint8_t pin_reset;        // Reset
    uint8_t pin_bl;           // Backlight
    
    uint16_t width;           // Width
    uint16_t height;          // Height
    Rotation rotation;        // Rotation direction
    
    // DMA configuration
    DmaConfig dma;
    
    // Constructor with default values for ST7796 320x480
    Config() : 
        spi_inst(spi1),
        spi_speed_hz(40 * 1000 * 1000),  // 40MHz - ST7796 can handle high speeds
        pin_din(11),
        pin_sck(14),
        pin_cs(9),
        pin_dc(12),
        pin_reset(13),
        pin_bl(8),
        width(320),
        height(480),
        rotation(ROTATION_0),  // Default rotation is 0 degrees
        dma() {}
};

} // namespace st7796
