#include "displays/st7796/st7796_gfx.hpp"
#include <cstdlib>
#include <cmath>
#include <algorithm>

namespace st7796 {

Graphics::Graphics() : _hal(nullptr), _width(320), _height(480) {
}

Graphics::~Graphics() {
}

void Graphics::init(HAL* hal) {
    _hal = hal;
    if (_hal && _hal->isInitialized()) {
        const Config& config = _hal->getConfig();
        _width = config.width;
        _height = config.height;
    }
}

// Convert RGB values to 16-bit color
uint16_t Graphics::color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

bool Graphics::isValidCoord(int16_t x, int16_t y) const {
    return (x >= 0 && x < _width && y >= 0 && y < _height);
}

void Graphics::clipCoords(int16_t& x, int16_t& y, int16_t& w, int16_t& h) const {
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (x + w > _width) {
        w = _width - x;
    }
    if (y + h > _height) {
        h = _height - y;
    }
}

// Draw a single pixel
void Graphics::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (!_hal || !isValidCoord(x, y)) {
        return;
    }
    
    _hal->setAddrWindow(x, y, x, y);
    _hal->writeData16(color);
}

// Draw a line
void Graphics::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    if (!_hal) return;
    
    // Check for horizontal line
    if (y0 == y1) {
        if (x0 > x1) std::swap(x0, x1);
        drawFastHLine(x0, y0, x1 - x0 + 1, color);
        return;
    }
    
    // Check for vertical line
    if (x0 == x1) {
        if (y0 > y1) std::swap(y0, y1);
        drawFastVLine(x0, y0, y1 - y0 + 1, color);
        return;
    }
    
    // Use Bresenham's algorithm for diagonal lines
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    
    if (steep) {
        std::swap(x0, y0);
        std::swap(x1, y1);
    }
    
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    
    int16_t dx = x1 - x0;
    int16_t dy = abs(y1 - y0);
    int16_t err = dx / 2;
    int16_t ystep = (y0 < y1) ? 1 : -1;
    
    for (; x0 <= x1; x0++) {
        if (steep) {
            drawPixel(y0, x0, color);
        } else {
            drawPixel(x0, y0, color);
        }
        
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

void Graphics::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    if (!_hal || y < 0 || y >= _height || w <= 0) return;
    
    // Clip to screen bounds
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (x + w > _width) {
        w = _width - x;
    }
    if (w <= 0) return;
    
    fillRect(x, y, w, 1, color);
}

void Graphics::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    if (!_hal || x < 0 || x >= _width || h <= 0) return;
    
    // Clip to screen bounds
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (y + h > _height) {
        h = _height - y;
    }
    if (h <= 0) return;
    
    fillRect(x, y, 1, h, color);
}

// Draw rectangle outline
void Graphics::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (!_hal || w <= 0 || h <= 0) return;
    
    drawFastHLine(x, y, w, color);           // Top edge
    drawFastHLine(x, y + h - 1, w, color);   // Bottom edge
    drawFastVLine(x, y, h, color);           // Left edge
    drawFastVLine(x + w - 1, y, h, color);   // Right edge
}

// Fill rectangle
void Graphics::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (!_hal || w <= 0 || h <= 0) return;
    
    // Clip to screen bounds
    clipCoords(x, y, w, h);
    if (w <= 0 || h <= 0) return;
    
    _hal->setAddrWindow(x, y, x + w - 1, y + h - 1);
    
    uint32_t total = (uint32_t)w * h;
    
    // For large areas, try to use DMA if available
    if (_hal->isDmaEnabled() && total > 64) {
        // Create a buffer with repeated color
        const size_t buffer_pixels = std::min((size_t)total, (size_t)1024);
        uint16_t* buffer = new uint16_t[buffer_pixels];
        
        for (size_t i = 0; i < buffer_pixels; i++) {
            buffer[i] = color;
        }
        
        size_t remaining = total;
        while (remaining > 0) {
            size_t current_batch = std::min(remaining, buffer_pixels);
            _hal->writeDataDMA(buffer, current_batch);
            _hal->waitForDmaComplete();
            remaining -= current_batch;
        }
        
        delete[] buffer;
    } else {
        // Use standard method for smaller areas
        for (uint32_t i = 0; i < total; i++) {
            _hal->writeData16(color);
        }
    }
}

