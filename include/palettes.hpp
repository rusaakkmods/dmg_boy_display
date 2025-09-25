#pragma once
#include <cstdint>


// Game Boy Color Palettes (RGB565 format)
// Color Order - 0: Lightest, 1: Dark, 2: Lighter, 3: Darkest
// Visit this website providing tools to preview color https://rgbcolorpicker.com/565

static const uint16_t GREEN_SHADES[4] = {0x9772, 0x2A85, 0x64ED, 0x1082};
static const uint16_t YELLOW_SHADES[4] = {0xffa6, 0x6302, 0xb544, 0x18c1};
static const uint16_t TEAL_SHADES[4] = {0x3e77, 0x1a89, 0x2c90, 0x08a2};
static const uint16_t RED_PASTEL_SHADES[4] = {0xca27, 0x50e3, 0x9185, 0x1841};
static const uint16_t GRAY_SHADES[4] = {0x9ccc, 0x39e5, 0x6b49, 0x1081};

