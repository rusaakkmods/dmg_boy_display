#pragma once
#include <cstdint>

// Fast ordered 8x8 Bayer dither that maps a RGB565 buffer in-place to
// black/white using the provided palette mapping.
// - buf: pointer to w*h RGB565 pixels (will be modified in-place)
// - w/h: width and height
// - palette: array of 4 RGB565 colors [lightest, light, dark, darkest]
// - bw_white / bw_black: output RGB565 values to write for white/black
void fast_bayer_dither(uint16_t* buf, int w, int h, const uint16_t palette[4], uint16_t bw_white, uint16_t bw_black);

// High quality Floyd-Steinberg error diffusion dithering
// Better quality than Bayer but slightly more computational cost
void floyd_steinberg_dither(uint16_t* buf, int w, int h, const uint16_t palette[4], uint16_t bw_white, uint16_t bw_black);

