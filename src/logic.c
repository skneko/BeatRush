#include "logic.h"

#include <stdlib.h>

#include "common.h"
#include "audio.h"

#define HIT_WINDOW_PERFECT   43
#define HIT_WINDOW_GOOD      91
#define HIT_WINDOW_OK        139
#define HIT_WINDOW_MISS      (HIT_WINDOW_OK * 2)

#define SCORE_PERFECT        300
#define SCORE_GOOD           100
#define SCORE_OK             50
#define SCORE_MISS           0

static Beatmap *beatmap;
static Note *next_note_to_hit;
static unsigned int remaining_notes_to_hit;

static int global_offset;

static unsigned long score;
static unsigned int hits_perfect;
static unsigned int hits_good;
static unsigned int hits_ok;
static unsigned int hits_miss;

void logic_init(Beatmap *const _beatmap) {
    beatmap = _beatmap;
    next_note_to_hit = beatmap->notes;
    remaining_notes_to_hit = beatmap->note_count;
    global_offset = beatmap->start_offset;
}

void logic_end(void) {

}

static void increase_score(int increment) {
    score += increment;
}

static void check_note(void) {
    if (remaining_notes_to_hit <= 0) {
        return;
    }

    unsigned long actual_position = audioPlaybackPosition();
    unsigned long expected_position = next_note_to_hit->position;

    long diff = actual_position + global_offset - expected_position ;
    unsigned long abs_diff = labs(diff);

    printf("HIT %ldms... ", diff);

    if (abs_diff <= HIT_WINDOW_MISS) {
        next_note_to_hit++;
        remaining_notes_to_hit--;
    }

    if (abs_diff <= HIT_WINDOW_PERFECT) {
        printf("PERFECT.\n");
        increase_score(SCORE_PERFECT);
        hits_perfect++;
    } else if (abs_diff <= HIT_WINDOW_GOOD) {
        printf("GOOD.\n");
        increase_score(SCORE_GOOD);
        hits_good++;
    } else if (abs_diff <= HIT_WINDOW_OK) {
        printf("OK.\n");
        increase_score(SCORE_OK);
        hits_ok++;
    } else if (abs_diff <= HIT_WINDOW_MISS) {
        printf("MISS.\n");
        increase_score(SCORE_MISS);
        hits_miss++;
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
        last_chance_position = audioPlaybackPosition() + global_offset - HIT_WINDOW_MISS;
    }
    
    //printf("%lu < %lu\n", next_note_to_hit->position, last_chance_position);
    while ( remaining_notes_to_hit > 0 && 
            next_note_to_hit->position < last_chance_position) 
    {
        printf("Lost chance for note at %lu.\n", next_note_to_hit->position);

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

unsigned long logic_score(void) {
    return score;
}