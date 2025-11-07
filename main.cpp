#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/spi.h"
#include "hardware/watchdog.h"
#include "gblcd.pio.h"
#include "scaler.hpp"
#include "config.h"
#include "helpers.h"

int main() {
    stdio_init_all();
    
    init_adc();
    init_gpio();
    
    ili9341::ILI9341 lcd;
    ili9341::Config config = init_lcd_config();
    
    lcd.begin(config);

    display_logo(lcd);
    sleep_ms(LOGO_DISPLAY_DELAY_MS);

    apply_brightness(lcd, LCD_BRIGHTNESS);
    sleep_ms(LOGO_TOTAL_DELAY_MS);

    static uint16_t screenBuffer[DMG_W * DMG_H];
    static uint16_t scaledBuf[SCALED_W * SCALED_H];
    
    static int xmap[SCALED_W];
    static int ymap[SCALED_H];
    buildScaleMaps(xmap, ymap, DMG_W, DMG_H, SCALED_W, SCALED_H, DISPLAY_SCALE);

#ifdef ENABLE_DISPLAY_TEST
    display_test(lcd, screenBuffer, scaledBuf, xmap, ymap);
#else
    // PIO initialization
    PIO pio = pio0;
    uint state_machine_id = 0;
    uint offset = pio_add_program(pio, &gblcd_program);
    gblcd_program_init(pio, state_machine_id, offset, GB_PIN_BASE);

    int x = 0, y = 0;
    bool vSyncPrev = false;
    bool vSyncCurrent = false;
    bool vSyncFallingEdgeDetected = false;
    bool firstRun = false;
    uint16_t data0, data1, vSync;

    while (true) {
        uint32_t result = pio_sm_get_blocking(pio, state_machine_id);
        vSync = (result >> 31) & 1;

        vSyncCurrent = vSync;
        vSyncFallingEdgeDetected = (!vSyncCurrent && vSyncPrev);
        vSyncPrev = vSyncCurrent;

        if (!vSyncFallingEdgeDetected) {
            continue;
        }

        if (!firstRun) {
            firstRun = true;
            lcd.clearScreen(FILL_COLOR);
        }

        uint16_t* bufPtr = screenBuffer;
        
        for (y = 0; y < DMG_H; y++) {
            for (x = 0; x < DMG_W; x++) {
                if (x > 0 || y > 0) {
                    result = pio_sm_get_blocking(pio, state_machine_id);
                }
                
                data0 = (result >> 29) & 1;
                data1 = (result >> 30) & 1;
                
                uint8_t gb_pixel_value = (data1 << 1) | data0;
                
                *bufPtr++ = gb_colors[gb_pixel_value];
            }
        }

        scale_frame(screenBuffer, scaledBuf, xmap, ymap);
        apply_dithering(scaledBuf);
        
        lcd.drawImage(X_OFF, Y_OFF, SCALED_W, SCALED_H, scaledBuf);
        
        update_hardware_controls(lcd);
        
        vSyncFallingEdgeDetected = false;
    }
#endif
    
    return 0;
}
