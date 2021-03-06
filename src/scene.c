#include "scene.h"

#include "common.h"
#include "audio.h"

Beatmap * beatmap;
Note *first_note_to_draw;
unsigned int remaining_notes;
float speed;

C2D_TextBuf songTimeLabelBuf;

void scene_init(Beatmap *const _beatmap) {
    beatmap = _beatmap;
    first_note_to_draw = beatmap->notes;
    remaining_notes = beatmap->note_count;
    speed = beatmap->approach_time / (TOP_SCREEN_WIDTH - HITLINE_LEFT_MARGIN);

    songTimeLabelBuf = C2D_TextBufNew(10);
}

void scene_end(void) {
    C2D_TextBufDelete(songTimeLabelBuf);
}

/* const unsigned short approach_time = 1400;
const long unsigned int note_time = 10000;
const float speed = approach_time / (TOP_SCREEN_WIDTH - HITLINE_LEFT_MARGIN);

static void draw_test_note(void) {
    long long int note_remaining_time = (long long int)note_time - audioPlaybackPosition();
    float note_x = (float)note_remaining_time / speed + HITLINE_LEFT_MARGIN;

    if (note_x < -LANE_HEIGHT && note_x > TOP_SCREEN_WIDTH) {
        return;
    }

    C2D_DrawCircle(
        note_x, LANE_TOP_MARGIN + NOTE_RADIUS, 0.0f, NOTE_RADIUS,
        C2D_PURPLE, C2D_PURPLE, C2D_PURPLE, C2D_PURPLE);
} */

typedef enum _NoteDrawingResult {
    NOTE_DRAWN,
    NOTE_BEHIND,
    NOTE_AHEAD
} NoteDrawingResult;

static NoteDrawingResult draw_note(const Note *const note) {
    long long int note_remaining_time = (long long int)note->position - audioPlaybackPosition();
    float note_x = (float)note_remaining_time / speed + HITLINE_LEFT_MARGIN;

    if (note_x < -LANE_HEIGHT) {
        return NOTE_BEHIND;
    }
    if (note_x > TOP_SCREEN_WIDTH) {
        return NOTE_AHEAD;
    }

    float lane_y;
    if (note->topLane) {
        lane_y = LANE_TOP_MARGIN + NOTE_RADIUS;
    } else {
        lane_y = TOP_SCREEN_HEIGHT - LANE_BOTTOM_MARGIN - LANE_HEIGHT + NOTE_RADIUS;
    }

    C2D_DrawCircle(
        note_x, lane_y, 0.0f, NOTE_RADIUS,
        C2D_PURPLE, C2D_PURPLE, C2D_PURPLE, C2D_PURPLE);

    return NOTE_DRAWN;
}

static void draw_notes(void) {
    printf("----------- %u\n", remaining_notes);

    Note *note_to_draw = first_note_to_draw;
    unsigned int remaining_notes_temp = remaining_notes;

    bool keep_advancing = true;
    while (keep_advancing && remaining_notes_temp > 0) {
        switch (draw_note(note_to_draw)) {
            case NOTE_BEHIND: {
                printf("Left behind note at %lums.\n", note_to_draw->position);
                remaining_notes--;
                first_note_to_draw++;
                break;
            }

            case NOTE_DRAWN: {
                printf("Drawn note at %lums.\n", note_to_draw->position);
                break;
            }

            default:
            case NOTE_AHEAD: {
                printf("Found note ahead at %lums, stop.\n", note_to_draw->position);
                keep_advancing = false;
                break;
            }
        }

        remaining_notes_temp--;
        note_to_draw++;
    }
}

static void draw_debug_song_time(void) {
    C2D_Text songTimeLabel;
    char buf[10];

    C2D_TextBufClear(songTimeLabelBuf);
    snprintf(buf, sizeof(buf), "%05ld", audioPlaybackPosition());
    C2D_TextParse(&songTimeLabel, songTimeLabelBuf, buf);
    C2D_TextOptimize(&songTimeLabel);
    C2D_DrawText(
        &songTimeLabel, C2D_WithColor | C2D_AtBaseline, 
        230.0f, 220.0f, DEBUG_DEPTH, 0.5f, 0.5f, 
        C2D_WHITE);
}

static void draw_debug_progress_bar(void) {
    float progress = (float)audioPlaybackPosition() / audioLength();
    C2D_DrawRectangle(
        0, TOP_SCREEN_HEIGHT - 3.0f, 
        DEBUG_DEPTH, 
        TOP_SCREEN_WIDTH * progress, 3.0f, 
        C2D_RED, C2D_RED, C2D_RED, C2D_RED);
}

static void draw_debug_rulers(void) {
    // top lane
    C2D_DrawLine(
        0, LANE_TOP_MARGIN, C2D_GREEN,
        TOP_SCREEN_WIDTH, LANE_TOP_MARGIN, C2D_GREEN,
        1.0f, DEBUG_DEPTH);
    C2D_DrawLine(
        0, LANE_TOP_MARGIN + LANE_HEIGHT, C2D_GREEN,
        TOP_SCREEN_WIDTH, LANE_TOP_MARGIN + LANE_HEIGHT, C2D_GREEN,
        1.0f, DEBUG_DEPTH);

    // bottom lane
    C2D_DrawLine(
        0, TOP_SCREEN_HEIGHT - LANE_BOTTOM_MARGIN, C2D_GREEN,
        TOP_SCREEN_WIDTH, TOP_SCREEN_HEIGHT - LANE_BOTTOM_MARGIN, C2D_GREEN,
        1.0f, DEBUG_DEPTH);
    C2D_DrawLine(
        0, TOP_SCREEN_HEIGHT - LANE_BOTTOM_MARGIN - LANE_HEIGHT, C2D_GREEN,
        TOP_SCREEN_WIDTH, TOP_SCREEN_HEIGHT - LANE_BOTTOM_MARGIN - LANE_HEIGHT, C2D_GREEN,
        1.0f, DEBUG_DEPTH);

    // hitline
    C2D_DrawLine(
        HITLINE_LEFT_MARGIN, 0.0f, C2D_WHITE,
        HITLINE_LEFT_MARGIN, TOP_SCREEN_HEIGHT, C2D_WHITE,
        1.0f, DEBUG_DEPTH);

    // hitpoints
    C2D_DrawLine(
        HITLINE_LEFT_MARGIN - 5.0f, LANE_TOP_MARGIN + LANE_HEIGHT / 2, C2D_GREEN,
        HITLINE_LEFT_MARGIN + 5.0f, LANE_TOP_MARGIN + LANE_HEIGHT / 2, C2D_GREEN,
        3.0f, DEBUG_DEPTH);
    C2D_DrawLine(
        HITLINE_LEFT_MARGIN - 5.0f, TOP_SCREEN_HEIGHT - LANE_BOTTOM_MARGIN - LANE_HEIGHT / 2, C2D_GREEN,
        HITLINE_LEFT_MARGIN + 5.0f, TOP_SCREEN_HEIGHT - LANE_BOTTOM_MARGIN - LANE_HEIGHT / 2, C2D_GREEN,
        3.0f, DEBUG_DEPTH);
}

static void draw_debug_overlay(void) {
    draw_debug_song_time();
    draw_debug_progress_bar();

    draw_debug_rulers();
}

void scene_draw(void) {
    audioAdvancePlaybackPosition();

    draw_notes();
    draw_debug_overlay();
}