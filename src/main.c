#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "common.h"
#include "debug.h"
#include "audio.h"

C3D_RenderTarget *top_left;

C2D_SpriteSheet test_spritesheet;

void main_loop(void);
void load_sprites(void);
void draw_test_screen(void);
void sprite_from_sheet(C2D_Sprite *sprite, C2D_SpriteSheet sheet, size_t index);
void draw_sprite(C2D_Sprite *sprite, float x, float y, float depth, float radians);
void draw_debug_overlay(void);

int main(int argc, char *argv[]) {
    /* Enable N3DS 804MHz operation, where available */
    osSetSpeedupEnable(true);

    /* Initialize ROM filesystem */
    romfsInit();

    /* Initialize debug subsystems */
    init_debug_log();
    printf("Debug log is enabled.\n");

    /* Initialize screen targets */
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    top_left = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    consoleInit(GFX_BOTTOM, NULL); // FIXME: temporary video+console combo for debug

    printf("Screen targets initialized.\n");

    /* Initialize audio subsystem */
    audioInit();

    /* Seed random number generator */
    srand(time(NULL));

    /* Game code */
    load_sprites();
    
    main_loop();

    /* Finalize engine */
    C2D_Fini();
    C3D_Fini();
    gfxExit();

    /* Finalize audio */
    audioExit();

    return EXIT_SUCCESS;
}

const unsigned short approach_time = 1400;
const long unsigned int note_time = 10000;
const float speed = approach_time / (TOP_SCREEN_WIDTH - HITLINE_LEFT_MARGIN);

long unsigned int last_key_time = 0;

void main_loop(void) {
    while (aptMainLoop()) {
        hidScanInput();

        /* Begin frame */
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(top_left, C2D_BLACK);
        C2D_SceneBegin(top_left);

        draw_test_screen();
        draw_debug_overlay();

        /* Quit on START */
        u32 k_down = hidKeysDown();
        if (k_down & KEY_A) {
            last_key_time = audioPlaybackPosition();
            printf("Pressed %lld off.\n", (long long int)last_key_time - note_time);
        }
        if (k_down & KEY_START) {
            printf("\n** Quitting... **\n");
            break;
        }

        /* End frame */
        C2D_Flush();
        C3D_FrameEnd(0);
    }
}

C2D_TextBuf dynamicTextBuf;

void load_sprites(void) {
    test_spritesheet = C2D_SpriteSheetLoad("romfs:/gfx/test_sprites.t3x");
    if (!test_spritesheet) {
        debug_printf("Failed to load spritesheet");
    }

    dynamicTextBuf = C2D_TextBufNew(10);
}

void draw_test_note(void) {
    long long int note_remaining_time = (long long int)note_time - audioPlaybackPosition();
    float note_x = (float)note_remaining_time / speed + HITLINE_LEFT_MARGIN;

    if (note_x < -LANE_HEIGHT && note_x > TOP_SCREEN_WIDTH) {
        return;
    }

    C2D_DrawCircle(
        note_x, LANE_TOP_MARGIN + NOTE_RADIUS, 0.0f, NOTE_RADIUS,
        C2D_PURPLE, C2D_PURPLE, C2D_PURPLE, C2D_PURPLE);
}

int x = 100, y = 100;
int prev_pos = 0;

void draw_test_screen(void) {
    C2D_DrawCircle(TOP_SCREEN_CENTER_HOR,
                    TOP_SCREEN_CENTER_VER,
                    0,
                    50,
                    C2D_RED,
                    C2D_BLUE,
                    C2D_GREEN,
                    C2D_WHITE);

    C2D_Sprite test_ship;
    sprite_from_sheet(&test_ship, test_spritesheet, 0);
    draw_sprite(&test_ship, (float)x, (float)y, 0, 0);

    if (audioAdvancePlaybackPosition()) {
        int delta = audioPlaybackPosition() - prev_pos;
        x = (x + delta / 16) % 200;
        y = (y + delta / 16) % 200;

        prev_pos = audioPlaybackPosition();
    }

    draw_test_note();
}

void sprite_from_sheet(C2D_Sprite *sprite, C2D_SpriteSheet sheet, size_t index) {
    C2D_SpriteFromSheet(sprite, sheet, index);
    C2D_SpriteSetCenter(sprite, 0.5f, 0.5f);
}

void draw_sprite(C2D_Sprite *sprite, float x, float y, float depth, float radians) {
    C2D_SpriteSetPos(sprite, x, y);
    C2D_SpriteSetRotation(sprite, radians);
    C2D_SpriteSetDepth(sprite, depth);
    C2D_DrawSprite(sprite);
}

void draw_debug_song_time(void) {
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

void draw_debug_progress_bar(void) {
    float progress = (float)audioPlaybackPosition() / audioLength();
    C2D_DrawRectangle(
        0, TOP_SCREEN_HEIGHT - 3.0f, 
        DEBUG_DEPTH, 
        TOP_SCREEN_WIDTH * progress, 3.0f, 
        C2D_RED, C2D_RED, C2D_RED, C2D_RED);
}

void draw_debug_rulers(void) {
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

void draw_debug_overlay(void) {
    draw_debug_song_time();
    draw_debug_progress_bar();

    draw_debug_rulers();
}