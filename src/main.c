#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "common.h"
#include "debug.h"
#include "audio.h"
#include "scene.h"
#include "beatmap.h"
#include "logic.h"

C3D_RenderTarget *top_left;

C2D_SpriteSheet test_spritesheet;

void main_loop(void);
void load_sprites(void);
void sprite_from_sheet(C2D_Sprite *sprite, C2D_SpriteSheet sheet, size_t index);
void draw_sprite(C2D_Sprite *sprite, float x, float y, float depth, float radians);

static const char *BEATMAP = "bassTelekinesis"; // FIXME

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

    /* Seed random number generator */
    srand(time(NULL));

    /* Game code */
    load_sprites();

    char beatmap_map[100];
    snprintf(beatmap_map, sizeof(beatmap_map), "romfs:/beatmaps/%s/beatmap.btrm", BEATMAP);
    char beatmap_track[100];
    snprintf(beatmap_track, sizeof(beatmap_track), "romfs:/beatmaps/%s/track.opus", BEATMAP);

    Beatmap *beatmap = beatmap_load_from_file(beatmap_map);

    /* Initialize subsystems */
    audioInit(beatmap_track);
    logic_init(beatmap);
    scene_init(beatmap);

    main_loop();

    /* Finalize subsystems */
    scene_end();
    logic_end();
    audioExit();

    free(beatmap);

    /* Finalize engine */
    C2D_Fini();
    C3D_Fini();
    gfxExit();

    

    return EXIT_SUCCESS;
}

void main_loop(void) {
    while (aptMainLoop()) {
        hidScanInput();

        /* Begin frame */
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(top_left, C2D_BLACK);
        C2D_SceneBegin(top_left);

        if (audioAdvancePlaybackPosition()) {
            logic_update();
        }
        scene_draw();
        
        /* Quit on START */
        u32 k_down = hidKeysDown();
        if (k_down & KEY_START) {
            printf("\n** Quitting... **\n");
            break;
        }

        /* End frame */
        C2D_Flush();
        C3D_FrameEnd(0);
    }
}

// FIXME: for reference, delete later
void load_sprites(void) {
    test_spritesheet = C2D_SpriteSheetLoad("romfs:/gfx/test_sprites.t3x");
    if (!test_spritesheet) {
        debug_printf("Failed to load spritesheet");
    }
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