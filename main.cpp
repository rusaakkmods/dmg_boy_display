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

//#define VERSION_V1_1a
#define VERSION_V1_0

// Configuration
#define ENABLE_DISPLAY_TEST
//#define ENABLE_BW_DITHER  // Uncomment for black & white dithering

// Dithering mode selection (only used if ENABLE_BW_DITHER is defined)
#define DITHER_FAST   // Bayer dithering (fastest)
//#define DITHER_BEST   // Floyd-Steinberg error diffusion (best quality)

// Hardware Pin Definitions - v1.1a
#ifdef VERSION_V1_1a
    #define SPI_CHANNEL     spi0
    #define PIN_MOSI        3
    #define PIN_SCK         6
    #define PIN_CS          5
    #define PIN_DC          4
    #define PIN_RESET       7
    #define PIN_BL          8
    #define PIN_PALETTE_ADC 29  // ADC3 - 10K potentiometer
    #define PIN_MODE_SWITCH 2   // Mode switch: LOW=brightness, HIGH=palette
#else // VERSION_V1_0
    #define SPI_CHANNEL spi1
    #define PIN_MOSI 11
    #define PIN_SCK 10
    #define PIN_CS 9
    #define PIN_DC 12
    #define PIN_RESET 13
    #define PIN_BL 8
#endif

// Game Boy LCD Specifications
#define DMG_W 160
#define DMG_H 144

// Display Configuration - ILI9341
#define LCD_W           320      
#define LCD_H           240      
#define DISPLAY_SCALE   1.6
#define DISPLAY_ROTATION ili9341::ROTATION_270
#define FILL_COLOR      ili9341::BLACK
#define X_OFF 49
#define Y_OFF 7
#define SCALED_W (int)(DMG_W * DISPLAY_SCALE + 0.5f)
#define SCALED_H (int)(DMG_H * DISPLAY_SCALE + 0.5f)

// LCD SPI Configuration
#define LCD_SPI_SPEED   (40 * 1000 * 1000)
#define LCD_DMA_BUFFER  2560
#define LCD_BRIGHTNESS  128  // 50% brightness

// Palette Configuration
static const uint16_t* const PALETTE_LIST[] = {
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
    PALETTE_ADVENTURER,
    PALETTE_MODERN
};

#define NUM_PALETTES (sizeof(PALETTE_LIST) / sizeof(PALETTE_LIST[0]))

// Dither palette setup
#ifdef ENABLE_BW_DITHER
    static const uint16_t BW_BLACK = 0x0000;
    static const uint16_t BW_WHITE = 0xFFFF;
    
    #ifdef DITHER_BEST
        static const uint16_t gb_colors[4] = {0xFFFF, 0xAAAA, 0x4444, 0x0000};
    #else
        static const uint16_t gb_colors[4] = {0xFFFF, 0x9999, 0x5555, 0x0000};
    #endif
#else
    static const uint16_t* gb_colors = PALETTE_MODERN;
#endif

uint8_t get_selected_palette_index() {
    uint16_t adc_value = adc_read();
    uint8_t palette_index = (adc_value * NUM_PALETTES) / 4096;
    
    if (palette_index >= NUM_PALETTES) {
        palette_index = NUM_PALETTES - 1;
    }
    
    return palette_index;
}

uint8_t get_brightness_from_adc() {
    uint16_t adc_value = adc_read();
    // Map 0-4095 to 5-128 (very dim to 50% max)
    uint8_t brightness = 5 + ((adc_value * 123) / 4096);
    return brightness;
}

void display_logo(ili9341::ILI9341& lcd) {
    static const uint16_t* logo = (uint16_t*)rMODS_logo_data;
    int logo_width = RMODS_LOGO_WIDTH;
    int logo_height = RMODS_LOGO_HEIGHT;
    int logo_x = (int)(X_OFF + (SCALED_W - logo_width) / 2);
    int logo_y = (int)(Y_OFF + (SCALED_H - logo_height) / 2);
    
    lcd.clearScreen(RMODS_LOGO_BACKGROUND);
    lcd.drawImage(logo_x, logo_y, logo_width, logo_height, logo);
}

