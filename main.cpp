#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "include/logo.h"
#include <stdbool.h>
#include "hardware/pio.h"
#include "gblcd.pio.h"

// Choose display type: uncomment one of these lines
#define USE_ST7789
//#define USE_ILI9341
//#define USE_ST7796

// Uncomment to enable dithering for monochrome display
#define ENABLE_BW_DITHER

// BW output constants used by the fast dither path
static const uint16_t BW_BLACK = 0x0000;
static const uint16_t BW_WHITE = 0xFFFF;

#ifdef USE_ST7789
    #include "displays/st7789/st7789.hpp"
#elif defined(USE_ILI9341)
    #include "displays/ili9341/ili9341.hpp"
#elif defined(USE_ST7796)
    #include "displays/st7796/st7796.hpp"
#else
    #error "Please define USE_ST7789, USE_ILI9341, or USE_ST7796"
#endif

#define DMG_W 160
#define DMG_H 144

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
    #define SCALED_H 288
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
    
    // Create display instance and configure based on type
#ifdef USE_ST7789
    st7789::ST7789 lcd;
    st7789::Config config;
    config.spi_speed_hz = 40 * 1000 * 1000;  // 40MHz for ST7789
    config.width = 240;
    config.height = 240;
    config.spi_inst = spi1;
    config.pin_din = 11;    // MOSI
    config.pin_sck = 10;    // SCK
    config.pin_cs = 9;     // CS
    config.pin_dc = 12;     // DC
    config.pin_reset = 13;  // RESET
    config.pin_bl = 8;     // Backlight
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
    config.spi_inst = spi1;
    config.pin_din = 11;    // MOSI
    config.pin_sck = 10;    // SCK
    config.pin_cs = 9;     // CS
    config.pin_dc = 12;     // DC
    config.pin_reset = 13;  // RESET
    config.pin_bl = 8;     // Backlight
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
    config.spi_inst = spi1;
    config.pin_din = 11;    // MOSI
    config.pin_sck = 10;    // SCK
    config.pin_cs = 9;     // CS
    config.pin_dc = 12;     // DC
    config.pin_reset = 13;  // RESET
    config.pin_bl = 8;     // Backlight
    config.rotation = st7796::ROTATION_180;  // 180 degrees for ST7796
    config.dma.enabled = true;
    config.dma.buffer_size = 4096;  // 4KB buffer for ST7796 (larger buffer for better frame rate)
    printf("Using ST7796 display (320x480)\n");
#endif
    
    if (!lcd.begin(config)) {
        printf("LCD initialization failed!\n");
        return -1;
    }
    
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
    
#ifdef USE_ST7789
    lcd.clearScreen(st7789::BLACK);
#elif defined(USE_ILI9341)
    lcd.clearScreen(ili9341::BLACK);
#elif defined(USE_ST7796)
    lcd.fillScreen(st7796::BLACK);
#endif
    
    sleep_ms(1000);
    
    #ifdef USE_ST7796
        lcd.drawImage(87, 56, 145, 115, rMODS_logo_data);
    #else
        lcd.drawImage(47, 62, 145, 115, rMODS_logo_data);
    #endif

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

    // Optional: compile-time enable the fast ordered 8x8 Bayer dither.
    // This replaces the slow Floyd–Steinberg pass with a simple ordered
    // dither that maps each scaled pixel directly to one of four constant
    // RGB565 outputs: white (0xFFFF), light (0x7777), dark (0x3333), black (0x0000).
    // Algorithm: compute luma per pixel, quantize to base level = floor(L*4),
    // then decide whether to bump to the next level by comparing the fractional
    // remainder to an 8x8 Bayer threshold (fast, branch-light, deterministic).
#ifdef ENABLE_BW_DITHER
    // 8x8 Bayer matrix (values 0..63)
    static const uint8_t bayer8[8][8] = {
        { 0,48,12,60, 3,51,15,63 },
        {32,16,44,28,35,19,47,31 },
        { 8,56, 4,52,11,59, 7,55 },
        {40,24,36,20,43,27,39,23 },
        { 2,50,14,62, 1,49,13,61 },
        {34,18,46,30,33,17,45,29 },
        {10,58, 6,54, 9,57, 5,53 },
        {42,26,38,22,41,25,37,21 }
    };

    // Compute Bayer thresholds for the two mid levels so they approximate
    // the target RGB565 constants 0x7777 (light) and 0x3333 (dark).
    auto rgb565_to_luma_fast = [](uint16_t pix) -> int {
        int r = (pix >> 11) & 0x1F;
        int g = (pix >> 5) & 0x3F;
        int b = pix & 0x1F;
        int r8 = (r * 255) / 31;
        int g8 = (g * 255) / 63;
        int b8 = (b * 255) / 31;
        return ((77 * r8) + (150 * g8) + (29 * b8)) >> 8; // 0..255
    };

    // Use existing palette entries in gb_colors[]: index 0 = lightest, 1 = light,
    // 2 = dark, 3 = darkest. Compute thresholds based on gb_colors[1] and [2].
    // compute fraction_black ~= 1 - L/255, then convert to 0..64 threshold
    int L_light = rgb565_to_luma_fast(gb_colors[1]);
    int L_dark  = rgb565_to_luma_fast(gb_colors[2]);
    int thresh_light = (int)(((255 - L_light) * 64 + 127) / 255); // 0..64
    int thresh_dark  = (int)(((255 - L_dark)  * 64 + 127) / 255); // 0..64

    for (int yy = 0; yy < SCALED_H; ++yy) {
        for (int xx = 0; xx < SCALED_W; ++xx) {
            int idx = yy * SCALED_W + xx;
            uint16_t col = scaledBuf[idx];

            if (col == gb_colors[0]) {
                scaledBuf[idx] = BW_WHITE; // keep solid white
            } else if (col == gb_colors[3]) {
                scaledBuf[idx] = BW_BLACK; // keep solid black
            } else {
                // choose appropriate threshold depending on whether this pixel
                // came from the "light" (gb_colors[1]) or "dark" (gb_colors[2])
                // source color. This assumes scaledBuf pixels are exact palette
                // entries (they are copied from screenBuffer earlier).
                int bval = bayer8[yy & 7][xx & 7]; // 0..63
                int t = (col == gb_colors[1]) ? thresh_light : thresh_dark;
                bool black = bval < t; // true -> black pixel, else white
                scaledBuf[idx] = black ? BW_BLACK : BW_WHITE;
            }
        }
    }
#endif

    lcd.drawImage(0, Y_OFF, SCALED_W, SCALED_H, scaledBuf);

        vSyncFallingEdgeDetected = false;
    }
    return 0;
}
