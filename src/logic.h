#ifndef LOGIC_H
#define LOGIC_H

#include "beatmap.h"

typedef enum _Lane {
    LANE_TOP,
    LANE_BOTTOM
} Lane;

extern void logic_init(void);
extern void logic_end(void);

extern void logic_update(unsigned int dt);

extern unsigned long logic_score(void);
extern unsigned int logic_combo(void);
extern bool logic_is_full_combo(void);
extern unsigned short logic_health(void);
extern bool logic_is_invencible(void);
extern bool logic_has_failed(void);
extern Lane logic_target_lane(void);

#endif