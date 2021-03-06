#include "logic.h"

#include <stdlib.h>

#include "common.h"
#include "audio.h"

#define HIT_WINDOW_PERFECT   43
#define HIT_WINDOW_GOOD      91
#define HIT_WINDOW_OK        139
#define HIT_WINDOW_MISS      HIT_WINDOW_OK * 2

static Beatmap *beatmap;
static Note *next_note_to_hit;
static unsigned int remaining_notes_to_hit;

void logic_init(Beatmap *const _beatmap) {
    beatmap = _beatmap;
    next_note_to_hit = beatmap->notes;
    remaining_notes_to_hit = beatmap->note_count;
}

void logic_end(void) {

}

static void check_note(void) {
    if (remaining_notes_to_hit <= 0) {
        return;
    }

    unsigned long actual_position = audioPlaybackPosition();
    unsigned long expected_position = next_note_to_hit->position;

    long diff = actual_position - expected_position;
    unsigned long abs_diff = labs(diff);

    printf("HIT %ldms... ", diff);

    if (abs_diff <= HIT_WINDOW_MISS) {
        next_note_to_hit++;
        remaining_notes_to_hit--;
    }

    if (abs_diff <= HIT_WINDOW_PERFECT) {
        printf("PERFECT.\n");
    } else if (abs_diff <= HIT_WINDOW_GOOD) {
        printf("GOOD.\n");
    } else if (abs_diff <= HIT_WINDOW_OK) {
        printf("OK.\n");
    } else if (abs_diff <= HIT_WINDOW_MISS) {
        printf("MISS.\n");
    } else {
        printf("ignored.\n");
    }
}

static void action_hit(void) {
    if (!next_note_to_hit->topLane) {
        check_note();
    } else {
        printf("HIT wrong lane.\n");
    }
}

static void action_jump(void) {
    if (next_note_to_hit->topLane) {
        check_note();
    } else {
        printf("HIT wrong lane.\n");
    }
}

static void advance_notes(void) {
    unsigned long last_chance_position = 0;
    if (audioPlaybackPosition() > HIT_WINDOW_MISS) {
        last_chance_position = audioPlaybackPosition() - HIT_WINDOW_MISS;
    }
    
    while ( remaining_notes_to_hit > 0 && 
            next_note_to_hit->position < last_chance_position) 
    {
        next_note_to_hit++;
        remaining_notes_to_hit--;
    }
}

void logic_update(void) {
    advance_notes();

    u32 k_down = hidKeysDown();

    if (k_down & KEY_A || k_down & KEY_B) {
        action_hit();
    }
    if (k_down & KEY_X || k_down & KEY_Y) {
        action_jump();
    }
}