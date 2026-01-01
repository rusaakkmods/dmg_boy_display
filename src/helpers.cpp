#include "helpers.h"
#include "config.h"
#include "logo.h"
#include "scaler.hpp" 
#include "dither.hpp"
#include "palettes.hpp"
#include "font5x7.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#ifdef VERSION_V1_1
#include "hardware/i2c.h"
#include "eeprom.h"
#endif

// Runtime offset variables
int16_t X_OFF = X_OFF_DEFAULT;
int16_t Y_OFF = Y_OFF_DEFAULT;

#ifndef ENABLE_BW_DITHER
extern const uint16_t* gb_colors;
#endif

// Track current palette index globally so we know what's actually being used
static uint8_t current_active_palette_index = 0xFF;

void set_active_palette_index(uint8_t index) {
    current_active_palette_index = index;
}

uint8_t get_active_palette_index() {
    return current_active_palette_index;
}

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

void draw_palette_osd(uint16_t* buffer, uint8_t palette_index, uint16_t fg_color, uint16_t bg_color) {
    if (palette_index >= NUM_PALETTES) return;
    
    const char* palette_name = PALETTE_NAMES[palette_index];
    const char prefix[] = "Palette: ";
    const int char_width = 6;  // 5 pixels + 1 spacing
    const int char_height = 8; // 7 pixels + 1 spacing
    const int padding = 2;
    const int start_x = 4;
    const int start_y = 4;
    
    int text_len = strlen(prefix) + strlen(palette_name);
    int box_width = text_len * char_width + padding * 2;
    int box_height = char_height + padding * 2;
    
    // Draw background box
    for (int by = 0; by < box_height; by++) {
        for (int bx = 0; bx < box_width; bx++) {
            int px = start_x + bx;
            int py = start_y + by;
            if (px < SCALED_W && py < SCALED_H) {
                buffer[py * SCALED_W + px] = bg_color;
            }
        }
    }
    
    // Draw text
    int cursor_x = start_x + padding;
    int cursor_y = start_y + padding;
    
    // Draw "Palette: "
    for (int i = 0; prefix[i] != '\0'; i++) {
        char c = prefix[i];
        uint8_t glyph_idx = get_font_index(c);
        const uint8_t* glyph = font5x7[glyph_idx];
        for (int cx = 0; cx < 5; cx++) {
            for (int cy = 0; cy < 7; cy++) {
                if (glyph[cx] & (1 << cy)) {
                    int px = cursor_x + cx;
                    int py = cursor_y + cy;
                    if (px < SCALED_W && py < SCALED_H) {
                        buffer[py * SCALED_W + px] = fg_color;
                    }
                }
            }
        }
        cursor_x += char_width;
    }
    
    // Draw palette name
    for (int i = 0; palette_name[i] != '\0'; i++) {
        char c = palette_name[i];
        uint8_t glyph_idx = get_font_index(c);
        const uint8_t* glyph = font5x7[glyph_idx];
        for (int cx = 0; cx < 5; cx++) {
            for (int cy = 0; cy < 7; cy++) {
                if (glyph[cx] & (1 << cy)) {
                    int px = cursor_x + cx;
                    int py = cursor_y + cy;
                    if (px < SCALED_W && py < SCALED_H) {
                        buffer[py * SCALED_W + px] = fg_color;
                    }
                }
            }
        }
        cursor_x += char_width;
    }
}

#ifdef VERSION_V1_1

// Control modes enum
enum ControlMode {
    MODE_BRIGHTNESS = 0,
    MODE_PALETTE,
    MODE_OFFSET_X,
    MODE_OFFSET_Y,
    MODE_RENDER,
    MODE_COUNT
};

// Track values when entering each mode
static uint8_t palette_on_mode_entry = 0;
static int16_t offset_x_on_mode_entry = X_OFF_BASE;
static int16_t offset_y_on_mode_entry = Y_OFF_BASE;
static uint8_t render_on_mode_entry = 0;

