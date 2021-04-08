#ifndef DIRECTOR_H
#define DIRECTOR_H

#include "common.h"
#include "beatmap.h"

typedef enum _GameState {
    SONG_SELECTION_MENU,
    RUNNING_BEATMAP
} GameState;

extern Beatmap *beatmap;

void director_init(void);
void director_end(void);

bool director_main_loop(void);
void director_change_state(GameState next_state);
void director_request_quit(void);
void director_set_audio_dt(bool use_audio_dt);

#endif