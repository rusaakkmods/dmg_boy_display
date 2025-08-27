#include "displays/ili9342/ili9342_hal.hpp"
#include <cstdio>
#include <cstdlib>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/pwm.h"

namespace ili9342 {

static HAL* g_hal_instance = nullptr;
void ili9342_dma_complete_handler() {
    if (g_hal_instance) {
        g_hal_instance->_dma_busy = false;
    }
}

HAL::HAL() : _initialized(false), _dma_tx_channel(-1), _dma_buffer(nullptr), _dma_buffer_size(0), _dma_enabled(false), _dma_busy(false) {}
HAL::~HAL() { cleanupDma(); }

bool HAL::init(const Config& config) {
    if (_initialized) return true;
    _config = config;
    uint actual_baud = spi_init(_config.spi_inst, _config.spi_speed_hz);
    printf("SPI initialized at %u Hz (requested %u Hz)\n", actual_baud, _config.spi_speed_hz);
    spi_set_format(_config.spi_inst, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(_config.pin_din, GPIO_FUNC_SPI);
    gpio_set_function(_config.pin_sck, GPIO_FUNC_SPI);
    gpio_init(_config.pin_cs); gpio_set_dir(_config.pin_cs, GPIO_OUT); gpio_put(_config.pin_cs, 1);
    gpio_init(_config.pin_dc); gpio_set_dir(_config.pin_dc, GPIO_OUT); gpio_put(_config.pin_dc, 0);
    gpio_init(_config.pin_reset); gpio_set_dir(_config.pin_reset, GPIO_OUT); gpio_put(_config.pin_reset, 1);
    gpio_init(_config.pin_bl); gpio_set_function(_config.pin_bl, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(_config.pin_bl);
    pwm_set_wrap(slice_num, 255);
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(_config.pin_bl), 0);
    pwm_set_enabled(slice_num, true);
    if (_config.dma.enabled) { initDma(); }
    _initialized = true;
    return true;
}

void HAL::initDma() {
    if (_dma_enabled) return;
    _dma_tx_channel = dma_claim_unused_channel(true);
    if (_dma_tx_channel < 0) { printf("Failed to claim DMA channel\n"); return; }
    _dma_buffer_size = _config.dma.buffer_size;
    _dma_buffer = (uint16_t*)malloc(_dma_buffer_size * sizeof(uint16_t));
    if (!_dma_buffer) { printf("Failed to allocate DMA buffer\n"); dma_channel_unclaim(_dma_tx_channel); _dma_tx_channel = -1; return; }
    dma_channel_config config = dma_channel_get_default_config(_dma_tx_channel);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_8);
    channel_config_set_dreq(&config, spi_get_dreq(_config.spi_inst, true));
    channel_config_set_write_increment(&config, false);
    channel_config_set_read_increment(&config, true);
    dma_channel_configure(_dma_tx_channel, &config, &spi_get_hw(_config.spi_inst)->dr, nullptr, 0, false);
    g_hal_instance = this;
    dma_channel_set_irq0_enabled(_dma_tx_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_0, ili9342_dma_complete_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    _dma_enabled = true;
    printf("DMA initialized on channel %d\n", _dma_tx_channel);
}

void HAL::cleanupDma() {
    if (_dma_enabled) {
        irq_set_enabled(DMA_IRQ_0, false);
        dma_channel_cleanup(_dma_tx_channel);
        dma_channel_unclaim(_dma_tx_channel);
        if (_dma_buffer) { free(_dma_buffer); _dma_buffer = nullptr; }
        _dma_enabled = false;
        _dma_tx_channel = -1;
        g_hal_instance = nullptr;
    }
}

void HAL::writeCommand(uint8_t cmd) {
    gpio_put(_config.pin_dc, 0);
    gpio_put(_config.pin_cs, 0);
    spi_write_blocking(_config.spi_inst, &cmd, 1);
    gpio_put(_config.pin_cs, 1);
}

void HAL::writeData(uint8_t data) {
    gpio_put(_config.pin_dc, 1);
    gpio_put(_config.pin_cs, 0);
    spi_write_blocking(_config.spi_inst, &data, 1);
    gpio_put(_config.pin_cs, 1);
}

void HAL::writeDataBulk(const uint8_t* data, size_t len) {
    gpio_put(_config.pin_dc, 1);
    gpio_put(_config.pin_cs, 0);
    spi_write_blocking(_config.spi_inst, data, len);
    gpio_put(_config.pin_cs, 1);
}

bool HAL::writeDataDma(const uint16_t* data, size_t len) {
    if (!_dma_enabled || _dma_busy) return false;
    const uint8_t* byte_data = reinterpret_cast<const uint8_t*>(data);
    size_t byte_len = len * 2;
    gpio_put(_config.pin_dc, 1);
    gpio_put(_config.pin_cs, 0);
    _dma_busy = true;
    dma_channel_set_read_addr(_dma_tx_channel, byte_data, false);
    dma_channel_set_trans_count(_dma_tx_channel, byte_len, true);
    return true;
}

bool HAL::waitForDmaComplete(uint32_t timeout_ms) {
    uint32_t start = time_us_32();
    while (_dma_busy && (time_us_32() - start) < (timeout_ms * 1000)) { tight_loop_contents(); }
    if (_dma_busy) { abortDma(); return false; }
    gpio_put(_config.pin_cs, 1);
    return true;
}

void HAL::abortDma() {
    if (_dma_enabled && _dma_busy) {
        dma_channel_abort(_dma_tx_channel);
        _dma_busy = false;
        gpio_put(_config.pin_cs, 1);
    }
}

void HAL::reset() {
    gpio_put(_config.pin_reset, 0);
    delay(10);
    gpio_put(_config.pin_reset, 1);
    delay(120);
}

void HAL::setBacklight(bool on) {
    gpio_put(_config.pin_bl, on ? 1 : 0);
}

void HAL::setBrightness(uint8_t brightness) {
    uint slice_num = pwm_gpio_to_slice_num(_config.pin_bl);
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(_config.pin_bl), brightness);
}

void HAL::delay(uint32_t ms) { sleep_ms(ms); }

} // namespace ili9342
