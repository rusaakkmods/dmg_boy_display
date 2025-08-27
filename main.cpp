#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "include/logo.h"
#include <stdbool.h>
#include "hardware/pio.h"
#include "hardware/spi.h"
#include "gblcd.pio.h"
#include "dither.hpp"

#define SPI_CHANNEL spi1
#define PIN_MOSI 11
#define PIN_SCK 10
#define PIN_CS 9
#define PIN_DC 12
#define PIN_RESET 13
#define PIN_BL 8

// Choose display type: uncomment one of these lines

//#define USE_ST7789
#define USE_ILI9341
//#define USE_ILI9342
//#define USE_ST7796

// Uncomment to enable dithering for monochrome display
//#define ENABLE_BW_DITHER

// BW output constants used by the fast dither path
static const uint16_t BW_BLACK = 0x0000;
static const uint16_t BW_WHITE = 0xFFFF;


#ifdef USE_ST7789
    #include "displays/st7789/st7789.hpp"
#elif defined(USE_ILI9341)
    #include "displays/ili9341/ili9341.hpp"
#elif defined(USE_ILI9342)
    #include "displays/ili9342/ili9342.hpp"
#elif defined(USE_ST7796)
    #include "displays/st7796/st7796.hpp"
#else
    #error "Please define USE_ST7789, USE_ILI9341, USE_ILI9342, or USE_ST7796"
#endif

// Pin definitions
#define PIN_MOSI 11
#define PIN_SCK 10
#define PIN_CS 9
#define PIN_DC 12
#define PIN_RESET 13
#define PIN_BL 8

#define DMG_W 160
#define DMG_H 144

#ifdef USE_ST7789
    #define LCD_W 240
    #define LCD_H 240
    #define SCALED_W 240
    #define SCALED_H 216
    #define Y_OFF 12 // vertically centered
    #define H_OFF 0


#elif defined(USE_ILI9341)
    #define LCD_W 240
    #define LCD_H 320
    #define SCALED_W 240
    #define SCALED_H 216
    #define Y_OFF 0 //top most
    #define H_OFF 0

#elif defined(USE_ILI9342)
    #define LCD_W 320
    #define LCD_H 240
    #define SCALED_W 240
    #define SCALED_H 216
    #define Y_OFF 12 // vertically centered
    #define H_OFF 40

#elif defined(USE_ST7796)
    #define LCD_W 320
    #define LCD_H 480
    #define SCALED_W 320
    #define SCALED_H 288
    #define Y_OFF 0  // Top aligned
    #define H_OFF 0
#endif

// Optional: precompute x/y maps once (integer NN: floor((dst*2)/3))
static int xmap[SCALED_W];
static int ymap[SCALED_H];
static bool maps_built = false;
static inline void build_maps(void) {
    if (maps_built) return;
    for (int x = 0; x < SCALED_W; x++) {
#ifdef USE_ST7796
        // 2x scaling (160->320)
        int sx = x / 2;
#else
        // 1.5x scaling (160->240)
        int sx = (x * 2) / 3;
#endif
        if (sx >= DMG_W) sx = DMG_W - 1;
        xmap[x] = sx;
    }
    for (int y = 0; y < SCALED_H; y++) {
#ifdef USE_ST7796
        // 2x scaling (144->288)
        int sy = y / 2;
#else
        // 1.5x scaling (144->216)  
        int sy = (y * 2) / 3;
#endif
        if (sy >= DMG_H) sy = DMG_H - 1;
        ymap[y] = sy;
    }
    maps_built = true;
}

