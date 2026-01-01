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

#endif // VERSION_V1_1

#endif // EEPROM_H
