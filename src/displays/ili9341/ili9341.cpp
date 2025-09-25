#include "displays/ili9341/ili9341.hpp"
#include <cstdio>

namespace ili9341 {

// ILI9341 command definitions
enum ILI9341_CMD {
    ILI9341_NOP         = 0x00,
    ILI9341_SWRESET     = 0x01,
    ILI9341_RDDID       = 0x04,
    ILI9341_RDDST       = 0x09,
    ILI9341_SLPIN       = 0x10,
    ILI9341_SLPOUT      = 0x11,
    ILI9341_PTLON       = 0x12,
    ILI9341_NORON       = 0x13,
    ILI9341_RDMODE      = 0x0A,
    ILI9341_RDMADCTL    = 0x0B,
    ILI9341_RDPIXFMT    = 0x0C,
    ILI9341_RDIMGFMT    = 0x0D,
    ILI9341_RDSELFDIAG  = 0x0F,
    ILI9341_INVOFF      = 0x20,
    ILI9341_INVON       = 0x21,
    ILI9341_GAMMASET    = 0x26,
    ILI9341_DISPOFF     = 0x28,
    ILI9341_DISPON      = 0x29,
    ILI9341_CASET       = 0x2A,
    ILI9341_PASET       = 0x2B,
    ILI9341_RAMWR       = 0x2C,
    ILI9341_RAMRD       = 0x2E,
    ILI9341_PTLAR       = 0x30,
    ILI9341_MADCTL      = 0x36,
    ILI9341_VSCRSADD    = 0x37,
    ILI9341_PIXFMT      = 0x3A,
    ILI9341_FRMCTR1     = 0xB1,
    ILI9341_FRMCTR2     = 0xB2,
    ILI9341_FRMCTR3     = 0xB3,
    ILI9341_INVCTR      = 0xB4,
    ILI9341_DFUNCTR     = 0xB6,
    ILI9341_PWCTR1      = 0xC0,
    ILI9341_PWCTR2      = 0xC1,
    ILI9341_PWCTR3      = 0xC2,
    ILI9341_PWCTR4      = 0xC3,
    ILI9341_PWCTR5      = 0xC4,
    ILI9341_VMCTR1      = 0xC5,
    ILI9341_VMCTR2      = 0xC7,
    ILI9341_GMCTRP1     = 0xE0,
    ILI9341_GMCTRN1     = 0xE1,
    ILI9341_PWCTR6      = 0xFC
};

// MADCTL parameter bit definitions
#define MADCTL_MY  0x80  // Bottom to top
#define MADCTL_MX  0x40  // Right to left
#define MADCTL_MV  0x20  // Reverse Mode
#define MADCTL_ML  0x10  // LCD refresh Bottom to top
#define MADCTL_RGB 0x00  // Red-Green-Blue pixel order
#define MADCTL_BGR 0x08  // Blue-Green-Red pixel order
#define MADCTL_MH  0x04  // LCD refresh right to left

ILI9341::ILI9341() : _gfx(this), _initialized(false) {
}

ILI9341::~ILI9341() {
}

bool ILI9341::begin(const Config& config) {
    if (_initialized) {
        return true;
    }
    
    // Convert to native config format
    Config native_config = config;
    
    // Initialize hardware abstraction layer
    if (!_hal.init(native_config)) {
        printf("Failed to initialize ILI9341 hardware abstraction layer\n");
        return false;
    }
    
    // Initialize display
    initializeDisplay();
    
    _initialized = true;
    return true;
}

bool ILI9341::begin(spi_inst_t* spi, uint8_t cs_pin, uint8_t dc_pin, 
                   uint8_t rst_pin, uint8_t bl_pin,
                   uint16_t width, uint16_t height) {
    Config config;
    config.spi_inst = spi;
    config.pin_cs = cs_pin;
    config.pin_dc = dc_pin;
    config.pin_reset = rst_pin;
    config.pin_bl = bl_pin;
    config.width = width;
    config.height = height;
    
    return begin(config);
}

void ILI9341::initializeDisplay() {
    // Hardware reset
    _hal.reset();
    
    // Software reset
    _hal.writeCommand(ILI9341_SWRESET);
    _hal.delay(150);
    
    // Exit sleep mode
    _hal.writeCommand(ILI9341_SLPOUT);
    _hal.delay(120);
    
    // Power control A
    _hal.writeCommand(0xCB);
    _hal.writeData(0x39);
    _hal.writeData(0x2C);
    _hal.writeData(0x00);
    _hal.writeData(0x34);
    _hal.writeData(0x02);
    
    // Power control B
    _hal.writeCommand(0xCF);
    _hal.writeData(0x00);
    _hal.writeData(0xC1);
    _hal.writeData(0x30);
    
    // Driver timing control A
    _hal.writeCommand(0xE8);
    _hal.writeData(0x85);
    _hal.writeData(0x00);
    _hal.writeData(0x78);
    
    // Driver timing control B
    _hal.writeCommand(0xEA);
    _hal.writeData(0x00);
    _hal.writeData(0x00);
    
    // Power on sequence control
    _hal.writeCommand(0xED);
    _hal.writeData(0x64);
    _hal.writeData(0x03);
    _hal.writeData(0x12);
    _hal.writeData(0x81);
    
    // Pump ratio control
    _hal.writeCommand(0xF7);
    _hal.writeData(0x20);
    
    // Power control 1
    _hal.writeCommand(ILI9341_PWCTR1);
    _hal.writeData(0x23);
    
    // Power control 2
    _hal.writeCommand(ILI9341_PWCTR2);
    _hal.writeData(0x10);
    
    // VCOM control 1
    _hal.writeCommand(ILI9341_VMCTR1);
    _hal.writeData(0x3e);
    _hal.writeData(0x28);
    
    // VCOM control 2
    _hal.writeCommand(ILI9341_VMCTR2);
    _hal.writeData(0x86);
    
    // Memory access control (BGR format for color correction)
    _hal.writeCommand(ILI9341_MADCTL);
    _hal.writeData(0x48);  // MX=1, BGR order to fix color channel swap
    
    // Pixel format
    _hal.writeCommand(ILI9341_PIXFMT);
    _hal.writeData(0x55);  // 16 bits/pixel
    
    // Frame rate control
    _hal.writeCommand(ILI9341_FRMCTR1);
    _hal.writeData(0x00);
    _hal.writeData(0x18);
    
    // Display function control
    _hal.writeCommand(ILI9341_DFUNCTR);
    _hal.writeData(0x08);
    _hal.writeData(0x82);
    _hal.writeData(0x27);
    
    // Gamma function disable
    _hal.writeCommand(0xF2);
    _hal.writeData(0x00);
    
    // Gamma curve selection
    _hal.writeCommand(ILI9341_GAMMASET);
    _hal.writeData(0x01);
    
    // Positive gamma control
    _hal.writeCommand(ILI9341_GMCTRP1);
    _hal.writeData(0x0F);
    _hal.writeData(0x31);
    _hal.writeData(0x2B);
    _hal.writeData(0x0C);
    _hal.writeData(0x0E);
    _hal.writeData(0x08);
    _hal.writeData(0x4E);
    _hal.writeData(0xF1);
    _hal.writeData(0x37);
    _hal.writeData(0x07);
    _hal.writeData(0x10);
    _hal.writeData(0x03);
    _hal.writeData(0x0E);
    _hal.writeData(0x09);
    _hal.writeData(0x00);
    
    // Negative gamma control
    _hal.writeCommand(ILI9341_GMCTRN1);
    _hal.writeData(0x00);
    _hal.writeData(0x0E);
    _hal.writeData(0x14);
    _hal.writeData(0x03);
    _hal.writeData(0x11);
    _hal.writeData(0x07);
    _hal.writeData(0x31);
    _hal.writeData(0xC1);
    _hal.writeData(0x48);
    _hal.writeData(0x08);
    _hal.writeData(0x0F);
    _hal.writeData(0x0C);
    _hal.writeData(0x31);
    _hal.writeData(0x36);
    _hal.writeData(0x0F);
    
    // Exit sleep mode
    _hal.writeCommand(ILI9341_SLPOUT);
    _hal.delay(120);
    
    // Display on
    _hal.writeCommand(ILI9341_DISPON);
    _hal.delay(120);
    
    // Set display orientation
    setRotation(_hal.getConfig().rotation);
    
    // Set memory window to full screen
    setAddrWindow(0, 0, _hal.getConfig().width - 1, _hal.getConfig().height - 1);
    
    // Clear screen to black
    fillScreen(BLACK);
    
    // Turn on backlight
    setBacklight(true);
}

void ILI9341::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // Set column address range
    _hal.writeCommand(ILI9341_CASET);
    uint8_t data[4];
    data[0] = (x0 >> 8) & 0xFF;
    data[1] = x0 & 0xFF;
    data[2] = (x1 >> 8) & 0xFF;
    data[3] = x1 & 0xFF;
    _hal.writeDataBulk(data, 4);
    
