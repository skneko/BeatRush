#include "director.h"

#include <stdlib.h>

#include "menu.h"
#include "scene.h"
#include "logic.h"
#include "audio.h"

Beatmap *beatmap;
C3D_RenderTarget *top_left;
C3D_RenderTarget *bottom;

GameState state;
bool state_change_requested;
bool quit_requested;
bool using_audio_dt;

u64 previous_time;

void (*init)(void);
void (*update)(unsigned int dt);
void (*draw_top)(void);
void (*draw_bottom)(void);
void (*end)(void);
void (*previous_end)(void);

void do_nothing() {
}

void director_init(void) {
	init = do_nothing;
	update = do_nothing;
	draw_top = do_nothing;
	draw_bottom = do_nothing;
	end = do_nothing;
	previous_end = do_nothing;

	state_change_requested = false;
	quit_requested = false;
	using_audio_dt = false;
}

void director_end(void) {
	if (beatmap != NULL) {  // FIXME
		free(beatmap);
	}
}

void director_change_state(GameState next_state) {
#ifdef DEBUG_DIRECTOR
    printf("Director: state change requested from %d to %d\n", state, next_state);
#endif

	previous_end = end;
	state_change_requested = true;

	switch (next_state) {
	case SONG_SELECTION_MENU: {
		init = menu_init;
		update = menu_update;
		draw_top = menu_draw;
		draw_bottom = do_nothing;
		end = menu_end;
		break;
	}

	case RUNNING_BEATMAP:
	{
		init = scene_init;
		update = logic_update;
		draw_top = scene_draw_top;
		draw_bottom = scene_draw_bottom;
		end = scene_end;
		break;
	}
	}

	state = next_state;
}

bool director_main_loop(void){
	if (quit_requested) {
		if (state_change_requested) {
			previous_end();
            state_change_requested = false;
		} else {
			end();
		}
		return false;
	}

	if (state_change_requested) {
#ifdef DEBUG_DIRECTOR
        printf("Director: changing state to %d\n", state);
#endif
		previous_end();
		init();
		state_change_requested = false;
	}

	u64 current_time;
	if (using_audio_dt) {
		current_time = audioPlaybackPosition();
	}else {
		current_time = osGetTime();
	}
	unsigned int dt = (unsigned int)(current_time - previous_time);
	previous_time = current_time;

	update(dt);
	if (state_change_requested) {
		return true;
	}

	C2D_TargetClear(top_left, C2D_BLACK);
	C2D_SceneBegin(top_left);
	draw_top();

#ifndef DEBUG_CONSOLE
	C2D_TargetClear(bottom, C2D_BLACK);
	C2D_SceneBegin(bottom);
	draw_bottom();
#endif

	return true;
}

void director_request_quit(void) {
	quit_requested = true;
}

void director_set_audio_dt(bool use_audio_dt) {
	using_audio_dt = use_audio_dt;

	if (using_audio_dt) {
		previous_time = audioPlaybackPosition();
	} else {
		previous_time = osGetTime();
	}
}