// Track ADC position on mode entry for soft-takeover
static uint16_t adc_on_palette_entry = 0;
static uint16_t adc_on_offset_x_entry = 0;
static uint16_t adc_on_offset_y_entry = 0;
static uint16_t adc_on_render_entry = 0;
static bool palette_pot_moved = false;
static bool offset_x_pot_moved = false;
static bool offset_y_pot_moved = false;
static bool render_pot_moved = false;

// Render mode storage
static uint8_t last_render_mode = 0;  // 0 = normal, 1 = scanline

// Current control mode
static ControlMode current_mode = MODE_BRIGHTNESS;

// Double-click detection for mode switch
static bool last_switch_state = false;
static uint32_t first_click_time = 0;
static uint32_t click_count = 0;
static uint32_t last_mode_change_time = 0;

const uint32_t DOUBLE_CLICK_WINDOW_MS = 500;  // 500ms window for double-click
const uint32_t MODE_TIMEOUT_MS = 3000; // 3 second timeout
const int ADC_THRESHOLD = 100;  // Soft-takeover threshold

static const char* RENDER_MODE_NAMES[] = {"normal", "scanline_h", "scanline_x"};

// Helper function to map ADC value to offset range
static int16_t map_adc_to_offset(uint16_t adc_val, int16_t min_val, int16_t max_val) {
    return min_val + ((adc_val * (max_val - min_val + 1)) / 4096);
}

// Helper function to draw OSD with mode-specific text
static void draw_mode_osd(uint16_t* buffer, ControlMode mode, const char* value_text, uint16_t fg_color, uint16_t bg_color, int pos_x = 4, int pos_y = 4) {
    const char* mode_prefix = nullptr;
    
    // Special case: if value_text is "settings saved", show no prefix
    bool show_prefix = true;
    if (strcmp(value_text, "settings saved") == 0) {
        show_prefix = false;
        mode_prefix = "";
    } else {
        switch (mode) {
            case MODE_PALETTE:
                mode_prefix = "palette:";
                break;
            case MODE_OFFSET_X:
                mode_prefix = "offset-x:";
                break;
            case MODE_OFFSET_Y:
                mode_prefix = "offset-y:";
                break;
            case MODE_RENDER:
                mode_prefix = "render:";
                break;
            default:
                return;
        }
    }
    
    const int char_width = 6;
    const int char_height = 7;
    const int padding = 2;
    
    // Calculate text dimensions
    int prefix_len = 0;
    for (int i = 0; mode_prefix[i] != '\0'; i++) prefix_len++;
    
    int value_len = 0;
    for (int i = 0; value_text[i] != '\0'; i++) value_len++;
    
    int total_chars = prefix_len + value_len;
    int box_width = total_chars * char_width + padding * 2;
    int box_height = char_height + padding * 2;
    
    int start_x = pos_x;
    int start_y = pos_y;
    
    // Draw background box
    for (int dy = 0; dy < box_height; dy++) {
        for (int dx = 0; dx < box_width; dx++) {
            int px = start_x + dx;
            int py = start_y + dy;
            if (px < SCALED_W && py < SCALED_H) {
                buffer[py * SCALED_W + px] = bg_color;
            }
        }
    }
    
    // Draw text
    int cursor_x = start_x + padding;
    int cursor_y = start_y + padding;
    
    // Draw mode prefix
    for (int i = 0; mode_prefix[i] != '\0'; i++) {
        char c = mode_prefix[i];
        uint8_t glyph_idx = get_font_index(c);
        const uint8_t* glyph = font5x7[glyph_idx];
        for (int cx = 0; cx < 5; cx++) {
            for (int cy = 0; cy < 7; cy++) {
                if (glyph[cx] & (1 << cy)) {
                    int px = cursor_x + cx;
                    int py = cursor_y + cy;
                    if (px < SCALED_W && py < SCALED_H) {
                        buffer[py * SCALED_W + px] = fg_color;
                    }
                }
            }
        }
        cursor_x += char_width;
    }
    
    // Draw value
    for (int i = 0; value_text[i] != '\0'; i++) {
        char c = value_text[i];
        uint8_t glyph_idx = get_font_index(c);
        const uint8_t* glyph = font5x7[glyph_idx];
        for (int cx = 0; cx < 5; cx++) {
            for (int cy = 0; cy < 7; cy++) {
                if (glyph[cx] & (1 << cy)) {
                    int px = cursor_x + cx;
                    int py = cursor_y + cy;
                    if (px < SCALED_W && py < SCALED_H) {
                        buffer[py * SCALED_W + px] = fg_color;
                    }
                }
            }
        }
        cursor_x += char_width;
    }
}

