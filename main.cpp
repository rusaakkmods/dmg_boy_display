#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "include/logo.h"
#include <stdbool.h>
#include "hardware/pio.h"
#include "hardware/spi.h"
#include "gblcd.pio.h"
#include "dither.hpp"

// Choose display type: uncomment one of these lines
//#define USE_ST7789
//#define USE_ILI9341
//#define USE_ILI9342
#define USE_ST7796

// Uncomment to enable dithering for monochrome display
//#define ENABLE_BW_DITHER

// Pin definitions
#define SPI_CHANNEL spi1
#define PIN_MOSI 11
#define PIN_SCK 10
#define PIN_CS 9
#define PIN_DC 12
#define PIN_RESET 13
#define PIN_BL 8

#define DMG_W 160
#define DMG_H 144

#ifdef USE_ST7789
    #include "displays/st7789/st7789.hpp"
    #define LCD_W 240
    #define LCD_H 240
    #define SCALED_W 240
    #define SCALED_H 216
    #define Y_OFF 12
    #define X_OFF 0
    #define DISPLAY_SCALE 1.5
#elif defined(USE_ILI9341)
    #include "displays/ili9341/ili9341.hpp"
    #define LCD_W 240
    #define LCD_H 320
    #define SCALED_W 240
    #define SCALED_H 216
    #define Y_OFF 0
    #define X_OFF 0
    #define DISPLAY_SCALE 1.5
#elif defined(USE_ILI9342)
    #include "displays/ili9342/ili9342.hpp"
    #define LCD_W 320
    #define LCD_H 240
    #define SCALED_W 240
    #define SCALED_H 216
    #define Y_OFF 12
    #define X_OFF 40
    #define DISPLAY_SCALE 1.5
#elif defined(USE_ST7796)
    #include "displays/st7796/st7796.hpp"
    #define LCD_W 320
    #define LCD_H 480
    #define SCALED_W 320
    #define SCALED_H 288
    #define Y_OFF 0 
    #define X_OFF 0
    #define DISPLAY_SCALE 2
#else
    #error "Please define USE_ST7789, USE_ILI9341, USE_ILI9342, or USE_ST7796"
#endif

// BW output constants used by the fast dither path
static const uint16_t BW_BLACK = 0x0000;
static const uint16_t BW_WHITE = 0xFFFF;

// Palettes setup
#ifdef ENABLE_BW_DITHER
    static const uint16_t* logo = (uint16_t*)rMODS_logo_bw_data;
    static const uint16_t gb_colors[4] = {
        0xFFFF,  // Lightest
        0x8888,  // Light
        0x4444,  // Dark
        0x0000   // Darkest
    };
#else
    static const uint16_t* logo = (uint16_t*)rMODS_logo_data;
    #ifdef USE_ST7789
        static const uint16_t gb_colors[4] = {
            0x9772,  // Bright saturated green - much more vibrant background
            0x64ED,  // Rich medium green - deeper saturation
            0x2A85,  // Dark forest green - good contrast
            0x1082   // Very dark green - strong contrast
        };
    #elif defined(USE_ILI9341)
        static const uint16_t gb_colors[4] = {
            0x4E09,  // Lightest green - #4bc24bff (background)
            0x3526,  // Light green - #37a537ff
            0x2384,  // Dark green - #277227ff
            0x2224   // Darkest green - #234623ff
        };
    #elif defined(USE_ILI9342)
        static const uint16_t gb_colors[4] = {
            0x3e88,  // Lightest green - (background)
            0x2c05,  // Light green
            0x1a63,  // Dark green
            0x08c1   // Darkest green
        };
    #elif defined(USE_ST7796)
        static const uint16_t gb_colors[4] = {
            0x4E09,  // Lightest green - #4bc24bff (background) - same as ILI9341
            0x3526,  // Light green - #37a537ff
            0x2384,  // Dark green - #277227ff
            0x2224   // Darkest green - #234623ff
        };
    #endif
#endif

static int xmap[SCALED_W];
static int ymap[SCALED_H];
static bool maps_built = false;
static inline void build_maps(void) {
    if (maps_built) return;

    for (int x = 0; x < SCALED_W; x++) {
        int sx;
        if (DISPLAY_SCALE == 1.5) sx = (x * 2) / 3;
        else if (DISPLAY_SCALE == 2) sx = x / 2;
        else  sx = x;

        if (sx >= DMG_W) sx = DMG_W - 1;
        xmap[x] = sx;
    }
    for (int y = 0; y < SCALED_H; y++) {
        int sy;
        if (DISPLAY_SCALE == 1.5) sy = (y * 2) / 3;
        else if (DISPLAY_SCALE == 2) sy = y / 2;
        else  sy = y;

        if (sy >= DMG_H) sy = DMG_H - 1;
        ymap[y] = sy;
    }
    
    maps_built = true;
}

