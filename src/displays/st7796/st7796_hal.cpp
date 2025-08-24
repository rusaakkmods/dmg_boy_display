#include "displays/st7796/st7796_hal.hpp"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>

namespace st7796 {

// Define global variable for DMA interrupt handling
static HAL* current_hal_instance = nullptr;

// DMA transfer completion handler
void dma_complete_handler() {
    // Ensure we have a valid instance
    if (current_hal_instance) {
        // Clear interrupt flag
        dma_hw->ints0 = 1u << current_hal_instance->_dma_tx_channel;
        
        // Update status
        current_hal_instance->_dma_busy = false;
    }
}

HAL::HAL() : _initialized(false), _dma_tx_channel(-1), _dma_buffer(nullptr), _dma_buffer_size(0), _dma_enabled(false), _dma_busy(false) {
}

HAL::~HAL() {
    if (_initialized) {
        cleanupDma();
    }
}

bool HAL::init(const Config& config) {
    if (_initialized) {
        return true;
    }
    
    _config = config;
    printf("ST7796 HAL: Starting initialization...\n");
    
    // Initialize SPI
    printf("ST7796 HAL: Initializing SPI at %d Hz...\n", _config.spi_speed_hz);
    spi_init(_config.spi_inst, _config.spi_speed_hz);
    
    // Set SPI format (8 bits, SPI mode 0)
    spi_set_format(_config.spi_inst, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    
    // Set up SPI pins
    gpio_set_function(_config.pin_din, GPIO_FUNC_SPI);
    gpio_set_function(_config.pin_sck, GPIO_FUNC_SPI);
    
    printf("ST7796 HAL: Setting up control pins...\n");
    
    // Initialize pins
    gpio_init(_config.pin_cs);
    gpio_init(_config.pin_dc);
    gpio_init(_config.pin_reset);
    gpio_init(_config.pin_bl);
    
    // Set direction
    gpio_set_dir(_config.pin_cs, GPIO_OUT);
    gpio_set_dir(_config.pin_dc, GPIO_OUT);
    gpio_set_dir(_config.pin_reset, GPIO_OUT);
    
    // Initialize PWM for backlight brightness control
    gpio_set_function(_config.pin_bl, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(_config.pin_bl);
    pwm_set_wrap(slice_num, 255);  // 8-bit resolution (0-255)
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(_config.pin_bl), 0);  // Start at 0
    pwm_set_enabled(slice_num, true);
    
    // Initial state
    gpio_put(_config.pin_cs, 1);     // Not selected
    gpio_put(_config.pin_dc, 1);     // Data mode
    gpio_put(_config.pin_reset, 1);  // Not reset
    
    // Setup DMA if enabled
    if (_config.dma.enabled) {
        printf("ST7796 HAL: Setting up DMA...\n");
        initDma();
    }
    
    printf("ST7796 HAL: Initialization complete!\n");
    _initialized = true;
    return true;
}

void HAL::initDma() {
    if (!_config.dma.enabled) {
        return;
    }
    
    // Claim a DMA channel
    _dma_tx_channel = dma_claim_unused_channel(true);
    
    // Configure DMA channel
    dma_channel_config c = dma_channel_get_default_config(_dma_tx_channel);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_dreq(&c, spi_get_dreq(_config.spi_inst, true));
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    
    dma_channel_configure(
        _dma_tx_channel,
        &c,
        &spi_get_hw(_config.spi_inst)->dr, // Write address
        nullptr,                           // Read address (set later)
        0,                                 // Transfer count (set later)
        false                              // Don't start yet
    );
    
    printf("ST7796 HAL: DMA channel %d initialized\n", _dma_tx_channel);
}

void HAL::cleanupDma() {
    if (_dma_tx_channel >= 0) {
        // Stop any ongoing transfer
        dma_channel_abort(_dma_tx_channel);
        
        // Disable interrupt
        dma_channel_set_irq0_enabled(_dma_tx_channel, false);
        
        // Release channel
        dma_channel_unclaim(_dma_tx_channel);
        _dma_tx_channel = -1;
    }
}

void HAL::writeCommand(uint8_t cmd) {
    gpio_put(_config.pin_dc, 0);  // Command mode
    gpio_put(_config.pin_cs, 0);  // Select
    spi_write_blocking(_config.spi_inst, &cmd, 1);
    gpio_put(_config.pin_cs, 1);  // Deselect
}

void HAL::writeData(uint8_t data) {
    gpio_put(_config.pin_dc, 1);  // Data mode
    gpio_put(_config.pin_cs, 0);  // Select
    spi_write_blocking(_config.spi_inst, &data, 1);
    gpio_put(_config.pin_cs, 1);  // Deselect
}

void HAL::writeData16(uint16_t data) {
    uint8_t buffer[2] = {(uint8_t)(data >> 8), (uint8_t)(data & 0xFF)};
    gpio_put(_config.pin_dc, 1);  // Data mode
    gpio_put(_config.pin_cs, 0);  // Select
    spi_write_blocking(_config.spi_inst, buffer, 2);
    gpio_put(_config.pin_cs, 1);  // Deselect
}

void HAL::writeDataBuffer(const uint8_t* buffer, size_t length) {
    gpio_put(_config.pin_dc, 1);  // Data mode
    gpio_put(_config.pin_cs, 0);  // Select
    spi_write_blocking(_config.spi_inst, buffer, length);
    gpio_put(_config.pin_cs, 1);  // Deselect
}

void HAL::writeDataBuffer16(const uint16_t* buffer, size_t length) {
    gpio_put(_config.pin_dc, 1);  // Data mode
    gpio_put(_config.pin_cs, 0);  // Select
    
    // Convert 16-bit values to bytes and send
    for (size_t i = 0; i < length; i++) {
        uint8_t data[2] = {(uint8_t)(buffer[i] >> 8), (uint8_t)(buffer[i] & 0xFF)};
        spi_write_blocking(_config.spi_inst, data, 2);
    }
    
    gpio_put(_config.pin_cs, 1);  // Deselect
}

bool HAL::writeDataDMA(const uint16_t* data, size_t length) {
    if (!_dma_enabled || !_dma_buffer || _dma_tx_channel < 0) {
        // If DMA is not available, fall back to normal method
        writeDataBuffer16(data, length);
        return false;
    }
    
    // If data is too large for DMA buffer, fall back to non-DMA method
    if (length * 2 > _dma_buffer_size) {
        writeDataBuffer16(data, length);
        return false;
    }
    
    // If DMA is busy, wait for completion
    if (_dma_busy) {
        if (!waitForDmaComplete(100)) {  // 100ms timeout
            printf("ST7796 DMA timeout, abort operation\n");
            abortDma();
            return false;
        }
    }
    
    // Set data/command pin to data mode
    gpio_put(_config.pin_dc, 1);
    // Set chip select pin
    gpio_put(_config.pin_cs, 0);
    
    // Convert 16-bit pixel data to 8-bit bytes before sending
    uint8_t* byte_buffer = (uint8_t*)_dma_buffer;
    for (size_t i = 0; i < length; i++) {
        uint16_t color = data[i];
        // Convert to bytes with proper byte order for ST7796
        byte_buffer[i * 2] = (uint8_t)(color >> 8);      // High byte first
        byte_buffer[i * 2 + 1] = (uint8_t)(color & 0xFF); // Low byte second
    }
    
    // Mark DMA busy
    _dma_busy = true;
    
    // Configure and start DMA transfer (8-bit transfers, byte count)
    dma_channel_set_read_addr(_dma_tx_channel, byte_buffer, false);
    dma_channel_set_trans_count(_dma_tx_channel, length * 2, true); // Start transfer (length * 2 for bytes)
    
    // Wait for transfer to complete
    if (!waitForDmaComplete(100)) {  // 100ms timeout
        printf("ST7796 DMA transfer timeout\n");
        abortDma();
        gpio_put(_config.pin_cs, 1); // Release chip select
        return false;
    }
    
    // Release chip select
    gpio_put(_config.pin_cs, 1);
    return true;
}

void HAL::waitForDmaComplete() {
    waitForDmaComplete(100); // 100ms timeout
}

bool HAL::waitForDmaComplete(uint32_t timeout_ms) {
    uint32_t start = to_ms_since_boot(get_absolute_time());
    while (_dma_busy) {
        // Check timeout
        if (to_ms_since_boot(get_absolute_time()) - start > timeout_ms) {
            return false;
        }
        // Yield CPU time
        tight_loop_contents();
    }
    return true;
}

void HAL::abortDma() {
    if (_dma_tx_channel >= 0) {
        dma_channel_abort(_dma_tx_channel);
    }
    _dma_busy = false;
}

void HAL::reset() {
    gpio_put(_config.pin_reset, 0);
    sleep_ms(10);
    gpio_put(_config.pin_reset, 1);
    sleep_ms(120);
}

void HAL::setBacklight(bool on) {
    // Use maximum drive strength for brightest backlight
    gpio_put(_config.pin_bl, on ? 1 : 0);
}

void HAL::setBrightness(uint8_t brightness) {
    // Use PWM to control backlight brightness (0-255)
    uint slice_num = pwm_gpio_to_slice_num(_config.pin_bl);
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(_config.pin_bl), brightness);
}

void HAL::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // Column address set
    writeCommand(ST7796_CASET);
    writeData(x0 >> 8);
    writeData(x0 & 0xFF);
    writeData(x1 >> 8);
    writeData(x1 & 0xFF);
    
    // Row address set
    writeCommand(ST7796_RASET);
    writeData(y0 >> 8);
    writeData(y0 & 0xFF);
    writeData(y1 >> 8);
    writeData(y1 & 0xFF);
    
    // Memory write
    writeCommand(ST7796_RAMWR);
}

} // namespace st7796
