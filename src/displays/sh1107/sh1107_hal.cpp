#include "displays/sh1107/sh1107_hal.hpp"
#include "pico/stdlib.h"
#include <string.h>

using namespace sh1107;

HAL::HAL() : _initialized(false) {}
HAL::~HAL() {}

static void spi_tx_blocking(spi_inst_t* spi, const uint8_t* buf, size_t len) {
    // Optimized: send entire buffer at once instead of byte-by-byte
    spi_write_blocking(spi, buf, len);
}

void HAL::spi_init_hw() {
    spi_init(_config.spi_inst, _config.spi_speed_hz);
    gpio_set_function(_config.pin_din, GPIO_FUNC_SPI);
    gpio_set_function(_config.pin_sck, GPIO_FUNC_SPI);
    gpio_set_dir(_config.pin_cs, GPIO_OUT);
    gpio_put(_config.pin_cs, 1);
    gpio_init(_config.pin_dc); gpio_set_dir(_config.pin_dc, GPIO_OUT);
    gpio_init(_config.pin_reset); gpio_set_dir(_config.pin_reset, GPIO_OUT);
}

bool HAL::init(const Config& config) {
    _config = config;
    spi_init_hw();
    reset();

    // Minimal init sequence based on SH1107 datasheet v2.1
    send_command(0xAE); // Display off
    send_command(0xA8); send_command(0x7F); // Set multiplex ratio 127
    send_command(0xD3); send_command(0x00); // Display offset
    send_command(0x40); // Set display start line to 0
    // Note: Segment remap and COM scan direction will be set by setRotation()
    send_command(0xDA); send_command(0x12); // COM pins hardware config
    send_command(0x81); send_command(0x7F); // Contrast
    send_command(0xA4); // Display RAM on
    send_command(0xA6); // Normal display
    
    _initialized = true;
    
    // Apply the configured rotation
    setRotation(_config.rotation);
    
    send_command(0xAF); // Display ON

    return true;
}

void HAL::reset() {
    gpio_put(_config.pin_reset, 0);
    sleep_ms(10);
    gpio_put(_config.pin_reset, 1);
    sleep_ms(10);
}

void HAL::send_command(uint8_t cmd) {
    gpio_put(_config.pin_cs, 0);
    gpio_put(_config.pin_dc, 0); // command
    spi_tx_blocking(_config.spi_inst, &cmd, 1);
    gpio_put(_config.pin_cs, 1);
}

void HAL::send_data(const uint8_t* data, size_t len) {
    gpio_put(_config.pin_cs, 0);
    gpio_put(_config.pin_dc, 1); // data
    spi_tx_blocking(_config.spi_inst, data, len);
    gpio_put(_config.pin_cs, 1);
}

void HAL::setContrast(uint8_t v) {
    send_command(0x81);
    send_command(v);
}

void HAL::setRotation(Rotation r) {
    _config.rotation = r;
    
    if (!_initialized) return;
    
    // SH1107 rotation using segment remap and COM scan direction
    switch (r) {
        case ROTATION_0:
            // Normal orientation
            send_command(0xA1); // Segment remap (column 127 mapped to SEG0)
            send_command(0xC8); // COM scan direction (remapped mode, scan from COM[N-1] to COM0)
            break;
        case ROTATION_90:
            // 90 degrees - for OLED this might not be perfectly supported, fallback to 180
            send_command(0xA0); // Segment remap (column 0 mapped to SEG0)
            send_command(0xC8); // COM scan direction (remapped mode)
            break;
        case ROTATION_180:
            // 180 degrees rotation
            send_command(0xA0); // Segment remap (column 0 mapped to SEG0)
            send_command(0xC0); // COM scan direction (normal mode, scan from COM0 to COM[N-1])
            break;
        case ROTATION_270:
            // 270 degrees - for OLED this might not be perfectly supported, fallback to 0
            send_command(0xA1); // Segment remap (column 127 mapped to SEG0)
            send_command(0xC0); // COM scan direction (normal mode)
            break;
    }
}

void HAL::writeCommand(uint8_t cmd) { send_command(cmd); }
void HAL::writeData(uint8_t data) { send_data(&data, 1); }
void HAL::writeDataBuffer(const uint8_t* buffer, size_t length) { send_data(buffer, length); }

void HAL::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // SH1107 uses page addressing; we'll set column start and page start
    // Note: driver Graphics implementation converts to page buffers, so nothing needed here for now
    (void)x0; (void)y0; (void)x1; (void)y1;
}
