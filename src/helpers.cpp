#include "helpers.h"
#include "config.h"
#include "logo.h"
#include "scaler.hpp" 
#include "dither.hpp"
#include "palettes.hpp"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"

extern const uint16_t* gb_colors;

uint8_t get_selected_palette_index() {
    // Read ADC multiple times and average to reduce noise
    uint32_t adc_sum = 0;
    for (int i = 0; i < ADC_OVERSAMPLE_COUNT; i++) {
        adc_sum += adc_read();
        sleep_us(ADC_DELAY_US);
    }
    uint16_t adc_value = adc_sum / ADC_OVERSAMPLE_COUNT;
    
    uint8_t palette_index = (adc_value * NUM_PALETTES) / ADC_MAX_VALUE;
    
    if (palette_index >= NUM_PALETTES) {
        palette_index = NUM_PALETTES - 1;
    }
    
    return palette_index;
}

uint8_t get_brightness_from_adc() {
    uint16_t adc_value = adc_read();
    uint8_t brightness = BRIGHTNESS_MIN + ((adc_value * BRIGHTNESS_RANGE) / ADC_MAX_VALUE);
    return brightness;
}

void apply_brightness(ili9341::ILI9341 &lcd, uint8_t brightness) {
#ifdef VERSION_V1_0
    // Add initialization delay for v1.0 hardware before enabling backlight
    static bool first_brightness_call = true;
    if (first_brightness_call) {
        sleep_ms(LOGO_BRIGHTNESS_DELAY_MS);
        first_brightness_call = false;
    }
    
    if (brightness > BRIGHTNESS_THRESHOLD_V1_0) {
        gpio_put(PIN_BL, 1);
    } else {
        gpio_put(PIN_BL, 0);
    }
#else
    // Use driver's PWM/brightness control
    lcd.setBrightness(brightness);
#endif
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


void init_adc() {
#ifndef DISABLE_PALETTE_SELECTION
    adc_init();
    adc_gpio_init(PIN_PALETTE_ADC);
    gpio_pull_up(PIN_PALETTE_ADC);  // Internal pull-up for stability
    adc_select_input(3);
#endif
}

void init_gpio() {
#ifdef VERSION_V1_1a
    // Initialize mode switch GPIO (pull-down: LOW=palette, HIGH=brightness)
    gpio_init(PIN_MODE_SWITCH);
    gpio_set_dir(PIN_MODE_SWITCH, GPIO_IN);
    gpio_pull_down(PIN_MODE_SWITCH);
#endif

#ifdef VERSION_V1_0
    // For v1.0: Initialize backlight as GPIO, start OFF
    gpio_init(PIN_BL);
    gpio_set_dir(PIN_BL, GPIO_OUT);
    gpio_put(PIN_BL, 0);
#endif
}

ili9341::Config init_lcd_config() {
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
    
#ifdef VERSION_V1_0
    config.pin_bl = -1;  // Disable PWM control for v1.0, handle manually
#else
    config.pin_bl = PIN_BL;  // v1.1a uses PWM via driver
#endif
    
    config.rotation = DISPLAY_ROTATION;
    
    return config;
}

// Palette and Color Management
void update_hardware_controls(ili9341::ILI9341& lcd) {
#ifdef VERSION_V1_0
    // v1.0: Simple palette control via trimmer (if palette selection enabled)
    #if !defined(ENABLE_BW_DITHER) && !defined(DISABLE_PALETTE_SELECTION)
        static uint8_t last_palette_index = 0xFF;
        uint8_t current_palette_index = get_selected_palette_index();
        
        if (current_palette_index != last_palette_index) {
            gb_colors = PALETTE_LIST[current_palette_index];
            last_palette_index = current_palette_index;
        }
    #endif
#endif

#ifdef VERSION_V1_1a
    // v1.1a: Advanced controls with mode switch (palette vs brightness)
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
            apply_brightness(lcd, current_brightness);
            last_brightness_candidate = current_brightness;
        }
    }
#endif
}

// Frame Processing Functions
void scale_frame(const uint16_t* screenBuffer, uint16_t* scaledBuf, const int* xmap, const int* ymap) {
    if (DISPLAY_SCALE == 1) {
        memcpy(scaledBuf, screenBuffer, DMG_W * DMG_H * sizeof(uint16_t));
    } else {
        for (int dy = 0; dy < SCALED_H; dy++) {
            const uint16_t* srcRow = &screenBuffer[ymap[dy] * DMG_W];
            uint16_t* dstRow = &scaledBuf[dy * SCALED_W];
            
            int dx = 0;
            // Unroll loop for better performance
            for (; dx <= SCALED_W - SCALE_UNROLL_FACTOR; dx += SCALE_UNROLL_FACTOR) {
                dstRow[dx]     = srcRow[xmap[dx]];
                dstRow[dx + 1] = srcRow[xmap[dx + 1]];
                dstRow[dx + 2] = srcRow[xmap[dx + 2]];
                dstRow[dx + 3] = srcRow[xmap[dx + 3]];
                dstRow[dx + 4] = srcRow[xmap[dx + 4]];
                dstRow[dx + 5] = srcRow[xmap[dx + 5]];
                dstRow[dx + 6] = srcRow[xmap[dx + 6]];
                dstRow[dx + 7] = srcRow[xmap[dx + 7]];
            }
            // Handle remaining pixels
            for (; dx < SCALED_W; dx++) {
                dstRow[dx] = srcRow[xmap[dx]];
            }
        }
    }
}

