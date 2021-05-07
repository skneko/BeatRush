#include "scene.h"

#include "common.h"
#include "audio.h"
#include "logic.h"
#include "draw.h"
#include "director.h"
#include "stdlib.h"
#include "player.h"

#define DEPTH_BG					 0.0
#define DEPTH_BG_BUILDINGS			 0.01
#define DEPTH_DECOR_BIRDS			 0.05
#define DEPTH_FG_BUILDINGS			 0.08
#define DEPTH_ROAD					 0.1
#define DEPTH_NOTES					 0.21
#define DEPTH_PLAYER				 0.5
#define DEPTH_UI_SCORE			     0.7
#define DEPTH_UI_COMBO				 0.7
#define DEPTH_UI_HEALTH				 0.7
#define DEPTH_UI_OVER                0.79
#define DEPTH_PAUSE_MENU             0.8
#define DEPTH_DEBUG_BASE			 0.9
#define DEPTH_DEBUG_RULERS			 0.98

#define SCORE_LABEL_BUF_SIZE         13
#define COMBO_LABEL_BUF_SIZE         13
#define REST_TIME_LABEL_BUF_SIZE     13
#define PAUSE_LABEL_BUF_SIZE         13
#define DYN_TEXT_BUF_SIZE            4096

#define COMBO_DRAW_THRESHOLD         5
#define COMBO_BASE_COLOR             C2D_WHITE
#define COMBO_HIGHLIGHT_THRESHOLD    100
#define COMBO_HIGHLIGHT_COLOR        C2D_ORANGE
#define COMBO_FULL_COLOR             C2D_ORANGERED

#define ATTENTION_REST_THRESHOLD     4000
#define ATTENTION_WARN_THRESHOLD     2500
#define ATTENTION_WARN_MARGIN_X      20
#define ATTENTION_WARN_MARGIN_Y      20
#define ATTENTION_WARN_BASE          10
#define ATTENTION_WARN_HEIGHT        14
#define ATTENTION_WARN_PERIOD        210
#define ATTENTION_WARN_VISIBLE       100

// *** SPRITES ***
#define MAX_CHAR_SPRITES             1
#define MAX_NOTE_SPRITES             50
#define MAX_BG_SPRITES               35
#define MAX_HIT_EVAL_SPRITES         2 //0	-	DOWN,	1	-	UP
#define MAX_BULLSEYE_SPRITES         2 //0	-	DOWN,	1	-	UP
#define MAX_HEART_ICONS				 2

//bg_buildings
#define BG_BUILDINGS_SPEED           -1
#define BG_BUILD_MIN_HEIGHT          160 //ABSOLUTE
#define BG_BUILD_MAX_HEIGHT          110 //RELATIVE

//fg_buildings
#define FG_BUILDINGS_SPEED           -1
#define FG_BUILD_MIN_HEIGHT          100 //ABSOLUTE
#define FG_BUILD_MAX_HEIGHT          100 //RELATIVE
#define FG_DISTANCE_BW_BUILDINGS     10 //10 base, more if random

// Health icons
#define IDX_HEART_FULL				 0
#define IDX_HEART_EMPTY				 1
#define SHEET_IDX_HEART_FULL		 0
#define SHEET_IDX_HEART_EMPTY	 	 1
#define HEART_AREA_X				 40
#define HEART_AREA_Y				 200
#define HEART_ICON_STRIDE			 20

// hit evaluation
#define IDX_HIT_EVAL_TOP			 1
#define IDX_HIT_EVAL_BOTTOM			 0
#define HIT_EVAL_TIME_ALIVE			 300
#define HIT_EVAL_SCROLL_SPEED		 0.05
#define HIT_EVAL_BOTTOM_X			 10
#define HIT_EVAL_BOTTOM_Y 			 120
#define HIT_EVAL_TOP_X				 10
#define HIT_EVAL_TOP_Y				 20

// ---

static Note *next_note_to_draw;
static unsigned int remaining_notes_to_draw;
static float speed;

static bool in_rest;

