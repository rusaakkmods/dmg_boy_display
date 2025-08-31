#include "dither.hpp"
#include <algorithm>

// Improved 8x8 Bayer matrix with better distribution
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

// More accurate RGB565 to luminance conversion using proper weights
static inline int rgb565_to_luma_accurate(uint16_t pix) {
    // Extract RGB components
    int r = (pix >> 11) & 0x1F;  // 5 bits
    int g = (pix >> 5) & 0x3F;   // 6 bits  
    int b = pix & 0x1F;          // 5 bits
    
    // Convert to 8-bit values with proper scaling
    int r8 = (r << 3) | (r >> 2);  // More accurate 5->8 bit conversion
    int g8 = (g << 2) | (g >> 4);  // More accurate 6->8 bit conversion
    int b8 = (b << 3) | (b >> 2);  // More accurate 5->8 bit conversion
    
    // Use ITU-R BT.601 luma coefficients (more accurate than simple weights)
    return (299 * r8 + 587 * g8 + 114 * b8) / 1000;
}

// Original fast method for backwards compatibility
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

void floyd_steinberg_dither(uint16_t* buf, int w, int h, const uint16_t palette[4], uint16_t bw_white, uint16_t bw_black) {
    // Create a working buffer with error accumulation (use int32 to handle error propagation)
    static int32_t error_buf[128 * 128]; // Assuming max size, adjust if needed
    
    // Calculate target luminance values
    int target_white = rgb565_to_luma_accurate(bw_white);
    int target_black = rgb565_to_luma_accurate(bw_black);
    int mid_luma = (target_white + target_black) / 2;
    
    // Initialize error buffer with input luminance values
    for (int i = 0; i < w * h; i++) {
        error_buf[i] = rgb565_to_luma_accurate(buf[i]);
    }
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = y * w + x;
            int old_pixel = error_buf[idx];
            
            // Determine new pixel value
            uint16_t new_pixel = (old_pixel >= mid_luma) ? bw_white : bw_black;
            int new_luma = (old_pixel >= mid_luma) ? target_white : target_black;
            
            buf[idx] = new_pixel;
            
            // Calculate error
            int error = old_pixel - new_luma;
            
            // Distribute error using Floyd-Steinberg weights
            // Pixel layout: X = current, . = future
            //     X 7
            //   3 5 1
            if (x + 1 < w) {
                error_buf[idx + 1] += (error * 7) / 16;
            }
            if (y + 1 < h) {
                if (x > 0) {
                    error_buf[idx + w - 1] += (error * 3) / 16;
                }
                error_buf[idx + w] += (error * 5) / 16;
                if (x + 1 < w) {
                    error_buf[idx + w + 1] += (error * 1) / 16;
                }
            }
        }
    }
}
