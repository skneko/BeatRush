#ifndef DRAW_H
#define DRAW_H

#include "common.h"

inline void draw_sideways_triangle(float x, float y, float base, float height, float scale_x, float scale_y, u32 color, float depth) {
    C2D_DrawTriangle(
        x, y, color, 
        x + height * scale_x, y + (base / 2) * scale_y, color, 
        x, y + base * scale_y, color, 
        depth);
}

inline bool flicker_is_visible(long long time, int period, int visible) {
    return (time % period) < visible;
}

#endif