static C2D_Sprite char_sprites[MAX_CHAR_SPRITES];
static C2D_Sprite note_sprites[MAX_NOTE_SPRITES];
static C2D_Sprite bg_sprites[MAX_BG_SPRITES];
static C2D_Sprite hit_eval_sprites[MAX_HIT_EVAL_SPRITES];
static C2D_Sprite bullseye_sprites[MAX_BULLSEYE_SPRITES];
static C2D_Image heart_icons[MAX_HEART_ICONS];

static C2D_SpriteSheet char_sprite_sheet;
static C2D_SpriteSheet note_sprite_sheet;
static C2D_SpriteSheet bg_sprite_sheet;
static C2D_SpriteSheet ui_sprite_sheet;

static C2D_Font font;

static int frame;

static int w; //since all fg buildings have different widths I need a variable to see where to put the next sprite
static int bird_dir;

static C2D_TextBuf dynamic_text_buf;

static void init_sprites(void) {
//load sheets from gfx
	char_sprite_sheet = load_sprite_sheet("romfs:/gfx/run_char_anim.t3x");
	note_sprite_sheet = load_sprite_sheet("romfs:/gfx/note.t3x");
	bg_sprite_sheet = load_sprite_sheet("romfs:/gfx/bg.t3x");
	ui_sprite_sheet = load_sprite_sheet("romfs:/gfx/ui.t3x");

	//------------------------------------------------------------------------------------------------
	//init char sprite to default state
	C2D_Sprite *player_sprite = &char_sprites[0]; //the sprite for the bg skybox, give or take you know what I mean
	C2D_SpriteFromSheet(player_sprite, char_sprite_sheet, 0);
	C2D_SpriteSetCenter(player_sprite, .5f, .5f);
	C2D_SpriteSetDepth(player_sprite, DEPTH_PLAYER);

	//------------------------------------------------------------------------------------------------
	//init bg sprite to default state (lmao OK)
	C2D_Sprite *bg_sprite = &bg_sprites[0]; //the sprite for the bg skybox, give or take you know what I mean
	C2D_SpriteFromSheet(bg_sprite, bg_sprite_sheet, 0);
	C2D_SpriteSetCenter(bg_sprite, .5f, .5f);
	C2D_SpriteSetPos(bg_sprite, TOP_SCREEN_WIDTH / 2, TOP_SCREEN_HEIGHT / 2);
	C2D_SpriteSetDepth(bg_sprite, DEPTH_BG);

	//there are 11 bg buildings to place (1 - 11)
	for (int i = 1; i < 12; i++) {
		C2D_Sprite *bg_building_sprite = &bg_sprites[i]; //the sprite for the bg building
		C2D_SpriteFromSheet(bg_building_sprite, bg_sprite_sheet, 1);
		C2D_SpriteSetPos(bg_building_sprite, (i - 1) * 40, BG_BUILD_MIN_HEIGHT - (rand() % BG_BUILD_MAX_HEIGHT));
		C2D_SpriteSetDepth(bg_building_sprite, DEPTH_BG_BUILDINGS);
	}
	frame = 0;

	//there are 17 fg buildings to place (maximum)
	w = 0;
	for (int i = 12; i < 29; i++) {
		C2D_Sprite *fg_building_sprite = &bg_sprites[i]; //the sprite for the fg building
		C2D_SpriteFromSheet(fg_building_sprite, bg_sprite_sheet, 2 + (rand() % 3));
		C2D_SpriteSetPos(fg_building_sprite, w, FG_BUILD_MIN_HEIGHT - (rand() % FG_BUILD_MAX_HEIGHT));
		C2D_SpriteSetDepth(fg_building_sprite, DEPTH_FG_BUILDINGS);
		w += fg_building_sprite->params.pos.w + FG_DISTANCE_BW_BUILDINGS;
	}

	//road
	C2D_Sprite *road = &bg_sprites[29];
	C2D_SpriteFromSheet(road, bg_sprite_sheet, 5);
	C2D_SpriteScale(road, 1, .7f);
	C2D_SpriteSetCenter(road, .5f, .5f);
	C2D_SpriteSetPos(road, TOP_SCREEN_WIDTH / 2, 200);
	C2D_SpriteSetDepth(road, DEPTH_ROAD);

	//let's draw a cute lil birdo friend owo
	C2D_Sprite *birdo = &bg_sprites[30];
	C2D_SpriteFromSheet(birdo, bg_sprite_sheet, 6);
	C2D_SpriteSetCenter(birdo, .5f, .5f);
	C2D_SpriteSetPos(birdo, 310, 50);
	C2D_SpriteSetDepth(birdo, DEPTH_DECOR_BIRDS);

	C2D_Sprite *birdo_2 = &bg_sprites[31];
	C2D_SpriteFromSheet(birdo_2, bg_sprite_sheet, 6);
	C2D_SpriteSetCenter(birdo_2, .5f, .5f);
	C2D_SpriteSetPos(birdo_2, 318, 55);
	C2D_SpriteSetDepth(birdo_2, DEPTH_DECOR_BIRDS);
	bird_dir = -1;

	//------------------------------------------------------------------------------------------------
	//init note sprites someway somehow yipee
	for (int i = 0; i < MAX_NOTE_SPRITES; i++) {
		C2D_Sprite *note_sprite = &note_sprites[i]; //initialize the note sprites
		C2D_SpriteFromSheet(note_sprite, note_sprite_sheet, 0);
		C2D_SpriteSetCenter(note_sprite, .5f, .5f);
		C2D_SpriteSetPos(note_sprite, 0, 0);
		//C2D_SpriteScale(note_sprite, 1.5f, 1.5f);
		C2D_SpriteSetDepth(note_sprite, DEPTH_NOTES);
	}

	heart_icons[IDX_HEART_FULL] = C2D_SpriteSheetGetImage(ui_sprite_sheet, SHEET_IDX_HEART_FULL);
	heart_icons[IDX_HEART_EMPTY] = C2D_SpriteSheetGetImage(ui_sprite_sheet, SHEET_IDX_HEART_EMPTY);

	//-----------------------------------------------------
	//hit evals
	C2D_Sprite *bot_eval_sprite = &hit_eval_sprites[0];
	C2D_SpriteFromSheet(bot_eval_sprite, ui_sprite_sheet, 2);
	C2D_SpriteSetDepth(bot_eval_sprite, DEPTH_UI_SCORE);

	C2D_Sprite *top_eval_sprite = &hit_eval_sprites[1];
	C2D_SpriteFromSheet(top_eval_sprite, ui_sprite_sheet, 2);
	C2D_SpriteSetDepth(top_eval_sprite, DEPTH_UI_SCORE);

	//bullseye
	C2D_Sprite *bot_bullseye_sprite = &bullseye_sprites[0];
	C2D_SpriteFromSheet(bot_bullseye_sprite, ui_sprite_sheet, 7);
	C2D_SpriteSetCenter(bot_bullseye_sprite, .5f, .5f);
	C2D_SpriteSetPos(bot_bullseye_sprite, HITLINE_LEFT_MARGIN, TOP_SCREEN_HEIGHT - LANE_BOTTOM_MARGIN - LANE_HEIGHT / 2);
	C2D_SpriteSetDepth(bot_bullseye_sprite, DEPTH_UI_SCORE);

	C2D_Sprite *top_bullseye_sprite = &bullseye_sprites[1];
	C2D_SpriteFromSheet(top_bullseye_sprite, ui_sprite_sheet, 6);
	C2D_SpriteSetCenter(top_bullseye_sprite, .5f, .5f);
	C2D_SpriteSetPos(top_bullseye_sprite, HITLINE_LEFT_MARGIN, LANE_TOP_MARGIN + LANE_HEIGHT / 2);
	C2D_SpriteSetDepth(top_bullseye_sprite, DEPTH_UI_SCORE);
}

