#include "include/displays/sh1107/sh1107.hpp"
#include "pico/stdlib.h"
#include "include/displays/sh1107/sh1107_hal.hpp"
#include <vector>
#include <cstring>

using namespace sh1107;

SH1107::SH1107() : _initialized(false) {}
SH1107::~SH1107() {}

bool SH1107::begin(const Config& config) {
    bool ok = _hal.init(config);
    if (!ok) return false;
    _initialized = true;
    return true;
}

void SH1107::setRotation(Rotation rotation) { _hal.setRotation(rotation); }

// Helper: send a full frame buffer (w x h RGB565) converting to monochrome pages
void SH1107::drawImage(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data) {
    if (!_initialized) return;
    // Clamp to display
    if (x < 0 || y < 0 || x + w > (int)_hal.getConfig().width || y + h > (int)_hal.getConfig().height) {
        // For simplicity, only support full-fit images
    }

    const int width = _hal.getConfig().width;
    const int height = _hal.getConfig().height;
    const int pages = (height + 7) / 8;

    // Build and send pages for the whole screen or the region
    for (int page = 0; page < pages; ++page) {
        // Set page address
        _hal.writeCommand(0xB0 | page);
        // Column address: SH1107 often uses 0x00 and 0x10 for low/high
        _hal.writeCommand(0x00); // col lower
        _hal.writeCommand(0x10); // col higher

        // Prepare buffer for this page
        std::vector<uint8_t> pageBuf(width);
        memset(pageBuf.data(), 0, pageBuf.size());

        for (int col = 0; col < width; ++col) {
            uint8_t byte = 0;
            for (int bit = 0; bit < 8; ++bit) {
                int yy = page * 8 + bit;
                if (yy >= height) continue;
                // pick pixel from input data
                int srcIndex = yy * w + col; // assumes image width == display width
                uint16_t pix = data[srcIndex];
                // convert RGB565 to luminance
                uint8_t r = (pix >> 11) & 0x1F;
                uint8_t g = (pix >> 5) & 0x3F;
                uint8_t b = pix & 0x1F;
                uint16_t rr = (r * 255) / 31;
                uint16_t gg = (g * 255) / 63;
                uint16_t bb = (b * 255) / 31;
                uint16_t lum = (uint16_t)((rr * 299 + gg * 587 + bb * 114) / 1000);
                if (lum > 128) byte |= (1 << bit);
            }
            pageBuf[col] = byte;
        }

        // Send page buffer
        _hal.writeDataBuffer(pageBuf.data(), pageBuf.size());
    }
}

// A convenience clear that fills the display with either on (color != 0) or off
void SH1107::clearScreen(uint16_t color) {
    if (!_initialized) return;
    const int width = _hal.getConfig().width;
    const int height = _hal.getConfig().height;
    const int pages = (height + 7) / 8;
    uint8_t fill = (color == 0) ? 0x00 : 0xFF;
    std::vector<uint8_t> pageBuf(width, fill);

    for (int page = 0; page < pages; ++page) {
        _hal.writeCommand(0xB0 | page);
        _hal.writeCommand(0x00);
        _hal.writeCommand(0x10);
        _hal.writeDataBuffer(pageBuf.data(), pageBuf.size());
    }
}
