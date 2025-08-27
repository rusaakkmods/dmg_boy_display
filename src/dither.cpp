#include "dither.hpp"

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

static inline int rgb565_to_luma_fast(uint16_t pix) {
    int r = (pix >> 11) & 0x1F;
    int g = (pix >> 5) & 0x3F;
    int b = pix & 0x1F;
    int r8 = (r * 255) / 31;
    int g8 = (g * 255) / 63;
    int b8 = (b * 255) / 31;
    return ((77 * r8) + (150 * g8) + (29 * b8)) >> 8; // 0..255
}

void fast_bayer_dither(uint16_t* buf, int w, int h, const uint16_t palette[4], uint16_t bw_white, uint16_t bw_black) {
    int L_light = rgb565_to_luma_fast(palette[1]);
    int L_dark  = rgb565_to_luma_fast(palette[2]);
    int thresh_light = (int)(((255 - L_light) * 64 + 127) / 255); // 0..64
    int thresh_dark  = (int)(((255 - L_dark)  * 64 + 127) / 255); // 0..64

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int idx = y * w + x;
            uint16_t col = buf[idx];
            if (col == palette[0]) {
                buf[idx] = bw_white;
            } else if (col == palette[3]) {
                buf[idx] = bw_black;
            } else {
                int bval = bayer8[y & 7][x & 7];
                int t = (col == palette[1]) ? thresh_light : thresh_dark;
                bool black = bval < t;
                buf[idx] = black ? bw_black : bw_white;
            }
        }
    }
}