void scene_init(void) {
	director_set_audio_dt(true);
	logic_init();

	next_note_to_draw = beatmap->notes;
	remaining_notes_to_draw = beatmap->note_count;
	speed = beatmap->approach_time / (TOP_SCREEN_WIDTH - HITLINE_LEFT_MARGIN);

	in_rest = true;

	dynamic_text_buf = C2D_TextBufNew(DYN_TEXT_BUF_SIZE);
	font = C2D_FontLoad("romfs:/fonts/The_Impostor.bcfnt");

	init_sprites();
	player_init();

	audioPlay();
}

void scene_end(void) {
	director_set_audio_dt(false);
	logic_end();
	
	player_end();

	C2D_FontFree(font);
	C2D_TextBufDelete(dynamic_text_buf);
}

typedef enum _NoteDrawingResult {
	NOTE_DRAWN,
	NOTE_BEHIND,
	NOTE_AHEAD
} NoteDrawingResult;

static NoteDrawingResult draw_note(const Note *const note, int noteId) {
	long long int note_remaining_time = (long long int)note->position - audioPlaybackPosition();
	float note_x = (float)note_remaining_time / speed + HITLINE_LEFT_MARGIN;

	if (note_x < -LANE_HEIGHT) {
		return NOTE_BEHIND;
	}
	if (note_x > TOP_SCREEN_WIDTH) {
		return NOTE_AHEAD;
	}

	int offset = 0;
	float lane_y;
	if (note->topLane) {
		lane_y = LANE_TOP_MARGIN + NOTE_RADIUS + NOTE_MARGIN;
		offset = 2;
	} else {
		lane_y = TOP_SCREEN_HEIGHT - LANE_BOTTOM_MARGIN - LANE_HEIGHT + NOTE_RADIUS + NOTE_MARGIN;
	}

	if (!note->hidden) {
		C2D_Sprite *note_sprite = &note_sprites[noteId];
		C2D_SpriteSetPos(note_sprite, floor(note_x), floor(lane_y));
		note_sprite->image = C2D_SpriteSheetGetImage(note_sprite_sheet, (frame % 24 < 12) ? offset : offset + 1); //animate
		C2D_DrawSprite(note_sprite);

#ifdef DEBUG_OVERLAY
		C2D_DrawLine(note_x, lane_y - NOTE_RADIUS, C2D_RED,
					 note_x, lane_y + NOTE_RADIUS, C2D_RED,
					 3.0f, DEPTH_DEBUG_BASE);
#endif
	}

	return NOTE_DRAWN;
}

