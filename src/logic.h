#ifndef LOGIC_H
#define LOGIC_H

#include "beatmap.h"

typedef enum _HitAssessmentValuation {
    HIT_VAL_PERFECT,
    HIT_VAL_GOOD,
    HIT_VAL_OK,
    HIT_VAL_MISS
} HitAssessmentValuation;

typedef struct _HitAssessment {
    bool valid;
    HitAssessmentValuation valuation;
    unsigned long press_position;
    unsigned long expected_position;
} HitAssessment;

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

extern HitAssessment logic_top_hit_assessment(void);
extern HitAssessment logic_bottom_hit_assessment(void);

#endif