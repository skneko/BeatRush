#include "scene.h"

#include "common.h"
#include "audio.h"
#include "logic.h"
#include "stdlib.h"

static Beatmap *beatmap;
static Note *first_note_to_draw;
static unsigned int remaining_notes_to_draw;
static float speed;

#define SCORE_LABEL_BUF_SIZE        13
#define COMBO_LABEL_BUF_SIZE        13

#define COMBO_DRAW_THRESHOLD        5
#define COMBO_BASE_COLOR            C2D_WHITE
#define COMBO_HIGHLIGHT_THRESHOLD   100
#define COMBO_HIGHLIGHT_COLOR       C2D_ORANGE
#define COMBO_FULL_COLOR            C2D_ORANGERED

C2D_TextBuf songTimeLabelBuf;
C2D_TextBuf scoreLabelBuf;
C2D_TextBuf comboLabelBuf;

//SPRITES
#define MAX_CHAR_SPRITES    1  //SUGGESTIVE     ###- NOT FINAL -###
#define MAX_NOTE_SPRITES    50   //SUGGESTIVE     ###- NOT FINAL -###
#define MAX_BG_SPRITES      35  //SUGGESTIVE     ###- NOT FINAL -###
C2D_Sprite char_sprites[MAX_CHAR_SPRITES];
C2D_Sprite note_sprites[MAX_NOTE_SPRITES];
C2D_Sprite bg_sprites[MAX_BG_SPRITES];
C2D_SpriteSheet char_sprite_sheet;
C2D_SpriteSheet note_sprite_sheet;
C2D_SpriteSheet bg_sprite_sheet;
//--------------
int frame;
//--------------
//bg_buildings
#define BG_BUILDINGS_SPEED  -1
#define BG_BUILD_MIN_HEIGHT 160 //ABSOLUTE
#define BG_BUILD_MAX_HEIGHT 110 //RELATIVE
//fg_buildings
#define FG_BUILDINGS_SPEED  -1
#define FG_BUILD_MIN_HEIGHT 100 //ABSOLUTE
#define FG_BUILD_MAX_HEIGHT 100 //RELATIVE
#define FG_DISTANCE_BW_BUILDINGS 10 //10 base, more if random
int w; //since all fg buildings have different widths I need a variable to see where to put the next sprite
int bird_dir;