int main() {
    stdio_init_all();
    
    printf("Starting Game Boy LCD capture...\n");
    

    // Common display config
#ifdef USE_ST7789
    st7789::ST7789 lcd;
    st7789::Config config;
    config.spi_speed_hz = 40 * 1000 * 1000;  // 40MHz for ST7789
    config.rotation = st7789::ROTATION_0;
    config.dma.buffer_size = 480;  // 240 pixels * 2 bytes = 480 bytes per line
    printf("Using ST7789 display (240x240)\n");
#elif defined(USE_ILI9341)
    ili9341::ILI9341 lcd;
    ili9341::Config config;
    config.spi_speed_hz = 40 * 1000 * 1000;  // 40MHz for ILI9341
    config.rotation = ili9341::ROTATION_0;
    config.dma.buffer_size = 960;  // 240 pixels * 2 bytes * 2 lines = 960 bytes
    printf("Using ILI9341 display (240x320)\n");
#elif defined(USE_ILI9342)
    ili9342::ILI9342 lcd;
    ili9342::Config config;
    config.spi_speed_hz = 40 * 1000 * 1000;  // 40MHz for ILI9342
    config.rotation = ili9342::ROTATION_0;
    config.dma.buffer_size = 960;  // 240 pixels * 2 bytes * 2 lines = 960 bytes
    printf("Using ILI9342 display (320x240)\n");
#elif defined(USE_ST7796)
    st7796::ST7796 lcd;
    st7796::Config config;
    config.spi_speed_hz = 62.5 * 1000 * 1000;  // 62.5MHz for ST7796
    config.rotation = st7796::ROTATION_180;  // 180 degrees for ST7796
    config.dma.buffer_size = 4096;  // 4KB buffer for ST7796
    printf("Using ST7796 display (320x480)\n");
#endif

    // Set common config values
    config.width = LCD_W;
    config.spi_inst = SPI_CHANNEL;
    config.spi_inst = spi1;
    config.pin_din = PIN_MOSI;
    config.pin_sck = PIN_SCK;
    config.pin_cs = PIN_CS;
    config.pin_dc = PIN_DC;
    config.pin_reset = PIN_RESET;
    config.pin_bl = PIN_BL;
    config.dma.enabled = true;

    
    if (!lcd.begin(config)) {
        printf("LCD initialization failed!\n");
        return -1;
    }
    
#ifdef USE_ST7789
    lcd.setRotation(st7789::ROTATION_0);
#elif defined(USE_ILI9341)
    lcd.setRotation(ili9341::ROTATION_0);
#elif defined(USE_ILI9342)
    lcd.setRotation(ili9342::ROTATION_0);
#elif defined(USE_ST7796)
    lcd.setRotation(st7796::ROTATION_180);  // ST7796 uses 180 degrees
    // Set maximum brightness for ST7796
#endif

    // Set static brightness
    lcd.setBrightness(255);  // Full brightness
    printf("Brightness set to 255 (maximum)\n");
    
#ifdef USE_ST7789
    lcd.clearScreen(st7789::BLACK);
#elif defined(USE_ILI9341)
    lcd.clearScreen(ili9341::BLACK);
#elif defined(USE_ILI9342)
    lcd.clearScreen(ili9342::BLACK);
#elif defined(USE_ST7796)
    lcd.fillScreen(st7796::BLACK);
#endif
    
    sleep_ms(1000);

    #ifdef ENABLE_BW_DITHER
        uint16_t* logo = (uint16_t*)rMODS_logo_bw_data;
    #else
        uint16_t* logo = (uint16_t*)rMODS_logo_data;
    #endif




// Always center the logo for all displays using RMODS_LOGO_WIDTH and RMODS_LOGO_HEIGHT from logo.h
int logo_x = (LCD_W - RMODS_LOGO_WIDTH) / 2;
int logo_y = (LCD_H - RMODS_LOGO_HEIGHT) / 2;
lcd.drawImage(logo_x, logo_y, RMODS_LOGO_WIDTH, RMODS_LOGO_HEIGHT, logo);

    sleep_ms(1000);
    
    printf("Starting Game Boy LCD capture...\n");
    
    // PIO setup
    PIO pio = pio0;
    uint state_machine_id = 0;
    uint offset = pio_add_program(pio, &gblcd_program);
    gblcd_program_init(pio, state_machine_id, offset);

    int x = 0, y = 0;
    bool vSyncPrev = false;
    bool vSyncCurrent = false;
    bool vSyncFallingEdgeDetected = false;
    bool firstRun = false;

    // Source (160x144)
    static uint16_t screenBuffer[DMG_W * DMG_H];

    // Destination (scaled 240x216) ~101 KB
    static uint16_t scaledBuf[SCALED_W * SCALED_H];

    uint16_t data0, data1, vSync;

#ifdef ENABLE_BW_DITHER
    static const uint16_t gb_colors[4] = {
        0xFFFF,  // Lightest
        0x8888,  // Light
        0x4444,  // Dark
        0x0000   // Darkest
    };
#else

    // palette
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

    build_maps();

#ifdef USE_ST7796
    printf("ST7796 optimizations enabled: DMA=%s, 2x scaling, lookup table colors\n", 
           config.dma.enabled ? "ON" : "OFF");
#endif

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
        // Optimize: minimize index calculation, unroll inner loop
        uint16_t* bufPtr = screenBuffer;
        for (y = 0; y < DMG_H; y++) {
            for (x = 0; x < DMG_W; x++) {
                if (x > 0 || y > 0) result = pio_sm_get_blocking(pio, state_machine_id);
                data0 = (result >> 29) & 1;
                data1 = (result >> 30) & 1;
                *bufPtr++ = gb_colors[(data1 << 1) | data0];
            }
        }

#ifdef USE_ST7796
        // For ST7796, optimize 2x scaling with direct pixel duplication and loop unrolling
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
#else
        // For other displays, standard scaling with loop unrolling
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
#endif

#ifdef ENABLE_BW_DITHER
    fast_bayer_dither(scaledBuf, SCALED_W, SCALED_H, gb_colors, BW_WHITE, BW_BLACK);
#endif

    lcd.drawImage(H_OFF, Y_OFF, SCALED_W, SCALED_H, scaledBuf);

        vSyncFallingEdgeDetected = false;
    }
    return 0;
}
