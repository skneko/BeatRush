#ifndef LOGIC_H
#define LOGIC_H

#include "beatmap.h"

extern void logic_init(Beatmap *const beatmap);
extern void logic_end(void);

extern void logic_update(void);

extern unsigned long logic_score(void);

#endif