void display_test(ili9341::ILI9341& lcd, uint16_t* screenBuffer, uint16_t* scaledBuf, int* xmap, int* ymap) {
    int blink_io = 2;
    #ifdef VERSION_V1_0
        blink_io = 7;
    #endif
    gpio_init(blink_io);
    gpio_set_dir(blink_io, GPIO_OUT);

    int pattern = 0;
    
    while (true) {
        // Check palette selection (only if not using BW dither)
        #ifdef VERSION_V1_1a
            #ifndef ENABLE_BW_DITHER
                static uint8_t last_palette_index = 0xFF;
                uint8_t current_palette_index = get_selected_palette_index();
                
                if (current_palette_index != last_palette_index) {
                    gb_colors = PALETTE_LIST[current_palette_index];
                    last_palette_index = current_palette_index;
                }
            #endif
        #endif
        
        // Generate test pattern in screenBuffer (160x144 Game Boy size)
        // Pattern 0: Horizontal gradient (all 4 shades)
        // Pattern 1: Vertical gradient
        // Pattern 2: Checkerboard
        
        for (int y = 0; y < DMG_H; y++) {
            for (int x = 0; x < DMG_W; x++) {
                uint8_t pixel_value = 0;
                
                if (pattern == 0) {
                    // Horizontal gradient
                    pixel_value = (x * 4) / DMG_W;
                } else if (pattern == 1) {
                    // Vertical gradient
                    pixel_value = (y * 4) / DMG_H;
                } else {
                    // Checkerboard
                    pixel_value = ((x / 40) + (y / 36)) % 4;
                }
                
                if (pixel_value > 3) pixel_value = 3;
                screenBuffer[y * DMG_W + x] = gb_colors[pixel_value];
            }
        }
        
        // Scale the pattern
        for (int dy = 0; dy < SCALED_H; dy++) {
            const uint16_t* srcRow = &screenBuffer[ymap[dy] * DMG_W];
            uint16_t* dstRow = &scaledBuf[dy * SCALED_W];
            
            for (int dx = 0; dx < SCALED_W; dx++) {
                dstRow[dx] = srcRow[xmap[dx]];
            }
        }
        
        // Apply dithering if enabled
        #ifdef ENABLE_BW_DITHER
            #ifdef DITHER_BEST
                floyd_steinberg_dither(scaledBuf, SCALED_W, SCALED_H, gb_colors, BW_WHITE, BW_BLACK);
            #else
                fast_bayer_dither(scaledBuf, SCALED_W, SCALED_H, gb_colors, BW_WHITE, BW_BLACK);
            #endif
        #endif
        
        // Draw to LCD
        lcd.clearScreen(FILL_COLOR);
        lcd.drawImage(X_OFF, Y_OFF, SCALED_W, SCALED_H, scaledBuf);
        
        gpio_put(blink_io, pattern % 2);
        sleep_ms(1000);
        
        pattern = (pattern + 1) % 3;
    }
}

int main() {
    stdio_init_all();
    
    #ifdef VERSION_V1_1a
        // Initialize mode switch GPIO (pull-down: LOW=palette, HIGH=brightness)
        gpio_init(PIN_MODE_SWITCH);
        gpio_set_dir(PIN_MODE_SWITCH, GPIO_IN);
        gpio_pull_down(PIN_MODE_SWITCH);
        
        // Initialize ADC for palette/brightness selection
        adc_init();
        adc_gpio_init(PIN_PALETTE_ADC);
        adc_select_input(3);
    #endif
    
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

    display_logo(lcd);
    sleep_ms(100); /// wait for a moment before setting brightness

    lcd.setBrightness(LCD_BRIGHTNESS);
    sleep_ms(900);

    // Buffer allocation
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
    gblcd_program_init(pio, state_machine_id, offset);

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
        
        // Apply dithering if enabled
        #ifdef ENABLE_BW_DITHER
            #ifdef DITHER_BEST
                floyd_steinberg_dither(scaledBuf, SCALED_W, SCALED_H, gb_colors, BW_WHITE, BW_BLACK);
            #else
                fast_bayer_dither(scaledBuf, SCALED_W, SCALED_H, gb_colors, BW_WHITE, BW_BLACK);
            #endif
        #endif
        
        lcd.drawImage(X_OFF, Y_OFF, SCALED_W, SCALED_H, scaledBuf);
        
        #ifdef VERSION_V1_1a

            // Check mode switch: LOW=brightness control, HIGH=palette selection
            bool palette_mode = gpio_get(PIN_MODE_SWITCH);
            
            #ifndef ENABLE_BW_DITHER
                static uint8_t last_palette_index = 0xFF;
                static uint8_t last_palette_candidate = 0xFF;
                static bool was_in_palette_mode = false;
            #endif
            
            static uint8_t last_brightness_candidate = 0xFF;
            static bool was_in_brightness_mode = false;
            
            if (palette_mode) {
                // Palette selection mode (only if not using BW dither)
                #ifndef ENABLE_BW_DITHER
                    uint8_t current_palette_index = get_selected_palette_index();
                    
                    // If we just entered palette mode, store current candidate
                    if (!was_in_palette_mode) {
                        last_palette_candidate = current_palette_index;
                        was_in_palette_mode = true;
                    }
                    
                    // Only change palette if pot value has changed while in palette mode
                    if (current_palette_index != last_palette_candidate) {
                        gb_colors = PALETTE_LIST[current_palette_index];
                        last_palette_index = current_palette_index;
                        last_palette_candidate = current_palette_index;
                    }
                #endif
                
                was_in_brightness_mode = false;
            } else {
                // Brightness control mode
                #ifndef ENABLE_BW_DITHER
                    was_in_palette_mode = false;
                #endif
                
                uint8_t current_brightness = get_brightness_from_adc();
                
                // If we just entered brightness mode, store current candidate
                if (!was_in_brightness_mode) {
                    last_brightness_candidate = current_brightness;
                    was_in_brightness_mode = true;
                }
                
                // Only change brightness if pot value has changed while in brightness mode
                if (current_brightness != last_brightness_candidate) {
                    lcd.setBrightness(current_brightness);
                    last_brightness_candidate = current_brightness;
                }
            }
        #endif
        
        vSyncFallingEdgeDetected = false;
    }
#endif
    
    return 0;
}