static void draw_notes(void) {
#ifdef DEBUG_NOTE_DRAWING
	printf("----------- %u\n", remaining_notes_to_draw);
#endif

	Note *note_to_draw = next_note_to_draw;
	unsigned int remaining_notes_temp = remaining_notes_to_draw;

	bool keep_advancing = true;
	int i = 0;
	while (keep_advancing && remaining_notes_temp > 0) {
		switch (draw_note(note_to_draw, i)) {
		case NOTE_BEHIND: {
#ifdef DEBUG_NOTE_DRAWING
			printf("Left behind note at %lums.\n", note_to_draw->position);
#endif
			remaining_notes_to_draw--;
			next_note_to_draw++;
			break;
		}

		case NOTE_DRAWN: {
#ifdef DEBUG_NOTE_DRAWING
			printf("Drawn note at %lums.\n", note_to_draw->position);
#endif
			break;
		}

		default:
		case NOTE_AHEAD: {
#ifdef DEBUG_NOTE_DRAWING
			printf("Found note ahead at %lums, stop.\n", note_to_draw->position);
#endif
			keep_advancing = false;
			break;
		}
		}

		remaining_notes_temp--;
		note_to_draw++;
		i++;
	}
}

static void draw_score(void) {
	C2D_Text score_label;
	char buf[SCORE_LABEL_BUF_SIZE];

	unsigned long score = logic_score();

	snprintf(buf, sizeof(buf), "%06lu", score);
	prepare_text_with_font(buf, &score_label, font, dynamic_text_buf);
	C2D_DrawText(
		&score_label, C2D_WithColor | C2D_AtBaseline | C2D_AlignRight,
		390, 15, DEPTH_UI_SCORE, .35f, .35f,
		C2D_WHITE);
}

