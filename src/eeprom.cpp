#include "eeprom.h"

#ifdef VERSION_V1_1

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "config.h"
#include "palettes.hpp"

#define EEPROM_SETTINGS_ADDR 0x00
#define SETTINGS_MAGIC 0xA5

struct SettingsPackage {
    uint8_t magic;
    uint8_t palette_index;
    int16_t offset_x;
    int16_t offset_y;
    uint8_t render_mode;
};

// Cache current settings
static SettingsPackage current_settings = {0, 0, X_OFF_BASE, Y_OFF_BASE, 0};
static bool settings_loaded = false;
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

static void save_settings_package() {
    if (!eeprom_available) return;
    
    uint8_t write_buf[8] = {
        EEPROM_SETTINGS_ADDR,
        SETTINGS_MAGIC,
        current_settings.palette_index,
        (uint8_t)(current_settings.offset_x & 0xFF),
        (uint8_t)((current_settings.offset_x >> 8) & 0xFF),
        (uint8_t)(current_settings.offset_y & 0xFF),
        (uint8_t)((current_settings.offset_y >> 8) & 0xFF),
        current_settings.render_mode
    };
    
    i2c_write_blocking(I2C_CHANNEL, EEPROM_ADDR, write_buf, 8, false);
    sleep_ms(5);
}

static void load_settings_package() {
    if (settings_loaded) return;
    
    uint8_t default_palette = get_default_palette_index();
    
    if (!eeprom_available) {
        current_settings.magic = SETTINGS_MAGIC;
        current_settings.palette_index = default_palette;
        current_settings.offset_x = X_OFF_BASE;
        current_settings.offset_y = Y_OFF_BASE;
        current_settings.render_mode = 0;
        settings_loaded = true;
        return;
    }
    
    uint8_t addr_buf[1] = {EEPROM_SETTINGS_ADDR};
    uint8_t read_buf[7];
    
    if (i2c_write_blocking(I2C_CHANNEL, EEPROM_ADDR, addr_buf, 1, true) != 1 ||
        i2c_read_blocking(I2C_CHANNEL, EEPROM_ADDR, read_buf, 7, false) != 7) {
        // EEPROM read failed, use defaults
        current_settings.magic = SETTINGS_MAGIC;
        current_settings.palette_index = default_palette;
        current_settings.offset_x = X_OFF_BASE;
        current_settings.offset_y = Y_OFF_BASE;
        current_settings.render_mode = 0;
        settings_loaded = true;
        save_settings_package();
        return;
    }
    
    // Validate magic byte and data
    if (read_buf[0] != SETTINGS_MAGIC || read_buf[1] >= NUM_PALETTES) {
        // Invalid data, use defaults
        current_settings.magic = SETTINGS_MAGIC;
        current_settings.palette_index = default_palette;
        current_settings.offset_x = X_OFF_BASE;
        current_settings.offset_y = Y_OFF_BASE;
        current_settings.render_mode = 0;
        settings_loaded = true;
        save_settings_package();
        return;
    }
    
    // Load valid settings
    current_settings.magic = SETTINGS_MAGIC;
    current_settings.palette_index = read_buf[1];
    current_settings.offset_x = (int16_t)(read_buf[2] | (read_buf[3] << 8));
    current_settings.offset_y = (int16_t)(read_buf[4] | (read_buf[5] << 8));
    current_settings.render_mode = read_buf[6];
    settings_loaded = true;
}

void save_palette_to_eeprom(uint8_t palette_index) {
    if (!eeprom_available || palette_index >= NUM_PALETTES) return;
    
    load_settings_package();
    
    if (current_settings.palette_index != palette_index) {
        current_settings.palette_index = palette_index;
        save_settings_package();
    }
}

uint8_t load_palette_from_eeprom() {
    load_settings_package();
    return current_settings.palette_index;
}

void save_offset_x_to_eeprom(int16_t offset_x) {
    if (!eeprom_available) return;
    
    load_settings_package();
    
    if (current_settings.offset_x != offset_x) {
        current_settings.offset_x = offset_x;
        save_settings_package();
    }
}

int16_t load_offset_x_from_eeprom() {
    load_settings_package();
    return current_settings.offset_x;
}

void save_offset_y_to_eeprom(int16_t offset_y) {
    if (!eeprom_available) return;
    
    load_settings_package();
    
    if (current_settings.offset_y != offset_y) {
        current_settings.offset_y = offset_y;
        save_settings_package();
    }
}

int16_t load_offset_y_from_eeprom() {
    load_settings_package();
    return current_settings.offset_y;
}

void save_render_mode_to_eeprom(uint8_t render_mode) {
    load_settings_package();
    current_settings.render_mode = render_mode;
    save_settings_package();
}

uint8_t load_render_mode_from_eeprom() {
    load_settings_package();
    return current_settings.render_mode;
}

#endif // VERSION_V1_1
