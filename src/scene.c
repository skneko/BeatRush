#include "scene.h"

#include "common.h"
#include "audio.h"

C2D_TextBuf songTimeLabelBuf;

void scene_init(void) {
    songTimeLabelBuf = C2D_TextBufNew(10);
}

void scene_end(void) {
    C2D_TextBufDelete(songTimeLabelBuf);
}

const unsigned short approach_time = 1400;
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

    draw_test_note();
    draw_debug_overlay();
}