static void draw_combo(void) {
	C2D_Text combo_label;
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
		snprintf(buf, sizeof(buf), "%u", combo);
		prepare_text_with_font(buf, &combo_label, font, dynamic_text_buf);
		C2D_DrawText(
			&combo_label, C2D_WithColor | C2D_AtBaseline | C2D_AlignCenter,
			200.0f, 25.0f, DEPTH_UI_COMBO, .55f, .55f,
			color);
	}
}

static void draw_health(void) {
	unsigned int health = logic_current_health();
	unsigned int max_health = logic_max_health();

	unsigned int i = 0;
	for (; i < health; i++) {
		float x = HEART_AREA_X + HEART_ICON_STRIDE * i;
		C2D_DrawImageAt(heart_icons[IDX_HEART_FULL], x, HEART_AREA_Y, DEPTH_UI_HEALTH, NULL, 1, 1);
	}
	for (; i < max_health; i++) {
		float x = HEART_AREA_X + HEART_ICON_STRIDE * i;
		C2D_DrawImageAt(heart_icons[IDX_HEART_EMPTY], x, HEART_AREA_Y, DEPTH_UI_HEALTH, NULL, 1, 1);
	}
}

static void draw_attention_warnings(long long time_until_next) {
	if (flicker_is_visible(time_until_next, ATTENTION_WARN_PERIOD, ATTENTION_WARN_VISIBLE)) {
		float left_x = ATTENTION_WARN_MARGIN_X;
		float right_x = TOP_SCREEN_WIDTH - ATTENTION_WARN_MARGIN_X;
		float top_y = ATTENTION_WARN_MARGIN_Y;
		float bottom_y = TOP_SCREEN_HEIGHT - ATTENTION_WARN_MARGIN_Y;

		draw_sideways_triangle(left_x, top_y, ATTENTION_WARN_BASE, ATTENTION_WARN_HEIGHT, 1, 1, C2D_RED, DEPTH_UI_OVER);
		draw_sideways_triangle(right_x, top_y, ATTENTION_WARN_BASE, ATTENTION_WARN_HEIGHT, -1, 1, C2D_RED, DEPTH_UI_OVER);
		draw_sideways_triangle(left_x, bottom_y, ATTENTION_WARN_BASE, ATTENTION_WARN_HEIGHT, 1, -1, C2D_RED, DEPTH_UI_OVER);
		draw_sideways_triangle(right_x, bottom_y, ATTENTION_WARN_BASE, ATTENTION_WARN_HEIGHT, -1, -1, C2D_RED, DEPTH_UI_OVER);
	}
}

static void draw_attention_cues(void) {
	if (remaining_notes_to_draw > 1) {
		long long time_until_next = (long long)next_note_to_draw->position - audioPlaybackPosition();

		if (time_until_next > ATTENTION_REST_THRESHOLD) {
			in_rest = true;
		}

		if (in_rest) {
			if (time_until_next <= beatmap->approach_time) {
				in_rest = false;
				return;
			}

			C2D_Text rest_time_label;
			char buf[REST_TIME_LABEL_BUF_SIZE];

			if (time_until_next < ATTENTION_WARN_THRESHOLD) {
				draw_attention_warnings(time_until_next);
				snprintf(buf, sizeof(buf), "READY");
			} else {
				snprintf(buf, sizeof(buf), "%0.3f", time_until_next / 1000.0f);
			}

			prepare_text_with_font(buf, &rest_time_label, font, dynamic_text_buf);
			C2D_DrawText(
				&rest_time_label, C2D_WithColor | C2D_AtBaseline | C2D_AlignCenter,
				TOP_SCREEN_CENTER_HOR, TOP_SCREEN_CENTER_VER + 5, DEPTH_UI_OVER, 0.5f, 0.5f,
				C2D_WHITE);
		}
	}
}

