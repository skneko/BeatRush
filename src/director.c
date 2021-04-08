#include "director.h"

#include <stdlib.h>

#include "menu.h"
#include "scene.h"
#include "logic.h"
#include "audio.h"

Beatmap *beatmap;

GameState state;
bool state_change_requested;
bool quit_requested;
bool using_audio_dt;

u64 previous_time;

void (*init)(void);
void (*update)(unsigned int dt);
void (*draw)(void);
void (*end)(void);
void (*previous_end)(void);

void do_nothing() { }

void director_init(void) {
    init = do_nothing;
    update = do_nothing;
    draw = do_nothing;
    end = do_nothing;
    previous_end = do_nothing;

    state_change_requested = false;
    quit_requested = false;
    using_audio_dt = false;

    beatmap = beatmap_load_from_file("romfs:/beatmaps/allYouAre/beatmap.btrm"); //FIXME
}

void director_end(void) {
    if (beatmap != NULL) {  // FIXME
        free(beatmap);
    }
}

void director_change_state(GameState next_state) {
    previous_end = end;
    state_change_requested = true;

    switch (next_state) {
        case SONG_SELECTION_MENU: {
            init = do_nothing;
            update = do_nothing;
            draw = do_nothing;
            end = do_nothing;
            break;
        }
        case RUNNING_BEATMAP:
        {
            init = scene_init;
            update = logic_update;
            draw = scene_draw;
            end = scene_end;
            break;
        }
    }

    state = next_state;
}

bool director_main_loop(void)
{
    if (quit_requested)
    {
        end();
        return false;
    }

    if (state_change_requested) {
        previous_end();
        init();
        state_change_requested = false;
    }

    u64 current_time;
    if (using_audio_dt)
    {
        current_time = audioPlaybackPosition();
    }
    else
    {
        current_time = osGetTime();
    }
    unsigned int dt = (unsigned int)(current_time - previous_time);
    previous_time = current_time;

    update(dt);
    draw();

    return true;
}

void director_request_quit(void) {
    quit_requested = true;
}

void director_set_audio_dt(bool use_audio_dt) {
    using_audio_dt = use_audio_dt;
}