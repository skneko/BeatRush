#include "scene.h"

#include "common.h"
#include "audio.h"
#include "logic.h"
#include "draw.h"

static Beatmap * beatmap;
static Note *first_note_to_draw;
static unsigned int remaining_notes_to_draw;
static float speed;

static bool first_note_seen;

#define OVER_UI_DEPTH               0.5f

#define SCORE_LABEL_BUF_SIZE        13
#define COMBO_LABEL_BUF_SIZE        13

#define COMBO_DRAW_THRESHOLD        5
#define COMBO_BASE_COLOR            C2D_WHITE
#define COMBO_HIGHLIGHT_THRESHOLD   100
#define COMBO_HIGHLIGHT_COLOR       C2D_ORANGE
#define COMBO_FULL_COLOR            C2D_ORANGERED

#define ATTENTION_WARN_THRESHOLD    3000
#define ATTENTION_WARN_MARGIN_X     20
#define ATTENTION_WARN_MARGIN_Y     20
#define ATTENTION_WARN_BASE         10
#define ATTENTION_WARN_HEIGHT       14
#define ATTENTION_WARN_PERIOD       210
#define ATTENTION_WARN_VISIBLE      100

C2D_TextBuf songTimeLabelBuf;
C2D_TextBuf scoreLabelBuf;
C2D_TextBuf comboLabelBuf;

void scene_init(Beatmap *const _beatmap) {
    beatmap = _beatmap;
    first_note_to_draw = beatmap->notes;
    remaining_notes_to_draw = beatmap->note_count;
    speed = beatmap->approach_time / (TOP_SCREEN_WIDTH - HITLINE_LEFT_MARGIN);

    songTimeLabelBuf = C2D_TextBufNew(10);
    scoreLabelBuf = C2D_TextBufNew(SCORE_LABEL_BUF_SIZE);
    comboLabelBuf = C2D_TextBufNew(COMBO_LABEL_BUF_SIZE);

    first_note_seen = false;
}

void scene_end(void) {
    C2D_TextBufDelete(songTimeLabelBuf);
}

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
        lane_y = LANE_TOP_MARGIN + NOTE_RADIUS + NOTE_MARGIN;
    } else {
        lane_y = TOP_SCREEN_HEIGHT - LANE_BOTTOM_MARGIN - LANE_HEIGHT + NOTE_RADIUS + NOTE_MARGIN;
    }

    if (!first_note_seen) {
        first_note_seen = true;
    }

    C2D_DrawCircleSolid(
        note_x, lane_y, 0.0f, NOTE_RADIUS,
        C2D_PURPLE);
    C2D_DrawLine(note_x, lane_y - NOTE_RADIUS, C2D_RED,
        note_x, lane_y + NOTE_RADIUS, C2D_RED,
        1.0f, 0.1f);

    return NOTE_DRAWN;
}

static void draw_notes(void) {
    //printf("----------- %u\n", remaining_notes_to_draw);

    Note *note_to_draw = first_note_to_draw;
    unsigned int remaining_notes_temp = remaining_notes_to_draw;

    bool keep_advancing = true;
    while (keep_advancing && remaining_notes_temp > 0) {
        switch (draw_note(note_to_draw)) {
            case NOTE_BEHIND: {
                //printf("Left behind note at %lums.\n", note_to_draw->position);
                remaining_notes_to_draw--;
                first_note_to_draw++;
                break;
            }

            case NOTE_DRAWN: {
                //printf("Drawn note at %lums.\n", note_to_draw->position);
                break;
            }

            default:
            case NOTE_AHEAD: {
                //printf("Found note ahead at %lums, stop.\n", note_to_draw->position);
                keep_advancing = false;
                break;
            }
        }

        remaining_notes_temp--;
        note_to_draw++;
    }
}