static void draw_bg_sprites(void){
	//skybox
	C2D_Sprite *bg_sprite = &bg_sprites[0];
	C2D_DrawSprite(bg_sprite);
	//bg building
	for (int i = 1; i < 12; i++) {
		C2D_Sprite *bg_building_sprite = &bg_sprites[i];
		//if it's off screen we move it to the front
		if (bg_building_sprite->params.pos.x <= -40) {
			C2D_SpriteSetPos(bg_building_sprite,
							 TOP_SCREEN_WIDTH,
							 BG_BUILD_MIN_HEIGHT - (rand() % BG_BUILD_MAX_HEIGHT)); //40 because that's the building sprite's width
		}
		//to slow down let's only move the sprites every other frame
		if (frame % 4 == 0) {
			C2D_SpriteMove(bg_building_sprite, BG_BUILDINGS_SPEED, 0);
		}
		C2D_DrawSprite(bg_building_sprite);
	}

	//birdo
	C2D_Sprite *birdo = &bg_sprites[30];
	if (frame % 40 == 0) {
		C2D_SpriteMove(birdo, -1, 0);               //movement in x
	}
	if (frame % 77 == 0) {
		C2D_SpriteMove(birdo, 0, bird_dir);               //movement in y
	}
	if (frame % 100 == 0) {
		birdo->image = C2D_SpriteSheetGetImage(bg_sprite_sheet, 7);                //flap
	}else if (frame % 100 == 30) {
		birdo->image = C2D_SpriteSheetGetImage(bg_sprite_sheet, 6);
	}
	C2D_DrawSprite(birdo);

	C2D_Sprite *birdo_2 = &bg_sprites[31];
	if (frame % 40 == 5) {
		C2D_SpriteMove(birdo_2, -1, 0);               //movement in x
	}
	if (frame % 77 == 5) {
		C2D_SpriteMove(birdo_2, 0, bird_dir);               //movement in y
	}
	if (frame % 100 == 5) {
		birdo_2->image = C2D_SpriteSheetGetImage(bg_sprite_sheet, 7);                //flap
	}else if (frame % 100 == 35) {
		birdo_2->image = C2D_SpriteSheetGetImage(bg_sprite_sheet, 6);
	}
	C2D_DrawSprite(birdo_2);

	if (birdo->params.pos.y < 5) {
		bird_dir = -bird_dir;
	}

	//fg building
	for (int i = 12; i < 29; i++) {
		C2D_Sprite *fg_building_sprite = &bg_sprites[i];
		if (fg_building_sprite->params.pos.x <= -fg_building_sprite->params.pos.w) {
			C2D_SpriteSetPos(fg_building_sprite,
							 w,
							 FG_BUILD_MIN_HEIGHT - (rand() % FG_BUILD_MAX_HEIGHT)); //40 because that's the building sprite's width
			w += fg_building_sprite->params.pos.w + FG_DISTANCE_BW_BUILDINGS;
		}
		if (frame % 3 == 0) {
			C2D_SpriteMove(fg_building_sprite, FG_BUILDINGS_SPEED, 0);
		}
		C2D_DrawSprite(fg_building_sprite);
	}
	if (frame % 3 == 0) {
		w += FG_BUILDINGS_SPEED;
	}
	//road
	C2D_Sprite *road = &bg_sprites[29];
	C2D_DrawSprite(road);

	//next frame
	frame += 1;
}

static void draw_pause(void) {
	C2D_Text pause_label;
	char buf[PAUSE_LABEL_BUF_SIZE];

	snprintf(buf, sizeof(buf), "PAUSE");
	prepare_text_with_font(buf, &pause_label, font, dynamic_text_buf);
	C2D_DrawText(
		&pause_label, C2D_WithColor | C2D_AlignCenter,
		TOP_SCREEN_CENTER_HOR, TOP_SCREEN_CENTER_VER - NOTE_RADIUS - 5, DEPTH_PAUSE_MENU, 1.5f, 1.5f,
		C2D_WHITE);
}

static void draw_failure(void) {
	C2D_Text failure_label;
	char buf[10];

	snprintf(buf, sizeof(buf), "GAME OVER");
	prepare_text_with_font(buf, &failure_label, font, dynamic_text_buf);
	C2D_DrawText(
		&failure_label, C2D_WithColor | C2D_AlignCenter,
		TOP_SCREEN_CENTER_HOR, TOP_SCREEN_CENTER_VER - NOTE_RADIUS - 5, DEPTH_PAUSE_MENU, 1.5f, 1.5f,
		C2D_RED);
}

