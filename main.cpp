#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "gblcd.pio.h"
#include "scaler.hpp"
#include "config.h"
#include "helpers.h"
#include "eeprom.h"

static uint16_t screenBuffer[DMG_W * DMG_H];
static uint16_t scaledBuf[SCALED_W * SCALED_H];
static int xmap[SCALED_W];
static int ymap[SCALED_H];

static inline bool validate_offset(int16_t saved, int16_t base, int16_t max_bound) {
    return (saved != 0) && (abs(saved - base) <= 10) && (saved >= 0) && (saved <= max_bound);
}

static void init_hardware(ili9341::ILI9341& lcd) {
#ifdef ENABLE_DISPLAY_TEST
    stdio_init_all();
    for (int i = 5; i > 0; i--) {
        printf("Starting in %d...\n", i);
        sleep_ms(1000);
    }
#endif
    
    init_adc();
    init_gpio();
    
#ifdef VERSION_V1_1
    init_eeprom();
#endif
    
    ili9341::Config config = init_lcd_config();
    lcd.begin(config);
    lcd.setRotation(config.rotation);
    
    display_logo(lcd);
    sleep_ms(LOGO_DISPLAY_DELAY_MS);
    apply_brightness(lcd, LCD_BRIGHTNESS);
    sleep_ms(LOGO_TOTAL_DELAY_MS);
    
    buildScaleMaps(xmap, ymap, DMG_W, DMG_H, SCALED_W, SCALED_H, DISPLAY_SCALE);
}

static void load_saved_settings() {
#if defined(VERSION_V1_1) && !defined(ENABLE_BW_DITHER)
    uint8_t saved_palette = load_palette_from_eeprom();
    gb_colors = PALETTE_LIST[saved_palette];
    set_active_palette_index(saved_palette);
    save_palette_to_eeprom(saved_palette);
#endif

#ifdef VERSION_V1_1
    int16_t saved_x = load_offset_x_from_eeprom();
    int16_t saved_y = load_offset_y_from_eeprom();
    
    X_OFF = validate_offset(saved_x, X_OFF_BASE, LCD_W - SCALED_W) ? saved_x : X_OFF_BASE;
    Y_OFF = validate_offset(saved_y, Y_OFF_BASE, LCD_H - SCALED_H) ? saved_y : Y_OFF_BASE;
    
    set_render_mode(load_render_mode_from_eeprom());
#endif
}

static void capture_frame(PIO pio, uint sm) {
    uint16_t* bufPtr = screenBuffer;
    
    for (int i = 0; i < DMG_W * DMG_H; i++) {
        uint32_t result = pio_sm_get_blocking(pio, sm);
        uint8_t pixel = ((result >> 30) & 1) << 1 | ((result >> 29) & 1);
        *bufPtr++ = gb_colors[pixel];
    }
}

static void process_and_render(ili9341::ILI9341& lcd) {
    scale_frame(screenBuffer, scaledBuf, xmap, ymap, DMG_W, SCALED_W, SCALED_H);
    apply_dithering(scaledBuf);
    
#ifdef VERSION_V1_1
    static int16_t last_x = -1, last_y = -1;
    static bool show_osd = false;
    
    uint8_t render = get_render_mode();
    if (render > 0) {
        apply_scanlines(scaledBuf, SCALED_W, SCALED_H, render, get_scanline_intensity());
    }
    
    update_hardware_controls(lcd, scaledBuf, &show_osd);
    
    if (last_x < 0 || last_y < 0) {
        last_x = X_OFF;
        last_y = Y_OFF;
    } else if (X_OFF != last_x || Y_OFF != last_y) {
        lcd.clearScreen(0x0000);
        last_x = X_OFF;
        last_y = Y_OFF;
    }
#else
    update_hardware_controls(lcd);
#endif
    
    lcd.drawImage(X_OFF, Y_OFF, SCALED_W, SCALED_H, scaledBuf);
}

int main() {
    ili9341::ILI9341 lcd;
    
    init_hardware(lcd);
    load_saved_settings();

#ifdef ENABLE_DISPLAY_TEST
    display_test(lcd, screenBuffer, scaledBuf, xmap, ymap);
#else
    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &gblcd_program);
    gblcd_program_init(pio, sm, offset, GB_PIN_BASE);

    bool vsync_prev = false;
    bool first_frame = false;

    while (true) {
        uint32_t result = pio_sm_get_blocking(pio, sm);
        bool vsync = (result >> 31) & 1;
        bool vsync_falling = !vsync && vsync_prev;
        vsync_prev = vsync;

        if (!vsync_falling) continue;

        if (!first_frame) {
            first_frame = true;
            lcd.clearScreen(FILL_COLOR);
        }

        capture_frame(pio, sm);
        process_and_render(lcd);
    }
#endif
    
    return 0;
}