static void draw_score(void) {
    C2D_Text scoreLabel;
    char buf[SCORE_LABEL_BUF_SIZE];

    unsigned long score = logic_score();

    C2D_TextBufClear(scoreLabelBuf);
    snprintf(buf, sizeof(buf), "%06lu", score);
    C2D_TextParse(&scoreLabel, scoreLabelBuf, buf);
    C2D_TextOptimize(&scoreLabel);
    C2D_DrawText(
        &scoreLabel, C2D_WithColor | C2D_AtBaseline, 
        290.0f, 25.0f, 0.0f, 0.8f, 0.8f, 
        C2D_WHITE);
}

static void draw_combo(void) {
    C2D_Text comboLabel;
    char buf[COMBO_LABEL_BUF_SIZE];

    unsigned int combo = logic_combo();
    bool is_full = logic_is_full_combo();

    u32 color = COMBO_BASE_COLOR;
    if (combo >= COMBO_HIGHLIGHT_THRESHOLD) {
        if (is_full) {
            color = COMBO_FULL_COLOR;
        } else {
            color = COMBO_HIGHLIGHT_COLOR;
        }
    }

    if (combo > COMBO_DRAW_THRESHOLD) {
        C2D_TextBufClear(comboLabelBuf);
        snprintf(buf, sizeof(buf), "%u", combo);
        C2D_TextParse(&comboLabel, comboLabelBuf, buf);
        C2D_TextOptimize(&comboLabel);
        C2D_DrawText(
            &comboLabel, C2D_WithColor | C2D_AtBaseline | C2D_AlignCenter, 
            200.0f, 25.0f, 0.0f, 1.0f, 1.0f, 
            color);
    }
}

static void draw_attention_warnings(void) {
    long long time_until_first = (long long)first_note_to_draw->position - audioPlaybackPosition();

    if (time_until_first < ATTENTION_WARN_THRESHOLD 
        && flicker_is_visible(time_until_first, ATTENTION_WARN_PERIOD, ATTENTION_WARN_VISIBLE)) 
    {
        float left_x = ATTENTION_WARN_MARGIN_X;
        float right_x = TOP_SCREEN_WIDTH - ATTENTION_WARN_MARGIN_X;
        float top_y = ATTENTION_WARN_MARGIN_Y;
        float bottom_y = TOP_SCREEN_HEIGHT - ATTENTION_WARN_MARGIN_Y;

        draw_sideways_triangle(left_x,  top_y,    ATTENTION_WARN_BASE, ATTENTION_WARN_HEIGHT,  1,  1, C2D_RED, OVER_UI_DEPTH);
        draw_sideways_triangle(right_x, top_y,    ATTENTION_WARN_BASE, ATTENTION_WARN_HEIGHT, -1,  1, C2D_RED, OVER_UI_DEPTH);
        draw_sideways_triangle(left_x,  bottom_y, ATTENTION_WARN_BASE, ATTENTION_WARN_HEIGHT,  1, -1, C2D_RED, OVER_UI_DEPTH);
        draw_sideways_triangle(right_x, bottom_y, ATTENTION_WARN_BASE, ATTENTION_WARN_HEIGHT, -1, -1, C2D_RED, OVER_UI_DEPTH);
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

static void draw_debug_keypress_hint(bool top_lane) {
    float lane_y = (top_lane ? LANE_TOP_MARGIN : TOP_SCREEN_HEIGHT - LANE_BOTTOM_MARGIN - LANE_HEIGHT) + LANE_HEIGHT / 2;

    C2D_DrawCircleSolid(
        HITLINE_LEFT_MARGIN, lane_y, 0.0f, NOTE_RADIUS / 2,
        C2D_ORANGE);
}

static void draw_debug_keypress_hints(void) {
    u32 k_down = hidKeysHeld();

     if (k_down & KEY_A || k_down & KEY_B) {
        draw_debug_keypress_hint(false);
    }
    if (k_down & KEY_X || k_down & KEY_Y) {
        draw_debug_keypress_hint(true);
    }
}

static void draw_debug_overlay(void) {
    draw_debug_song_time();
    draw_debug_progress_bar();

    draw_debug_rulers();

    draw_debug_keypress_hints();
}

void scene_draw(void) {
    draw_notes();
    draw_score();
    draw_combo();

    if (!first_note_seen) {
        draw_attention_warnings();
    }

    draw_debug_overlay();
}