#endif

void apply_brightness(ili9341::ILI9341 &lcd, uint8_t brightness) {
#ifdef VERSION_V1_0
    static bool first_brightness_call = true;
    if (first_brightness_call) {
        sleep_ms(LOGO_BRIGHTNESS_DELAY_MS);
        first_brightness_call = false;
    }
    gpio_put(PIN_BL, brightness > BRIGHTNESS_THRESHOLD_V1_0 ? 1 : 0);
#else
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
    gpio_pull_up(PIN_PALETTE_ADC);
    adc_select_input(3);
#endif
}

void init_gpio() {
#ifdef VERSION_V1_1
    gpio_init(PIN_MODE_SWITCH);
    gpio_set_dir(PIN_MODE_SWITCH, GPIO_IN);
    gpio_pull_down(PIN_MODE_SWITCH);
#endif

#ifdef VERSION_V1_0
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
    config.pin_bl = PIN_BL;  // v1.1 uses PWM via driver
#endif
    
    config.rotation = DISPLAY_ROTATION;
    
    return config;
}

#ifdef VERSION_V1_1
uint8_t get_render_mode() {
    return last_render_mode;
}

void set_render_mode(uint8_t mode) {
    if (mode <= 2) {
        last_render_mode = mode;
    }
}

// Apply scanline effect (CRT-style darkening)
// mode: 1=horizontal lines, 2=crosshatch (horizontal+vertical)
void apply_scanlines(uint16_t* buf, int w, int h, uint8_t mode, uint8_t intensity) {
    if (mode == 1) {
        // Horizontal scanlines only - darken every other horizontal line (starting at y=1)
        for (int y = 1; y < h; y += 2) {
            for (int x = 0; x < w; x++) {
                int idx = y * w + x;
                uint16_t pixel = buf[idx];
                
                // Extract RGB565 components
                uint8_t r = (pixel >> 11) & 0x1F;  // 5 bits
                uint8_t g = (pixel >> 5) & 0x3F;   // 6 bits
                uint8_t b = pixel & 0x1F;          // 5 bits
                
                // Apply darkening based on intensity (0-255 maps to 0-100% darkening)
                r = (r * (255 - intensity)) / 255;
                g = (g * (255 - intensity)) / 255;
                b = (b * (255 - intensity)) / 255;
                
                // Recombine to RGB565
                buf[idx] = (r << 11) | (g << 5) | b;
            }
        }
    } else if (mode == 2) {
        // Crosshatch pattern - darken both horizontal and vertical lines
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                // Darken if on odd row OR odd column
                if ((y % 2 == 1) || (x % 2 == 1)) {
                    int idx = y * w + x;
                    uint16_t pixel = buf[idx];
                    
                    // Extract RGB565 components
                    uint8_t r = (pixel >> 11) & 0x1F;  // 5 bits
                    uint8_t g = (pixel >> 5) & 0x3F;   // 6 bits
                    uint8_t b = pixel & 0x1F;          // 5 bits
                    
                    // Apply darkening based on intensity
                    r = (r * (255 - intensity)) / 255;
                    g = (g * (255 - intensity)) / 255;
                    b = (b * (255 - intensity)) / 255;
                    
                    // Recombine to RGB565
                    buf[idx] = (r << 11) | (g << 5) | b;
                }
            }
        }
    }
}
#endif // VERSION_V1_1