    // Set row address range
    _hal.writeCommand(ILI9341_PASET);
    data[0] = (y0 >> 8) & 0xFF;
    data[1] = y0 & 0xFF;
    data[2] = (y1 >> 8) & 0xFF;
    data[3] = y1 & 0xFF;
    _hal.writeDataBulk(data, 4);
    
    // Prepare for memory write
    _hal.writeCommand(ILI9341_RAMWR);
}

void ILI9341::setRotation(Rotation rotation) {
    if (!_initialized) return;
    
    uint8_t madctl = 0;
    uint16_t old_width = _hal.getConfig().width;
    uint16_t old_height = _hal.getConfig().height;
    
    // Get original width and height in non-rotated state
    uint16_t native_width, native_height;
    if (_hal.getConfig().rotation == ROTATION_0 || _hal.getConfig().rotation == ROTATION_180) {
        native_width = old_width;
        native_height = old_height;
    } else {
        native_width = old_height;
        native_height = old_width;
    }
    
    // Set MADCTL value for ILI9341 with BGR color order
    switch (rotation) {
        case ROTATION_0:
            madctl = MADCTL_MY | MADCTL_BGR;  // BGR order for color correction
            _hal.setWidth(native_width);
            _hal.setHeight(native_height);
            break;
        case ROTATION_90:
            madctl = MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR;
            _hal.setWidth(native_height);
            _hal.setHeight(native_width);
            break;
        case ROTATION_180:
            madctl = MADCTL_MX | MADCTL_BGR;
            _hal.setWidth(native_width);
            _hal.setHeight(native_height);
            break;
        case ROTATION_270:
            madctl = MADCTL_MV | MADCTL_BGR;
            _hal.setWidth(native_height);
            _hal.setHeight(native_width);
            break;
    }
    
    _hal.writeCommand(ILI9341_MADCTL);
    _hal.writeData(madctl);
    _hal.setRotation(rotation);
    
    // If size has changed, re-set screen window
    if (old_width != _hal.getConfig().width || old_height != _hal.getConfig().height) {
        setAddrWindow(0, 0, _hal.getConfig().width - 1, _hal.getConfig().height - 1);
    }
}

