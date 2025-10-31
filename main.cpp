#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/spi.h"
#include "hardware/adc.h"
#include "hardware/watchdog.h"
#include "gblcd.pio.h"
#include "logo.h"
#include "scaler.hpp"
#include "dither.hpp"
#include "palettes.hpp"
#include "displays/ili9341/ili9341.hpp"

#define FIRMWARE_VERSION "v1.1a"

// Configuration
#define ENABLE_DISPLAY_TEST

// Hardware Pin Definitions - v1.1a
#define SPI_CHANNEL     spi0
#define PIN_MOSI        3
#define PIN_SCK         6
#define PIN_CS          5
#define PIN_DC          4
#define PIN_RESET       7
#define PIN_BL          8
#define PIN_PALETTE_ADC 29  // ADC3 - 10K potentiometer

// Game Boy LCD Specifications
#define DMG_W 160
#define DMG_H 144

// Display Configuration - ILI9341
#define LCD_W           320      
#define LCD_H           240      
#define DISPLAY_SCALE   1.6
#define DISPLAY_ROTATION ili9341::ROTATION_270
#define FILL_COLOR      ili9341::BLACK
#define X_OFF 48
#define Y_OFF 9
#define SCALED_W (int)(DMG_W * DISPLAY_SCALE + 0.5f)
#define SCALED_H (int)(DMG_H * DISPLAY_SCALE + 0.5f)

// LCD SPI Configuration
#define LCD_SPI_SPEED   (40 * 1000 * 1000)
#define LCD_DMA_BUFFER  2560
#define LCD_BRIGHTNESS  255

// Palette Configuration
#define DEFAUT_PALETTE PALETTE_MODERN

static const uint16_t* const PALETTE_LIST[] = {
    PALETTE_MODERN,
    PALETTE_GRAYSCALE,
    PALETTE_GRAYSCALE_INVERT,
    PALETTE_GREEN_SHADES,
    PALETTE_YELLOW_SHADES,
    PALETTE_TEAL_SHADES,
    PALETTE_RED_PASTEL_SHADES,
    PALETTE_GRAY_SHADES,
    PALETTE_RETRO,
    PALETTE_ROMANCE,
    PALETTE_MODERN2,
    PALETTE_PEACH,
    PALETTE_NEON,
    PALETTE_HIGHLIGHT_BLUE,
    PALETTE_BLUE_HUE,
    PALETTE_VINTAGE,
    PALETTE_CLOUDY,
    PALETTE_LCD,
    PALETTE_SGB,
    PALETTE_ADVENTURER
};

#define NUM_PALETTES (sizeof(PALETTE_LIST) / sizeof(PALETTE_LIST[0]))

static const uint16_t* gb_colors = DEFAUT_PALETTE;

uint8_t get_selected_palette_index() {
    uint16_t adc_value = adc_read();
    uint8_t palette_index = (adc_value * NUM_PALETTES) / 4096;
    if (palette_index >= NUM_PALETTES) {
        palette_index = NUM_PALETTES - 1;
    }
    return palette_index;
}