// Palette and Color Management
void update_hardware_controls(ili9341::ILI9341& lcd, uint16_t* scaled_buffer, bool* show_osd) {
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

#ifdef VERSION_V1_1
    // v1.1: Multi-mode controls (brightness → palette → offset-x → offset-y)
    static uint8_t last_palette_index = get_active_palette_index();
    static uint8_t last_brightness_value = 0xFF;
    static int16_t last_offset_x_value = X_OFF;
    static int16_t last_offset_y_value = Y_OFF;
    
    uint32_t now = to_ms_since_boot(get_absolute_time());
    
    // Detect button click (press and release cycle)
    bool current_switch_state = gpio_get(PIN_MODE_SWITCH);
    
    // Detect falling edge (button release = complete click)
    if (!current_switch_state && last_switch_state) {
        // Button released - count as one click
        if (click_count == 0) {
            // First click
            click_count = 1;
            first_click_time = now;
        } else if (click_count == 1 && (now - first_click_time < DOUBLE_CLICK_WINDOW_MS)) {
            // Second click within window - double-click detected!
            click_count = 0;
            last_mode_change_time = now;
            
            // Cycle modes: brightness → palette → offset-x → offset-y → palette (loop)
            if (current_mode == MODE_BRIGHTNESS) {
                current_mode = MODE_PALETTE;
                // First time entering settings - capture starting values
                palette_on_mode_entry = (last_palette_index == 0xFF) ? get_active_palette_index() : last_palette_index;
                last_palette_index = palette_on_mode_entry;
                offset_x_on_mode_entry = X_OFF;
                last_offset_x_value = X_OFF;
                offset_y_on_mode_entry = Y_OFF;
                last_offset_y_value = Y_OFF;
                // Capture initial ADC positions and reset moved flags
                adc_on_palette_entry = adc_read();
                adc_on_offset_x_entry = adc_read();
                adc_on_offset_y_entry = adc_read();
                adc_on_render_entry = adc_read();
                render_on_mode_entry = last_render_mode;
                palette_pot_moved = false;
                offset_x_pot_moved = false;
                offset_y_pot_moved = false;
                render_pot_moved = false;
                if (show_osd) *show_osd = true;
            } else if (current_mode == MODE_PALETTE) {
                // Move to offset-x mode
                current_mode = MODE_OFFSET_X;
                adc_on_offset_x_entry = adc_read();
                offset_x_pot_moved = false;
                if (show_osd) *show_osd = true;
            } else if (current_mode == MODE_OFFSET_X) {
                // Move to offset-y mode
                current_mode = MODE_OFFSET_Y;
                adc_on_offset_y_entry = adc_read();
                offset_y_pot_moved = false;
                if (show_osd) *show_osd = true;
            } else if (current_mode == MODE_OFFSET_Y) {
                // Move to render mode
                current_mode = MODE_RENDER;
                adc_on_render_entry = adc_read();
                render_pot_moved = false;
                if (show_osd) *show_osd = true;
            } else if (current_mode == MODE_RENDER) {
                // Loop back to palette mode
                current_mode = MODE_PALETTE;
                adc_on_palette_entry = adc_read();
                palette_pot_moved = false;
                if (show_osd) *show_osd = true;
            }
        }
    }
    
    // Reset click count if window expired
    if (click_count > 0 && (now - first_click_time > DOUBLE_CLICK_WINDOW_MS)) {
        click_count = 0;
    }
    
    last_switch_state = current_switch_state;
    
    // Auto-exit modes after timeout (except brightness mode)
    static bool showing_saved_message = false;
    static uint32_t saved_message_start = 0;
    
    if (showing_saved_message) {
        // Display "settings saved" for 1 second
        if (now - saved_message_start < 1000) {
            if (show_osd && scaled_buffer) {
                *show_osd = true;
                #ifndef ENABLE_BW_DITHER
                    draw_mode_osd(scaled_buffer, MODE_PALETTE, "settings saved", gb_colors[0], gb_colors[3], 4, 4);
                #else
                    draw_mode_osd(scaled_buffer, MODE_PALETTE, "settings saved", BW_WHITE, BW_BLACK, 4, 4);
                #endif
            }
        } else {
            // Done showing message, exit to brightness mode
            showing_saved_message = false;
            current_mode = MODE_BRIGHTNESS;
            if (show_osd) *show_osd = false;
        }
    } else if (current_mode != MODE_BRIGHTNESS && (now - last_mode_change_time > MODE_TIMEOUT_MS)) {
        // Save all settings before exiting
        if (last_palette_index != palette_on_mode_entry) {
            save_palette_to_eeprom(last_palette_index);
        }
        if (last_offset_x_value != offset_x_on_mode_entry) {
            save_offset_x_to_eeprom(last_offset_x_value);
        }
        if (last_offset_y_value != offset_y_on_mode_entry) {
            save_offset_y_to_eeprom(last_offset_y_value);
        }
        if (last_render_mode != render_on_mode_entry) {
            save_render_mode_to_eeprom(last_render_mode);
        }
        
        // Show "settings saved" message
        showing_saved_message = true;
        saved_message_start = now;
    }
    
    // Handle mode-specific controls (skip if showing saved message)
    if (!showing_saved_message) {
        switch (current_mode) {
            case MODE_BRIGHTNESS: {
                uint8_t current_brightness = get_brightness_from_adc();
                if (current_brightness != last_brightness_value) {
                    apply_brightness(lcd, current_brightness);
                    last_brightness_value = current_brightness;
                }
                break;
            }
        
        case MODE_PALETTE: {
            #ifndef ENABLE_BW_DITHER
                uint16_t current_adc = adc_read();
                const uint16_t ADC_THRESHOLD = 100; // Require pot movement before takeover
                
                // Check if pot has moved significantly from entry position
                if (!palette_pot_moved) {
                    if (abs((int)current_adc - (int)adc_on_palette_entry) > ADC_THRESHOLD) {
                        palette_pot_moved = true;
                    }
                }
                
                // Only update palette if pot has been moved
                if (palette_pot_moved) {
                    uint8_t current_palette_index = get_selected_palette_index();
                    if (current_palette_index != last_palette_index) {
                        gb_colors = PALETTE_LIST[current_palette_index];
                        last_palette_index = current_palette_index;
                        set_active_palette_index(current_palette_index);
                        last_mode_change_time = now; // Reset timeout on change
                    }
                }
                
                // Show OSD with palette name
                if (show_osd && scaled_buffer) {
                    *show_osd = true;
                    draw_mode_osd(scaled_buffer, MODE_PALETTE, PALETTE_NAMES[last_palette_index], 
                                 gb_colors[0], gb_colors[3]);
                }
            #endif
            break;
        }
        
        case MODE_OFFSET_X: {
            // Read ADC and map to offset range
            uint16_t adc_val = adc_read();
            const uint16_t ADC_THRESHOLD = 100; // Require pot movement before takeover
            
            // Check if pot has moved significantly from entry position
            if (!offset_x_pot_moved) {
                if (abs((int)adc_val - (int)adc_on_offset_x_entry) > ADC_THRESHOLD) {
                    offset_x_pot_moved = true;
                }
            }
            
            // Only update offset if pot has been moved
            if (offset_x_pot_moved) {
                int16_t new_offset_x = map_adc_to_offset(adc_val, OFFSET_X_MIN, OFFSET_X_MAX);
                
                // Clamp to screen bounds
                if (new_offset_x < 0) new_offset_x = 0;
                if (new_offset_x > (LCD_W - SCALED_W)) new_offset_x = (LCD_W - SCALED_W);
                
                // Only update if ADC value changed significantly (prevents jitter)
                if (new_offset_x != last_offset_x_value) {
                    X_OFF = new_offset_x;
                    last_offset_x_value = new_offset_x;
                    last_mode_change_time = now; // Reset timeout on change
                }
            }
            
            // Show OSD with relative offset value from base
            if (show_osd && scaled_buffer) {
                *show_osd = true;
                char value_str[8];
                int16_t relative_offset = last_offset_x_value - X_OFF_BASE;
                snprintf(value_str, sizeof(value_str), "%+d", relative_offset);
                #ifndef ENABLE_BW_DITHER
                    draw_mode_osd(scaled_buffer, MODE_OFFSET_X, value_str, gb_colors[0], gb_colors[3]);
                #else
                    draw_mode_osd(scaled_buffer, MODE_OFFSET_X, value_str, BW_WHITE, BW_BLACK);
                #endif
            }
            break;
        }
        
        case MODE_OFFSET_Y: {
            // Read ADC and map to offset range
            uint16_t adc_val = adc_read();
            const uint16_t ADC_THRESHOLD = 100; // Require pot movement before takeover
            
            // Check if pot has moved significantly from entry position
            if (!offset_y_pot_moved) {
                if (abs((int)adc_val - (int)adc_on_offset_y_entry) > ADC_THRESHOLD) {
                    offset_y_pot_moved = true;
                }
            }
            
            // Only update offset if pot has been moved
            if (offset_y_pot_moved) {
                int16_t new_offset_y = map_adc_to_offset(adc_val, OFFSET_Y_MIN, OFFSET_Y_MAX);
                
                // Clamp to screen bounds
                if (new_offset_y < 0) new_offset_y = 0;
                if (new_offset_y > (LCD_H - SCALED_H)) new_offset_y = (LCD_H - SCALED_H);
                
                if (new_offset_y != last_offset_y_value) {
                    Y_OFF = new_offset_y;
                    last_offset_y_value = new_offset_y;
                    last_mode_change_time = now; // Reset timeout on change
                }
            }
            
            // Show OSD with relative offset value from base
            if (show_osd && scaled_buffer) {
                *show_osd = true;
                char value_str[8];
                int16_t relative_offset = last_offset_y_value - Y_OFF_BASE;
                snprintf(value_str, sizeof(value_str), "%+d", relative_offset);
                #ifndef ENABLE_BW_DITHER
                    draw_mode_osd(scaled_buffer, MODE_OFFSET_Y, value_str, gb_colors[0], gb_colors[3]);
                #else
                    draw_mode_osd(scaled_buffer, MODE_OFFSET_Y, value_str, BW_WHITE, BW_BLACK);
                #endif
            }
            break;
        }
                case MODE_RENDER: {
            // Read ADC and map to render mode (0, 1, or 2)
            uint16_t adc_val = adc_read();
            
            // Check if pot has moved significantly from entry position
            if (!render_pot_moved) {
                if (abs((int)adc_val - (int)adc_on_render_entry) > ADC_THRESHOLD) {
                    render_pot_moved = true;
                }
            }
            
            // Only update render mode if pot has been moved
            if (render_pot_moved) {
                // Map ADC to 0, 1, or 2 (divide into thirds: 0-1365, 1366-2730, 2731-4095)
                uint8_t new_render_mode = (adc_val < 1366) ? 0 : (adc_val < 2731) ? 1 : 2;
                
                if (new_render_mode != last_render_mode) {
                    last_render_mode = new_render_mode;
                    last_mode_change_time = now; // Reset timeout on change
                }
            }
            
            // Show OSD with render mode name
            if (show_osd && scaled_buffer) {
                *show_osd = true;
                #ifndef ENABLE_BW_DITHER
                    draw_mode_osd(scaled_buffer, MODE_RENDER, RENDER_MODE_NAMES[last_render_mode], gb_colors[0], gb_colors[3]);
                #else
                    draw_mode_osd(scaled_buffer, MODE_RENDER, RENDER_MODE_NAMES[last_render_mode], BW_WHITE, BW_BLACK);
                #endif
            }
            break;
        }
                default:
            break;
    }
    }  // End of !showing_saved_message check
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
    return TEST_BLINK_IO_V1_1;
#endif
}