void apply_dithering(uint16_t* scaledBuf) {
#ifdef ENABLE_BW_DITHER
    #ifdef DITHER_BEST
        floyd_steinberg_dither(scaledBuf, SCALED_W, SCALED_H, gb_colors, BW_WHITE, BW_BLACK);
    #else
        fast_bayer_dither(scaledBuf, SCALED_W, SCALED_H, gb_colors, BW_WHITE, BW_BLACK);
    #endif
#endif
}

// Test Pattern Generation
void generate_test_pattern(uint16_t* screenBuffer, int pattern) {
    // Standard RGB565 colors for test patterns
    static const uint16_t RED = 0xF800;      // Pure red
    static const uint16_t GREEN = 0x07E0;    // Pure green  
    static const uint16_t BLUE = 0x001F;     // Pure blue
    static const uint16_t YELLOW = 0xFFE0;   // Yellow (red + green)
    static const uint16_t WHITE = 0xFFFF;    // White
    static const uint16_t BLACK = 0x0000;    // Black
    
    bool use_palette = (pattern % 2 == 0);
    int base_pattern = pattern / 2;
    
    for (int y = 0; y < DMG_H; y++) {
        for (int x = 0; x < DMG_W; x++) {
            uint16_t color = BLACK;
            
            if (base_pattern == 0) {
                // Horizontal gradient
                if (use_palette) {
                    uint8_t pixel_value = (x * 4) / DMG_W;
                    if (pixel_value > 3) pixel_value = 3;
                    color = gb_colors[pixel_value];
                } else {
                    // RGB horizontal gradient
                    int stripe_width = DMG_W / 4;
                    if (x < stripe_width) color = RED;
                    else if (x < stripe_width * 2) color = GREEN;
                    else if (x < stripe_width * 3) color = BLUE;
                    else color = YELLOW;
                }
            } else if (base_pattern == 1) {
                // Vertical gradient/bars
                if (use_palette) {
                    uint8_t pixel_value = (y * 4) / DMG_H;
                    if (pixel_value > 3) pixel_value = 3;
                    color = gb_colors[pixel_value];
                } else {
                    // RGB vertical bars
                    int stripe_height = DMG_H / 4;
                    if (y < stripe_height) color = RED;
                    else if (y < stripe_height * 2) color = GREEN;
                    else if (y < stripe_height * 3) color = BLUE;
                    else color = YELLOW;
                }
            } else if (base_pattern == 2) {
                // Checkerboard
                if (use_palette) {
                    uint8_t pixel_value = ((x / 40) + (y / 36)) % 4;
                    if (pixel_value > 3) pixel_value = 3;
                    color = gb_colors[pixel_value];
                } else {
                    // RGB checkerboard
                    int block_size = 20;
                    bool checker = ((x / block_size) + (y / block_size)) % 2;
                    int color_index = ((x / block_size) + (y / block_size * 2)) % 4;
                    if (checker) {
                        if (color_index == 0) color = RED;
                        else if (color_index == 1) color = GREEN;
                        else if (color_index == 2) color = BLUE;
                        else color = YELLOW;
                    } else {
                        color = WHITE;
                    }
                }
            }
            
            screenBuffer[y * DMG_W + x] = color;
        }
    }
}

uint8_t get_blink_pin() {
#ifdef VERSION_V1_0
    return TEST_BLINK_IO_V1_0;
#else
    return TEST_BLINK_IO_V1_1A;
#endif
}

void display_test(ili9341::ILI9341& lcd, uint16_t* screenBuffer, uint16_t* scaledBuf, int* xmap, int* ymap) {
    uint8_t blink_pin = get_blink_pin();
    gpio_init(blink_pin);
    gpio_set_dir(blink_pin, GPIO_OUT);

    int pattern = 0;
    
    while (true) {
        // Check palette and brightness controls (only if not using BW dither)
        #ifndef ENABLE_BW_DITHER
            #ifdef VERSION_V1_1a
                // v1.1a: Mode switch controls palette vs brightness
                bool palette_mode = gpio_get(PIN_MODE_SWITCH);
                static uint8_t last_palette_index = 0xFF;
                static uint8_t last_brightness = 0xFF;
                
                if (palette_mode) {
                    // Palette selection mode
                    uint8_t current_palette_index = get_selected_palette_index();
                    if (current_palette_index != last_palette_index) {
                        gb_colors = PALETTE_LIST[current_palette_index];
                        last_palette_index = current_palette_index;
                    }
                } else {
                    // Brightness control mode
                    uint8_t current_brightness = get_brightness_from_adc();
                    if (current_brightness != last_brightness) {
                        apply_brightness(lcd, current_brightness);
                        last_brightness = current_brightness;
                    }
                }
            #else
                // v1.0: Direct palette control with trimmer (if enabled)
                #ifndef DISABLE_PALETTE_SELECTION
                    static uint8_t last_palette_index = 0xFF;
                    uint8_t current_palette_index = get_selected_palette_index();
                    
                    if (current_palette_index != last_palette_index) {
                        gb_colors = PALETTE_LIST[current_palette_index];
                        last_palette_index = current_palette_index;
                    }
                #endif
            #endif
        #endif
        
        generate_test_pattern(screenBuffer, pattern);
        
        scale_frame(screenBuffer, scaledBuf, xmap, ymap);
        
        apply_dithering(scaledBuf);
        
        lcd.clearScreen(FILL_COLOR);
        lcd.drawImage(X_OFF, Y_OFF, SCALED_W, SCALED_H, scaledBuf);
        
        gpio_put(blink_pin, pattern % 2);
        sleep_ms(TEST_PATTERN_DELAY_MS);
        
        pattern = (pattern + 1) % NUM_TEST_PATTERNS;
    }
}