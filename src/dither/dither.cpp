#include "dither.hpp"
#include <cstring>
#include <algorithm>

// Level values (0..255) for the 4 target levels
static const int LEVEL_VAL[4] = {0, 85, 170, 255};

// 8x8 pattern bytes (MSB-first per row)
static const uint8_t dither_patterns[4][8] = {
    {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}, // black (0)
    {0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55}, // 50%
    {0x88,0x22,0x44,0x11,0x88,0x22,0x44,0x11}, // 25%
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}  // white (3)
};

static inline int rgb565_to_luma(uint16_t pix) {
    // Approximate luminance from RGB565: use integer weights
    // R:5 bits, G:6 bits, B:5 bits. Expand to 8-bit approximations.
    int r = (pix >> 11) & 0x1F;
    int g = (pix >> 5) & 0x3F;
    int b = pix & 0x1F;
    // Expand to 0..255 roughly
    int r8 = (r * 255) / 31;
    int g8 = (g * 255) / 63;
    int b8 = (b * 255) / 31;
    // luma approx: 0.299 R + 0.587 G + 0.114 B -> integer approx
    return ( (77 * r8) + (150 * g8) + (29 * b8) ) >> 8; // out of 0..255
}

void floyd_steinberg_rgb565_to_4levels(const uint16_t* src_rgb565, uint8_t* levels_out, int w, int h) {
    // Two error rows, padded by 2
    int16_t* err_curr = new int16_t[w + 2];
    int16_t* err_next = new int16_t[w + 2];
    std::memset(err_curr, 0, (w + 2) * sizeof(int16_t));
    std::memset(err_next, 0, (w + 2) * sizeof(int16_t));

    for (int y = 0; y < h; ++y) {
        bool left_to_right = (y % 2 == 0);
        if (left_to_right) {
            for (int x = 0; x < w; ++x) {
                int idx = y * w + x;
                int luma = rgb565_to_luma(src_rgb565[idx]);
                int oldv = luma + err_curr[x + 1];
                if (oldv < 0) oldv = 0; else if (oldv > 255) oldv = 255;
                int k = (oldv + 42) / 85;
                if (k < 0) k = 0; else if (k > 3) k = 3;
                levels_out[idx] = uint8_t(k);
                int err = oldv - LEVEL_VAL[k];
                // distribute
                err_curr[x + 2] += (err * 7) / 16;
                err_next[x + 0] += (err * 3) / 16;
                err_next[x + 1] += (err * 5) / 16;
                err_next[x + 2] += (err * 1) / 16;
            }
        } else {
            for (int x = w - 1; x >= 0; --x) {
                int idx = y * w + x;
                int luma = rgb565_to_luma(src_rgb565[idx]);
                int oldv = luma + err_curr[x + 1];
                if (oldv < 0) oldv = 0; else if (oldv > 255) oldv = 255;
                int k = (oldv + 42) / 85;
                if (k < 0) k = 0; else if (k > 3) k = 3;
                levels_out[idx] = uint8_t(k);
                int err = oldv - LEVEL_VAL[k];
                // mirrored distribution
                err_curr[x + 0] += (err * 7) / 16;
                err_next[x + 2] += (err * 3) / 16;
                err_next[x + 1] += (err * 5) / 16;
                err_next[x + 0] += (err * 1) / 16;
            }
        }
        // move next -> curr
        std::memcpy(err_curr, err_next, (w + 2) * sizeof(int16_t));
        std::memset(err_next, 0, (w + 2) * sizeof(int16_t));
    }

    delete[] err_curr;
    delete[] err_next;
}

bool mapped_dither_bit(uint8_t level, int x, int y) {
    const uint8_t* row = dither_patterns[level & 3];
    uint8_t pattern_byte = row[y & 7];
    return (pattern_byte & (1u << (7 - (x & 7)))) != 0;
}

// Pack one byte for an 8-pixel-wide tile at tile_x*8, tile_y*8 from levels
uint8_t pack_dither_byte_from_levels(const uint8_t* levels, int w, int h, int tile_x, int tile_y) {
    int base_x = tile_x * 8;
    int base_y = tile_y * 8;
    uint8_t out = 0;
    for (int bit = 0; bit < 8; ++bit) {
        int x = base_x + bit;
        int y = base_y;
        if (x >= w || y >= h) {
            out = (out << 1) | 0; // white
            continue;
        }
        uint8_t level = levels[y * w + x];
        bool b = mapped_dither_bit(level, x, y);
        out = (out << 1) | (b ? 1 : 0);
    }
    return out;
}
