#include "displays/ili9342/ili9342.hpp"
#include <cstdio>

namespace ili9342 {

// ILI9342 command definitions (mostly same as ILI9341, adjust if needed)
enum ILI9342_CMD {
    ILI9342_NOP         = 0x00,
    ILI9342_SWRESET     = 0x01,
    ILI9342_RDDID       = 0x04,
    ILI9342_RDDST       = 0x09,
    ILI9342_SLPIN       = 0x10,
    ILI9342_SLPOUT      = 0x11,
    ILI9342_PTLON       = 0x12,
    ILI9342_NORON       = 0x13,
    ILI9342_RDMODE      = 0x0A,
    ILI9342_RDMADCTL    = 0x0B,
    ILI9342_RDPIXFMT    = 0x0C,
    ILI9342_RDIMGFMT    = 0x0D,
    ILI9342_RDSELFDIAG  = 0x0F,
    ILI9342_INVOFF      = 0x20,
    ILI9342_INVON       = 0x21,
    ILI9342_GAMMASET    = 0x26,
    ILI9342_DISPOFF     = 0x28,
    ILI9342_DISPON      = 0x29,
    ILI9342_CASET       = 0x2A,
    ILI9342_PASET       = 0x2B,
    ILI9342_RAMWR       = 0x2C,
    ILI9342_RAMRD       = 0x2E,
    ILI9342_PTLAR       = 0x30,
    ILI9342_MADCTL      = 0x36,
    ILI9342_VSCRSADD    = 0x37,
    ILI9342_PIXFMT      = 0x3A,
    ILI9342_FRMCTR1     = 0xB1,
    ILI9342_FRMCTR2     = 0xB2,
    ILI9342_FRMCTR3     = 0xB3,
    ILI9342_INVCTR      = 0xB4,
    ILI9342_DFUNCTR     = 0xB6,
    ILI9342_PWCTR1      = 0xC0,
    ILI9342_PWCTR2      = 0xC1,
    ILI9342_PWCTR3      = 0xC2,
    ILI9342_PWCTR4      = 0xC3,
    ILI9342_PWCTR5      = 0xC4,
    ILI9342_VMCTR1      = 0xC5,
    ILI9342_VMCTR2      = 0xC7,
    ILI9342_GMCTRP1     = 0xE0,
    ILI9342_GMCTRN1     = 0xE1,
    ILI9342_PWCTR6      = 0xFC
};

#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH  0x04

ILI9342::ILI9342() : _gfx(this), _initialized(false) {}
ILI9342::~ILI9342() {}

bool ILI9342::begin(const Config& config) {
    if (_initialized) return true;
    Config native_config = config;
    if (!_hal.init(native_config)) {
        printf("Failed to initialize ILI9342 hardware abstraction layer\n");
        return false;
    }
    initializeDisplay();
    _initialized = true;
    return true;
}

bool ILI9342::begin(spi_inst_t* spi, uint8_t cs_pin, uint8_t dc_pin, uint8_t rst_pin, uint8_t bl_pin, uint16_t width, uint16_t height) {
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

void ILI9342::initializeDisplay() {
    _hal.reset();
    _hal.writeCommand(ILI9342_SWRESET);
    _hal.delay(150);
    _hal.writeCommand(ILI9342_SLPOUT);
    _hal.delay(120);
    // ILI9342 initialization sequence (see datasheet, adjust as needed)
    // The following is based on ILI9341, but you may need to tweak values for ILI9342
    _hal.writeCommand(0xCB); _hal.writeData(0x39); _hal.writeData(0x2C); _hal.writeData(0x00); _hal.writeData(0x34); _hal.writeData(0x02);
    _hal.writeCommand(0xCF); _hal.writeData(0x00); _hal.writeData(0xC1); _hal.writeData(0x30);
    _hal.writeCommand(0xE8); _hal.writeData(0x85); _hal.writeData(0x00); _hal.writeData(0x78);
    _hal.writeCommand(0xEA); _hal.writeData(0x00); _hal.writeData(0x00);
    _hal.writeCommand(0xED); _hal.writeData(0x64); _hal.writeData(0x03); _hal.writeData(0x12); _hal.writeData(0x81);
    _hal.writeCommand(0xF7); _hal.writeData(0x20);
    _hal.writeCommand(ILI9342_PWCTR1); _hal.writeData(0x23);
    _hal.writeCommand(ILI9342_PWCTR2); _hal.writeData(0x10);
    _hal.writeCommand(ILI9342_VMCTR1); _hal.writeData(0x3e); _hal.writeData(0x28);
    _hal.writeCommand(ILI9342_VMCTR2); _hal.writeData(0x86);
    _hal.writeCommand(ILI9342_MADCTL); _hal.writeData(0x40);
    _hal.writeCommand(ILI9342_PIXFMT); _hal.writeData(0x55);
    _hal.writeCommand(ILI9342_FRMCTR1); _hal.writeData(0x00); _hal.writeData(0x18);
    _hal.writeCommand(ILI9342_DFUNCTR); _hal.writeData(0x08); _hal.writeData(0x82); _hal.writeData(0x27);
    _hal.writeCommand(0xF2); _hal.writeData(0x00);
    _hal.writeCommand(ILI9342_GAMMASET); _hal.writeData(0x01);
    _hal.writeCommand(ILI9342_GMCTRP1); _hal.writeData(0x0F); _hal.writeData(0x31); _hal.writeData(0x2B); _hal.writeData(0x0C); _hal.writeData(0x0E); _hal.writeData(0x08); _hal.writeData(0x4E); _hal.writeData(0xF1); _hal.writeData(0x37); _hal.writeData(0x07); _hal.writeData(0x10); _hal.writeData(0x03); _hal.writeData(0x0E); _hal.writeData(0x09); _hal.writeData(0x00);
    _hal.writeCommand(ILI9342_GMCTRN1); _hal.writeData(0x00); _hal.writeData(0x0E); _hal.writeData(0x14); _hal.writeData(0x03); _hal.writeData(0x11); _hal.writeData(0x07); _hal.writeData(0x31); _hal.writeData(0xC1); _hal.writeData(0x48); _hal.writeData(0x08); _hal.writeData(0x0F); _hal.writeData(0x0C); _hal.writeData(0x31); _hal.writeData(0x36); _hal.writeData(0x0F);
    _hal.writeCommand(ILI9342_SLPOUT); _hal.delay(120);
    _hal.writeCommand(ILI9342_DISPON); _hal.delay(120);
    setRotation(_hal.getConfig().rotation);
    setAddrWindow(0, 0, _hal.getConfig().width - 1, _hal.getConfig().height - 1);
    fillScreen(BLACK);
    setBacklight(true);
}

void ILI9342::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    _hal.writeCommand(ILI9342_CASET);
    uint8_t data[4];
    data[0] = (x0 >> 8) & 0xFF;
    data[1] = x0 & 0xFF;
    data[2] = (x1 >> 8) & 0xFF;
    data[3] = x1 & 0xFF;
    _hal.writeDataBulk(data, 4);
    _hal.writeCommand(ILI9342_PASET);
    data[0] = (y0 >> 8) & 0xFF;
    data[1] = y0 & 0xFF;
    data[2] = (y1 >> 8) & 0xFF;
    data[3] = y1 & 0xFF;
    _hal.writeDataBulk(data, 4);
    _hal.writeCommand(ILI9342_RAMWR);
}

void ILI9342::setRotation(Rotation rotation) {
    if (!_initialized) return;
    uint8_t madctl = 0;
    uint16_t old_width = _hal.getConfig().width;
    uint16_t old_height = _hal.getConfig().height;
    uint16_t native_width, native_height;
    if (_hal.getConfig().rotation == ROTATION_0 || _hal.getConfig().rotation == ROTATION_180) {
        native_width = old_width;
        native_height = old_height;
    } else {
        native_width = old_height;
        native_height = old_width;
    }
        switch (rotation) {
            case ROTATION_0:
                madctl = MADCTL_MX | MADCTL_MY | MADCTL_RGB;
                _hal.setWidth(native_width);
                _hal.setHeight(native_height);
                break;
            case ROTATION_90:
                madctl = MADCTL_MV | MADCTL_MY | MADCTL_RGB;
                _hal.setWidth(native_height);
                _hal.setHeight(native_width);
                break;
            case ROTATION_180:
                madctl = MADCTL_RGB;
                _hal.setWidth(native_width);
                _hal.setHeight(native_height);
                break;
            case ROTATION_270:
                madctl = MADCTL_MV | MADCTL_MX | MADCTL_RGB;
                _hal.setWidth(native_height);
                _hal.setHeight(native_width);
                break;
        }
    _hal.writeCommand(ILI9342_MADCTL);
    _hal.writeData(madctl);
    _hal.setRotation(rotation);
    if (old_width != _hal.getConfig().width || old_height != _hal.getConfig().height) {
        setAddrWindow(0, 0, _hal.getConfig().width - 1, _hal.getConfig().height - 1);
    }
}

void ILI9342::invertDisplay(bool invert) {
    _hal.writeCommand(invert ? ILI9342_INVON : ILI9342_INVOFF);
}

void ILI9342::fillScreen(uint16_t color) {
    if (_hal.isDmaEnabled()) {
        fillRectDMA(0, 0, _hal.getConfig().width, _hal.getConfig().height, color);
    } else {
        _gfx.fillRect(0, 0, _hal.getConfig().width, _hal.getConfig().height, color);
    }
}

void ILI9342::sleepDisplay(bool sleep) {
    _hal.writeCommand(sleep ? ILI9342_SLPIN : ILI9342_SLPOUT);
    _hal.delay(120);
}

void ILI9342::setBacklight(bool on) {
    _hal.setBacklight(on);
}

void ILI9342::setBrightness(uint8_t brightness) {
    _hal.setBrightness(brightness);
}

void ILI9342::reset() {
    _hal.reset();
    initializeDisplay();
}

bool ILI9342::drawImageDMA(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data) {
    if (!_initialized || x >= _hal.getConfig().width || y >= _hal.getConfig().height) return false;
    int16_t x1 = x + w - 1;
    int16_t y1 = y + h - 1;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 >= _hal.getConfig().width) x1 = _hal.getConfig().width - 1;
    if (y1 >= _hal.getConfig().height) y1 = _hal.getConfig().height - 1;
    w = x1 - x + 1;
    h = y1 - y + 1;
    if (w <= 0 || h <= 0) return false;
    setAddrWindow(x, y, x1, y1);
    return _hal.writeDataDma(data, w * h);
}

bool ILI9342::fillRectDMA(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (!_initialized || w <= 0 || h <= 0 || x >= _hal.getConfig().width || y >= _hal.getConfig().height) return false;
    int16_t x1 = x + w - 1;
    int16_t y1 = y + h - 1;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 >= _hal.getConfig().width) x1 = _hal.getConfig().width - 1;
    if (y1 >= _hal.getConfig().height) y1 = _hal.getConfig().height - 1;
    w = x1 - x + 1;
    h = y1 - y + 1;
    if (w <= 0 || h <= 0) return false;
    setAddrWindow(x, y, x1, y1);
    const size_t buffer_size = 256;
    uint16_t fill_buffer[buffer_size];
    for (size_t i = 0; i < buffer_size; i++) fill_buffer[i] = color;
    size_t total_pixels = w * h;
    size_t pixels_sent = 0;
    while (pixels_sent < total_pixels) {
        size_t pixels_to_send = (total_pixels - pixels_sent > buffer_size) ? buffer_size : (total_pixels - pixels_sent);
        if (!_hal.writeDataDma(fill_buffer, pixels_to_send)) return false;
        pixels_sent += pixels_to_send;
    }
    return true;
}

} // namespace ili9342
