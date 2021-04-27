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
#include "director.h"

C3D_RenderTarget *top_left;

C2D_SpriteSheet test_spritesheet;
C2D_SpriteSheet run_char_anim;

void main_loop(void){
	while (aptMainLoop()) {
		hidScanInput();

		/* Begin frame */
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		C2D_TargetClear(top_left, C2D_BLACK);
		C2D_SceneBegin(top_left);

		if (!director_main_loop()) {
			return;
		}

		/* End frame */
		C2D_Flush();
		C3D_FrameEnd(0);
	}
}

int main(){
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

	/* Initialize subsystems */
	audioInit();
	director_init();
	director_set_audio_dt(false);
	director_change_state(SONG_SELECTION_MENU);

	main_loop();

	/* Finalize subsystems */
	director_end();
	audioExit();

	/* Finalize engine */
	C2D_Fini();
	C3D_Fini();
	gfxExit();

	return EXIT_SUCCESS;
}

// FIXME: for reference, delete later
// FIXME: modified but never called?
void load_sprites(void) {
	test_spritesheet = C2D_SpriteSheetLoad("romfs:/gfx/test_sprites.t3x");
	run_char_anim = C2D_SpriteSheetLoad("romfs:/gfx/run_char_anim.t3x");
	if (!test_spritesheet) {
		debug_printf("Failed to load test_spritesheet spritesheet");
	}
	if (!run_char_anim) {
		debug_printf("Failed to load run_char_anim spritesheet");
	}
}