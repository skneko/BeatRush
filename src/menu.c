#include "common.h"
#include "menu.h"
#include "draw.h"
#include "director.h"
#include "audio.h"

#define DYN_TEXT_BUF_SIZE       4096

#define MENU_DEPTH              0.5f

#define MENU_MARGIN_LEFT        100
#define MENU_SELECTED_INDENT    50
#define MENU_OPTION_SEP         18
#define MENU_RADIUS_EFFECT      0.6f

static C2D_TextBuf dynamic_text_buf;

const char *options[] = {
	"[Easy] K/DA - POP/STARS",
	"[Norm] Haloweak - Bass Telekinesis",
	"[Hard] Memtrix - All You Are",
	"Quit"
};

int option_count = 4;
int selected_option;

void menu_init(void) {
	dynamic_text_buf = C2D_TextBufNew(DYN_TEXT_BUF_SIZE);

	selected_option = 0;
}

void draw_unselected_option(int i, float x, float y) {
	C2D_Text optionText;

	C2D_TextBufClear(dynamic_text_buf);
	C2D_TextParse(&optionText, dynamic_text_buf, options[i]);
	C2D_TextOptimize(&optionText);
	C2D_DrawText(
		&optionText, C2D_WithColor,
		x, y, MENU_DEPTH, 0.5f, 0.5f,
		C2D_WHITE);
}

void draw_options(void) {
	for (int i = selected_option + 1; i < option_count; i++) {
		int offset = i - selected_option;
		float x = MENU_MARGIN_LEFT + offset * offset * MENU_RADIUS_EFFECT;
		float y = TOP_SCREEN_CENTER_VER + offset * MENU_OPTION_SEP;

		if (y > TOP_SCREEN_HEIGHT) {
			break;
		}

		draw_unselected_option(i, x, y);
	}

	for (int i = selected_option - 1; i >= 0; i--) {
		int offset = selected_option - i;
		float x = MENU_MARGIN_LEFT + offset * offset * MENU_RADIUS_EFFECT;
		float y = TOP_SCREEN_CENTER_VER - offset * MENU_OPTION_SEP;

		if (y < 0) {
			break;
		}

		draw_unselected_option(i, x, y);
	}

	C2D_Text optionText;

	C2D_TextBufClear(dynamic_text_buf);
	C2D_TextParse(&optionText, dynamic_text_buf, options[selected_option]);
	C2D_TextOptimize(&optionText);
	C2D_DrawText(
		&optionText, C2D_WithColor,
		MENU_MARGIN_LEFT - MENU_SELECTED_INDENT, TOP_SCREEN_CENTER_VER, MENU_DEPTH, 0.5f, 0.5f,
		C2D_WHITE);

	draw_sideways_triangle(
		MENU_MARGIN_LEFT - MENU_SELECTED_INDENT - 20, TOP_SCREEN_CENTER_VER + 2,
		8, 10, 1, 1, C2D_RED, MENU_DEPTH);
}

void menu_draw(void) {
	draw_options();

	//C2D_DrawCircleSolid(
	//    TOP_SCREEN_WIDTH, TOP_SCREEN_CENTER_VER, 0.0f,
	//    TOP_SCREEN_WIDTH - MENU_MARGIN_LEFT,
	//    C2D_BLUE);
}

Beatmap *select_beatmap(void) {
	switch (selected_option) {
	case 0: {
		audioSetSong("romfs:/beatmaps/popStars/track.opus");
		return beatmap_load_from_file("romfs:/beatmaps/popStars/beatmap.btrm");
	}

	case 1: {
		audioSetSong("romfs:/beatmaps/bassTelekinesis/track.opus");
		return beatmap_load_from_file("romfs:/beatmaps/bassTelekinesis/beatmap.btrm");
	}

	case 2: {
		audioSetSong("romfs:/beatmaps/allYouAre/track.opus");
		return beatmap_load_from_file("romfs:/beatmaps/allYouAre/beatmap.btrm");
	}

	default: {
		director_request_quit();
		return NULL;
	}
	}
}

void menu_update(__attribute__((unused)) unsigned int dt) {
	u32 k_down = hidKeysDown();

	if (k_down & KEY_A) {
		beatmap = select_beatmap();             // FIXME
		director_change_state(RUNNING_BEATMAP);
	}

	if ((k_down & KEY_UP) | (k_down & KEY_CSTICK_UP)) {
		if (selected_option > 0) {
			selected_option = (selected_option - 1) % option_count;
		} else {
			selected_option = option_count - 1;
		}
	}
	if ((k_down & KEY_DOWN) | (k_down & KEY_CSTICK_DOWN)) {
		selected_option = (selected_option + 1) % option_count;
	}
}

void menu_end(void) {
	C2D_TextBufDelete(dynamic_text_buf);
}