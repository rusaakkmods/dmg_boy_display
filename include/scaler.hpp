#pragma once

#include <cstdint>

void buildScaleMaps(int* xmap, int* ymap, 
                    int source_w, int source_h,
                    int scaled_w, int scaled_h,
                    float display_scale);

void scale_frame(const uint16_t* src, uint16_t* dst, 
                 const int* xmap, const int* ymap,
                 int src_w, int scaled_w, int scaled_h);
