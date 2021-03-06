#ifndef SCENE_H
#define SCENE_H

#include "beatmap.h"

extern void scene_init(Beatmap *const beatmap);
extern void scene_end(void);

extern void scene_draw(void);

#endif