static void draw_hit_popup(HitAssessment hit, C2D_Sprite *sprite, float x, float y) {
	unsigned long time_alive = audioPlaybackPosition() - hit.press_position;
	if(hit.valid && time_alive < HIT_EVAL_TIME_ALIVE){
		C2D_SpriteSetPos(sprite, x, y - (time_alive * HIT_EVAL_SCROLL_SPEED));

		switch (hit.valuation)
		{
		case HIT_VAL_PERFECT:
			sprite->image = C2D_SpriteSheetGetImage(ui_sprite_sheet, 2);
			break;

		case HIT_VAL_GOOD:
			sprite->image = C2D_SpriteSheetGetImage(ui_sprite_sheet, 3);
			break;

		case HIT_VAL_OK:
			sprite->image = C2D_SpriteSheetGetImage(ui_sprite_sheet, 4);
			break;

		case HIT_VAL_MISS:
			sprite->image = C2D_SpriteSheetGetImage(ui_sprite_sheet, 5);
			break;

		default:
			break;
		}

		C2D_DrawSprite(sprite);
	}
}

static void draw_hit_popups(void) {
	draw_hit_popup(logic_top_hit_assessment(), &hit_eval_sprites[IDX_HIT_EVAL_TOP], 
		HIT_EVAL_TOP_X, HIT_EVAL_TOP_Y);
	draw_hit_popup(logic_bottom_hit_assessment(), &hit_eval_sprites[IDX_HIT_EVAL_BOTTOM], 
		HIT_EVAL_BOTTOM_X, HIT_EVAL_BOTTOM_Y);
}

static void draw_bullseyes(void){
	C2D_Sprite *bot_bullseye_sprite = &bullseye_sprites[0];
	C2D_DrawSprite(bot_bullseye_sprite);

	C2D_Sprite *top_bullseye_sprite = &bullseye_sprites[1];
	C2D_DrawSprite(top_bullseye_sprite);
}

static void draw_progress_bar(void) {
	float progress = (float)audioPlaybackPosition() / audioLength();
	
	u32 color = C2D_Color32(128, 128, 128, 128);
	C2D_DrawRectangle(
		0, TOP_SCREEN_HEIGHT - 3,
		DEPTH_UI_OVER,
		TOP_SCREEN_WIDTH * progress, 3,
		color, color, color, color);
}

#ifdef DEBUG_OVERLAY
static void draw_debug_song_time(void) {
	C2D_Text song_time_label;
	char buf[10];

	snprintf(buf, sizeof(buf), "%05ld", audioPlaybackPosition());
	prepare_text_with_font(buf, &song_time_label, NULL, dynamic_text_buf);
	C2D_DrawText(
		&song_time_label, C2D_WithColor | C2D_AtBaseline,
		230.0f, 220.0f, DEPTH_DEBUG_BASE, 0.5f, 0.5f,
		C2D_WHITE);
}

