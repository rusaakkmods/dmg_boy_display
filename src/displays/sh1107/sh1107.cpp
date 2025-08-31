#include "displays/sh1107/sh1107.hpp"
#include "pico/stdlib.h"
#include "displays/sh1107/sh1107_hal.hpp"
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
    
    const int display_width = _hal.getConfig().width;
    const int display_height = _hal.getConfig().height;
    
    // Clamp coordinates to display bounds
    if (x >= display_width || y >= display_height || x + w <= 0 || y + h <= 0) {
        return; // Completely outside display
    }
    
    // Calculate actual drawing bounds
    int draw_x = (x < 0) ? 0 : x;
    int draw_y = (y < 0) ? 0 : y;
    int draw_w = ((x + w) > display_width) ? (display_width - draw_x) : (w - (draw_x - x));
    int draw_h = ((y + h) > display_height) ? (display_height - draw_y) : (h - (draw_y - y));
    
    // Calculate which pages we need to update
    int start_page = draw_y / 8;
    int end_page = (draw_y + draw_h - 1) / 8;
    
    // Static buffer to avoid memory allocations in time-critical path
    static uint8_t pageBuf[128]; // SH1107 max width
    
    // Build and send pages
    for (int page = start_page; page <= end_page; ++page) {
        // Set page address
        _hal.writeCommand(0xB0 | page);
        // Set column address to start of drawing area
        _hal.writeCommand(0x00 | (draw_x & 0x0F));        // Set lower column address
        _hal.writeCommand(0x10 | ((draw_x >> 4) & 0x0F)); // Set higher column address

        // Clear buffer
        memset(pageBuf, 0, draw_w);

        for (int col = 0; col < draw_w; ++col) {
            uint8_t byte = 0;
            for (int bit = 0; bit < 8; ++bit) {
                int screen_y = page * 8 + bit;
                int src_y = screen_y - y;
                int src_x = (draw_x + col) - x;
                
                // Skip if outside source image bounds
                if (src_x < 0 || src_x >= w || src_y < 0 || src_y >= h) continue;
                
                // pick pixel from input data
                int srcIndex = src_y * w + src_x;
                if (srcIndex >= w * h || srcIndex < 0) continue; // Additional safety check
                
                uint16_t pix = data[srcIndex];
                // Optimized RGB565 to luminance conversion
                // Extract and convert in one go with bit shifting
                uint8_t r = (pix >> 8) & 0xF8;  // Red: take top 5 bits, shift to 8-bit range
                uint8_t g = (pix >> 3) & 0xFC;  // Green: take top 6 bits, shift to 8-bit range  
                uint8_t b = (pix << 3) & 0xF8;  // Blue: take top 5 bits, shift to 8-bit range
                
                // Fast luminance using integer approximation: Y = (R + 2*G + B) / 4
                uint16_t lum = (r + (g << 1) + b) >> 2;
                if (lum > 128) byte |= (1 << bit);
            }
            pageBuf[col] = byte;
        }

        // Send page buffer
        _hal.writeDataBuffer(pageBuf, draw_w);
    }
}

// A convenience clear that fills the display with either on (color != 0) or off
void SH1107::clearScreen(uint16_t color) {
    if (!_initialized) return;
    const int width = _hal.getConfig().width;
    const int height = _hal.getConfig().height;
    const int pages = (height + 7) / 8;
    uint8_t fill = (color == 0) ? 0x00 : 0xFF;
    
    // Static buffer to avoid memory allocations
    static uint8_t pageBuf[128]; // SH1107 max width
    memset(pageBuf, fill, width);

    for (int page = 0; page < pages; ++page) {
        _hal.writeCommand(0xB0 | page);
        _hal.writeCommand(0x00 | (0 & 0x0F));        // Set lower column address (0)
        _hal.writeCommand(0x10 | ((0 >> 4) & 0x0F)); // Set higher column address (0)
        _hal.writeDataBuffer(pageBuf, width);
    }
}

void SH1107::invertDisplay(bool invert) {
    if (!_initialized) return;
    _hal.writeCommand(invert ? 0xA7 : 0xA6); // 0xA7 = inverted, 0xA6 = normal
}
