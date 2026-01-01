#pragma once

#include <stdint.h>
#include "config.h"
#include "displays/ili9341/ili9341.hpp"

#ifdef ENABLE_BW_DITHER
extern const uint16_t gb_colors[4];
#else
extern const uint16_t* gb_colors;
#endif

void set_active_palette_index(uint8_t index);
uint8_t get_active_palette_index();
uint8_t get_selected_palette_index();
uint8_t get_brightness_from_adc();

void apply_brightness(ili9341::ILI9341 &lcd, uint8_t brightness);
void display_logo(ili9341::ILI9341& lcd);
void display_test(ili9341::ILI9341& lcd, uint16_t* screenBuffer, uint16_t* scaledBuf, int* xmap, int* ymap);

void init_adc();
void init_gpio();
ili9341::Config init_lcd_config();

void update_hardware_controls(ili9341::ILI9341& lcd, uint16_t* scaled_buffer = nullptr, bool* show_osd = nullptr);
void draw_palette_osd(uint16_t* buffer, uint8_t palette_index, uint16_t fg_color, uint16_t bg_color);

#ifdef VERSION_V1_1
void init_eeprom();
void test_eeprom_save_load(ili9341::ILI9341& lcd);
void save_palette_to_eeprom(uint8_t palette_index);
uint8_t load_palette_from_eeprom();
uint8_t get_default_palette_index();
void apply_scanlines(uint16_t* buf, int w, int h, uint8_t mode, uint8_t intensity);
uint8_t get_render_mode();
void set_render_mode(uint8_t mode);
uint8_t get_scanline_intensity();
void set_scanline_intensity(uint8_t intensity);
#endif

void apply_dithering(uint16_t* scaledBuf);
void generate_test_pattern(uint16_t* screenBuffer, int pattern);
uint8_t get_blink_pin();