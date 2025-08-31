#include "displays/sh1107/sh1107_gfx.hpp"
#include "displays/sh1107/sh1107_hal.hpp"
#include "pico/stdlib.h"
#include <vector>
#include <cstring>

using namespace sh1107;

Graphics::Graphics() {}
Graphics::~Graphics() {}

// Convert RGB565 to monochrome using simple threshold on luminance
static inline bool rgb565_to_pixel_on(uint16_t rgb565) {
    uint8_t r = (rgb565 >> 11) & 0x1F;
    uint8_t g = (rgb565 >> 5) & 0x3F;
    uint8_t b = rgb565 & 0x1F;
    // Convert to 0-255 range approximately
    uint16_t rr = (r * 255) / 31;
    uint16_t gg = (g * 255) / 63;
    uint16_t bb = (b * 255) / 31;
    uint16_t lum = (uint16_t)((rr * 299 + gg * 587 + bb * 114) / 1000);
    return lum > 128;
}

void Graphics::clearScreen(uint16_t width, uint16_t height, uint16_t color) {
    // Build a full buffer: SH1107 uses 128x128, organized in 8-pixel pages (16 pages)
    const int pages = (height + 7) / 8;
    std::vector<uint8_t> pageBuf(width);

    uint8_t fill = (color == 0) ? 0x00 : 0xFF;
    for (int p = 0; p < pages; ++p) {
        // set page address and column address commands - using common SH1107 sequence
        uint8_t cmd_page = 0xB0 | p; // page start
        // column address lower and higher
        // for simplicity set column start to 0

        // send commands directly via HAL functions — find a HAL instance? We don't have one here.
        // Instead, clear by writing to SPI directly would require HAL. Leave a no-op; main will call clearScreen on driver which may call HAL.
        (void)pageBuf;
        (void)cmd_page;
    }
    (void)width; (void)height; (void)fill;
}

void Graphics::drawImage(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data) {
    // Simple clipping and conversion: build a page buffer and use a local SPI send via HAL by creating an ephemeral HAL
    Config cfg; // default config pins won't be accurate; in practice Graphics should use HAL instance. For now perform no-op to keep compilation and allow higher-level code to call HAL directly.
    (void)x; (void)y; (void)w; (void)h; (void)data; (void)cfg;
    // NOTE: This is a minimal placeholder. Proper implementation would require access to the HAL instance to send page buffers.
}
