#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "include/logo.h"
#include <stdbool.h>
#include "hardware/pio.h"
#include "gblcd.pio.h"

// Choose display type: uncomment one of these lines
//#define USE_ST7789
#define USE_ILI9341
//#define USE_ST7796

#ifdef USE_ST7789
    #include "displays/st7789/st7789.hpp"
#elif defined(USE_ILI9341)
    #include "displays/ili9341/ili9341.hpp"
#elif defined(USE_ST7796)
    #include "displays/st7796/st7796.hpp"
#else
    #error "Please define USE_ST7789, USE_ILI9341, or USE_ST7796"
#endif

// ---- Dimensions ----
#define SRC_W 160
#define SRC_H 144

#ifdef USE_ST7789
    #define LCD_W 240
    #define LCD_H 240
    #define SCALED_W 240
    #define SCALED_H 216
    #define Y_OFF 12 // vertically centered

#elif defined(USE_ILI9341)
    #define LCD_W 240
    #define LCD_H 320
    #define SCALED_W 240
    #define SCALED_H 216
    #define Y_OFF 0 //top most

#elif defined(USE_ST7796)
    #define LCD_W 320
    #define LCD_H 480
    #define SCALED_W 320
    #define SCALED_H 288  // 144 * 2 = 288 (2x scaling for better fit)
    #define Y_OFF 0  // Top aligned
#endif



// Optional: precompute x/y maps once (integer NN: floor((dst*2)/3))
static int xmap[SCALED_W];
static int ymap[SCALED_H];
static bool maps_built = false;
static inline void build_maps(void) {
    if (maps_built) return;
    for (int x = 0; x < SCALED_W; x++) {
#ifdef USE_ST7796
        // For ST7796 320x480, use 2x scaling (160->320)
        int sx = x / 2;
#else
        // For ST7789/ILI9341, use 1.5x scaling (160->240)
        int sx = (x * 2) / 3;
#endif
        if (sx >= SRC_W) sx = SRC_W - 1;
        xmap[x] = sx;
    }
    for (int y = 0; y < SCALED_H; y++) {
#ifdef USE_ST7796
        // For ST7796, use 2x scaling (144->288)
        int sy = y / 2;
#else
        // For ST7789/ILI9341, use 1.5x scaling (144->216)  
        int sy = (y * 2) / 3;
#endif
        if (sy >= SRC_H) sy = SRC_H - 1;
        ymap[y] = sy;
    }
    maps_built = true;
}

