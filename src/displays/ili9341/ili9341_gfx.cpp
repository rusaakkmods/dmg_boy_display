#include "displays/ili9341/ili9341_gfx.hpp"
#include "displays/ili9341/ili9341.hpp"
#include <cstdlib>
#include <cstring>

namespace ili9341 {

Graphics::Graphics(ILI9341* display) : _display(display) {
}

Graphics::~Graphics() {
}

void Graphics::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (!_display || x < 0 || y < 0 || 
        x >= _display->_hal.getConfig().width || 
        y >= _display->_hal.getConfig().height) {
        return;
    }
    
    _display->setAddrWindow(x, y, x, y);
    
    uint8_t colorBytes[2];
    colorBytes[0] = (color >> 8) & 0xFF;
    colorBytes[1] = color & 0xFF;
    _display->_hal.writeDataBulk(colorBytes, 2);
}

void Graphics::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    int16_t dx = abs(x1 - x0);
    int16_t dy = abs(y1 - y0);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;
    
    while (true) {
        drawPixel(x0, y0, color);
        
        if (x0 == x1 && y0 == y1) break;
        
        int16_t e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void Graphics::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    drawLine(x, y, x + w - 1, y, color);         // Top
    drawLine(x, y + h - 1, x + w - 1, y + h - 1, color); // Bottom
    drawLine(x, y, x, y + h - 1, color);         // Left
    drawLine(x + w - 1, y, x + w - 1, y + h - 1, color); // Right
}

void Graphics::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (!_display || w <= 0 || h <= 0) return;
    
    // Clip to screen bounds
    const auto& config = _display->_hal.getConfig();
    if (x >= config.width || y >= config.height) return;
    
    if (x + w > config.width) w = config.width - x;
    if (y + h > config.height) h = config.height - y;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    
    if (w <= 0 || h <= 0) return;
    
    _display->setAddrWindow(x, y, x + w - 1, y + h - 1);
    
    // Fill with color
    uint8_t colorBytes[2] = { (uint8_t)((color >> 8) & 0xFF), (uint8_t)(color & 0xFF) };
    
    for (int16_t i = 0; i < h; i++) {
        for (int16_t j = 0; j < w; j++) {
            _display->_hal.writeDataBulk(colorBytes, 2);
        }
    }
}

void Graphics::drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
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

void Graphics::fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    drawLine(x0, y0 - r, x0, y0 + r, color);
    
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
        
        drawLine(x0 + x, y0 - y, x0 + x, y0 + y, color);
        drawLine(x0 - x, y0 - y, x0 - x, y0 + y, color);
        drawLine(x0 + y, y0 - x, x0 + y, y0 + x, color);
        drawLine(x0 - y, y0 - x, x0 - y, y0 + x, color);
    }
}

void Graphics::drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    drawLine(x0, y0, x1, y1, color);
    drawLine(x1, y1, x2, y2, color);
    drawLine(x2, y2, x0, y0, color);
}

void Graphics::drawChar(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, uint8_t size) {
    // Simple 5x7 font implementation - this is a basic version
    // You might want to include a proper font library for better text rendering
    
    if (c < 32 || c > 126) return; // Only printable ASCII
    
    // Simple block character for now
    fillRect(x, y, 5 * size, 7 * size, bg);
    fillRect(x + size, y + size, 3 * size, 5 * size, color);
}

void Graphics::drawString(int16_t x, int16_t y, const char* str, uint16_t color, uint16_t bg, uint8_t size) {
    int16_t cursor_x = x;
    int16_t cursor_y = y;
    
    while (*str) {
        if (*str == '\n') {
            cursor_y += 8 * size;
            cursor_x = x;
        } else {
            drawChar(cursor_x, cursor_y, *str, color, bg, size);
            cursor_x += 6 * size;
        }
        str++;
    }
}

void Graphics::drawImage(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data) {
    if (!_display || !data) return;
    
    _display->setAddrWindow(x, y, x + w - 1, y + h - 1);
    
    // Send data in chunks for better performance
    const int CHUNK_SIZE = 480; // 240 pixels * 2 bytes = 480 bytes per line
    uint8_t colorBytes[CHUNK_SIZE];
    
    for (int32_t i = 0; i < w * h; i += 240) {
        int pixels_in_chunk = (i + 240 <= w * h) ? 240 : (w * h - i);
        
        // Convert chunk to bytes
        for (int j = 0; j < pixels_in_chunk; j++) {
            colorBytes[j * 2] = (data[i + j] >> 8) & 0xFF;
            colorBytes[j * 2 + 1] = data[i + j] & 0xFF;
        }
        
        // Send chunk
        _display->_hal.writeDataBulk(colorBytes, pixels_in_chunk * 2);
    }
}

void Graphics::clearScreen(uint16_t width, uint16_t height, uint16_t color) {
    fillRect(0, 0, width, height, color);
}

uint16_t Graphics::color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void Graphics::swap(int16_t& a, int16_t& b) {
    int16_t temp = a;
    a = b;
    b = temp;
}

} // namespace ili9341