void ILI9341::invertDisplay(bool invert) {
    _hal.writeCommand(invert ? ILI9341_INVON : ILI9341_INVOFF);
}

void ILI9341::fillScreen(uint16_t color) {
    if (_hal.isDmaEnabled()) {
        fillRectDMA(0, 0, _hal.getConfig().width, _hal.getConfig().height, color);
    } else {
        _gfx.fillRect(0, 0, _hal.getConfig().width, _hal.getConfig().height, color);
    }
}

void ILI9341::sleepDisplay(bool sleep) {
    _hal.writeCommand(sleep ? ILI9341_SLPIN : ILI9341_SLPOUT);
    _hal.delay(120);
}

void ILI9341::setBacklight(bool on) {
    _hal.setBacklight(on);
}

void ILI9341::setBrightness(uint8_t brightness) {
    _hal.setBrightness(brightness);
}

void ILI9341::reset() {
    _hal.reset();
    // Re-initialize display
    initializeDisplay();
}

bool ILI9341::drawImageDMA(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data) {
    if (!_initialized || x >= _hal.getConfig().width || y >= _hal.getConfig().height) {
        return false;
    }
    
    // Clip coordinates
    int16_t x1 = x + w - 1;
    int16_t y1 = y + h - 1;
    
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 >= _hal.getConfig().width) x1 = _hal.getConfig().width - 1;
    if (y1 >= _hal.getConfig().height) y1 = _hal.getConfig().height - 1;
    
    w = x1 - x + 1;
    h = y1 - y + 1;
    
    if (w <= 0 || h <= 0) return false;
    
    // Set drawing window
    setAddrWindow(x, y, x1, y1);
    
    // Use DMA to transfer image data
    return _hal.writeDataDma(data, w * h);
}

bool ILI9341::fillRectDMA(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (!_initialized || w <= 0 || h <= 0 ||
        x >= _hal.getConfig().width || y >= _hal.getConfig().height) {
        return false;
    }
    
    // Clip coordinates
    int16_t x1 = x + w - 1;
    int16_t y1 = y + h - 1;
    
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 >= _hal.getConfig().width) x1 = _hal.getConfig().width - 1;
    if (y1 >= _hal.getConfig().height) y1 = _hal.getConfig().height - 1;
    
    w = x1 - x + 1;
    h = y1 - y + 1;
    
    if (w <= 0 || h <= 0) return false;
    
    // Set drawing window
    setAddrWindow(x, y, x1, y1);
    
    // Prepare fill data
    const size_t buffer_size = 256;
    uint16_t fill_buffer[buffer_size];
    for (size_t i = 0; i < buffer_size; i++) {
        fill_buffer[i] = color;
    }
    
    // Calculate total pixels
    size_t total_pixels = w * h;
    size_t pixels_sent = 0;
    
    // Send in batches
    while (pixels_sent < total_pixels) {
        size_t pixels_to_send = (total_pixels - pixels_sent > buffer_size) ? 
                                buffer_size : (total_pixels - pixels_sent);
        
        if (!_hal.writeDataDma(fill_buffer, pixels_to_send)) {
            return false;
        }
        
        pixels_sent += pixels_to_send;
    }
    
    return true;
}

} // namespace ili9341
