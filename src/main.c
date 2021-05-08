#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "audio.h"
#include "beatmap.h"
#include "common.h"
#include "debug.h"
#include "director.h"
#include "logic.h"
#include "scene.h"

C2D_SpriteSheet test_spritesheet;
C2D_SpriteSheet run_char_anim;

void main_loop(void) {
  while (aptMainLoop()) {
    hidScanInput();

    /* Begin frame */
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

    if (!director_main_loop()) {
      return;
    }

    /* End frame */
    C2D_Flush();
    C3D_FrameEnd(0);
  }
}

int main() {
  /* Enable N3DS 804MHz operation, where available */
  osSetSpeedupEnable(true);

  /* Initialize ROM filesystem */
  romfsInit();

  /* Initialize debug subsystems */
#ifdef DEBUG_LOG
  init_debug_log();
#endif
  printf("Debug log is enabled.\n");

  /* Initialize screen targets */
  gfxInitDefault();
  C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
  C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
  C2D_Prepare();

  top_left = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
#ifdef DEBUG_CONSOLE
  consoleInit(GFX_BOTTOM, NULL);
#else
  bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
#endif

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