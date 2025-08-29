#pragma once

#include <cstdint>

class Scaler {
public:
    static void buildScaleMaps(int* xmap, int* ymap, 
                              int source_w, int source_h,
                              int scaled_w, int scaled_h,
                              float display_scale);
};
