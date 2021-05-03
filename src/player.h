#include "common.h"

typedef enum _PlayerState {
    PLAYER_STATE_RUNNING,
    PLAYER_STATE_FLOATING,
    PLAYER_STATE_HITTING,
    PLAYER_STATE_JUMPING,
    PLAYER_STATE_FALLING
} PlayerState;

typedef enum _Lane {
    LANE_TOP,
    LANE_BOTTOM,
    LANE_TRANSITIONING
} Lane;

PlayerState player_current_state(void);
Lane player_current_lane(void);

void player_hit(void);
void player_jump(void);
void player_quick_jump(void);
void player_quick_fall(void);

void player_init(void);
void player_end(void);

void player_update(unsigned int dt);
void player_draw(C2D_Sprite char_sprites[]);