#ifndef LOGIC_H
#define LOGIC_H

#include "beatmap.h"

extern void logic_init(void);
extern void logic_end(void);

extern void logic_update(unsigned int dt);

extern unsigned long logic_score(void);
extern unsigned int logic_combo(void);
extern bool logic_is_full_combo(void);
extern unsigned short logic_current_health(void);
extern unsigned short logic_max_health(void);
extern bool logic_is_invencible(void);
extern bool logic_has_failed(void);

#endif