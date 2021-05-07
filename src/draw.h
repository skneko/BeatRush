#ifndef DRAW_H
#define DRAW_H

#include "common.h"

inline void draw_sideways_triangle(float x, float y, float base, float height, float scale_x, float scale_y, u32 color, float depth) {
    x -= base / 2;
    C2D_DrawTriangle(
        x, y, color, 
        x + height * scale_x, y + (base / 2) * scale_y, color, 
        x, y + base * scale_y, color, 
        depth);
}

inline bool flicker_is_visible(long long time, int period, int visible) {
    return (time % period) < visible;
}

inline C2D_SpriteSheet load_sprite_sheet(const char *path) {
	C2D_SpriteSheet sheet = C2D_SpriteSheetLoad(path);

	if (!sheet) {
		printf("Failed to load sprite sheet: %s", path);
		svcBreak(USERBREAK_PANIC);
	}

	return sheet;
}

#endif