static void draw_debug_rulers(void) {
	// top lane
	C2D_DrawLine(
		0, LANE_TOP_MARGIN, C2D_GREEN,
		TOP_SCREEN_WIDTH, LANE_TOP_MARGIN, C2D_GREEN,
		1.0f, DEPTH_DEBUG_RULERS);
	C2D_DrawLine(
		0, LANE_TOP_MARGIN + LANE_HEIGHT, C2D_GREEN,
		TOP_SCREEN_WIDTH, LANE_TOP_MARGIN + LANE_HEIGHT, C2D_GREEN,
		1.0f, DEPTH_DEBUG_RULERS);

	// bottom lane
	C2D_DrawLine(
		0, TOP_SCREEN_HEIGHT - LANE_BOTTOM_MARGIN, C2D_GREEN,
		TOP_SCREEN_WIDTH, TOP_SCREEN_HEIGHT - LANE_BOTTOM_MARGIN, C2D_GREEN,
		1.0f, DEPTH_DEBUG_RULERS);
	C2D_DrawLine(
		0, TOP_SCREEN_HEIGHT - LANE_BOTTOM_MARGIN - LANE_HEIGHT, C2D_GREEN,
		TOP_SCREEN_WIDTH, TOP_SCREEN_HEIGHT - LANE_BOTTOM_MARGIN - LANE_HEIGHT, C2D_GREEN,
		1.0f, DEPTH_DEBUG_RULERS);

	// hitline
	C2D_DrawLine(
		HITLINE_LEFT_MARGIN, 0.0f, C2D_WHITE,
		HITLINE_LEFT_MARGIN, TOP_SCREEN_HEIGHT, C2D_WHITE,
		1.0f, DEPTH_DEBUG_RULERS);

	// hitpoints
	C2D_DrawLine(
		HITLINE_LEFT_MARGIN - 5.0f, LANE_TOP_MARGIN + LANE_HEIGHT / 2, C2D_GREEN,
		HITLINE_LEFT_MARGIN + 5.0f, LANE_TOP_MARGIN + LANE_HEIGHT / 2, C2D_GREEN,
		3.0f, DEPTH_DEBUG_RULERS);
	C2D_DrawLine(
		HITLINE_LEFT_MARGIN - 5.0f, TOP_SCREEN_HEIGHT - LANE_BOTTOM_MARGIN - LANE_HEIGHT / 2, C2D_GREEN,
		HITLINE_LEFT_MARGIN + 5.0f, TOP_SCREEN_HEIGHT - LANE_BOTTOM_MARGIN - LANE_HEIGHT / 2, C2D_GREEN,
		3.0f, DEPTH_DEBUG_RULERS);
}

static void draw_debug_keypress_hint(bool top_lane) {
	float lane_y = (top_lane ? LANE_TOP_MARGIN : TOP_SCREEN_HEIGHT - LANE_BOTTOM_MARGIN - LANE_HEIGHT) + LANE_HEIGHT / 2;

	C2D_DrawCircleSolid(
		HITLINE_LEFT_MARGIN, lane_y, DEPTH_DEBUG_BASE, NOTE_RADIUS / 2,
		C2D_ORANGE);
}

static void draw_debug_invincibility_hint(void) {
	if (logic_is_invencible()) {
		C2D_Text invincibility_hint_label;

		const char *message = "INV";
		prepare_text_with_font(message, &invincibility_hint_label, NULL, dynamic_text_buf);
		C2D_DrawText(
			&invincibility_hint_label, C2D_WithColor | C2D_AtBaseline | C2D_AlignCenter,
			50, 230, DEPTH_DEBUG_BASE, 0.5f, 0.5f,
			C2D_BLUE);
	}
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

	draw_debug_rulers();

	draw_debug_keypress_hints();
	draw_debug_invincibility_hint();
}
#endif

#ifdef DEBUG_AUTO
static void draw_debug_auto_hint(void) {
	C2D_Text auto_hint_label;

	const char *message = "AUTO";
	prepare_text_with_font(message, &auto_hint_label, NULL, dynamic_text_buf);
	C2D_DrawText(
		&auto_hint_label, C2D_WithColor | C2D_AtBaseline | C2D_AlignCenter,
		TOP_SCREEN_CENTER_HOR, 50, DEPTH_DEBUG_BASE, 0.6, 0.6,
		C2D_RED);
}
#endif

void scene_draw(void) {
	draw_bg_sprites();
	draw_notes();
	player_draw(&char_sprites[0], char_sprite_sheet);

	draw_score();
	draw_combo();
	draw_health();
	draw_hit_popups();
	draw_bullseyes();
	draw_progress_bar();

	if (logic_has_failed()) {
		draw_failure();
	} else if (audioIsPaused()) {
		draw_pause();
	}

	draw_attention_cues();

#ifdef DEBUG_OVERLAY
	draw_debug_overlay();
#endif

#ifdef DEBUG_AUTO
	draw_debug_auto_hint();
#endif
}