void scene_init(Beatmap *const _beatmap) {
    beatmap = _beatmap;
    first_note_to_draw = beatmap->notes;
    remaining_notes_to_draw = beatmap->note_count;
    speed = beatmap->approach_time / (TOP_SCREEN_WIDTH - HITLINE_LEFT_MARGIN);

    songTimeLabelBuf = C2D_TextBufNew(10);
    scoreLabelBuf = C2D_TextBufNew(SCORE_LABEL_BUF_SIZE);
    comboLabelBuf = C2D_TextBufNew(COMBO_LABEL_BUF_SIZE);

    //############ SPRITE LOADER ##################

    //load sheets from gfx
    char_sprite_sheet = C2D_SpriteSheetLoad("romfs:/gfx/run_char_anim.t3x"); //char
    if (!char_sprite_sheet) svcBreak(USERBREAK_PANIC);
    note_sprite_sheet = C2D_SpriteSheetLoad("romfs:/gfx/note.t3x"); //note  ### TO DRAW ###
    if (!note_sprite_sheet) svcBreak(USERBREAK_PANIC);
    bg_sprite_sheet = C2D_SpriteSheetLoad("romfs:/gfx/bg.t3x"); //bg        ### TO DRAW ###
    if (!bg_sprite_sheet) svcBreak(USERBREAK_PANIC);

    //------------------------------------------------------------------------------------------------
    //init char sprite to default state
    C2D_Sprite* debug_player_sprite = &char_sprites[0]; //the sprite for the bg skybox, give or take you know what I mean
    C2D_SpriteFromSheet(debug_player_sprite, char_sprite_sheet, 0); 
    C2D_SpriteSetCenter(debug_player_sprite, .5f, .5f);
    C2D_SpriteSetPos(debug_player_sprite, HITLINE_LEFT_MARGIN, TOP_SCREEN_HEIGHT - LANE_BOTTOM_MARGIN - LANE_HEIGHT / 2);
    C2D_SpriteSetDepth(debug_player_sprite, .9f);
    C2D_SpriteScale(debug_player_sprite, 2, 2);

    //------------------------------------------------------------------------------------------------
    //init bg sprite to default state (lmao OK)
    C2D_Sprite* bg_sprite = &bg_sprites[0]; //the sprite for the bg skybox, give or take you know what I mean
    C2D_SpriteFromSheet(bg_sprite, bg_sprite_sheet, 0); 
    C2D_SpriteSetCenter(bg_sprite, .5f, .5f);
    C2D_SpriteSetPos(bg_sprite, TOP_SCREEN_WIDTH / 2, TOP_SCREEN_HEIGHT / 2);
    C2D_SpriteSetDepth(bg_sprite, 0);

    //there are 11 bg buildings to place (1 - 11)
    for(int i = 1; i < 12; i++){
        C2D_Sprite* bg_building_sprite = &bg_sprites[i]; //the sprite for the bg building
        C2D_SpriteFromSheet(bg_building_sprite, bg_sprite_sheet, 1); 
        C2D_SpriteSetPos(bg_building_sprite, (i - 1) * 40, BG_BUILD_MIN_HEIGHT - (rand() % BG_BUILD_MAX_HEIGHT));
        C2D_SpriteSetDepth(bg_building_sprite, .1f);
    }
    frame = 0;

    //there are 17 fg buildings to place (maximum)
    w = 0;
    for(int i = 12; i < 29; i++){
        C2D_Sprite* fg_building_sprite = &bg_sprites[i]; //the sprite for the fg building
        C2D_SpriteFromSheet(fg_building_sprite, bg_sprite_sheet, 2 + (rand()%3));
        C2D_SpriteSetPos(fg_building_sprite, w, FG_BUILD_MIN_HEIGHT - (rand() % FG_BUILD_MAX_HEIGHT));
        C2D_SpriteSetDepth(fg_building_sprite, .3f);
        w += fg_building_sprite->params.pos.w + FG_DISTANCE_BW_BUILDINGS;
    }

    //road TEMPORARY!!!!
    C2D_Sprite* road = &bg_sprites[29];
    C2D_SpriteFromSheet(road, bg_sprite_sheet, 5);
    C2D_SpriteScale(road, 1, .7f); 
    C2D_SpriteSetCenter(road, .5f, .5f);
    C2D_SpriteSetPos(road, TOP_SCREEN_WIDTH / 2, 200);
    C2D_SpriteSetDepth(road, .4f);

    //let's draw a cute lil birdo friend owo
    C2D_Sprite* birdo = &bg_sprites[30];
    C2D_SpriteFromSheet(birdo, bg_sprite_sheet, 6); 
    C2D_SpriteSetCenter(birdo, .5f, .5f);
    //C2D_SpriteScale(birdo, 5, 5); //I put this here for debugging, the sprite is crazy small
    C2D_SpriteSetPos(birdo, 310, 50);
    C2D_SpriteSetDepth(birdo, .2f);

    C2D_Sprite* birdo_2 = &bg_sprites[31];
    C2D_SpriteFromSheet(birdo_2, bg_sprite_sheet, 6); 
    C2D_SpriteSetCenter(birdo_2, .5f, .5f);
    //C2D_SpriteScale(birdo, 5, 5); //I put this here for debugging, the sprite is crazy small
    C2D_SpriteSetPos(birdo_2, 318, 55);
    C2D_SpriteSetDepth(birdo_2, .2f);
    bird_dir = -1;

    //------------------------------------------------------------------------------------------------
    //init note sprites someway somehow yipee
    for(int i = 0; i < MAX_NOTE_SPRITES; i++){
        C2D_Sprite* note_sprite = &note_sprites[i]; //initialize the note sprites
        C2D_SpriteFromSheet(note_sprite, note_sprite_sheet, 0); 
        C2D_SpriteSetCenter(note_sprite, .5f, .5f);
        C2D_SpriteSetPos(note_sprite, 0, 0);
        //C2D_SpriteScale(note_sprite, 1.5f, 1.5f);
        C2D_SpriteSetDepth(note_sprite, .5f);
    }
}

void scene_end(void) {
    C2D_TextBufDelete(songTimeLabelBuf);
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

    // dibujar nota SUSTITUIR POR SPRITE    
    C2D_Sprite* note_sprite = &note_sprites[noteId];
    C2D_SpriteSetPos(note_sprite, floor(note_x), floor(lane_y));
    note_sprite->image = C2D_SpriteSheetGetImage(note_sprite_sheet, (frame%24 < 12) ? offset : offset + 1); //animate
    C2D_DrawSprite(note_sprite);
    //C2D_DrawCircleSolid(
    //    note_x, lane_y, DEBUG_DEPTH, NOTE_RADIUS,
    //    C2D_PURPLE);
    C2D_DrawLine(note_x, lane_y - NOTE_RADIUS, C2D_RED,
        note_x, lane_y + NOTE_RADIUS, C2D_RED,
        1.0f, DEBUG_DEPTH + 1);

    return NOTE_DRAWN;
}

