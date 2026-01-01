#pragma once

#include <stdint.h>
#include "config.h"
#include "displays/ili9341/ili9341.hpp"

// Forward declarations
#ifdef ENABLE_BW_DITHER
extern const uint16_t gb_colors[4];
#else
extern const uint16_t* gb_colors;
#endif

/**
 * ADC and Input Functions
 */

/**
 * Set the currently active palette index (tracks what's actually being displayed)
 */
void set_active_palette_index(uint8_t index);

/**
 * Get the currently active palette index
 */
uint8_t get_active_palette_index();

/**
 * Get the selected palette index from ADC reading
 * Uses oversampling to reduce noise
 * @return Palette index (0 to NUM_PALETTES-1)
 */
uint8_t get_selected_palette_index();

/**
 * Get brightness value from ADC reading
 * Maps ADC range to brightness range
 * @return Brightness value (BRIGHTNESS_MIN to BRIGHTNESS_MAX)
 */
uint8_t get_brightness_from_adc();

/**
 * Display Control Functions
 */

/**
 * Apply brightness control - handles both v1.0 and v1.1 hardware differences
 * @param lcd LCD display instance
 * @param brightness Brightness value to apply
 */
void apply_brightness(ili9341::ILI9341 &lcd, uint8_t brightness);

/**
 * Display the logo screen
 * @param lcd LCD display instance
 */
void display_logo(ili9341::ILI9341& lcd);

/**
 * Display test patterns (for ENABLE_DISPLAY_TEST mode)
 * @param lcd LCD display instance
 * @param screenBuffer Game Boy sized buffer (160x144)
 * @param scaledBuf Scaled display buffer
 * @param xmap X-axis scaling map
 * @param ymap Y-axis scaling map
 */
void display_test(ili9341::ILI9341& lcd, uint16_t* screenBuffer, uint16_t* scaledBuf, int* xmap, int* ymap);

/**
 * Hardware Initialization Functions  
 */

/**
 * Initialize ADC for palette/brightness selection
 */
void init_adc();

/**
 * Initialize GPIO pins for the specific hardware version
 */
void init_gpio();

/**
 * Initialize and configure the LCD display
 * @param lcd LCD display instance
 * @return Configured LCD config structure
 */
ili9341::Config init_lcd_config();

/**
 * Palette and Color Management
 */

/**
 * Update hardware controls (palette and brightness) for both hardware versions
 * v1.0: Simple palette control via trimmer
 * v1.1: Advanced mode-switched palette/brightness control with OSD support
 * @param lcd LCD display instance
 * @param scaled_buffer Optional buffer to draw OSD on (for v1.1 palette mode)
 * @param show_osd Optional pointer to flag indicating whether OSD should be shown (v1.1 only)
 */
void update_hardware_controls(ili9341::ILI9341& lcd, uint16_t* scaled_buffer = nullptr, bool* show_osd = nullptr);

/**
 * Draw OSD (On-Screen Display) showing palette name
 * @param buffer 16-bit RGB565 frame buffer (256x230)
 * @param palette_index Index of currently selected palette
 * @param fg_color Foreground color (RGB565)
 * @param bg_color Background color (RGB565)
 */
void draw_palette_osd(uint16_t* buffer, uint8_t palette_index, uint16_t fg_color, uint16_t bg_color);

#ifdef VERSION_V1_1
/**
 * I2C EEPROM Storage Functions (v1.1 only - AT24C02)
 */

/**
 * Initialize I2C EEPROM (AT24C02) on GPIO14/GPIO15
 */
void init_eeprom();

/**
 * Test EEPROM save/load functionality with visual feedback
 * Shows green screen if successful, red if failed
 */
void test_eeprom_save_load(ili9341::ILI9341& lcd);

/**
 * Save the current palette index to EEPROM
 * Only writes if the palette has changed from what's currently saved
 * @param palette_index The palette index to save (0 to NUM_PALETTES-1)
 */
void save_palette_to_eeprom(uint8_t palette_index);

/**
 * Load the saved palette index from EEPROM
 * @return Saved palette index, or DEFAULT_PALETTE_INDEX if not found/invalid
 */
uint8_t load_palette_from_eeprom();

/**
 * Get the default palette index based on DEFAULT_PALETTE_NAME
 * @return Index of the default palette
 */
uint8_t get_default_palette_index();
#endif // VERSION_V1_1

/**
 * Frame Processing Functions
 */

/**
 * Scale a frame from Game Boy resolution to display resolution
 * @param screenBuffer Source buffer (160x144)
 * @param scaledBuf Destination buffer (scaled size)
 * @param xmap X-axis scaling map
 * @param ymap Y-axis scaling map
 */
void scale_frame(const uint16_t* screenBuffer, uint16_t* scaledBuf, const int* xmap, const int* ymap);

/**
 * Apply dithering to the scaled buffer if enabled
 * @param scaledBuf Buffer to apply dithering to
 */
void apply_dithering(uint16_t* scaledBuf);

/**
 * Test Pattern Generation
 */

/**
 * Generate a test pattern in the screen buffer
 * @param screenBuffer Buffer to fill with test pattern
 * @param pattern Pattern type (0=horizontal gradient, 1=vertical gradient, 2=checkerboard)
 */
void generate_test_pattern(uint16_t* screenBuffer, int pattern);

/**
 * Utility Functions
 */

/**
 * Get the blink GPIO pin for the current hardware version
 * @return GPIO pin number for status LED
 */
uint8_t get_blink_pin();

/**
 * Apply scanline effect to buffer (CRT-style darkening of alternate lines)
 * @param buf Buffer containing RGB565 pixels
 * @param w Width of buffer
 * @param h Height of buffer
 * @param mode 1=horizontal scanlines, 2=crosshatch (h+v)
 * @param intensity Darkening intensity (0-255, 128=50%)
 */
void apply_scanlines(uint16_t* buf, int w, int h, uint8_t mode, uint8_t intensity);

/**
 * Get current render mode
 * @return 0=normal, 1=scanline
 */
uint8_t get_render_mode();

/**
 * Set render mode
 * @param mode 0=normal, 1=scanline
 */
void set_render_mode(uint8_t mode);