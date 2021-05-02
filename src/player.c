#include "player.h"

#define HIT_ANIMATION_COUNT     2
#define QUICK_JUMP_DISTANCE     0.8
#define QUICK_FALL_DISTANCE     0.8
#define JUMP_SPEED              0.3
#define FALL_SPEED              0.3

static PlayerState state;
static Lane current_lane;

static float player_position;

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

static inline void perform_run(void) {
    state = PLAYER_STATE_RUNNING;
    player_position = 0;
    current_lane = LANE_BOTTOM;
}

static inline void perform_float(void) {
    state = PLAYER_STATE_FLOATING;
    player_position = 1;
    current_lane = LANE_TOP;
}

static inline void perform_hit(void) {
    state = PLAYER_STATE_HITTING;

    // TODO: start hit animation

    next_hit_animation = (next_hit_animation + 1) % HIT_ANIMATION_COUNT;
}

static inline void perform_jump(void) {
    current_lane = LANE_TRANSITIONING;

    state = PLAYER_STATE_JUMPING;
}

static inline void perform_quick_jump(void) {
    perform_jump();

    player_position -= QUICK_JUMP_DISTANCE;
}

static inline void perform_fall(void) {
    current_lane = LANE_TRANSITIONING;

    state = PLAYER_STATE_FALLING;
}

static inline void perform_quick_fall(void) {
    perform_fall();

    player_position += QUICK_FALL_DISTANCE;
}

void player_hit(void) {
    switch (state) {
    case PLAYER_STATE_RUNNING:
    case PLAYER_STATE_FLOATING:
    case PLAYER_STATE_HITTING: {
        perform_hit();
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
        perform_jump();
        break;
    }
    case PLAYER_STATE_FALLING: {
        perform_quick_jump();
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
        perform_quick_jump();
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
        perform_quick_fall();
        break;
    }
    case PLAYER_STATE_RUNNING: {}
    }
}

void player_update(void) {
    switch (state) {
    case PLAYER_STATE_HITTING: {
        state = current_lane == LANE_BOTTOM ? PLAYER_STATE_RUNNING : PLAYER_STATE_FLOATING; // FIXME: do this after animation done
        break;
    }
    case PLAYER_STATE_FLOATING: {
        state = PLAYER_STATE_FALLING; // FIXME: do this after some floating time
        break;
    }
    case PLAYER_STATE_FALLING: {
        player_position += FALL_SPEED;

        if (player_position >= 1) {
            perform_run();
        }
        break;
    }
    case PLAYER_STATE_JUMPING: {
        player_position -= JUMP_SPEED;

        if (player_position <= 0) {
            perform_float();
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
