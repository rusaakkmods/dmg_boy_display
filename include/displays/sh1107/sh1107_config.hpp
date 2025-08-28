#pragma once

#include <cstdint>
#include "hardware/spi.h"

namespace sh1107 {

enum Color {
    BLACK     = 0x0000,
    WHITE     = 0xFFFF,
};

// Rotation direction (kept for API compatibility)
enum Rotation {
    ROTATION_0   = 0,
    ROTATION_90  = 1,
    ROTATION_180 = 2,
    ROTATION_270 = 3
};

// DMA placeholder
struct DmaConfig {
    bool enabled;
    uint dma_tx_channel;
    size_t buffer_size;
    DmaConfig() : enabled(false), dma_tx_channel(0), buffer_size(0) {}
};

struct Config {
    spi_inst_t* spi_inst;
    uint32_t spi_speed_hz;

    uint8_t pin_din;   // MOSI
    uint8_t pin_sck;   // SCK
    uint8_t pin_cs;    // CS
    uint8_t pin_dc;    // D/C
    uint8_t pin_reset; // RST
    uint8_t pin_bl;    // optional (not used for OLED)

    uint16_t width;
    uint16_t height;
    Rotation rotation;

    DmaConfig dma;

    Config() :
        spi_inst(spi1),
        spi_speed_hz(8 * 1000 * 1000), // default 8MHz
        pin_din(11),
        pin_sck(10),
        pin_cs(9),
        pin_dc(12),
        pin_reset(13),
        pin_bl(8),
        width(128),
        height(128),
        rotation(ROTATION_0),
        dma() {}
};

} // namespace sh1107
