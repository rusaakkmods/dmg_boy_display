#pragma once

#include <cstdint>

// Convert an RGB565 buffer (width*height uint16_t) to 4-levels (0..3) using
// Floyd-Steinberg error diffusion. levels_out must be width*height bytes.
void floyd_steinberg_rgb565_to_4levels(const uint16_t* src_rgb565, uint8_t* levels_out, int w, int h);

// Given level 0..3 and absolute coordinates x,y, return true if output pixel
// should be black (1) or white (0) using the 8x8 patterns from the design.
bool mapped_dither_bit(uint8_t level, int x, int y);

// Pack 8 horizontal mapped dither bits into a byte (MSB first) from levels buffer
uint8_t pack_dither_byte_from_levels(const uint8_t* levels, int w, int h, int tile_x, int tile_y);
