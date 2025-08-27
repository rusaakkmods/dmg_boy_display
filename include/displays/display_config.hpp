#pragma once

#include <cstdint>
#include "hardware/spi.h"
#include "hardware/dma.h"

namespace display {

// Color definitions (RGB565 format) - common for both displays
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

// Rotation direction - common for both displays
enum Rotation {
    ROTATION_0   = 0,
    ROTATION_90  = 1,
    ROTATION_180 = 2,
    ROTATION_270 = 3
};

// DMA configuration - common for both displays
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

// Display type enumeration
enum DisplayType {
    DISPLAY_ST7789,
    DISPLAY_ILI9341,
    DISPLAY_ILI9342
};

// Common configuration structure for all displays
struct Config {
    DisplayType display_type; // Type of display
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
    
    // Constructor with default values for ST7789
    Config() : 
        display_type(DISPLAY_ST7789),
        spi_inst(spi1),
        spi_speed_hz(40 * 1000 * 1000),  // 40MHz
        pin_din(11),
        pin_sck(14),
        pin_cs(9),
        pin_dc(12),
        pin_reset(13),
        pin_bl(8),
        width(240),
        height(320),
        rotation(ROTATION_0),  // Default rotation is 0 degrees
        dma() {}
    
    // Constructor for ILI9341 with appropriate defaults
    static Config createILI9341Config() {
        Config config;
        config.display_type = DISPLAY_ILI9341;
        config.spi_speed_hz = 25 * 1000 * 1000;  // 25MHz for ILI9341
        config.width = 240;
        config.height = 320;
        return config;
    }

    // Constructor for ILI9342 with appropriate defaults
    static Config createILI9342Config() {
        Config config;
        config.display_type = DISPLAY_ILI9342;
        config.spi_speed_hz = 25 * 1000 * 1000;  // 25MHz for ILI9342 (adjust if needed)
        config.width = 240;
        config.height = 320;
        return config;
    }
};

} // namespace display
