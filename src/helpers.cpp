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

int16_t X_OFF = X_OFF_DEFAULT;
int16_t Y_OFF = Y_OFF_DEFAULT;

#ifndef ENABLE_BW_DITHER
extern const uint16_t* gb_colors;
#endif

static uint8_t active_palette_index = 0xFF;

void set_active_palette_index(uint8_t index) { active_palette_index = index; }
uint8_t get_active_palette_index() { return active_palette_index; }

uint8_t get_selected_palette_index() {
    uint32_t sum = 0;
    for (int i = 0; i < ADC_OVERSAMPLE_COUNT; i++) {
        sum += adc_read();
        sleep_us(ADC_DELAY_US);
    }
    uint8_t idx = ((sum / ADC_OVERSAMPLE_COUNT) * NUM_PALETTES) / ADC_MAX_VALUE;
    return (idx >= NUM_PALETTES) ? NUM_PALETTES - 1 : idx;
}

uint8_t get_brightness_from_adc() {
    return BRIGHTNESS_MIN + ((adc_read() * BRIGHTNESS_RANGE) / ADC_MAX_VALUE);
}

static void draw_text(uint16_t* buffer, int x, int y, const char* text, uint16_t fg, uint16_t bg) {
    const int char_w = 6, char_h = 7;
    
    for (int i = 0; text[i]; i++) {
        const uint8_t* glyph = font5x7[get_font_index(text[i])];
        for (int cx = 0; cx < 5; cx++) {
            for (int cy = 0; cy < 7; cy++) {
                if (glyph[cx] & (1 << cy)) {
                    int px = x + cx, py = y + cy;
                    if (px < SCALED_W && py < SCALED_H) {
                        buffer[py * SCALED_W + px] = fg;
                    }
                }
            }
        }
        x += char_w;
    }
}

void draw_palette_osd(uint16_t* buffer, uint8_t palette_index, uint16_t fg, uint16_t bg) {
    if (palette_index >= NUM_PALETTES) return;
    
    const char* name = PALETTE_NAMES[palette_index];
    const int padding = 2, start_x = 4, start_y = 4;
    int box_w = (9 + strlen(name)) * 6 + padding * 2;
    int box_h = 7 + padding * 2;
    
    for (int dy = 0; dy < box_h; dy++) {
        for (int dx = 0; dx < box_w; dx++) {
            int px = start_x + dx, py = start_y + dy;
            if (px < SCALED_W && py < SCALED_H) {
                buffer[py * SCALED_W + px] = bg;
            }
        }
    }
    
    char text[64];
    snprintf(text, sizeof(text), "Palette: %s", name);
    draw_text(buffer, start_x + padding, start_y + padding, text, fg, bg);
}

#ifdef VERSION_V1_1

enum ControlMode { MODE_BRIGHTNESS = 0, MODE_PALETTE, MODE_OFFSET_X, MODE_OFFSET_Y, MODE_RENDER, MODE_SCANLINE_INTENSITY, MODE_COUNT };

static struct {
    ControlMode mode = MODE_BRIGHTNESS;
    uint8_t palette_entry = 0;
    int16_t offset_x_entry = X_OFF_BASE;
    int16_t offset_y_entry = Y_OFF_BASE;
    uint8_t render_entry = 0;
    uint8_t intensity_entry = SCANLINE_INTENSITY_DEFAULT;
    uint16_t adc_palette = 0, adc_offset_x = 0, adc_offset_y = 0, adc_render = 0, adc_intensity = 0;
    bool pot_palette = false, pot_offset_x = false, pot_offset_y = false, pot_render = false, pot_intensity = false;
    uint8_t render_mode = 0;
    uint8_t scanline_intensity = SCANLINE_INTENSITY_DEFAULT;
} ctrl;

static struct {
    bool last_state = false;
    uint32_t first_click = 0;
    uint32_t click_count = 0;
    uint32_t last_change = 0;
} click;

static const uint32_t DOUBLE_CLICK_MS = 500;
static const uint32_t MODE_TIMEOUT_MS = 3000;
static const int ADC_THRESHOLD = 100;
static const char* RENDER_NAMES[] = {"normal", "scanline_h", "scanline_x"};