int main() {
    stdio_init_all();
    
    // Initialize ADC for palette selection
    adc_init();
    adc_gpio_init(PIN_PALETTE_ADC);
    adc_select_input(3);
    
    // Display initialization
    ili9341::ILI9341 lcd;
    ili9341::Config config;
    
    config.spi_speed_hz = LCD_SPI_SPEED;
    config.dma.buffer_size = LCD_DMA_BUFFER;
    config.dma.enabled = true;
    
    config.width = LCD_W;
    config.spi_inst = SPI_CHANNEL;
    config.pin_din = PIN_MOSI;
    config.pin_sck = PIN_SCK;
    config.pin_cs = PIN_CS;
    config.pin_dc = PIN_DC;
    config.pin_reset = PIN_RESET;
    config.pin_bl = PIN_BL;
    config.rotation = DISPLAY_ROTATION;
    
    lcd.begin(config);
    lcd.setRotation(config.rotation);

    // Display logo
    static const uint16_t* logo = (uint16_t*)rMODS_logo_data;
    int logo_width = RMODS_LOGO_WIDTH;
    int logo_height = RMODS_LOGO_HEIGHT;
    int logo_x = (int)(X_OFF + (SCALED_W - logo_width) / 2);
    int logo_y = (int)(Y_OFF + (SCALED_H - logo_height) / 2);
    lcd.clearScreen(RMODS_LOGO_BACKGROUND);
    lcd.drawImage(logo_x, logo_y, logo_width, logo_height, logo);
    sleep_ms(100);
    lcd.setBrightness(LCD_BRIGHTNESS);
    sleep_ms(900);

#ifdef ENABLE_DISPLAY_TEST
    gpio_init(3);
    gpio_set_dir(3, GPIO_OUT);

    uint16_t test_red = 0xF800;
    uint16_t test_green = 0x07E0;
    uint16_t test_blue = 0x001F;
    uint16_t test_yellow = 0xFFE0;
    int i = 1;
    while (true) {
        lcd.clearScreen(RMODS_LOGO_BACKGROUND);
        
        int h = 8*i;
        int w = 12*i;
        lcd.fillRect(0, 0, h, w, test_red);
        lcd.fillRect(80, 0,  h, w, test_green);  
        lcd.fillRect(160, 0,  h, w, test_blue);
        lcd.fillRect(240, 0,  h, w, test_yellow);
        gpio_put(3,1);
        sleep_ms(500);
        
        lcd.fillRect(0, 120,  h, w, gb_colors[0]);
        lcd.fillRect(80, 120,  h, w, gb_colors[1]);
        lcd.fillRect(160, 120,  h, w, gb_colors[2]);
        lcd.fillRect(240, 120,  h, w, gb_colors[3]);
        gpio_put(3,0);
        sleep_ms(500);

        if (i>= 10) {
            i = 1;
        } else {
            i++;
        }
    }
#else
    // PIO initialization
    PIO pio = pio0;
    uint state_machine_id = 0;
    uint offset = pio_add_program(pio, &gblcd_program);
    gblcd_program_init(pio, state_machine_id, offset);

    // Buffer allocation
    static uint16_t screenBuffer[DMG_W * DMG_H];
    static uint16_t scaledBuf[SCALED_W * SCALED_H];
    
    static int xmap[SCALED_W];
    static int ymap[SCALED_H];
    buildScaleMaps(xmap, ymap, DMG_W, DMG_H, SCALED_W, SCALED_H, DISPLAY_SCALE);

    // Main loop variables
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

        // Palette selection
        static uint8_t last_palette_index = 0xFF;
        uint8_t current_palette_index = get_selected_palette_index();
        
        if (current_palette_index != last_palette_index) {
            gb_colors = PALETTE_LIST[current_palette_index];
            last_palette_index = current_palette_index;
        }

        // Capture Game Boy frame
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

        // Scale frame buffer
        if (DISPLAY_SCALE == 1) {
            memcpy(scaledBuf, screenBuffer, DMG_W * DMG_H * sizeof(uint16_t));
        }
        else {
            for (int dy = 0; dy < SCALED_H; dy++) {
                const uint16_t* srcRow = &screenBuffer[ymap[dy] * DMG_W];
                uint16_t* dstRow = &scaledBuf[dy * SCALED_W];
                
                int dx = 0;
                for (; dx <= SCALED_W - 8; dx += 8) {
                    dstRow[dx]     = srcRow[xmap[dx]];
                    dstRow[dx + 1] = srcRow[xmap[dx + 1]];
                    dstRow[dx + 2] = srcRow[xmap[dx + 2]];
                    dstRow[dx + 3] = srcRow[xmap[dx + 3]];
                    dstRow[dx + 4] = srcRow[xmap[dx + 4]];
                    dstRow[dx + 5] = srcRow[xmap[dx + 5]];
                    dstRow[dx + 6] = srcRow[xmap[dx + 6]];
                    dstRow[dx + 7] = srcRow[xmap[dx + 7]];
                }
                for (; dx < SCALED_W; dx++) {
                    dstRow[dx] = srcRow[xmap[dx]];
                }
            }
        }
        
        lcd.drawImage(X_OFF, Y_OFF, SCALED_W, SCALED_H, scaledBuf);
        vSyncFallingEdgeDetected = false;
    }
#endif
    
    return 0;
}