#ifdef VERSION_V1_1
void test_eeprom_save_load(ili9341::ILI9341& lcd) {
    printf("[EEPROM TEST] Testing hardware I2C...\n");
    printf("[EEPROM TEST] GPIO14=SDA, GPIO15=SCL\n");
    
    // Test write/read
    const uint8_t TEST_VALUE = 42;
    
    uint8_t write_buf[4] = {0x00, EEPROM_PALETTE_ADDR, PALETTE_MAGIC, TEST_VALUE};
    i2c_write_blocking(I2C_CHANNEL, EEPROM_ADDR, write_buf, 4, false);
    sleep_ms(10);
    
    uint8_t addr_buf[2] = {0x00, EEPROM_PALETTE_ADDR};
    i2c_write_blocking(I2C_CHANNEL, EEPROM_ADDR, addr_buf, 2, true);
    
    uint8_t read_buf[2];
    i2c_read_blocking(I2C_CHANNEL, EEPROM_ADDR, read_buf, 2, false);
    
    printf("[EEPROM TEST] Magic=0x%02X, Value=%d\n", read_buf[0], read_buf[1]);
    
    if (read_buf[0] == PALETTE_MAGIC && read_buf[1] == TEST_VALUE) {
        printf("[EEPROM TEST] SUCCESS!\n");
        lcd.clearScreen(0x07E0);  // Green
    } else {
        printf("[EEPROM TEST] FAILED!\n");
        lcd.clearScreen(0xF800);  // Red
    }
    
    sleep_ms(2000);
}
#endif

