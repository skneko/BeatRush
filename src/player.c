#include "player.h"

#define HIT_ANIMATION_COUNT     2
#define QUICK_JUMP_DISTANCE     0.8
#define QUICK_FALL_DISTANCE     0.8
#define JUMP_SPEED              0.35
#define FALL_SPEED              0.2

#define FLOAT_TIME              650

static PlayerState state;
static Lane current_lane;

static float player_position;

static int remaining_float_time;

static int next_hit_animation;

void player_init(void) {
    state = PLAYER_STATE_RUNNING;
    current_lane = LANE_BOTTOM;

    player_position = 1;

    next_hit_animation = 0;
}

void player_end(void) {
}

PlayerState player_current_state(void) {
    return state;
}

Lane player_current_lane(void) {
    return current_lane;
}

static inline void do_run(void) {
    printf("Player: state (%d) -> RUNNING (%d)\n", state, PLAYER_STATE_RUNNING);

    state = PLAYER_STATE_RUNNING;
    player_position = 1;
    current_lane = LANE_BOTTOM;
}

static inline void do_float(void) {
    printf("Player: state (%d) -> FLOATING (%d)\n", state, PLAYER_STATE_FLOATING);

    state = PLAYER_STATE_FLOATING;
    player_position = 0;
    current_lane = LANE_TOP;

    remaining_float_time = FLOAT_TIME;
}

static inline void do_hit(void) {
    printf("Player: state (%d) -> HITTING (%d)\n", state, PLAYER_STATE_HITTING);

    state = PLAYER_STATE_HITTING;

    // TODO: start hit animation

    next_hit_animation = (next_hit_animation + 1) % HIT_ANIMATION_COUNT;
}

static inline void do_jump(void) {
    printf("Player: state (%d) -> JUMPING (%d)\n", state, PLAYER_STATE_JUMPING);

    current_lane = LANE_TRANSITIONING;
    state = PLAYER_STATE_JUMPING;
}

static inline void do_quick_jump(void) {
    printf("Player: state (%d) -> QUICK JUMP ...\n", state);

    do_jump();

    player_position -= QUICK_JUMP_DISTANCE;
    player_position = C2D_Clamp(player_position, 0, 1);
}

static inline void do_fall(void) {
    printf("Player: state (%d) -> FALLING (%d)\n", state, PLAYER_STATE_FALLING);

    current_lane = LANE_TRANSITIONING;
    state = PLAYER_STATE_FALLING;
}

static inline void do_quick_fall(void) {
    printf("Player: state (%d) -> QUICK FALL ...\n", state);

    do_fall();

    player_position += QUICK_FALL_DISTANCE;
    player_position = C2D_Clamp(player_position, 0, 1);
}

void player_hit(void) {
    switch (state) {
    case PLAYER_STATE_RUNNING:
    case PLAYER_STATE_FLOATING:
    case PLAYER_STATE_HITTING: {
        do_hit();
        break;
    }
    case PLAYER_STATE_JUMPING:
    case PLAYER_STATE_FALLING: {}
    }
}

void player_jump(void) {
    switch (state) {
    case PLAYER_STATE_HITTING: {
        if (current_lane == LANE_TOP) {
            break;
        }
        __attribute__ ((fallthrough));
    }
    case PLAYER_STATE_RUNNING: {
        do_jump();
        break;
    }
    case PLAYER_STATE_FALLING: {
        do_quick_jump();
        break;
    }
    case PLAYER_STATE_JUMPING:
    case PLAYER_STATE_FLOATING: {}
    }
}

void player_quick_jump(void) {
    switch (state) {
    case PLAYER_STATE_HITTING: {
        if (current_lane == LANE_TOP) {
            break;
        }
        __attribute__ ((fallthrough));
    }
    case PLAYER_STATE_RUNNING: 
    case PLAYER_STATE_FALLING:
    case PLAYER_STATE_JUMPING: {
        do_quick_jump();
        break;
    }
    case PLAYER_STATE_FLOATING: {}
    }
}

void player_quick_fall(void) {
    switch (state) {
    case PLAYER_STATE_HITTING: {
        if (current_lane == LANE_BOTTOM) {
            break;
        }
        __attribute__ ((fallthrough));
    }
    case PLAYER_STATE_FLOATING:
    case PLAYER_STATE_FALLING:
    case PLAYER_STATE_JUMPING: {
        do_quick_fall();
        break;
    }
    case PLAYER_STATE_RUNNING: {}
    }
}

void player_update(unsigned int dt) {
    switch (state) {
    case PLAYER_STATE_HITTING: {
        if (current_lane == LANE_BOTTOM) {  // FIXME: do this after animation done
            do_run();
        } else {
            do_float();
        }
        break;
    }
    case PLAYER_STATE_FLOATING: {
        remaining_float_time -= dt;
        if (remaining_float_time < 0) {
            do_fall();
        }
        break;
    }
    case PLAYER_STATE_FALLING: {
        player_position += FALL_SPEED;

        if (player_position >= 1) {
            do_run();
        }
        break;
    }
    case PLAYER_STATE_JUMPING: {
        player_position -= JUMP_SPEED;

        if (player_position <= 0) {
            do_float();
        }
        break;
    }
    case PLAYER_STATE_RUNNING: {}
    }
}

static void set_calculated_player_pos(C2D_Sprite *player_sprite) {
	const float bottom_y = TOP_SCREEN_HEIGHT - LANE_BOTTOM_MARGIN - LANE_HEIGHT / 2;
	const float top_y = LANE_TOP_MARGIN + LANE_HEIGHT / 2;

	float dy = (bottom_y - top_y) * player_position;

	C2D_SpriteSetPos(player_sprite, HITLINE_LEFT_MARGIN, top_y + dy);
}

void player_draw(C2D_Sprite char_sprites[]) {   // FIXME: don't accept whole array?
switch (state) {
    case PLAYER_STATE_HITTING: {
        // TODO
        break;
    }
    case PLAYER_STATE_FLOATING: {
        // TODO
        break;
    }
    case PLAYER_STATE_FALLING: {
        // TODO
        break;
    }
    case PLAYER_STATE_JUMPING: {
        // TODO
        break;
    }
    case PLAYER_STATE_RUNNING: {
        // TODO
        break;
    }
    }

    C2D_Sprite *player_sprite = &char_sprites[0];
	set_calculated_player_pos(player_sprite);
	C2D_DrawSprite(player_sprite);
}
