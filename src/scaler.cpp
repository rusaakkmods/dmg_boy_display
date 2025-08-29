#include "scaler.hpp"

void Scaler::buildScaleMaps(int* xmap, int* ymap, 
                           int source_w, int source_h,
                           int scaled_w, int scaled_h,
                           float display_scale) {

    for (int x = 0; x < scaled_w; x++) {
        int sx;
        if (display_scale == 1.0f) {
            sx = x;
        } else {
            sx = (x * 1000) / (int)(display_scale * 1000);
        }
        if (sx >= source_w) sx = source_w - 1;
        xmap[x] = sx;
    }
    
    for (int y = 0; y < scaled_h; y++) {
        int sy;
        if (display_scale == 1.0f) {
            sy = y;
        } else {
            sy = (y * 1000) / (int)(display_scale * 1000);
        }
        if (sy >= source_h) sy = source_h - 1;
        ymap[y] = sy;
    }
}