int main() {
    stdio_init_all();
    
    printf("Starting Game Boy LCD capture...\n");
    
    // Create display instance and configure based on type
#ifdef USE_ST7789
    st7789::ST7789 lcd;
    st7789::Config config;
    config.spi_speed_hz = 40 * 1000 * 1000;  // 40MHz for ST7789
    config.width = 240;
    config.height = 240;
    config.spi_inst = spi0;
    config.pin_din = 19;    // MOSI
    config.pin_sck = 18;    // SCK
    config.pin_cs = 17;     // CS
    config.pin_dc = 20;     // DC
    config.pin_reset = 15;  // RESET
    config.pin_bl = 10;     // Backlight
    config.rotation = st7789::ROTATION_0;
    config.dma.enabled = true;
    config.dma.buffer_size = 480;  // 240 pixels * 2 bytes = 480 bytes per line
    printf("Using ST7789 display (240x240)\n");
    
#elif defined(USE_ILI9341)
    ili9341::ILI9341 lcd;
    ili9341::Config config;
    config.spi_speed_hz = 40 * 1000 * 1000;  // 40MHz for ILI9341 (same as ST7789)
    config.width = 240;
    config.height = 320;
    config.spi_inst = spi0;
    config.pin_din = 19;    // MOSI
    config.pin_sck = 18;    // SCK
    config.pin_cs = 17;     // CS
    config.pin_dc = 20;     // DC
    config.pin_reset = 15;  // RESET
    config.pin_bl = 10;     // Backlight
    config.rotation = ili9341::ROTATION_0;
    config.dma.enabled = true;
    config.dma.buffer_size = 960;  // 240 pixels * 2 bytes * 2 lines = 960 bytes (double buffer for speed)
    printf("Using ILI9341 display (240x320)\n");
    
#elif defined(USE_ST7796)
    st7796::ST7796 lcd;
    st7796::Config config;
    config.spi_speed_hz = 62.5 * 1000 * 1000;  // 62.5MHz for ST7796 (higher speed for better frame rate)
    config.width = 320;
    config.height = 480;
    config.spi_inst = spi0;
    config.pin_din = 19;    // MOSI
    config.pin_sck = 18;    // SCK
    config.pin_cs = 17;     // CS
    config.pin_dc = 20;     // DC
    config.pin_reset = 15;  // RESET
    config.pin_bl = 10;     // Backlight
    config.rotation = st7796::ROTATION_180;  // 180 degrees for ST7796
    config.dma.enabled = true;
    config.dma.buffer_size = 4096;  // 4KB buffer for ST7796 (larger buffer for better frame rate)
    printf("Using ST7796 display (320x480)\n");
#endif
    
    // Initialize LCD with proper config for each display type
    if (!lcd.begin(config)) {
        printf("LCD initialization failed!\n");
        return -1;
    }
    
    // Set rotation - already set in config, but ensure it's applied
#ifdef USE_ST7789
    lcd.setRotation(st7789::ROTATION_0);
#elif defined(USE_ILI9341)
    lcd.setRotation(ili9341::ROTATION_0);
#elif defined(USE_ST7796)
    lcd.setRotation(st7796::ROTATION_180);  // ST7796 uses 180 degrees
    // Set maximum brightness for ST7796
#endif

    // Set static brightness
    lcd.setBrightness(255);  // Full brightness
    printf("Brightness set to 255 (maximum)\n");
    
    // Clear screen with proper color enum for each display
#ifdef USE_ST7789
    lcd.clearScreen(st7789::BLACK);
#elif defined(USE_ILI9341)
    lcd.clearScreen(ili9341::BLACK);
#elif defined(USE_ST7796)
    lcd.fillScreen(0x0000); // ST7796 might use fillScreen instead
#endif
    
    sleep_ms(1000);
    
    // Display some test graphics
    printf("Drawing logo \n");
    
    #ifdef USE_ST7796
        lcd.drawImage(87, 56, 145, 115, rMODS_logo_data);
    #else
        lcd.drawImage(47, 62, 145, 115, rMODS_logo_data);
    #endif

    sleep_ms(1000);
    
    // Now start the Game Boy LCD capture part
    printf("Starting Game Boy LCD capture...\n");
    
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
    static uint16_t screenBuffer[SRC_W * SRC_H];

    // Destination (scaled 240x216) ~101 KB
    static uint16_t scaledBuf[SCALED_W * SCALED_H];

    uint16_t data0, data1, vSync;

    // Pre-computed color lookup tables for better performance
#ifdef USE_ST7789
    static const uint16_t gb_colors[4] = {
        0x670F,  // Lightest green - #67e767ff (background)
        0x560C,  // Light green - #56b556ff (brighter)
        0x1A23,  // Dark green - #1a341aff (darker)
        0x0800   // Darkest green - #081808ff (much darker)
    };
#elif defined(USE_ILI9341)
    static const uint16_t gb_colors[4] = {
        0x4E09,  // Lightest green - #4bc24bff (background)
        0x3526,  // Light green - #37a537ff
        0x2384,  // Dark green - #277227ff
        0x2224   // Darkest green - #234623ff
    };
#elif defined(USE_ST7796)
    static const uint16_t gb_colors[4] = {
        0x4E09,  // Lightest green - #4bc24bff (background) - same as ILI9341
        0x3526,  // Light green - #37a537ff
        0x2384,  // Dark green - #277227ff
        0x2224   // Darkest green - #234623ff
    };
#endif

    // Build mapping once
    build_maps();

#ifdef USE_ST7796
    printf("ST7796 optimizations enabled: DMA=%s, 2x scaling, lookup table colors\n", 
           config.dma.enabled ? "ON" : "OFF");
#endif

    while (true) {
        uint32_t result = pio_sm_get_blocking(pio, state_machine_id);
        vSync = (result >> 31) & 1;

        vSyncCurrent = vSync;
        if (!vSyncCurrent && vSyncPrev) vSyncFallingEdgeDetected = true;
        vSyncPrev = vSyncCurrent;

        // Wait for VSync
        if (!vSyncFallingEdgeDetected) {
            continue;
        }

        if (!firstRun) {
            firstRun = true;
            lcd.clearScreen(0x0000); // Clear to black on first run (gives bars background)
        }

        // ---- Capture 160x144 into screenBuffer ----
        for (y = 0; y < SRC_H; y++) {
            for (x = 0; x < SRC_W; x++) {
                if (x > 0 || y > 0) result = pio_sm_get_blocking(pio, state_machine_id);
                data0 = (result >> 29) & 1;
                data1 = (result >> 30) & 1;

                // Use lookup table for better performance
                uint8_t color_index = (data1 << 1) | data0;
                screenBuffer[y * SRC_W + x] = gb_colors[color_index];
            }
        }

        // ---- Scale to target resolution (optimized) ----
#ifdef USE_ST7796
        // For ST7796, optimize 2x scaling with direct pixel duplication
        for (int dy = 0; dy < SCALED_H; dy++) {
            const uint16_t* srcRow = &screenBuffer[ ymap[dy] * SRC_W ];
            uint16_t* dstRow = &scaledBuf[ dy * SCALED_W ];
            
            // Unroll loop for better performance (process 4 pixels at a time when possible)
            int dx = 0;
            for (; dx < SCALED_W - 3; dx += 4) {
                dstRow[dx] = srcRow[xmap[dx]];
                dstRow[dx + 1] = srcRow[xmap[dx + 1]];
                dstRow[dx + 2] = srcRow[xmap[dx + 2]];
                dstRow[dx + 3] = srcRow[xmap[dx + 3]];
            }
            // Handle remaining pixels
            for (; dx < SCALED_W; dx++) {
                dstRow[dx] = srcRow[xmap[dx]];
            }
        }
#else
        // For other displays, standard scaling
        for (int dy = 0; dy < SCALED_H; dy++) {
            const uint16_t* srcRow = &screenBuffer[ ymap[dy] * SRC_W ];
            uint16_t* dstRow = &scaledBuf[ dy * SCALED_W ];
            for (int dx = 0; dx < SCALED_W; dx++) {
                dstRow[dx] = srcRow[ xmap[dx] ];
            }
        }
#endif

        // Use optimized drawing method for all displays
        lcd.drawImage(0, Y_OFF, SCALED_W, SCALED_H, scaledBuf);

        vSyncFallingEdgeDetected = false;
    }
    return 0;
}