// Draw circle
void Graphics::drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    if (!_hal || r <= 0) return;
    
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    
    drawPixel(x0, y0 + r, color);
    drawPixel(x0, y0 - r, color);
    drawPixel(x0 + r, y0, color);
    drawPixel(x0 - r, y0, color);
    
    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        drawPixel(x0 + x, y0 + y, color);
        drawPixel(x0 - x, y0 + y, color);
        drawPixel(x0 + x, y0 - y, color);
        drawPixel(x0 - x, y0 - y, color);
        drawPixel(x0 + y, y0 + x, color);
        drawPixel(x0 - y, y0 + x, color);
        drawPixel(x0 + y, y0 - x, color);
        drawPixel(x0 - y, y0 - x, color);
    }
}

// Fill circle
void Graphics::fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    if (!_hal || r <= 0) return;
    
    drawFastVLine(x0, y0 - r, 2 * r + 1, color);
    
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    
    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        drawFastVLine(x0 + x, y0 - y, 2 * y + 1, color);
        drawFastVLine(x0 - x, y0 - y, 2 * y + 1, color);
        drawFastVLine(x0 + y, y0 - x, 2 * x + 1, color);
        drawFastVLine(x0 - y, y0 - x, 2 * x + 1, color);
    }
}

// Draw triangle
void Graphics::drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    drawLine(x0, y0, x1, y1, color);
    drawLine(x1, y1, x2, y2, color);
    drawLine(x2, y2, x0, y0, color);
}

// Simple character drawing (basic 8x8 font)
void Graphics::drawChar(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, uint8_t size) {
    if (!_hal || c < 32 || c > 126) return;
    
    // Simple 8x8 font - just draw a filled rectangle for now
    // In a real implementation, you'd have font data
    fillRect(x, y, 8 * size, 8 * size, bg);
    fillRect(x + size, y + size, 6 * size, 6 * size, color);
}

// Draw string
void Graphics::drawString(int16_t x, int16_t y, const char* str, uint16_t color, uint16_t bg, uint8_t size) {
    if (!_hal || !str) return;
    
    int16_t cursor_x = x;
    while (*str) {
        drawChar(cursor_x, y, *str, color, bg, size);
        cursor_x += 8 * size;
        str++;
    }
}

// Draw image - optimized to match ST7789/ILI9341 performance
void Graphics::drawImage(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data) {
    if (!_hal || !data || w <= 0 || h <= 0) return;
    
    // Boundary check
    if (x >= _width || y >= _height) return;
    
    // Simple clipping
    int16_t x1 = x + w - 1;
    int16_t y1 = y + h - 1;
    
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 >= _width) x1 = _width - 1;
    if (y1 >= _height) y1 = _height - 1;
    
    w = x1 - x + 1;
    h = y1 - y + 1;
    
    if (w <= 0 || h <= 0) return;
    
    // Set drawing window
    _hal->setAddrWindow(x, y, x1, y1);
    
    // Send image data using same efficient chunked approach as ST7789/ILI9341
    const int CHUNK_SIZE = 640; // 320 pixels * 2 bytes = 640 bytes per line (ST7796 optimized)
    uint8_t colorBytes[CHUNK_SIZE];
    
    for (int32_t i = 0; i < w * h; i += 320) {
        int pixels_in_chunk = (i + 320 <= w * h) ? 320 : (w * h - i);
        
        // Convert chunk to bytes with proper endianness
        for (int j = 0; j < pixels_in_chunk; j++) {
            colorBytes[j * 2] = (data[i + j] >> 8) & 0xFF;
            colorBytes[j * 2 + 1] = data[i + j] & 0xFF;
        }
        
        // Send chunk using HAL's bulk write method
        _hal->writeDataBuffer(colorBytes, pixels_in_chunk * 2);
    }
}

// Clear screen
void Graphics::clearScreen(uint16_t width, uint16_t height, uint16_t color) {
    fillRect(0, 0, width, height, color);
}

void Graphics::fillScreen(uint16_t color) {
    clearScreen(_width, _height, color);
}

} // namespace st7796