uint8_t get_render_mode() { return ctrl.render_mode; }
void set_render_mode(uint8_t mode) { if (mode <= 2) ctrl.render_mode = mode; }
uint8_t get_scanline_intensity() { return ctrl.scanline_intensity; }
void set_scanline_intensity(uint8_t intensity) { 
    ctrl.scanline_intensity = (intensity < SCANLINE_INTENSITY_MIN) ? SCANLINE_INTENSITY_MIN : 
                              (intensity > SCANLINE_INTENSITY_MAX) ? SCANLINE_INTENSITY_MAX : intensity;
}

static int16_t map_adc_to_offset(uint16_t val, int16_t min_v, int16_t max_v) {
    return min_v + ((val * (max_v - min_v + 1)) / 4096);
}

static void draw_mode_osd(uint16_t* buf, ControlMode mode, const char* value, uint16_t fg, uint16_t bg) {
    const char* prefix = "";
    bool show_prefix = true;
    
    if (strcmp(value, "settings saved") == 0) {
        show_prefix = false;
    } else {
        switch (mode) {
            case MODE_PALETTE: prefix = "palette:"; break;
            case MODE_OFFSET_X: prefix = "offset-x:"; break;
            case MODE_OFFSET_Y: prefix = "offset-y:"; break;
            case MODE_RENDER: prefix = "render:"; break;
            case MODE_SCANLINE_INTENSITY: prefix = "intensity:"; break;
            default: return;
        }
    }
    
    int text_len = strlen(prefix) + strlen(value);
    int box_w = text_len * 6 + 4;
    int box_h = 11;
    
    for (int dy = 0; dy < box_h; dy++) {
        for (int dx = 0; dx < box_w; dx++) {
            if (4 + dx < SCALED_W && 4 + dy < SCALED_H) {
                buf[(4 + dy) * SCALED_W + 4 + dx] = bg;
            }
        }
    }
    
    char text[64];
    snprintf(text, sizeof(text), "%s%s", prefix, value);
    draw_text(buf, 6, 6, text, fg, bg);
}

void apply_scanlines(uint16_t* buf, int w, int h, uint8_t mode, uint8_t intensity) {
    uint8_t factor = 255 - intensity;
    
    if (mode == 1) {
        for (int y = 1; y < h; y += 2) {
            uint16_t* row = &buf[y * w];
            for (int x = 0; x < w; x++) {
                uint16_t p = row[x];
                uint8_t r = ((p >> 11) & 0x1F) * factor >> 8;
                uint8_t g = ((p >> 5) & 0x3F) * factor >> 8;
                uint8_t b = (p & 0x1F) * factor >> 8;
                row[x] = (r << 11) | (g << 5) | b;
            }
        }
    } else if (mode == 2) {
        for (int y = 0; y < h; y++) {
            uint16_t* row = &buf[y * w];
            int start = (y & 1) ? 0 : 1;
            for (int x = start; x < w; x += 2) {
                uint16_t p = row[x];
                uint8_t r = ((p >> 11) & 0x1F) * factor >> 8;
                uint8_t g = ((p >> 5) & 0x3F) * factor >> 8;
                uint8_t b = (p & 0x1F) * factor >> 8;
                row[x] = (r << 11) | (g << 5) | b;
            }
            if (y & 1) {
                for (int x = 0; x < w; x += 2) {
                    uint16_t p = row[x];
                    uint8_t r = ((p >> 11) & 0x1F) * factor >> 8;
                    uint8_t g = ((p >> 5) & 0x3F) * factor >> 8;
                    uint8_t b = (p & 0x1F) * factor >> 8;
                    row[x] = (r << 11) | (g << 5) | b;
                }
            }
        }
    }
}

#endif

void apply_brightness(ili9341::ILI9341 &lcd, uint8_t brightness) {
#ifdef VERSION_V1_0
    static bool first = true;
    if (first) { sleep_ms(LOGO_BRIGHTNESS_DELAY_MS); first = false; }
    gpio_put(PIN_BL, brightness > BRIGHTNESS_THRESHOLD_V1_0 ? 1 : 0);
#else
    lcd.setBrightness(brightness);
#endif
}