void display_test(ili9341::ILI9341& lcd, uint16_t* screenBuffer, uint16_t* scaledBuf, int* xmap, int* ymap) {
    uint8_t blink_pin = get_blink_pin();
    gpio_init(blink_pin);
    gpio_set_dir(blink_pin, GPIO_OUT);

    int pattern = 0;
    
    while (true) {
        // Check palette and brightness controls (only if not using BW dither)
        #ifndef ENABLE_BW_DITHER
            #ifdef VERSION_V1_1
                // v1.1: Mode switch controls palette vs brightness
                bool palette_mode = gpio_get(PIN_MODE_SWITCH);
                static uint8_t last_palette_index = 0xFF;
                static uint8_t last_brightness = 0xFF;
                static uint8_t palette_on_mode_entry = 0xFF;  // Track palette when entering palette mode
                static bool was_in_palette_mode = false;
                
                if (palette_mode) {
                    // Palette selection mode
                    uint8_t current_palette_index = get_selected_palette_index();
                    
                    if (!was_in_palette_mode) {
                        // Just entered palette mode - save the starting palette
                        palette_on_mode_entry = current_palette_index;
                        was_in_palette_mode = true;
                    }
                    
                    if (current_palette_index != last_palette_index) {
                        gb_colors = PALETTE_LIST[current_palette_index];
                        last_palette_index = current_palette_index;
                    }
                } else {
                    // Brightness control mode
                    // Exiting palette mode - save if palette changed
                    if (was_in_palette_mode) {
                        if (last_palette_index != palette_on_mode_entry && last_palette_index != 0xFF) {
                            // Palette changed during palette mode, save to EEPROM
                            save_palette_to_eeprom(last_palette_index);
                        }
                        was_in_palette_mode = false;
                        // Update brightness candidate to current ADC position WITHOUT applying
                        // This prevents brightness jump when exiting palette mode
                        last_brightness = get_brightness_from_adc();
                    }
                    
                    uint8_t current_brightness = get_brightness_from_adc();
                    // Only apply brightness if it changed after we entered brightness mode
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