#include "eeprom.h"

#ifdef VERSION_V1_1

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "config.h"
#include "palettes.hpp"

#define EEPROM_PALETTE_ADDR 0x00
#define PALETTE_MAGIC 0x45

struct PaletteEEPROMData {
    uint8_t magic;
    uint8_t palette_index;
};

// Cache last EEPROM value to avoid redundant writes
static uint8_t cached_eeprom_palette = 0;
static bool eeprom_cache_valid = false;
static bool eeprom_available = false;

static uint8_t get_default_palette_index() {
    const uint16_t* default_palette = DEFAULT_PALETTE_NAME;
    for (size_t i = 0; i < NUM_PALETTES; i++) {
        if (PALETTE_LIST[i] == default_palette) {
            return i;
        }
    }
    return 0;
}

void init_eeprom() {
    i2c_init(I2C_CHANNEL, 100 * 1000);
    gpio_set_function(PIN_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_I2C_SDA);
    gpio_pull_up(PIN_I2C_SCL);
    sleep_ms(10);
    
    uint8_t test_buf[1] = {0x00};
    eeprom_available = (i2c_write_blocking(I2C_CHANNEL, EEPROM_ADDR, test_buf, 1, true) == 1);
}

void save_palette_to_eeprom(uint8_t palette_index) {
    if (!eeprom_available || palette_index >= NUM_PALETTES || (eeprom_cache_valid && cached_eeprom_palette == palette_index)) {
        return;
    }
    
    uint8_t write_buf[4] = {0x00, EEPROM_PALETTE_ADDR, PALETTE_MAGIC, palette_index};
    
    if (i2c_write_blocking(I2C_CHANNEL, EEPROM_ADDR, write_buf, 4, false) == 4) {
        sleep_ms(5);
        cached_eeprom_palette = palette_index;
        eeprom_cache_valid = true;
    }
}

uint8_t load_palette_from_eeprom() {
    uint8_t default_idx = get_default_palette_index();
    
    if (!eeprom_available) {
        return default_idx;
    }
    
    uint8_t addr_buf[2] = {0x00, EEPROM_PALETTE_ADDR};
    uint8_t read_buf[2];
    
    if (i2c_write_blocking(I2C_CHANNEL, EEPROM_ADDR, addr_buf, 2, true) != 2 ||
        i2c_read_blocking(I2C_CHANNEL, EEPROM_ADDR, read_buf, 2, false) != 2) {
        cached_eeprom_palette = 0xFF;
        eeprom_cache_valid = false;
        save_palette_to_eeprom(default_idx);
        return default_idx;
    }
    
    if (read_buf[0] != PALETTE_MAGIC || read_buf[1] >= NUM_PALETTES) {
        cached_eeprom_palette = 0xFF;
        eeprom_cache_valid = false;
        save_palette_to_eeprom(default_idx);
        return default_idx;
    }
    
    cached_eeprom_palette = read_buf[1];
    eeprom_cache_valid = true;
    return read_buf[1];
}

#endif // VERSION_V1_1