void display_logo(ili9341::ILI9341& lcd) {
    static const uint16_t* logo = (uint16_t*)rMODS_logo_data;
    int logo_w = RMODS_LOGO_WIDTH, logo_h = RMODS_LOGO_HEIGHT;
    int logo_x = X_OFF + (SCALED_W - logo_w) / 2;
    int logo_y = Y_OFF + (SCALED_H - logo_h) / 2;
    
    lcd.clearScreen(RMODS_LOGO_BACKGROUND);
    lcd.drawImage(logo_x, logo_y, logo_w, logo_h, logo);
}

void init_adc() {
    adc_init();
    adc_gpio_init(PIN_PALETTE_ADC);
    gpio_pull_up(PIN_PALETTE_ADC);
    adc_select_input(3);
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

void update_hardware_controls(ili9341::ILI9341& lcd, uint16_t* buf, bool* show_osd) {
#ifdef VERSION_V1_0
    #ifndef ENABLE_BW_DITHER
    static uint8_t last_idx = 0xFF;
    uint8_t idx = get_selected_palette_index();
    if (idx != last_idx) {
        gb_colors = PALETTE_LIST[idx];
        last_idx = idx;
    }
    #endif
#endif

#ifdef VERSION_V1_1
    static uint8_t last_palette = get_active_palette_index();
    static uint8_t last_brightness = 0xFF;
    static int16_t last_x = X_OFF, last_y = Y_OFF;
    static bool showing_saved = false;
    static uint32_t saved_start = 0;
    
    uint32_t now = to_ms_since_boot(get_absolute_time());
    bool sw = gpio_get(PIN_MODE_SWITCH);
    
    if (!sw && click.last_state) {
        if (click.click_count == 0) {
            click.click_count = 1;
            click.first_click = now;
        } else if (now - click.first_click < DOUBLE_CLICK_MS) {
            click.click_count = 0;
            click.last_change = now;
            
            uint16_t adc = adc_read();
            if (ctrl.mode == MODE_BRIGHTNESS) {
                ctrl.mode = MODE_PALETTE;
                ctrl.palette_entry = (last_palette == 0xFF) ? get_active_palette_index() : last_palette;
                last_palette = ctrl.palette_entry;
                ctrl.offset_x_entry = X_OFF; last_x = X_OFF;
                ctrl.offset_y_entry = Y_OFF; last_y = Y_OFF;
                ctrl.adc_palette = ctrl.adc_offset_x = ctrl.adc_offset_y = ctrl.adc_render = ctrl.adc_intensity = adc;
                ctrl.render_entry = ctrl.render_mode;
                ctrl.intensity_entry = ctrl.scanline_intensity;
                ctrl.pot_palette = ctrl.pot_offset_x = ctrl.pot_offset_y = ctrl.pot_render = ctrl.pot_intensity = false;
            } else if (ctrl.mode == MODE_PALETTE) {
                ctrl.mode = MODE_OFFSET_X;
                ctrl.adc_offset_x = adc;
                ctrl.pot_offset_x = false;
            } else if (ctrl.mode == MODE_OFFSET_X) {
                ctrl.mode = MODE_OFFSET_Y;
                ctrl.adc_offset_y = adc;
                ctrl.pot_offset_y = false;
            } else if (ctrl.mode == MODE_OFFSET_Y) {
                ctrl.mode = MODE_RENDER;
                ctrl.adc_render = adc;
                ctrl.pot_render = false;
            } else if (ctrl.mode == MODE_RENDER) {
                ctrl.mode = MODE_SCANLINE_INTENSITY;
                ctrl.adc_intensity = adc;
                ctrl.pot_intensity = false;
            } else {
                ctrl.mode = MODE_PALETTE;
                ctrl.adc_palette = adc;
                ctrl.pot_palette = false;
            }
            if (show_osd) *show_osd = true;
        }
    }
    
    if (click.click_count > 0 && (now - click.first_click > DOUBLE_CLICK_MS)) {
        click.click_count = 0;
    }
    click.last_state = sw;
    
    if (showing_saved) {
        if (now - saved_start < 1000) {
            if (show_osd && buf) {
                *show_osd = true;
                #ifndef ENABLE_BW_DITHER
                draw_mode_osd(buf, MODE_PALETTE, "settings saved", gb_colors[0], gb_colors[3]);
                #else
                draw_mode_osd(buf, MODE_PALETTE, "settings saved", BW_WHITE, BW_BLACK);
                #endif
            }
        } else {
            showing_saved = false;
            ctrl.mode = MODE_BRIGHTNESS;
            if (show_osd) *show_osd = false;
        }
    } else if (ctrl.mode != MODE_BRIGHTNESS && (now - click.last_change > MODE_TIMEOUT_MS)) {
        if (last_palette != ctrl.palette_entry) save_palette_to_eeprom(last_palette);
        if (last_x != ctrl.offset_x_entry) save_offset_x_to_eeprom(last_x);
        if (last_y != ctrl.offset_y_entry) save_offset_y_to_eeprom(last_y);
        if (ctrl.render_mode != ctrl.render_entry) save_render_mode_to_eeprom(ctrl.render_mode);
        showing_saved = true;
        saved_start = now;
    }
    
    if (!showing_saved) {
        uint16_t adc = adc_read();
        
        switch (ctrl.mode) {
        case MODE_BRIGHTNESS: {
            uint8_t br = get_brightness_from_adc();
            if (br != last_brightness) {
                apply_brightness(lcd, br);
                last_brightness = br;
            }
            break;
        }
        case MODE_PALETTE: {
            #ifndef ENABLE_BW_DITHER
            if (!ctrl.pot_palette && abs((int)adc - (int)ctrl.adc_palette) > ADC_THRESHOLD) {
                ctrl.pot_palette = true;
            }
            if (ctrl.pot_palette) {
                uint8_t idx = get_selected_palette_index();
                if (idx != last_palette) {
                    gb_colors = PALETTE_LIST[idx];
                    last_palette = idx;
                    set_active_palette_index(idx);
                    click.last_change = now;
                }
            }
            if (show_osd && buf) {
                *show_osd = true;
                draw_mode_osd(buf, MODE_PALETTE, PALETTE_NAMES[last_palette], gb_colors[0], gb_colors[3]);
            }
            #endif
            break;
        }
        case MODE_OFFSET_X: {
            if (!ctrl.pot_offset_x && abs((int)adc - (int)ctrl.adc_offset_x) > ADC_THRESHOLD) {
                ctrl.pot_offset_x = true;
            }
            if (ctrl.pot_offset_x) {
                int16_t ox = map_adc_to_offset(adc, OFFSET_X_MIN, OFFSET_X_MAX);
                ox = (ox < 0) ? 0 : (ox > LCD_W - SCALED_W) ? LCD_W - SCALED_W : ox;
                if (ox != last_x) {
                    X_OFF = last_x = ox;
                    click.last_change = now;
                }
            }
            if (show_osd && buf) {
                *show_osd = true;
                char s[8]; snprintf(s, sizeof(s), "%+d", last_x - X_OFF_BASE);
                #ifndef ENABLE_BW_DITHER
                draw_mode_osd(buf, MODE_OFFSET_X, s, gb_colors[0], gb_colors[3]);
                #else
                draw_mode_osd(buf, MODE_OFFSET_X, s, BW_WHITE, BW_BLACK);
                #endif
            }
            break;
        }
        case MODE_OFFSET_Y: {
            if (!ctrl.pot_offset_y && abs((int)adc - (int)ctrl.adc_offset_y) > ADC_THRESHOLD) {
                ctrl.pot_offset_y = true;
            }
            if (ctrl.pot_offset_y) {
                int16_t oy = map_adc_to_offset(adc, OFFSET_Y_MIN, OFFSET_Y_MAX);
                oy = (oy < 0) ? 0 : (oy > LCD_H - SCALED_H) ? LCD_H - SCALED_H : oy;
                if (oy != last_y) {
                    Y_OFF = last_y = oy;
                    click.last_change = now;
                }
            }
            if (show_osd && buf) {
                *show_osd = true;
                char s[8]; snprintf(s, sizeof(s), "%+d", last_y - Y_OFF_BASE);
                #ifndef ENABLE_BW_DITHER
                draw_mode_osd(buf, MODE_OFFSET_Y, s, gb_colors[0], gb_colors[3]);
                #else
                draw_mode_osd(buf, MODE_OFFSET_Y, s, BW_WHITE, BW_BLACK);
                #endif
            }
            break;
        }
        case MODE_RENDER: {
            if (!ctrl.pot_render && abs((int)adc - (int)ctrl.adc_render) > ADC_THRESHOLD) {
                ctrl.pot_render = true;
            }
            if (ctrl.pot_render) {
                uint8_t rm = (adc < 1366) ? 0 : (adc < 2731) ? 1 : 2;
                if (rm != ctrl.render_mode) {
                    ctrl.render_mode = rm;
                    click.last_change = now;
                }
            }
            if (show_osd && buf) {
                *show_osd = true;
                #ifndef ENABLE_BW_DITHER
                draw_mode_osd(buf, MODE_RENDER, RENDER_NAMES[ctrl.render_mode], gb_colors[0], gb_colors[3]);
                #else
                draw_mode_osd(buf, MODE_RENDER, RENDER_NAMES[ctrl.render_mode], BW_WHITE, BW_BLACK);
                #endif
            }
            break;
        }
        case MODE_SCANLINE_INTENSITY: {
            if (!ctrl.pot_intensity && abs((int)adc - (int)ctrl.adc_intensity) > ADC_THRESHOLD) {
                ctrl.pot_intensity = true;
            }
            if (ctrl.pot_intensity) {
                uint8_t intensity = SCANLINE_INTENSITY_MIN + 
                    ((adc * (SCANLINE_INTENSITY_MAX - SCANLINE_INTENSITY_MIN)) / ADC_MAX_VALUE);
                if (intensity != ctrl.scanline_intensity) {
                    ctrl.scanline_intensity = intensity;
                    click.last_change = now;
                }
            }
            if (show_osd && buf) {
                *show_osd = true;
                char s[16]; 
                int pct = ((ctrl.scanline_intensity - SCANLINE_INTENSITY_MIN) * 100) / 
                          (SCANLINE_INTENSITY_MAX - SCANLINE_INTENSITY_MIN);
                snprintf(s, sizeof(s), "%d%%", pct);
                #ifndef ENABLE_BW_DITHER
                draw_mode_osd(buf, MODE_SCANLINE_INTENSITY, s, gb_colors[0], gb_colors[3]);
                #else
                draw_mode_osd(buf, MODE_SCANLINE_INTENSITY, s, BW_WHITE, BW_BLACK);
                #endif
            }
            break;
        }
        default: break;
        }
    }
#endif
}

void apply_dithering(uint16_t* buf) {
#ifdef ENABLE_BW_DITHER
    #ifdef DITHER_BEST
    floyd_steinberg_dither(buf, SCALED_W, SCALED_H, gb_colors, BW_WHITE, BW_BLACK);
    #else
    fast_bayer_dither(buf, SCALED_W, SCALED_H, gb_colors, BW_WHITE, BW_BLACK);
    #endif
#endif
}

void generate_test_pattern(uint16_t* buf, int pattern) {
    static const uint16_t COLORS[] = {0xF800, 0x07E0, 0x001F, 0xFFE0, 0xFFFF, 0x0000};
    enum { RED = 0, GREEN, BLUE, YELLOW, WHITE, BLACK };
    
    bool use_pal = (pattern % 2 == 0);
    int base = pattern / 2;
    
    for (int y = 0; y < DMG_H; y++) {
        for (int x = 0; x < DMG_W; x++) {
            uint16_t c = COLORS[BLACK];
            
            if (base == 0) {
                if (use_pal) {
                    c = gb_colors[(x * 4 / DMG_W) & 3];
                } else {
                    int s = DMG_W / 4;
                    c = COLORS[(x < s) ? RED : (x < s*2) ? GREEN : (x < s*3) ? BLUE : YELLOW];
                }
            } else if (base == 1) {
                if (use_pal) {
                    c = gb_colors[(y * 4 / DMG_H) & 3];
                } else {
                    int s = DMG_H / 4;
                    c = COLORS[(y < s) ? RED : (y < s*2) ? GREEN : (y < s*3) ? BLUE : YELLOW];
                }
            } else if (base == 2) {
                if (use_pal) {
                    c = gb_colors[((x / 40) + (y / 36)) & 3];
                } else {
                    int bx = x / 20, by = y / 20;
                    bool chk = (bx + by) & 1;
                    c = chk ? COLORS[(bx + by * 2) & 3] : COLORS[WHITE];
                }
            }
            buf[y * DMG_W + x] = c;
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
    printf("[EEPROM] Testing I2C (GPIO14=SDA, GPIO15=SCL)\n");
    
    const uint8_t TEST_VAL = 42;
    uint8_t wbuf[4] = {0x00, EEPROM_PALETTE_ADDR, PALETTE_MAGIC, TEST_VAL};
    i2c_write_blocking(I2C_CHANNEL, EEPROM_ADDR, wbuf, 4, false);
    sleep_ms(10);
    
    uint8_t addr[2] = {0x00, EEPROM_PALETTE_ADDR};
    i2c_write_blocking(I2C_CHANNEL, EEPROM_ADDR, addr, 2, true);
    
    uint8_t rbuf[2];
    i2c_read_blocking(I2C_CHANNEL, EEPROM_ADDR, rbuf, 2, false);
    
    bool ok = (rbuf[0] == PALETTE_MAGIC && rbuf[1] == TEST_VAL);
    printf("[EEPROM] %s (magic=0x%02X, val=%d)\n", ok ? "OK" : "FAIL", rbuf[0], rbuf[1]);
    lcd.clearScreen(ok ? 0x07E0 : 0xF800);
    sleep_ms(2000);
}
#endif

void display_test(ili9341::ILI9341& lcd, uint16_t* buf, uint16_t* scaled, int* xmap, int* ymap) {
    uint8_t pin = get_blink_pin();
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);

    int pattern = 0;
    
    while (true) {
#ifndef ENABLE_BW_DITHER
    #ifdef VERSION_V1_1
        static uint8_t last_pal = 0xFF, last_br = 0xFF, pal_entry = 0xFF;
        static bool was_pal = false;
        bool pal_mode = gpio_get(PIN_MODE_SWITCH);
        
        if (pal_mode) {
            uint8_t idx = get_selected_palette_index();
            if (!was_pal) { pal_entry = idx; was_pal = true; }
            if (idx != last_pal) { gb_colors = PALETTE_LIST[idx]; last_pal = idx; }
        } else {
            if (was_pal) {
                if (last_pal != pal_entry && last_pal != 0xFF) save_palette_to_eeprom(last_pal);
                was_pal = false;
                last_br = get_brightness_from_adc();
            }
            uint8_t br = get_brightness_from_adc();
            if (br != last_br) { apply_brightness(lcd, br); last_br = br; }
        }
    #else
        static uint8_t last_pal = 0xFF;
        uint8_t idx = get_selected_palette_index();
        if (idx != last_pal) { gb_colors = PALETTE_LIST[idx]; last_pal = idx; }
    #endif
#endif

        generate_test_pattern(buf, pattern);
        scale_frame(buf, scaled, xmap, ymap, DMG_W, SCALED_W, SCALED_H);
        apply_dithering(scaled);
        
        lcd.clearScreen(FILL_COLOR);
        lcd.drawImage(X_OFF, Y_OFF, SCALED_W, SCALED_H, scaled);
        
        gpio_put(pin, pattern & 1);
        sleep_ms(TEST_PATTERN_DELAY_MS);
        pattern = (pattern + 1) % NUM_TEST_PATTERNS;
    }
}