#include "scaler.hpp"
#include <cstring>

void buildScaleMaps(int* xmap, int* ymap, 
                    int source_w, int source_h,
                    int scaled_w, int scaled_h,
                    float display_scale) {
    int scale_factor = (int)(display_scale * 1000);
    
    for (int x = 0; x < scaled_w; x++) {
        int sx = (display_scale == 1.0f) ? x : (x * 1000) / scale_factor;
        xmap[x] = (sx >= source_w) ? source_w - 1 : sx;
    }
    
    for (int y = 0; y < scaled_h; y++) {
        int sy = (display_scale == 1.0f) ? y : (y * 1000) / scale_factor;
        ymap[y] = (sy >= source_h) ? source_h - 1 : sy;
    }
}

void scale_frame(const uint16_t* src, uint16_t* dst, 
                 const int* xmap, const int* ymap,
                 int src_w, int scaled_w, int scaled_h) {
    for (int dy = 0; dy < scaled_h; dy++) {
        const uint16_t* srcRow = &src[ymap[dy] * src_w];
        uint16_t* dstRow = &dst[dy * scaled_w];
        
        int dx = 0;
        for (; dx <= scaled_w - 8; dx += 8) {
            dstRow[dx]     = srcRow[xmap[dx]];
            dstRow[dx + 1] = srcRow[xmap[dx + 1]];
            dstRow[dx + 2] = srcRow[xmap[dx + 2]];
            dstRow[dx + 3] = srcRow[xmap[dx + 3]];
            dstRow[dx + 4] = srcRow[xmap[dx + 4]];
            dstRow[dx + 5] = srcRow[xmap[dx + 5]];
            dstRow[dx + 6] = srcRow[xmap[dx + 6]];
            dstRow[dx + 7] = srcRow[xmap[dx + 7]];
        }
        for (; dx < scaled_w; dx++) {
            dstRow[dx] = srcRow[xmap[dx]];
        }
    }
}

