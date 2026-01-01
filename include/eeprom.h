#ifndef EEPROM_H
#define EEPROM_H

#include <stdint.h>

#ifdef VERSION_V1_1

// EEPROM memory layout constants
#define EEPROM_PALETTE_ADDR 0x00
#define PALETTE_MAGIC 0x45

/**
 * Initialize I2C EEPROM and detect chip presence
 * Sets up I2C1 interface and checks if AT24C02D is responding
 */
void init_eeprom();

/**
 * Save current palette index to EEPROM
 * @param palette_index Index of palette to save (0 to NUM_PALETTES-1)
 */
void save_palette_to_eeprom(uint8_t palette_index);

/**
 * Load saved palette index from EEPROM
 * @return Palette index (0 to NUM_PALETTES-1), or default if EEPROM unavailable/invalid
 */
uint8_t load_palette_from_eeprom();

/**
 * Save X offset to EEPROM
 * @param offset_x X offset value (-47 to +273)
 */
void save_offset_x_to_eeprom(int16_t offset_x);

/**
 * Load X offset from EEPROM
 * @return X offset value, or 0 if EEPROM unavailable/invalid
 */
int16_t load_offset_x_from_eeprom();

/**
 * Save Y offset to EEPROM
 * @param offset_y Y offset value (-2 to +238)
 */
void save_offset_y_to_eeprom(int16_t offset_y);

/**
 * Load Y offset from EEPROM
 * @return Y offset value, or 0 if EEPROM unavailable/invalid
 */
int16_t load_offset_y_from_eeprom();

/**
 * Save render mode to EEPROM
 * @param render_mode Render mode value (0=normal, 1=scanline)
 */
void save_render_mode_to_eeprom(uint8_t render_mode);

/**
 * Load render mode from EEPROM
 * @return Render mode value (0=normal, 1=scanline)
 */
uint8_t load_render_mode_from_eeprom();

#endif // VERSION_V1_1

#endif // EEPROM_H