static void draw_notes(void) {
    //printf("----------- %u\n", remaining_notes_to_draw);

    Note *note_to_draw = first_note_to_draw;
    unsigned int remaining_notes_temp = remaining_notes_to_draw;

    bool keep_advancing = true;
    int i = 0;
    while (keep_advancing && remaining_notes_temp > 0) {
        switch (draw_note(note_to_draw, i)) {
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
        i++;
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
    //dibujar texto ########################### PUNTUACION ######
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
        //dibujar texto ############################## COMBO ##############
        C2D_DrawText(
            &comboLabel, C2D_WithColor | C2D_AtBaseline | C2D_AlignCenter, 
            200.0f, 25.0f, 0.0f, 1.0f, 1.0f, 
            color);
    }
}

static void draw_player_sprite(void){
    //SOMETHING HERE
    //player debug sprite
    C2D_Sprite* debug_player_sprite = &char_sprites[0];
    C2D_DrawSprite(debug_player_sprite);
}

static void draw_bg_sprites(void){
    //SOMETHING HERE
    //skybox
    C2D_Sprite* bg_sprite = &bg_sprites[0];
    C2D_DrawSprite(bg_sprite);
    //bg building
    for(int i = 1; i < 12; i++){
        C2D_Sprite* bg_building_sprite = &bg_sprites[i];
        //if it's off screen we move it to the front
        if(bg_building_sprite->params.pos.x <= -40) {
            C2D_SpriteSetPos(bg_building_sprite, 
                TOP_SCREEN_WIDTH, 
                BG_BUILD_MIN_HEIGHT - (rand() % BG_BUILD_MAX_HEIGHT)); //40 because that's the building sprite's width
        }
        //to slow down let's only move the sprites every other frame
        if(frame%4 == 0) C2D_SpriteMove(bg_building_sprite, BG_BUILDINGS_SPEED, 0);        
        C2D_DrawSprite(bg_building_sprite);
    }
    //fg building
    for(int i = 12; i < 29; i++){
        C2D_Sprite* fg_building_sprite = &bg_sprites[i];
        if(fg_building_sprite->params.pos.x <= -fg_building_sprite->params.pos.w) {
            C2D_SpriteSetPos(fg_building_sprite, 
                w, 
                FG_BUILD_MIN_HEIGHT - (rand() % FG_BUILD_MAX_HEIGHT)); //40 because that's the building sprite's width
            w += fg_building_sprite->params.pos.w + FG_DISTANCE_BW_BUILDINGS;
        }
        if(frame%3 == 0) C2D_SpriteMove(fg_building_sprite, FG_BUILDINGS_SPEED, 0);
        C2D_DrawSprite(fg_building_sprite);
    } if(frame%3 == 0) w += FG_BUILDINGS_SPEED;
    //road TEMPORARY
    C2D_Sprite* road = &bg_sprites[29];
    C2D_DrawSprite(road);
    //birdo
    C2D_Sprite* birdo = &bg_sprites[30];
    if(frame%40 == 0) C2D_SpriteMove(birdo, -1, 0); //movement in x
    if(frame%77 == 0) C2D_SpriteMove(birdo, 0, bird_dir); //movement in y
    if(frame%100 == 0) birdo->image = C2D_SpriteSheetGetImage(bg_sprite_sheet, 7); //flap
    else if(frame%100 == 30) birdo->image = C2D_SpriteSheetGetImage(bg_sprite_sheet, 6);
    C2D_DrawSprite(birdo);

    C2D_Sprite* birdo_2 = &bg_sprites[31];
    if(frame%40 == 5) C2D_SpriteMove(birdo_2, -1, 0); //movement in x
    if(frame%77 == 5) C2D_SpriteMove(birdo_2, 0, bird_dir); //movement in y
    if(frame%100 == 5) birdo_2->image = C2D_SpriteSheetGetImage(bg_sprite_sheet, 7); //flap
    else if(frame%100 == 35) birdo_2->image = C2D_SpriteSheetGetImage(bg_sprite_sheet, 6);
    C2D_DrawSprite(birdo_2);

    if (birdo->params.pos.y < 5) bird_dir = -bird_dir;

    //next frame
    frame += 1;
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
    draw_player_sprite();
    draw_bg_sprites();
    draw_notes();
    draw_score();
    draw_combo();

    draw_debug_overlay();
}