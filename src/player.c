#include "player.h"

#define QUICK_JUMP_DISTANCE     0.75
#define QUICK_FALL_DISTANCE     0.75
#define JUMP_SPEED              0.35
#define FALL_SPEED              0.2

#define FLOAT_TIME              500

#define RUN_ANIM_SPEED		    5 // Delta frames. Best is 5, higher for debug
#define RUN_ANIM_OFFSET         0
#define RUN_ANIM_LIMIT          10

#define HIT_ANIM_COUNT          2
#define HIT_ANIM_SPEED          5
#define HIT_ANIM_OFFSET         10
#define HIT_ANIM_LENGTH         2

#define JUMP_ANIM_SPEED         9
#define JUMP_ANIM_OFFSET        14
#define JUMP_ANIM_LENGTH        2

#define FALL_ANIM_SPEED         5
#define FALL_ANIM_OFFSET        16
#define FALL_ANIM_LENGTH        2

static PlayerState state;
static Lane current_lane;

static float player_position;

static int remaining_float_time;

static unsigned int anim_current_frame;
static unsigned int anim_speed;
static unsigned int real_frames_elapsed;
static unsigned int next_hit_animation;

void player_init(void) {
    state = PLAYER_STATE_RUNNING;
    current_lane = LANE_BOTTOM;

    player_position = 1;

    anim_current_frame = 0;
    anim_speed = RUN_ANIM_SPEED;
    real_frames_elapsed = 0;
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

    anim_speed = RUN_ANIM_SPEED;
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

    anim_current_frame = 0;
    anim_speed = HIT_ANIM_SPEED;
    next_hit_animation = (next_hit_animation + 1) % HIT_ANIM_COUNT;
}

static inline void do_jump(void) {
    printf("Player: state (%d) -> JUMPING (%d)\n", state, PLAYER_STATE_JUMPING);

    current_lane = LANE_TRANSITIONING;
    state = PLAYER_STATE_JUMPING;

    anim_current_frame = 0;
    anim_speed = JUMP_ANIM_SPEED;
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

    anim_current_frame = 0;
    anim_speed = FALL_ANIM_SPEED;
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
    real_frames_elapsed++;

    if (real_frames_elapsed > anim_speed) {
        anim_current_frame++;
        real_frames_elapsed = 0;
    }

    switch (state) {
    case PLAYER_STATE_HITTING: {
        if (anim_current_frame >= HIT_ANIM_LENGTH) {
            if (current_lane == LANE_BOTTOM) {
                do_run();
            } else {
                do_float();
            }
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

void player_draw(C2D_Sprite *player_sprite, C2D_SpriteSheet player_sprite_sheet) {
    switch (state) {
    case PLAYER_STATE_HITTING: {
        int offset = HIT_ANIM_OFFSET + next_hit_animation * HIT_ANIM_LENGTH;
        int image_index = offset + anim_current_frame;
        player_sprite->image = C2D_SpriteSheetGetImage(player_sprite_sheet, image_index);
        break;
    }
    case PLAYER_STATE_FALLING: {
        int image_index = FALL_ANIM_OFFSET + C2D_Clamp(anim_current_frame, 0, FALL_ANIM_LENGTH - 1);
        player_sprite->image = C2D_SpriteSheetGetImage(player_sprite_sheet, image_index);
        break;
    }
    case PLAYER_STATE_JUMPING:
    case PLAYER_STATE_FLOATING: {
        int image_index = JUMP_ANIM_OFFSET + C2D_Clamp(anim_current_frame, 0, JUMP_ANIM_LENGTH - 1);
        player_sprite->image = C2D_SpriteSheetGetImage(player_sprite_sheet, image_index);
        break;
    }
    case PLAYER_STATE_RUNNING: {
        if (anim_current_frame > 1) {
            int image_index = RUN_ANIM_OFFSET + anim_current_frame % RUN_ANIM_LIMIT;
            player_sprite->image = C2D_SpriteSheetGetImage(player_sprite_sheet, image_index);
            break;
        }
    }
    }

	set_calculated_player_pos(player_sprite);
	C2D_DrawSprite(player_sprite);
}
