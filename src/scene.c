#include "scene.h"

#include "common.h"
#include "audio.h"
#include "logic.h"
#include "draw.h"

static Beatmap * beatmap;
static Note *next_note_to_draw;
static unsigned int remaining_notes_to_draw;
static float speed;

static bool in_rest;

#define OVER_UI_DEPTH               0.5f
#define PAUSE_MENU_DEPTH            0.9f

#define SCORE_LABEL_BUF_SIZE        13
#define COMBO_LABEL_BUF_SIZE        13
#define HEALTH_ICON_AREA_BUF_SIZE   41
#define REST_TIME_LABEL_BUF_SIZE    13
#define DYN_TEXT_BUF_SIZE           4096

#define COMBO_DRAW_THRESHOLD        5
#define COMBO_BASE_COLOR            C2D_WHITE
#define COMBO_HIGHLIGHT_THRESHOLD   100
#define COMBO_HIGHLIGHT_COLOR       C2D_ORANGE
#define COMBO_FULL_COLOR            C2D_ORANGERED

#define ATTENTION_REST_THRESHOLD    3000
#define ATTENTION_WARN_THRESHOLD    2500
#define ATTENTION_WARN_MARGIN_X     20
#define ATTENTION_WARN_MARGIN_Y     20
#define ATTENTION_WARN_BASE         10
#define ATTENTION_WARN_HEIGHT       14
#define ATTENTION_WARN_PERIOD       210
#define ATTENTION_WARN_VISIBLE      100

C2D_TextBuf dynamicTextBuf;

void scene_init(Beatmap *const _beatmap) {
    beatmap = _beatmap;
    next_note_to_draw = beatmap->notes;
    remaining_notes_to_draw = beatmap->note_count;
    speed = beatmap->approach_time / (TOP_SCREEN_WIDTH - HITLINE_LEFT_MARGIN);

    in_rest = true;

    dynamicTextBuf = C2D_TextBufNew(DYN_TEXT_BUF_SIZE);
}

void scene_end(void) {
    C2D_TextBufDelete(dynamicTextBuf);
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

    if (!note->hidden) {
        C2D_DrawCircleSolid(
            note_x, lane_y, 0.0f, NOTE_RADIUS,
            C2D_PURPLE);
        C2D_DrawLine(note_x, lane_y - NOTE_RADIUS, C2D_RED,
            note_x, lane_y + NOTE_RADIUS, C2D_RED,
            1.0f, 0.1f);
    }

    return NOTE_DRAWN;
}

static void draw_notes(void) {
    //printf("----------- %u\n", remaining_notes_to_draw);

    Note *note_to_draw = next_note_to_draw;
    unsigned int remaining_notes_temp = remaining_notes_to_draw;

    bool keep_advancing = true;
    while (keep_advancing && remaining_notes_temp > 0) {
        switch (draw_note(note_to_draw)) {
            case NOTE_BEHIND: {
                //printf("Left behind note at %lums.\n", note_to_draw->position);
                remaining_notes_to_draw--;
                next_note_to_draw++;
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

    C2D_TextBufClear(dynamicTextBuf);
    snprintf(buf, sizeof(buf), "%06lu", score);
    C2D_TextParse(&scoreLabel, dynamicTextBuf, buf);
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
        C2D_TextBufClear(dynamicTextBuf);
        snprintf(buf, sizeof(buf), "%u", combo);
        C2D_TextParse(&comboLabel, dynamicTextBuf, buf);
        C2D_TextOptimize(&comboLabel);
        C2D_DrawText(
            &comboLabel, C2D_WithColor | C2D_AtBaseline | C2D_AlignCenter, 
            200.0f, 25.0f, 0.0f, 1.0f, 1.0f, 
            color);
    }
}

static void draw_health(void) {
    C2D_Text healthIconArea;
    char buf[HEALTH_ICON_AREA_BUF_SIZE];

    unsigned int health = logic_health();

    C2D_TextBufClear(dynamicTextBuf);
    
    buf[0] = '\0';
    for (int i = 0; i < health; i++) {
        strcat(buf, "♥");
    }

    C2D_TextParse(&healthIconArea, dynamicTextBuf, buf);
    C2D_TextOptimize(&healthIconArea);
    C2D_DrawText(
        &healthIconArea, C2D_WithColor | C2D_AtBaseline | C2D_AlignCenter, 
        130.0f, 230.0f, 0.0f, 1.5f, 1.0f, 
        logic_is_invencible() ? C2D_BLUE : C2D_RED);
}

static void draw_attention_warnings(long long time_until_next) {
    if (flicker_is_visible(time_until_next, ATTENTION_WARN_PERIOD, ATTENTION_WARN_VISIBLE)) 
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

static void draw_attention_cues(void) {
    long long time_until_next = (long long)next_note_to_draw->position - audioPlaybackPosition();

    if (time_until_next > ATTENTION_REST_THRESHOLD) {
        in_rest = true;
    }

    if (in_rest) {
        if (time_until_next <= beatmap->approach_time) {
            in_rest = false;
            return;
        }

        C2D_Text restTimeLabel;
        char buf[REST_TIME_LABEL_BUF_SIZE];
        C2D_TextBufClear(dynamicTextBuf);

        if (time_until_next < ATTENTION_WARN_THRESHOLD)
        {
            draw_attention_warnings(time_until_next);
            snprintf(buf, sizeof(buf), "READY");
        } else {
            snprintf(buf, sizeof(buf), "%0.3f", time_until_next / 1000.0f);
        }

        C2D_TextParse(&restTimeLabel, dynamicTextBuf, buf);
        C2D_TextOptimize(&restTimeLabel);
        C2D_DrawText(
            &restTimeLabel, C2D_WithColor | C2D_AtBaseline,
            TOP_SCREEN_CENTER_HOR - 20, TOP_SCREEN_CENTER_VER + 5, OVER_UI_DEPTH, 0.5f, 0.5f,
            C2D_WHITE);
    }
}

static void draw_debug_song_time(void) {
    C2D_Text songTimeLabel;
    char buf[10];

    C2D_TextBufClear(dynamicTextBuf);
    snprintf(buf, sizeof(buf), "%05ld", audioPlaybackPosition());
    C2D_TextParse(&songTimeLabel, dynamicTextBuf, buf);
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

void draw_pause(void) {
    C2D_Text pauseLabel;
    char buf[10];

    C2D_TextBufClear(dynamicTextBuf);
    snprintf(buf, sizeof(buf), "PAUSE");
    C2D_TextParse(&pauseLabel, dynamicTextBuf, buf);
    C2D_TextOptimize(&pauseLabel);
    C2D_DrawText(
        &pauseLabel, C2D_WithColor | C2D_AlignCenter, 
        TOP_SCREEN_CENTER_HOR, TOP_SCREEN_CENTER_VER - NOTE_RADIUS - 5, PAUSE_MENU_DEPTH, 1.5f, 1.5f, 
        C2D_WHITE);
}

void scene_draw(void) {
    if (audioIsPaused()) {
        draw_pause();
    }

    draw_notes();

    draw_score();
    draw_combo();
    draw_health();

    draw_attention_cues();

    draw_debug_overlay();
}