int main() {
    stdio_init_all();
    
#ifdef USE_ST7789
    st7789::ST7789 lcd;
    st7789::Config config;
    config.spi_speed_hz = 40 * 1000 * 1000;  // 40MHz
    config.rotation = st7789::ROTATION_0;
    config.dma.buffer_size = 480;  // 240 pixels * 2 bytes = 480 bytes per line
#elif defined(USE_ILI9341)
    ili9341::ILI9341 lcd;
    ili9341::Config config;
    config.spi_speed_hz = 40 * 1000 * 1000;  // 40MHz
    config.rotation = ili9341::ROTATION_0;
    config.dma.buffer_size = 960;  // 240 pixels * 2 bytes * 2 lines = 960 bytes
#elif defined(USE_ILI9342)
    ili9342::ILI9342 lcd;
    ili9342::Config config;
    config.spi_speed_hz = 40 * 1000 * 1000;  // 40MHz
    config.rotation = ili9342::ROTATION_0;
    config.dma.buffer_size = 960;  // 240 pixels * 2 bytes * 2 lines = 960 bytes
#elif defined(USE_ST7796)
    st7796::ST7796 lcd;
    st7796::Config config;
    config.spi_speed_hz = 62.5 * 1000 * 1000;  // 62.5MHz
    config.rotation = st7796::ROTATION_180;
    config.dma.buffer_size = 4096;  // 4KB buffer
#endif

    // Set common config values
    config.width = LCD_W;
    config.spi_inst = SPI_CHANNEL;
    config.pin_din = PIN_MOSI;
    config.pin_sck = PIN_SCK;
    config.pin_cs = PIN_CS;
    config.pin_dc = PIN_DC;
    config.pin_reset = PIN_RESET;
    config.pin_bl = PIN_BL;
    config.dma.enabled = true;
    
    lcd.begin(config);

    // set lcd rotation
    lcd.setRotation(config.rotation);

    // Set static brightness 0 off, 255 brightest
    lcd.setBrightness(255); 

    // fill screen with black
    lcd.clearScreen(0x0000);

    //(LCD_W - RMODS_LOGO_WIDTH) / 2;
    int logo_x = X_OFF + (SCALED_W - RMODS_LOGO_WIDTH) / 2;
    int logo_y = Y_OFF + (SCALED_H - RMODS_LOGO_HEIGHT) / 2;
    lcd.drawImage(logo_x, logo_y, RMODS_LOGO_WIDTH, RMODS_LOGO_HEIGHT, logo);
    sleep_ms(1000);

    // PIO setup
    PIO pio = pio0; // gblcd.pio
    uint state_machine_id = 0;
    uint offset = pio_add_program(pio, &gblcd_program);
    gblcd_program_init(pio, state_machine_id, offset);

    int x = 0, y = 0;
    bool vSyncPrev = false;
    bool vSyncCurrent = false;
    bool vSyncFallingEdgeDetected = false;
    bool firstRun = false;

    static uint16_t screenBuffer[DMG_W * DMG_H];
    static uint16_t scaledBuf[SCALED_W * SCALED_H];
    uint16_t data0, data1, vSync;

    build_maps();

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
            lcd.clearScreen(0x0000);
        }

        // ---- Capture 160x144 into screenBuffer ----
        uint16_t* bufPtr = screenBuffer;
        for (y = 0; y < DMG_H; y++) {
            for (x = 0; x < DMG_W; x++) {
                if (x > 0 || y > 0) result = pio_sm_get_blocking(pio, state_machine_id);
                data0 = (result >> 29) & 1;
                data1 = (result >> 30) & 1;
                *bufPtr++ = gb_colors[(data1 << 1) | data0];
            }
        }

        if (DISPLAY_SCALE > 1) {
            for (int dy = 0; dy < SCALED_H; dy++) {
                const uint16_t* srcRow = &screenBuffer[ ymap[dy] * DMG_W ];
                uint16_t* dstRow = &scaledBuf[ dy * SCALED_W ];
                int dx = 0;
                for (; dx <= SCALED_W - 8; dx += 8) {
                    dstRow[dx] = srcRow[xmap[dx]];
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
        else {
            //no-scaling
            for (int i = 0; i < DMG_W * DMG_H; i++) {
                scaledBuf[i] = screenBuffer[i];
            }
        }
        
#ifdef ENABLE_BW_DITHER
        fast_bayer_dither(scaledBuf, SCALED_W, SCALED_H, gb_colors, BW_WHITE, BW_BLACK);
#endif

        lcd.drawImage(X_OFF, Y_OFF, SCALED_W, SCALED_H, scaledBuf);
        vSyncFallingEdgeDetected = false;
    }
    return 0;
}
