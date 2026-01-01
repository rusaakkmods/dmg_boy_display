#include "palettes.hpp"
#include "config.h"

// Make DEFAULT_PALETTE_NAME accessible
const uint16_t* const DEFAULT_PALETTE_PTR = DEFAULT_PALETTE_NAME;

const uint16_t* const PALETTE_LIST[] = {
    MODERN,
    ADVENTURER,
    SGB,
    LCD,
    CLOUDY,
    VINTAGE,
    BLUE_HUE,
    HIGHLIGHT_BLUE,
    NEON,
    PEACH,
    MODERN2,
    ROMANCE,
    RETRO,
    GRAY_SHADES,
    RED_PASTEL_SHADES,
    TEAL_SHADES,
    YELLOW_SHADES,
    GREEN_SHADES,
    GRAYSCALE_INVERT,
    GRAYSCALE
};

const char* const PALETTE_NAMES[] = {
    "modern",
    "adventurer",
    "sgb",
    "lcd",
    "cloudy",
    "vintage",
    "blue_hue",
    "highlight",
    "neon",
    "peach",
    "modern_2",
    "romance",
    "retro",
    "gray",
    "red_pastel",
    "teal",
    "yellow",
    "green",
    "grayscale-",
    "grayscale"
};

const size_t NUM_PALETTES = sizeof(PALETTE_LIST) / sizeof(PALETTE_LIST[0]);

#ifdef ENABLE_BW_DITHER
    const uint16_t BW_BLACK = 0x0000;
    const uint16_t BW_WHITE = 0xFFFF;
    
    #ifdef DITHER_BEST
        const uint16_t gb_colors[4] = {0xFFFF, 0xAAAA, 0x4444, 0x0000};
    #else
        const uint16_t gb_colors[4] = {0xFFFF, 0x9999, 0x5555, 0x0000};
    #endif
#else
    const uint16_t* gb_colors = DEFAULT_PALETTE_NAME;
#endif