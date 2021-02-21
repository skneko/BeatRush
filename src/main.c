#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "common.h"
#include "debug.h"

C3D_RenderTarget *top_left;

C2D_SpriteSheet test_spritesheet;

void main_loop();
void load_sprites();
void draw_test_screen();
void sprite_from_sheet(C2D_Sprite *sprite, C2D_SpriteSheet sheet, size_t index);
void draw_sprite(C2D_Sprite *sprite, float x, float y, float depth, float radians);

int main(int argc, char *argv[]) {
    /* Seed random number generator */
    srand(time(NULL));

    /* Initialize ROM filesystem */
    romfsInit();

    /* Initialize debug */
#ifdef DEBUG_LOG
    init_debug_log();
#endif

    /* Initialize screens targets */
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    top_left = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

    load_sprites();
    
    main_loop();

    /* Finalize engine */
    C2D_Fini();
    C3D_Fini();
    gfxExit();

    return 0;
}

void main_loop() {
    while (aptMainLoop()) {
        hidScanInput();

        /* Begin frame */
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(top_left, C2D_BLACK);
        C2D_SceneBegin(top_left);

        draw_test_screen();

        /* End frame */
        C2D_Flush();
        C3D_FrameEnd(0);
    }
}

void load_sprites() {
    test_spritesheet = C2D_SpriteSheetLoad("romfs:/gfx/test_sprites.t3x");
    if (!test_spritesheet) {
        debug_printf("Failed to load spritesheet");
    }
}

void draw_test_screen() {
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
    draw_sprite(&test_ship, 100, 100, 1, 0);
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