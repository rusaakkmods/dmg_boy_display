#pragma once
#include <cstdint>


// Game Boy Color Palettes (RGB565 format)
// Color Order - 0: Lightest, 1: Dark, 2: Lighter, 3: Darkest
// Visit this website providing tools to preview color https://rgbcolorpicker.com/565

static const uint16_t PALETTE_GREEN_SHADES[4] = {0x9772, 0x2A85, 0x64ED, 0x1082};
static const uint16_t PALETTE_YELLOW_SHADES[4] = {0xffa6, 0x6302, 0xb544, 0x18c1};
static const uint16_t PALETTE_TEAL_SHADES[4] = {0x3e77, 0x1a89, 0x2c90, 0x08a2};
static const uint16_t PALETTE_RED_PASTEL_SHADES[4] = {0xca27, 0x50e3, 0x9185, 0x1841};
static const uint16_t PALETTE_GRAY_SHADES[4] = {0x9ccc, 0x39e5, 0x6b49, 0x1081};
static const uint16_t PALETTE_RETRO[4] = {0xcdb1, 0xb284, 0x8574, 0x1147};
static const uint16_t PALETTE_ROMANCE[4] = {0xbcb3, 0xbeca, 0x9ed8, 0x0840};
static const uint16_t PALETTE_MODERN[4] = {0xc6ff, 0x2bd3, 0xbd4f, 0x32ab};
static const uint16_t PALETTE_MODERN2[4] = {0xcedd, 0x2c94, 0xcbb2, 0x0883};
static const uint16_t PALETTE_PEACH[4] = {0xf7be, 0xc4ac, 0xfe94, 0x31d3};
static const uint16_t PALETTE_NEON[4] = {0xffc0, 0x101f, 0x07e0, 0xf800};
static const uint16_t PALETTE_HIGHLIGHT_BLUE[4] = {0xef9e, 0x337a, 0x651c, 0x1084};
